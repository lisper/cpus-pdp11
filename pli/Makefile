
SUBDIRS = ide rk pdp11 test
#iv 

define submake
	echo "[ $(1) make $(2) ]";
	eval "make -C $(1) $(2) || exit 1";
endef

all clean:
	$(foreach dir,$(SUBDIRS),$(call submake,$(dir),$@))
