#include <stdio.h>

typedef unsigned int u22;
typedef unsigned short u16;

enum {
    T_SIMH=1,
    T_RTL=2,

    T_MEM=1,
    T_IO=2,

    T_READW = 1,
    T_READB,
    T_WRITEW,
    T_WRITEB
};

struct {
	int rw, what;
	unsigned int pa;
	unsigned int data;
} transactions[2][100];
int tcount0, tcount1;

void reset_transactions(void)
{
	tcount0 = 0;
	tcount1 = 0;
}

static char *text_what(int w)
{
	switch (w) {
	case T_MEM: return "mem";
	case T_IO: return "io ";
	}
}

static char *text_rw(int r)
{
	switch (r) {
	case T_READW: return "readw";
	case T_READB: return "readb";
	case T_WRITEW: return "writew";
	case T_WRITEB: return "writeb";
	}
}

void check_transactions(void)
{
	int i, t;
	int show = 0;

	if (tcount0 != tcount1) {
		printf("compare: transactions disagree; rtl %d, simh %d\n",
		       tcount1, tcount0);
		show = 1;
	}

	for (i = 0; i < tcount0; i++) {
		if (transactions[0][i].what != transactions[1][i].what ||
/*		    transactions[0][i].rw != transactions[1][i].rw || */
/*		    transactions[0][i].pa != transactions[1][i].pa || */
		    (transactions[0][i].pa & 0xffff) !=
		    (transactions[1][i].pa & 0xffff) ||
		    transactions[0][i].data != transactions[1][i].data)
		{
			printf("compare: transaction %d differs\n", i+1);
			show = 1;
		}
	}

	t = tcount0;
	if (tcount1 < tcount0)
		t = tcount1;

	if (show) {
		printf("compare: transaction disagree\n");
		printf("simh\t\t| rtl\n");
		for (i = 0; i < t; i++) {
			printf("%d: %s %s %o %o | %s %s %o %o\n",
			       i+1, 
			       text_what(transactions[0][i].what),
			       text_rw(transactions[0][i].rw),
			       transactions[0][i].pa,
			       transactions[0][i].data,
			       text_what(transactions[1][i].what),
			       text_rw(transactions[1][i].rw),
			       transactions[1][i].pa,
			       transactions[1][i].data);
		       
		}
	}
}

void record_transaction(int who, int rw, int what, unsigned int pa,
                        unsigned int data)
{
	int t;

	t = (who == T_SIMH) ? tcount0 : tcount1;

	if (0) printf("[%d][%d] %d %d %o %o\n",
	       who-1, t, rw, what, pa, data);

	transactions[who-1][t].rw = rw;
	transactions[who-1][t].what = what;
	transactions[who-1][t].pa = pa;
	transactions[who-1][t].data = data;

	if (who == T_SIMH)
		tcount0++;
	else
		tcount1++;
}

void simh_record_mem_read_word(u22 pa, u16 data)
{
printf("simh: readw %o -> %o\n", pa, data & 0xffff);
record_transaction(T_SIMH, T_READW, T_MEM, pa, data);
}

void simh_record_io_read_word(u22 pa, u16 data)
{
printf("simh: io read %o -> %o\n", pa, data & 0xffff);
record_transaction(T_SIMH, T_READW, T_IO, pa, data);
}

void simh_record_mem_write_word(u22 pa, u16 data)
{
printf("simh: writew %o <- %o\n", pa, data & 0xffff);
record_transaction(T_SIMH, T_WRITEW, T_MEM, pa, data);
}

void simh_record_io_write_word(u22 pa, u16 data)
{
printf("simh: io write %o <- %o\n", pa, data & 0xffff);
record_transaction(T_SIMH, T_WRITEW, T_IO, pa, data);
}

void simh_record_mem_read_byte(u22 pa, u16 data)
{
printf("simh: readb %o -> %o\n", pa, data & 0xffff);
record_transaction(T_SIMH, T_READB, T_MEM, pa, data);
}

void simh_record_io_read_byte(u22 pa, u16 data)
{
printf("simh: io readb %o -> %o\n", pa, data & 0xffff);
record_transaction(T_SIMH, T_READB, T_IO, pa, data);
}

void simh_record_mem_write_byte(u22 pa, u16 data)
{
printf("simh: writeb %o <- %o\n", pa, data & 0xffff);
record_transaction(T_SIMH, T_WRITEB, T_MEM, pa, data);
}

void simh_record_io_write_byte(u22 pa, u16 data)
{
printf("simh: io writeb %o <- %o\n", pa, data & 0xffff);
record_transaction(T_SIMH, T_WRITEB, T_IO, pa, data);
}

void simh_report_pc(int PC, int IR)
{
printf("simh: pc %06o isn %06o\n", PC, IR);
}


/* --- */

void rtl_record_mem_read_word(u22 pa, u16 data)
{
if (1) printf("READ-MEM %6o -> %o\n", pa, data);
record_transaction(T_RTL, T_READW, T_MEM, pa, data);
}

void rtl_record_io_read_word(u22 pa, u16 data)
{
if (1) printf("READ-IO %6o -> %o\n", pa, data);
record_transaction(T_RTL, T_READW, T_IO, pa, data);
}

void rtl_record_mem_write_word(u22 pa, u16 data)
{
if (1) printf("WRITE-MEM %6o <- %6o\n", pa, data);
record_transaction(T_RTL, T_WRITEW, T_MEM, pa, data);
}

void rtl_record_io_write_word(u22 pa, u16 data)
{
if (1) printf("WRITE-IO %6o <- %6o\n", pa, data);
record_transaction(T_RTL, T_WRITEW, T_IO, pa, data);
}

void rtl_record_mem_read_byte(u22 pa, u16 data)
{
printf("READB-MEM %o -> %o\n", pa, data & 0xffff);
record_transaction(T_RTL, T_READB, T_MEM, pa, data);
}

void rtl_record_io_read_byte(u22 pa, u16 data)
{
printf("READB-IO %o -> %o\n", pa, data & 0xffff);
record_transaction(T_RTL, T_READB, T_IO, pa, data);
}

void rtl_record_mem_write_byte(u22 pa, u16 data)
{
printf("WRITEB-MEM %o <- %o\n", pa, data & 0xffff);
record_transaction(T_RTL, T_WRITEB, T_MEM, pa, data);
}

void rtl_record_io_write_byte(u22 pa, u16 data)
{
printf("WRITEB-IO %o <- %o\n", pa, data & 0xffff);
record_transaction(T_RTL, T_WRITEB, T_IO, pa, data);
}

