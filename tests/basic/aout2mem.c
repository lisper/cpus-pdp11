#include <stdio.h>
#include <stdlib.h>

/*
 * dump BSD style a.out text as octal ".mem" format
 */

unsigned char buffer[256];

typedef unsigned char u8;

struct {
	u8 magic[2];
	u8 version[2];
	u8 tsize[4];
	u8 dsize[4];
	u8 bsize[4];
	u8 entry[4];
	u8 tstart[4];
	u8 dstart[4];
	u8 bstart[4];
	u8 unused[4*3];
} hdr;

main()
{
	int ret, loc, v;
	unsigned int tsize;

	ret = read(0, (char *)&hdr, 16);
	if (ret != 16) {
		perror("stdin");
		exit(1);
	}

	tsize = (hdr.version[3] << 24) |
		(hdr.version[2] << 16) |
		(hdr.version[1] <<  8) |
		(hdr.version[0] <<  0);

	fprintf(stderr,"tsize %08x\n", tsize);

	loc = 0500;
	while (tsize > 0) {
		ret = read(0, (char *)buffer, 2);
		if (ret < 2)
			break;

		v = (buffer[1] << 8) | buffer[0];
		printf("0%06o 0%06o\n", loc, v);
		loc += 2;
		tsize -= 2;
	}

	exit(0);
}
