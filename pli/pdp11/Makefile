#

INCS = -I../../cver/gplcver-2.12a.src/pli_incs

CFLAGS= -fPIC -Wall -g $(INCS)
LFLAGS= -G -shared -export-dynamic

all: pli_pdp11dis.so pli_pdp11dis.vpi

pli_pdp11dis.o: pli_pdp11dis.c dis.c
	$(CC) $(CFLAGS) -c pli_pdp11dis.c dis.c

pli_pdp11dis.so: pli_pdp11dis.o
	$(LD) $(LFLAGS) pli_pdp11dis.o dis.o -o pli_pdp11dis.so

pli_pdp11dis.vpi: pli_pdp11dis.o
	$(LD) $(LFLAGS) pli_pdp11dis.o dis.o -o pli_pdp11dis.vpi

clean:
	rm -f *.o *.so *.vpi

