transcript on
if {[file exists rtl_work]} {
  vdel -lib rtl_work -all
}
vlib rtl_work
vmap work rtl_work

#
transcript file "log"

#
vlog run.v

noview wave

#vsim -novopt test
vsim -voptargs="+acc=rnp" -pli ../pli/rk/pli_rk.dll test
do wave.do

run
