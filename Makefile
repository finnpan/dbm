LIBRARYFILES = libdbm.a libdbm.so
LIBOBJFILES = util.o db.o net.o
COMMANDFILES = server client

CC = gcc
CPPFLAGS = -I. -DNDEBUG -D_GNU_SOURCE=1 -D_REENTRANT
CFLAGS = -g -O2 -std=c99 -Wall -fPIC -fsigned-char
LDFLAGS = -L.
LIBS = -lz -ldl -lrt -lpthread -lm
CMDLIBS = -ldbm
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
	$(RUNENV) ./client version
	$(RUNENV) ./client vanish 127.0.0.1
	$(RUNENV) ./client put 127.0.0.1 one first
	$(RUNENV) ./client put 127.0.0.1 two second
	$(RUNENV) ./client put -dk 127.0.0.1 three third
	$(RUNENV) ./client put -dc 127.0.0.1 three third
	$(RUNENV) ./client put -dc 127.0.0.1 three third
	$(RUNENV) ./client put -dc 127.0.0.1 three third
	$(RUNENV) ./client put 127.0.0.1 four fourth
	$(RUNENV) ./client put -dk 127.0.0.1 five fifth
	$(RUNENV) ./client out 127.0.0.1 one
	$(RUNENV) ./client out 127.0.0.1 two
	$(RUNENV) ./client get 127.0.0.1 three > check.out
	$(RUNENV) ./client get 127.0.0.1 four > check.out
	$(RUNENV) ./client get 127.0.0.1 five > check.out
	$(RUNENV) ./client mget 127.0.0.1 one two three four five > check.out
	$(RUNENV) ./client misc 127.0.0.1 putlist six sixth seven seventh
	$(RUNENV) ./client misc 127.0.0.1 outlist six
	$(RUNENV) ./client misc 127.0.0.1 getlist three four five six > check.out
	$(RUNENV) ./client list -pv 127.0.0.1 > check.out
	$(RUNENV) ./client list -pv -fm f 127.0.0.1 > check.out
	$(RUNENV) ./client http -ih http://127.0.0.1:1978/five > check.out
	rm -rf ulog check.out
	@printf '\n'
	@printf '#================================================================\n'
	@printf '# Checking completed.\n'
	@printf '#================================================================\n'


.PHONY : all clean check



libdbm.a : $(LIBOBJFILES)
	$(AR) $(ARFLAGS) $@ $(LIBOBJFILES)


libdbm.so : $(LIBOBJFILES)
	$(CC) $(CFLAGS) -shared -o $@ $(LIBOBJFILES) $(LDFLAGS) $(LIBS)


server : server.o $(LIBRARYFILES)
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(CMDLIBS) $(LIBS)


client : client.o $(LIBRARYFILES)
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(CMDLIBS) $(LIBS)


util.o : util.h

db.o : util.h db.h

net.o : util.h db.h net.h

client.o : util.h db.h net.h

server.o : util.h db.h net.h
