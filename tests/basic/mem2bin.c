/*
 * take a .mem file and create a binary load file suitable
 * for loading by simh
 */
#include <stdio.h>
#include <stdlib.h>

void dumpblock(unsigned int base, int bsize, unsigned short *b)
{
	unsigned char buf[6];
	int ret, count, checksum;

	if (0) printf("block @ %o, words %d\n", base, bsize);

	count = 6 + (bsize * 2);

	buf[0] = 1;
	buf[1] = 0;
	buf[2] = count & 0xff;
	buf[3] = count >> 8;
	buf[4] = base & 0xff;
	buf[5] = base >> 8;
	ret = write(1, buf, 6);
	checksum = buf[0] + buf[1] + buf[2] + buf[3] + buf[4] + buf[5];

	while (bsize--) {
		buf[0] = *b;
		buf[1] = *b >> 8;
		ret = write(1, buf, 2);
		b++;
		checksum += buf[0];
		checksum += buf[1];
	}

	buf[0] = -checksum;
	ret = write(1, buf, 1);
	if (0) fprintf(stderr, "checksum %08x, result %08x\n",
		       checksum,  checksum + buf[0]);
}

main()
{
	int ret;
	unsigned short block[1024];
	int bsize;
	unsigned a, v, base, next;

	bsize = 0;
	while (1) {
		ret = scanf("%o %o\n", &a, &v);
		if (ret < 2)
			break;

		if (bsize == 0) {
		nb:
			base = a;
			block[bsize++] = v;
			next = a + 2;
		} else {
			if (a == next) {
				block[bsize++] = v;
				next = a + 2;
			} else {
				dumpblock(base, bsize, block);
				bsize = 0;
				goto nb;
			}
		}
	}

	if (bsize) {
		dumpblock(base, bsize, block);
		bsize = 0;
	}

	dumpblock(base, bsize, block);

	exit(0);
}
