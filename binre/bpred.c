/* bpred.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "binre.h"
#include "mach.h"
#include "isn.h"
#include "support.h"

extern int debug;

struct bpred_cache_s {
    int bits;
    int c_pc;
    int b_pc;
} bpred_cache[1024];

unsigned long bpred_attempts;
unsigned long bpred_correct;
unsigned long bpred_wrong;
unsigned long bpred_target_correct;

void bpred_inform(int taken, int cpc, int bpc)
{
    int index;
    struct bpred_cache_s *b;

    index = (u32)pc & (1024-1);
    b = &bpred_cache[index];

//
    bpred_attempts++;

    if (b->bits) {
        /* pred says we should take branch */
        if (taken) {
            bpred_correct++;

            if (b->b_pc == bpc)
                bpred_target_correct++;
        } else
            bpred_wrong++;
    } else {
        /* pred says we should NOT take branch */
        if (!taken)
            bpred_correct++;
        else
            bpred_wrong++;
    }

    {
        double bgood, bbad, bwise;

        bgood = (((double)bpred_correct) / ((double)bpred_attempts)) * 100.0;
        bbad = (((double)bpred_wrong) / ((double)bpred_attempts)) * 100.0;
        bwise = (((double)bpred_target_correct)/((double)bpred_correct))*100.0;

        printf("bpred: total %ld/%ld/%ld/%ld good %g bad %g wise %g\n",
               bpred_attempts, bpred_correct, bpred_wrong, bpred_target_correct,
               bgood, bbad, bwise);
    }
//

    b->c_pc = cpc;
    b->b_pc = bpc;

    if (taken) {
        b->bits = ((b->bits&1) << 1) | 1;
    } else {
        b->bits = ((b->bits&1) << 1) | 0;
    }
}

int bpred_check(int cpc, int *pbpc)
{
    int index;
    struct bpred_cache_s *b;

    index = (u32)pc & (1024-1);
    b = &bpred_cache[index];

    if (b->bits) {
        return 1;
        *pbpc = b->b_pc;
    }

    return 0;
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/

