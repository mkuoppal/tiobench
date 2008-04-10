/*
 *    Threaded io test
 *
 *  Copyright (C) 1999-2008 Mika Kuoppala <miku at iki.fi>
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

#include <assert.h>

static const char* const versionStr = "tiotest v0.4.1 (C) 1999-2008 tiobench team <http://tiobench.sf.net/>";

static ArgumentOptions args;

static float timeval_percentage_of(const struct timeval* value, const struct timeval* from, unsigned int divider);
static void add_timer(struct timeval* v, const struct timeval* start_time, const struct timeval* end_time);

static void update_latency_info(Latencies *lat, struct timeval tv_start, struct timeval tv_stop)
{
	double value;

	value = tv_stop.tv_sec - tv_start.tv_sec;
	value += (tv_stop.tv_usec - tv_start.tv_usec)/1000000.0;

	if (value > lat->max)
		lat->max = value;
	lat->avg += value;
	lat->count++;
	if (value > (double)LATENCY_STAT1)
		lat->count1++;
	if (value > (double)LATENCY_STAT2)
		lat->count2++;
	return;
}

static void * aligned_alloc(const ssize_t size)
{
	caddr_t a;

	a = TIO_mmap((caddr_t )0, size, 
				 PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, (TIO_off_t)0);
	if (a == MAP_FAILED) {
		perror("Error " xstr(TIO_mmap) "()ing anonymous memory chunk");
		exit(-1);
	}
	return a;
}

static int aligned_free(caddr_t a, const ssize_t size)
{
	return munmap(a, size);
}

int main(int argc, char *argv[])
{
	ThreadTest test;
	int i;

	strcpy(args.path[0], DEFAULT_DIRECTORY );
	args.pathsCount = 1;
	args.fileSizeInMBytes = DEFAULT_FILESIZE;
	args.blockSize = DEFAULT_BLOCKSIZE;
	args.numThreads = DEFAULT_THREADS;
	args.numRandomOps = DEFAULT_RANDOM_OPS;
	args.debugLevel = DEFAULT_DEBUG_LEVEL;
	args.verbose = FALSE;
	args.terse = FALSE;
	args.use_mmap = FALSE;
	args.consistencyCheckData = FALSE;
	args.syncWriting = FALSE;
	args.rawDrives = FALSE;
	args.showLatency = TRUE;
	args.threadOffset = DEFAULT_RAW_OFFSET;
	args.useThreadOffsetForFirstThread = FALSE;
	
	for(i = 0; i < TESTS_COUNT; i++)
		args.testsToRun[i] = 1;
	
	parse_args( &args, argc, argv );
    
	initialize_test( &test );
	
	do_tests( &test );

	print_results( &test );

	cleanup_test( &test );

	return 0;
}

static void checkValidFileSize(const int value)
{
#ifndef USE_LARGEFILES
	if (value > MAXINT / (1024*1024)) 
	{
		fprintf(stderr, "Specified file size too large, please specify something under 2GB\n");
		exit(1);
	}
#endif
}

static void checkIntZero(const int value, const char* const mess)
{
	if (value <= 0) 
	{
		fprintf(stderr, mess);
		fprintf(stderr, "Try 'tiotest -h' for more information.\n");
		exit(1);
	}
}

static void checkLong(const long value, const char* const mess)
{
	if (value < 0)
	{
		fprintf(stderr, mess);
		fprintf(stderr, "Try 'tiotest -h' for more information\n");
		exit(1);
	}
}

static void parse_args( ArgumentOptions* args, int argc, char *argv[] )
{
	int c;
	int once = 0;

	while (1)
	{
		c = getopt( argc, argv, "f:b:d:t:r:D:k:o:hLRTWSOcM");

		if (c == -1)
			break;
	
		switch (c)
		{
		case 'f':
			args->fileSizeInMBytes = atoi(optarg);
			checkIntZero(args->fileSizeInMBytes, "Wrong file size\n");
			checkValidFileSize(args->fileSizeInMBytes);
			break;
	    
		case 'b':
			args->blockSize = atoi(optarg);
			checkIntZero(args->blockSize, "Wrong block size\n");
			break;
	    
		case 'd':
			if (args->pathsCount < MAX_PATHS) 
			{
				if (!once) 
				{
					args->pathsCount = 0;           
					once = 1;
				}
				strcpy(args->path[args->pathsCount++], optarg);
			}
			break;
	    
		case 't':
			args->numThreads = atoi(optarg);
			checkIntZero(args->numThreads, "Wrong number of threads\n");
			break;
	    
		case 'r':
			args->numRandomOps = atoi(optarg);
			checkIntZero(args->numRandomOps, "Wrong number of random I/O operations\n");
			break;
	    
		case 'L':
			args->showLatency = FALSE;
			break;
	    
		case 'T':
			args->terse = TRUE;
			break;

		case 'M':
			args->use_mmap = TRUE;
			break;

		case 'W':
			args->sequentialWriting = TRUE;
			break;
			
		case 'S':
			args->syncWriting = TRUE;
			break;
			
		case 'R':
			args->rawDrives = TRUE;
			break;

		case 'c':
			args->consistencyCheckData = TRUE;
			break;

		case 'h':
			print_help_and_exit();
			break;
		
		case 'D':
			args->debugLevel = atoi(optarg);
			break;
		
		case 'o':
			args->threadOffset = atol(optarg);
			checkLong(args->threadOffset, "Wrong offset between threads\n");
			break;
			
		case 'O':
			args->useThreadOffsetForFirstThread = TRUE;
			break;
			
		case 'k':
		{
			const int i = atoi(optarg);
			if (i < TESTS_COUNT) 
			{
				args->testsToRun[i] = 0;
				break;
			}
			else
				fprintf(stderr, "Wrong test number %d\n", i);
			/* Go through */
		}
		case '?':
		default:
			fprintf(stderr, "Try 'tiotest -h' for more information\n");
			exit(1);
			break;
		}
	}
}

