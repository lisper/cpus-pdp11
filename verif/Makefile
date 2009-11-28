#
CVER_FLAGS = +loadvpi=../pli/ide/pli_ide.so:vpi_compat_bootstrap

TESTS = test_bus test_rk test_tt test_ide test_div test_mul test_shift

RTL = ../rtl/mmu_regs.v ../rtl/clk_regs.v ../rtl/sr_regs.v ../rtl/psw_regs.v \
	../rtl/rk_regs.v ../rtl/ide.v \
	../rtl/tt_regs.v ../rtl/brg.v ../rtl/uart.v \
	../rtl/bootrom.v ../rtl/iopage.v ../rtl/bus.v \
	../rtl/ram.v ../rtl/ram_sync.v ../rtl/ram_async.v ../rtl/ram_s3board.v

.PHONY: regress diags bootrt11
all: rk.dsk regress diags bootrt11


.PHONY: $(TESTS)
regress: $(TESTS)

diags:

bootrt11:

define testone
	cver $(CVER_FLAGS) $(1) >$(2).log;
	@if grep -q ERROR $(2).log; then exit 1; fi;
	@if grep -q FAILURE $(2).log; then exit 1; fi;
endef

LOGS = $(addsuffix .log,$(TESTS))

$(LOGS) : %.log : %.v $(RTL)
	$(call testone,$<,$@)

test_bus: test_bus.v
	$(call testone,$<,$@)

test_rk: test_rk.v
	$(call testone,$<,$@)

test_tt: test_tt.v
	$(call testone,$<,$@)

test_ide: test_ide.v
	$(call testone,$<,$@)

test_div: test_div.v
	$(call testone,$<,$@)

test_mul: test_mul.v
	$(call testone,$<,$@)

test_shift: test_shift.v
	$(call testone,$<,$@)

rk.dsk:
	dd if=/dev/zero of=rk.dsk bs=512 count=100

clean:
	rm -f *.vcd *.log
	rm -f rk.dsk

