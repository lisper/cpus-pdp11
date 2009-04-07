#!/bin/sh

for t in `seq 1 12`; do
 echo TEST $t:
 blf=../behave/test$t.log
 rlf=./test$t.log
 grep f1 $blf >/tmp/blf
 grep f1 $rlf >/tmp/rlf
 diff -u /tmp/blf /tmp/rlf
done
exit 0