static void initialize_test( ThreadTest *d )
{
	int i;
	int pathLoadBalIdx = 0;
	TIO_off_t offs, cur_offs[KBYTE] = {0};

	memset( d, 0, sizeof(ThreadTest) );
    
	d->numThreads = args.numThreads; 

	d->threads = calloc( d->numThreads, sizeof(ThreadData) );
	if( d->threads == NULL )
	{
		perror("Error calloc()ing thread data memory");
		exit(-1);
	}

	/* Initializing thread data */
	if (args.rawDrives) 
	{
		if (args.threadOffset != 0) 
		{
			offs = (args.threadOffset + args.fileSizeInMBytes) * MBYTE;
			if (args.useThreadOffsetForFirstThread) 
			{
				int k;
				for(k = 0; k < KBYTE; k++)
					cur_offs[k] = (TIO_off_t)args.threadOffset * MBYTE;
			}
		}
		else
			offs = (TIO_off_t)args.fileSizeInMBytes * MBYTE;
	}
	else
		offs = 0;

	for(i = 0; i < d->numThreads; i++)
	{
		d->threads[i].myNumber = i;
		d->threads[i].blockSize = args.blockSize;
		d->threads[i].numRandomOps = args.numRandomOps;
		d->threads[i].fileSizeInMBytes = args.fileSizeInMBytes;
		if (args.rawDrives)
		{
			d->threads[i].fileOffset = cur_offs[pathLoadBalIdx];
			cur_offs[pathLoadBalIdx] += offs;
			sprintf(d->threads[i].fileName, "%s",
					args.path[pathLoadBalIdx++]);
		}
		else
		{
			d->threads[i].fileOffset = 0;
			sprintf(d->threads[i].fileName, "%s/_tiotest_pid%d.thr%d",
					args.path[pathLoadBalIdx++], (int) getpid(), i);
		}
		
		if( pathLoadBalIdx >= args.pathsCount )
			pathLoadBalIdx = 0;

		pthread_attr_init( &(d->threads[i].thread_attr) );

		pthread_attr_setscope(&(d->threads[i].thread_attr),
							  PTHREAD_SCOPE_SYSTEM);

		d->threads[i].buffer = aligned_alloc( d->threads[i].blockSize );

		if( args.consistencyCheckData )
		{
			int j;
			const unsigned long bsize = d->threads[i].blockSize;
			unsigned char *b = d->threads[i].buffer;

			for(j = 0; j < bsize; j++)
			{
				b[j] = rand() & 0xFF;
			}

			d->threads[i].bufferCrc = crc32(b, bsize, 0);
		}
	}
}

static void print_option(const char* s, 
						 const char* desc, 
						 const char* def)
{
	printf("  %s          %s", s, desc);
    
	if(def)
		printf(" (default: %s)", def);

	printf("\n");
   
}

static char *my_int_to_string(int a)
{
	static char tempBuffer[128];

	sprintf(tempBuffer, "%d", a);

	return tempBuffer;
}

static void print_help_and_exit()
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
		     
	print_option("-o", "Offset in Mb on disk between threads. Use with -R option",
				 0);
	
	print_option("-k", "Skip test number n. Could be used several times.", 0);	  
	
	print_option("-L", "Hide latency output", 0);	  
	
	print_option("-R", "Use raw devices. Set device name with -d option", 0);

	print_option("-T", "More terse output", 0);

	print_option("-M", "Use mmap for I/O", 0);

	print_option("-W", "Do writing phase sequentially", 0);
	
	print_option("-S", "Do writing synchronously", 0);
	
	print_option("-O", "Use offset from -o option for first thread. Use with -R option",
				 0);

	print_option("-c", 
				 "Consistency check data (will slow io and raise cpu%)",
				 0);
	
	print_option("-D", "Debug level",
				 my_int_to_string(DEFAULT_DEBUG_LEVEL));

	print_option("-h", "Print this help and exit", 0);

	exit(1);
}

static void cleanup_test( ThreadTest *d )
{
	int i;

	for(i = 0; i < d->numThreads; i++)
	{
		if (!args.rawDrives)
			unlink(d->threads[i].fileName);
		aligned_free( (char *)d->threads[i].buffer, d->threads[i].blockSize );
		d->threads[i].buffer = 0;
	
		pthread_attr_destroy( &(d->threads[i].thread_attr) );
	}

	free(d->threads);
    
	d->threads = 0;
}

static void wait_for_threads( ThreadTest *d )
{
	int i;

	for(i = 0; i < d->numThreads; i++)
		pthread_join(d->threads[i].thread, NULL);	
}

