/* */

#include <stdio.h>

#include "cpu.h"
#include "support.h"

#if 0
static u16 rkds;
static u16 rkcs;
static u16 rkda;
static u16 rkwc;
static u16 rkba;
static u16 rker;

int rk_write_prot;
int rk_func;
int rk_fd;

static unsigned short rkxb[256*20];

static int sect;
static int cyl;

static int rkintq;

#define CSR_GO		(1 << 0)
#define CSR_IE		(1 << 6)
#define CSR_DONE	(1 << 7)
#define CSR_BUSY	(1 << 11)

/* RKDS */

#define RKDS_SC		0000017	/* sector counter */
#define RKDS_ON_SC	0000020	/* on sector */
#define RKDS_WLK	0000040	/* write locked */
#define RKDS_RWS	0000100	/* rd/wr/seek ready */
#define RKDS_RDY	0000200	/* drive ready */
#define RKDS_SC_OK	0000400	/* SC valid */
#define RKDS_INC	0001000	/* seek incomplete */
#define RKDS_UNSAFE	0002000	/* unsafe */
#define RKDS_RK05	0004000	/* RK05 */
#define RKDS_PWR	0010000	/* power low */
#define RKDS_ID		0160000	/* drive ID */


#define RKER_WCE	0000001	/* write check */
#define RKER_CSE	0000002	/* checksum */
#define RKER_NXS	0000040	/* nx sector */
#define RKER_NXC	0000100	/* nx cylinder */
#define RKER_NXD	0000200	/* nx drive */
#define RKER_TE		0000400	/* timing error */
#define RKER_DLT	0001000	/* data late */
#define RKER_NXM	0002000	/* nx memory */
#define RKER_PGE	0004000	/* programming error */
#define RKER_SKE	0010000	/* seek error */
#define RKER_WLK	0020000	/* write lock */
#define RKER_OVR	0040000	/* overrun */
#define RKER_DRE	0100000	/* drive error */

#define RKER_SOFT	(RKER_WCE+RKER_CSE)		/* soft errors */
#define RKER_HARD	0177740				/* hard errors */

#define	 RKCS_CTLRESET	0
#define	 RKCS_WRITE	1
#define	 RKCS_READ	2
#define	 RKCS_WCHK	3
#define	 RKCS_SEEK	4
#define	 RKCS_RCHK	5
#define	 RKCS_DRVRESET	6
#define	 RKCS_WLK	7
#define RKCS_MEX	0000060	/* memory extension */
#define RKCS_SSE	0000400	/* stop on soft err */
#define RKCS_FMT	0002000	/* format */
#define RKCS_INH	0004000	/* inhibit increment */
#define RKCS_SCP	0020000	/* search complete */
#define RKCS_HERR	0040000	/* hard error */
#define RKCS_ERR	0100000	/* error */
#define RKCS_RW		0006576	/* read/write */


u16 io_rk_read(u22 addr)
{
    printf("io_rk_read %o decode %o\n", addr, ((addr >> 1) & 07));

    switch ((addr >> 1) & 07) {			/* decode PA<3:1> */

    case 0:						/* RKDS: read only */
        rkds = (rkds & RKDS_ID) | RKDS_RK05 | RKDS_SC_OK;
#if 0
        /* selected unit */
        uptr = rk_dev.units + GET_DRIVE (rkda);
        if (uptr->flags & UNIT_ATT)
            rkds = rkds | RKDS_RDY;	/* attached? */
        if (!sim_is_active (uptr))
            rkds = rkds | RKDS_RWS;	/* idle? */
        if (uptr->flags & UNIT_WPRT)
            rkds = rkds | RKDS_WLK;
        if (GET_SECT (rkda) == (rkds & RKDS_SC))
            rkds = rkds | RKDS_ON_SC;
#endif
        return rkds;

    case 1:						/* RKER: read only */
        return rker;

    case 2:						/* RKCS */
//		rkcs &= RKCS_REAL;
        if (rker) rkcs |= RKCS_ERR;
        if (rker & RKER_HARD) rkcs |= RKCS_HERR;
        return rkcs;

    case 3:						/* RKWC */
        return rkwc;

    case 4:						/* RKBA */
        return rkba;

    case 5:						/* RKDA */
        return rkda;

    default:
        return 0;
    }
}

