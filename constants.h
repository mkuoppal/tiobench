#ifndef CONSTANTS_H
#define CONSTANTS_H

#define KB (1024)
#define MB (1024*1024)
#define GB (1024*1024*1024)

#ifdef USE_LARGEFILES 
#define __USE_LARGEFILE64
#define _LARGEFILE64_SOURCE 
#endif

#define _GNU_SOURCE
#define _REENTRANT
#define _THREAD_SAFE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <values.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>

#ifdef USE_LARGEFILES
#ifndef _LFS64_LARGEFILE
#error no large file support available, remove -DLARGEFILES from makefile
#endif
#endif

#if (USE_LARGEFILES && USE_MMAP)
#warning LARGEFILES and USE_MMAP does not currently work on 32-bit architectures, may need to remove USE_MMAP from Makefile
#endif


#ifdef LONG_OPTIONS
#include <getopt.h>
#endif

#define TESTS_COUNT            4

/* should be in sync with perl script */
#define LEVEL_NONE             0
#define LEVEL_FATAL            10
#define LEVEL_ERROR            20
#define LEVEL_WARN             30
#define LEVEL_INFO             40
#define LEVEL_DEBUG            50
#define LEVEL_TRACE            60

#define DEFAULT_DEBUG_LEVEL    (LEVEL_NONE)

#define LATENCY_STAT1          2
#define LATENCY_STAT2          10

#define MAX_PATHS              50

#define KBYTE                  1024
#define MBYTE                  (1024*KBYTE)
#define PAGE_SIZE              (4096)

#define DEFAULT_FILESIZE       (10) /* In Megs !!! */
#define DEFAULT_THREADS        4
#define DEFAULT_RANDOM_OPS     1000
#define DEFAULT_DIRECTORY      "."
#define DEFAULT_BLOCKSIZE      (4*KBYTE)
#define DEFAULT_RAW_OFFSET     0

#define TRUE                   1
#define FALSE                  0

#define xstr(s) str(s)
#define str(s) #s

/* NOTE: a lot of this could be done with __REDIRECT and unistd.h */
#ifdef USE_LARGEFILES
typedef off64_t        TIO_off_t;
#define TIO_lseek      lseek64
#define TIO_mmap       mmap64
#define TIO_ftruncate  ftruncate64
#define OFFSET_FORMAT  "%Lx"
#else
typedef off_t          TIO_off_t;
#define TIO_lseek      lseek
#define TIO_mmap       mmap
#define TIO_ftruncate  ftruncate
#define OFFSET_FORMAT  "%lx"
#endif

typedef struct {
	struct timeval startRealTime;
	struct timeval startUserTime;
	struct timeval startSysTime;

	struct timeval stopRealTime;
	struct timeval stopUserTime;
	struct timeval stopSysTime;
} Timings;

typedef struct {
	double avg, max;
	int count, count1, count2;
} Latencies;

typedef struct {

	pthread_t        thread;
	pthread_attr_t   thread_attr;
    
	char             fileName[KBYTE];
	TIO_off_t        fileSizeInMBytes;
	TIO_off_t        fileOffset;
	unsigned long    numRandomOps;

	unsigned long    blockSize;
	unsigned char*   buffer;
	unsigned long    bufferCrc;

	unsigned long    myNumber;

	unsigned long    blocksWritten;
	Timings          writeTimings;
	Latencies	     writeLatency;

	unsigned long    blocksRandomWritten;
	Timings          randomWriteTimings;
	Latencies	     randomWriteLatency;

	unsigned long    blocksRead;
	Timings          readTimings;
	Latencies	     readLatency;

	unsigned long    blocksRandomRead;
	Timings          randomReadTimings;
	Latencies	     randomReadLatency;

} ThreadData;

typedef struct {
    
	ThreadData* threads;
	int         numThreads;
    
	Timings totalTimeWrite;
	Timings totalTimeRandomWrite;
	Timings totalTimeRead;
	Timings totalTimeRandomRead;

} ThreadTest;

typedef struct {
	
	char     path[MAX_PATHS][KBYTE];
	int      pathsCount;
	int      fileSizeInMBytes;
	int      numThreads;
	int      blockSize;
	int      numRandomOps;
	int      verbose;
	int      terse;
	int      use_mmap;
	int      sequentialWriting;
	int      syncWriting;
	int	     rawDrives;
	int      consistencyCheckData;
	int      showLatency;
	long	 threadOffset;
	int	     useThreadOffsetForFirstThread;
	
	int	     testsToRun[TESTS_COUNT];
	int	     runRandomWrite;
	int	     runRead;
	int	     runRandomRead;

	/*
	  Debug level
	  This should be from 0 - 10
	*/
	int      debugLevel;

} ArgumentOptions;

#endif /* CONSTANTS_H */
