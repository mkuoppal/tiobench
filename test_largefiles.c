/*
 *    Largefile support tester
 *
 *  Copyright (C) 2003 tiobench team <http://tiobench.sf.net/>
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

#include "constants.h"

#define LARGEFILE_NAME "large_file_test.dat"
#define LARGEFILE_SIZE (TIO_off_t)5*GB
#define CHUNK_SIZE (TIO_off_t)1*GB

static const char* versionStr = "$Id$ (C) 2003 tiobench team <http://tiobench.sf.net/>";

void print_version()
{
        printf("%s\n", versionStr);
        exit(0);
}

int main(int argc, char *argv[])
{
    int fd;
    int rc;
    void *file_loc;
    TIO_off_t offset;

    printf("unlink()ing large test file %s\n", LARGEFILE_NAME);
    rc = unlink(LARGEFILE_NAME);
    printf("Creating large test file %s\n", LARGEFILE_NAME);
    fd = open(LARGEFILE_NAME, O_RDWR | O_CREAT | O_TRUNC | O_LARGEFILE, 0600 );
    if(fd == -1)
    {
        perror("open() failed");
        exit(1);
    }

    printf( xstr(TIO_ftruncate) "()'ing large test file to size %Ld\n", LARGEFILE_SIZE);
    rc = TIO_ftruncate(fd,LARGEFILE_SIZE); /* pre-allocate space */
    if(rc != 0) 
    {
        perror(xstr(TIO_ftruncate) "() failed");
        exit(1);
    }

    for(offset = (TIO_off_t)0; offset + CHUNK_SIZE <= LARGEFILE_SIZE; offset += CHUNK_SIZE) {

        printf(xstr(TIO_mmap) "()ing chunk of size %Ld at offset %Ld\n", CHUNK_SIZE, offset);
        file_loc = TIO_mmap((caddr_t )0, CHUNK_SIZE,
                 PROT_READ | PROT_WRITE, MAP_SHARED, fd, (TIO_off_t)offset);
        if (file_loc == MAP_FAILED) {
            perror("Error " xstr(TIO_mmap) "()ing data file chunk");
            exit(-1);
        }

        printf("  madvise()ing to MADV_RANDOM\n");
        rc = madvise(file_loc,CHUNK_SIZE,MADV_RANDOM);
        if (rc != 0) {
            perror("Error madvise()ing memory area #1");
            exit(-1);
        }

        printf("  madvise()ing to MADV_SEQUENTIAL\n");
        rc = madvise(file_loc,CHUNK_SIZE,MADV_SEQUENTIAL);
        if (rc != 0) {
            perror("Error madvise()ing memory area #1");
            exit(-1);
        }

        printf("  munmap()ing chunk\n");
        rc = munmap(file_loc,CHUNK_SIZE);
        if (rc != 0) {
            perror("Error munmap()ing memory area");
            exit(-1);
        }
    }

    printf("unlink()ing large test file %s\n", LARGEFILE_NAME);
    rc = unlink(LARGEFILE_NAME);
    if (rc != 0) {
        perror("Error unlink()ing memory area");
        exit(-1);
    }
    printf("All large-file operations work fine\n");
    exit(0);
}
