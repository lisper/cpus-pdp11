/* simple rl02 emulation */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "binre.h"
#include "mach.h"
#include "isn.h"
#include "support.h"

extern int initial_pc;

/* register offsets */
#define	CS	0
#define	BA	2
#define	DA	4
#define	MP	6

/* CS */
#define CS_DRDY		0000001		/* drive ready - r/o */
#define CS_FUNC		0000016		/* function */
#define  CS_FUNC_NOP		0
#define  CS_FUNC_WRITECHK	1
#define  CS_FUNC_GETSTATUS	2
#define  CS_FUNC_SEEK		3
#define  CS_FUNC_READHDR	4
#define  CS_FUNC_WRITE		5
#define  CS_FUNC_READ		6
#define  CS_FUNC_READNOHDR	7

#define CS_BA1617	0000060		/* BA17,BA16 */
#define CS_IE           0000100
#define CS_CRDY		0000200
#define CS_DS		0001400
#define CS_E		0036000

#define CS_E_OPI	(1 << 10)
#define CS_E_DCRC	(2 << 10)
#define CS_E_HCRC	(3 << 10)
#define CS_E_DLT	(4 << 10)
#define CS_E_HNF	(5 << 10)
#define CS_E_NXM	(8 << 10)
#define CS_E_MPE	(9 << 10)

#define CS_DE		0040000		/* drive error */
#define CS_ERR		0100000

#define CS_ANY_ERR	(CS_ERR | (017 << 10))
#define CS_RW		0001776		/* read/write bits */

/* DA */
#define DA_SEEK		0000002
#define DA_DIR		0000004
#define DA_CLR		0000010
#define DA_HS		0000020

/* MP bits for get status */
#define MP_GS_ST	0000007
#define MP_GS_ST_LOAD	0	/* Load cartridge */
#define MP_GS_ST_SPINUP	1	/* Spin-up */
#define MP_GS_ST_BRUSH	2	/* Brush cycle */
#define MP_GS_ST_LOADH	3	/* Load heads */
#define MP_GS_ST_SEEK	4	/* Seeks */
#define MP_GS_ST_LOCK	5	/* lock on */
#define MP_GS_ST_UNLDH	6	/* Unload heads */
#define MP_GS_ST_SPINDN	7	/* Spin-down */

#define MP_GS_BH	0000010		/* brush home */
#define MP_GS_HO	0000020		/* heads out */
#define MP_GS_CO	0000040		/* cover open */
#define MP_GS_HS	0000100		/* head select */
#define MP_GS_DT	0000200		/* drive type */
#define MP_GS_DSE	0000400		/* drive-select error */
#define MP_GS_VC	0001000		/* volume check */
#define MP_GS_WGE	0002000		/* write gate error */
#define MP_GS_SPE	0004000		/* spin error */
#define MP_GS_SKTO	0010000		/* seek time out */
#define MP_GS_WL	0020000		/* write locked */
#define MP_GS_CHE	0040000		/* current head error */
#define MP_GS_WDE	0100000		/* write data error */

#define RL11_BASE	017774400
#define RL11_VECTOR	0160


int rl_fd;

static unsigned short cs;
static unsigned short ba;
static unsigned short da;
static unsigned short mp[3];
static unsigned short mp_gs;

static u_char ds10;		/* currently selected drive */

static u_char cmd_pending;
static u_char int_pending;
static u_char init_pending;

static u_char seek_pending;
static u_short seek_time;
static short seek_ms;


static struct drive_s {
    char ready;
    char rl02;
    char write_prot;

    u_short da;
    u_short cyls;

    char curr_head;
    u_short curr_cyl;

    char new_head;
    u_short new_cyl;
} drive[4];

/* 256 byte block */
static u_short buffer2[128];

#define byte_place(addr, old, byte) \
    (((addr) & 1) ? ((old) & 0377) | ((byte) << 8) : ((old) & ~0377) | (byte))