static void do_tests( ThreadTest *thisTest )
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
	if (args.testsToRun[WRITE_TEST])
		do_test( thisTest, WRITE_TEST, args.sequentialWriting,
				 timeWrite,  "Waiting write threads to finish...");

	/*
	  RandomWrite testing 
	*/
	if (args.testsToRun[RANDOM_WRITE_TEST])
		do_test( thisTest, RANDOM_WRITE_TEST, FALSE, timeRandomWrite,
				 "Waiting random write threads to finish...");

	/*
	  Read testing 
	*/
	if (args.testsToRun[READ_TEST])
		do_test( thisTest, READ_TEST, FALSE, timeRead,
				 "Waiting read threads to finish..." );

	/*
	  RandomRead testing 
	*/
	if (args.testsToRun[RANDOM_READ_TEST])
		do_test( thisTest, RANDOM_READ_TEST, FALSE, timeRandomRead,
				 "Waiting random read threads to finish...");
}

typedef struct 
{
	volatile int *child_status;
	TestFunc fn;
	ThreadData *d;
	volatile int *pstart;
} StartData;

static void* start_proc( void *data )
{
	StartData *sd = (StartData*)data;
	*sd->child_status = getpid();
	if (sd->pstart != NULL)
		while (*sd->pstart == 0) sleep(0);
	sd->fn(sd->d);
	return NULL;
}

static void t_log (int level, char *message)
{
	if(args.debugLevel >= level)
		fprintf(stderr, "%s\n", message);
}

static void do_test( ThreadTest *test, int testCase, int sequential,
					 Timings *t, char *debugMessage )
{
	int i;
	volatile int *child_status;
	StartData *sd;
	int synccount;
	volatile int start = 0;
	
	child_status = (volatile int *)calloc(test->numThreads, sizeof(int));
	if (child_status == NULL) 
	{
		perror("Error calloc()ing thread status memory");
		return;
	}
	
	sd = (StartData*)calloc(test->numThreads, sizeof(StartData));
	if (sd == NULL) 
	{
		perror("Error calloc()ing thread start data memory");
		free((int*)child_status);
		return;
	}
	
	if (sequential)
		timer_start(t);
	
	for(i = 0; i < test->numThreads; i++)
	{
		sd[i].child_status = &child_status[i];
		sd[i].fn = Tests[testCase];
		sd[i].d = &test->threads[i];
		if (sequential)
			sd[i].pstart = NULL;
		else
			sd[i].pstart = &start;
		if( pthread_create(
				&(test->threads[i].thread), 
				&(test->threads[i].thread_attr), 
				start_proc,
				(void *)&sd[i]))
		{
			perror("Error from pthread_create()");
			free((int*)child_status);
			free(sd);
			exit(-1);
		}

		if(sequential)
		{
			t_log(LEVEL_INFO,"Waiting previous thread to finish before starting a new one");
			pthread_join(test->threads[i].thread, NULL);
		}
	}
	
	if(sequential) 
		timer_stop(t);
	else 
	{
		struct timeval tv1, tv2;
		gettimeofday(&tv1, NULL);
		do 
		{
			synccount = 0;
			for(i = 0; i < test->numThreads; i++) 
				if (child_status[i]) 
					synccount++;
			if (synccount == test->numThreads) 
				break;
			sleep(1);
			gettimeofday(&tv2, NULL);
		} while ((tv2.tv_sec - tv1.tv_sec) < 30);

		if (synccount != test->numThreads) 
		{
			fprintf(stderr, "Unable to start %d threads (started %d)\n", 
					test->numThreads, synccount);
			start = 1;
			wait_for_threads(test);
			free((int*)child_status);
			free(sd);
			return;
		}

        t_log(LEVEL_INFO, "Created threads");
	
		timer_start(t);

		start = 1;
    
		t_log(LEVEL_INFO, "Waiting threads");

		wait_for_threads(test);
    
		timer_stop(t);
	}
	free((int*)child_status);

	free(sd);
    
	t_log(LEVEL_INFO, "Done!");
}

static void add_timer(struct timeval* v, const struct timeval* start_time, const struct timeval* end_time)
{
	struct timeval tmp;

	assert(end_time->tv_sec >= start_time->tv_sec);
	if(start_time->tv_sec == end_time->tv_sec)
	{
		assert(start_time->tv_usec <= end_time->tv_usec);
	}
	
	memset(&tmp, 0, sizeof(struct timeval));
	
	timersub(end_time, start_time, &tmp);

	timeradd(v, &tmp, v);
}

static double timeval_to_secs(const struct timeval* v)
{
	double s;

	s = v->tv_sec;
	s += v->tv_usec / (1000.0*1000.0);

	return s;
}

