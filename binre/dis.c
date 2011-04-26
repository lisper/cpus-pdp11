/*
 * dis.c
 * a very simple pdp-11 disassembler
 * with some local label discovery
 * 
 * brad@heeltoe.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

typedef unsigned short u16;
typedef unsigned char u8;

#include "isn.h"

u8 *code;
int code_len;

int dist_threshold;

extern raw_isn_t raw_isns[];
extern raw_isn_t *isn_decode[0x10000];

struct ref_s {
    struct ref_s *rnext;
    int rtype;
    int rval;
};

struct label_s {
    int lrefs;
    int lspan_min;
    int lspan_max;
    int llabel;
    int llocal;
};

struct ref_s **refs;
struct label_s *labels;

int
init_refs(void)
{
    int i;

    refs = (struct ref_s **)malloc(code_len * sizeof(struct ref_s));

    labels = (struct label_s *)malloc(code_len * sizeof(struct label_s));

    for (i = 0; i < code_len; i++) {
        refs[i] = NULL;
        labels[i].llabel = 0;
        labels[i].llocal = 0;
    }
}

int
make_ref(int addr, int rt, int rv)
{
    struct ref_s *r;

    r = (struct ref_s *)malloc(sizeof(struct ref_s));
    if (r == NULL)
        return -1;

    r->rnext = refs[addr];
    r->rtype = rt;
    r->rval = rv;
    refs[addr] = r;

    return 0;
}

int
add_ref_at(int inst_addr, int ref_to)
{
    struct ref_s *r;

    make_ref(inst_addr, 1, ref_to);
    make_ref(ref_to, 2, inst_addr);

    return 0;
}

void
dump_refs(void)
{
    int o;

    for (o = 0; o < code_len; o++) {
        struct ref_s *r = refs[o];
        struct label_s *l = &labels[o];

        if (r) {
            printf("%06o: ", o);
            if (l->llabel)
                printf("L%d", l->llabel);
            printf("\n");
        }

        while (r) {
            printf(" %d %06o\n", r->rtype, r->rval);
            r = r->rnext;
        }
    }
}

void
scan_refs(void)
{
    int o, active;

    /* count refs and find spans */
    for (o = 0; o < code_len; o++) {
        struct ref_s *r = refs[o];
        int min, max;

        labels[o].lrefs = 0;

        if (r && r->rtype == 2) {
            min = 1024*1024;
            max = 0;

            while (r) {
                labels[o].lrefs++;

                if (r->rval < min)
                    min = r->rval;
                if (r->rval > max)
                    max = r->rval;

                r = r->rnext;
            }

            /* include label in span */
            if (o < min)
                min = o;
            if (o > max)
                max = o;

            labels[o].lspan_min = min;
            labels[o].lspan_max = max;

            if (0) printf("%06o: span %o - %o\n", o, 
                          labels[o].lspan_min, labels[o].lspan_max);
        }
    }

    /* find local-only references */
    for (o = 0; o < code_len; o++) {
        struct ref_s *r = refs[o];

        if (r && r->rtype == 2) {
            labels[o].llocal = 1;

            while (r) {
                int dist;

                dist = r->rval - o;
                if (dist < 0) dist = -dist;
                if (r->rtype == 2 && dist > dist_threshold) {
                    labels[o].llocal = 0;
                    labels[o].llabel = o;
                }

                r = r->rnext;
            }
        }
    }

    /* assign */
    active = 0;
    for (o = 0; o < code_len; o++) {
        struct ref_s *r = refs[o];
        struct label_s *l = &labels[o];
        struct label_s *lref;

        /* if we hit a forward reference to a local label */
        if (r && r->rtype == 1) {
            lref = &labels[ r->rval ];

            /* if not a local label, ignore it */
            if (lref->llocal == 0)
                continue;

            /* start of span? */
            if (lref->lspan_min == o) {
                active++;
                lref->llabel = active;
                //need_label(lref);
            }

            /* end of span? */
            if (lref->lspan_max == o) {
                //retire_label(lref);
                active--;
            }
        }

        /* if we hit a label */
        if (l->llocal) {
            if (0) printf("local %o; %d (%o-%o)\n",
                          o, active, l->lspan_min, l->lspan_max);

            /* start of span? */
            if (l->lspan_min == o) {
                active++;
                l->llabel = active;
                //need_label(l);
            }

            /* end of span? */
            if (l->lspan_max == o) {
                //retire_label(l);
                active--;
            }
        }
    }
}

