# Makefile for tiotest

CC=gcc
#CFLAGS=-O3 -fomit-frame-pointer -Wall
CFLAGS=-O2 -Wall

#DEFINES=-DUSE_MMAP -DUSE_MADVISE

# This enables support for 64bit file offsets, allowing
# possibility to test with files larger than (2^31) bytes.

#DEFINES=-DLARGEFILES

DEFINES=
LINK=gcc
EXE=tiotest
PROJECT=tiobench
VERSION=`egrep "tiotest v[0-9]+.[0-9]+" tiotest.c | cut -d " " -f 7 | sed "s/v//g"`
DISTNAME=$(PROJECT)-$(VERSION)
INSTALL=install
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
DOCDIR=/usr/local/doc/$(DISTNAME)

all: $(EXE)

tiotest.o: tiotest.c tiotest.h Makefile
	$(CC) -c $(CFLAGS) $(DEFINES) tiotest.c -o tiotest.o

$(EXE): tiotest.o
	$(LINK) -o $(EXE) tiotest.o -lpthread
	@echo
	@echo "./tiobench.pl --help for usage options"
	@echo

clean:
	rm -f tiotest.o $(EXE) core

dist:
	ln -s . $(DISTNAME)
	tar -zcvf $(DISTNAME).tar.gz $(DISTNAME)/*.c $(DISTNAME)/*.h $(DISTNAME)/Makefile $(DISTNAME)/COPYING $(DISTNAME)/README $(DISTNAME)/TODO $(DISTNAME)/ChangeLog $(DISTNAME)/BUGS $(DISTNAME)/tiobench.pl $(DISTNAME)/scripts
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
	$(INSTALL) README $(DOCDIR)
	$(INSTALL) BUGS $(DOCDIR)
	$(INSTALL) COPYING $(DOCDIR)
	$(INSTALL) ChangeLog $(DOCDIR)
	$(INSTALL) TODO $(DOCDIR)

uninstall:
	rm -f $(BINDIR)/tiotest
	rm -f $(BINDIR)/tiobench.pl
	rm -rf $(DOCDIR)
