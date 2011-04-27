#
#

all: binre dis

isn.h: isn.txt maketables.pl
	./maketables.pl >isn.h

SRC = binre.c isn.c tc.c mach.c pdp11.c support.c rk.c rl.c mmu.c bpred.c
HDR = binre.h isn.h mach.h

CFLAGS += -g -O2

binre: $(SRC) $(HDR)
	cc -o binre $(CFLAGS) $(SRC)

dis: dis.c isn.c isn.h
	cc -o dis dis.c isn.c

clean:
	rm -f binre dis *.o