static void rk_set_done(int error)
{
    if (1) printf("rk: done; error %o\n", error);

    rkcs |= CSR_DONE;
    if (error != 0) {
        rker |= error;
        if (rker)
            rkcs |= RKCS_ERR;
        if (rker & RKER_HARD)
            rkcs |= RKCS_HERR;
    }

    if (rkcs & CSR_IE) {
        rkintq |= 1;
        cpu_int_set(3);
    } else {
        rkintq = 0;
        cpu_int_clear(3);
    }
}

static void rk_clr_done(void)
{
    if (1) printf("rk: not done\n");

    rkcs &= ~CSR_DONE;
    rkintq &= ~1;
    cpu_int_clear(3);
}

void rk_service(void)
{
    int i, drv, err, awc, wc, cma, cda, t;
    int da, cyl, track, sector;
    unsigned int ma;
    unsigned short comp;

    printf("rk_service; func %o\n", rk_func);

    if (rk_func == RKCS_SEEK) {
        rkcs |= RKCS_SCP;
        if (rkcs & CSR_IE) {
            rkintq |= 2;
            if (rkcs & CSR_DONE)
                cpu_int_set(3);
        } else {
            rkintq = 0;
            cpu_int_clear(3);
        }

        return;
    }

    cyl = (rkda >> 5) & 0377;
    track = (rkda >> 4) & 0777;
    sector = rkda & 017;

    ma = ((rkcs & RKCS_MEX) << (16 - 4)) | rkba;

    if (sect >= 12) {
        rk_set_done(RKER_NXS);
        return;
    }

    if (cyl >= 203) {
        rk_set_done(RKER_NXC);
        return;
    }

    da = ((track * 12) + sector) * 256;
    wc = 0200000 - rkwc;

//    if ((da + wc) > (int) uptr->capac) {
//        wc = uptr->capac - da;
//        rker |= RKER_OVR;
//    }

    printf("rk: seek %d\n", da * sizeof(short));
    err = lseek(rk_fd, da * sizeof(short), SEEK_SET);
    if (wc && (err >= 0)) {
        err = 0;

        switch (rk_func) {

        case RKCS_READ:
            if (rkcs & RKCS_FMT) {
                for (i = 0, cda = da; i < wc; i++) {
//                    if (cda >= (int) uptr->capac) {       /* overrun? */
//                        rker |= RKER_OVR;
//                        wc = i;
//                        break;
//                    }
                    rkxb[i] = (cda / 256) / (2 * 12);
                    cda = cda + 256;
                }
            } else {
printf("rk: read() wc %d\n", wc);
                i = read(rk_fd, rkxb, sizeof(short)*wc);
printf("rk: read() ret %d\n", i);
                if (i >= 0 && i < sizeof(short)*wc) {
                    i /= 2;
                    for (; i < wc; i++)
                        rkxb[i] = 0;
                }
            }

            if (rkcs & RKCS_INH) {
                raw_write_memory(ma, rkxb[wc - 1]);
            } else {
int oldma = ma;
printf("rk: read(), dma wc=%d, ma=%o\n", wc, ma);
printf("rk: buffer %06o %06o %06o %06o\n",
       rkxb[0], rkxb[1], rkxb[2], rkxb[3]);
                for (i = 0; i < wc; i++) {
                    raw_write_memory(ma, rkxb[i]);
                    ma += 2;
                }
show(oldma);
            }
            break;

        case RKCS_WRITE:
            if (rkcs & RKCS_INH) {
                comp = raw_read_memory(ma);
                for (i = 0; i < wc; i++)
                    rkxb[i] = comp;
            } else {
                for (i = 0; i < wc; i++) {
                    rkxb[i] = raw_read_memory(ma);
                    ma += 2;
                }
            }

            awc = (wc + (256 - 1)) & ~(256 - 1);
printf("rk: write()\n");
            write(rk_fd, rkxb, awc*2);
            break;

        case RKCS_WCHK:
            i = read(rk_fd, rkxb, sizeof(short)*wc);
            if (i < 0) {
                wc = 0;
                break;
            }

            if (i >= 0 && i < sizeof(short)*wc) {
                i /= 2;
                for (; i < wc; i++)
                    rkxb[i] = 0;
            }

            awc = wc;
            for (wc = 0, cma = ma; wc < awc; wc++)  {
                comp = raw_read_memory(cma);
                if (comp != rkxb[wc])  {
                    rker |= rker;
                    if (rkcs & RKCS_SSE)
                        break;
                }
                if (!(rkcs & RKCS_INH))
                    cma += 2;
            }
            break;

        default:
            break;
        }
    }

    rkwc = (rkwc + wc) & 0177777;
    if (!(rkcs & RKCS_INH))
        ma = ma + (wc << 1);

    rkba = ma & 0xffff;
    rkcs = (rkcs & ~RKCS_MEX) | ((ma >> (16 - 4)) & RKCS_MEX);

    if ((rk_func == RKCS_READ) && (rkcs & RKCS_FMT))
        da = da + (wc * 256);
    else
        da = da + wc + (256 - 1);

    track = (da / 256) / 12;
    sect = (da / 256) % 12;

    rkda = (track << 4) | sect;
    rk_set_done(0);

    if (err != 0) {
        printf("RK I/O error\n");
    }
}

