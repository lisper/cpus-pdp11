#include <stdio.h>
#include <stdlib.h>

main()
{
	while (1) {
		int r;
		unsigned int a, v;
		char b[2];
		r = scanf("%o %o\n", &a, &v);
		if (0) printf("%06o %06o\n", a, v);
		if (r < 2)
			break;

		b[0] = v & 0xff;
		b[1] = v >> 8;
		write(1, (char *)b, 2);
	}
	exit(0);
}
