#!/bin/sh

#cver +loadvpi=../pli/ide/pli_ide.so:vpi_compat_bootstrap run_bus.v >log
cver +loadvpi=../pli/ide/pli_ide.so:vpi_compat_bootstrap run.v >log

