
SUBDIRS = basic diags rom src

define makeone
	echo "[ $(1) ]";
	make -C $(1) $(2);
endef

all clean:
	@$(foreach dir,$(SUBDIRS),$(call makeone,$(dir),$@))
