LIBOBJFILES = util.o net.o
COMMANDFILES = server client

CC = gcc
CPPFLAGS = -I. -DNDEBUG -D_GNU_SOURCE=1 -D_REENTRANT
CFLAGS = -g -O2 -std=c99 -Wall -fPIC -fsigned-char -Wno-unused-but-set-variable -Wno-format-truncation
LDFLAGS = -L.
LIBS = -ldl -lrt -lpthread -lm
LDENV = LD_RUN_PATH=/lib:/usr/lib:/usr/local/lib:.


.SUFFIXES :
.SUFFIXES : .c .o

.c.o :
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<


all : $(COMMANDFILES)
	@printf '\n'
	@printf '#================================================================\n'
	@printf '# Complete.\n'
	@printf '#================================================================\n'


clean :
	rm -rf $(LIBOBJFILES) $(COMMANDFILES) *.o


check :
	./client version
	./client vanish 127.0.0.1
	./client put 127.0.0.1 one first
	./client put 127.0.0.1 two second
	./client put -dk 127.0.0.1 three third
	./client put -dc 127.0.0.1 three third
	./client put -dc 127.0.0.1 three third
	./client put -dc 127.0.0.1 three third
	./client put 127.0.0.1 four fourth
	./client put -dk 127.0.0.1 five fifth
	./client out 127.0.0.1 one
	./client out 127.0.0.1 two
	./client get 127.0.0.1 three > check.out
	./client get 127.0.0.1 four > check.out
	./client get 127.0.0.1 five > check.out
	./client mget 127.0.0.1 one two three four five > check.out
	./client misc 127.0.0.1 putlist six sixth seven seventh
	./client misc 127.0.0.1 outlist six
	./client misc 127.0.0.1 getlist three four five six > check.out
	./client list -pv 127.0.0.1 > check.out
	./client list -pv -fm f 127.0.0.1 > check.out
	./client http -ih http://127.0.0.1:1978/five > check.out
	rm -rf ulog check.out
	@printf '\n'
	@printf '#================================================================\n'
	@printf '# Checking completed.\n'
	@printf '#================================================================\n'


.PHONY : all clean check


server : server.o $(LIBOBJFILES)
	$(LDENV) $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)


client : client.o $(LIBOBJFILES)
	$(LDENV) $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)


util.o : util.h

server.o client.o net.o : util.h net.h
