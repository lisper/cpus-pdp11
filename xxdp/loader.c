#include <stdio.h>
#include <fcntl.h>
#include <string.h>

u_char buf[256];
u_char tmp[2];

main()
{
	int i, ret, offset;
	unsigned bc, sum, cs;

	offset = 0;
	while (1) {
		/* ----- */
		ret = read(0, buf, 4);
		if (ret <= 0)
			break;

		if (buf[1] != 0 || buf[0] != 1) {
			printf("bad signature @ %d %02x %02x\n",
			       offset, buf[0], buf[1]);
			return -1;
		}
		bc = (buf[3] << 8) | buf[2];
		bc -= 6;
		offset += 4;
		sum = buf[0] + buf[1] + buf[2] + buf[3];

		printf("bc %d\n", bc);

		/* ----- */
		ret = read(0, buf, bc);
		if (ret != bc) {
			printf("short read; @ %d, wanted %d\n", offset, bc);
			break;
		}
		offset += bc;
		for (i = 0; i < bc; i++)
			sum += buf[i];

		/* ----- */
		ret = read(0, tmp, 2);
		if (ret != 2) {
			printf("missing checksum @ %d\n", offset);
			break;
		}
		offset += 2;
		cs = (tmp[1] << 8) | tmp[0];
		printf("cs %x, sum %x %x\n", cs, sum, -sum);
	}
}
