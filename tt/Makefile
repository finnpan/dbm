VERSION = 1.1.41

LIBRARYFILES = libtt.a libtt.so ttskelmock.so ttskeldir.so ttskelproxy.so ttskelnull.so
LIBOBJFILES = ttutil.o tculog.o tcrdb.o ttconf.o
COMMANDFILES = ttserver ttulmgr tcrmgr

CC = gcc
CPPFLAGS = -I../tc -I. -DNDEBUG -D_GNU_SOURCE=1 -D_REENTRANT -D__EXTENSIONS__
CFLAGS = -g -O2 -std=c99 -Wall -fPIC -fsigned-char -O2
LDFLAGS = -L../tc -L.
LIBS = -ltc -lbz2 -lz -lresolv -lnsl -ldl -lrt -lpthread -lm -lc
LDENV = LD_RUN_PATH=../tc:/lib:/usr/lib:/usr/local/lib:.
RUNENV = LD_LIBRARY_PATH=../tc:/lib:/usr/lib:/usr/local/lib:.


.SUFFIXES :
.SUFFIXES : .c .o

.c.o :
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<


all : $(LIBRARYFILES) $(COMMANDFILES)
	@printf '\n'
	@printf '#================================================================\n'
	@printf '# Complete.\n'
	@printf '#================================================================\n'


clean :
	rm -rf $(LIBRARYFILES) $(LIBOBJFILES) $(COMMANDFILES) *.o


check :
	$(RUNENV) ./tcrmgr version
	$(RUNENV) ./tcrmgr vanish 127.0.0.1
	$(RUNENV) ./tcrmgr put 127.0.0.1 one first
	$(RUNENV) ./tcrmgr put 127.0.0.1 two second
	$(RUNENV) ./tcrmgr put -dk 127.0.0.1 three third
	$(RUNENV) ./tcrmgr put -dc 127.0.0.1 three third
	$(RUNENV) ./tcrmgr put -dc 127.0.0.1 three third
	$(RUNENV) ./tcrmgr put -dc 127.0.0.1 three third
	$(RUNENV) ./tcrmgr put 127.0.0.1 four fourth
	$(RUNENV) ./tcrmgr put -dk 127.0.0.1 five fifth
	$(RUNENV) ./tcrmgr out 127.0.0.1 one
	$(RUNENV) ./tcrmgr out 127.0.0.1 two
	$(RUNENV) ./tcrmgr get 127.0.0.1 three > check.out
	$(RUNENV) ./tcrmgr get 127.0.0.1 four > check.out
	$(RUNENV) ./tcrmgr get 127.0.0.1 five > check.out
	$(RUNENV) ./tcrmgr mget 127.0.0.1 one two three four five > check.out
	$(RUNENV) ./tcrmgr misc 127.0.0.1 putlist six sixth seven seventh
	$(RUNENV) ./tcrmgr misc 127.0.0.1 outlist six
	$(RUNENV) ./tcrmgr misc 127.0.0.1 getlist three four five six > check.out
	$(RUNENV) ./tcrmgr list -pv 127.0.0.1 > check.out
	$(RUNENV) ./tcrmgr list -pv -fm f 127.0.0.1 > check.out
	$(RUNENV) ./tcrmgr http -ih http://127.0.0.1:1978/five > check.out
	rm -rf ulog check.out
	@printf '\n'
	@printf '#================================================================\n'
	@printf '# Checking completed.\n'
	@printf '#================================================================\n'


.PHONY : all clean check


libtt.a : $(LIBOBJFILES)
	$(AR) $(ARFLAGS) $@ $(LIBOBJFILES)


libtt.so : $(LIBOBJFILES)
	$(CC) $(CFLAGS) -shared -o $@ $(LIBOBJFILES) $(LDFLAGS) $(LIBS)


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


ttserver : ttserver.o ttscrext.o $(LIBRARYFILES)
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< ttscrext.o $(LDFLAGS) $(CMDLDFLAGS) -ltt $(LIBS)


ttulmgr : ttulmgr.o $(LIBRARYFILES)
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(CMDLDFLAGS) -ltt $(LIBS)


tcrmgr : tcrmgr.o $(LIBRARYFILES)
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(CMDLDFLAGS) -ltt $(LIBS)


ttconf.o ttscrext.o : ttconf.h

ttutil.o : ttconf.h ttutil.h

tculog.o : ttconf.h ttutil.h tculog.h

tcrdb.o : ttconf.h ttutil.h tcrdb.h

ttskelproxy.o : ttconf.h ttutil.h tcrdb.h

ttserver.o ttulmgr.o : ttconf.h ttutil.h tculog.h tcrdb.h

ttserver.o : ttscrext.h
