#include <stdio.h>
main()
{
	char line[1024];
	int pc, sp, psw, r[8];
	while (fgets(line, sizeof(line), stdin)) {
		if (memcmp(line, "f1: ", 4) == 0) {
			sscanf(line+4, "pc=%o, sp=%o, psw=%o",
			       &pc, &sp, &psw);
			fgets(line, sizeof(line), stdin);
			fgets(line, sizeof(line), stdin);
			sscanf(line, "regs %o %o %o %o",
			       &r[0], &r[1], &r[2], &r[3]);
			fgets(line, sizeof(line), stdin);
			sscanf(line, "regs %o %o %o %o",
			       &r[4], &r[5], &r[6], &r[7]);

			printf("pc=%06o psw=%06o %06o %06o %06o %06o %06o %06o %06o %06o\n",
			       pc, psw, 
			       r[0], r[1], r[2], r[3],
			       r[4], r[5], r[6], r[7]);
		}
	}
}
