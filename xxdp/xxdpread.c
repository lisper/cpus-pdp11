/* 
 * xxdpread.c
 * simple program to extract files from xxdp disk image
 * brad@heeltoe.com 4/2009
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

typedef unsigned short u16;

int fd;

/* map a character into its radix 50 representation */
int ator50(int ascii)
{
	switch(ascii) {
	case ' ': return 0;
	case '$': return 033;
	case '.': return 034;
	}
	ascii = toupper(ascii);

	if ((ascii -= '0') <= 9)
		return ascii + 036;

	if ((ascii -= ('A' - '0')) <= ('Z' - 'A'))
		return ascii + 1;

	return 255;
}


/* convert ascii file name to radix-50 */
int
name_to_rdx50(char *ascii, u16 *fn)
{
	int c;
	char i, j, *pntr;

	if (0) printf("name_to_rdx50(ascii='%s',fn=%p)\n",
		      ascii, fn);

	pntr = ascii;
	fn[0] = fn[1] = fn[2] = 0;

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 3; j++) {
			c = *pntr;
			if (c == '.' || c == '\0') c = ' ';
			else pntr++;
			if ((c = ator50(c)) == 255) return 0;
			fn[i] = fn[i] * 050 + c;
		}
	}

	if (*pntr == '.')
		pntr++;

	for (i = 0; i < 3; i++) {
		c = *pntr;
		if (c == '\0') c = ' ';
		else pntr++;
		if ((c = ator50(c)) == 255)
			return 0;
		fn[2] = fn[2] * 050 + c;
	}

	return 1;
}

/*
 * convert a radix 50 character into an ASCII character
 * returns the character * if the rad 50 character is illegal
 */

char r50toa(char rad50)
{
	switch (rad50) {
	case 0: return ' ';
	case 033: return '$';
	case 034: return '.';
	}

	if (rad50-- <= 031)
		return rad50 + 'A';

	if ((rad50 -= 035) <= 9) 
		return rad50 + '0';

	return '*';
}


/* convert an RT-11 file from radix 50 into an ASCII string */
char *
filename_text(u16 *fn, char *str)
{
	u16 t;
	char *p = str;

	t = fn[0];
	p[2] = r50toa(t % 050); t /= 050;
	p[1] = r50toa(t % 050); t /= 050;
	p[0] = r50toa(t % 050); t /= 050;

	t = fn[1];
	p[5] = r50toa(t % 050); t /= 050;
	p[4] = r50toa(t % 050); t /= 050;
	p[3] = r50toa(t % 050); t /= 050;
	p[6] = '.';

	t = fn[2];
	p[9] = r50toa(t % 050); t /= 050;
	p[8] = r50toa(t % 050); t /= 050;
	p[7] = r50toa(t % 050); t /= 050;
	p[10] = 0;

	return str;
}

int
open_file(char *filename)
{
	fd = open(filename, O_RDONLY);
	return 0;
}

int
read_block(int block, u_char *buf)
{
	int ret;
	off_t offset;

	offset = block * 512;
	ret = lseek(fd, offset, SEEK_SET);
	if (ret < 0) {
		perror("lseek");
		return -1;
	}

	ret = read(fd, buf, 512);
	if (ret < 0) {
		perror("read");
		return -1;
	}

	return 0;
}

u_char homeblock[512];
u_char dirblock[512];
u_char datablock[512];

struct homeblock_s {
	u16 link;
	u16 first_dir_block;
	u16 dir_blocks;
	u16 first_bitmap_block;
	u16 bitmap_blocks;
	u16 unknown1;
	u16 unknown2;
	u16 disk_blocks;
	u16 data_block;
};

int
read_homeblock(void)
{
	struct homeblock_s *h = (struct homeblock_s *)homeblock;
	read_block(1, homeblock);

	printf("first dir block    %d\n", h->first_dir_block);
	printf("dir blocks         %d\n", h->dir_blocks);
	printf("first bitmap block %d\n", h->first_bitmap_block);
	printf("bitmap blocks      %d\n", h->bitmap_blocks);
}