static void rk_go(void)
{
    printf("rk_go!\n");

    rk_func = (rkcs >> 1) & 7;
    if (rk_func == RKCS_CTLRESET) {
        rker = 0;
        rkda = 0;
        rkba = 0;
        rkcs = CSR_DONE;
        rkintq = 0;
        cpu_int_clear(3);
        return;
    }

    rker &= ~RKER_SOFT;
    if (rker == 0)
        rkcs &= ~RKCS_ERR;

    rkcs &= ~RKCS_SCP;
    rk_clr_done();

    if ((rkcs & RKCS_FMT) &&
        (rk_func != RKCS_READ) && (rk_func != RKCS_WRITE)) {
	rk_set_done(RKER_PGE);
	return;
    }

    if ((rk_func == RKCS_WRITE) && rk_write_prot) {
        rk_set_done(RKER_WLK);
        return;
    }

    if (rk_func == RKCS_WLK) {
        rk_set_done(0);
        return;
    }

    if (rk_func == RKCS_DRVRESET) {
        cyl = 0;
        sect = 0;
        rk_func = RKCS_SEEK;
    } else {
        sect = rkda & 017;
        cyl = (rkda >> 5) & 0377;
    }

    if (sect >= 12) {
        rk_set_done(RKER_NXS);
        return;
    }

    if (cyl >= 203) {
        rk_set_done(RKER_NXC);
        return;
    }

    if (rk_func == RKCS_SEEK) {
        rk_set_done(0);
    }

    rk_service();
}

void io_rk_write(u22 addr, u16 data, int writeb)
{
    printf("io_rk_write %o decode %o\n", addr, ((addr >> 1) & 07));

    switch ((addr >> 1) & 07) {			/* decode PA<3:1> */

    case 2:						/* RKCS */
        printf("rk: rkcs <- %o\n", data);
        if (writeb) {
            data = (addr & 1)? (rkcs & 0377) |
                (data << 8): (rkcs & ~0377) | data;
        }
        if ((data & CSR_IE) == 0) {		/* int disable? */
            rkintq = 0;			/* clr int queue */
            cpu_int_clear(3);
        }
        else if ((rkcs & (CSR_DONE + CSR_IE)) == CSR_DONE) {
            rkintq |= 1;
            cpu_int_set(3);
        }
        rkcs = (rkcs & ~RKCS_RW) | (data & RKCS_RW);
        if ((rkcs & CSR_DONE) && (data & CSR_GO))
            rk_go ();
        return;
		
    case 3:						/* RKWC */
        if (writeb)  {
            data = (addr & 1) ?
                (rkwc & 0377) | (data << 8) :
                (rkwc & ~0377) | data;
        }
        rkwc = data;
        printf("rk: rkwc <- %o\n", rkwc);
        return;

    case 4:						/* RKBA */
        if (writeb) {
            data = (addr & 1)?
                (rkba & 0377) | (data << 8) :
                (rkba & ~0377) | data;
        }
        rkba = data;
        printf("rk: rkba <- %o\n", rkba);
        return;

    case 5:						/* RKDA */
        if ((rkcs & CSR_DONE) == 0)
            return;
        if (writeb) {
            data = (addr & 1) ?
                (rkda & 0377) | (data << 8) :
                (rkda & ~0377) | data;
        }
        rkda = data;
        printf("rk: rkda <- %o\n", rkda);
        return;

    default:
        printf("rk: ??\n");
        return;
    }
}

#include <fcntl.h>

void
io_rk_reset(void)
{
    rkcs = CSR_DONE;

    rk_fd = open("rk.dsk", O_RDONLY);
}
#endif



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/

