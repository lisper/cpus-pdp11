#!/bin/sh

#verilator -cc -exe --Mdir ./tmp --top-module test run.v
#verilator -cc -exe --trace --Mdir ./tmp2 test_top.v test.cpp ide.cpp

TRACE=--trace

verilator -cc -exe $TRACE --Mdir ./tmp test_top.v ../verilator/test.cpp ../verilator/ide.cpp ../verilator/ram.cpp && \
(cd tmp; make OPT="-O2" -f Vtest_top.mk)


