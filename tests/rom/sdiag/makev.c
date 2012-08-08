#include <stdio.h>

main()
{
	unsigned int a, v;
	char line[1024];
	while (fgets(line,sizeof(line),stdin)) {
		sscanf(line, "%o %o", &a, &v);
		printf("%d: fetch = 16'o%06o;\n", a & 0x3ff, v);
	}
}
