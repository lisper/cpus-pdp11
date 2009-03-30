/* sim_timer.h: simulator timer library headers

   Copyright (c) 1993-2005, Robert M Supnik

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

   02-Jan-04    RMS     Split out from SCP
*/

#ifndef _SIM_TIMER_H_
#define _SIM_TIMER_H_   0

#define SIM_NTIMERS     8                               /* # timers */
#define SIM_TMAX        500                             /* max timer makeup */

int32 sim_rtcn_init (int32 time, int32 tmr);
int32 sim_rtcn_calb (int32 ticksper, int32 tmr);
int32 sim_rtc_init (int32 time);
int32 sim_rtc_calb (int32 ticksper);
uint32 sim_os_msec (void);
void sim_os_sleep (unsigned int sec);

#endif
