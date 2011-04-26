/*
 * private_rk.c
 *
 * minimal implementation of rk disk controller,
 * designed to be shared by the rtl simulation and simh
 * so they both operate the same way
 * (to facilitate comparisons of cpu flow)
 */

#include <stdio.h>
#include <unistd.h>

#include "pdp11_defs.h"

UNIT rk_unit[];
#define LOCAL_IO

//typedef int int32;
typedef unsigned short u16;
typedef unsigned int u22;

struct rk_context_s {
    u16 rkds;
    u16 rkcs;
    u16 rkda;
    u16 rkwc;
    u16 rkba;
    u16 rker;

    int rk_write_prot;
    int rk_func;
    int rk_fd;

    int track;
    int sect;
    int cyl;
    int rkintq;

    int has_init;

    u16 rkxb[256*256];
};

#ifndef CSR_GO
#define CSR_GO		(1 << 0)
#define CSR_IE		(1 << 6)
#define CSR_DONE	(1 << 7)
#define CSR_BUSY	(1 << 11)
#endif

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

struct rk_context_s rk_context[2];
static int io_rk_debug = 0;

void io_rk_reset(void);

/* ----------------- */

extern void cpu_int_set(int n);
extern void cpu_int_clear(int n);
extern void raw_write_memory(int, u16);
extern u16 raw_read_memory(int);

void io_rk_cpu_int_set(struct rk_context_s *rk)
{
    extern int int_req[];

    printf("io_rk_cpu_int_set() %s\n",
           rk == &rk_context[1] ? "rtl" :
           rk == &rk_context[0] ? "simh" :
           "unknown");

    if (rk == &rk_context[1])
        cpu_int_set(3);
    if (rk == &rk_context[0])
        SET_INT(RK);
}

void io_rk_cpu_int_clear(struct rk_context_s *rk)
{
    extern int int_req[];

    printf("io_rk_cpu_int_clear() %s\n",
           rk == &rk_context[1] ? "rtl" :
           rk == &rk_context[0] ? "simh" :
           "unknown");

    if (rk == &rk_context[1])
        cpu_int_clear(3);
    if (rk == &rk_context[0])
        CLR_INT(RK);
}

void rk_raw_write_memory(struct rk_context_s *rk, int ma, u16 data)
{
    extern u16 *M;

    if (rk == &rk_context[1])
        raw_write_memory(ma, data);
    if (rk == &rk_context[0])
        M[ma >> 1] = data;;
}

u16 rk_raw_read_memory(struct rk_context_s *rk, int ma)
{
    extern u16 *M;

    if (rk == &rk_context[1])
        return raw_read_memory(ma);
    if (rk == &rk_context[0])
        return M[ma >> 1];
}

/* ----------------- */

u16 _io_rk_read(struct rk_context_s *rk, u22 addr)
{
    if (io_rk_debug) printf("io_rk_read %o decode %o\n", addr, ((addr >> 1) & 07));

    if (!rk->has_init) {
        io_rk_reset();
    }

    switch ((addr >> 1) & 07) {			/* decode PA<3:1> */

    case 0:						/* RKDS: read only */
        rk->rkds = (rk->rkds & RKDS_ID) | RKDS_RK05 | RKDS_SC_OK;
        return rk->rkds;

    case 1:						/* RKER: read only */
        return rk->rker;

    case 2:						/* RKCS */
        if (rk->rker) rk->rkcs |= RKCS_ERR;
        if (rk->rker & RKER_HARD) rk->rkcs |= RKCS_HERR;
        return rk->rkcs;

    case 3:						/* RKWC */
        return rk->rkwc;

    case 4:						/* RKBA */
        return rk->rkba;

    case 5:						/* RKDA */
        return rk->rkda;

    default:
        return 0;
    }
}