static void inline
update_cs(void)
{
    if (cs & CS_ANY_ERR)
        cs |= CS_ERR;

    /* check drive ready */
    ds10 = (cs >> 8) & 3;
    if (drive[ds10].ready) {
        cs |= CS_DRDY;
        mp_gs = MP_GS_HO | MP_GS_BH | MP_GS_ST_LOCK;
    } else {
        cs &= ~CS_DRDY;
        mp_gs = MP_GS_CO | MP_GS_ST_LOAD /*| MP_GS_SPE*/;
    }
}

static inline void
cmd_done()
{
    cs |= CS_CRDY;
    if (cs & CS_IE) {
	int_pending = 1;
    } else {
	int_pending = 0;
        cpu_int_clear(3);
    }
}

static void
unibus_dma_buffer(int write, int pa, ushort *wbuff, int wlen)
{
    while (wlen > 0) {

	if (write) {
	    raw_write_memory(pa, *wbuff);
	} else {
	    *wbuff = raw_read_memory(pa);
	}

	wbuff++;
	wlen--;
	pa += 2;
    }
}

static void 
read_disk_block256(int unit, int blockno, u16 **bufferp)
{
    int ret;
    off_t offset;
    static u16 b[256];

    offset = blockno*256;
    lseek(rl_fd, offset, SEEK_SET);
    ret = read(rl_fd, b, 256);
    if (ret < 0) {
	perror("write");
    }
    *bufferp = b;
}

static void 
write_disk_block256(int unit, int blockno, u16 *buffer)
{
    int ret;
    off_t offset;

    offset = blockno*256;
    lseek(rl_fd, offset, SEEK_SET);
    ret = write(rl_fd, buffer, 256);
    if (ret < 0) {
	perror("write");
    }
}

static void
rl11_drive_online(void)
{
    int i;

    printf("drive %d online\n", 0);
    drive[0].ready = 1;
    drive[0].rl02 = 1;

    if (drive[ds10].ready)
        mp_gs = MP_GS_HO | MP_GS_BH | MP_GS_ST_LOCK;

    update_cs();
}

static void
rl11_drive_offine(void)
{
    drive[ds10].ready = 0;
    mp_gs = MP_GS_CO | MP_GS_ST_LOAD;
    printf("drive %d offline\n", ds10);

    update_cs();
}

static void
rl11_reset(void)
{
    int i;

printf("rl11_reset\n");
    cmd_pending = 0;
    int_pending = 0;
    init_pending = 1;

    cs = CS_CRDY;
    ba = 0;
    da = 0;
    mp[0] = mp[1] = mp[2] = 0;
    ds10 = 0;

    rl11_drive_offine();
    update_cs();

    for (i = 0; i < 4; i++) {
        memset((char *)&drive[i], 0, sizeof(struct drive_s));
        drive[i].ready = 0;
        drive[i].rl02 = 0;
        drive[i].write_prot = 0;
        drive[i].da = 0;
        drive[i].cyls = 512;

        drive[i].curr_cyl = 0;
        drive[i].curr_head = 0;
        drive[i].new_cyl = 0;
        drive[i].new_head = 0;
    }
}

