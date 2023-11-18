# Package information
PACKAGE = tokyotyrant
VERSION = 1.1.41
LIBVER = 3
LIBREV = 24

# Targets
LIBRARYFILES = libtokyotyrant.a libtokyotyrant.so.3.24.0 libtokyotyrant.so.3 libtokyotyrant.so ttskelmock.so ttskeldir.so ttskelproxy.so ttskelnull.so
LIBOBJFILES = ttutil.o tculog.o tcrdb.o myconf.o
COMMANDFILES = ttserver ttulmgr ttultest tcrtest tcrmttest tcrmgr


# Install destinations
prefix = /usr/local
exec_prefix = ${prefix}
INCLUDEDIR = ${prefix}/include
LIBDIR = ${exec_prefix}/lib
BINDIR = ${exec_prefix}/bin
LIBEXECDIR = ${exec_prefix}/libexec


# Building configuration
CC = gcc
CPPFLAGS = -I../cabinet -I. -I$(INCLUDEDIR) -I/home/fpan/include -I/usr/local/include \
  -DNDEBUG -D_GNU_SOURCE=1 -D_REENTRANT -D__EXTENSIONS__ \
  -D_TT_PREFIX="\"$(prefix)\"" -D_TT_INCLUDEDIR="\"$(INCLUDEDIR)\"" \
  -D_TT_LIBDIR="\"$(LIBDIR)\"" -D_TT_BINDIR="\"$(BINDIR)\"" -D_TT_LIBEXECDIR="\"$(LIBEXECDIR)\"" \
  -D_TT_APPINC="\"-I$(INCLUDEDIR)\"" -D_TT_APPLIBS="\"-L$(LIBDIR) \
  -ltokyotyrant -ltokyocabinet -lbz2 -lz -lresolv -lnsl -ldl -lrt -lpthread -lm -lc \""
CFLAGS = -g -O2 -std=c99 -Wall -fPIC -fsigned-char -O2
LDFLAGS = -L../cabinet -L. -L$(LIBDIR) -L/home/fpan/lib -L/usr/local/lib
LIBS = -ltokyocabinet -lbz2 -lz -lresolv -lnsl -ldl -lrt -lpthread -lm -lc
LDENV = LD_RUN_PATH=../cabinet:/lib:/usr/lib:$(LIBDIR):$(HOME)/lib:/usr/local/lib:$(LIBDIR):.
RUNENV = LD_LIBRARY_PATH=../cabinet:.:/lib:/usr/lib:$(LIBDIR):$(HOME)/lib:/usr/local/lib:$(LIBDIR)


.SUFFIXES :
.SUFFIXES : .c .o

.c.o :
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<


all : $(LIBRARYFILES) $(COMMANDFILES)
	@printf '\n'
	@printf '#================================================================\n'
	@printf '# Ready to install.\n'
	@printf '#================================================================\n'


clean :
	rm -rf $(LIBRARYFILES) $(LIBOBJFILES) $(COMMANDFILES) \
	  *.o a.out check.in check.out gmon.out *.vlog words.tsv \
	  casket casket-* casket.* *.tch *.tcb *.tcf *.tct *.idx.* \
	  *.ulog ulog 1978* 1979* *.rts *.pid *~ hoge moge tako ika


