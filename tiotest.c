/*
 *    Threaded io test
 *
 *  Copyright (C) 1999-2000 Mika Kuoppala <miku@iki.fi>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 *
 *
 */

#include "tiotest.h"
#include "crc32.h"

static const char* versionStr = "tiotest v0.3.1 (C) 1999-2000 Mika Kuoppala <miku@iki.fi>";

/* 
   This is global for easier usage. If you put changing data
   in here from threads, be sure to protect it with mutexes.
*/
ArgumentOptions args;

int main(int argc, char *argv[])
{
	ThreadTest test;

	strcpy(args.path[0], DEFAULT_DIRECTORY );
	args.pathsCount = 1;
	args.fileSizeInMBytes = DEFAULT_FILESIZE;
	args.blockSize = DEFAULT_BLOCKSIZE;
	args.numThreads = DEFAULT_THREADS;
	args.numRandomOps = DEFAULT_RANDOM_OPS;
	args.debugLevel = 0;
	args.verbose = 0;
	args.terse = 0;
	args.consistencyCheckData = 0;

#if (LARGEFILES && USE_MMAP)
	printf("warning: LARGEFILES with MMAP needs mmap64 support which is not working yet in tiotest!\n");
#endif

	parse_args( &args, argc, argv );
    
	initialize_test( &test );

	do_tests( &test );

	print_results( &test );

	cleanup_test( &test );

	return 0;
}

void parse_args( ArgumentOptions* args, int argc, char *argv[] )
{
	int c;
	int once = 0;

	while (1)
	{
		c = getopt( argc, argv, "f:b:d:t:r:hTWc");

		if (c == -1)
			break;
	
		switch (c)
		{

		case 'f':
			args->fileSizeInMBytes = atoi(optarg);
			break;
	    
		case 'b':
			args->blockSize = atoi(optarg);
			break;
	    
		case 'd':
			if (args->pathsCount < MAX_PATHS) {
				if (!once) {
					args->pathsCount = 0;           
					once = 1;
				}
				strcpy(args->path[args->pathsCount++], optarg);
			}
			break;
	    
		case 't':
			args->numThreads = atoi(optarg);
			break;
	    
		case 'r':
			args->numRandomOps = atoi(optarg);
			break;
	    
		case 'T':
			args->terse = TRUE;
			break;

		case 'W':
			args->sequentialWriting = TRUE;
			break;

		case 'c':
			args->consistencyCheckData = TRUE;
			break;

		case 'h':
			print_help_and_exit();
			break;

		case '?':
		default:	    
			printf("Try 'tiotest -h' for more information.\n");
			exit(1);
			break;
		}
	}
}

void initialize_test( ThreadTest *d )
{
	int i;
	int pathLoadBalIdx = 0;

	memset( d, 0, sizeof(ThreadTest) );
    
	d->numThreads = args.numThreads; 

	for(i = 0; i < d->numThreads; i++)
	{
		d->threads = calloc( d->numThreads, sizeof(ThreadData) );
		if( d->threads == NULL )
		{
			perror("Error allocating memory");
			exit(-1);
		}
	}

	/* Initializing thread data */
    
	for(i = 0; i < d->numThreads; i++)
	{
		d->threads[i].myNumber = i;
		d->threads[i].fileSizeInMBytes = args.fileSizeInMBytes;
		d->threads[i].blockSize = args.blockSize;
		d->threads[i].numRandomOps = args.numRandomOps;
		sprintf(d->threads[i].fileName, "%s/_%d_tiotest.%d",
			args.path[pathLoadBalIdx++], getpid(), i);
		
		if( pathLoadBalIdx >= args.pathsCount )
			pathLoadBalIdx = 0;

		pthread_attr_init( &(d->threads[i].thread_attr) );

		pthread_attr_setscope(&(d->threads[i].thread_attr),
				      PTHREAD_SCOPE_SYSTEM);

		d->threads[i].buffer = malloc( d->threads[i].blockSize );
		if( d->threads[i].buffer == NULL )
		{
			perror("Error allocating memory");
			exit(-1);
		}

		if( args.consistencyCheckData )
		{
			int j;
			const unsigned long bsize = d->threads[i].blockSize;
			unsigned char *b = d->threads[i].buffer;

			for(j = 0; j < bsize; j++)
				b[j] = rand() & 0xFF;

			d->threads[i].bufferCrc = crc32(b, bsize, 0);
		}
	}
}