static void print_results( ThreadTest *d )
{
/*
  This is messy and should be rewritten but some of unixes, didn't
  understand all printf options and long long formats.
*/
	int i;    
	double totalBlocksWrite = 0, totalBlocksRead = 0, 
		totalBlocksRandomWrite = 0, totalBlocksRandomRead = 0;

	double read_rate,write_rate,random_read_rate,random_write_rate;
	struct timeval realtime_write, usrtime_write, systime_write;
	struct timeval realtime_rwrite, usrtime_rwrite, systime_rwrite;
	struct timeval realtime_read, usrtime_read, systime_read;
	struct timeval realtime_rread, usrtime_rread, systime_rread;

	double mbytesWrite, mbytesRandomWrite, mbytesRead, mbytesRandomRead;
	
	double avgWriteLat=0, avgRWriteLat=0, avgReadLat=0, avgRReadLat=0;
	double maxWriteLat=0, maxRWriteLat=0, maxReadLat=0, maxRReadLat=0;
	double countWriteLat=0, countRWriteLat=0, countReadLat=0, countRReadLat=0;
	double count1WriteLat=0, count1RWriteLat=0, count1ReadLat=0, 
		count1RReadLat=0;
	double count2WriteLat=0, count2RWriteLat=0, count2ReadLat=0, 
		count2RReadLat=0;
	double perc1WriteLat=0, perc1RWriteLat=0, perc1ReadLat=0, 
		perc1RReadLat=0;
	double perc2WriteLat=0, perc2RWriteLat=0, perc2ReadLat=0, 
		perc2RReadLat=0;
	double avgLat=0, maxLat=0, countLat=0, count1Lat=0, count2Lat=0,
		perc1Lat=0, perc2Lat=0;

	
	memset(&realtime_write, 0, sizeof(struct timeval));
	memset(&usrtime_write, 0, sizeof(struct timeval));
	memset(&systime_write, 0, sizeof(struct timeval));

	memset(&realtime_rwrite, 0, sizeof(struct timeval));
	memset(&usrtime_rwrite, 0, sizeof(struct timeval));
	memset(&systime_rwrite, 0, sizeof(struct timeval));

	memset(&realtime_read, 0, sizeof(struct timeval));
	memset(&usrtime_read, 0, sizeof(struct timeval));
	memset(&systime_read, 0, sizeof(struct timeval));

	memset(&realtime_rread, 0, sizeof(struct timeval));
	memset(&usrtime_rread, 0, sizeof(struct timeval));
	memset(&systime_rread, 0, sizeof(struct timeval));

	for(i = 0; i < d->numThreads; i++)
	{
		add_timer( &usrtime_write, &(d->threads[i].writeTimings.startUserTime), &(d->threads[i].writeTimings.stopUserTime) );
		add_timer( &systime_write, &(d->threads[i].writeTimings.startSysTime), &(d->threads[i].writeTimings.stopSysTime) );

		add_timer( &usrtime_rwrite, &(d->threads[i].randomWriteTimings.startUserTime), &(d->threads[i].randomWriteTimings.stopUserTime) );
		add_timer( &systime_rwrite, &(d->threads[i].randomWriteTimings.startSysTime), &(d->threads[i].randomWriteTimings.stopSysTime) );

		add_timer( &usrtime_read, &(d->threads[i].readTimings.startUserTime), &(d->threads[i].readTimings.stopUserTime) );
		add_timer( &systime_read, &(d->threads[i].readTimings.startSysTime), &(d->threads[i].readTimings.stopSysTime) );

		add_timer( &usrtime_rread, &(d->threads[i].randomReadTimings.startUserTime), &(d->threads[i].randomReadTimings.stopUserTime) );
		add_timer( &systime_rread, &(d->threads[i].randomReadTimings.startSysTime), &(d->threads[i].randomReadTimings.stopSysTime) );

		totalBlocksWrite       += d->threads[i].blocksWritten;
		totalBlocksRandomWrite += d->threads[i].blocksRandomWritten;
		totalBlocksRead        += d->threads[i].blocksRead; 
		totalBlocksRandomRead  += d->threads[i].blocksRandomRead;
		
		avgWriteLat += d->threads[i].writeLatency.avg;
		avgRWriteLat += d->threads[i].randomWriteLatency.avg;
		avgReadLat += d->threads[i].readLatency.avg;
		avgRReadLat += d->threads[i].randomReadLatency.avg;
		
		avgLat += d->threads[i].writeLatency.avg;
		avgLat += d->threads[i].randomWriteLatency.avg;
		avgLat += d->threads[i].readLatency.avg;
		avgLat += d->threads[i].randomReadLatency.avg;
		
		countWriteLat += d->threads[i].writeLatency.count;
		countRWriteLat += d->threads[i].randomWriteLatency.count;
		countReadLat += d->threads[i].readLatency.count;
		countRReadLat += d->threads[i].randomReadLatency.count;
		
		count1WriteLat += d->threads[i].writeLatency.count1;
		count1RWriteLat += d->threads[i].randomWriteLatency.count1;
		count1ReadLat += d->threads[i].readLatency.count1;
		count1RReadLat += d->threads[i].randomReadLatency.count1;
		
		count2WriteLat += d->threads[i].writeLatency.count2;
		count2RWriteLat += d->threads[i].randomWriteLatency.count2;
		count2ReadLat += d->threads[i].readLatency.count2;
		count2RReadLat += d->threads[i].randomReadLatency.count2;
		
		countLat += d->threads[i].writeLatency.count;
		countLat += d->threads[i].randomWriteLatency.count;
		countLat += d->threads[i].readLatency.count;
		countLat += d->threads[i].randomReadLatency.count;
		
		count1Lat += d->threads[i].writeLatency.count1;
		count1Lat += d->threads[i].randomWriteLatency.count1;
		count1Lat += d->threads[i].readLatency.count1;
		count1Lat += d->threads[i].randomReadLatency.count1;
		
		count2Lat += d->threads[i].writeLatency.count2;
		count2Lat += d->threads[i].randomWriteLatency.count2;
		count2Lat += d->threads[i].readLatency.count2;
		count2Lat += d->threads[i].randomReadLatency.count2;
		
		if (maxWriteLat < d->threads[i].writeLatency.max)
			maxWriteLat = d->threads[i].writeLatency.max;
		if (maxRWriteLat < d->threads[i].randomWriteLatency.max)
			maxRWriteLat = d->threads[i].randomWriteLatency.max;
		if (maxReadLat < d->threads[i].readLatency.max)
			maxReadLat = d->threads[i].readLatency.max;
		if (maxRReadLat < d->threads[i].randomReadLatency.max)
			maxRReadLat = d->threads[i].randomReadLatency.max;
			
		if (maxLat < maxWriteLat)
			maxLat = maxWriteLat;
		if (maxLat < maxRWriteLat)
			maxLat = maxRWriteLat;
		if (maxLat < maxReadLat)
			maxLat = maxReadLat;
		if (maxLat <maxRReadLat)
			maxLat = maxRReadLat;
	}

	if (countWriteLat > 0) 
	{	
		avgWriteLat /= countWriteLat;
		perc1WriteLat = count1WriteLat*100.0/countWriteLat;
		perc2WriteLat = count2WriteLat*100.0/countWriteLat;
	}
	else
		avgWriteLat = 0;

	if (countRWriteLat > 0)
	{
		avgRWriteLat /= countRWriteLat;
		perc1RWriteLat = count1RWriteLat*100.0/countRWriteLat;
		perc2RWriteLat = count2RWriteLat*100.0/countRWriteLat;
	}
	else
		avgRWriteLat = 0;

	if (countReadLat > 0)
	{
		avgReadLat /= countReadLat;
		perc1ReadLat = count1ReadLat*100.0/countReadLat;
		perc2ReadLat = count2ReadLat*100.0/countReadLat;
	}
	else
		avgReadLat = 0;

	if (countRReadLat > 0)
	{
		avgRReadLat /= countRReadLat;
		perc1RReadLat = count1RReadLat*100.0/countRReadLat;
		perc2RReadLat = count2RReadLat*100.0/countRReadLat;
	}
	else
		avgRReadLat = 0;

	if (countLat > 0)
	{
		avgLat /= countLat;
		perc1Lat = count1Lat*100.0/countLat;
		perc2Lat = count2Lat*100.0/countLat;
	}
	else
		avgLat = 0;
		
	mbytesWrite = totalBlocksWrite / 
		((double)MBYTE/(double)(d->threads[0].blockSize));
	mbytesRandomWrite = totalBlocksRandomWrite /
		((double)MBYTE/(double)(d->threads[0].blockSize));

	mbytesRead = totalBlocksRead / 
		((double)MBYTE/(double)(d->threads[0].blockSize));
	mbytesRandomRead = totalBlocksRandomRead / 
		((double)MBYTE/(double)(d->threads[0].blockSize));

	add_timer( &realtime_write, &(d->totalTimeWrite.startRealTime), &(d->totalTimeWrite.stopRealTime) );
	add_timer( &realtime_rwrite, &(d->totalTimeRandomWrite.startRealTime), &(d->totalTimeRandomWrite.stopRealTime) );
	add_timer( &realtime_read, &(d->totalTimeRead.startRealTime), &(d->totalTimeRead.stopRealTime) );
	add_timer( &realtime_rread, &(d->totalTimeRandomRead.startRealTime), &(d->totalTimeRandomRead.stopRealTime) );

	if(args.terse)
	{
		printf("write:%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f\n",
		       mbytesWrite, 
		       timeval_to_secs(&realtime_write), timeval_to_secs(&usrtime_write)/d->numThreads, timeval_to_secs(&systime_write)/d->numThreads,
		       avgWriteLat*1000, maxWriteLat*1000,
		       perc1WriteLat, perc2WriteLat );

		printf("rwrite:%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f\n",
		       mbytesRandomWrite, 
		       timeval_to_secs(&realtime_rwrite), timeval_to_secs(&usrtime_rwrite)/d->numThreads, timeval_to_secs(&systime_rwrite)/d->numThreads,
		       avgRWriteLat*1000, maxRWriteLat*1000,
		       perc1RWriteLat, perc2RWriteLat );

		printf("read:%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f\n",
		       mbytesRead, 
		       timeval_to_secs(&realtime_read), timeval_to_secs(&usrtime_read)/d->numThreads, timeval_to_secs(&systime_read)/d->numThreads,
		       avgReadLat*1000, maxReadLat*1000,
		       perc1ReadLat, perc2ReadLat );

		printf("rread:%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f\n",
		       mbytesRandomRead, 
		       timeval_to_secs(&realtime_rread), timeval_to_secs(&usrtime_rread)/d->numThreads, timeval_to_secs(&systime_rread)/d->numThreads,
		       avgRReadLat*1000, maxRReadLat*1000,
		       perc1RReadLat, perc2RReadLat );

		printf("total:%.5f,%.5f,%.5f,%.5f\n", 
		       avgLat*1000, maxLat*1000, perc1Lat, perc2Lat );

		return;
	}

	write_rate = mbytesWrite / timeval_to_secs(&realtime_write);
	random_write_rate = mbytesRandomWrite / timeval_to_secs(&realtime_rwrite);
 
	read_rate  = mbytesRead / timeval_to_secs(&realtime_read);
	random_read_rate  = mbytesRandomRead / timeval_to_secs(&realtime_rread);

	printf("Tiotest results for %d concurrent io threads:\n", 
	       d->numThreads);
	
	printf(",----------------------------------------------------------------------.\n");
	printf("| Item                  | Time     | Rate         | Usr CPU  | Sys CPU |\n");
	printf("+-----------------------+----------+--------------+----------+---------+\n");
    
	if(totalBlocksWrite)
		printf("| Write %11.0f MBs | %6.1f s | %7.3f MB/s | %5.1f %%  | %5.1f %% |\n",
		       mbytesWrite,
		       timeval_to_secs(&realtime_write),write_rate,
		       timeval_percentage_of(&usrtime_write, &realtime_write, d->numThreads),
		       timeval_percentage_of(&systime_write, &realtime_write, d->numThreads) );

	if(totalBlocksRandomWrite)
		printf("| Random Write %4.0f MBs | %6.1f s | %7.3f MB/s | %5.1f %%  | %5.1f %% |\n",
		       mbytesRandomWrite,
		       timeval_to_secs(&realtime_rwrite),random_write_rate,
		       timeval_percentage_of(&usrtime_rwrite, &realtime_rwrite, d->numThreads),
		       timeval_percentage_of(&systime_rwrite, &realtime_rwrite, d->numThreads) );
    
	if(totalBlocksRead)
		printf("| Read %12.0f MBs | %6.1f s | %7.3f MB/s | %5.1f %%  | %5.1f %% |\n",
		       mbytesRead,
		       timeval_to_secs(&realtime_read),read_rate,
		       timeval_percentage_of(&usrtime_read, &realtime_read, d->numThreads),
		       timeval_percentage_of(&systime_read, &realtime_read, d->numThreads) );

    
	if(totalBlocksRandomRead)
		printf("| Random Read %5.0f MBs | %6.1f s | %7.3f MB/s | %5.1f %%  | %5.1f %% |\n",
		       mbytesRandomRead,
		       timeval_to_secs(&realtime_rread),random_read_rate,
		       timeval_percentage_of(&usrtime_rread, &realtime_rread, d->numThreads),
		       timeval_percentage_of(&systime_rread, &realtime_rread, d->numThreads) );

	printf("`----------------------------------------------------------------------'\n");
	
	if (args.showLatency)
	{
		printf("Tiotest latency results:\n");
	
		printf(",-------------------------------------------------------------------------.\n");
		printf("| Item         | Average latency | Maximum latency | %% >%d sec | %% >%d sec |\n", 
		       LATENCY_STAT1, LATENCY_STAT2);
		printf("+--------------+-----------------+-----------------+----------+-----------+\n");
    
		if(totalBlocksWrite)
			printf("| Write        | %12.3f ms | %12.3f ms | %8.5f | %9.5f |\n",
			       avgWriteLat*1000, maxWriteLat*1000, perc1WriteLat,
			       perc2WriteLat);

		if(totalBlocksRandomWrite)
			printf("| Random Write | %12.3f ms | %12.3f ms | %8.5f | %9.5f |\n",
			       avgRWriteLat*1000, maxRWriteLat*1000, perc1RWriteLat,
			       perc2RWriteLat);
    
		if(totalBlocksRead)
			printf("| Read         | %12.3f ms | %12.3f ms | %8.5f | %9.5f |\n",
			       avgReadLat*1000, maxReadLat*1000, perc1ReadLat,
			       perc2ReadLat);
    
		if(totalBlocksRandomRead)
			printf("| Random Read  | %12.3f ms | %12.3f ms | %8.5f | %9.5f |\n",
			       avgRReadLat*1000, maxRReadLat*1000, perc1RReadLat,
			       perc2RReadLat);

		printf("|--------------+-----------------+-----------------+----------+-----------|\n");

		printf("| Total        | %12.3f ms | %12.3f ms | %8.5f | %9.5f |\n",
		       avgLat*1000, maxLat*1000, perc1Lat, perc2Lat);

		printf("`--------------+-----------------+-----------------+----------+-----------'\n\n");
	}
}