check :
	$(RUNENV) $(RUNCMD) ./tcrmgr version
	$(RUNENV) $(RUNCMD) ./tcrtest write -cnum 5 -tout 5 -rnd 127.0.0.1 50000
	$(RUNENV) $(RUNCMD) ./tcrtest write -cnum 5 -tout 5 -nr -rnd 127.0.0.1 50000
	$(RUNENV) $(RUNCMD) ./tcrtest write -cnum 5 -tout 5 127.0.0.1 50000
	$(RUNENV) $(RUNCMD) ./tcrtest read -cnum 5 -tout 5 127.0.0.1
	$(RUNENV) $(RUNCMD) ./tcrtest read -cnum 5 -tout 5 -mul 5 127.0.0.1
	$(RUNENV) $(RUNCMD) ./tcrtest remove -cnum 5 -tout 5 127.0.0.1
	$(RUNENV) $(RUNCMD) ./tcrtest rcat -cnum 5 -tout 5 127.0.0.1 50000
	$(RUNENV) $(RUNCMD) ./tcrtest rcat -cnum 5 -tout 5 -shl 50 127.0.0.1 50000
	$(RUNENV) $(RUNCMD) ./tcrmgr vanish 127.0.0.1
	$(RUNENV) $(RUNCMD) ./tcrtest rcat -cnum 5 -tout 5 -dad 127.0.0.1 50000
	$(RUNENV) $(RUNCMD) ./tcrtest rcat -cnum 5 -tout 5 -ext putcat -xlr 127.0.0.1 50000
	$(RUNENV) $(RUNCMD) ./tcrtest misc -cnum 5 -tout 5 127.0.0.1 5000
	$(RUNENV) $(RUNCMD) ./tcrtest wicked -cnum 5 -tout 5 127.0.0.1 5000
	$(RUNENV) $(RUNCMD) ./tcrmgr inform 127.0.0.1
	$(RUNENV) $(RUNCMD) ./tcrmgr vanish 127.0.0.1
	$(RUNENV) $(RUNCMD) ./tcrmttest write -tnum 5 127.0.0.1 5000
	$(RUNENV) $(RUNCMD) ./tcrmttest read -tnum 5 127.0.0.1
	$(RUNENV) $(RUNCMD) ./tcrmttest remove -tnum 5 127.0.0.1
	$(RUNENV) $(RUNCMD) ./tcrmttest write -tnum 5 -ext putcat -rnd 127.0.0.1 5000
	$(RUNENV) $(RUNCMD) ./tcrmttest typical -tnum 5 127.0.0.1 5000
	$(RUNENV) $(RUNCMD) ./tcrmgr vanish 127.0.0.1
	$(RUNENV) $(RUNCMD) ./tcrmgr put 127.0.0.1 one first
	$(RUNENV) $(RUNCMD) ./tcrmgr put 127.0.0.1 two second
	$(RUNENV) $(RUNCMD) ./tcrmgr put -dk 127.0.0.1 three third
	$(RUNENV) $(RUNCMD) ./tcrmgr put -dc 127.0.0.1 three third
	$(RUNENV) $(RUNCMD) ./tcrmgr put -dc 127.0.0.1 three third
	$(RUNENV) $(RUNCMD) ./tcrmgr put -dc 127.0.0.1 three third
	$(RUNENV) $(RUNCMD) ./tcrmgr put 127.0.0.1 four fourth
	$(RUNENV) $(RUNCMD) ./tcrmgr put -dk 127.0.0.1 five fifth
	$(RUNENV) $(RUNCMD) ./tcrmgr out 127.0.0.1 one
	$(RUNENV) $(RUNCMD) ./tcrmgr out 127.0.0.1 two
	$(RUNENV) $(RUNCMD) ./tcrmgr get 127.0.0.1 three > check.out
	$(RUNENV) $(RUNCMD) ./tcrmgr get 127.0.0.1 four > check.out
	$(RUNENV) $(RUNCMD) ./tcrmgr get 127.0.0.1 five > check.out
	$(RUNENV) $(RUNCMD) ./tcrmgr mget 127.0.0.1 one two three four five > check.out
	$(RUNENV) $(RUNCMD) ./tcrmgr misc 127.0.0.1 putlist six sixth seven seventh
	$(RUNENV) $(RUNCMD) ./tcrmgr misc 127.0.0.1 outlist six
	$(RUNENV) $(RUNCMD) ./tcrmgr misc 127.0.0.1 getlist three four five six > check.out
	$(RUNENV) $(RUNCMD) ./tcrmgr list -pv 127.0.0.1 > check.out
	$(RUNENV) $(RUNCMD) ./tcrmgr list -pv -fm f 127.0.0.1 > check.out
	$(RUNENV) $(RUNCMD) ./tcrmgr http -ih http://127.0.0.1:1978/five > check.out
	rm -rf ulog ; mkdir -p ulog
	$(RUNENV) $(RUNCMD) ./ttultest write -lim 10000 ulog 5000
	$(RUNENV) $(RUNCMD) ./ttultest write -lim 10000 -as ulog 5000
	$(RUNENV) $(RUNCMD) ./ttultest read ulog
	rm -rf ulog ; mkdir -p ulog
	$(RUNENV) $(RUNCMD) ./ttultest thread -lim 10000 ulog 5 5000
	$(RUNENV) $(RUNCMD) ./ttultest thread -lim 10000 -as ulog 5 5000
	rm -rf check.out ulog
	@printf '\n'
	@printf '#================================================================\n'
	@printf '# Checking completed.\n'
	@printf '#================================================================\n'


