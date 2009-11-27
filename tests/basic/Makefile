#
AS = ../../macro11/macro11
DUMP = ../../macro11/dumpobj

TESTS = ttytest.mac \
	test0.mac test1.mac test2.mac test3.mac test4.mac test5.mac \
	test6.mac test7.mac test8.mac test9.mac test10.mac test11.mac \
	test12.mac test13.mac test14.mac test15.mac test16.mac

MEMS = $(TESTS:%.mac=%.mem)

all: aout2mem unmem $(MEMS)

aout2mem: aout2mem.c
	cc -o aout2mem aout2mem.c

unmem: unmem.c
	cc -o unmem unmem.c

%.mem: %.mac
	rm -f a.out
	$(AS) -o $*.obj -l $*.lst $<
	$(DUMP) $*.obj $*.mem

clean:
	rm -f *.lst *.obj
	rm -f *.mem
	rm -f unmem aout2mem