static void* do_generic_test(file_io_function io_func, mmap_io_function mmap_func, 
							 file_offset_function offset_func, mmap_loc_function loc_func, 
							 ThreadData *d, Timings *timings, Latencies *latencies,
							 int madvise_advice, unsigned long *blockCount, unsigned long io_ops)
{
	int     fd;
	TIO_off_t  blocks=((TIO_off_t)d->fileSizeInMBytes*MBYTE)/d->blockSize;
	unsigned int seed = get_random_seed();
	
	int     rc;
	TIO_off_t  bytesize=blocks*d->blockSize; /* truncates down to BS multiple */

	// for now, always read/write, just easier
	int openFlags = O_RDWR;

	// if not a pre-existing device, add create flag
	if (!args.rawDrives)
		openFlags |= O_CREAT;

	// if sync I/O requested, do it at open time
	if( args.syncWriting )
		openFlags |= O_SYNC;

#ifdef USE_LARGEFILES
	openFlags |= O_LARGEFILE;
#endif
    
	fd = open(d->fileName, openFlags, 0600 );
	if(fd == -1) {
		fprintf(stderr, "%s: %s\n", strerror(errno), d->fileName);
		return 0;
	}

	/* if doing real files, get them pre-allocated in size */
	if (!args.rawDrives) {
		t_log(LEVEL_DEBUG, "calling " xstr(TIO_ftruncate) "() on file descriptor");
		rc = TIO_ftruncate(fd, bytesize); /* pre-allocate space */
		if(rc != 0) {
			perror(xstr(TIO_ftruncate) "() failed");
			close(fd);
			return 0;
		}
	}

	timer_start( timings );

	if(args.use_mmap)
	{
		/**
		 * MEMORY-MAPPED OPERATIONS
		 */
		unsigned long chunk_num;
		// rounds the number of mmap chunks up, basically ceiling function
		TIO_off_t num_mmap_chunks = bytesize/MMAP_CHUNK_SIZE + 1;

		for(chunk_num=0; chunk_num < num_mmap_chunks; chunk_num++)
		{
			void *file_loc = NULL;
			long this_chunk_offset = d->fileOffset + chunk_num*MMAP_CHUNK_SIZE;
			long this_chunk_size = MIN(MMAP_CHUNK_SIZE, (TIO_off_t)bytesize - chunk_num*MMAP_CHUNK_SIZE);
			long this_chunk_blocks = this_chunk_size / d->blockSize;
			void *current_loc = NULL;

			file_loc=TIO_mmap(NULL,this_chunk_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,
							  this_chunk_offset);
			if(file_loc == MAP_FAILED) {
				fprintf(stderr, "this_chunk_size=%ld, fd=%d, offset=" OFFSET_FORMAT 
						"\n", this_chunk_size, fd, d->fileOffset);
				perror("Error " xstr(TIO_mmap) "()ing data file");
				close(fd);
				return 0;
			}

			madvise(file_loc, this_chunk_size, madvise_advice);

			current_loc = file_loc - d->blockSize; // back-one hack for sequential case
			while(io_ops--) {
				int ret;
				struct timeval tv_start, tv_stop;
				
				current_loc = (*loc_func)(file_loc, current_loc, d, &(seed));

				gettimeofday(&tv_start, NULL);

				ret = mmap_func(current_loc, d);
				if(ret != 0)
					exit(ret);

				if( args.syncWriting ) msync(current_loc, d->blockSize, MS_SYNC);

				gettimeofday(&tv_stop, NULL);
				update_latency_info(latencies, tv_start, tv_stop);
			} 
			
			(*blockCount) += this_chunk_blocks; // take this out of the for loop, we don't handle errors that well
			
			munmap(file_loc, this_chunk_size);
		}
	} else {
		/**
		 * REGULAR I/O OPERATIONS
		 */
		//TIO_off_t current_offset = d->fileOffset;
		TIO_off_t current_offset = d->fileOffset - d->blockSize; // back-one hack for sequential case

		while(io_ops--)
		{
			struct timeval tv_start, tv_stop;
			int ret;

			current_offset = (*offset_func)(current_offset, d, &(seed));

			gettimeofday(&tv_start, NULL);
			ret = (*io_func)(fd, current_offset, d);
			if(ret != 0)
				exit(ret);

			gettimeofday(&tv_stop, NULL);
			update_latency_info(latencies, tv_start, tv_stop);
		} 

		(*blockCount) += blocks; // take this out of the for loop, we don't handle errors that well
	}
	
	fsync(fd);

	close(fd);

	timer_stop( timings );

	return 0;
}

