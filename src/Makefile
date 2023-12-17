LIBRARYFILES = libtc.a libtc.so tcddb.so libtt.a libtt.so
LIBOBJFILES = tcutil.o tchdb.o tcadb.o tcconf.o ttutil.o tculog.o tcrdb.o ttconf.o
COMMANDFILES = ttserver tculmgr tcrmgr

CC = gcc
CPPFLAGS = -I. -DNDEBUG -D_GNU_SOURCE=1 -D_REENTRANT -D__EXTENSIONS__
CFLAGS = -g -O2 -std=c99 -Wall -fPIC -fsigned-char -O2
LDFLAGS = -L.
LIBS_tc = -lbz2 -lz -lrt -lpthread -lm -lc
LIBS_tt = -ltc -lbz2 -lz -lresolv -lnsl -ldl -lrt -lpthread -lm -lc
LDENV = LD_RUN_PATH=/lib:/usr/lib:/usr/local/lib:.
RUNENV = LD_LIBRARY_PATH=/lib:/usr/lib:/usr/local/lib:.


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



libtc.a : $(LIBOBJFILES)
	$(AR) $(ARFLAGS) $@ $(LIBOBJFILES)


libtc.so : $(LIBOBJFILES)
	$(CC) $(CFLAGS) -shared -o $@ $(LIBOBJFILES) $(LDFLAGS) $(LIBS_tc)


tcddb.so : tcddb.o
	$(CC) $(CFLAGS) -shared -o $@ $< $(LDFLAGS) $(LIBS_tc)


tcconf.o : tcconf.h

tcutil.o : tcconf.h tcutil.h

tchdb.o : tcconf.h tcutil.h tchdb.h

tcadb.o : tcconf.h tcutil.h tchdb.h tcadb.h



libtt.a : $(LIBOBJFILES)
	$(AR) $(ARFLAGS) $@ $(LIBOBJFILES)


libtt.so : $(LIBOBJFILES)
	$(CC) $(CFLAGS) -shared -o $@ $(LIBOBJFILES) $(LDFLAGS) $(LIBS_tt)


ttserver : ttserver.o $(LIBRARYFILES)
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(CMDLDFLAGS) -ltt $(LIBS_tt)


tculmgr : tculmgr.o $(LIBRARYFILES)
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(CMDLDFLAGS) -ltt $(LIBS_tt)


tcrmgr : tcrmgr.o $(LIBRARYFILES)
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(CMDLDFLAGS) -ltt $(LIBS_tt)


ttconf.o : ttconf.h

ttutil.o : ttconf.h ttutil.h

tculog.o : ttconf.h ttutil.h tculog.h

tcrdb.o : ttconf.h ttutil.h tcrdb.h

ttserver.o tculmgr.o : ttconf.h ttutil.h tculog.h tcrdb.h
