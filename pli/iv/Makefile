#

INCS = -I../../cver/gplcver-2.12a.src/pli_incs

CFLAGS= -fPIC -Wall -g $(INCS)
LFLAGS= -G -shared -export-dynamic

all: pli_all.so pli_all.vpi

OBJ = pli_all.o pli_pdp11dis.o dis.o pli_ide.o pli_ram.o pli_rk.o

pli_pdp11dis.o: ../pdp11/pli_pdp11dis.c ../pdp11/dis.c
	$(CC) $(CFLAGS) -c ../pdp11/pli_pdp11dis.c ../pdp11/dis.c

pli_ide.o: ../ide/pli_ide.c 
	$(CC) $(CFLAGS) -c ../ide/pli_ide.c

pli_ram.o: ../rk/pli_ram.c 
	$(CC) $(CFLAGS) -c ../rk/pli_ram.c

pli_rk.o: ../rk/pli_rk.c 
	$(CC) $(CFLAGS) -c ../rk/pli_rk.c

pli_all.so: $(OBJ)
	$(LD) $(LFLAGS) $(OBJ) -o pli_all.so

pli_all.vpi: $(OBJ)
	$(LD) $(LFLAGS) $(OBJ) -o pli_all.vpi

clean:
	rm -f *.o *.so *.vpi
