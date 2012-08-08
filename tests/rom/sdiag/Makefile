
AS = ../../utils/macro11/macro11
DUMP = ../../utils/macro11/dumpobj

SRC = diag.mac

all: diag.mem
	../basic/mem2bin <diag.mem >diag.bin

#------------------------------

%.mem: %.mac
	rm -f a.out
	$(AS) -o $*.obj -l $*.lst $<
	$(DUMP) $*.obj $*.mem

makev: makev.c
	cc -o makev makev.c

clean:
	rm -f *.lst *.obj *.log
	rm -f *.mem
	rm -f unmem aout2mem mem2bin