void print_option(const char* s, 
		  const char* desc, 
		  const char* def)
{
	printf("  %s          %s", s, desc);
    
	if(def)
		printf(" (default: %s)", def);

	printf("\n");
   
}

char *my_int_to_string(int a)
{
	static char tempBuffer[128];

	sprintf(tempBuffer, "%d", a);

	return tempBuffer;
}

void print_help_and_exit()
{
	printf("%s\n", versionStr);

	printf("Usage: tiotest [options]\n");

	print_option("-f", "Filesize per thread in MBytes",
		     my_int_to_string(DEFAULT_FILESIZE));

	print_option("-b", "Blocksize to use in bytes",
		     my_int_to_string(DEFAULT_BLOCKSIZE));

	print_option("-d", "Directory for test files", 
		     DEFAULT_DIRECTORY);

	print_option("-t", "Number of concurrent test threads",
		     my_int_to_string(DEFAULT_THREADS));

	print_option("-r", "Random I/O operations per thread", 
		     my_int_to_string(DEFAULT_RANDOM_OPS));

	print_option("-T", "More terse output", 0);

	print_option("-W", "Do writing phase sequentially", 0);

	print_option("-c", 
		     "Consistency check data (will slow io and raise cpu%)",
		     0);

	print_option("-h", "Print this help and exit", 0);

	exit(1);
}

void cleanup_test( ThreadTest *d )
{
	int i;

	for(i = 0; i < d->numThreads; i++)
	{
		unlink(d->threads[i].fileName);
		free( d->threads[i].buffer );
		d->threads[i].buffer = 0;
	
		pthread_attr_destroy( &(d->threads[i].thread_attr) );
	}

	free(d->threads);
    
	d->threads = 0;
}

void wait_for_threads( ThreadTest *d )
{
	int i;

	for(i = 0; i < d->numThreads; i++)
		pthread_join(d->threads[i].thread, NULL);	
}

void do_tests( ThreadTest *thisTest )
{
	Timings *timeWrite       = &(thisTest->totalTimeWrite);
	Timings *timeRandomWrite = &(thisTest->totalTimeRandomWrite);
	Timings *timeRead        = &(thisTest->totalTimeRead);
	Timings *timeRandomRead  = &(thisTest->totalTimeRandomRead);

	timer_init( timeWrite );
	timer_init( timeRandomWrite );
	timer_init( timeRead );
	timer_init( timeRandomRead );

	/*
	  Write testing 
	*/
    
	timer_start( timeWrite );
    
	start_test_threads( thisTest, WRITE_TEST, args.sequentialWriting );
    
	if(args.debugLevel > 4)
	{
		fprintf(stderr, "Waiting write threads to finish...");
		fflush(stderr);
	}
    
	wait_for_threads( thisTest );
    
	timer_stop( timeWrite );

	if(args.debugLevel > 4)
	{
		fprintf(stderr, "Done!\n");
		fflush(stderr);
	}

	/*
	  RandomWrite testing 
	*/
    
	timer_start( timeRandomWrite );
    
	start_test_threads( thisTest, RANDOM_WRITE_TEST, FALSE );
    
	if(args.debugLevel > 4)
	{
		fprintf(stderr, "Waiting random write threads to finish...");
		fflush(stderr);
	}
    
	wait_for_threads( thisTest );
    
	timer_stop( timeRandomWrite );

	if(args.debugLevel > 4)
	{
		fprintf(stderr, "Done!\n");
		fflush(stderr);
	}

	/*
	  Read testing 
	*/
    
	timer_start( timeRead );
    
	start_test_threads( thisTest, READ_TEST, FALSE );
    
	if(args.debugLevel > 4)
	{
		fprintf(stderr, "Waiting read threads to finish...");
		fflush(stderr);
	}
    
	wait_for_threads( thisTest );
    
	timer_stop( timeRead );
    
	if(args.debugLevel > 4)
	{
		fprintf(stderr, "Done!\n");
		fflush(stderr);
	}

	/*
	  RandomRead testing 
	*/

	timer_start( timeRandomRead );
    
	start_test_threads( thisTest, RANDOM_READ_TEST, FALSE );
    
	if(args.debugLevel > 4)
	{
		fprintf(stderr, "Waiting random read threads to finish...");
		fflush(stderr);
	}
    
	wait_for_threads( thisTest );
    
	timer_stop( timeRandomRead );
    
	if(args.debugLevel > 4)
	{
		fprintf(stderr, "Done!\n");
		fflush(stderr);
	}
}