int
read_allocbitmap(void)
{
}

struct dirent_s {
	u16 rad50_name[3];
	u16 date;
	u16 unknown;
	u16 start;
	u16 length;
	u16 last;
	u16 flags;
};

char *rad50_text(u16 *pn, char *buf)
{
	strcpy(buf, "      ");
	return buf;
}

char *date_text(u16 date, char *buf)
{
	strcpy(buf, "xx-xxx-xx");
	return buf;
}

int
show_dir(void)
{
	struct homeblock_s *h = (struct homeblock_s *)homeblock;
	struct dirent_s *d;
	int i, j;
	int next_block;

	next_block = h->first_dir_block;

	for (i = 0; i < h->dir_blocks; i++) {
		char buf1[10], buf2[10];

		//printf("block %d\n", next_block);
		read_block(next_block, dirblock);

		d = (struct dirent_s *)(dirblock + 2);
		for (j = 0; j < 29; j++, d++) {
			if (d->rad50_name[0] == 0 &&
			    d->rad50_name[1] == 0 &&
			    d->rad50_name[2] == 0)
				continue;

			printf("%s %s %c %5d %5d %5d %4x\n",
			       filename_text(d->rad50_name, buf1),
			       date_text(d->date, buf2),
			       d->date & 0x8000 ? 'C' : ' ',
			       d->start, d->length, d->last, d->flags);
		}

		next_block = (dirblock[1] << 8) | dirblock[0];
	}

	return 0;
}

int
search_dir(u16 *fn, struct dirent_s *dd)
{
	struct homeblock_s *h = (struct homeblock_s *)homeblock;
	struct dirent_s *d;
	int i, j;
	int next_block;

	next_block = h->first_dir_block;

	for (i = 0; i < h->dir_blocks; i++) {
		char buf1[10], buf2[10];

		read_block(next_block, (char *)&dirblock);

		d = (struct dirent_s *)(dirblock + 2);
		for (j = 0; j < 29; j++, d++) {
			if (d->rad50_name[0] == 0 &&
			    d->rad50_name[1] == 0 &&
			    d->rad50_name[2] == 0)
				continue;

			if (d->rad50_name[0] == fn[0] &&
			    d->rad50_name[1] == fn[1] &&
			    d->rad50_name[2] == fn[2])
			{
				*dd = *d;
				return 0;
			}
		}

		next_block = (dirblock[1] << 8) | dirblock[0];
	}

	return -1;
}

int
copy_file(struct dirent_s *d, char *ascii)
{
	int f, next_block, blocks, ret;
	unsigned bytes;

	printf("extracting file %s...\n", ascii);

	f = open(ascii, O_RDWR | O_CREAT, 0666);
	if (f < 0) {
		perror(ascii);
		return -1;
	}

	bytes = 0;
	blocks = 0;
	next_block = d->start;
	while (next_block != 0) {
		//printf("data block %d\n", next_block);
		read_block(next_block, (char *)&datablock);

		ret = write(f, datablock+2, 510);
		bytes += 510;
		blocks++;

		next_block = (datablock[1] << 8) | datablock[0];
	}

	close (f);

	printf("%u bytes, %d blocks\n", bytes, blocks);
	return 0;
}

int
extract_file(char *ascii)
{
	u16 fn[3];
	struct dirent_s d;

	name_to_rdx50(ascii, fn);
	if (search_dir(fn, &d)) {
		printf("can't find file %s\n", ascii);
		return -1;
	}
	copy_file(&d, ascii);

	return 0;
}

main(int argc, char *argv[])
{
	char *image, *file;

	if (argc < 2) {
		fprintf(stderr, "usage: xxdpread <disk-image-filename> "
			"{<extract-name>}\n");
		exit(1);
	}

	image = argv[1];

	open_file(image);
	read_homeblock();
	read_allocbitmap();
	show_dir();

	if (argc > 2) {
		file = argv[2];
		extract_file(file);
	}

	exit(0);
}
