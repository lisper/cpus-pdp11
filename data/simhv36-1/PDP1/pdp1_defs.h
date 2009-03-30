/* pdp1_defs.h: 18b PDP simulator definitions

   Copyright (c) 1993-2005, Robert M. Supnik

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

   22-Jul-05    RMS     Fixed definition of CPLS_DPY
   08-Feb-04    PLB     Added support for display
   08-Dec-03    RMS     Added support for parallel drum
   18-Oct-03    RMS     Added DECtape off reel message
   22-Jul-03    RMS     Updated for "hardware" RIM loader
                        Revised to detect I/O wait hang
   05-Dec-02    RMS     Added IOT skip support (required by drum)
   14-Apr-99    RMS     Changed t_addr to unsigned

   The PDP-1 was Digital's first computer.  The system design evolved during
   its life, and as a result, specifications are sketchy or contradictory.
   This simulator is based on the 1962 maintenance manual.

   This simulator implements the following options:

   Automatic multiply/divide    Type 10
   Memory extension control     Type 15
   Parallel drum                Type 23
   Serial drum                  Type 24
   Graphic display              Type 30
   Line printer control         Type 62
   Microtape (DECtape) control  Type 550
*/

#ifndef _PDP1_DEFS_H_
#define _PDP1_DEFS_H_   0

#include "sim_defs.h"

/* Simulator stop codes */

#define STOP_RSRV       1                               /* must be 1 */
#define STOP_HALT       2                               /* HALT */
#define STOP_IBKPT      3                               /* breakpoint */
#define STOP_XCT        4                               /* nested XCT's */
#define STOP_IND        5                               /* nested indirects */
#define STOP_WAIT       6                               /* IO wait hang */
#define STOP_DTOFF      7                               /* DECtape off reel */

/* Memory */

#define ASIZE           16                              /* address bits */
#define MAXMEMSIZE      (1u << ASIZE)                   /* max mem size */
#define AMASK           (MAXMEMSIZE - 1)                /* address mask */
#define MEMSIZE         (cpu_unit.capac)                /* actual memory size */
#define MEM_ADDR_OK(x)  (((uint32) (x)) < MEMSIZE)

/* Architectural constants */

#define DMASK           0777777                         /* data mask */
#define DAMASK          0007777                         /* direct addr */
#define EPCMASK         (AMASK & ~DAMASK)               /* extended addr */
#define IA              0010000                         /* indirect flag */
#define IO_WAIT         0010000                         /* I/O sync wait */
#define IO_CPLS         0004000                         /* completion pulse */
#define OP_DAC          0240000                         /* DAC */
#define OP_DIO          0320000                         /* DIO */
#define OP_JMP          0600000                         /* JMP */
#define GEN_CPLS(x)     (((x) ^ ((x) << 1)) & IO_WAIT)  /* completion pulse? */

/* IOT subroutine return codes */

#define IOT_V_SKP       18                              /* skip */
#define IOT_SKP         (1 << IOT_V_SKP)
#define IOT_V_REASON    (IOT_V_SKP + 1)                 /* reason */
#define IOT_REASON      (1 << IOT_V_REASON)
#define IORETURN(f,v)   ((f)? (v): SCPE_OK)             /* stop on error */

/* I/O status flags */

#define IOS_V_LPN       17                              /* light pen */
#define IOS_V_PTR       16                              /* paper tape reader */
#define IOS_V_TTO       15                              /* typewriter out */
#define IOS_V_TTI       14                              /* typewriter in */
#define IOS_V_PTP       13                              /* paper tape punch */
#define IOS_V_DRM       12                              /* drum */
#define IOS_V_SQB       11                              /* sequence break */
#define IOS_V_PNT       2                               /* print done */
#define IOS_V_SPC       1                               /* space done */
#define IOS_V_DRP       0                               /* parallel drum busy */

#define IOS_LPN         (1 << IOS_V_LPN)
#define IOS_PTR         (1 << IOS_V_PTR)
#define IOS_TTO         (1 << IOS_V_TTO)
#define IOS_TTI         (1 << IOS_V_TTI)
#define IOS_PTP         (1 << IOS_V_PTP)
#define IOS_DRM         (1 << IOS_V_DRM)
#define IOS_SQB         (1 << IOS_V_SQB)
#define IOS_PNT         (1 << IOS_V_PNT)
#define IOS_SPC         (1 << IOS_V_SPC)
#define IOS_DRP         (1 << IOS_V_DRP)

/* Completion pulses */

#define CPLS_V_PTR      5
#define CPLS_V_PTP      4
#define CPLS_V_TTO      3
#define CPLS_V_LPT      2
#define CPLS_V_DPY      1
#define CPLS_PTR        (1 << CPLS_V_PTR)
#define CPLS_PTP        (1 << CPLS_V_PTP)
#define CPLS_TTO        (1 << CPLS_V_TTO)
#define CPLS_LPT        (1 << CPLS_V_LPT)
#define CPLS_DPY        (1 << CPLS_V_DPY)

/* Sequence break flags */

#define SB_V_IP         0                               /* in progress */
#define SB_V_RQ         1                               /* request */
#define SB_V_ON         2                               /* enabled */

#define SB_IP           (1 << SB_V_IP)
#define SB_RQ           (1 << SB_V_RQ)
#define SB_ON           (1 << SB_V_ON)

#endif