static void rk_set_done(struct rk_context_s *rk, int error)
{
    if (1) printf("rk: done; error %o\n", error);

    rk->rkcs |= CSR_DONE;
    if (error != 0) {
        rk->rker |= error;
        if (rk->rker)
            rk->rkcs |= RKCS_ERR;
        if (rk->rker & RKER_HARD)
            rk->rkcs |= RKCS_HERR;
    }

    if (rk->rkcs & CSR_IE) {
        rk->rkintq |= 1;
        io_rk_cpu_int_set(rk);
    } else {
        rk->rkintq = 0;
        io_rk_cpu_int_clear(rk);
    }
}

static void rk_clr_done(struct rk_context_s *rk)
{
    if (1) printf("rk: not done\n");

    rk->rkcs &= ~CSR_DONE;
    rk->rkintq &= ~1;
    io_rk_cpu_int_clear(rk);
}

void rk_service(struct rk_context_s *rk)
{
    int i, drv, err, awc, wc, cma, cda, t;
    int da, cyl, track, sector, ret;
    unsigned int ma;
    unsigned short comp;

    printf("rk_service; func %o\n", rk->rk_func);

    if (rk->rk_func == RKCS_SEEK) {
        rk->rkcs |= RKCS_SCP;
        if (rk->rkcs & CSR_IE) {
            rk->rkintq |= 2;
            if (rk->rkcs & CSR_DONE)
                io_rk_cpu_int_set(rk);
        } else {
            rk->rkintq = 0;
            io_rk_cpu_int_clear(rk);
        }

        return;
    }

    cyl = (rk->rkda >> 5) & 0377;
    track = (rk->rkda >> 4) & 0777;
    sector = rk->rkda & 017;

    ma = ((rk->rkcs & RKCS_MEX) << (16 - 4)) | rk->rkba;

    if (sector >= 12) {
        rk_set_done(rk, RKER_NXS);
        return;
    }

    if (cyl >= 203) {
        rk_set_done(rk, RKER_NXC);
        return;
    }

    da = ((track * 12) + sector) * 256;
    wc = 0200000 - rk->rkwc;

//    if ((da + wc) > (int) uptr->capac) {
//        wc = uptr->capac - da;
//        rker |= RKER_OVR;
//    }

    printf("rk: seek %s %d (0x%x)\n",
           rk == &rk_context[1] ? "rtl" :
           rk == &rk_context[0] ? "simh" :
           "unknown", da * sizeof(short), da * sizeof(short));

#ifdef LOCAL_IO
    err = lseek(rk->rk_fd, da * sizeof(short), SEEK_SET);
#else
    err = fseek (rk_unit[0].fileref, da * sizeof (int16), SEEK_SET);
#endif

    if (wc && (err >= 0)) {
        err = 0;

        switch (rk->rk_func) {

        case RKCS_READ:
            if (rk->rkcs & RKCS_FMT) {
                for (i = 0, cda = da; i < wc; i++) {
//                    if (cda >= (int) uptr->capac) {       /* overrun? */
//                        rker |= RKER_OVR;
//                        wc = i;
//                        break;
//                    }
                    rk->rkxb[i] = (cda / 256) / (2 * 12);
                    cda = cda + 256;
                }
            } else {
printf("rk: read() wc %d\n", wc);
#ifdef LOCAL_IO
                i = read(rk->rk_fd, rk->rkxb, sizeof(short)*wc);
#else
	        i = fxread (rk->rkxb, sizeof (int16), wc, rk_unit[0].fileref);
#endif
printf("rk: read() ret %d\n", i);
                if (i >= 0 && i < sizeof(short)*wc) {
                    i /= 2;
                    for (; i < wc; i++)
                        rk->rkxb[i] = 0;
                }
            }

            if (rk->rkcs & RKCS_INH) {
                rk_raw_write_memory(rk, ma, rk->rkxb[wc - 1]);
            } else {
int oldma = ma;
printf("rk: read() dma wc=%d, ma=%o\n", wc, ma);
printf("rk: buffer %06o %06o %06o %06o\n",
       rk->rkxb[0], rk->rkxb[1], rk->rkxb[2], rk->rkxb[3]);
                for (i = 0; i < wc; i++) {
                    rk_raw_write_memory(rk, ma, rk->rkxb[i]);
                    ma += 2;
                }
//show(oldma);
            }
            break;

        case RKCS_WRITE:
            if (rk->rkcs & RKCS_INH) {
                comp = rk_raw_read_memory(rk, ma);
                for (i = 0; i < wc; i++)
                    rk->rkxb[i] = comp;
            } else {
                for (i = 0; i < wc; i++) {
                    rk->rkxb[i] = rk_raw_read_memory(rk, ma);
                    ma += 2;
                }
            }

            awc = (wc + (256 - 1)) & ~(256 - 1);
printf("rk: write()\n");
#ifdef LOCAL_IO
	    ret = write(rk->rk_fd, rk->rkxb, awc*2);
#else
	    ret = fxwrite (rk->rkxb, sizeof (int16), awc, rk_unit[0].fileref);
#endif
            break;

        case RKCS_WCHK:
#ifdef LOCAL_IO
            i = read(rk->rk_fd, rk->rkxb, sizeof(short)*wc);
#else
	    i = fxread (rk->rkxb, sizeof (int16), wc, rk_unit[0].fileref);
#endif
            if (i < 0) {
                wc = 0;
                break;
            }

            if (i >= 0 && i < sizeof(short)*wc) {
                i /= 2;
                for (; i < wc; i++)
                    rk->rkxb[i] = 0;
            }

            awc = wc;
            for (wc = 0, cma = ma; wc < awc; wc++)  {
                comp = rk_raw_read_memory(rk, cma);
                if (comp != rk->rkxb[wc])  {
                    rk->rker |= rk->rker;
                    if (rk->rkcs & RKCS_SSE)
                        break;
                }
                if (!(rk->rkcs & RKCS_INH))
                    cma += 2;
            }
            break;

        default:
            break;
        }
    }

    rk->rkwc = (rk->rkwc + wc) & 0177777;
    if (!(rk->rkcs & RKCS_INH))
        ma = ma + (wc << 1);

    rk->rkba = ma & 0xffff;
    rk->rkcs = (rk->rkcs & ~RKCS_MEX) | ((ma >> (16 - 4)) & RKCS_MEX);

    if ((rk->rk_func == RKCS_READ) && (rk->rkcs & RKCS_FMT))
        da = da + (wc * 256);
    else
        da = da + wc + (256 - 1);

    rk->track = (da / 256) / 12;
    rk->sect = (da / 256) % 12;

    rk->rkda = (rk->track << 4) | rk->sect;
    rk_set_done(rk, 0);

    if (err != 0) {
        printf("RK I/O error\n");
    }
}

