#!/bin/sh

# run a single diag "tape" (in same manner as Makefile)

cver +loadvpi=../pli/ide/pli_ide.so:vpi_compat_bootstrap \
     +loadvpi=../pli/rk/pli_rk.so:vpi_compat_bootstrap \
     +loadvpi=../pli/pdp11/pli_pdp11dis.so:vpi_compat_bootstrap \
     +test=../tests/diags/$*.mem +pc=200 \
     run.v
