#!/bin/sh -x

./binre -f unix0_v6_rk.dsk -c1500000 >xx.o
grep "f1:" xx.o >xx1.o
diff -b -U1 xx1.o xxx1.o >yy.o

./binre -m ../tests/diags/FKAAC0.mem -p 200 -c 100000  | ../utils/graboutput/graboutput

./binre -m ../tests/diags/FKABD0.mem -p 200 -c 300000 >ww.o

./binre -m ../tests/diags/FKTHB0.mem -p 200 -c 100000 | ../utils/graboutput/graboutput