static void rk_go(struct rk_context_s *rk)
{
    printf("rk_go!\n");

    rk->rk_func = (rk->rkcs >> 1) & 7;
    if (rk->rk_func == RKCS_CTLRESET) {
        rk->rker = 0;
        rk->rkda = 0;
        rk->rkba = 0;
        rk->rkcs = CSR_DONE;
        rk->rkintq = 0;
        io_rk_cpu_int_clear(rk);
        return;
    }

    rk->rker &= ~RKER_SOFT;
    if (rk->rker == 0)
        rk->rkcs &= ~RKCS_ERR;

    rk->rkcs &= ~RKCS_SCP;
    rk_clr_done(rk);

    if ((rk->rkcs & RKCS_FMT) &&
        (rk->rk_func != RKCS_READ) && (rk->rk_func != RKCS_WRITE)) {
	rk_set_done(rk, RKER_PGE);
	return;
    }

    if ((rk->rk_func == RKCS_WRITE) && rk->rk_write_prot) {
        rk_set_done(rk, RKER_WLK);
        return;
    }

    if (rk->rk_func == RKCS_WLK) {
        rk_set_done(rk, 0);
        return;
    }

    if (rk->rk_func == RKCS_DRVRESET) {
        rk->cyl = 0;
        rk->sect = 0;
        rk->rk_func = RKCS_SEEK;
    } else {
        rk->sect = rk->rkda & 017;
        rk->cyl = (rk->rkda >> 5) & 0377;
    }

    if (rk->sect >= 12) {
        rk_set_done(rk, RKER_NXS);
        return;
    }

    if (rk->cyl >= 203) {
        rk_set_done(rk, RKER_NXC);
        return;
    }

    if (rk->rk_func == RKCS_SEEK) {
        rk_set_done(rk, 0);
    }

    rk_service(rk);
}

