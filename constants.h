#ifndef CONSTANTS_H
#define CONSTANTS_H

#define KB (1024)
#define MB (1024*1024)
#define GB (1024*1024*1024)

#ifdef USE_LARGEFILES
#define _LARGEFILE64_SOURCE
#define _LARGEFILE_SOURCE
#define _XOPEN_SOURCE
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

#ifdef LONG_OPTIONS
#include <getopt.h>
#endif

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
#define TIO_pread      pread64
#define TIO_pwrite     pwrite64
#define OFFSET_FORMAT  "0x%Lx"
#else
typedef off_t          TIO_off_t;
#define TIO_lseek      lseek
#define TIO_mmap       mmap
#define TIO_ftruncate  ftruncate
#define TIO_pread      pread
#define TIO_pwrite     pwrite
#define OFFSET_FORMAT  "0x%lx"
#endif


#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MMAP_CHUNK_SIZE 1073741824uLL

#endif /* CONSTANTS_H */