.PHONY : all clean install check



#================================================================
# Building binaries
#================================================================


libtokyotyrant.a : $(LIBOBJFILES)
	$(AR) $(ARFLAGS) $@ $(LIBOBJFILES)


libtokyotyrant.so.$(LIBVER).$(LIBREV).0 : $(LIBOBJFILES)
	if uname -a | egrep -i 'SunOS' > /dev/null ; \
	  then \
	    $(CC) $(CFLAGS) -shared -Wl,-G,-h,libtokyotyrant.so.$(LIBVER) -o $@ \
	      $(LIBOBJFILES) $(LDFLAGS) $(LIBS) ; \
	  else \
	    $(CC) $(CFLAGS) -shared -Wl,-soname,libtokyotyrant.so.$(LIBVER) -o $@ \
	      $(LIBOBJFILES) $(LDFLAGS) $(LIBS) ; \
	  fi


libtokyotyrant.so.$(LIBVER) : libtokyotyrant.so.$(LIBVER).$(LIBREV).0
	ln -f -s libtokyotyrant.so.$(LIBVER).$(LIBREV).0 $@


libtokyotyrant.so : libtokyotyrant.so.$(LIBVER).$(LIBREV).0
	ln -f -s libtokyotyrant.so.$(LIBVER).$(LIBREV).0 $@


ttskelmock.so : ttskelmock.o
	$(CC) $(CFLAGS) -shared -o $@ $< $(LDFLAGS) $(LIBS)


ttskeldir.so : ttskeldir.o
	$(CC) $(CFLAGS) -shared -o $@ $< $(LDFLAGS) $(LIBS)


ttskelproxy.so : ttskelproxy.o
	$(CC) $(CFLAGS) -shared -o $@ $< $(LDFLAGS) $(LIBS)


ttskelnull.so : ttskelnull.o
	$(CC) $(CFLAGS) -shared -o $@ $< $(LDFLAGS) $(LIBS)


ttskelmock.bundle : ttskelmock.o
	$(CC) $(CFLAGS) -bundle -flat_namespace -undefined suppress -o $@ $< $(LDFLAGS) $(LIBS)


ttskeldir.bundle : ttskeldir.o
	$(CC) $(CFLAGS) -bundle -flat_namespace -undefined suppress -o $@ $< $(LDFLAGS) $(LIBS)


ttskelproxy.bundle : ttskelproxy.o
	$(CC) $(CFLAGS) -bundle -flat_namespace -undefined suppress -o $@ $< $(LDFLAGS) $(LIBS)


ttskelnull.bundle : ttskelnull.o
	$(CC) $(CFLAGS) -bundle -flat_namespace -undefined suppress -o $@ $< $(LDFLAGS) $(LIBS)


ttserver : ttserver.o scrext.o $(LIBRARYFILES)
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< scrext.o $(LDFLAGS) $(CMDLDFLAGS) -ltokyotyrant $(LIBS)


ttulmgr : ttulmgr.o $(LIBRARYFILES)
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(CMDLDFLAGS) -ltokyotyrant $(LIBS)


ttultest : ttultest.o $(LIBRARYFILES)
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(CMDLDFLAGS) -ltokyotyrant $(LIBS)


tcrtest : tcrtest.o $(LIBRARYFILES)
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(CMDLDFLAGS) -ltokyotyrant $(LIBS)


tcrmttest : tcrmttest.o $(LIBRARYFILES)
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(CMDLDFLAGS) -ltokyotyrant $(LIBS)


tcrmgr : tcrmgr.o $(LIBRARYFILES)
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(CMDLDFLAGS) -ltokyotyrant $(LIBS)


myconf.o scrext.o : myconf.h

ttutil.o : myconf.h ttutil.h

tculog.o : myconf.h ttutil.h tculog.h

tcrdb.o : myconf.h ttutil.h tcrdb.h

ttskelproxy.o : myconf.h ttutil.h tcrdb.h

ttserver.o ttulmgr.o ttultest.o tcrtest.o tcrmgr.o : myconf.h ttutil.h tculog.h tcrdb.h

ttserver.o : scrext.h