void _io_rk_write(struct rk_context_s *rk, u22 addr, u16 data, int writeb)
{
    if (0) printf("_io_rk_write %o %d decode %o\n",
                  addr, writeb, ((addr >> 1) & 07));

    if (!rk->has_init) {
        io_rk_reset();
    }

    switch ((addr >> 1) & 07) {			/* decode PA<3:1> */

    case 2:						/* RKCS */
        printf("rk: rkcs <- %o\n", data);
        if (writeb) {
            data = (addr & 1)? (rk->rkcs & 0377) |
                (data << 8): (rk->rkcs & ~0377) | data;
        }
        if ((data & CSR_IE) == 0) {		/* int disable? */
            rk->rkintq = 0;			/* clr int queue */
            io_rk_cpu_int_clear(rk);
        } else 
            if ((rk->rkcs & (CSR_DONE | CSR_IE)) == CSR_DONE) {
                rk->rkintq |= 1;
                io_rk_cpu_int_set(rk);
            }

        rk->rkcs = (rk->rkcs & ~RKCS_RW) | (data & RKCS_RW);
        printf("rk: rkcs %o\n", data);

        if ((rk->rkcs & CSR_DONE) && (data & CSR_GO))
            rk_go(rk);
        return;
		
    case 3:						/* RKWC */
        if (writeb)  {
            data = (addr & 1) ?
                (rk->rkwc & 0377) | (data << 8) :
                (rk->rkwc & ~0377) | data;
        }
        rk->rkwc = data;
        printf("rk: rkwc <- %o\n", rk->rkwc);
        return;

    case 4:						/* RKBA */
        if (writeb) {
            data = (addr & 1)?
                (rk->rkba & 0377) | (data << 8) :
                (rk->rkba & ~0377) | data;
        }
        rk->rkba = data;
        printf("rk: rkba <- %o\n", rk->rkba);
        return;

    case 5:						/* RKDA */
        if ((rk->rkcs & CSR_DONE) == 0)
            return;
        if (writeb) {
            data = (addr & 1) ?
                (rk->rkda & 0377) | (data << 8) :
                (rk->rkda & ~0377) | data;
        }
        rk->rkda = data;
        printf("rk: rkda <- %o\n", rk->rkda);
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
    rk_context[0].rkcs = CSR_DONE;
    rk_context[1].rkcs = CSR_DONE;

    rk_context[0].rk_fd = open("rk.dsk", O_RDONLY);
    rk_context[1].rk_fd = rk_context[0].rk_fd;

    rk_context[0].has_init = 1;
    rk_context[1].has_init = 1;
}




/* ------------------------------------------------------ */

/* simh */
int private_rk_rd(int32 *data, int32 addr, int32 access)
{
	int byte;
	byte = access == 4 ? 1 : 0;
        *data = _io_rk_read(&rk_context[0], addr);
	return 0;
}

int private_rk_wr(int32 data, int32 addr, int32 access)
{
	int byte;
	byte = access == 4 ? 1 : 0;
	_io_rk_write(&rk_context[0], addr, data, byte);
	return 0;
}

int private_rk_inta(void)
{
    struct rk_context_s *rk = &rk_context[0];
    int i;
    extern int int_req[];


    for (i = 0; i < 2; i++) {
        if (rk->rkintq & (1 << i)) {
            rk->rkintq = rk->rkintq & ~(1 << i);
            if (rk->rkintq)
                SET_INT(RK);
            printf("rk: inta 0220\n");
            return 0220;
        }
    }

    rk->rkintq = 0;

    printf("rk: inta 0\n");
    return 0;
}

/* rtl */
u16 io_rk_read(u22 addr)
{
    return _io_rk_read(&rk_context[1], addr);
}

void io_rk_write(u22 addr, u16 data, int writeb)
{
    _io_rk_write(&rk_context[1], addr, data, writeb);
}



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/

