/* i1620_fp.c: IBM 1620 floating point simulator

   Copyright (c) 2002-2005, Robert M. Supnik

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Robert M Supnik shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Robert M Supnik.

   The IBM 1620 uses a variable length floating point format, with a fixed
   two digit decimal exponent and a variable length decimal mantissa:

        _       S_S
        M.......MEE

   where S represents flag bits if the mantissa or exponent are negative.
*/

#include "i1620_defs.h"

#define FP_LMAX         100                             /* max fp mant lnt */
#define FP_EMAX         99                              /* max fp exponent */

/* Unpacked floating point operand */

typedef struct {
    int32       sign;                                   /* 0 => +, 1 => - */
    int32       exp;                                    /* binary exponent */
    uint32      lnt;                                    /* mantissa length */
    uint32      addr;                                   /* mantissa addr */
    uint32      zero;                                   /* 0 => nz, 1 => zero */
    } FPA;

extern uint8 M[MAXMEMSIZE];                             /* main memory */
extern uint8 ind[NUM_IND];                              /* indicators */
extern UNIT cpu_unit;

t_stat fp_scan_mant (uint32 ad, uint32 *lnt, uint32 *zro);
t_stat fp_zero (FPA *fp);

extern t_stat xmt_field (uint32 d, uint32 s, uint32 skp);
extern t_stat add_field (uint32 d, uint32 s, t_bool sub, t_bool sto, int32 *sta);
extern t_stat mul_field (uint32 d, uint32 s);
extern t_stat xmt_divd (uint32 d, uint32 s);
extern t_stat div_field (uint32 dvd, uint32 dvr, int32 *ez);

/* Unpack and validate a floating point argument */

t_stat fp_unpack (uint32 ad, FPA *fp)
{
uint8 d0, d1, esign;

esign = M[ad] & FLAG;                                   /* get exp sign */
d0 = M[ad] & DIGIT;                                     /* get exp lo digit */
MM (ad);
if ((M[ad] & FLAG) == 0) return STOP_FPMF;              /* no flag on hi exp? */
d1 = M[ad] & DIGIT;                                     /* get exp hi digit */
MM (ad);
fp->addr = ad;                                          /* save mant addr */
if (BAD_DIGIT (d1) || BAD_DIGIT (d0)) return STOP_INVDIG; /* exp bad dig? */
fp->exp = ((d1 * 10) + d0) * (esign? -1: 1);            /* convert exponent */
fp->sign = (M[ad] & FLAG)? 1: 0;                        /* get mantissa sign */
return fp_scan_mant (fp->addr, &(fp->lnt), &(fp->zero));
}

/* Unpack and validate source and destination arguments */

t_stat fp_unpack_two (uint32 dad, uint32 sad, FPA *dfp, FPA *sfp)
{
t_stat r;

if ((r = fp_unpack (dad, dfp)) != SCPE_OK) return r;    /* unpack dst */
if ((r = fp_unpack (sad, sfp)) != SCPE_OK) return r;    /* unpack src */
if (sfp->lnt != dfp->lnt) return STOP_FPUNL;            /* lnts must be equal */
return SCPE_OK;
}

/* Pack floating point result */

t_stat fp_pack (FPA *fp)
{
int32 e;
uint32 i, mad;

e = (fp->exp >= 0)? fp->exp: -fp->exp;                  /* get |exp| */                                 
if (e > FP_EMAX) {                                      /* too big? */
    ind[IN_EXPCHK] = 1;                                 /* set indicator */
    if (fp->exp < 0) return fp_zero (fp);               /* underflow? */
    mad = fp->addr;
    for (i = 0; i < fp->lnt; i++) {                     /* mant = 99...99 */
        M[mad] = (M[mad] & FLAG) | 9;
        MM (mad);
        }
    e = FP_EMAX;                                        /* cap at max */
    }
M[ADDR_A (fp->addr, 1)] = (e / 10) | FLAG;              /* high exp digit */
M[ADDR_A (fp->addr, 2)] = (e % 10) |                    /* low exp digit */
     ((fp->exp < 0)? FLAG: 0);
return SCPE_OK;
}

