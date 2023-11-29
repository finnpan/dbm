LIBRARYFILES = libtc.a libtc.so tcddb.so
LIBOBJFILES = tcutil.o tchdb.o tcbdb.o tcfdb.o tctdb.o tcadb.o tcconf.o tcmd5.o

CC = gcc
CPPFLAGS = -I. -DNDEBUG -D_GNU_SOURCE=1 -D_REENTRANT -D__EXTENSIONS__
CFLAGS = -g -O2 -std=c99 -Wall -fPIC -fsigned-char -O2
LDFLAGS = -L.
LIBS = -lbz2 -lz -lrt -lpthread -lm -lc


.SUFFIXES :
.SUFFIXES : .c .o

.c.o :
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<


all : $(LIBRARYFILES)
	@printf '\n'
	@printf '#================================================================\n'
	@printf '# Complete.\n'
	@printf '#================================================================\n'


clean :
	rm -rf $(LIBRARYFILES) $(LIBOBJFILES)


.PHONY : all clean


libtc.a : $(LIBOBJFILES)
	$(AR) $(ARFLAGS) $@ $(LIBOBJFILES)


libtc.so : $(LIBOBJFILES)
	$(CC) $(CFLAGS) -shared -o $@ $(LIBOBJFILES) $(LDFLAGS) $(LIBS)


tcddb.so : tcddb.o
	$(CC) $(CFLAGS) -shared -o $@ $< $(LDFLAGS) $(LIBS)


tcmd5.o : tcmd5.h

tcconf.o : tcconf.h

tcutil.o : tcconf.h tcutil.h tcmd5.h

tchdb.o : tcconf.h tcutil.h tchdb.h

tcbdb.o : tcconf.h tcutil.h tchdb.h tcbdb.h

tcfdb.o : tcconf.h tcutil.h tcfdb.h

tctdb.o : tcconf.h tcutil.h tchdb.h tctdb.h

tcadb.o : tcconf.h tcutil.h tchdb.h tcbdb.h tcfdb.h tctdb.h tcadb.h
