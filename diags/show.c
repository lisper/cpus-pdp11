#include <stdio.h>

typedef unsigned char u8;
typedef unsigned short u16;

u8 buffer[1024*64];

u16 M[1024*64];
u8 hit[1024*64];

main()
{
	int size, o;
	unsigned orig, count;

	size = read(0, buffer, sizeof(buffer));
	printf("size %d\n", size);

#if 0
	for (o = 0; o < size; o++) {
		if (buffer[o] != 0)
			break;
	}
	printf("start at offset %d\n", o);
	for (; o < size;) {
		if (buffer[o] != 0001) {
			printf("missing block maker, offset %d\n", o);
			break;
		}
		count = buffer[o+2] | (buffer[o+3] << 8);
		orig = buffer[o+4] | (buffer[o+5] << 8);

		printf("%02x %02x %02x %02x %02x %02x\n",
		       buffer[o], buffer[o+1], buffer[o+2], buffer[o+3],
		       buffer[o+4], buffer[o+5]);

		printf("offset %d; orig %o, count %d (0x%x)\n",
		       o, orig, count, count);
		o += count;
		if (count > 6) o++;
		if (orig != 0000001) printf("pc %o\n", orig);
	}
#else
	int state, csum, i;
	unsigned int PC, origin;

	state = csum = 0;
	for (o = 0; o < size; o++) {
		i = buffer[o];
		csum = csum + i;
		//printf("offset %o, state %d\n", o, state);
		switch (state) {

		case 0:					/* leader */
			if (i == 1) state = 1;
			else csum = 0;
			break;

		case 1:					/* ignore after 001 */
			state = 2;
			break;

		case 2:					/* low count */
			count = i;
			state = 3;
			break;

		case 3:					/* high count */
			count = (i << 8) | count;
			state = 4;
			break;

		case 4:					/* low origin */
			origin = i;
			state = 5;
			break;

		case 5:					/* high origin */
			origin = (i << 8) | origin;
			printf("origin %o, count %d, offset 0x%x\n", origin, count, o);
			if (count == 6) {
				PC = 0;
				if (origin != 1) PC = origin & 0177776;
				printf("PC %o\n", PC);
				goto done;
			}
			count = count - 6;
			state = 6;
			break;

		case 6:					/* data */
			hit[origin] = 1;
			M[origin >> 1] = (origin & 1)?
				(M[origin >> 1] & 0377) | (i << 8):
				(M[origin >> 1] & 0177400) | i;
			origin = origin + 1;
			count = count - 1;
			state = state + (count == 0);
			break;

		case 7:					/* checksum */
			if (csum & 0377) printf("bad checksum %d\n", o);
			csum = state = 0;
			break;
		}
	}

done:
	printf("done, pc %o\n", PC);

	printf("dump:\n");
	for (i = 0; i < 64*1024; i += 2) {
		if (hit[i] || hit[i+1]) {
			printf("  0%06o, 0%06o,\n", i, M[i >> 1]);
		}
	}
#endif
}
