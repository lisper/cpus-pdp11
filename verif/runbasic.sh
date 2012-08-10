#!/bin/sh

# run basic tests as an ultra simple regression test

tests="`seq -f \"test%g\" 1 17` ttytest inttest"

for t in $tests; do
 echo TEST $t:
 lf=/tmp/$t.log
 tf=$t.mem
 cver +loadvpi=../pli/ide/pli_ide.so:vpi_compat_bootstrap +test=../tests/basic/$tf +pc=500 +max_cycles=1000 run.v >$lf
done
exit 0
