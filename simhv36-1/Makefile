
CFLAGS = -std=c99 -O2 -U__STRICT_ANSI__ -g -lm -D_GNU_SOURCE -I .  -DVM_PDP11 -I PDP11

PDP11_SRC = \
	PDP11/pdp11_fp.c PDP11/pdp11_cpu.c PDP11/pdp11_dz.c PDP11/pdp11_cis.c \
	PDP11/pdp11_lp.c PDP11/pdp11_rk.c PDP11/pdp11_rl.c PDP11/pdp11_rp.c \
	PDP11/pdp11_rx.c PDP11/pdp11_stddev.c PDP11/pdp11_sys.c PDP11/pdp11_tc.c\
	PDP11/pdp11_tm.c PDP11/pdp11_ts.c PDP11/pdp11_io.c PDP11/pdp11_rq.c \
	PDP11/pdp11_tq.c PDP11/pdp11_pclk.c PDP11/pdp11_ry.c PDP11/pdp11_pt.c \
	PDP11/pdp11_hk.c PDP11/pdp11_xq.c PDP11/pdp11_xu.c PDP11/pdp11_vh.c \
	PDP11/pdp11_rh.c PDP11/pdp11_tu.c PDP11/pdp11_cpumod.c PDP11/pdp11_cr.c \
	PDP11/pdp11_rf.c PDP11/pdp11_dl.c PDP11/pdp11_ke.c \
	PDP11/private_rk.c PDP11/private_stddev.c PDP11/stubs.c

SIMH_SRC = \
	scp.c sim_console.c sim_fio.c sim_timer.c sim_sock.c sim_tmxr.c \
	sim_ether.c sim_tape.c

PDP11_OBJ = $(addprefix BIN/,$(notdir $(PDP11_SRC:.c=.o)))
SIMH_OBJ = $(addprefix BIN/,$(notdir $(SIMH_SRC:.c=.o)))

all: tester

tester: tester.c BIN/libpdp11.a
	cc -o tester tester.c -L BIN -lpdp11 -lm

BIN/libpdp11.a: $(SIMH_OBJ) $(PDP11_OBJ)
	ar crv $@ $(SIMH_OBJ) $(PDP11_OBJ)

$(PDP11_OBJ): BIN/%.o: PDP11/%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(SIMH_OBJ): BIN/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@


PDP11_OPT = ${CFLAGS} -DWITH_MAIN
LDFLAGS = 

BIN/pdp11: ${PDP11_SRC} ${SIMH_SRC}
	${CC} ${PDP11_SRC} ${SIMH_SRC} ${PDP11_OPT} -o $@ ${LDFLAGS}
#PDP11/stubs.c 