/*
 * p{write,read} functions
 */

// 
// define functions to get the next offset for the next I/O operation
//

static TIO_off_t get_sequential_offset(TIO_off_t current_offset, ThreadData *d, unsigned int *seed)
{
	return current_offset + d->blockSize;
}

static TIO_off_t get_random_offset(TIO_off_t current_offset, ThreadData *d, unsigned int *seed)
{
	TIO_off_t blocks=(d->fileSizeInMBytes*MBYTE/d->blockSize);
	TIO_off_t offset = get_random_number(blocks, seed) * d->blockSize;

	return d->fileOffset + offset;
}

// 
// define READ/WRITE operations on file descriptors
//

static int do_pread_operation(int fd, TIO_off_t offset, ThreadData *d)
{
	ssize_t rc = TIO_pread( fd, d->buffer, d->blockSize, offset );
	if( rc != d->blockSize ) {
		if( rc == -1 ) {
			perror("Error " xstr(TIO_pread) "()ing to file");
		} else {
			fprintf(stderr, "Tried to read %ld bytes from offset " OFFSET_FORMAT " of file %s of length " OFFSET_FORMAT ", but only read %d bytes\n", d->blockSize, offset, d->fileName, d->fileSizeInMBytes*MB, rc);
		}

		return -1;
	}
	else {
		if( args.consistencyCheckData )
		{
			const unsigned crc = crc32(d->buffer, d->blockSize, 0);

			if(crc != d->bufferCrc)
			{
				fprintf(stderr, "Thread(%lu) consistency check failed at offset %Lu\n", d->myNumber, (long long unsigned int)offset);
				return -1;
			}
		}
	}
	
	return 0;
}

