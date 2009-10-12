#

INCS = -I../../cver/gplcver-2.12a.src/pli_incs

CFLAGS= -fPIC -Wall -g $(INCS) -D__CVER__
LFLAGS= -G -shared -export-dynamic

all: pli_rk.so pli_rk.vpi

pli_rk.o: pli_rk.c
	$(CC) $(CFLAGS) -c pli_rk.c

pli_ram.o: pli_ram.c
	$(CC) $(CFLAGS) -c pli_ram.c

pli_rk.so: pli_rk.o pli_ram.o
	$(LD) $(LFLAGS) pli_rk.o pli_ram.o -o pli_rk.so

pli_rk.vpi: pli_rk.o pli_ram.o
	$(LD) $(LFLAGS) pli_rk.o pli_ram.o -o pli_rk.vpi


clean:
	rm -f *.o *.so *.vpi