int
has_refs_from(int addr)
{
    struct ref_s *r = refs[addr];

    while (r) {
        if (r->rtype == 2)
            return 1;
        r = r->rnext;
    }

    return 0;
}

int
has_refs_to(int addr)
{
    struct ref_s *r = refs[addr];

    while (r) {
        if (r->rtype == 1)
            return 1;
        r = r->rnext;
    }

    return 0;
}

int
has_refs_from_fwd(int addr)
{
    struct ref_s *r = refs[addr];

    while (r) {
        if (r->rtype == 2 && r->rval > addr)
            return 1;
        r = r->rnext;
    }

    return 0;
}

int
fmt_reg_mode(char *buf, int m, int r, int arg)
{
    char rn[10];

    if (r < 6)
        sprintf(rn, "r%d", r);
    else
        if (r == 6)
            strcpy(rn, "sp");
        else
            strcpy(rn, "pc");

    switch (m & 7) {
    case M_REG:
        strcpy(buf, rn);
        break;
    case M_REG_DEF:
        sprintf(buf, "@%s", rn);
        //sprintf(buf, "(%s)", rn);
        break;
    case M_AUTOINC:
//        if (r < 7)
            sprintf(buf, "(%s)+", rn);
//        else
//            sprintf(buf, "0%o", arg);
        break;
    case M_AUTOINC_DEF:
        sprintf(buf, "@(%s)+", rn);
        break;
    case M_AUTODEC:
        sprintf(buf, "-(%s)", rn);
        break;
    case M_AUTODEC_DEF:
        sprintf(buf, "@-(%s)", rn);
    case M_INDEXED:
//        if (r < 7)
            sprintf(buf, "%d.(%s)", arg, rn);
//        else
//            sprintf(buf, "0%o", arg);
        break;
    case M_INDEX_DEF:
        sprintf(buf, "@%d.(%s)", arg, rn);
        break;
    }
}