static int do_pwrite_operation(int fd, TIO_off_t offset, ThreadData *d)
{
	ssize_t rc = TIO_pwrite( fd, d->buffer, d->blockSize, offset );
	if( rc  != d->blockSize ) {
		if( rc == -1 ) {
			perror("Error " xstr(TIO_pwrite) "()ing to file");
		} else {
			fprintf(stderr, "Tried to write %ld bytes from offset " OFFSET_FORMAT " of file %s of length " OFFSET_FORMAT ", but only wrote %d bytes\n", d->blockSize, offset, d->fileName, d->fileSizeInMBytes*MB, rc);
		}
		return -1;
	}

	return 0;
}

/*
 * MMAP functions
 */

// 
// define functions to get the next memory location for the next mmap operation
//

static void *get_sequential_loc(void *base_loc, void *current_loc, ThreadData *d, unsigned int *seed)
{
	return current_loc + d->blockSize;
}

static void *get_random_loc(void *base_loc, void *current_loc, ThreadData *d, unsigned int *seed)
{
	// limit ourselves to a single (the current) mmap chunk for now, just easier
	TIO_off_t max_bytes = MIN(MMAP_CHUNK_SIZE, d->fileSizeInMBytes*MBYTE);
	TIO_off_t blocks    = (max_bytes/d->blockSize);
	TIO_off_t offset    = get_random_number(blocks, seed) * d->blockSize;

	return base_loc + offset;
}

