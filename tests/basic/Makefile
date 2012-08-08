#
# Automagically take small macro-11 source files and
# generate .mem files for later RTL sims and generate
# .log file using simh for comparison with the RTL
#
# good engineering requires good process
# and good process requires reproducible results.
#
# regression tests are the cornerstone of reproducible results.
#

AS = ../../utils/macro11/macro11
DUMP = ../../utils/macro11/dumpobj

TESTS = ttytest.mac \
	test0.mac test1.mac test2.mac test3.mac test4.mac test5.mac \
	test6.mac test7.mac test8.mac test9.mac test10.mac test11.mac \
	test12.mac test13.mac test14.mac test15.mac test16.mac test17.mac

MEMS = $(TESTS:%.mac=%.mem)
LOGS = $(TESTS:%.mac=%.log)

all: mems logs

#------------------------------

mems: aout2mem unmem mem2bin $(MEMS)

aout2mem: aout2mem.c
	cc -o aout2mem aout2mem.c

unmem: unmem.c
	cc -o unmem unmem.c

mem2bin: mem2bin.c
	cc -o mem2bin mem2bin.c

%.mem: %.mac
	rm -f a.out
	$(AS) -o $*.obj -l $*.lst $<
	$(DUMP) $*.obj $*.mem

#------------------------------

define genlog_from_script
	../../simhv36-1/BIN/pdp11 $(1) >$(2)
endef

# make the bin file for loading into simh
%.bin: %.mem
	./mem2bin <$< >$@

# make a script file to loading the bin file into simh
%.simh.script: %.bin
	echo -n >$@
	echo "load $*.bin" >>$@
#	echo "set cpu 11/34" >>$@
	echo "set cpu 11/40" >>$@
	echo "go 500" >>$@
	echo "quit" >>$@


# make a trace file using simh
%.log : %.simh.script
	$(call genlog_from_script,$<,$@)

logs: $(LOGS)

#------------------------------

clean:
	rm -f *.lst *.obj *.log *.bin
	rm -f unmem aout2mem mem2bin