void
rl11_poll(void)
{
    u_short offset, da_cyl;
    short newcyl, maxcyl, wlen;
    u_char func, da_hd, da_sect, max_sectors, swlen, unit;
    int i, phys_addr, blockno;

    if (int_pending) {
        int_pending = 0;
        //unibus_interrupt();
        cpu_int_set(3);
    }

    if (cmd_pending) {
        cmd_pending = 0;

        func = (cs >> 1) & 7;

        if (0) printf("rl%d: cmd %d cs %o %c%c\n",
                      ds10, func, cs,
                      (cs & CS_CRDY) ? 'r' : '-',
                      (cs & CS_IE) ? 'i' : '-' );

        switch (func) {
        case CS_FUNC_NOP:
            cmd_done();
            break;

        case CS_FUNC_GETSTATUS:
            /* check da */
//hack
mp_gs = MP_GS_HO | MP_GS_BH | MP_GS_ST_LOCK;
            if (da & DA_CLR) {
                mp_gs &=
                    ~(MP_GS_DSE | MP_GS_VC | MP_GS_WGE | MP_GS_SPE | 
                      MP_GS_SKTO | MP_GS_CHE | MP_GS_WDE);
            }

            mp_gs &= ~MP_GS_HS;
            mp_gs |= (drive[ds10].curr_head ? MP_GS_HS : 0);

            if (drive[ds10].rl02)
                mp_gs |= MP_GS_DT;
            if (drive[ds10].write_prot)
                mp_gs |= MP_GS_WL;

//hack
mp_gs |= MP_GS_DT;

            mp[0] = mp[1] = mp[2] = mp_gs;
//printf("write mp[0], mp_gs %o\n", mp_gs);
//printf("getstatus: %o\n", mp_gs);
            cmd_done();
            break;

        case CS_FUNC_SEEK:
            offset = da >> 7;

            if (0) printf("seek: at cyl %o, da %o, offset %o\n",
                          drive[ds10].curr_cyl, da, offset);

            if (da & DA_DIR) {
                /* forward */
                newcyl = drive[ds10].curr_cyl + offset;
                maxcyl = drive[ds10].cyls;
                if (newcyl >= maxcyl)
                    newcyl = maxcyl - 1;
            } else {
                /* reverse */
                newcyl = drive[ds10].curr_cyl - offset;
                if (newcyl < 0)
                    newcyl = 0;
            }

            drive[ds10].new_head = (da >> 4) & 1;
            drive[ds10].new_cyl = newcyl;

            drive[ds10].da = newcyl << 7 | (da & DA_HS);

            seek_pending = 1;
            seek_time = (newcyl - drive[ds10].curr_cyl) * seek_ms;
            if (0) printf("seek: to cyl %o\n", drive[ds10].new_cyl);
#if 0
seek_pending = 0;
seek_time = 0;
drive[0].curr_head = drive[ds10].new_head;
drive[0].curr_cyl = drive[ds10].new_cyl;
cmd_done();
#else
seek_time = 1000;
#endif
            break;

        case CS_FUNC_READHDR:
            mp[0] = (drive[ds10].curr_cyl << 7) | 
                (drive[ds10].curr_head << 6) |
                (da & 077);
            if (0) printf("readhdr; ba %o, da %o, mp %o\n", ba, da, mp[0]);
            mp[1] = 0;
            mp[2] = 0; /* crc */
            cmd_done();
            break;

        case CS_FUNC_READNOHDR:
            break;

        case CS_FUNC_WRITE:
            /* are we write-protected? */
            if (drive[ds10].write_prot) {
                mp_gs |= MP_GS_WGE;
                cs |= CS_ERR | CS_DE;
                cmd_done();
                break;
            }
            /* fall through */

        case CS_FUNC_READ:
        case CS_FUNC_WRITECHK:
            unit = (cs >> 8) & 3;
            da_cyl = da >> 7;
            da_hd = (da >> 6) & 1;
            da_sect = da & 077;

            if (0) printf("read: ba %o, da %o, wc %o\n", ba, da, mp[0]);

printf("read: da %o, offset %o, u %d, c %d, h %d s %d\n",
                  da, ((da >> 6)*40 + da_sect)*256,
                  unit, da_cyl, da_hd, da_sect);

            drive[ds10].curr_head = (da >> 4) & 1;

#if 0
            if (drive[ds10].curr_cyl != da_cyl || da_sect >= 40) {
                /* cyl doesn't match */
printf("cyl! %d %d %d %d\n", ds10, drive[ds10].curr_cyl, da_cyl, da_sect);
printf("da %x\n", da);
// just for now - looks like we complete seek too soon
//                cs |= CS_ERR | CS_E_HCRC | CS_E_OPI;
            }
#endif

            phys_addr = (((cs & CS_BA1617) >> 4) << 16) | ba;
            wlen = 02000000 - mp[0];
printf("rl read: wlen %o, mp[0] %o\n", wlen, mp[0]);

            /* clamp wlen at remaining sectors */
            max_sectors = 40 - (da & 077);
            if (wlen > max_sectors * 128)
                wlen = max_sectors * 128;

            if (wlen < 0) {
printf("rl read: wlen! %d\n", wlen);
                cs |= CS_E_OPI;
                cmd_done();
                break;
            }

            blockno = (da_cyl*80) + (da_hd*40) + da_sect;

#if 0
            printf("rl%d: da %o, offset %o, blockno %d, chs %d/%d/%d "
                   "wlen %d\n",
                   unit, da, ((da >> 6)*40 + da_sect)*256, blockno,
                   da_cyl, da_hd, da_sect, wlen);
#endif

            while (wlen > 0) {

                u_short *bufferp;

                swlen = wlen > 128 ? 128 : wlen;

                if (func == CS_FUNC_READ) {
printf("read; u%d, b%d, len %d => %o\n", unit, blockno, swlen, phys_addr);
                    read_disk_block256(unit, blockno, &bufferp);
                    unibus_dma_buffer(1, phys_addr, bufferp, swlen);
                }

                if (func == CS_FUNC_WRITECHK) {
printf("writechk; u%d, b%d, len %d => %o\n", unit, blockno, swlen, phys_addr);
                    read_disk_block256(unit, blockno, &bufferp);
                    unibus_dma_buffer(0, phys_addr, buffer2, swlen);
                    for (i = 0; i < swlen; i++) {
                        if (bufferp[i] != buffer2[i]) {
                            cs |= CS_ERR | CS_E_DCRC;
                        }
                    }
                }

                if (func == CS_FUNC_WRITE) {
printf("write; u%d, b%d, len %d => %o\n", unit, blockno, swlen, phys_addr);
                    unibus_dma_buffer(0, phys_addr, buffer2, swlen);
                    if (swlen < 128) {
                        int resid_bytes = (128-swlen)*2;
                        memset(((char *)buffer2) + swlen, 0, resid_bytes);
                    }
                    write_disk_block256(unit, blockno, buffer2);
                }

//printf("mp[0] %o + swlen %o = %o\n", mp[0], swlen, (mp[0] + swlen) & 0177777);

                mp[0] = (mp[0] + swlen) & 0177777;
                phys_addr += swlen*2;

                ba = phys_addr & 0177776;
                cs = (cs & ~CS_BA1617) | (((phys_addr >> 16) & 3) << 4);
                da++;

                blockno++;
                wlen -= swlen;
            }
            
            if (mp[0] != 0) {
printf("mp[0]! %o\n", mp[0]);
                cs |= CS_ERR | CS_E_OPI;
            }

//drive[ds10].curr_cyl = da >> 7;
//drive[ds10].curr_head = (da >> 6) & 1;

printf("rl done\n");
            cmd_done();
            break;
        }
    }

    if (seek_pending) {
        if (--seek_time == 0) {
            seek_pending = 0;

            drive[ds10].curr_head = drive[ds10].new_head;
            drive[ds10].curr_cyl = drive[ds10].new_cyl;

//printf("seek done; new h %d, c %d\n", drive[ds10].new_head, drive[ds10].new_cyl);
            cmd_done();
        }
    }

    if (int_pending) {
        int_pending = 0;
        //unibus_interrupt();
        cpu_int_set(3);
    }

    if (init_pending) {
        rl11_reset();
        init_pending = 0;
        rl11_drive_online();
    }
}


