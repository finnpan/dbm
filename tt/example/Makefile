MYBINS = tcrdbex tcrdbtblex

CC = gcc
CFLAGS = -ansi -Wall -pedantic -I. -I.. -I../../tc -Wall
LDFLAGS = -L. -L.. -L../../tc
LIBS = -ltt -ltc -lz -lresolv -lnsl -lpthread -lm -lc
LDENV = LD_RUN_PATH=/lib:/usr/lib:/usr/local/lib:.:..


.SUFFIXES :
.SUFFIXES : .c .o

.c.o :
	$(CC) -c $(CFLAGS) $<


all : $(MYBINS)


clean :
	rm -rf $(MYBINS) *.o


.PHONY : all clean


tcrdbex : tcrdbex.o
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)


tcrdbtblex : tcrdbtblex.o
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)
