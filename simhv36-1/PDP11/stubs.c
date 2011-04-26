#include <stdio.h>

//broken
//#include "pdp11_defs.h"

int show_m = 1;
int show_i = 1;
int isn_count;
int max_cycles;

#include "stubs.h"

void simh_record_mem_read_word(unsigned int pa, unsigned short data)
{
	if (show_m) printf("ram: read %o -> %o\n", pa, data);
}

void simh_record_io_read_word(unsigned int pa, unsigned short data)
{
	if (show_m) printf("bus: iopage read %o -> %o (byte 0, error 0)\n",
			   pa, data);
}

void simh_record_mem_write_word(unsigned int pa, unsigned short data)
{
	if (show_m) printf("ram: write %o <- %o\n", pa, data);
}

void simh_record_io_write_word(unsigned int pa, unsigned short data)
{
	if (show_m) printf("bus: iopage write %o <- %o (byte 0, error 0)\n",
			   pa, data);
}

void simh_record_mem_read_byte(unsigned int pa, unsigned short data)
{
	if (show_m) {
		if (pa & 1) printf("ram: readh %o -> %o\n", pa, data);
		else printf("ram: readl %o -> %o\n", pa, data);
	}
}

void simh_record_io_read_byte(unsigned int pa, unsigned short data)
{
	if (show_m) printf("bus: iopage read %o -> %o (byte 1, error 0)\n",
			   pa, data);
}

void simh_record_mem_write_byte(unsigned int pa, unsigned short data)
{
	if (show_m) {
		if (pa & 1) printf("ram: writeh %o <- %o\n", pa, data);
		else printf("ram: writel %o <- %o\n", pa, data);
	}
}

void simh_record_io_write_byte(unsigned int pa, unsigned short data)
{
	if (show_m) printf("bus: iopage write %o <- %o (byte 1, error 0)\n",
			   pa, data);
}

typedef unsigned int uint32;
typedef int int32;
extern int fprint_sym (FILE *ofile, uint32 addr, uint32 *val,
			  uint32 *uptr, int32 sw);
#define SWMASK(x) (1u << (((int) (x)) - ((int) 'A')))
extern int cpu_unit[];

void simh_report_pc(int PC, int IR)
{
	uint32 v[4];
	uint32 o = PC/2;
	extern unsigned short *M;

	{ extern int sim_quiet; if (sim_quiet) show_i = 0; }

	if (show_i) {
		//printf("simh: pc %06o isn %06o\n", PC, IR);
//		v[0] = M[o];
//		v[1] = M[o+1];
//		v[2] = M[o+2];
		extern int isenable;
		unsigned int pa, va;
		extern int MMR0;

		pa = relocR(PC | isenable); 
		v[0] = ReadE ((PC+0) | isenable);
		v[1] = ReadE ((PC+2) | isenable);
		v[2] = ReadE ((PC+4) | isenable);

		printf("f2: pc %06o (%08o->%06o) ", PC, pa, v[0]);
		fprint_sym(stdout, PC, v, &cpu_unit, SWMASK('M'));
		printf("\n");

#if 1
#define MMR0_MME        0000001                         /* mem mgt enable */
#define VA_V_APF        13                              /* offset to APF */
#define VA_DF           0017777                         /* displacement */
#define MAXMEMSIZE      020000000                       /* 2**22 */
#define PAMASK          (MAXMEMSIZE - 1)                /* 2**22 - 1 */
		va = PC | isenable;
		if (va == 054766 && (MMR0 & MMR0_MME)) {
			int32 apridx, apr, pa;
			extern int32 APRFILE[64];
			apridx = (va >> VA_V_APF) & 077; /* index into APR */
			apr = APRFILE[apridx]; /* with va<18:13> */
			pa = ((va & VA_DF) + ((apr >> 10) & 017777700)) & PAMASK;
			printf("apridx %o, apr %o, pa %o\n",
			       apridx, apr, pa);
		}
#endif
	}
}

void raw_read_memory() {}
void raw_write_memory() {}
void cpu_int_clear() {}
void cpu_int_set() {}