u16 io_rl_read(u32 addr)
{
    u16 data;

    printf("io_rl_read %o decode %o\n", addr, ((addr >> 1) & 07));

    rl11_poll();

    switch (addr & 07) {			/* decode PA<3:1> */
    case CS:
	update_cs();
	data = cs;
	break;
    case BA:
	data = ba & 0177776;
	break;
    case DA:
	data = da;
	break;
    case MP:
	data = mp[0];
	mp[0] = mp[1];
	mp[1] = mp[2];
	break;
    }

    return data;
}

void io_rl_write(u32 addr, u16 data, int writeb)
{
    printf("io_rl_write %o decode %o, data %o\n",
           addr, ((addr >> 1) & 07), data);

    rl11_poll();

    switch (addr & 07) {			/* decode PA<3:1> */
    case CS:
            /* honor byte writes */
            if (writeb)
                data = byte_place(addr, cs, data);

            cs = (cs & ~CS_RW) | (data & CS_RW);
            update_cs();

            /* write CRDY=1 */
            if (data & CS_CRDY) {
		if ((data & CS_IE) == 0) {
//                    int_pending = 0;
                } else
                    /* writing RDY & IE - if we were RDY, gen int */
		    if ((cs & (CS_CRDY | CS_IE)) == CS_CRDY) {
                        int_pending = 1;
                    }
                break;
            }

            /* wrote CRDY=0; clear errors */
            int_pending = 0;
            cs &= ~CS_ANY_ERR;
            cmd_pending = 1;
            break;

    case BA:
	if (writeb)
	    data = byte_place(addr, ba, data);
	ba = data & 0177776;
	break;
    case DA:
	if (writeb)
	    data = byte_place(addr, da, data);
	da = data;
	break;
    case MP:
	if (writeb)
	    data = byte_place(addr, mp[0], data);
	mp[0] = mp[1] = mp[2] = data;
	break;
    }

    rl11_poll();
}

