#
# make "mem" files for simulation from various binary formats
# we get to xxdp and other places.
#

MEMS = dzqkc.mem FKAAC0.mem FKABD0.mem FKACA0.mem EKBAD0.mem

all: loadtomem mems

mems: $(MEMS)

loadtomem: loadtomem.c
	cc -o loadtomem loadtomem.c

define loadone
	./loadtomem <$(1) >$(2)
endef

%.mem: %.bin
	$(call loadone,$<,$@)

%.mem: %.BIC
	$(call loadone,$<,$@)

dzqkc.mem: maindec-11-dzqkc-e-pb.bin
	$(call loadone,$<,$@)


clean:
	rm -rf *.mem
