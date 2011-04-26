#!/bin/sh -x

./binre -f unix0_v6_rk.dsk -m1500000 >xx.o
grep "f1:" xx.o >xx1.o
diff -b -U1 xx1.o xxx1.o >yy.o
