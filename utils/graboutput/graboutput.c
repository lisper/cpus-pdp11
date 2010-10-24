/* collect output from log files and present in a readable fashion */
#include <stdio.h>
#include <stdlib.h>

main()
{
	char line[1024], str[128], str2[128];
	int ich, cch, r;
	char ch;
	while (fgets(line, sizeof(line), stdin)) {
		/* cver */
		if (memcmp(line, "tto_data ", 9) == 0) {
			r = sscanf(line, "%s %o %c\n",
				   str, &ich, &ch);
			if (ich != 0) {
				printf("%c", ich & 0x7f);
				fflush(stdout);
			}
		}
		/* modelsim */
		if (memcmp(line+2, "tto_data ", 9) == 0) {
			r = sscanf(line+2, "%s %o %c\n",
				   str, &ich, &ch);
			if (ich != 0) {
				printf("%c", ich & 0x7f);
				fflush(stdout);
			}
		}
		/**/
		//_io_tto_write() addr=17777566 data=170; simh
		if (memcmp(line, "_io_tto_write", 13) == 0) {
			r = sscanf(line+2, "%s %s data=%o\n",
				   str, str2, &ich);
			if (ich != 0) {
				printf("%c", ich & 0x7f);
				fflush(stdout);
			}
		}
	}
	printf("\n\n");
	exit(0);
}