void
io_rl_reset(const char *fn)
{
//    rkcs = CSR_DONE;

    if (rl_fd == 0) {
        rl_fd = open(fn, O_RDONLY);
    }

    rl11_reset();
}

static const u16 boot_rom[] = {
    0042114,                        /* "LD" */
    0012706, 02000,                 /* MOV #boot_start, SP */
    0012700, 0000000,               /* MOV #unit, R0 */
    0010003,                        /* MOV R0, R3 */
    0000303,                        /* SWAB R3 */
    0012701, 0174400,               /* MOV #RLCS, R1        ; csr */
    0012761, 0000013, 0000004,      /* MOV #13, 4(R1)       ; clr err */
    0052703, 0000004,               /* BIS #4, R3           ; unit+gstat */
    0010311,                        /* MOV R3, (R1)         ; issue cmd */
    0105711,                        /* TSTB (R1)            ; wait */
    0100376,                        /* BPL .-2 */
    0105003,                        /* CLRB R3 */
    0052703, 0000010,               /* BIS #10, R3          ; unit+rdhdr */
    0010311,                        /* MOV R3, (R1)         ; issue cmd */
    0105711,                        /* TSTB (R1)            ; wait */
    0100376,                        /* BPL .-2 */
    0016102, 0000006,               /* MOV 6(R1), R2        ; get hdr */
    0042702, 0000077,               /* BIC #77, R2          ; clr sector */
    0005202,                        /* INC R2               ; magic bit */
    0010261, 0000004,               /* MOV R2, 4(R1)        ; seek to 0 */
    0105003,                        /* CLRB R3 */
    0052703, 0000006,               /* BIS #6, R3           ; unit+seek */
    0010311,                        /* MOV R3, (R1)         ; issue cmd */
    0105711,                        /* TSTB (R1)            ; wait */
    0100376,                        /* BPL .-2 */
    0005061, 0000002,               /* CLR 2(R1)            ; clr ba */
    0005061, 0000004,               /* CLR 4(R1)            ; clr da */
    0012761, 0177000, 0000006,      /* MOV #-512., 6(R1)    ; set wc */
    0105003,                        /* CLRB R3 */
    0052703, 0000014,               /* BIS #14, R3          ; unit+read */
    0010311,                        /* MOV R3, (R1)         ; issue cmd */
    0105711,                        /* TSTB (R1)            ; wait */
    0100376,                        /* BPL .-2 */
    0042711, 0000377,               /* BIC #377, (R1) */
    0005002,                        /* CLR R2 */
    0005003,                        /* CLR R3 */
    0012704, 02020,                 /* MOV #START+20, R4 */
    0005005,                        /* CLR R5 */
    0005007                         /* CLR PC */
    };


void io_rl_bootrom(void)
{
    u32 addr;
    int i;

    addr = 02000;
    for (i = 0; i < sizeof(boot_rom)/2; i++) {
        raw_write_memory(addr, boot_rom[i]);
        addr += 2;
    }

    initial_pc = 02002;
}



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/

