#!/bin/bash

for t in `seq 1 12`; do
 echo TEST $t:
 lf=test$t.log
 (./cpu -t $t | tee $lf | grep disagree) && echo "test $t failed"
done
exit 0

