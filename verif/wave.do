onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate -format Logic /test/clk
add wave -noupdate -format Logic /test/reset
add wave -noupdate -format Logic /test/bus_rd
add wave -noupdate -format Logic /test/bus_wr
add wave -noupdate -format Logic /test/bus_byte_op
add wave -noupdate -format Logic /test/bus_arbitrate
add wave -noupdate -format Logic /test/bus_ack
add wave -noupdate -format Logic /test/bus_error
add wave -noupdate -format Logic /test/interrupt
add wave -noupdate -format Literal -radix octal /test/ram_addr
add wave -noupdate -format Literal -radix octal /test/ram_data_in
add wave -noupdate -format Literal -radix octal /test/ram_data_out
add wave -noupdate -format Logic /test/ram_rd
add wave -noupdate -format Logic /test/ram_wr
add wave -noupdate -format Logic /test/ram_byte_op
add wave -noupdate -format Literal -radix octal /test/cpu/r0
add wave -noupdate -format Literal -radix octal /test/cpu/r1
add wave -noupdate -format Literal -radix octal /test/cpu/r2
add wave -noupdate -format Literal -radix octal /test/cpu/r3
add wave -noupdate -format Literal -radix octal /test/cpu/r4
add wave -noupdate -format Literal -radix octal /test/cpu/r5
add wave -noupdate -format Literal -radix octal /test/cpu/r6
add wave -noupdate -format Literal -radix octal /test/cpu/sp
add wave -noupdate -format Literal /test/cpu/mode_kernel
add wave -noupdate -format Literal /test/cpu/mode_super
add wave -noupdate -format Literal /test/cpu/mode_undef
add wave -noupdate -format Literal /test/cpu/mode_user
add wave -noupdate -format Logic /test/cpu/clk
add wave -noupdate -format Logic /test/cpu/reset
add wave -noupdate -format Literal /test/cpu/initial_pc
add wave -noupdate -format Logic /test/cpu/halted
add wave -noupdate -format Logic /test/cpu/waited
add wave -noupdate -format Logic /test/cpu/trapped
add wave -noupdate -format Literal -radix octal /test/cpu/bus_addr
add wave -noupdate -format Literal -radix octal /test/cpu/bus_data_in
add wave -noupdate -format Literal -radix octal /test/cpu/bus_data_out
add wave -noupdate -format Logic /test/cpu/bus_rd
add wave -noupdate -format Logic /test/cpu/bus_wr
add wave -noupdate -format Logic /test/cpu/bus_byte_op
add wave -noupdate -format Logic /test/cpu/bus_arbitrate
add wave -noupdate -format Logic /test/cpu/bus_ack
add wave -noupdate -format Logic /test/cpu/bus_error
add wave -noupdate -format Logic /test/cpu/bus_int
add wave -noupdate -format Literal /test/cpu/bus_int_ipl
add wave -noupdate -format Literal -radix octal /test/cpu/bus_int_vector
add wave -noupdate -format Literal -radix octal /test/cpu/interrupt_ack_ipl
add wave -noupdate -format Literal -radix octal /test/cpu/psw
add wave -noupdate -format Literal -radix octal /test/cpu/pc
add wave -noupdate -format Logic /test/cpu/psw_io_wr
add wave -noupdate -format Logic /test/cpu/interrupt
add wave -noupdate -format Logic /test/cpu/bus_i_access
add wave -noupdate -format Literal /test/cpu/bus_cpu_cm
add wave -noupdate -format Logic /test/cpu/mmu_fetch_va
add wave -noupdate -format Logic /test/cpu/mmu_abort
add wave -noupdate -format Logic /test/cpu/mmu_trap
add wave -noupdate -format Logic /test/cpu/trap
add wave -noupdate -format Logic /test/cpu/trap_bpt
add wave -noupdate -format Logic /test/cpu/trap_iot
add wave -noupdate -format Logic /test/cpu/trap_emt
add wave -noupdate -format Logic /test/cpu/trap_trap
add wave -noupdate -format Logic /test/cpu/trap_odd
add wave -noupdate -format Logic /test/cpu/trap_oflo
add wave -noupdate -format Logic /test/cpu/trap_bus
add wave -noupdate -format Logic /test/cpu/trap_ill
add wave -noupdate -format Logic /test/cpu/trap_res
add wave -noupdate -format Logic /test/cpu/trap_priv
add wave -noupdate -format Logic /test/cpu/trace_inhibit
add wave -noupdate -format Literal /test/cpu/vector
add wave -noupdate -format Logic /test/cpu/trace
add wave -noupdate -format Logic /test/cpu/odd_fetch
add wave -noupdate -format Logic /test/cpu/odd_pc
add wave -noupdate -format Logic /test/cpu/cc_n
add wave -noupdate -format Logic /test/cpu/cc_z
add wave -noupdate -format Logic /test/cpu/cc_v
add wave -noupdate -format Logic /test/cpu/cc_c
add wave -noupdate -format Literal -radix octal /test/cpu/ipl
add wave -noupdate -format Literal -radix octal /test/cpu/r0
add wave -noupdate -format Literal -radix octal /test/cpu/r1
add wave -noupdate -format Literal -radix octal /test/cpu/r2
add wave -noupdate -format Literal -radix octal /test/cpu/r3
add wave -noupdate -format Literal -radix octal /test/cpu/r4
add wave -noupdate -format Literal -radix octal /test/cpu/r5
add wave -noupdate -format Literal -radix octal /test/cpu/r6
add wave -noupdate -format Literal -radix octal /test/cpu/sp
add wave -noupdate -format Logic /test/cpu/assert_wait
add wave -noupdate -format Logic /test/cpu/assert_halt
add wave -noupdate -format Logic /test/cpu/assert_reset
add wave -noupdate -format Logic /test/cpu/assert_bpt
add wave -noupdate -format Logic /test/cpu/assert_iot
add wave -noupdate -format Logic /test/cpu/assert_trap_odd
add wave -noupdate -format Logic /test/cpu/assert_trap_oflo
add wave -noupdate -format Logic /test/cpu/assert_trap_ill
add wave -noupdate -format Logic /test/cpu/assert_trap_res
add wave -noupdate -format Logic /test/cpu/assert_trap_priv
add wave -noupdate -format Logic /test/cpu/assert_trap_emt
add wave -noupdate -format Logic /test/cpu/assert_trap_trap
add wave -noupdate -format Logic /test/cpu/assert_trap_bus
add wave -noupdate -format Logic /test/cpu/assert_trace_inhibit
add wave -noupdate -format Literal /test/cpu/isn_15_12
add wave -noupdate -format Literal /test/cpu/isn_15_9
add wave -noupdate -format Literal /test/cpu/isn_15_6
add wave -noupdate -format Literal /test/cpu/isn_11_6
add wave -noupdate -format Literal /test/cpu/isn_11_9
add wave -noupdate -format Literal /test/cpu/isn_5_0
add wave -noupdate -format Logic /test/cpu/is_isn_rss
add wave -noupdate -format Logic /test/cpu/is_isn_rdd
add wave -noupdate -format Logic /test/cpu/is_isn_rxx
add wave -noupdate -format Logic /test/cpu/is_isn_r32
add wave -noupdate -format Logic /test/cpu/is_isn_byte
add wave -noupdate -format Logic /test/cpu/is_isn_mfpx
add wave -noupdate -format Logic /test/cpu/is_isn_mtpx
add wave -noupdate -format Logic /test/cpu/is_illegal
add wave -noupdate -format Logic /test/cpu/is_reserved
add wave -noupdate -format Logic /test/cpu/no_operand
add wave -noupdate -format Logic /test/cpu/store_result
add wave -noupdate -format Logic /test/cpu/store_result32
add wave -noupdate -format Logic /test/cpu/store_ss_reg
add wave -noupdate -format Logic /test/cpu/need_srcspec_dd_word
add wave -noupdate -format Logic /test/cpu/need_srcspec_dd_byte
add wave -noupdate -format Logic /test/cpu/need_srcspec_dd
add wave -noupdate -format Logic /test/cpu/need_destspec_dd_word
add wave -noupdate -format Logic /test/cpu/need_destspec_dd_byte
add wave -noupdate -format Logic /test/cpu/need_destspec_dd
add wave -noupdate -format Logic /test/cpu/need_pop_reg
add wave -noupdate -format Logic /test/cpu/need_pop_pc_psw
add wave -noupdate -format Logic /test/cpu/need_push_state
add wave -noupdate -format Logic /test/cpu/need_dest_data
add wave -noupdate -format Logic /test/cpu/need_src_data
add wave -noupdate -format Logic /test/cpu/need_s1
add wave -noupdate -format Logic /test/cpu/need_s2
add wave -noupdate -format Logic /test/cpu/need_s4
add wave -noupdate -format Logic /test/cpu/need_d1
add wave -noupdate -format Logic /test/cpu/need_d2
add wave -noupdate -format Logic /test/cpu/need_d4
add wave -noupdate -format Literal -radix octal /test/cpu/dd_ea_mux
add wave -noupdate -format Literal -radix octal /test/cpu/ss_ea_mux
add wave -noupdate -format Literal -radix octal /test/cpu/new_pc
add wave -noupdate -format Logic /test/cpu/latch_pc
add wave -noupdate -format Logic /test/cpu/latch_sp
add wave -noupdate -format Logic /test/cpu/new_cc_n
add wave -noupdate -format Logic /test/cpu/new_cc_z
add wave -noupdate -format Logic /test/cpu/new_cc_v
add wave -noupdate -format Logic /test/cpu/new_cc_c
add wave -noupdate -format Logic /test/cpu/latch_cc
add wave -noupdate -format Literal /test/cpu/new_psw_cc
add wave -noupdate -format Literal /test/cpu/new_psw_prio
add wave -noupdate -format Logic /test/cpu/latch_psw_prio
add wave -noupdate -format Literal -radix octal /test/cpu/isn
add wave -noupdate -format Literal -radix octal /test/cpu/dd_ea
add wave -noupdate -format Literal -radix octal /test/cpu/ss_ea
add wave -noupdate -format Literal -radix octal /test/cpu/dd_mode
add wave -noupdate -format Literal -radix octal /test/cpu/dd_reg
add wave -noupdate -format Logic /test/cpu/dd_dest_mem
add wave -noupdate -format Logic /test/cpu/dd_dest_reg
add wave -noupdate -format Logic /test/cpu/dd_ea_ind
add wave -noupdate -format Logic /test/cpu/dd_post_incr
add wave -noupdate -format Logic /test/cpu/dd_pre_dec
add wave -noupdate -format Literal -radix octal /test/cpu/new_dd_reg_post_incr
add wave -noupdate -format Literal -radix octal /test/cpu/new_dd_reg_pre_decr
add wave -noupdate -format Literal -radix octal /test/cpu/new_dd_reg_incdec
add wave -noupdate -format Literal -radix octal /test/cpu/new_ss_reg_post_incr
add wave -noupdate -format Literal -radix octal /test/cpu/new_ss_reg_pre_decr
add wave -noupdate -format Literal -radix octal /test/cpu/new_ss_reg_incdec
add wave -noupdate -format Literal -radix octal /test/cpu/ss_mode
add wave -noupdate -format Literal -radix octal /test/cpu/ss_reg
add wave -noupdate -format Literal -radix octal /test/cpu/ss_reg_value
add wave -noupdate -format Literal -radix octal /test/cpu/ss_rego1_value
add wave -noupdate -format Literal -radix octal /test/cpu/dd_reg_value
add wave -noupdate -format Literal -radix octal /test/cpu/ss_data
add wave -noupdate -format Literal -radix octal /test/cpu/dd_data
add wave -noupdate -format Literal -radix octal /test/cpu/e1_data
add wave -noupdate -format Literal -radix octal /test/cpu/e32_data
add wave -noupdate -format Logic /test/cpu/ss_ea_ind
add wave -noupdate -format Logic /test/cpu/ss_post_incr
add wave -noupdate -format Logic /test/cpu/ss_pre_dec
add wave -noupdate -format Literal -radix octal /test/cpu/pc_mux
add wave -noupdate -format Literal -radix octal /test/cpu/sp_mux
add wave -noupdate -format Literal -radix octal /test/cpu/ss_data_mux
add wave -noupdate -format Literal -radix octal /test/cpu/dd_data_mux
add wave -noupdate -format Literal -radix octal /test/cpu/e1_data_mux
add wave -noupdate -format Literal -radix octal /test/cpu/e1_result
add wave -noupdate -format Literal -radix octal /test/cpu/e32_result
add wave -noupdate -format Logic /test/cpu/e1_advance
add wave -noupdate -format Literal /test/cpu/current_mode
add wave -noupdate -format Literal /test/cpu/previous_mode
add wave -noupdate -format Literal -radix unsigned /test/cpu/new_istate
add wave -noupdate -format Literal -radix unsigned /test/cpu/istate
add wave -noupdate -format Logic /test/cpu/enable_execute
add wave -noupdate -format Logic /test/cpu/trap_or_int
add wave -noupdate -format Logic /test/cpu/ok_to_assert_trap
add wave -noupdate -format Logic /test/cpu/ok_to_reset_trap
add wave -noupdate -format Logic /test/cpu/ok_to_reset_trace_inhibit
add wave -noupdate -format Logic /test/cpu/ok_to_assert_int
add wave -noupdate -format Logic /test/mmu1/pxr_addr_5_0
add wave -noupdate -format Logic /test/mmu1/pxr_be
add wave -noupdate -format Literal -radix octal /test/mmu1/pxr_data_in
add wave -noupdate -format Literal -radix octal /test/mmu1/pxr_data_out
add wave -noupdate -format Logic /test/mmu1/pxr_wr
add wave -noupdate -format Logic /test/mmu1/pxr_rd
add wave -noupdate -format Logic /test/mmu1/pxr_addr
add wave -noupdate -format Logic /test/mmu1/mmu_on
add wave -noupdate -format Logic /test/mmu1/maint_mode
add wave -noupdate -format Logic /test/mmu1/cpu_d_access
add wave -noupdate -format Logic /test/cpu/soft_reset
add wave -noupdate -format Literal -radix octal /test/mmu1/mmr0
add wave -noupdate -format Literal -radix octal /test/mmu1/mmr2
add wave -noupdate -format Literal -radix octal /test/mmu1/mmr3
add wave -noupdate -format Literal -radix octal /test/mmu1/map_address
add wave -noupdate -format Logic /test/mmu1/pdr_ed
add wave -noupdate -format Logic /test/mmu1/signal_abort
add wave -noupdate -format Logic /test/mmu1/signal_trap
add wave -noupdate -format Literal -radix octal /test/mmu1/cpu_bn
add wave -noupdate -format Literal -radix octal /test/mmu1/pdr_plf
add wave -noupdate -format Literal -radix octal /test/bus_addr_p
add wave -noupdate -format Literal -radix octal /test/bus_addr_v
add wave -noupdate -format Literal -radix octal /test/bus1/bus_error
add wave -noupdate -format Literal -radix octal /test/bus1/iopage_bus_error
add wave -noupdate -format Literal -radix octal /test/bus1/ram_bus_error
add wave -noupdate -format Literal -radix octal /test/bus1/ram_present
add wave -noupdate -format Literal -radix octal /test/bus1/bus_addr
add wave -noupdate -format Logic /test/cpu/mmu_wr_inhibit
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {1488 ns} 0}
configure wave -namecolwidth 252
configure wave -valuecolwidth 98
configure wave -justifyvalue left
configure wave -signalnamewidth 0
configure wave -snapdistance 10
configure wave -datasetprefix 0
configure wave -rowmargin 4
configure wave -childrowmargin 2
configure wave -gridoffset 0
configure wave -gridperiod 1
configure wave -griddelta 40
configure wave -timeline 0
update
WaveRestoreZoom {1304 ns} {2112 ns}