void start_test_threads( ThreadTest *test, int testCase, int sequential )
{
	int i;

	for(i = 0; i < test->numThreads; i++)
	{
		if( pthread_create(
			&(test->threads[i].thread), 
			&(test->threads[i].thread_attr), 
			Tests[testCase], 
			(void *)&test->threads[i]))
		{
			perror("Error creating threads");
			exit(-1);
		}

		if(sequential)
		{
			if(args.debugLevel > 2)
				fprintf(stderr, 
					"Waiting previous thread "
					"to finish before starting "
					"a new one\n" );
	    
			pthread_join(test->threads[i].thread, NULL);
		}
	}
    
	if(args.debugLevel > 4)
		printf("Created %d threads\n", i);
}

void print_results( ThreadTest *d )
{
/*
  This is messy and should be rewritten but some of unixes, didn't
  understand all printf options and long long formats.
*/
	int i;    
	double totalBlocksWrite = 0, totalBlocksRead = 0, 
	    totalBlocksRandomWrite = 0, totalBlocksRandomRead = 0;

	double read_rate,write_rate,random_read_rate,random_write_rate;
	double realtime_write,usrtime_write = 0, systime_write = 0;
	double realtime_rwrite = 0, usrtime_rwrite = 0, systime_rwrite = 0;
	double realtime_read, usrtime_read = 0, systime_read = 0;
	double realtime_rread = 0, usrtime_rread= 0, systime_rread = 0;

	double mbytesWrite, mbytesRandomWrite, mbytesRead, mbytesRandomRead;

	for(i = 0; i < d->numThreads; i++)
	{
		usrtime_write += 
		    timer_usertime( &(d->threads[i].writeTimings) );
		systime_write += 
		    timer_systime( &(d->threads[i].writeTimings) );

		usrtime_rwrite += 
		    timer_usertime( &(d->threads[i].randomWriteTimings) );
		systime_rwrite += 
		    timer_systime( &(d->threads[i].randomWriteTimings) );

		usrtime_read += timer_usertime( &(d->threads[i].readTimings) );
		systime_read += timer_systime( &(d->threads[i].readTimings) );

		usrtime_rread += 
		    timer_usertime( &(d->threads[i].randomReadTimings) );
		systime_rread += 
		    timer_systime( &(d->threads[i].randomReadTimings) );

		totalBlocksWrite       += d->threads[i].blocksWritten;
		totalBlocksRandomWrite += d->threads[i].blocksRandomWritten;
		totalBlocksRead        += d->threads[i].blocksRead; 
		totalBlocksRandomRead  += d->threads[i].blocksRandomRead;
	}

	mbytesWrite = totalBlocksWrite / 
	    ((double)MBYTE/(double)(d->threads[0].blockSize));
	mbytesRandomWrite = totalBlocksRandomWrite /
	    ((double)MBYTE/(double)(d->threads[0].blockSize));

	mbytesRead = totalBlocksRead / 
	    ((double)MBYTE/(double)(d->threads[0].blockSize));
	mbytesRandomRead = totalBlocksRandomRead / 
	    ((double)MBYTE/(double)(d->threads[0].blockSize));

	realtime_write  = timer_realtime( &(d->totalTimeWrite) );
	realtime_rwrite = timer_realtime( &(d->totalTimeRandomWrite) );
	realtime_read   = timer_realtime( &(d->totalTimeRead) );
	realtime_rread  = timer_realtime( &(d->totalTimeRandomRead) );

	if(args.terse)
	{
		printf("write:%.5f,%.5f,%.5f,%.5f\n",
		       mbytesWrite, 
		       realtime_write, usrtime_write, systime_write );

		printf("rwrite:%.5f,%.5f,%.5f,%.5f\n",
		       mbytesRandomWrite, 
		       realtime_rwrite, usrtime_rwrite, systime_rwrite );

		printf("read:%.5f,%.5f,%.5f,%.5f\n",
		       mbytesRead, 
		       realtime_read, usrtime_read, systime_read );

		printf("rread:%.5f,%.5f,%.5f,%.5f\n",
		       mbytesRandomRead, 
		       realtime_rread, usrtime_rread, systime_rread );

		return;
	}

	
	write_rate = mbytesWrite / realtime_write;
	random_write_rate = mbytesRandomWrite / realtime_rwrite;
 
	read_rate  = mbytesRead / realtime_read;
	random_read_rate  = mbytesRandomRead / realtime_rread;

	printf("Tiotest results for %d concurrent io threads:\n\n", 
	       d->numThreads);
	
	printf(",----------------------------------------------------------------------.\n");
	printf("| Item                  | Time     | Rate         | Usr CPU | Sys CPU  |\n");
	printf("+-----------------------+----------+--------------+----------+---------+\n");
    
	if(totalBlocksWrite)
		printf("| Write %11.0f MBs | %6.1f s | %7.3f MB/s | %5.1f %%  | %5.1f %% |\n",
		       mbytesWrite,
		       realtime_write,write_rate,
		       usrtime_write*100.0/realtime_write,
		       systime_write*100.0/realtime_write );

	if(totalBlocksRandomWrite)
		printf("| Random Write %4.0f MBs | %6.1f s | %7.3f MB/s | %5.1f %%  | %5.1f %% |\n",
		       mbytesRandomWrite,
		       realtime_rwrite,random_write_rate,
		       usrtime_rwrite*100.0/realtime_rwrite,
		       systime_rwrite*100.0/realtime_rwrite );

    
	if(totalBlocksRead)
		printf("| Read %12.0f MBs | %6.1f s | %7.3f MB/s | %5.1f %%  | %5.1f %% |\n",
		       mbytesRead,
		       realtime_read,read_rate,
		       usrtime_read*100.0/realtime_read,
		       systime_read*100.0/realtime_read );

    
	if(totalBlocksRandomRead)
		printf("| Random Read %5.0f MBs | %6.1f s | %7.3f MB/s | %5.1f %%  | %5.1f %% |\n",
		       mbytesRandomRead,
		       realtime_rread,random_read_rate,
		       usrtime_rread*100.0/realtime_rread,
		       systime_rread*100.0/realtime_rread );

	printf("`----------------------------------------------------------------------'\n\n");
}

