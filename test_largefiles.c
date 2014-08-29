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

#define TEST_DATA1 (int)0xCAFEBABE
#define TEST_DATA2 (int)0xDEADBEEF

static const char* versionStr = "v0.4.2";

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
    TIO_off_t offset_ret;
    int data;
    ssize_t count;

    printf("unlink()ing large test file %s\n", LARGEFILE_NAME);
    rc = unlink(LARGEFILE_NAME);
    printf("Creating large test file %s\n", LARGEFILE_NAME);
    fd = open(LARGEFILE_NAME, O_RDWR | O_CREAT | O_TRUNC | O_LARGEFILE, 0600 );
    if(fd == -1)
    {
        perror("open() failed");
        exit(1);
    }

    printf( xstr(TIO_ftruncate) "()'ing large test file to size %Lx\n", LARGEFILE_SIZE);
    rc = TIO_ftruncate(fd, LARGEFILE_SIZE); /* pre-allocate space */
    if(rc != 0) 
    {
        perror(xstr(TIO_ftruncate) "() failed");
        exit(1);
    }

    for(offset = (TIO_off_t)0; offset + CHUNK_SIZE <= LARGEFILE_SIZE; offset += CHUNK_SIZE) {

        printf(xstr(TIO_lseek) "()ing to offset %Lx\n", offset);
        offset_ret = TIO_lseek(fd, offset, SEEK_SET);
        if (offset_ret != offset) {
            perror("Error " xstr(TIO_lseek) "()ing");
            exit(-1);
        }

        printf("read()ing a chunk of data\n");
        count = read(fd, &data, sizeof(data));
        if (count != sizeof(data)) {
            fprintf(stderr, "Error read()ing, %d byte(s) read (!= %d)\n", count, sizeof(data));
            exit(-1);
        }
        if (data != 0) {
            fprintf(stderr, "Error read()ing, data was not null (was %x)\n", data);
            exit(-1);
        }

        printf(xstr(TIO_pread)"()ing a data value\n");
        count = TIO_pread(fd, &data, sizeof(data), offset);
        if (count != sizeof(data)) {
            fprintf(stderr, "Error " xstr(TIO_pread) "()ing, %d byte(s) read (!= %d)\n", count, sizeof(data));
            exit(-1);
        }
        if (data != 0) {
            fprintf(stderr, "Error " xstr(TIO_pread) "()ing, data was not null (was %x)\n", data);
            exit(-1);
        }

        data = TEST_DATA1;
        printf(xstr(TIO_pwrite)"()ing a test data value (%x)\n", TEST_DATA1);
        count = TIO_pwrite(fd, &data, sizeof(data), offset);
        if (count != sizeof(data)) {
            fprintf(stderr, "Error " xstr(TIO_pwrite) "()ing, %d bytes written (!= %d)\n", count, sizeof(data));
            exit(-1);
        }
        data = 0;

        printf(xstr(TIO_mmap) "()ing chunk of size %Lx at offset %Lx\n", CHUNK_SIZE, offset);
        file_loc = TIO_mmap((caddr_t )0, CHUNK_SIZE,
                 PROT_READ | PROT_WRITE, MAP_SHARED, fd, (TIO_off_t)offset);
        if (file_loc == MAP_FAILED) {
            perror("Error " xstr(TIO_mmap) "()ing data file chunk");
            exit(-1);
        }

        printf("madvise()ing to MADV_SEQUENTIAL\n");
        rc = madvise(file_loc,CHUNK_SIZE,MADV_SEQUENTIAL);
        if (rc != 0) {
            perror("Error madvise()ing memory area");
            exit(-1);
        }

        printf("checking for test data chunk (%x) in memory map\n", TEST_DATA1);
        data = *((int *)file_loc);
        if (data != TEST_DATA1) {
            fprintf(stderr, "Error, test data was wrong (%x)\n", data);
            exit(-1);
        }
        data = 0;

        printf("writing test data chunk (%x) in memory map\n", TEST_DATA2);
        *((int *)(file_loc+sizeof(data))) = TEST_DATA2;

        printf("msync()ing\n");
        rc = msync(file_loc,CHUNK_SIZE,MS_SYNC);
        if (rc != 0) {
            perror("Error msync()ing memory area");
            exit(-1);
        }

        printf("munmap()ing chunk\n");
        rc = munmap(file_loc,CHUNK_SIZE);
        if (rc != 0) {
            perror("Error munmap()ing memory area");
            exit(-1);
        }

        printf(xstr(TIO_pread)"()ing a data value, checking value == %x\n", TEST_DATA2);
        count = TIO_pread(fd, &data, sizeof(data), offset+sizeof(data));
        if (count != sizeof(data)) {
            fprintf(stderr, "Error " xstr(TIO_pread) "()ing, %d byte(s) read (!= %d)\n", count, sizeof(data));
            exit(-1);
        }
        if (data != TEST_DATA2) {
            fprintf(stderr, "Error " xstr(TIO_pread) "()ing, data was wrong (was %x)\n", data);
            exit(-1);
        }

        printf("\n");
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