int
scan_code(int pass)
{
    int hdr_len, o;
    u8 *start;
    u16 magic, inst;

    printf("pass %d\n", pass);

    if (pass == 1) {
        init_refs();
    }

    if (pass == 2) {
        scan_refs();
        if (0) dump_refs();
    }

    magic = *(u16 *)(code + 04000);
    switch (magic) {
    case 0405:
        hdr_len = 12 + 04000;
        break;
    case 0407:
        hdr_len = 14 + 04000;
        break;
    }

    start = code + hdr_len;

    for (o = hdr_len; o < code_len; o += 2) {

        raw_isn_t *ri;
        char sm, sr, dm, dr, rr, hasarg1, hasarg2;
        u16 arg1, arg2;
        int oi, off, ref;

        inst = *(u16 *)(code + o);
        oi = o;
        ri = isn_decode[inst];
        hasarg1 = 0;
        hasarg2 = 0;

        if (ri) {

            sm = -1;
            dm = -1;
            sr = -1;
            dr = -1;
            ref = -1;
            arg1 = 0;
            arg2 = 0;

            switch (ri->isn_regs) {
            case R_NONE:
                break;
            case R_DD:
                dm = (inst >> 3) & 07; /* 00070 */
                dr = (inst >> 0) & 07; /* 00007 */
                break;
            case R_SS:
                sm = (inst >> 3) & 07; /* 00070 */
                sr = (inst >> 0) & 07; /* 00007 */
                break;
            case R_SSDD:
                sm = (inst >> 9) & 07; /* 07000 */
                sr = (inst >> 6) & 07; /* 00700 */
                dm = (inst >> 3) & 07; /* 00070 */
                dr = (inst >> 0) & 07; /* 00007 */
                break;
            case R_R:
                break;
            case R_RSS:
                rr = (inst >> 6) & 07; /* 00700 */
                sm = (inst >> 3) & 07; /* 00070 */
                sr = (inst >> 0) & 07; /* 00007 */
                break;
            case R_RDD:
                rr = (inst >> 6) & 07; /* 00700 */
                dm = (inst >> 3) & 07; /* 00070 */
                dr = (inst >> 0) & 07; /* 00007 */
                break;
            case R_N:
            case R_NN:
                break;
            case R_8OFF:
                off = (signed char)(inst & 0377) * 2;
                ref = o + off + 2;
                if (pass == 1)
                    add_ref_at(oi, ref);
                break;
            case R_R8OFF:
                break;
            }

            if ((sm == 2 || sm == 3 || sm == 6 || sm == 7) && sr == 7) {
                o += 2;
                arg1 = *(u16 *)(code + o);
                hasarg1 = 1;
            }

            if ((dm == 2 || dm == 3 || dm == 6 || dm == 7) && dr == 7) {
                o += 2;
                arg2 = *(u16 *)(code + o);
                hasarg2 = 1;
            }

            if (pass <= 2)
                continue;

            if (labels[oi].llabel) {
                if (labels[oi].llocal)
                    printf("%d:\n", labels[oi].llabel);
                else
                    printf("L%d:\n", labels[oi].llabel);
            }

            if (has_refs_from(oi))
                printf("-> ");
            else
                printf("   ");

            printf("%06o %06o %s", oi, inst, ri->isn_name);

            if (sr >= 0) {
                char b[64];
                fmt_reg_mode(b, sm, sr, arg1);
                printf(" %s", b);
            }

            if (dr >= 0) {
                char b[64];
                fmt_reg_mode(b, dm, dr, arg2);
                if (sr >= 0)
                    printf(",");

                printf(" %s", b);
            }

            if (ref >= 0) {
                if (labels[ref].llocal) {
                    printf(" ; (ref %o, %d%c)",
                           ref,
                           labels[ref].llabel,
                           ref > o ? 'f' : 'b'
                        );
                } else {
                    printf(" ; (ref %o, L%d)",
                           ref,
                           labels[ref].llabel);
                }
            }

            printf("\n");

            if (hasarg1) {
                printf("          %06o", arg1);
                printf("\n");
            }

            if (hasarg2) {
                printf("          %06o", arg2);
                printf("\n");
            }
        } else {
            if (pass <= 2)
                continue;

            printf("   %06o %06o ?\n", o, inst);
        }
    }

    return 0;
}

int
read_code_file(char *filename)
{
    int fd, msize;

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror(filename);
        return -1;
    }

    msize = 1024*1024;
    code = malloc(msize);
    code_len = read(fd, code + 04000, msize);
    if (code_len < 0) {
        perror(filename);
    }

    code_len += 04000;

    close(fd);
    return 0;
}

int
scan_file(char *filename)
{
    make_isn_table();

    if (read_code_file(filename))
        return -1;

    if (scan_code(1))
        return -1;

    if (scan_code(2))
        return -1;

    if (scan_code(3))
        return -1;

    return 0;
}

void
usage(void)
{
}

extern int optind;
extern char *optarg;

main(int argc, char *argv[])
{
    int c;
    char *filename;

    filename = NULL;

    while ((c = getopt(argc, argv, "d")) != -1) {
        switch (c) {
        case 'd':
            break;
	}
    }

    dist_threshold = 64;
    dist_threshold = 32;

    if (optind < argc) {
        filename = argv[optind];
        scan_file(filename);
    } else
        usage();

    exit(0);
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
