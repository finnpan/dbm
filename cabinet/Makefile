VERSION = 1.4.48
LIBVER = 9
LIBREV = 11

# Targets
LIBRARYFILES = libtokyocabinet.a libtokyocabinet.so.9.11.0 libtokyocabinet.so.9 libtokyocabinet.so
LIBOBJFILES = tcutil.o tchdb.o tcbdb.o tcfdb.o tctdb.o tcadb.o myconf.o md5.o

# Install destinations
prefix = /usr/local
exec_prefix = ${prefix}
INCLUDEDIR = ${prefix}/include
LIBDIR = ${exec_prefix}/lib
BINDIR = ${exec_prefix}/bin
LIBEXECDIR = ${exec_prefix}/libexec

# Building configuration
CC = gcc
CPPFLAGS = -I. -I$(INCLUDEDIR) -I/home/fpan/include -I/usr/local/include -DNDEBUG -D_GNU_SOURCE=1 -D_REENTRANT -D__EXTENSIONS__ \
  -D_TC_PREFIX="\"$(prefix)\"" -D_TC_INCLUDEDIR="\"$(INCLUDEDIR)\"" \
  -D_TC_LIBDIR="\"$(LIBDIR)\"" -D_TC_BINDIR="\"$(BINDIR)\"" -D_TC_LIBEXECDIR="\"$(LIBEXECDIR)\"" \
  -D_TC_APPINC="\"-I$(INCLUDEDIR)\"" -D_TC_APPLIBS="\"-L$(LIBDIR) -ltokyocabinet -lbz2 -lz -lrt -lpthread -lm -lc \""
CFLAGS = -g -O2 -std=c99 -Wall -fPIC -fsigned-char -O2
LDFLAGS = -L. -L$(LIBDIR) -L/home/fpan/lib -L/usr/local/lib
LIBS = -lbz2 -lz -lrt -lpthread -lm -lc
LDENV = LD_RUN_PATH=/lib:/usr/lib:$(LIBDIR):$(HOME)/lib:/usr/local/lib:$(LIBDIR):.
RUNENV = LD_LIBRARY_PATH=.:/lib:/usr/lib:$(LIBDIR):$(HOME)/lib:/usr/local/lib:$(LIBDIR)


.SUFFIXES :
.SUFFIXES : .c .o

.c.o :
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<


all : $(LIBRARYFILES)
	@printf '\n'
	@printf '#================================================================\n'
	@printf '# Ready to install.\n'
	@printf '#================================================================\n'


clean :
	rm -rf $(LIBRARYFILES) $(LIBOBJFILES) \
	  *.o a.out check.in check.out gmon.out *.vlog words.tsv \
	  casket casket-* casket.* *.tch *.tcb *.tcf *.tct *.idx.* *.wal *~ hoge moge tako ika


.PHONY : all clean install check


libtokyocabinet.a : $(LIBOBJFILES)
	$(AR) $(ARFLAGS) $@ $(LIBOBJFILES)


libtokyocabinet.so.$(LIBVER).$(LIBREV).0 : $(LIBOBJFILES)
	if uname -a | egrep -i 'SunOS' > /dev/null ; \
	  then \
	    $(CC) $(CFLAGS) -shared -Wl,-G,-h,libtokyocabinet.so.$(LIBVER) -o $@ \
	      $(LIBOBJFILES) $(LDFLAGS) $(LIBS) ; \
	  else \
	    $(CC) $(CFLAGS) -shared -Wl,-soname,libtokyocabinet.so.$(LIBVER) -o $@ \
	      $(LIBOBJFILES) $(LDFLAGS) $(LIBS) ; \
	  fi


libtokyocabinet.so.$(LIBVER) : libtokyocabinet.so.$(LIBVER).$(LIBREV).0
	ln -f -s libtokyocabinet.so.$(LIBVER).$(LIBREV).0 $@


libtokyocabinet.so : libtokyocabinet.so.$(LIBVER).$(LIBREV).0
	ln -f -s libtokyocabinet.so.$(LIBVER).$(LIBREV).0 $@


myconf.o : myconf.h

tcutil.o : myconf.h tcutil.h md5.h

tchdb.o : myconf.h tcutil.h tchdb.h

tcbdb.o : myconf.h tcutil.h tchdb.h tcbdb.h

tcfdb.o : myconf.h tcutil.h tcfdb.h

tctdb.o : myconf.h tcutil.h tchdb.h tctdb.h

tcadb.o : myconf.h tcutil.h tchdb.h tcbdb.h tcfdb.h tctdb.h tcadb.h
