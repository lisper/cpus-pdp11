/* sim_rev.h: simulator revisions and current rev level

   Copyright (c) 1993-2006, Robert M Supnik

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
*/

#ifndef _SIM_REV_H_
#define _SIM_REV_H_     0

#define SIM_MAJOR       3
#define SIM_MINOR       6
#define SIM_PATCH       1

/* V3.6 revision history 

patch   date            module(s) and fix(es)

  1     25-Jul-06       sim_console.c:
                        - implemented SET/SHOW PCHAR

                        all DECtapes:
                        - fixed conflict in ATTACH switches

                        hp2100_ms.c (from Dave Bryan):
                        - added CAPACITY as alternate for REEL
                        - fixed EOT test for unlimited reel size

                        i1620_cd.c (from Tom McBride):
                        - fixed card reader fgets call
                        - fixed card reader boot sequence

                        i7094_cd.c:
                        - fixed problem with 80 column full cards

                        i7094_cpu.c:
                        - fixed bug in halt IO wait loop

                        i7094_sys.c:
                        - added binary loader (courtesy of Dave Pitt)

                        pdp1_cpu.c:
                        - fixed bugs in MUS and DIV

                        pdp11_cis.c:
                        - added interrupt tests to character instructions
                        - added 11/44 stack probe test to MOVCx (only)

                        pdp11_dl.c:
                        - first release

                        pdp11_rf.c:
                        - first release

                        pdp11_stddev.c:
                        - added UC support to TTI, TTO

                        pdp18b_cpu.c:
                        - fixed RESET to clear AC, L, and MQ

                        pdp18b_dt.c:
                        - fixed checksum calculation bug for Type 550

                        pdp18b_fpp.c:
                        - fixed bugs in left shift, multiply

                        pdp18b_stddev.c:
                        - fixed Baudot letters/figures inversion for PDP-4
                        - fixed letters/figures tracking for PDP-4
                        - fixed PDP-4/PDP-7 default terminal to be local echo

                        pdp18b_sys.c:
                        - added Fiodec, Baudot display
                        - generalized LOAD to handle HRI, RIM, and BIN files

                        pdp8_ttx.c:
                        - fixed bug in DETACH routine

  0     15-May-06       scp.c:
                        - revised save file format to save options, unit capacity

                        sim_tape.c, sim_tape.h:
                        - added support for finite reel size
                        - fixed bug in P7B write record

                        most magtapes:
                        - added support for finite reel size

                        h316_cpu.c: fixed bugs in LLL, LRL (found by Theo Engel)

                        h316_lp.c: fixed bug in blanks backscanning (found by Theo Engel)

                        h316_stddev.c: fixed bugs in punch state handling (found by Theo Engel)

                        i1401_cpu.c: fixed bug in divide (reported by Van Snyder)

                        i16_cpu.c: fixed bug in DH (found by Mark Hittinger)

                        i32_cpu.c:
                        - fixed bug in DH (found by Mark Hittinger)
                        - added support for 8 register banks in 8/32

                        i7094: first release

                        id_io.c: fixed bug, GO preserves EXA and SSTA (found by Davis Johnson)

                        id_idc.c:
                        - fixed WD/WH handling (found by Davis Johnson)
                        - fixed bug, nop command should be ignored (found by Davis Johnson)

                        nova_cpu.c: fixed bug in DIVS (found by Mark Hittinger)

                        pdp11_cis.c: (all reported by John Dundas)
                        - fixed bug in decode table
                        - fixed bug in ASHP
                        - fixed bug in write decimal string with mmgt enabled
                        - fixed bug in 0-length strings in multiply/divide

                        pdp11_cpu.c: fixed order of operand fetching in XOR for SDSD models

                        pdp11_cr.c: added CR11/CD11 support

                        pdp11_tc.c:
                        - fixed READ to set extended data bits in TCST (found by Alan Frisbie)

                        vax780_fload.c: added FLOAD command

                        vax780_sbi.c: fixed writes to ACCS

                        vax780_stddev.c: revised timer logic for EVKAE (reported by Tim Stark)

                        vax_cis.c: (all reported by Tim Stark)
                        - fixed MOVTC, MOVTUC to preserve cc's through page faults
                        - fixed MOVTUC to stop on translated == escape
                        - fixed CVTPL to set registers before destination reg write
                        - fixed CVTPL to set correct cc bit on overflow
                        - fixed EDITPC to preserve cc's through page faults
                        - fixed EDITPC EO$BLANK_ZERO count, cc test
                        - fixed EDITPC EO$INSERT to insert fill instead of blank
                        - fixed EDITPC EO$LOAD_PLUS/MINUS to skip character

                        vax_cpu.c:
                        - added KESU capability to virtual examine
                        - fixed bugs in virtual examine
                        - rewrote CPU history function for improved usability
                        (bugs below reported by Tim Stark)
                        - fixed fault cleanup to clear PSL<tp>
                        - fixed ADAWI r-mode to preserve dst<31:16>
                        - fixed ACBD/G to test correct operand
                        - fixed access checking on modify-class specifiers
                        - fixed branch address calculation in CPU history
                        - fixed bug in reported VA on faulting cross-page write

                        vax_cpu1.c: (all reported by Tim Stark)
                        - added access check on system PTE for 11/780
                        - added mbz check in LDPCTX for 11/780

                        vax_cmode.c: (all reported by Tim Stark)
                        - fixed omission of SXT
                        - fixed order of operand fetching in XOR

                        vax_fpa.c: (all reported by Tim Stark)
                        - fixed POLYD, POLYG to clear R4, R5
                        - fixed POLYD, POLYG to set R3 correctly
                        - fixed POLYD, POLYG to not exit prematurely if arg = 0
                        - fixed POLYD, POLYG to do full 64b multiply
                        - fixed POLYF, POLYD, POLYG to remove truncation on add
                        - fixed POLYF, POLYD, POLYG to mask mul reslt to 31b/63b/63b
                        - fixed fp add routine to test for zero via fraction
                          to support "denormal" argument from POLYF, POLYD, POLYG
                        - fixed bug in 32b floating multiply routine
                        - fixed bug in 64b extended modulus routine

                        vax_mmu.c:
                        - added access check on system PTE for 11/780

                        vax_octa.c: (all reported by Tim Stark)
                        - fixed MNEGH to test negated sign, clear C
                        - fixed carry propagation in qp_inc, qp_neg, qp_add
                        - fixed pack routines to test for zero via fraction
                        - fixed ACBH to set cc's on result
                        - fixed POLYH to set R3 correctly
                        - fixed POLYH to not exit prematurely if arg = 0
                        - fixed POLYH to mask mul reslt to 127b
                        - fixed fp add routine to test for zero via fraction
                          to support "denormal" argument from POLYH
                        - fixed EMODH to concatenate 15b of 16b extension
                        - fixed bug in reported VA on faulting cross-page write


/* V3.5 revision history 

patch   date            module(s) and fix(es)

  2     07-Jan-06       scp.c:
                        - added breakpoint spaces
                        - added REG_FIT support

                        sim_console.c: added ASCII character processing routines

                        sim_tape.c, sim_tape.h:
                        - added write support for P7B format
                        - fixed bug in write forward (found by Dave Bryan)

                        h316_stddev.c, hp2100_stddev.c, hp2100_mux.c, id_tt.c,
                        id_ttp.c, id_pas.c, pdp8_tt.c, pdp8_ttx.c, pdp11_stddev.c,
                        pdp11_dz.c, pdp18b_stddev.c, pdp18b_tt1.c, vax_stddev,
                        gri_stddev.c:
                        - revised to support new character handling routines

                        pdp10_rp.c: fixed DCLR not to clear disk address

                        pdp11_hk.c: fixed overlapped seek interaction with NOP, etc

                        pdp11_rh.c: added enable/disable routine

                        pdp11_rq.c, pdp11_tm.c, pdp11_tq.c, pdp11_ts.c
                        - widened address display to 64b when USE_ADDR64

                        pdp11_rp.c:
                        - fixed DCLR not to clear disk address
                        - fixed device enable/disable logic to include Massbus adapter
                        - widened address display to 64b when USE_ADDR64

                        pdp11_tu.c:
                        - fixed device enable/disable logic to include Massbus adapter
                        - widened address display to 64b when USE_ADDR64
                        - changed default adapter to TM03 (for VMS)

                        pdp8_df.c, pdp8_dt.c, pdp8_rf.c:
                        - fixed unaligned access bug (found by Doug Carman)

                        pdp8_rl.c: fixed IOT 61 decoding bug (found by David Gesswein)

                        vax_cpu.c:
                        - fixed breakpoint detection when USE_ADDR64 option is active
                        - fixed CVTfi to trap on integer overflow if PSW<iv> set

  1     15-Oct-05       All CPU's, other sources: fixed declaration inconsistencies
                        (from Sterling Garwood)

                        i1401_cpu.c: added control for old/new character encodings

                        i1401_cd.c, i1401_lpt.c, i1401_tty.c:
                        - changed character encodings to be consistent with 7094
                        - changed column binary format to be consistent with 7094
                        - added choice of business or Fortran set for output encoding

                        i1401_sys.c: changed WM character to ` under new encodings

                        i1620_cd.c, i1620_lpt.c, i1620_tty.c:
                        - changed character encodings to be consistent with 7094

                        pdp10_cpu.c: changed MOVNI to eliminate gcc warning

                        pdp11_io.c: fixed bug in autoconfiguration (missing XU)

                        vax_io.c: fixed bug in autoconfiguration (missing XU)

                        vax_fpa.c: fixed bug in 32b structure definitions (from Jason Stevens)

  0     1-Sep-05        Note: most source modules have been edited to improve
                        readability and to fix declaration and cast problems in C++

                        all instruction histories: fixed reversed arguments to calloc

                        scp.c: revised to trim trailing spaces on file inputs

                        sim_sock.c: fixed SIGPIPE error on Unix

                        sim_ether.c: added Windows user-defined adapter names (from Timothe Litt)

                        sim_tape.c: fixed misallocation of TPC map array

                        sim_tmxr.c: added support for SET <unit> DISCONNECT

                        hp2100_mux.c: added SET MUXLn DISCONNECT

                        i1401_cpu.c:
                        - fixed SSB-SSG clearing on RESET (reported by Ralph Reinke)
                        - removed error stops in MCE

                        i1401_cd.c: fixed read, punch to ignore modifier on 1, 4 char inst
                        (reported by Van Snyder)

                        id_pas.c:
                        - fixed bug in SHOW CONN/STATS
                        - added SET PASLn DISCONNECT

                        pdp10_ksio.c: revised for new autoconfiguration interface

                        pdp11_cpu.c: replaced WAIT clock queue check with API call

                        pdp11_cpumod.c: added additional 11/60 registers

                        pdp11_io.c: revised autoconfiguration algorithm and interface

                        pdp11_dz.c: revised for new autoconfiguration interface

                        pdp11_vh.c:
                        - revised for new autoconfiguration interface
                        - fixed bug in vector display routine

                        pdp11_xu.c: fixed runt packet processing (found by Tim Chapman)

                        pdp18b_cpu.c, pdp18b_sys.c:
                        - removed spurious AAS instruction

                        pdp18b_tt1.c:
                        - fixed bug in SHOW CONN/STATS
                        - fixed bug in SET LOG/NOLOG
                        - added SET TTOXn DISCONNECT

                        pdp8_ttx.c:
                        - fixed bug in SHOW CONN/STATS
                        - fixed bug in SET LOG/NOLOG
                        - added SET TTOXn DISCONNECT

                        sds_mux.c:
                        - fixed bug in SHOW CONN/STATS
                        - added SET MUXLn DISCONNECT

                        vaxmod_defs.h: added QDSS support

                        vax_io.c: revised autoconfiguration algorithm and interface

/* V3.4 revision history 

  0     01-May-04       scp.c:
                        - fixed ASSERT code
                        - revised syntax for SET DEBUG (from Dave Bryan)
                        - revised interpretation of fprint_sym, fparse_sym returns
                        - moved DETACH sanity tests into detach_unit

                        sim_sock.h and sim_sock.c:
                        - added test for WSAEINPROGRESS (from Tim Riker)

                        many: revised detach routines to test for attached state

                        hp2100_cpu.c: reorganized CPU options (from Dave Bryan)

                        hp2100_cpu1.c: reorganized EIG routines (from Dave Bryan)

                        hp2100_fp1.c: added FFP support (from Dave Bryan)

                        id16_cpu.c:
                        - fixed bug in show history routine (from Mark Hittinger)
                        - revised examine/deposit to do words rather than bytes

                        id32_cpu.c:
                        - fixed bug in initial memory allocation
                        - fixed bug in show history routine (from Mark Hittinger)
                        - revised examine/deposit to do words rather than bytes

                        id16_sys.c, id32_sys:
                        - revised examine/deposit to do words rather than bytes

                        pdp10_tu.c:
                        - fixed bug, ERASE and WREOF should not clear done (reported
                          by Rich Alderson)
                        - fixed error reporting

                        pdp11_tu.c: fixed error reporting

/* V3.3 revision history 

  2     08-Mar-05       scp.c: added ASSERT command (from Dave Bryan)

                        h316_defs.h: fixed IORETURN macro

                        h316_mt.c: fixed error reporting from OCP (found by Philipp Hachtmann)

                        h316_stddev.c: fixed bug in OCP '0001 (found by Philipp Hachtmann)

                        hp2100_cpu.c: split out EAU and MAC instructions

                        hp2100_cpu1.c: (from Dave Bryan)
                        - fixed missing MPCK on JRS target
                        - removed EXECUTE instruction (is NOP in actual microcode)

                        hp2100_fp: (from Dave Bryan)
                        - fixed missing negative overflow renorm in StoreFP

                        i1401_lp.c: fixed bug in write_line (reported by Van Snyder)

                        id32_cpu.c: fixed branches to mask new PC (from Greg Johnson)

                        pdp11_cpu.c: fixed bugs in RESET for 11/70 (reported by Tim Chapman)

                        pdp11_cpumod.c:
                        - fixed bug in SHOW MODEL (from Sergey Okhapkin)
                        - made SYSID variable for 11/70 (from Tim Chapman)
                        - added MBRK write case for 11/70 (from Tim Chapman)

                        pdp11_rq: added RA60, RA71, RA81 disks

                        pdp11_ry: fixed bug in boot code (reported by Graham Toal)

                        vax_cpu.c: fixed initial state of cpu_extmem

  1     05-Jan-05       h316_cpu.c: fixed bug in DIV

                        h316_stddev.c:
                        - fixed bug in SKS '104 (reported by Philipp Hachtmann)
                        - fixed bug in SKS '504
                        - adder reader/punch ASCII file support
                        - added Teletype reader/punch support

                        h316_dp.c: fixed bug in skip on !seeking

                        h316_mt.c: fixed bug in DMA/DMC support

                        h316_lp.c: fixed bug in DMA/DMC support

                        hp2100_cpu.c:
                        - fixed DMA reset to clear alternate CTL flop (from Dave Bryan)
                        - fixed DMA reset to not clear control words (from Dave Bryan)
                        - fixed SBS, CBS, TBS to do virtual reads
                        - separated A/B from M[0/1], for DMA IO (from Dave Bryan)
                        - added SET CPU 21MX-M, 21MX-E (from Dave Brian)
                        - disabled TIMER/EXECUTE/DIAG instructions for 21MX-M (from Dave Bryan)
                        - added post-processor to maintain T/M consistency (from Dave Bryan)

                        hp2100_ds.c: first release

                        hp2100_lps.c (all changes from Dave Bryan)
                        - added restart when set online, etc.
                        - fixed col count for non-printing chars

                        hp2100_lpt.c (all changes from Dave Bryan)
                        - added restart when set online, etc.

                        hp2100_sys.c (all changes from Dave Bryan):
                        - added STOP_OFFLINE, STOP_PWROFF messages

                        i1401_sys.c: added address argument support (from Van Snyder)

                        id_mt.c: added read-only file support

                        lgp_cpu.c, lgp_sys.c: modified VM pointer setup

                        pdp11_cpu.c: fixed WAIT to work in all modes (from John Dundas)

                        pdp11_tm.c, pdp11_ts.c: added read-only file support

                        sds_mt.c: added read-only file support

  0     23-Nov-04       scp.c:
                        - added reset_all_p (powerup)
                        - fixed comma-separated SET options (from Dave Bryan)
                        - changed ONLINE/OFFLINE to ENABLED/DISABLED (from Dave Bryan)
                        - modified to flush device buffers on stop (from Dave Bryan)
                        - changed HELP to suppress duplicate command displays

                        sim_console.c:
                        - moved SET/SHOW DEBUG under CONSOLE hierarchy

                        hp2100_cpu.c: (all fixes by Dave Bryan)
                        - moved MP into its own device; added MP option jumpers
                        - modified DMA to allow disabling
                        - modified SET CPU 2100/2116 to truncate memory > 32K
                        - added -F switch to SET CPU to force memory truncation
                        - fixed S-register behavior on 2116
                        - fixed LIx/MIx behavior for DMA on 2116 and 2100
                        - fixed LIx/MIx behavior for empty I/O card slots
                        - modified WRU to be REG_HRO
                        - added BRK and DEL to save console settings
                        - fixed use of "unsigned int16" in cpu_reset

                        hp2100_dp.c: (all fixes by Dave Bryan)
                        - fixed enable/disable from either device
                        - fixed ANY ERROR status for 12557A interface
                        - fixed unattached drive status for 12557A interface
                        - status cmd without prior STC DC now completes (12557A)
                        - OTA/OTB CC on 13210A interface also does CLC CC
                        - fixed RAR model
                        - fixed seek check on 13210 if sector out of range

                        hp2100_dq.c: (all fixes by Dave Bryan)
                        - fixed enable/disable from either device
                        - shortened xtime from 5 to 3 (drive avg 156KW/second)
                        - fixed not ready/any error status
                        - fixed RAR model

                        hp2100_dr.c: (all fixes by Dave Bryan)
                        - fixed enable/disable from either device
                        - fixed sector return in status word
                        - provided protected tracks and "Writing Enabled" status bit
                        - fixed DMA last word write, incomplete sector fill value
                        - added "parity error" status return on writes for 12606
                        - added track origin test for 12606
                        - added SCP test for 12606
                        - fixed 12610 SFC operation
                        - added "Sector Flag" status bit
                        - added "Read Inhibit" status bit for 12606
                        - fixed current-sector determination
                        - added TRACKPROT modifier

                        hp2100_ipl.c, hp2100_ms.c: (all fixes by Dave Bryan)
                        - fixed enable/disable from either device

                        hp2100_lps.c: (all fixes by Dave Bryan)
                        - added SET OFFLINE/ONLINE, POWEROFF/POWERON
                        - fixed status returns for error conditions
                        - fixed handling of non-printing characters
                        - fixed handling of characters after column 80
                        - improved timing model accuracy for RTE
                        - added fast/realistic timing
                        - added debug printouts

                        hp2100_lpt.c: (all fixes by Dave Bryan)
                        - added SET OFFLINE/ONLINE, POWEROFF/POWERON
                        - fixed status returns for error conditions
                        - fixed TOF handling so form remains on line 0

                        hp2100_stddev.c (all fixes by Dave Bryan)
                        - added paper tape loop mode, DIAG/READER modifiers to PTR
                        - added PV_LEFT to PTR TRLLIM register
                        - modified CLK to permit disable

                        hp2100_sys.c: (all fixes by Dave Bryan)
                        - added memory protect device
                        - fixed display of CCA/CCB/CCE instructions

                        i1401_cpu.c: added =n to SHOW HISTORY

                        id16_cpu.c: added instruction history

                        id32_cpu.c: added =n to SHOW HISTORY

                        pdp10_defs.h: revised Unibus DMA API's

                        pdp10_ksio.c: revised Unibus DMA API's

                        pdp10_lp20.c: revised Unibus DMA API's

                        pdp10_rp.c: replicated register state per drive

                        pdp10_tu.c:
                        - fixed to set FCE on short record
                        - fixed to return bit<15> in drive type
                        - fixed format specification, 1:0 are don't cares
                        - implemented write check
                        - TMK is cleared by new motion command, not DCLR
                        - DONE is set on data transfers, ATA on non data transfers

                        pdp11_defs.h: 
                        - revised Unibus/Qbus DMA API's
                        - added CPU type and options flags

                        pdp11_cpumod.h, pdp11_cpumod.c:
                        - new routines for setting CPU type and options

                        pdp11_io.c: revised Unibus/Qbus DMA API's

                        all PDP-11 DMA peripherals:
                        - revised Unibus/Qbus DMA API's

                        pdp11_hk.c: CS2 OR must be zero for M+

                        pdp11_rh.c, pdp11_rp.c, pdp11_tu.c:
                        - split Massbus adapter from controllers
                        - replicated RP register state per drive
                        - added TM02/TM03 with TE16/TU45/TU77 drives

                        pdp11_rq.c, pdp11_tq.c:
                        - provided different default timing for PDP-11, VAX
                        - revised to report CPU bus type in stage 1
                        - revised to report controller type reflecting bus type
                        - added -L switch (LBNs) to RAUSER size specification

                        pdp15_cpu.c: added =n to SHOW HISTORY

                        pdp15_fpp.c:
                        - fixed URFST to mask low 9b of fraction
                        - fixed exception PC setting

                        pdp8_cpu.c: added =n to SHOW HISTORY

                        vax_defs.h:
                        - added octaword, compatibility mode support

                        vax_moddefs.h: 
                        - revised Unibus/Qbus DMA API's

                        vax_cpu.c:
                        - moved processor-specific code to vax_sysdev.c
                        - added =n to SHOW HISTORY

                        vax_cpu1.c:
                        - moved processor-specific IPR's to vax_sysdev.c
                        - moved emulation trap to vax_cis.c
                        - added support for compatibility mode

                        vax_cis.c: new full VAX CIS instruction emulator

                        vax_octa.c: new full VAX octaword and h_floating instruction emulator

                        vax_cmode.c: new full VAX compatibility mode instruction emulator

                        vax_io.c:
                        - revised Unibus/Qbus DMA API's

                        vax_io.c, vax_stddev.c, vax_sysdev.c:
                        - integrated powerup into RESET (with -p)

                        vax_sys.c:
                        - fixed bugs in parsing indirect displacement modes
                        - fixed bugs in displaying and parsing character data

                        vax_syscm.c: added display and parse for compatibility mode

                        vax_syslist.c:
                        - split from vax_sys.c
                        - removed PTR, PTP

/* V3.2 revision history 

  3     03-Sep-04       scp.c:
                        - added ECHO command (from Dave Bryan)
                        - qualified RESTORE detach with SIM_SW_REST

                        sim_console: added OS/2 EMX fixes (from Holger Veit)

                        sim_sock.h: added missing definition for OS/2 (from Holger Veit)

                        hp2100_cpu.c: changed error stops to report PC not PC + 1
                        (from Dave Bryan)

                        hp2100_dp.c: functional and timing fixes (from Dave Bryan)
                        - controller sets ATN for all commands except read status
                        - controller resumes polling for ATN interrupts after read status
                        - check status on unattached drive set busy and not ready
                        - check status tests wrong unit for write protect status
                        - drive on line sets ATN, will set FLG if polling

                        hp2100_dr.c: fixed CLC to stop operation (from Dave Bryan)

                        hp2100_ms.c: functional and timing fixes (from Dave Bryan)
                        - fixed erroneous execution of rejected command
                        - fixed erroneous execution of select-only command
                        - fixed erroneous execution of clear command
                        - fixed odd byte handling for read
                        - fixed spurious odd byte status on 13183A EOF
                        - modified handling of end of medium
                        - added detailed timing, with fast and realistic modes
                        - added reel sizes to simulate end of tape
                        - added debug printouts

                        hp2100_mt.c: modified handling of end of medium (from Dave Bryan)

                        hp2100_stddev.c: added tab to control char set (from Dave Bryan)

                        pdp11_rq.c: VAX controllers luns start at 0 (from Andreas Cejna)

                        vax_cpu.c: fixed bug in EMODD/G, second word of quad dst not probed

  2     17-Jul-04       scp.c: fixed problem ATTACHing to read only files
                        (found by John Dundas)

                        sim_console.c: revised Windows console code (from Dave Bryan)

                        sim_fio.c: fixed problem in big-endian read
                        (reported by Scott Bailey)

                        gri_cpu.c: updated MSR, EAO functions

                        hp_stddev.c: generalized handling of control char echoing
                        (from Dave Bryan)

                        vax_sys.c: fixed bad block initialization routine

  1     10-Jul-04       scp.c: added SET/SHOW CONSOLE subhierarchy

                        hp2100_cpu.c: fixes and added features (from Dave Bryan)
                        - SBT increments B after store
                        - DMS console map must check dms_enb
                        - SFS x,C and SFC x,C work
                        - MP violation clears automatically on interrupt
                        - SFS/SFC 5 is not gated by protection enabled
                        - DMS enable does not disable mem prot checks
                        - DMS status inconsistent at simulator halt
                        - Examine/deposit are checking wrong addresses
                        - Physical addresses are 20b not 15b
                        - Revised DMS to use memory rather than internal format
                        - Added instruction printout to HALT message
                        - Added M and T internal registers
                        - Added N, S, and U breakpoints
                        Revised IBL facility to conform to microcode
                        Added DMA EDT I/O pseudo-opcode
                        Separated DMA SRQ (service request) from FLG

                        all HP2100 peripherals:
                        - revised to make SFS x,C and SFC x,C work
                        - revised to separate SRQ from FLG

                        all HP2100 IBL bootable peripherals:
                        - revised boot ROMs to use IBL facility
                        - revised SR values to preserve SR<5:3>

                        hp2100_lps.c, hp2100_lpt.c: fixed timing
                        
                        hp2100_dp.c: fixed interpretation of SR<0>

                        hp2100_dr.c: revised boot code to use IBL algorithm

                        hp2100_mt.c, hp2100_ms.c: fixed spurious timing error after CLC
                         (found by Dave Bryan)

                        hp2100_stddev.c:
                        - fixed input behavior during typeout for RTE-IV
                        - suppressed nulls on TTY output for RTE-IV

                        hp2100_sys.c: added SFS x,C and SFC x,C to print/parse routines

                        pdp10_fe.c, pdp11_stddev.c, pdp18b_stddev.c, pdp8_tt.c, vax_stddev.c:
                        - removed SET TTI CTRL-C option

                        pdp11_tq.c:
                        - fixed bug in reporting write protect (reported by Lyle Bickley)
                        - fixed TK70 model number and media ID (found by Robert Schaffrath)

                        pdp11_vh.c: added DHQ11 support (from John Dundas)

                        pdp11_io.c, vax_io.c: fixed DHQ11 autoconfigure (from John Dundas)

                        pdp11_sys.c, vax_sys.c: added DHQ11 support (from John Dundas)

                        vax_cpu.c: fixed bug in DIVBx, DIVWx (reported by Peter Trimmel)

  0     04-Apr-04       scp.c:
                        - added sim_vm_parse_addr and sim_vm_fprint_addr
                        - added REG_VMAD
                        - moved console logging to SCP
                        - changed sim_fsize to use descriptor rather than name
                        - added global device/unit show modifiers
                        - added device debug support (Dave Hittner)
                        - moved device and unit flags, updated save format

                        sim_ether.c:
                        - further generalizations (Dave Hittner, Mark Pizzolato)

                        sim_tmxr.h, sim_tmxr.c:
                        - added tmxr_linemsg
                        - changed TMXR definition to support variable number of lines

                        sim_libraries:
                        - new console library (sim_console.h, sim_console.c)
                        - new file I/O library (sim_fio.h, sim_fio.c)
                        - new timer library (sim_timer.h, sim_timer.c)

                        all terminal multiplexors: revised for tmxr library changes

                        all DECtapes:
                        - added STOP_EOR to enable end-of-reel stop
                        - revised for device debug support

                        all variable-sized devices: revised for sim_fsize change

                        eclipse_cpu.c, nova_cpu.c: fixed device enable/disable support
                           (found by Bruce Ray)

                        nova_defs.h, nova_sys.c, nova_qty.c:
                        - added QTY and ALM support (Bruce Ray)

                        id32_cpu.c, id_dp.c: revised for device debug support

                        lgp: added LGP-30 [LGP-21] simulator

                        pdp1_sys.c: fixed bug in LOAD (found by Mark Crispin)

                        pdp10_mdfp.c:
                        - fixed bug in floating unpack
                        - fixed bug in FIXR (found by Philip Stone, fixed by Chris Smith)

                        pdp11_dz.c: added per-line logging

                        pdp11_rk.c:
                        - added formatting support
                        - added address increment inhibit support
                        - added transfer overrun detection

                        pdp11_hk.c, pdp11_rp.c: revised for device debug support

                        pdp11_rq.c: fixed bug in interrupt control (found by Tom Evans)

                        pdp11_ry.c: added VAX support

                        pdp11_tm.c, pdp11_tq.c, pdp11_ts.c: revised for device debug support

                        pdp11_xu.c: replaced stub with real implementation (Dave Hittner)

                        pdp18b_cpu.c:
                        - fixed bug in XVM g_mode implementation
                        - fixed bug in PDP-15 indexed address calculation
                        - fixed bug in PDP-15 autoindexed address calculation

                        pdp18b_fpp.c: fixed bugs in instruction decode

                        pdp18b_stddev.c:
                        - fixed clock response to CAF
                        - fixed bug in hardware read-in mode bootstrap

                        pdp18b_sys.c: fixed XVM instruction decoding errors

                        pdp18b_tt1.c: added support for 1-16 additional terminals

                        vax_moddef.h, vax_cpu.c, vax_sysdev.c:
                        - added extended physical memory support (Mark Pizzolato)
                        - added RXV21 support

                        vax_cpu1.c:
                        - added PC read fault in EXTxV
                        - fixed PC write fault in INSV

/* V3.1 revision history

  0     29-Dec-03       sim_defs.h, scp.c: added output stall status

                        all console emulators: added output stall support

                        sim_ether.c (Dave Hittner, Mark Pizzolato, Anders Ahgren):
                        - added Alpha/VMS support
                        - added FreeBSD, Mac OS/X support
                        - added TUN/TAP support
                        - added DECnet duplicate address detection

                        all memory buffered devices (fixed head disks, floppy disks):
                        - cleaned up buffer copy code

                        all DECtapes:
                        - fixed reverse checksum in read all
                        - added DECtape off reel message
                        - simplified timing

                        eclipse_cpu.c (Charles Owen):
                        - added floating point support
                        - added programmable interval timer support
                        - bug fixes

                        h316_cpu.c:
                        - added instruction history
                        - added DMA/DMC support
                        - added device ENABLE/DISABLE support
                        - change default to HSA option included

                        h316_dp.c: added moving head disk support

                        h316_fhd.c: added fixed head disk support

                        h316_mt.c: added magtape support

                        h316_sys.c: added new device support

                        nova_dkp.c (Charles Owen):
                        - fixed bug in flag clear sequence
                        - added diagnostic mode support for disk sizing

`                       nova_mt.c (Charles Owen):
                        - fixed bug, space operations return record count
                        - fixed bug, reset doesn't cancel rewind

                        nova_sys.c: added floating point, timer support (from Charles Owen)

                        i1620_cpu.c: fixed bug in branch digit (found by Dave Babcock)

                        pdp1_drm.c:
                        - added parallel drum support
                        - fixed bug in serial drum instructin decoding

                        pdp1_sys.c: added parallel drum support, mnemonics

                        pdp11_cpu.c:
                        - added autoconfiguration controls
                        - added support for 18b-only Qbus devices
                        - cleaned up addressing/bus definitions

                        pdp11_rk.c, pdp11_ry.c, pdp11_tm.c, pdp11_hk.c:
                        - added Q18 attribute

                        pdp11_io.c:
                        - added autoconfiguration controls
                        - fixed bug in I/O configuration (found by Dave Hittner)

                        pdp11_rq.c:
                        - revised MB->LBN conversion for greater accuracy
                        - fixed bug with multiple RAUSER drives

                        pdp11_tc.c: changed to be off by default (base config is Qbus)

                        pdp11_xq.c (Dave Hittner, Mark Pizzolato):
                        - fixed second controller interrupts
                        - fixed bugs in multicast and promiscuous setup
  
                        pdp18b_cpu.c:
                        - added instruction history
                        - fixed PDP-4,-7,-9 autoincrement bug
                        - change PDP-7,-9 default to API option included

                        pdp8_defs.h, pdp8_sys.c:
                        - added DECtape off reel message
                        - added support for TSC8-75 (ETOS) option
                        - added support for TD8E controller

                        pdp8_cpu.c: added instruction history

                        pdp8_rx.c:
                        - fixed bug in RX28 read status (found by Charles Dickman)
                        - fixed double density write

                        pdp8_td.c: added TD8E controller

                        pdp8_tsc.c: added TSC8-75 option

                        vax_cpu.c:
                        - revised instruction history for dynamic sizing
                        - added autoconfiguration controls

                        vax_io.c:
                        - added autoconfiguration controls
                        - fixed bug in I/O configuration (found by Dave Hittner)

                        id16_cpu.c: revised instruction decoding

                        id32_cpu.c:
                        - revised instruction decoding
                        - added instruction history

/* V3.0 revision history 

  2     15-Sep-03       scp.c:
                        - fixed end-of-file problem in dep, idep
                        - fixed error on trailing spaces in dep, idep

                        pdp1_stddev.c
                        - fixed system hang if continue after PTR error
                        - added PTR start/stop functionality
                        - added address switch functionality to PTR BOOT

                        pdp1_sys.c: added multibank capability to LOAD
  
                        pdp18b_cpu.c:
                        - fixed priorities in PDP-15 API (PI between 3 and 4)
                        - fixed sign handling in PDP-15 unsigned mul/div
                        - fixed bug in CAF, must clear API subsystem

                        i1401_mt.c:
                        - fixed tape read end-of-record handling based on real 1401
                        - added diagnostic read (space forward)

                        i1620_cpu.c
                        - fixed bug in immediate index add (found by Michael Short)

  1     27-Jul-03       pdp1_cpu.c: updated to detect indefinite I/O wait

                        pdp1_drm.c: fixed incorrect logical, missing activate, break

                        pdp1_lp.c:
                        - fixed bugs in instruction decoding, overprinting
                        - updated to detect indefinite I/O wait

                        pdp1_stddev.c:
                        - changed RIM loader to be "hardware"
                        - updated to detect indefinite I/O wait

                        pdp1_sys.c: added block loader format support to LOAD

                        pdp10_rp.c: fixed bug in read header

                        pdp11_rq: fixed bug in user disk size (found by Chaskiel M Grundman)

                        pdp18b_cpu.c:
                        - added FP15 support
                        - added XVM support
                        - added EAE support to the PDP-4
                        - added PDP-15 "re-entrancy ECO"
                        - fixed memory protect/skip interaction
                        - fixed CAF to only reset peripherals

                        pdp18b_fpp.c: added FP15

                        pdp18b_lp.c: fixed bug in Type 62 overprinting

                        pdp18b_rf.c: fixed bug in set size routine

                        pdp18b_stddev.c:
                        - increased PTP TIME for PDP-15 operating systems
                        - added hardware RIM loader for PDP-7, PDP-9, PDP-15

                        pdp18b_sys.c: added FP15, KT15, XVM instructions

                        pdp8b_df.c, pdp8_rf.c: fixed bug in set size routine

                        hp2100_dr.c:
                        - fixed drum sizes
                        - fixed variable capacity interaction with SAVE/RESTORE

                        i1401_cpu.c: revised fetch to model hardware more closely

                        ibm1130: fixed bugs found by APL 1130

                        nova_dsk.c: fixed bug in set size routine

                        altairz80: fixed bug in real-time clock on Windows host

  0     15-Jun-03       scp.c:
                        - added ASSIGN/DEASSIGN
                        - changed RESTORE to detach files
                        - added u5, u6 unit fields
                        - added USE_ADDR64 support
                        - changed some structure fields to unsigned

                        scp_tty.c: added extended file seek

                        sim_sock.c: fixed calling sequence in stubs

                        sim_tape.c:
                        - added E11 and TPC format support
                        - added extended file support

                        sim_tmxr.c: fixed bug in SHOW CONNECTIONS

                        all magtapes:
                        - added multiformat support
                        - added extended file support

                        i1401_cpu.c:
                        - fixed mnemonic, instruction lengths, and reverse
                           scan length check bug for MCS
                        - fixed MCE bug, BS off by 1 if zero suppress
                        - fixed chaining bug, D lost if return to SCP
                        - fixed H branch, branch occurs after continue
                        - added check for invalid 8 character MCW, LCA

                        i1401_mt.c: fixed load-mode end of record response

                        nova_dsk.c: fixed variable size interaction with restore

                        pdp1_dt.c: fixed variable size interaction with restore

                        pdp10_rp.c: fixed ordering bug in attach

                        pdp11_cpu.c:
                        - fixed bug in MMR1 update (found by Tim Stark)
                        - fixed bug in memory size table

                        pdp11_lp.c, pdp11_rq.c: added extended file support

                        pdp11_rl.c, pdp11_rp.c, pdp11_ry.c: fixed ordering bug in attach

                        pdp11_tc.c: fixed variable size interaction with restore

                        pdp11_xq.c:
                        - corrected interrupts on IE state transition (code by Tom Evans)
                        - added interrupt clear on soft reset (first noted by Bob Supnik)
                        - removed interrupt when setting XL or RL (multiple people)
                        - added SET/SHOW XQ STATS
                        - added SHOW XQ FILTERS
                        - added ability to split received packet into multiple buffers
                        - added explicit runt & giant packet processing

                        vax_fpa.c:
                        - fixed integer overflow bug in CVTfi
                        - fixed multiple bugs in EMODf

                        vax_io.c: optimized byte and word DMA routines

                        vax_sysdev.c:
                        - added calibrated delay to ROM reads (from Mark Pizzolato)
                        - fixed calibration problems in interval timer (from Mark Pizzolato)

                        pdp1_dt.c: fixed variable size interaction with restore

                        pdp18b_dt.c: fixed variable size interaction with restore

                        pdp18b_mt.c: fixed bug in MTTR

                        pdp18b_rf.c: fixed variable size interaction with restore

                        pdp8_df.c, pdp8_rf.c: fixed variable size interaction
                        with restore

                        pdp8_dt.c: fixed variable size interaction with restore

                        pdp8_mt.c: fixed bug in SKTR

                        hp2100_dp.c,hp2100_dq.c:
                        - fixed bug in read status (13210A controller)
                        - fixed bug in seek completion

                        id_pt.c: fixed type declaration (found by Mark Pizzolato)

                        gri_cpu.c: fixed bug in SC queue pointer management

/* V2.10 revision history

  4     03-Mar-03       scp.c
                        - added .ini startup file capability
                        - added multiple breakpoint actions
                        - added multiple switch evaluation points
                        - fixed bug in multiword deposits to file

                        sim_tape.c: magtape simulation library

                        h316_stddev.c: added set line frequency command

                        hp2100_mt.c, hp2100_ms.c: revised to use magtape library

                        i1401_mt.c: revised to use magtape library

                        id_dp.c, id_idc.c: fixed cylinder overflow on writes

                        id_mt.c:
                        - fixed error handling to stop selector channel
                        - revised to use magtape library

                        id16_sys.c, id32_sys.c: added relative addressing support

                        id_uvc.c:
                        - added set frequency command to line frequency clock
                        - improved calibration algorithm for precision clock

                        nova_clk.c: added set line frequency command

                        nova_dsk.c: fixed autosizing algorithm

                        nova_mt.c: revised to use magtape library

                        pdp10_tu.c: revised to use magtape library

                        pdp11_cpu.c: fixed bug in MMR1 update (found by Tim Stark)

                        pdp11_stddev.c
                        - added set line frequency command
                        - added set ctrl-c command

                        pdp11_rq.c:
                        - fixed ordering problem in queue process
                        - fixed bug in vector calculation for VAXen
                        - added user defined drive support

                        pdp11_ry.c: fixed autosizing algorithm

                        pdp11_tm.c, pdp11_ts.c: revised to use magtape library

                        pdp11_tq.c:
                        - fixed ordering problem in queue process
                        - fixed overly restrictive test for bad modifiers
                        - fixed bug in vector calculation for VAXen
                        - added variable controller, user defined drive support
                        - revised to use magtape library

                        pdp18b_cpu.c: fixed three EAE bugs (found by Hans Pufal)

                        pdp18b_mt.c:
                        - fixed bugs in BOT error handling, interrupt handling
                        - revised to use magtape library

                        pdp18b_rf.c:
                        - removed 22nd bit from disk address
                        - fixed autosizing algorithm

                        pdp18b_stddev.c:
                        - added set line frequency command
                        - added set ctrl-c command

                        pdp18b_sys.c: fixed FMTASC printouts (found by Hans Pufal)

                        pdp8_clk.c: added set line frequency command

                        pdp8_df.c, pdp8_rf.c, pdp8_rx.c: fixed autosizing algorithm

                        pdp8_mt.c:
                        - fixed bug in BOT error handling
                        - revised to use magtape library

                        pdp8_tt.c: added set ctrl-c command

                        sds_cpu.c: added set line frequency command

                        sds_mt.c: revised to use magtape library

                        vax_stddev.c: added set ctrl-c command

  3     06-Feb-03       scp.c:
                        - added dynamic extension of the breakpoint table
                        - added breakpoint actions

                        hp2100_cpu.c: fixed last cycle bug in DMA output (found by
                        Mike Gemeny)

                        hp2100_ipl.c: individual links are full duplex (found by
                        Mike Gemeny)

                        pdp11_cpu.c: changed R, SP to track PSW<rs,cm> respectively

                        pdp18b_defs.h, pdp18b_sys.c: added RB09 fixed head disk,
                        LP09 printer

                        pdp18b_rf.c:
                        - fixed IOT decoding (found by Hans Pufal)
                        - fixed address overrun logic
                        - added variable number of platters and autosizing

                        pdp18b_rf.c:
                        - fixed IOT decoding
                        - fixed bug in command initiation

                        pdp18b_rb.c: new RB09 fixed head disk

                        pdp18b_lp.c: new LP09 line printer

                        pdp8_df.c: added variable number of platters and autosizing

                        pdp8_rf.c: added variable number of platters and autosizing

                        nova_dsk.c: added variable number of platters and autosizing

                        id16_cpu.c: fixed bug in SETM, SETMR (found by Mark Pizzolato)

  2     15-Jan-03       scp.c:
                        - added dynamic memory size flag and RESTORE support
                        - added EValuate command
                        - added get_ipaddr routine
                        - added ! (OS command) feature (from Mark Pizzolato)
                        - added BREAK support to sim_poll_kbd (from Mark Pizzolato)

                        sim_tmxr.c:
                        - fixed bugs in IAC+IAC handling (from Mark Pizzolato)
                        - added IAC+BRK handling (from Mark Pizzolato)

                        sim_sock.c:
                        - added use count for Windows start/stop
                        - added sim_connect_sock

                        pdp1_defs.h, pdp1_cpu.c, pdp1_sys.c, pdp1_drm.c:
                        added Type 24 serial drum

                        pdp18_defs.h: added PDP-4 drum support

                        hp2100_cpu.c: added 21MX IOP support

                        hp2100_ipl.c: added HP interprocessor link support

                        pdp11_tq.c: fixed bug in transfer end packet length

                        pdp11_xq.c:
                        - added VMScluster support (thanks to Mark Pizzolato)
                        - added major performance enhancements (thanks to Mark Pizzolato)
                        - added local packet processing
                        - added system id broadcast

                        pdp11_stddev.c: changed default to 7b (for early UNIX)

                        vax_cpu.c, vax_io.c, vax_stddev.c, vax_sysdev.c:
                        added console halt capability (from Mark Pizzolato)

                        all terminals and multiplexors: added BREAK support

  1     21-Nov-02       pdp1_stddev.c: changed typewriter to half duplex
                        (found by Derek Peschel)

                        pdp10_tu.c:
                        - fixed bug in bootstrap (reported by Michael Thompson)
                        - fixed bug in read (reported by Harris Newman)

  0     15-Nov-02       SCP and libraries
                        scp.c:
                        - added Telnet console support
                        - removed VT emulation support
                        - added support for statically buffered devices
                        - added HELP <command>
                        - fixed bugs in set_logon, ssh_break (found by David Hittner)
                        - added VMS file optimization (from Robert Alan Byer)
                        - added quiet mode, DO with parameters, GUI interface,
                           extensible commands (from Brian Knittel)
                        - added DEVICE context and flags
                        - added central device enable/disable support
                        - modified SAVE/GET to save and restore flags
                        - modified boot routine calling sequence
                        scp_tty.c:
                        - removed VT emulation support
                        - added sim_os_sleep, renamed sim_poll_kbd, sim_putchar
                        sim_tmxr.c:
                        - modified for Telnet console support
                        - fixed bug in binary (8b) support
                        sim_sock.c: modified for Telnet console support
                        sim_ether.c: new library for Ethernet (from David Hittner)

                        all magtapes:
                        - added support for end of medium
                        - cleaned up BOT handling

                        all DECtapes: added support for RT11 image file format

                        most terminals and multiplexors:
                        - added support for 7b vs 8b character processing

                        PDP-1
                        pdp1_cpu.c, pdp1_sys.c, pdp1_dt.c: added PDP-1 DECtape support

                        PDP-8
                        pdp8_cpu.c, all peripherals:
                        - added variable device number support
                        - added new device enabled/disable support
                        pdp8_rx.c: added RX28/RX02 support

                        PDP-11
                        pdp11_defs.h, pdp11_io.c, pdp11_sys.c, all peripherals:
                        - added variable vector support
                        - added new device enable/disable support
                        - added autoconfiguration support
                        all bootstraps: modified to support variable addresses
                        dec_mscp.h, pdp11_tq.c: added TK50 support
                        pdp11_rq.c:
                        - added multicontroller support
                        - fixed bug in HBE error log packet
                        - fixed bug in ATP processing
                        pdp11_ry.c: added RX211/RX02 support
                        pdp11_hk.c: added RK611/RK06/RK07 support
                        pdp11_tq.c: added TMSCP support
                        pdp11_xq.c: added DEQNA/DELQA support (from David Hittner)
                        pdp11_pclk.c: added KW11P support
                        pdp11_ts.c:
                        - fixed bug in CTL decoding
                        - fixed bug in extended status XS0_MOT
                        pdp11_stddev.c: removed paper tape to its own module

                        PDP-18b
                        pdp18b_cpu.c, all peripherals:
                        - added variable device number support
                        - added new device enabled/disabled support

                        VAX
                        dec_dz.h: fixed bug in number of boards calculation
                        vax_moddefs.h, vax_io.c, vax_sys.c, all peripherals:
                        - added variable vector support
                        - added new device enable/disable support
                        - added autoconfiguration support
                        vax_sys.c:
                        - generalized examine/deposit
                        - added TMSCP, multiple RQDX3, DEQNA/DELQA support
                        vax_stddev.c: removed paper tape, now uses PDP-11 version
                        vax_sysdev.c:
                        - allowed NVR to be attached to file
                        - removed unused variables (found by David Hittner)

                        PDP-10
                        pdp10_defs.h, pdp10_ksio.c, all peripherals:
                        - added variable vector support
                        - added new device enable/disable support
                        pdp10_defs.h, pdp10_ksio.c: added support for standard PDP-11
                           peripherals, added RX211 support
                        pdp10_pt.c: rewritten to reference common implementation

                        Nova, Eclipse:
                        nova_cpu.c, eclipse_cpu.c, all peripherals:
                        - added new device enable/disable support

                        HP2100
                        hp2100_cpu:
                        - fixed bugs in the EAU, 21MX, DMS, and IOP instructions
                        - fixed bugs in the memory protect and DMS functions
                        - created new options to enable/disable EAU, MPR, DMS
                        - added new device enable/disable support
                        hp2100_fp.c:
                        - recoded to conform to 21MX microcode algorithms
                        hp2100_stddev.c:
                        - fixed bugs in TTY reset, OTA, time base generator
                        - revised BOOT support to conform to RBL loader
                        - added clock calibration
                        hp2100_dp.c:
                        - changed default to 13210A
                        - added BOOT support
                        hp2100_dq.c:
                        - finished incomplete functions, fixed head switching
                        - added BOOT support
                        hp2100_ms.c:
                        - fixed bugs found by diagnostics
                        - added 13183 support
                        - added BOOT support
                        hp2100_mt.c:
                        - fixed bugs found by diagnostics
                        - disabled by default
                        hp2100_lpt.c: implemented 12845A controller
                        hp2100_lps.c:
                        - renamed 12653A controller
                        - added diagnostic mode for MPR, DCPC diagnostics
                        - disabled by default

                        IBM 1620: first release

/* V2.9 revision history

  11    20-Jul-02       i1401_mt.c: on read, end of record stores group mark
                           without word mark (found by Van Snyder)

                        i1401_dp.c: reworked address generation and checking

                        vax_cpu.c: added infinite loop detection and halt to
                           boot ROM option (from Mark Pizzolato)

                        vax_fpa.c: changed function names to prevent conflict
                           with C math library

                        pdp11_cpu.c: fixed bug in MMR0 update logic (from
                           John Dundas)

                        pdp18b_stddev.c: added "ASCII mode" for reader and
                           punch (from Hans Pufal)

                        gri_*.c: added GRI-909 simulator

                        scp.c: added DO echo, DO exit (from Brian Knittel)

                        scp_tty.c: added Windows priority hacking (from
                           Mark Pizzolato)

  10    15-Jun-02       scp.c: fixed error checking on calls to fxread/fxwrite
                           (found by Norm Lastovic)

                        scp_tty.c, sim_vt.h, sim_vt.c: added VTxxx emulation
                           support for Windows (from Fischer Franz)

                        sim_sock.c: added OS/2 support (from Holger Veit)

                        pdp11_cpu.c: fixed bugs (from John Dundas)
                        - added special case for PS<15:12> = 1111 to MFPI
                        - removed special case from MTPI
                        - added masking of relocation adds 

                        i1401_cpu.c:
                        - added multiply/divide
                        - fixed bugs (found by Van Snyder)
                           o 5 and 7 character H, 7 character doesn't branch
                           o 8 character NOP
                           o 1401-like memory dump

                        i1401_dp.c: added 1311 disk

  9     04-May-02       pdp11_rq: fixed bug in polling routine

  8     03-May-02       scp.c:
                        - changed LOG/NOLOG to SET LOG/NOLOG
                        - added SHOW LOG
                        - added SET VT/NOVT and SHOW VT for VT emulation
  
                        sim_sock.h: changed VMS stropt.h include to ioctl.h

                        vax_cpu.c
                        - added TODR powerup routine to set date, time on boot
                        - fixed exception flows to clear trap request
                        - fixed register logging in autoincrement indexed

                        vax_stddev.c: added TODR powerup routine
                        
                        vax_cpu1.c: fixed exception flows to clear trap request

  7     30-Apr-02       scp.c: fixed bug in clock calibration when (real) clock
                           jumps forward due too far (found by Jonathan Engdahl)
  
                        pdp11_cpu.c: fixed bugs, added features (from John Dundas
                           and Wolfgang Helbig)
                        - added HTRAP and BPOK to maintenance register
                        - added trap on kernel HALT if MAINT<HTRAP> set
                        - fixed red zone trap, clear odd address and nxm traps
                        - fixed RTS SP, don't increment restored SP
                        - fixed TSTSET, write dst | 1 rather than prev R0 | 1
                        - fixed DIV, set N=0,Z=1 on div by zero (J11, 11/70)
                        - fixed DIV, set set N=Z=0 on overfow (J11, 11/70)
                        - fixed ASH, ASHC, count = -32 used implementation-
                           dependent 32 bit right shift
                        - fixed illegal instruction test to detect 000010
                        - fixed write-only page test

                        pdp11_rp.c: fixed SHOW ADDRESS command

                        vaxmod_defs.h: fixed DZ vector base and number of lines

                        dec_dz.h:
                        - fixed interrupt acknowledge routines
                        - fixed SHOW ADDRESS command

                        all magtape routines: added test for badly formed
                           record length (suggested by Jonathan Engdahl)

  6     18-Apr-02       vax_cpu.c: fixed CASEL condition codes

                        vax_cpu1.c: fixed vfield pos > 31 test to be unsigned

                        vax_fpu.c: fixed EDIV overflow test for 0 quotient

  5     14-Apr-02       vax_cpu1.c:
                        - fixed interrupt, prv_mode set to 0 (found by Tim Stark)
                        - fixed PROBEx to mask mode to 2b (found by Kevin Handy)

  4     1-Apr-02        pdp11_rq.c: fixed bug, reset cleared write protect status

                        pdp11_ts.c: fixed bug in residual frame count after space

  3     15-Mar-02       pdp11_defs.h: changed default model to KDJ11A (11/73)

                        pdp11_rq.c: adjusted delays for M+ timing bugs

                        hp2100_cpu.c, pdp10_cpu.c, pdp11_cpu.c: tweaked abort
                           code for ANSI setjmp/longjmp compliance

                        hp2100_cpu.c, hp2100_fp.c, hp2100_stddev.c, hp2100_sys.c:
                           revised to allocate memory dynamically

  2     01-Mar-02       pdp11_cpu.c:
                        - fixed bugs in CPU registers
                        - fixed double operand evaluation order for M+

                        pdp11_rq.c: added delays to initialization for
                           RSX11M+ prior to V4.5

  1     20-Feb-02       scp.c: fixed bug in clock calibration when (real)
                        time runs backwards

                        pdp11_rq.c: fixed bug in host timeout logic

                        pdp11_ts.c: fixed bug in message header logic

                        pdp18b_defs.h, pdp18b_dt.c, pdp18b_sys.c: added
                           PDP-7 DECtape support

                        hp2100_cpu.c:
                        - added floating point and DMS
                        - fixed bugs in DIV, ASL, ASR, LBT, SBT, CBT, CMW

                        hp2100_sys.c: added floating point, DMS

                        hp2100_fp.c: added floating point

                        ibm1130: added Brian Knittel's IBM 1130 simulator

  0     30-Jan-02       scp.c:
                        - generalized timer package for multiple timers
                        - added circular register arrays
                        - fixed bugs, line spacing in modifier display
                        - added -e switch to attach
                        - moved device enable/disable to simulators

                        scp_tty.c: VAX specific fix (from Robert Alan Byer)

                        sim_tmxr.c, sim_tmxr.h:
                        - added tmxr_fstats, tmxr_dscln
                        - renamed tmxr_fstatus to tmxr_fconns

                        sim_sock.c, sim_sock.h: added VMS support (from
                        Robert Alan Byer)

                        pdp_dz.h, pdp18b_tt1.c, nova_tt1.c:
                        - added SET DISCONNECT
                        - added SHOW STATISTICS

                        pdp8_defs.h: fixed bug in interrupt enable initialization

                        pdp8_ttx.c: rewrote as unified multiplexor

                        pdp11_cpu.c: fixed calc_MMR1 macro (found by Robert Alan Byer)

                        pdp11_stddev.c: fixed bugs in KW11L (found by John Dundas)

                        pdp11_rp.c: fixed bug in 18b mode boot

                        pdp11 bootable I/O devices: fixed register setup at boot
                           exit (found by Doug Carman)

                        hp2100_cpu.c:
                        - fixed DMA register tables (found by Bill McDermith)
                        - fixed SZx,SLx,RSS bug (found by Bill McDermith)
                        - fixed flop restore logic (found by Bill McDermith)

                        hp2100_mt.c: fixed bug on write of last character

                        hp2100_dq,dr,ms,mux.c: added new disk, magtape, and terminal
                           multiplexor controllers

                        i1401_cd.c, i1401_mt.c: new zero footprint bootstraps
                           (from Van Snyder)

                        i1401_sys.c: fixed symbolic display of H, NOP with no trailing
                           word mark (found by Van Snyder)

                        most CPUs:
                        - replaced OLDPC with PC queue
                        - implemented device enable/disable locally

   V2.8 revision history

5       25-Dec-01       scp.c: fixed bug in DO command (found by John Dundas)

                        pdp10_cpu.c:
                        - moved trap-in-progress to separate variable
                        - cleaned up declarations
                        - cleaned up volatile state for GNU C longjmp

                        pdp11_cpu.c: cleaned up declarations
  
                        pdp11_rq.c: added RA-class disks

4       17-Dec-01       pdp11_rq.c: added delayed processing of packets

3       16-Dec-01       pdp8_cpu.c:
                        - mode A EAE instructions didn't clear GTF
                        - ASR shift count > 24 mis-set GTF
                        - effective shift count == 32 didn't work

2       07-Dec-01       scp.c: added breakpoint package

                        all CPU's: revised to use new breakpoint package

1       05-Dec-01       scp.c: fixed bug in universal register name logic

0       30-Nov-01       Reorganized simh source and documentation tree

                        scp: Added DO command, universal registers, extended
                           SET/SHOW logic

                        pdp11: overhauled PDP-11 for DMA map support, shared
                           sources with VAX, dynamic buffer allocation

                        18b pdp: overhauled interrupt structure

                        pdp8: added RL8A

                        pdp10: fixed two ITS-related bugs (found by Dave Conroy)

   V2.7 revision history

patch   date            module(s) and fix(es)

15      23-Oct-01       pdp11_rp.c, pdp10_rp.c, pdp10_tu.c: fixed bugs
                           error interrupt handling

                        pdp10_defs.h, pdp10_ksio.c, pdp10_fe.c, pdp10_fe.c,
                        pdp10_rp.c, pdp10_tu.c: reworked I/O page interface
                           to use symbolic base addresses and lengths

14      20-Oct-01       dec_dz.h, sim_tmxr_h, sim_tmxr.c: fixed bug in Telnet
                           state handling (found by Thord Nilson), removed
                           tmxr_getchar, added tmxr_rqln and tmxr_tqln

13      18-Oct-01       pdp11_tm.c: added stub diagnostic register clock
                           for RSTS/E (found by Thord Nilson)

12      15-Oct-01       pdp11_defs.h, pdp11_cpu.c, pdp11_tc.c, pdp11_ts.c,
                           pdp11_rp.c: added operations logging

11      8-Oct-01        scp.c: added sim_rev.h include and version print

                        pdp11_cpu.c: fixed bug in interrupt acknowledge,
                           multiple outstanding interrupts caused the lowest
                           rather than the highest to be acknowledged

10      7-Oct-01        pdp11_stddev.c: added monitor bits (CSR<7>) for full
                           KW11L compatibility, needed for RSTS/E autoconfiguration

9       6-Oct-01        pdp11_rp.c, pdp10_rp.c, pdp10_tu.c: rewrote interrupt
                           logic from RH11/RH70 schematics, to mimic hardware quirks

                        dec_dz.c: fixed bug in carrier detect logic, carrier
                           detect was being cleared on next modem poll

8       4-Oct-01        pdp11_rp.c, pdp10_rp.c, pdp10_tu.c: undid edit of
                           28-Sep-01; real problem was level-sensitive nature of
                           CS1_SC, but CS1_SC can only trigger an interrupt if
                           DONE is set

7       2-Oct-01        pdp11_rp.c, pdp10_rp.c: CS1_SC is evaluated as a level-
                           sensitive, rather than an edge-sensitive, input to
                           interrupt request

6       30-Sep-01       pdp11_rp.c, pdp10_rp.c: separated out CS1<5:0> to per-
                           drive registers

                        pdp10_tu.c: based on above, cleaned up handling of
                           non-existent formatters, fixed non-data transfer
                           commands clearing DONE

5       28-Sep-01       pdp11_rp.c, pdp10_rp.c, pdp10_tu.c: controller should
                           interrupt if ATA or SC sets when IE is set, was
                           interrupting only if DON = 1 as well

4       27-Sep-01       pdp11_ts.c:
                        - NXM errors should return TC4 or TC5; were returning TC3
                        - extended features is part of XS2; was returned in XS3
                        - extended characteristics (fifth) word needed for RSTS/E

                        pdp11_tc.c: stop, stop all do cause an interrupt

                        dec_dz.h: scanner should find a ready output line, even
                           if there are no connections; needed for RSTS/E autoconfigure

                        scp.c:
                        - added routine sim_qcount for 1130
                        - added "simulator exit" detach routine for 1130

                        sim_defs.h: added header for sim_qcount

3       20-Sep-01       pdp11_ts.c: boot code binary was incorrect

2       19-Sep-01       pdp18b_cpu.c: EAE should interpret initial count of 00
                           as 100

                        scp.c: modified Macintosh support

1       17-Sep-01       pdp8_ttx.c: new module for PDP-8 multi-terminal support

                        pdp18b_tt1.c: modified to use sim_tmxr library

                        nova_tt1.c: modified to use sim_tmxr library

                        dec_dz.h: added autodisconnect support

                        scp.c: removed old multiconsole support

                        sim_tmxr.c: modified calling sequence for sim_putchar_ln

                        sim_sock.c: added Macintosh sockets support
*/

#endif