// 
// define functions to perform the next mmap-based read or write
//

static int do_mmap_read_operation(void *loc, ThreadData *d)
{
	memcpy(d->buffer, loc, d->blockSize);

	if( args.consistencyCheckData )
	{
		const unsigned crc = crc32(d->buffer, d->blockSize, 0);

		if(crc != d->bufferCrc)
		{
			fprintf(stderr, "Thread(%lu) mmap consistency check failed at 0x%x\n", d->myNumber, (unsigned int)loc);
			return -1;
		}
	}

	return 0;
}

static int do_mmap_write_operation(void *loc, ThreadData *d)
{
	memcpy(loc, d->buffer, d->blockSize);

	return 0;
}

static unsigned long get_number_of_blocks(ThreadData *d)
{
	return (d->fileSizeInMBytes * MB) / d->blockSize;
}

////////////////////////////////////////////////////////////////////////////////////
static void do_read_test( ThreadData *d )
{
	t_log(LEVEL_INFO, "Doing sequential read test");
	do_generic_test(do_pread_operation, do_mmap_read_operation,
					get_sequential_offset, get_sequential_loc,
					d, &(d->readTimings), &(d->readLatency),
					MADV_SEQUENTIAL, &(d->blocksRead), get_number_of_blocks(d));
}

void do_write_test( ThreadData *d )
{
	t_log(LEVEL_INFO, "Doing sequential write test");
	do_generic_test(do_pwrite_operation, do_mmap_write_operation, 
					get_sequential_offset, get_sequential_loc,
					d, &(d->writeTimings), &(d->writeLatency),
					MADV_SEQUENTIAL, &(d->blocksWritten), get_number_of_blocks(d));
}

void do_random_read_test( ThreadData *d )
{
	t_log(LEVEL_INFO, "Doing random read test");
	do_generic_test(do_pread_operation, do_mmap_read_operation, 
					get_random_offset, get_random_loc,
					d, &(d->randomReadTimings), &(d->randomReadLatency),
					MADV_RANDOM, &(d->blocksRandomRead), d->numRandomOps);
}

void do_random_write_test( ThreadData *d )
{
	t_log(LEVEL_INFO, "Doing random write test");
	do_generic_test(do_pwrite_operation, do_mmap_write_operation, 
					get_random_offset, get_random_loc,
					d, &(d->randomWriteTimings), &(d->randomWriteLatency),
					MADV_RANDOM, &(d->blocksRandomWritten), d->numRandomOps);
}

////////////////////////////////////////////////////////////////////////////////////

static unsigned int get_random_seed()
{
	unsigned int seed;
	struct timeval r;
    
	if(gettimeofday( &r, NULL ) == 0) {
		seed = r.tv_usec;
	} else {
		seed = 0xDEADBEEF;
	}

	return seed;
}

static TIO_off_t get_random_number(const TIO_off_t max, unsigned int *seed)
{
	unsigned long rr = rand_r(seed);

    // if it doesn't give us enough random bits, add some more
	if(RAND_MAX < max)
	{
		rr |= (rand_r(seed) << 16);
	}

	return (TIO_off_t) (rr % max);
}

static void timer_init(Timings *t)
{
	memset( t, 0, sizeof(Timings) );
}

static void timer_start(Timings *t)
{
	struct rusage ru;

	if(gettimeofday( &(t->startRealTime), NULL ))
	{
		perror("Error in timer_start from gettimeofday()\n");
		exit(10);
	}

	if(getrusage( RUSAGE_SELF, &ru ))
	{
		perror("Error in timer_start from getrusage()\n");
		exit(11);
	}

	memcpy( &(t->startUserTime), &(ru.ru_utime), sizeof( struct timeval ));
	memcpy( &(t->startSysTime), &(ru.ru_stime), sizeof( struct timeval ));
}

static void timer_stop(Timings *t)
{
	struct rusage ru;

	if( getrusage( RUSAGE_SELF, &ru ))
	{
		perror("Error in timer_stop from getrusage()\n");
		exit(11);
	}

	if(gettimeofday( &(t->stopRealTime), NULL ))
	{
		perror("Error in timer_stop from gettimeofday()\n");
		exit(10);
	}

	memcpy( &(t->stopUserTime), &(ru.ru_utime), sizeof( struct timeval ));
	memcpy( &(t->stopSysTime), &(ru.ru_stime), sizeof( struct timeval ));
}

static unsigned long long tv_to_usec(const struct timeval* v)
{
	return v->tv_sec * (1000*1000) + v->tv_usec;
}

static float timeval_percentage_of(const struct timeval* value, const struct timeval* from, unsigned int divider)
{
	unsigned long long v;
	float p;

	v = (100 * 100 * tv_to_usec(value)) / tv_to_usec(from);
	
	p = (float)v / (100.0 * divider);

	return p;
}
