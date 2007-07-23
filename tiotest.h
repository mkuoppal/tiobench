/*
 *    Threaded io test
 *
 *  Copyright (C) 1999-2000 Mika Kuoppala <miku at iki.fi>
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

#ifndef TIOTEST_H
#define TIOTEST_H

#include "constants.h"

static void    print_help_and_exit();

static void   do_write_test( ThreadData *d );
static void   do_read_test( ThreadData *d );
static void   do_random_read_test( ThreadData *d );
static void   do_random_write_test( ThreadData *d );

static void    initialize_test( ThreadTest *d  );

static void    cleanup_test( ThreadTest *d );
static void 	do_test( ThreadTest *test, int testCase, int sequential,
		Timings *t, char *debugMessage );
static void    print_results( ThreadTest *threadTest );
static void    do_tests( ThreadTest *d );

static void    timer_init(Timings *t);
static void    timer_start(Timings *t);
static void    timer_stop(Timings *t);

static unsigned int get_random_seed();

static TIO_off_t get_random_number(const TIO_off_t filesize, unsigned int *seed);

static void    parse_args( ArgumentOptions* args, int argc, char *argv[] );

typedef void (*TestFunc)(ThreadData *);

// operation functions
static int do_pwrite_operation(int fd, TIO_off_t offset, ThreadData *d);
static int do_pread_operation(int fd, TIO_off_t offset, ThreadData *d);
static int do_mmap_read_operation(void *loc, ThreadData *d);
static int do_mmap_write_operation(void *loc, ThreadData *d);

// offset functions
static TIO_off_t get_sequential_offset(TIO_off_t current_offset, ThreadData *d, unsigned int *seed);
static TIO_off_t get_random_offset(TIO_off_t current_offset, ThreadData *d, unsigned int *seed);
static void *get_sequential_loc(void *base_loc, void *current_loc, ThreadData *d, unsigned int *seed);
static void *get_random_loc(void *base_loc, void *current_loc, ThreadData *d, unsigned int *seed);

#define WRITE_TEST         0
#define RANDOM_WRITE_TEST  1
#define READ_TEST          2
#define RANDOM_READ_TEST   3

TestFunc Tests[TESTS_COUNT+1] = { 
    do_write_test, 
    do_random_write_test, 
    do_read_test, 
    do_random_read_test, 
    0 };

#endif /* TIOTEST_H */
