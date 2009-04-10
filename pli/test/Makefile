#

INCS = -I../../cver/gplcver-2.12a.src/pli_incs

CFLAGS= -fPIC -Wall -g $(INCS)
LFLAGS= -G -shared -export-dynamic

all: vhello1.so vhello2.so

vhello1.o:	vhello1.c 
	$(CC) $(CFLAGS) -c vhello1.c

vhello1.so:	vhello1.o
	$(LD) $(LFLAGS) vhello1.o -o vhello1.so


vhello2.o:	vhello2.c 
	$(CC) $(CFLAGS) -c vhello2.c

vhello2.so:	vhello2.o
	$(LD) $(LFLAGS) vhello2.o -o vhello2.so