void* do_write_test( void *data )
{
	ThreadData *d = (ThreadData *)data;
	int     fd;
	char    *buf = d->buffer;
	toff_t  blocks=(d->fileSizeInMBytes*MBYTE)/d->blockSize;
	toff_t  i;
	int     openFlags = O_RDWR | O_CREAT | O_TRUNC;

#ifdef USE_MMAP
	toff_t  bytesize=blocks*d->blockSize; /* truncates down to BS multiple */
	void *file_loc;
#endif

#ifdef LARGEFILES
	openFlags |= O_LARGEFILE;
#endif
    
	fd = open(d->fileName, openFlags, 0600 );
	if(fd == -1)
		perror("Error opening file");

#ifdef USE_MMAP
	ftruncate(fd,bytesize); /* pre-allocate space */
	file_loc=mmap(NULL,bytesize,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if(file_loc == MAP_FAILED)
		perror("Error mmap()ing file");
#ifdef USE_MADVISE
	/* madvise(file_loc,bytesize,MADV_DONTNEED); */
	madvise(file_loc,bytesize,MADV_RANDOM);
#endif
#endif

	timer_start( &(d->writeTimings) );

	for(i = 0; i < blocks; i++)
	{
#ifdef USE_MMAP
		memcpy(file_loc + i * d->blockSize,buf,d->blockSize);
#else
		if( write( fd, buf, d->blockSize ) != d->blockSize )
		{
			perror("Error writing to file");
			break;
		}
#endif
		d->blocksWritten++;
	} 
    
#ifdef USE_MMAP
	munmap(file_loc,bytesize);
#endif

	fsync(fd);

	close(fd);

	timer_stop( &(d->writeTimings) );

	return 0;
}

void* do_random_write_test( void *data )
{
	ThreadData *d = (ThreadData *)data;

	int      i;
	char     *buf = d->buffer;
	toff_t   blocks=(d->fileSizeInMBytes*MBYTE/d->blockSize);
	int      fd;
	toff_t   offset;
	ssize_t  bytesWritten;
	int      openFlags = O_WRONLY;

	unsigned int seed = get_random_seed();

#ifdef LARGEFILES
	openFlags |= O_LARGEFILE;
#endif

	fd = open(d->fileName, openFlags);
	if(fd == -1)
		perror("Error opening file");
    
	timer_start( &(d->randomWriteTimings) );

	for(i = 0; i < d->numRandomOps; i++)
	{
		offset = get_random_offset(blocks-1, &seed) * d->blockSize;

		if(args.debugLevel > 10)
		{
			fprintf(stderr, "Thread: %u chose seek of %Lu\n", 
				(unsigned)getpid(), (long long)offset );
			fflush(stderr);
		}

#ifdef LARGEFILES
		if( lseek64( fd, offset, SEEK_SET ) != offset )
		{
			sprintf(buf, 
				"Error in seek, offset= %Ld, seeks = %ld: ", 
				offset, d->blocksRandomWritten );
	    
			perror(buf);
			break;
		}
#else
		if( lseek( fd, offset, SEEK_SET ) != offset )
		{
			sprintf(buf, 
				"Error in seek, offset = %ld, seeks = %ld:",
				offset, d->blocksRandomWritten );

			perror(buf);
			break;
		}
#endif
		if( (bytesWritten = write( fd, buf, d->blockSize )) != d->blockSize )
		{
			sprintf(buf, 
#ifdef LARGEFILES
				"Error in randomwrite, off=%Ld, read=%d, seeks=%ld : ", 
#else
				"Error in randomwrite, off=%ld, read=%d, seeks=%ld : ",
#endif
				offset, bytesWritten, d->blocksRandomWritten );
		    
			perror(buf);
			break;
		}
	
		d->blocksRandomWritten++;
	} 

	fsync(fd);

	close(fd);

	timer_stop( &(d->randomWriteTimings) );
	
	return 0;
}

void* do_read_test( void *data )
{
	ThreadData *d = (ThreadData *)data;

	char    *buf = d->buffer;
	int     fd;
	toff_t  blocks=(d->fileSizeInMBytes*MBYTE)/d->blockSize;
	toff_t  i;
	int     openFlags = O_RDONLY;
 
#ifdef USE_MMAP
	toff_t  bytesize=blocks*d->blockSize; /* truncates down to BS multiple */
	void *file_loc;
#endif

#ifdef LARGEFILES
	openFlags |= O_LARGEFILE;
#endif

	fd = open(d->fileName, openFlags);
	if(fd == -1)
		perror("Error opening file");

#ifdef USE_MMAP
	file_loc=mmap(NULL,bytesize,PROT_READ,MAP_SHARED,fd,0);
#ifdef USE_MADVISE
	/* madvise(file_loc,bytesize,MADV_DONTNEED); */
	madvise(file_loc,bytesize,MADV_RANDOM);
#endif
#endif

	timer_start( &(d->readTimings) );

	for(i = 0; i < blocks; i++)
	{
#ifdef USE_MMAP
		memcpy(buf,file_loc + i * d->blockSize,d->blockSize);
#else
		if( read( fd, buf, d->blockSize ) != d->blockSize )
		{
			perror("Error read from file");
			break;
		}
#endif
		
		if( args.consistencyCheckData )
		{
		    if( crc32(buf, d->blockSize, 0) != d->bufferCrc )
		    {
			fprintf(stderr, 
				"io error: crc read error in file %s "
				"on block %lu\n",
				d->fileName, d->blocksRead );

			exit(10);
		    }
		}
		
		d->blocksRead++;
	} 
    
	timer_stop( &(d->readTimings) );

#ifdef MMAP
	munmap(file_loc,bytesize);
#endif
	close(fd);

	return 0;
}

void* do_random_read_test( void *data )
{
	ThreadData *d = (ThreadData *)data;

	int      i;
	char     *buf = d->buffer;
	toff_t   blocks=(d->fileSizeInMBytes*MBYTE/d->blockSize);
	int      fd;
	toff_t   offset;
	ssize_t  bytesRead;
	int      openFlags = O_RDONLY;

	unsigned int seed = get_random_seed();

#ifdef LARGEFILES
	openFlags |= O_LARGEFILE;
#endif

	fd = open(d->fileName, openFlags);
	if(fd == -1)
		perror("Error opening file");
    
	timer_start( &(d->randomReadTimings) );

	for(i = 0; i < d->numRandomOps; i++)
	{
		offset = get_random_offset(blocks-1, &seed) * d->blockSize;

		if(args.debugLevel > 10)
		{
			fprintf(stderr, "Thread: %u chose seek of %Lu\n", 
				(unsigned)getpid(), (long long)offset );
			fflush(stderr);
		}

#ifdef LARGEFILES
		if( lseek64( fd, offset, SEEK_SET ) != offset )
		{
			sprintf(buf, 
				"Error in randomread, offset= %Ld, seeks = %ld: ", 
				offset, d->blocksRandomRead );
	    
			perror(buf);
			break;
		}
#else
		if( lseek( fd, offset, SEEK_SET ) != offset )
		{
			sprintf(buf, 
				"Error in randomread, offset = %ld, seeks = %ld:",
				offset, d->blocksRandomRead );

			perror(buf);
			break;
		}
#endif
		if( (bytesRead = read( fd, buf, d->blockSize )) != d->blockSize )
		{
			sprintf(buf, 
#ifdef LARGEFILES
				"Error in seek/read, off=%Ld, read=%d, seeks=%ld : ", 
#else
				"Error in seek/read, off=%ld, read=%d, seeks=%ld : ",
#endif
				offset, bytesRead, d->blocksRandomRead );
		    
			perror(buf);
			break;
		}
	
		if( args.consistencyCheckData )
		{
		    if( crc32(buf, d->blockSize, 0) != d->bufferCrc )
		    {
			fprintf(stderr, 
				"io error: crc seek/read error in file %s "
				"on block %lu\n",
				d->fileName, d->blocksRandomRead );
			
			exit(11);
		    }
		}

		d->blocksRandomRead++;
	} 
	
	timer_stop( &(d->randomReadTimings) );

	close(fd);

	return 0;
}

clock_t get_time()
{
	struct tms buf;
    
	return times(&buf);
}

unsigned int get_random_seed()
{
	unsigned int seed;
	struct timeval r;
    
	if(gettimeofday( &r, NULL ) == 0)
	{
		seed = r.tv_usec;
	}
	else
	{
		seed = 0x12345678;
	}

	return seed;
}

inline const toff_t get_random_offset(const toff_t max, unsigned int *seed)
{
#if (RAND_MAX < 2147483647)
	unsigned long rr_max = RAND_MAX;
#endif
	unsigned long rr = rand_r(seed);

/* 
   This should fix bug in glibc < 2.1.3 which returns too high
   random numbers
*/
	if( rr > RAND_MAX )
	{
		rr &= RAND_MAX;
	}
/*
  This is for braindead unixes having 15bit RAND_MAX :)
  The whole random stuff would need rethinking.
  If this didn't have to be portable /dev/urandom would
  be the best choice.
*/

#if (RAND_MAX < 2147483647)
	rr |= rand_r(seed) << 16;
	rr_max = rr_max << 16;
#endif

#if 0
	return (toff_t) ((double)(max) * rr / (rr_max + 1.0));
#else
	return (toff_t) (rr % max);
#endif
}

void timer_init(Timings *t)
{
	memset( t, 0, sizeof(Timings) );
}

void timer_start(Timings *t)
{
	struct rusage ru;

	if(gettimeofday( &(t->startRealTime), NULL ))
	{
		perror("Error in gettimeofday\n");
		exit(10);
	}

	if(getrusage( RUSAGE_SELF, &ru ))
	{
		perror("Error in getrusage\n");
		exit(11);
	}

	memcpy( &(t->startUserTime), &(ru.ru_utime), sizeof( struct timeval ));
	memcpy( &(t->startSysTime), &(ru.ru_stime), sizeof( struct timeval ));
}

void timer_stop(Timings *t)
{
	struct rusage ru;

	if(gettimeofday( &(t->stopRealTime), NULL ))
	{
		perror("Error in gettimeofday\n");
		exit(10);
	}

	if( getrusage( RUSAGE_SELF, &ru ))
	{
		perror("Error in getrusage\n");
		exit(11);
	}

	memcpy( &(t->stopUserTime), &(ru.ru_utime), sizeof( struct timeval ));
	memcpy( &(t->stopSysTime), &(ru.ru_stime), sizeof( struct timeval ));
}

const double timer_realtime(const Timings *t)
{
	double value;

	value = t->stopRealTime.tv_sec - t->startRealTime.tv_sec;
	value += (t->stopRealTime.tv_usec - 
		  t->startRealTime.tv_usec)/1000000.0;

	return value;
}

const double timer_usertime(const Timings *t)
{
	double value;

	value = t->stopUserTime.tv_sec - t->startUserTime.tv_sec;
	value += (t->stopUserTime.tv_usec - 
		  t->startUserTime.tv_usec)/1000000.0;

	return value;
}

const double timer_systime(const Timings *t)
{
	double value;

	value = t->stopSysTime.tv_sec - t->startSysTime.tv_sec;
	value += (t->stopSysTime.tv_usec - 
		  t->startSysTime.tv_usec)/1000000.0;

	return value;
}
