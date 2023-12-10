LIBRARYFILES = libtc.a libtc.so tcddb.so
LIBOBJFILES = tcutil.o tchdb.o tcbdb.o tcadb.o tcconf.o

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
	rm -rf $(LIBRARYFILES) $(LIBOBJFILES) tcddb.o


.PHONY : all clean


libtc.a : $(LIBOBJFILES)
	$(AR) $(ARFLAGS) $@ $(LIBOBJFILES)


libtc.so : $(LIBOBJFILES)
	$(CC) $(CFLAGS) -shared -o $@ $(LIBOBJFILES) $(LDFLAGS) $(LIBS)


tcddb.so : tcddb.o
	$(CC) $(CFLAGS) -shared -o $@ $< $(LDFLAGS) $(LIBS)


tcconf.o : tcconf.h

tcutil.o : tcconf.h tcutil.h

tchdb.o : tcconf.h tcutil.h tchdb.h

tcbdb.o : tcconf.h tcutil.h tchdb.h tcbdb.h

tcadb.o : tcconf.h tcutil.h tchdb.h tcbdb.h tcadb.h