/* Shift mantissa right n positions */

void fp_rsh (FPA *fp, uint32 n)
{
uint32 i, sad, dad;

if (n == 0) return;                                     /* zero? done */
sad = ADDR_S (fp->addr, n);                             /* src = addr - n */
dad = fp->addr;                                         /* dst = n */
for (i = 0; i < fp->lnt; i++) {                         /* move digits */
    if (i >= (fp->lnt - n)) M[dad] = M[dad] & FLAG;
    else M[dad] = (M[dad] & FLAG) | (M[sad] & DIGIT);
    MM (dad);
    MM (sad);
    }
return;
}

/* Shift mantissa left 1 position */

void fp_lsh_1 (FPA *fp)
{
uint32 i, mad, nxt;

mad = ADDR_S (fp->addr, fp->lnt - 1);                   /* hi order digit */
for (i = 0; i < (fp->lnt - 1); i++) {                   /* move lnt-1 digits */
    nxt = ADDR_A (mad, 1);
    M[mad] = (M[mad] & FLAG) | (M[nxt] & DIGIT);
    mad = nxt;
    }
M[mad] = M[mad] & FLAG;                                 /* clear last digit */
return;
}

/* Clear floating point number */

t_stat fp_zero (FPA *fp)
{
uint32 i, mad = fp->addr;

for (i = 0; i < fp->lnt; i++) {                         /* clear mantissa */
    M[mad] = (i? M[mad] & FLAG: 0);                     /* clear sign bit */
    MM (mad);
    }
M[ADDR_A (fp->addr, 1)] = FLAG + 9;                     /* exp = -99 */
M[ADDR_A (fp->addr, 2)] = FLAG + 9;                     /* exp = -99 */
ind[IN_EZ] = 1;                                         /* result = 0 */
ind[IN_HP] = 0;
return SCPE_OK;
}

/* Scan floating point mantissa for length and (optionally) zero */

t_stat fp_scan_mant (uint32 ad, uint32 *lnt, uint32 *zro)
{
uint8 d, l, z;

z = 1;                                                  /* assume zero */
for (l = 1; l <= FP_LMAX; l++) {                        /* scan to get length */
    d = M[ad] & DIGIT;                                  /* get mant digit */
    if (d) z = 0;                                       /* non-zero? */
    if ((l != 1) && (M[ad] & FLAG)) {                   /* flag past first dig? */
        *lnt = l;                                       /* set returns */
        if (zro) *zro = z;
        return SCPE_OK;
        }
    MM (ad);
    }
return STOP_FPLNT;                                      /* too long */
}

/* Copy floating point mantissa */

void fp_copy_mant (uint32 d, uint32 s, uint32 l)
{
uint32 i;

if (ind[IN_HP]) M[d] = M[d] & ~FLAG;                    /* clr/set sign */
else M[d] = M[d] | FLAG;
for (i = 0; i < l; i++) {                               /* copy src */
    M[d] = (M[d] & FLAG) | (M[s] & DIGIT);              /* preserve flags */
    MM (d);
    MM (s);
    }
return;
}

/* Compare floating point mantissa */

int32 fp_comp_mant (uint32 d, uint32 s, uint32 l)
{
uint8 i, dd, sd;

d = ADDR_S (d, l - 1);                                  /* start of mantissa */
s = ADDR_S (s, l - 1);
for (i = 0; i < l; i++) {                               /* compare dst:src */
    dd = M[d] & DIGIT;                                  /* get dst digit */
    sd = M[s] & DIGIT;                                  /* get src digit */
    if (dd > sd) return 1;                              /* >? done */
    if (dd < sd) return -1;                             /* <? done */
    PP (d);                                             /* =? continue */
    PP (s);
    }
return 0;                                               /* done, equal */
}

/* Floating point add */

