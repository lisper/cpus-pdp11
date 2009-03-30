#include <stdio.h>
#include <stdlib.h>

void simh_record_mem_read_word(pa, data)
{
}

void simh_record_io_read_word(pa, data)
{
}

void simh_record_mem_write_word(pa, data)
{
}

void simh_record_io_write_word(pa, data)
{
}

void simh_record_mem_read_byte(pa, data)
{
}

void simh_record_io_read_byte(pa, data)
{
}

void simh_record_mem_write_byte(pa, data)
{
}

void simh_record_io_write_byte(pa, data)
{
}

void simh_report_pc(int PC, int IR)
{
//	printf("simh: pc %06o isn %06o\n", PC, IR);
}

void raw_read_memory() {}
void raw_write_memory() {}
void cpu_int_clear() {}
void cpu_int_set() {}

main()
{
	printf("tester\n");

	simh_init();
	simh_command("set cpu 11/34");
	simh_command("set cpu 256k");
        simh_command("att rk0 rk.dsk");
        simh_command("boot rk");

//	while (1) {
//		simh_step();
//	}

	simh_exit();
	exit(0);
}
