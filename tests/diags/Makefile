#
# make "mem" files for simulation from various binary formats
# we get to xxdp and other places.
#

SIMH_PDP11 = ../../simhv36-1/BIN/pdp11

DIAGS = FKAAC0.BIC FKABD0.BIC FKACA0.BIC FKTHB0.BIN FKTGC0.BIC KKTAB1.BIC
#dzqkc.bin

MEMS = $(addsuffix .mem,$(basename $(DIAGS)))
LOGS = $(addsuffix .log,$(basename $(DIAGS)))

all: loadtomem mems logs

mems: $(MEMS)

loadtomem: loadtomem.c
	cc -o loadtomem loadtomem.c

define loadone
	./loadtomem <$(1) >$(2);
	echo "0000042 0040000" >>$(2)
endef

define genlog_from_script
	$(SIMH_PDP11) $(1) >$(2)
endef

#------------------------------

%.mem: %.bin
	$(call loadone,$<,$@)

%.mem: %.BIN
	$(call loadone,$<,$@)

%.mem: %.BIC
	$(call loadone,$<,$@)

dzqkc.mem: maindec-11-dzqkc-e-pb.bin
	$(call loadone,$<,$@)

#------------------------------

logs: $(LOGS)

%.log : %.simh.script
	$(call genlog_from_script,$<,$@)


#------------------------------

clean:
	rm -rf *.mem *.log
	rm -rf loadtomem