t_stat fp_add (uint32 d, uint32 s, t_bool sub)
{
FPA sfp, dfp;
uint32 i, sad, hi;
int32 dif, sta;
uint8 sav_src[FP_LMAX];
t_stat r;

r = fp_unpack_two (d, s, &dfp, &sfp);                   /* unpack operands */
if (r != SCPE_OK) return r;                             /* error? */
dif = dfp.exp - sfp.exp;                                /* exp difference */

if (sfp.zero || (dif >= ((int32) dfp.lnt))) {           /* src = 0, or too small? */
    if (dfp.zero) return fp_zero (&dfp);                /* res = dst, zero? */          
    ind[IN_EZ] = 0;                                     /* res nz, set EZ, HP */
    ind[IN_HP] = (dfp.sign == 0);
    return SCPE_OK;
    }
if (dfp.zero || (dif <= -((int32) dfp.lnt))) {          /* dst = 0, or too small? */
    if (sfp.zero) return fp_zero (&dfp);                /* res = src, zero? */
    r = xmt_field (d, s, 3);                            /* copy src to dst */
    ind[IN_EZ] = 0;                                     /* res nz, set EZ, HP */
    ind[IN_HP] = (dfp.sign == 0);
    return r;
    }

if (dif > 0) {                                          /* dst exp > src exp? */
    sad = sfp.addr;                                     /* save src in save area */
    for (i = 0; i < sfp.lnt; i++) {
        sav_src[i] = M[sad];
        MM (sad);
        }
    fp_rsh (&sfp, dif);                                 /* denormalize src */
    }
else if (dif < 0) {                                     /* dst exp < src exp? */
    dfp.exp = sfp.exp;                                  /* res exp = src exp */
    fp_rsh (&dfp, -dif);                                /* denormalize dst */
    }
r = add_field (dfp.addr, sfp.addr, sub, TRUE, &sta);    /* add mant, set EZ, HP */
if (dif > 0) {                                          /* src denormalized? */
    sad = sfp.addr;                                     /* restore src from */
    for (i = 0; i < sfp.lnt; i++) {                     /* save area */
        M[sad] = sav_src[i];
        MM (sad);
        }
    }
if (r != SCPE_OK) return r;                             /* add error? */

hi = ADDR_S (dfp.addr, dfp.lnt - 1);                    /* addr of hi digit */
if (sta == ADD_CARRY) {                                 /* carry out? */
    fp_rsh (&dfp, 1);                                   /* shift mantissa */
    M[hi] = FLAG + 1;                                   /* high order 1 */
    dfp.exp = dfp.exp + 1;
    ind[IN_EZ] = 0;                                     /* not zero */
    ind[IN_HP] = (dfp.sign == 0);                       /* set HP */
    }
else if (ind[IN_EZ]) return fp_zero (&dfp);             /* result zero? */
else {
    while ((M[hi] & DIGIT) == 0) {                      /* until normalized */
        fp_lsh_1 (&dfp);                                /* left shift */
        dfp.exp = dfp.exp - 1;                          /* decr exponent */
        }
    }

return fp_pack (&dfp);                                  /* pack and exit */
}

/* Floating point multiply */

t_stat fp_mul (uint32 d, uint32 s)
{
FPA sfp, dfp;
uint32 pad;
t_stat r;

r = fp_unpack_two (d, s, &dfp, &sfp);                   /* unpack operands */
if (r != SCPE_OK) return r;                             /* error? */
if (sfp.zero || dfp.zero) return fp_zero (&dfp);        /* either zero? */

r = mul_field (dfp.addr, sfp.addr);                     /* mul, set EZ, HP */
if (r != SCPE_OK) return r;
if (M[ADDR_S (PROD_AREA_END, 2 * dfp.lnt)] & DIGIT) {   /* hi prod dig set? */
    pad = ADDR_S (PROD_AREA_END - 1, dfp.lnt);          /* no normalization */
    dfp.exp = dfp.exp + sfp.exp;                        /* res exp = sum */
    }
else {
    pad = ADDR_S (PROD_AREA_END, dfp.lnt);              /* 'normalize' 1 */
    dfp.exp = dfp.exp + sfp.exp - 1;                    /* res exp = sum - 1 */
    }
fp_copy_mant (dfp.addr, pad, dfp.lnt);                  /* copy prod to mant */

return fp_pack (&dfp);                                  /* pack and exit */
}

/* Floating point divide */

