# Makefile for tiotest

CC=gcc
#CFLAGS=-O3 -fomit-frame-pointer -Wall
CFLAGS=-O2 -Wall
#CFLAGS=-O0 -g -Wall

# This enables support for 64bit file offsets, allowing
# possibility to test with files larger than (2^31-1) bytes.

DEFINES=-DUSE_LARGEFILES

# This define is for Solaris and others where getrusage returns 
# in process scope despite of threads
# DEFINES=-DGETRUSAGE_PROCESS_SCOPE

LINK=gcc
TIOTEST=tiotest
TEST_LARGE=test_largefiles
PROJECT=tiobench
# do it once instead of each time referenced
VERSION=$(shell egrep "tiotest v[0-9]+.[0-9]+" tiotest.c | cut -d " " -f 8 | sed "s/v//g")
DISTNAME=$(PROJECT)-$(VERSION)
INSTALL=install
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
DOCDIR=/usr/local/doc/$(DISTNAME)

all: $(TEST_LARGE) $(TIOTEST)

crc32.o: crc32.c crc32.h
	$(CC) -c $(CFLAGS) $(DEFINES) crc32.c -o crc32.o

tiotest.o: tiotest.c tiotest.h crc32.h crc32.c Makefile constants.h
	$(CC) -c $(CFLAGS) $(DEFINES) tiotest.c -o tiotest.o

test_largefiles.o: tiotest.h test_largefiles.c
	$(CC) -c -DUSE_LARGEFILES $(CFLAGS) $(DEFINES) test_largefiles.c -o test_largefiles.o

$(TIOTEST): tiotest.o crc32.o
	$(LINK) -o $(TIOTEST) tiotest.o crc32.o -lpthread
	@echo
	@echo "./tiobench.pl --help for usage options"
	@echo

$(TEST_LARGE): test_largefiles.o
	$(LINK) -o $(TEST_LARGE) test_largefiles.o

clean:
	rm -f test_largefiles.o tiotest.o crc32.o $(TIOTEST) $(TEST_LARGE) core

dist:
	ln -s . $(DISTNAME)
	tar -zcvf $(DISTNAME).tar.gz $(DISTNAME)/*.c $(DISTNAME)/*.h $(DISTNAME)/Makefile $(DISTNAME)/COPYING $(DISTNAME)/README $(DISTNAME)/TODO $(DISTNAME)/ChangeLog $(DISTNAME)/BUGS $(DISTNAME)/tiobench.pl $(DISTNAME)/tiosum.pl $(DISTNAME)/scripts
	rm $(DISTNAME)

install:
	if [ ! -d $(BINDIR) ]; then \
		mkdir -p $(BINDIR); \
	fi;
	if [ ! -d $(DOCDIR) ]; then \
		mkdir -p $(DOCDIR); \
	fi;
	$(INSTALL) tiotest $(BINDIR)
	$(INSTALL) tiobench.pl $(BINDIR)
	$(INSTALL) tiosum.pl $(BINDIR)
	$(INSTALL) README $(DOCDIR)
	$(INSTALL) BUGS $(DOCDIR)
	$(INSTALL) COPYING $(DOCDIR)
	$(INSTALL) ChangeLog $(DOCDIR)
	$(INSTALL) TODO $(DOCDIR)

uninstall:
	rm -f $(BINDIR)/tiotest
	rm -f $(BINDIR)/tiobench.pl
	rm -f $(BINDIR)/tiosum.pl
	rm -rf $(DOCDIR)

