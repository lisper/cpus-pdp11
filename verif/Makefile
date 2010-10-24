#
# pdp-11 cpu verification tests
#

CVER_FLAGS = +loadvpi=../pli/ide/pli_ide.so:vpi_compat_bootstrap

RTESTS = test_bus test_rk test_tt test_ide test_div test_mul test_shift

BTESTS = $(wildcard ../tests/basic/*.mem)
BTESTS := $(filter-out %test11.mem,$(BTESTS))
BTESTS := $(filter-out %test14.mem,$(BTESTS))
BTESTS := $(filter-out %test15.mem,$(BTESTS))
BTESTS := $(filter-out %test16.mem,$(BTESTS))

DTESTS = $(wildcard ../tests/diags/*.mem)
DTESTS := $(filter-out %FKAAC0.mem,$(DTESTS))
DTESTS := $(filter-out %FKABD0.mem,$(DTESTS))
DTESTS := $(filter-out %FKACA0.mem,$(DTESTS))
DTESTS := $(filter-out %FKTGC0.mem,$(DTESTS))
DTESTS := $(filter-out %FKTHB0.mem,$(DTESTS))

RTL = \
	../rtl/pdp11.v ../rtl/ipl_below.v ../rtl/add8.v ../rtl/mmu.v \
	../rtl/execute.v ../rtl/mul1616.v ../rtl/div3216.v ../rtl/shift32.v \
	../rtl/mmu_regs.v ../rtl/clk_regs.v ../rtl/sr_regs.v ../rtl/psw_regs.v \
	../rtl/rk_regs.v ../rtl/ide.v \
	../rtl/tt_regs.v ../rtl/brg.v ../rtl/uart.v \
	../rtl/bootrom.v ../rtl/iopage.v ../rtl/bus.v \
	../rtl/ram.v ../rtl/ram_sync.v ../rtl/ram_async.v ../rtl/ram_s3board.v

.PHONY: regress basic diags bootrt11

all: rk-test.dsk regress basic diags bootrt11

# booting RT-11
bootrt11:

#
rk-test.dsk:
	dd if=/dev/zero of=rk-test.dsk bs=512 count=100

clean:
	rm -f *.vcd *.log
	rm -f rk-test.dsk

#---------------------------------------------------------------------

define checkone_basic_code
     @echo checking $(2)
     @grep -a f1 $(1) >/tmp/a;
     @grep -a f1 $(2) | sed -e 's/ 077 / 77 /g' -e 's/(077 /(77 /g' >/tmp/b;
     @diff -q /tmp/a /tmp/b;
     @if [ $$? -ne 0 ]; then \
       echo blah; \
     fi;
endef

define runone_basic_code
     @echo creating $(2)
     @eval "cver +loadvpi=../pli/ide/pli_ide.so:vpi_compat_bootstrap \
	     +loadvpi=../pli/rk/pli_rk.so:vpi_compat_bootstrap \
	     +loadvpi=../pli/pdp11/pli_pdp11dis.so:vpi_compat_bootstrap \
	     +test=$(1) +pc=$(3) \
	     run.v" >$(2);
endef

BTEST_LOGS = $(addsuffix .log,$(basename $(notdir $(BTESTS))))

$(BTEST_LOGS): %.log: ../tests/basic/%.mem $(RTL)
	$(call runone_basic_code,$<,$@,500)
	$(call checkone_basic_code,$(patsubst %.mem,%.log,$<),$@)

# simple pdp-11 code regression tests
basic: $(BTEST_LOGS)

#---------------------------------------------------------------------

DTEST_LOGS = $(addsuffix .log,$(basename $(notdir $(DTESTS))))

$(DTEST_LOGS) : %.log : ../tests/diags/%.mem $(RTL)
	$(call runone_basic_code,$<,$@,200)
	$(call checkone_basic_code,$(patsubst %.mem,%.log,$<),$@)

# dec diagnostics
diags: $(DTEST_LOGS)
	@echo $(DTEST_LOGS)

#---------------------------------------------------------------------

define runone_verilog_regression
	IDEIMAGE=rk-test.dsk cver $(CVER_FLAGS) $(1) >$(2);
	@if grep -q ERROR $(2); then exit 1; fi;
	@if grep -q FAILURE $(2); then exit 1; fi;
endef

RTEST_LOGS = $(addsuffix .log,$(RTESTS))

$(RTEST_LOGS) : %.log : %.v $(RTL)
	$(call runone_verilog_regression,$<,$@)

# verilog testbench regression tests
regress: rk-test.dsk $(RTEST_LOGS)

test_bus: test_bus.v
	$(call runone_verilog_regression,$<,$@)

test_rk: test_rk.v
	$(call runone_verilog_regression,$<,$@)

test_tt: test_tt.v
	$(call runone_verilog_regression,$<,$@)

test_ide: test_ide.v
	$(call runone_verilog_regression,$<,$@)

test_div: test_div.v
	$(call runone_verilog_regression,$<,$@)

test_mul: test_mul.v
	$(call runone_verilog_regression,$<,$@)

test_shift: test_shift.v
	$(call runone_verilog_regression,$<,$@)