t_stat fp_div (uint32 d, uint32 s)
{
FPA sfp, dfp;
uint32 i, pad, a100ml, a99ml;
int32 ez;
t_stat r;

r = fp_unpack_two (d, s, &dfp, &sfp);                   /* unpack operands */
if (r != SCPE_OK) return r;                             /* error? */
if (sfp.zero) {                                         /* divide by zero? */
    ind[IN_OVF] = 1;                                    /* dead jim */
    return SCPE_OK;
    }
if (dfp.zero) return fp_zero (&dfp);                    /* divide into zero? */

for (i = 0; i < PROD_AREA_LEN; i++)                     /* clear prod area */
    M[PROD_AREA + i] = 0;
a100ml = ADDR_S (PROD_AREA_END, dfp.lnt);               /* 100 - lnt */
a99ml = ADDR_S (PROD_AREA_END - 1, dfp.lnt);            /* 99 - lnt */
if (fp_comp_mant (dfp.addr, sfp.addr, dfp.lnt) >= 0) {  /* |Mdst| >= |Msrc|? */
    pad = a100ml;
    dfp.exp = dfp.exp - sfp.exp + 1;                    /* res exp = diff + 1 */
    }
else {
    pad = a99ml;
    dfp.exp = dfp.exp - sfp.exp;                        /* res exp = diff */
    }
r = xmt_divd (pad, dfp.addr);                           /* xmt dividend */
if (r != SCPE_OK) return r;                             /* error? */
r = div_field (a100ml, sfp.addr, &ez);                  /* divide fractions */
if (r != SCPE_OK) return r;                             /* error? */
if (ez) return fp_zero (&dfp);                          /* result zero? */

ind[IN_HP] = ((dfp.sign ^ sfp.sign) == 0);              /* set res sign */
ind[IN_EZ] = 0;                                         /* not zero */
fp_copy_mant (dfp.addr, a99ml, dfp.lnt);                /* copy result */

return fp_pack (&dfp);
}

/* Floating shift right */

t_stat fp_fsr (uint32 d, uint32 s)
{
uint32 cnt;
uint8 t;

if (d == s) return SCPE_OK;                             /* no move? */

cnt = 0;
M[d] = (M[d] & FLAG) | (M[s] & DIGIT);                  /* move 1st wo flag */
do {
    MM (d);                                             /* decr ptrs */
    MM (s);
    t = M[d] = M[s] & (FLAG | DIGIT);                   /* copy others */
    if (cnt++ > MEMSIZE) return STOP_FWRAP;             /* (stop runaway) */
    } while ((t & FLAG) == 0);                          /* until src flag */

cnt = 0;
do {
    MM (d);                                             /* decr pointer */
    t = M[d];                                           /* save old val */
    M[d] = 0;                                           /* zero field */
    if (cnt++ > MEMSIZE) return STOP_FWRAP;             /* (stop runaway) */
    } while ((t & FLAG) == 0);                          /* until dst flag */
return SCPE_OK;
} 

/* Floating shift left - note that dst is addr of high order digit */

t_stat fp_fsl (uint32 d, uint32 s)
{
uint32 i, lnt;
uint8 sign;
t_stat r;

if (d == s) return SCPE_OK;
sign = M[s] & FLAG;                                     /* get src sign */
r = fp_scan_mant (s, &lnt, NULL);                       /* get src length */
if (r != SCPE_OK) return r;                             /* error? */
s = ADDR_S (s, lnt - 1);                                /* hi order src */
M[d] = M[s] & (FLAG | DIGIT);                           /* move 1st w flag */
M[s] = M[s] & ~FLAG;                                    /* clr flag from src */
for (i = 1; i < lnt; i++) {                             /* move src to dst */
    PP (d);                                             /* incr ptrs */
    PP (s);
    M[d] = M[s] & DIGIT;                                /* move just digit */
    }
PP (d);                                                 /* incr pointer */
while ((M[d] & FLAG) == 0) {                            /* until flag */
    M[d] = 0;                                           /* clear field */
    PP (d);
    }
if (sign) M[d] = FLAG;                                  /* -? zero under sign */
return SCPE_OK;
}
