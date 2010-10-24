#!/bin/sh

# run basic tests as an ultra simple regression test

for t in `seq 1 17`; do
 echo TEST $t:
 lf=/tmp/test$t.log
 tf=test$t.mem
 cver +loadvpi=../pli/ide/pli_ide.so:vpi_compat_bootstrap +test=../tests/basic/$tf +pc=500 run.v >$lf
done
exit 0
