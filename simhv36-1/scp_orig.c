/* scp.c: simulator control program

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

   14-Jul-06    RMS     Added sim_activate_abs
   14-Feb-06    RMS     Upgraded save file format to V3.5
   18-Jan-06    RMS     Added fprint_stopped_gen
                        Added breakpoint spaces
                        Fixed unaligned register access (found by Doug Carman)
   22-Sep-05    RMS     Fixed declarations (from Sterling Garwood)
   30-Aug-05    RMS     Revised to trim trailing spaces on file names
   25-Aug-05    RMS     Added variable default device support
   23-Aug-05    RMS     Added Linux line history support
   16-Aug-05    RMS     Fixed C++ declaration and cast problems
   01-May-05    RMS     Revised syntax for SET DEBUG (from Dave Bryan)
   22-Mar-05    JDB     Modified DO command to allow ten-level nesting
   18-Mar-05    RMS     Moved DETACH tests into detach_unit (from Dave Bryan)
                        Revised interface to fprint_sym, fparse_sym
   07-Feb-05    RMS     Added ASSERT command (from Dave Bryan)
   02-Feb-05    RMS     Fixed bug in global register search
   26-Dec-04    RMS     Qualified SAVE examine, RESTORE deposit with SIM_SW_REST
   10-Nov-04    JDB     Fixed logging of errors from cmds in "do" file
   05-Nov-04    RMS     Moved SET/SHOW DEBUG under CONSOLE hierarchy
                        Renamed unit OFFLINE/ONLINE to DISABLED/ENABLED (from Dave Bryan)
                        Revised to flush output files after simulation stop (from Dave Bryan)
   15-Oct-04    RMS     Fixed HELP to suppress duplicate descriptions
   27-Sep-04    RMS     Fixed comma-separation options in set (from David Bryan)
   09-Sep-04    RMS     Added -p option for RESET
   13-Aug-04    RMS     Qualified RESTORE detach with SIM_SW_REST
   17-Jul-04    RMS     Added ECHO command (from Dave Bryan)
   12-Jul-04    RMS     Fixed problem ATTACHing to read only files
                        (found by John Dundas)
   28-May-04    RMS     Added SET/SHOW CONSOLE
   14-Feb-04    RMS     Updated SAVE/RESTORE (V3.2)
                RMS     Added debug print routines (from Dave Hittner)
                RMS     Added sim_vm_parse_addr and sim_vm_fprint_addr
                RMS     Added REG_VMAD support
                RMS     Split out libraries
                RMS     Moved logging function to SCP
                RMS     Exposed step counter interface(s)
                RMS     Fixed double logging of SHOW BREAK (found by Mark Pizzolato)
                RMS     Fixed implementation of REG_VMIO
                RMS     Added SET/SHOW DEBUG, SET/SHOW <device> DEBUG,
                        SHOW <device> MODIFIERS, SHOW <device> RADIX
                RMS     Changed sim_fsize to take uptr argument
   29-Dec-03    RMS     Added Telnet console output stall support
   01-Nov-03    RMS     Cleaned up implicit detach on attach/restore
                        Fixed bug in command line read while logging (found by Mark Pizzolato)
   01-Sep-03    RMS     Fixed end-of-file problem in dep, idep
                        Fixed error on trailing spaces in dep, idep
   15-Jul-03    RMS     Removed unnecessary test in reset_all
   15-Jun-03    RMS     Added register flag REG_VMIO
   25-Apr-03    RMS     Added extended address support (V3.0)
                        Fixed bug in SAVE (found by Peter Schorn)
                        Added u5, u6 fields
                        Added logical name support
   03-Mar-03    RMS     Added sim_fsize
   27-Feb-03    RMS     Fixed bug in multiword deposits to files
   08-Feb-03    RMS     Changed sim_os_sleep to void, match_ext to char*
                        Added multiple actions, .ini file support
                        Added multiple switch evaluations per line
   07-Feb-03    RMS     Added VMS support for ! (from Mark Pizzolato)
   01-Feb-03    RMS     Added breakpoint table extension, actions
   14-Jan-03    RMS     Added missing function prototypes
   10-Jan-03    RMS     Added attach/restore flag, dynamic memory size support,
                        case sensitive SET options
   22-Dec-02    RMS     Added ! (OS command) feature (from Mark Pizzolato)
   17-Dec-02    RMS     Added get_ipaddr
   02-Dec-02    RMS     Added EValuate command
   16-Nov-02    RMS     Fixed bug in register name match algorithm
   13-Oct-02    RMS     Fixed Borland compiler warnings (found by Hans Pufal)
   05-Oct-02    RMS     Fixed bugs in set_logon, ssh_break (found by David Hittner)
                        Added support for fixed buffer devices
                        Added support for Telnet console, removed VT support
                        Added help <command>
                        Added VMS file optimizations (from Robert Alan Byer)
                        Added quiet mode, DO with parameters, GUI interface,
                           extensible commands (from Brian Knittel)
                        Added device enable/disable commands
   14-Jul-02    RMS     Fixed exit bug in do, added -v switch (from Brian Knittel)
   17-May-02    RMS     Fixed bug in fxread/fxwrite error usage (found by
                        Norm Lastovic)
   02-May-02    RMS     Added VT emulation interface, changed {NO}LOG to SET {NO}LOG
   22-Apr-02    RMS     Fixed laptop sleep problem in clock calibration, added
                        magtape record length error (found by Jonathan Engdahl)
   26-Feb-02    RMS     Fixed initialization bugs in do_cmd, get_aval
                        (found by Brian Knittel)
   10-Feb-02    RMS     Fixed problem in clock calibration
   06-Jan-02    RMS     Moved device enable/disable to simulators
   30-Dec-01    RMS     Generalized timer packaged, added circular arrays
   19-Dec-01    RMS     Fixed DO command bug (found by John Dundas)
   07-Dec-01    RMS     Implemented breakpoint package
   05-Dec-01    RMS     Fixed bug in universal register logic
   03-Dec-01    RMS     Added read-only units, extended SET/SHOW, universal registers
   24-Nov-01    RMS     Added unit-based registers
   16-Nov-01    RMS     Added DO command
   28-Oct-01    RMS     Added relative range addressing
   08-Oct-01    RMS     Added SHOW VERSION
   30-Sep-01    RMS     Relaxed attach test in BOOT
   27-Sep-01    RMS     Added queue count routine, fixed typo in ex/mod
   17-Sep-01    RMS     Removed multiple console support
   07-Sep-01    RMS     Removed conditional externs on function prototypes
                        Added special modifier print
   31-Aug-01    RMS     Changed int64 to t_int64 for Windoze (V2.7)
   18-Jul-01    RMS     Minor changes for Macintosh port
   12-Jun-01    RMS     Fixed bug in big-endian I/O (found by Dave Conroy)
   27-May-01    RMS     Added multiple console support
   16-May-01    RMS     Added logging
   15-May-01    RMS     Added features from Tim Litt
   12-May-01    RMS     Fixed missing return in disable_cmd
   25-Mar-01    RMS     Added ENABLE/DISABLE
   14-Mar-01    RMS     Revised LOAD/DUMP interface (again)
   05-Mar-01    RMS     Added clock calibration support
   05-Feb-01    RMS     Fixed bug, DETACH buffered unit with hwmark = 0
   04-Feb-01    RMS     Fixed bug, RESTORE not using device's attach routine
   21-Jan-01    RMS     Added relative time
   22-Dec-00    RMS     Fixed find_device for devices ending in numbers
   08-Dec-00    RMS     V2.5a changes
   30-Oct-00    RMS     Added output file option to examine
   11-Jul-99    RMS     V2.5 changes
   13-Apr-99    RMS     Fixed handling of 32b addresses
   04-Oct-98    RMS     V2.4 changes
   20-Aug-98    RMS     Added radix commands
   05-Jun-98    RMS     Fixed bug in ^D handling for UNIX
   10-Apr-98    RMS     Added switches to all commands
   26-Oct-97    RMS     Added search capability
   25-Jan-97    RMS     Revised data types
   23-Jan-97    RMS     Added bi-endian I/O
   06-Sep-96    RMS     Fixed bug in variable length IEXAMINE
   16-Jun-96    RMS     Changed interface to parse/print_sym
   06-Apr-96    RMS     Added error checking in reset all
   07-Jan-96    RMS     Added register buffers in save/restore
   11-Dec-95    RMS     Fixed ordering bug in save/restore
   22-May-95    RMS     Added symbolic input
   13-Apr-95    RMS     Added symbolic printouts
*/

/* Macros and data structures */

#include "sim_defs.h"
#include "sim_rev.h"
#include <signal.h>
#include <ctype.h>

#if defined (HAVE_READLINE)
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define EX_D            0                               /* deposit */
#define EX_E            1                               /* examine */
#define EX_I            2                               /* interactive */
#define SCH_OR          0                               /* search logicals */
#define SCH_AND         1
#define SCH_XOR         2
#define SCH_E           0                               /* search booleans */
#define SCH_N           1
#define SCH_G           2
#define SCH_L           3
#define SCH_EE          4
#define SCH_NE          5
#define SCH_GE          6
#define SCH_LE          7
#define SSH_ST          0                               /* set */
#define SSH_SH          1                               /* show */
#define SSH_CL          2                               /* clear */

#define DO_NEST_LVL     10                              /* DO cmd nesting level */
#define SRBSIZ          1024                            /* save/restore buffer */
#define SIM_BRK_INILNT  4096                            /* bpt tbl length */
#define SIM_BRK_ALLTYP  0xFFFFFFFF
#define UPDATE_SIM_TIME(x) sim_time = sim_time + (x - sim_interval); \
    sim_rtime = sim_rtime + ((uint32) (x - sim_interval)); \
    x = sim_interval

#define SZ_D(dp) (size_map[((dp)->dwidth + CHAR_BIT - 1) / CHAR_BIT])
#define SZ_R(rp) \
    (size_map[((rp)->width + (rp)->offset + CHAR_BIT - 1) / CHAR_BIT])
#if defined (USE_INT64)
#define SZ_LOAD(sz,v,mb,j) \
    if (sz == sizeof (uint8)) v = *(((uint8 *) mb) + ((uint32) j)); \
    else if (sz == sizeof (uint16)) v = *(((uint16 *) mb) + ((uint32) j)); \
    else if (sz == sizeof (uint32)) v = *(((uint32 *) mb) + ((uint32) j)); \
    else v = *(((t_uint64 *) mb) + ((uint32) j));
#define SZ_STORE(sz,v,mb,j) \
    if (sz == sizeof (uint8)) *(((uint8 *) mb) + j) = (uint8) v; \
    else if (sz == sizeof (uint16)) *(((uint16 *) mb) + ((uint32) j)) = (uint16) v; \
    else if (sz == sizeof (uint32)) *(((uint32 *) mb) + ((uint32) j)) = (uint32) v; \
    else *(((t_uint64 *) mb) + ((uint32) j)) = v;
#else
#define SZ_LOAD(sz,v,mb,j) \
    if (sz == sizeof (uint8)) v = *(((uint8 *) mb) + ((uint32) j)); \
    else if (sz == sizeof (uint16)) v = *(((uint16 *) mb) + ((uint32) j)); \
    else v = *(((uint32 *) mb) + ((uint32) j));
#define SZ_STORE(sz,v,mb,j) \
    if (sz == sizeof (uint8)) *(((uint8 *) mb) + ((uint32) j)) = (uint8) v; \
    else if (sz == sizeof (uint16)) *(((uint16 *) mb) + ((uint32) j)) = (uint16) v; \
    else *(((uint32 *) mb) + ((uint32) j)) = v;
#endif
#define GET_SWITCHES(cp) \
    if ((cp = get_sim_sw (cp)) == NULL) return SCPE_INVSW
#define GET_RADIX(val,dft) \
    if (sim_switches & SWMASK ('O')) val = 8; \
    else if (sim_switches & SWMASK ('D')) val = 10; \
    else if (sim_switches & SWMASK ('H')) val = 16; \
    else val = dft;

/* VM interface */

extern char sim_name[];
extern DEVICE *sim_devices[];
extern REG *sim_PC;
extern const char *sim_stop_messages[];
extern t_stat sim_instr (void);
extern t_stat sim_load (FILE *ptr, char *cptr, char *fnam, int32 flag);
extern int32 sim_emax;
extern t_stat fprint_sym (FILE *ofile, t_addr addr, t_value *val,
    UNIT *uptr, int32 sw);
extern t_stat parse_sym (char *cptr, t_addr addr, UNIT *uptr, t_value *val,
    int32 sw);

/* The per-simulator init routine is a weak global that defaults to NULL
   The other per-simulator pointers can be overrriden by the init routine */

void (*sim_vm_init) (void);
char* (*sim_vm_read) (char *ptr, int32 size, FILE *stream) = NULL;
void (*sim_vm_post) (t_bool from_scp) = NULL;
CTAB *sim_vm_cmd = NULL;
void (*sim_vm_fprint_addr) (FILE *st, DEVICE *dptr, t_addr addr) = NULL;
t_addr (*sim_vm_parse_addr) (DEVICE *dptr, char *cptr, char **tptr) = NULL;

/* Prototypes */

/* Set and show command processors */

t_stat set_dev_radix (DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr);
t_stat set_dev_enbdis (DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr);
t_stat set_dev_debug (DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr);
t_stat set_unit_enbdis (DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr);
t_stat ssh_break (FILE *st, char *cptr, int32 flg);
t_stat show_cmd_fi (FILE *ofile, int32 flag, char *cptr);
t_stat show_config (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr);
t_stat show_queue (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr);
t_stat show_time (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr);
t_stat show_mod_names (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr);
t_stat show_log_names (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr);
t_stat show_dev_radix (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr);
t_stat show_dev_debug (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr);
t_stat show_dev_logicals (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr);
t_stat show_dev_modifiers (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr);
t_stat show_version (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr);
t_stat show_break (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr);
t_stat show_device (FILE *st, DEVICE *dptr, int32 flag);
t_stat show_unit (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag);
t_stat show_all_mods (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flg);
t_stat show_one_mod (FILE *st, DEVICE *dptr, UNIT *uptr, MTAB *mptr, char *cptr, int32 flag);
t_stat sim_check_console (int32 sec);
t_stat sim_save (FILE *sfile);
t_stat sim_rest (FILE *rfile);

/* Breakpoint package */

t_stat sim_brk_init (void);
t_stat sim_brk_set (t_addr loc, int32 sw, int32 ncnt, char *act);
t_stat sim_brk_clr (t_addr loc, int32 sw);
t_stat sim_brk_clrall (int32 sw);
t_stat sim_brk_show (FILE *st, t_addr loc, int32 sw);
t_stat sim_brk_showall (FILE *st, int32 sw);
char *sim_brk_getact (char *buf, int32 size);
void sim_brk_clract (void);
void sim_brk_npc (uint32 cnt);
BRKTAB *sim_brk_new (t_addr loc);

/* Commands support routines */

SCHTAB *get_search (char *cptr, int32 radix, SCHTAB *schptr);
int32 test_search (t_value val, SCHTAB *schptr);
char *get_glyph_gen (char *iptr, char *optr, char mchar, t_bool uc);
int32 get_switches (char *cptr);
char *get_sim_sw (char *cptr);
t_stat get_aval (t_addr addr, DEVICE *dptr, UNIT *uptr);
t_value get_rval (REG *rptr, uint32 idx);
void put_rval (REG *rptr, uint32 idx, t_value val);
t_value strtotv (char *inptr, char **endptr, uint32 radix);
void fprint_help (FILE *st);
void fprint_stopped (FILE *st, t_stat r);
void fprint_capac (FILE *st, DEVICE *dptr, UNIT *uptr);
char *read_line (char *ptr, int32 size, FILE *stream);
REG *find_reg_glob (char *ptr, char **optr, DEVICE **gdptr);
char *sim_trim_endspc (char *cptr);

/* Forward references within commands */

t_stat scp_attach_unit (DEVICE *dptr, UNIT *uptr, char *cptr);
t_stat scp_detach_unit (DEVICE *dptr, UNIT *uptr);
t_bool qdisable (DEVICE *dptr);
t_stat attach_err (UNIT *uptr, t_stat stat);
t_stat detach_all (int32 start_device, t_bool shutdown);
t_stat assign_device (DEVICE *dptr, char *cptr);
t_stat deassign_device (DEVICE *dptr);
t_stat run_boot_prep (void);
t_stat exdep_reg_loop (FILE *ofile, SCHTAB *schptr, int32 flag, char *cptr,
    REG *lowr, REG *highr, uint32 lows, uint32 highs);
t_stat ex_reg (FILE *ofile, t_value val, int32 flag, REG *rptr, uint32 idx);
t_stat dep_reg (int32 flag, char *cptr, REG *rptr, uint32 idx);
t_stat exdep_addr_loop (FILE *ofile, SCHTAB *schptr, int32 flag, char *cptr,
    t_addr low, t_addr high, DEVICE *dptr, UNIT *uptr);
t_stat ex_addr (FILE *ofile, int32 flag, t_addr addr, DEVICE *dptr, UNIT *uptr);
t_stat dep_addr (int32 flag, char *cptr, t_addr addr, DEVICE *dptr,
    UNIT *uptr, int32 dfltinc);
t_stat step_svc (UNIT *ptr);
void sub_args (char *instr, char *tmpbuf, int32 maxstr, int32 nargs, char *do_arg[]);

/* Global data */

DEVICE *sim_dflt_dev = NULL;
UNIT *sim_clock_queue = NULL;
int32 sim_interval = 0;
int32 sim_switches = 0;
FILE *sim_ofile = NULL;
SCHTAB *sim_schptr = FALSE;
DEVICE *sim_dfdev = NULL;
UNIT *sim_dfunit = NULL;
int32 sim_is_running = 0;
uint32 sim_brk_summ = 0;
uint32 sim_brk_types = 0;
uint32 sim_brk_dflt = 0;
char *sim_brk_act = NULL;
BRKTAB *sim_brk_tab = NULL;
int32 sim_brk_ent = 0;
int32 sim_brk_lnt = 0;
int32 sim_brk_ins = 0;
t_bool sim_brk_pend[SIM_BKPT_N_SPC] = { FALSE };
t_addr sim_brk_ploc[SIM_BKPT_N_SPC] = { 0 };
int32 sim_quiet = 0;
int32 sim_step = 0;
static double sim_time;
static uint32 sim_rtime;
static int32 noqueue_time;
volatile int32 stop_cpu = 0;
t_value *sim_eval = NULL;
int32 sim_deb_close = 0;                                /* 1 = close debug */
FILE *sim_log = NULL;                                   /* log file */
FILE *sim_deb = NULL;                                   /* debug file */
static SCHTAB sim_stab;

static UNIT sim_step_unit = { UDATA (&step_svc, 0, 0)  };
#if defined USE_INT64
static const char *sim_si64 = "64b data";
#else
static const char *sim_si64 = "32b data";
#endif
#if defined USE_ADDR64
static const char *sim_sa64 = "64b addresses";
#else
static const char *sim_sa64 = "32b addresses";
#endif
#if defined USE_NETWORK
static const char *sim_snet = "Ethernet support";
#else
static const char *sim_snet = "no Ethernet";
#endif

/* Tables and strings */

const char save_vercur[] = "V3.5";
const char save_ver32[] = "V3.2";
const char save_ver30[] = "V3.0";
const char *scp_error_messages[] = {
    "Address space exceeded",
    "Unit not attached",
    "I/O error",
    "Checksum error",
    "Format error",
    "Unit not attachable",
    "File open error",
    "Memory exhausted",
    "Invalid argument",
    "Step expired",
    "Unknown command",
    "Read only argument",
    "Command not completed",
    "Simulation stopped",
    "Goodbye",
    "Console input I/O error",
    "Console output I/O error",
    "End of file",
    "Relocation error",
    "No settable parameters",
    "Unit already attached",
    "Hardware timer error",
    "SIGINT handler setup error",
    "Console terminal setup error",
    "Subscript out of range",
    "Command not allowed",
    "Unit disabled",
    "Read only operation not allowed",
    "Invalid switch",
    "Missing value",
    "Too few arguments",
    "Too many arguments",
    "Non-existent device",
    "Non-existent unit",
    "Non-existent register",
    "Non-existent parameter",
    "Nested DO command limit exceeded",
    "Internal error",
    "Invalid magtape record length",
    "Console Telnet connection lost",
    "Console Telnet connection timed out",
    "Console Telnet output stall",
    ""                                                  /* printed by assert */
    };

const size_t size_map[] = { sizeof (int8),
    sizeof (int8), sizeof (int16), sizeof (int32), sizeof (int32)
#if defined (USE_INT64)
    , sizeof (t_int64), sizeof (t_int64), sizeof (t_int64), sizeof (t_int64)
#endif
};

const t_value width_mask[] = { 0,
    0x1, 0x3, 0x7, 0xF,
    0x1F, 0x3F, 0x7F, 0xFF,
    0x1FF, 0x3FF, 0x7FF, 0xFFF,
    0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
    0x1FFFF, 0x3FFFF, 0x7FFFF, 0xFFFFF,
    0x1FFFFF, 0x3FFFFF, 0x7FFFFF, 0xFFFFFF,
    0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF, 0xFFFFFFF,
    0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF
#if defined (USE_INT64)
    , 0x1FFFFFFFF, 0x3FFFFFFFF, 0x7FFFFFFFF, 0xFFFFFFFFF,
    0x1FFFFFFFFF, 0x3FFFFFFFFF, 0x7FFFFFFFFF, 0xFFFFFFFFFF,
    0x1FFFFFFFFFF, 0x3FFFFFFFFFF, 0x7FFFFFFFFFF, 0xFFFFFFFFFFF,
    0x1FFFFFFFFFFF, 0x3FFFFFFFFFFF, 0x7FFFFFFFFFFF, 0xFFFFFFFFFFFF,
    0x1FFFFFFFFFFFF, 0x3FFFFFFFFFFFF, 0x7FFFFFFFFFFFF, 0xFFFFFFFFFFFFF,
    0x1FFFFFFFFFFFFF, 0x3FFFFFFFFFFFFF, 0x7FFFFFFFFFFFFF, 0xFFFFFFFFFFFFFF,
    0x1FFFFFFFFFFFFFF, 0x3FFFFFFFFFFFFFF,
    0x7FFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFF,
    0x1FFFFFFFFFFFFFFF, 0x3FFFFFFFFFFFFFFF,
    0x7FFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF
#endif
    };

static CTAB cmd_table[] = {
    { "RESET", &reset_cmd, 0,
      "r{eset} {ALL|<device>}   reset simulator\n" },
    { "EXAMINE", &exdep_cmd, EX_E,
      "e{xamine} <list>         examine memory or registers\n" },
    { "IEXAMINE", &exdep_cmd, EX_E+EX_I,
      "ie{xamine} <list>        interactive examine memory or registers\n" },
    { "DEPOSIT", &exdep_cmd, EX_D,
      "d{eposit} <list> <val>   deposit in memory or registers\n" },
    { "IDEPOSIT", &exdep_cmd, EX_D+EX_I,
      "id{eposit} <list>        interactive deposit in memory or registers\n" },
    { "EVALUATE", &eval_cmd, 0,
      "ev{aluate} <expr>        evaluate symbolic expression\n" },
    { "RUN", &run_cmd, RU_RUN,
      "ru{n} {new PC}           reset and start simulation\n" },
    { "GO", &run_cmd, RU_GO,
      "go {new PC}              start simulation\n" }, 
    { "STEP", &run_cmd, RU_STEP,
      "s{tep} {n}               simulate n instructions\n" },
    { "CONT", &run_cmd, RU_CONT,
      "c{ont}                   continue simulation\n" },
    { "BOOT", &run_cmd, RU_BOOT,
      "b{oot} <unit>            bootstrap unit\n" },
    { "BREAK", &brk_cmd, SSH_ST,
      "br{eak} <list>           set breakpoints\n" },
    { "NOBREAK", &brk_cmd, SSH_CL,
      "nobr{eak} <list>         clear breakpoints\n" },
    { "ATTACH", &attach_cmd, 0,
      "at{tach} <unit> <file>   attach file to simulated unit\n" },
    { "DETACH", &detach_cmd, 0,
      "det{ach} <unit>          detach file from simulated unit\n" },
    { "ASSIGN", &assign_cmd, 0,
      "as{sign} <device> <name> assign logical name for device\n" },
    { "DEASSIGN", &deassign_cmd, 0,
      "dea{ssign} <device>      deassign logical name for device\n" },
    { "SAVE", &save_cmd, 0,
      "sa{ve} <file>            save simulator to file\n" },
    { "RESTORE", &restore_cmd, 0,
      "rest{ore}|ge{t} <file>   restore simulator from file\n" },
    { "GET", &restore_cmd, 0, NULL },
    { "LOAD", &load_cmd, 0,
      "l{oad} <file> {<args>}   load binary file\n" },
    { "DUMP", &load_cmd, 1,
      "du(mp) <file> {<args>}   dump binary file\n" },
    { "EXIT", &exit_cmd, 0,
      "exi{t}|q{uit}|by{e}      exit from simulation\n" },
    { "QUIT", &exit_cmd, 0, NULL },
    { "BYE", &exit_cmd, 0, NULL },
    { "SET", &set_cmd, 0,
      "set console arg{,arg...} set console options\n"
      "set <dev> OCT|DEC|HEX    set device display radix\n"
      "set <dev> ENABLED        enable device\n"
      "set <dev> DISABLED       disable device\n"
      "set <dev> DEBUG{=arg}    set device debug flags\n"
      "set <dev> NODEBUG={arg}  clear device debug flags\n"
      "set <dev> arg{,arg...}   set device parameters\n"
      "set <unit> ENABLED       enable unit\n"
      "set <unit> DISABLED      disable unit\n"
      "set <unit> arg{,arg...}  set unit parameters\n"
      },
    { "SHOW", &show_cmd, 0,
      "sh{ow} br{eak} <list>    show breakpoints on address list\n"
      "sh{ow} con{figuration}   show configuration\n"
      "sh{ow} cons{ole} {arg}   show console options\n"
      "sh{ow} dev{ices}         show devices\n"  
      "sh{ow} m{odifiers}       show modifiers\n" 
      "sh{ow} n{ames}           show logical names\n" 
      "sh{ow} q{ueue}           show event queue\n"  
      "sh{ow} ti{me}            show simulated time\n"  
      "sh{ow} ve{rsion}         show simulator version\n" 
      "sh{ow} <dev> RADIX       show device display radix\n"
      "sh{ow} <dev> DEBUG       show device debug flags\n"
      "sh{ow} <dev> MODIFIERS   show device modifiers\n"
      "sh{ow} <dev} NAMES       show device logical name\n"
      "sh{ow} <dev> {arg,...}   show device parameters\n"
      "sh{ow} <unit> {arg,...}  show unit parameters\n"  },
    { "DO", &do_cmd, 1,
      "do <file> {arg,arg...}   process command file\n" },
    { "ECHO", &echo_cmd, 0,
      "echo <string>            display <string>\n" },
    { "ASSERT", &assert_cmd, 0,
      "assert {<dev>} <cond>    test simulator state against condition\n" },
    { "HELP", &help_cmd, 0,
      "h{elp}                   type this message\n"
      "h{elp} <command>         type help for command\n" },
    { "!", &spawn_cmd, 0,
      "!                        execute local command interpreter\n"
      "! <command>              execute local host command\n" },
    { NULL, NULL, 0 }
    };

/* Main command loop */

int main (int argc, char *argv[])
{
char cbuf[CBUFSIZE], gbuf[CBUFSIZE], *cptr;
int32 i, sw;
t_bool lookswitch;
t_stat stat;
CTAB *cmdp;

#if defined (__MWERKS__) && defined (macintosh)
argc = ccommand (&argv);
#endif

*cbuf = 0;                                              /* init arg buffer */
sim_switches = 0;                                       /* init switches */
lookswitch = TRUE;
for (i = 1; i < argc; i++) {                            /* loop thru args */
    if (argv[i] == NULL) continue;                      /* paranoia */
    if ((*argv[i] == '-') && lookswitch) {              /* switch? */
        if ((sw = get_switches (argv[i])) < 0) {
            fprintf (stderr, "Invalid switch %s\n", argv[i]);
            return 0;
            }
        sim_switches = sim_switches | sw;
        }
    else {
        if ((strlen (argv[i]) + strlen (cbuf) + 1) >= CBUFSIZE) {
            fprintf (stderr, "Argument string too long\n");
            return 0;
            }
        if (*cbuf) strcat (cbuf, " ");                  /* concat args */
        strcat (cbuf, argv[i]);
        lookswitch = FALSE;                             /* no more switches */
        }
    }                                                   /* end for */
sim_quiet = sim_switches & SWMASK ('Q');                /* -q means quiet */

if (sim_vm_init != NULL) (*sim_vm_init)();              /* call once only */
sim_finit ();                                           /* init fio package */
stop_cpu = 0;
sim_interval = 0;
sim_time = sim_rtime = 0;
noqueue_time = 0;
sim_clock_queue = NULL;
sim_is_running = 0;
sim_log = NULL;
if (sim_emax <= 0) sim_emax = 1;

if ((stat = sim_ttinit ()) != SCPE_OK) {
    fprintf (stderr, "Fatal terminal initialization error\n%s\n",
        scp_error_messages[stat - SCPE_BASE]);
    return 0;
    }
if ((sim_eval = (t_value *) calloc (sim_emax, sizeof (t_value))) == NULL) {
    fprintf (stderr, "Unable to allocate examine buffer\n");
    return 0;
    };
if ((stat = reset_all_p (0)) != SCPE_OK) {
    fprintf (stderr, "Fatal simulator initialization error\n%s\n",
        scp_error_messages[stat - SCPE_BASE]);
    return 0;
    }
if ((stat = sim_brk_init ()) != SCPE_OK) {
    fprintf (stderr, "Fatal breakpoint table initialization error\n%s\n",
        scp_error_messages[stat - SCPE_BASE]);
    return 0;
    }
if (!sim_quiet) {
    printf ("\n");
    show_version (stdout, NULL, NULL, 0, NULL);
    }
if (sim_dflt_dev == NULL) sim_dflt_dev = sim_devices[0]; /* if no default */

if (*cbuf) {                                            /* cmd file arg? */
    stat = do_cmd (0, cbuf);                            /* proc cmd file */
    if (stat == SCPE_OPENERR)                           /* error? */
         fprintf (stderr, "Can't open file %s\n", cbuf);
    }
else if (*argv[0]) {                                    /* sim name arg? */
    char nbuf[PATH_MAX + 7], *np;                       /* "path.ini" */
    nbuf[0] = '"';                                      /* starting " */
    strncpy (nbuf + 1, argv[0], PATH_MAX + 1);          /* copy sim name */
    if (np = match_ext (nbuf, "EXE")) *np = 0;          /* remove .exe */
    strcat (nbuf, ".ini\"");                            /* add .ini" */
    stat = do_cmd (0, nbuf);                            /* proc cmd file */
    }

while (stat != SCPE_EXIT) {                             /* in case exit */
    printf ("sim> ");                                   /* prompt */
    if (cptr = sim_brk_getact (cbuf, CBUFSIZE))         /* pending action? */
        printf ("%s\n", cptr);                          /* echo */
    else if (sim_vm_read != NULL)                       /* sim routine? */
        cptr = (*sim_vm_read) (cbuf, CBUFSIZE, stdin);
    else cptr = read_line (cbuf, CBUFSIZE, stdin);      /* read command line */
    if (cptr == NULL) continue;                         /* ignore EOF */
    if (*cptr == 0) continue;                           /* ignore blank */
    if (sim_log) fprintf (sim_log, "sim> %s\n", cptr);  /* log cmd */
    cptr = get_glyph (cptr, gbuf, 0);                   /* get command glyph */
    sim_switches = 0;                                   /* init switches */
    if (cmdp = find_cmd (gbuf))                         /* lookup command */
        stat = cmdp->action (cmdp->arg, cptr);          /* if found, exec */
    else stat = SCPE_UNK;
    if (stat >= SCPE_BASE) {                            /* error? */
        printf ("%s\n", scp_error_messages[stat - SCPE_BASE]);
        if (sim_log) fprintf (sim_log, "%s\n",
            scp_error_messages[stat - SCPE_BASE]);
        }
    if (sim_vm_post != NULL) (*sim_vm_post) (TRUE);
    }                                                   /* end while */

detach_all (0, TRUE);                                   /* close files */
sim_set_deboff (0, NULL);                               /* close debug */
sim_set_logoff (0, NULL);                               /* close log */
sim_set_notelnet (0, NULL);                             /* close Telnet */
sim_ttclose ();                                         /* close console */
return 0;
}

/* Find command routine */

CTAB *find_cmd (char *gbuf)
{
CTAB *cmdp = NULL;

if (sim_vm_cmd) cmdp = find_ctab (sim_vm_cmd, gbuf);    /* try ext commands */
if (cmdp == NULL) cmdp = find_ctab (cmd_table, gbuf);   /* try regular cmds */
return cmdp;
}

/* Exit command */

t_stat exit_cmd (int32 flag, char *cptr)
{
return SCPE_EXIT;
}

/* Help command */

void fprint_help (FILE *st)
{
CTAB *cmdp;

for (cmdp = sim_vm_cmd; cmdp && (cmdp->name != NULL); cmdp++) {
    if (cmdp->help) fprintf (st, cmdp->help);
    }
for (cmdp = cmd_table; cmdp && (cmdp->name != NULL); cmdp++) {
    if (cmdp->help && (!sim_vm_cmd || !find_ctab (sim_vm_cmd, cmdp->name)))
        fprintf (st, cmdp->help);
    }
return;
}

t_stat help_cmd (int32 flag, char *cptr)
{
char gbuf[CBUFSIZE];
CTAB *cmdp;

GET_SWITCHES (cptr);
if (*cptr) {
    cptr = get_glyph (cptr, gbuf, 0);
    if (*cptr) return SCPE_2MARG;
    if (cmdp = find_cmd (gbuf)) {
        printf (cmdp->help);
        if (sim_log) fprintf (sim_log, cmdp->help);
        }
    else return SCPE_ARG;
    }
else {
    fprint_help (stdout);
    if (sim_log) fprint_help (sim_log);
    }
return SCPE_OK;
}

/* Spawn command */

t_stat spawn_cmd (int32 flag, char *cptr)
{
if ((cptr == NULL) || (strlen (cptr) == 0)) cptr = getenv("SHELL");
if ((cptr == NULL) || (strlen (cptr) == 0)) cptr = getenv("ComSpec");
#if defined (VMS)
if ((cptr == NULL) || (strlen (cptr) == 0)) cptr = "SPAWN/INPUT=SYS$COMMAND:";
#endif
fflush(stdout);                                         /* flush stdout */
if (sim_log) fflush (sim_log);                          /* flush log if enabled */
system (cptr);
#if defined (VMS)
printf ("\n");
#endif

return SCPE_OK;
}

/* Echo command */

t_stat echo_cmd (int32 flag, char *cptr)
{
puts (cptr);
if (sim_log) fprintf (sim_log, "%s\n", cptr);
return SCPE_OK;
}

/* Do command */

t_stat do_cmd (int32 flag, char *fcptr)
{
char *cptr, cbuf[CBUFSIZE], gbuf[CBUFSIZE], *c, quote, *do_arg[10];
FILE *fpin;
CTAB *cmdp;
int32 echo, nargs;
t_stat stat = SCPE_OK;

if (flag > 0) { GET_SWITCHES (fcptr); }                 /* get switches */
else flag = 1;                                          /* start at level 1 */
echo = sim_switches & SWMASK ('V');                     /* -v means echo */

c = fcptr;
for (nargs = 0; nargs < 10; ) {                         /* extract arguments */
    while (isspace (*c)) c++;                           /* skip blanks */
    if (*c == 0) break;                                 /* all done */
    if (*c == '\'' || *c == '"') quote = *c++;          /* quoted string? */
    else quote = 0;
    do_arg[nargs++] = c;                                /* save start */
    while (*c && (quote? (*c != quote): !isspace (*c))) c++;
    if (*c) *c++ = 0;                                   /* term at quote/spc */
    }                                                   /* end for */
if (nargs <= 0) return SCPE_2FARG;                      /* need at least 1 */
if ((fpin = fopen (do_arg[0], "r")) == NULL)            /* cmd file failed to open? */
    return SCPE_OPENERR;

do {
    cptr = read_line (cbuf, CBUFSIZE, fpin);            /* get cmd line */
    sub_args (cbuf, gbuf, CBUFSIZE, nargs, do_arg);
    if (cptr == NULL) break;                            /* exit on eof */
    if (*cptr == 0) continue;                           /* ignore blank */
    if (echo) printf("do> %s\n", cptr);                 /* echo if -v */
    if (echo && sim_log) fprintf (sim_log, "do> %s\n", cptr);
    cptr = get_glyph (cptr, gbuf, 0);                   /* get command glyph */
    sim_switches = 0;                                   /* init switches */
    if (cmdp = find_cmd (gbuf)) {                       /* lookup command */
        if (cmdp->action == &do_cmd) {                  /* DO command? */
            if (flag >= DO_NEST_LVL) stat = SCPE_NEST;  /* nest too deep? */
            else stat = do_cmd (flag + 1, cptr);        /* exec DO cmd */
            }
        else stat = cmdp->action (cmdp->arg, cptr);     /* exec other cmd */
        }
    else stat = SCPE_UNK;                               /* bad cmd given */
    if (stat >= SCPE_BASE) {                            /* error? */
        printf ("%s\n", scp_error_messages[stat - SCPE_BASE]);
        if (sim_log) fprintf (sim_log, "%s\n",
            scp_error_messages[stat - SCPE_BASE]);
        }
    if (sim_vm_post != NULL) (*sim_vm_post) (TRUE);
    } while ((stat != SCPE_EXIT) && (stat != SCPE_AFAIL));

fclose (fpin);                                          /* close file */
return (stat == SCPE_EXIT)? SCPE_EXIT: SCPE_OK;
}

/* Substitute_args - replace %n tokens in 'instr' with the do command's arguments

   Calling sequence
   instr        =       input string
   tmpbuf       =       temp buffer
   maxstr       =       min (len (instr), len (tmpbuf))
   nargs        =       number of arguments
   do_arg[10]   =       arguments
*/

void sub_args (char *instr, char *tmpbuf, int32 maxstr, int32 nargs, char *do_arg[])
{
char *ip, *op, *ap, *oend = tmpbuf + maxstr - 2;

for (ip = instr, op = tmpbuf; *ip && (op < oend); ) {
    if ((*ip == '\\') && (ip[1] == '%')) {              /* \% = literal % */
        ip++;                                           /* skip \ */
        if (*ip) *op++ = *ip++;                         /* copy next */
        }
    else if ((*ip == '%') &&                            /* %n = sub */
             ((ip[1] >= '1') && (ip[1] <= ('0'+ nargs - 1)))) {
        ap = do_arg[ip[1] - '0'];
        ip = ip + 2;
        while (*ap && (op < oend)) *op++ = *ap++;       /* copy the argument */
        }
    else *op++ = *ip++;                                 /* literal character */
    }
*op = 0;                                                /* term buffer */
strcpy (instr, tmpbuf);
return;
}

/* Assert command
   
   Syntax: ASSERT {<dev>} <reg>{<logical-op><value>}<conditional-op><value>

   If <dev> is not specified, CPU is assumed.  <value> is expressed in the radix
   specified for <reg>.  <logical-op> and <conditional-op> are the same as that
   allowed for examine and deposit search specifications. */

t_stat assert_cmd (int32 flag, char *cptr)
{
char gbuf[CBUFSIZE], *gptr, *aptr, *tptr;
REG *rptr;
uint32 idx;
t_value val;
t_stat r;

aptr = cptr;                                            /* save assertion */
cptr = get_sim_opt (CMD_OPT_SW|CMD_OPT_DFT, cptr, &r);  /* get sw, default */
if (*cptr == 0) return SCPE_2FARG;                      /* must be more */
cptr = get_glyph (cptr, gbuf, 0);                       /* get register */
rptr = find_reg (gbuf, &gptr, sim_dfdev);               /* parse register */
if (!rptr) return SCPE_NXREG;                           /* not there */
if (*gptr == '[') {                                     /* subscript? */
    if (rptr->depth <= 1) return SCPE_ARG;              /* array register? */
    idx = (uint32) strtotv (++gptr, &tptr, 10);         /* convert index */
    if ((gptr == tptr) || (*tptr++ != ']')) return SCPE_ARG;
    gptr = tptr;                                        /* update */
    }
else idx = 0;                                           /* not array */
if (idx >= rptr->depth) return SCPE_SUB;                /* validate subscript */
if (*gptr != 0) get_glyph (gptr, gbuf, 0);              /* more? must be search */
else {
    if (*cptr == 0) return SCPE_2FARG;                  /* must be more */
    cptr = get_glyph (cptr, gbuf, 0);                   /* get search cond */
    }
if (*cptr != 0) return SCPE_2MARG;                      /* must be done */
if (!get_search (gbuf, rptr->radix, &sim_stab))         /* parse condition */
    return SCPE_MISVAL;
val = get_rval (rptr, idx);                             /* get register value */
if (test_search (val, &sim_stab)) return SCPE_OK;       /* test condition */
printf ("Assertion failed (%s)", aptr);                 /* report failing assertion */
if (sim_log) fprintf (sim_log, "Assertion failed (%s)", aptr);
return SCPE_AFAIL;                                      /* condition fails */
}

/* Set command */

t_stat set_cmd (int32 flag, char *cptr)
{
int32 lvl;
t_stat r;
char gbuf[CBUFSIZE], *cvptr, *svptr;
DEVICE *dptr;
UNIT *uptr;
MTAB *mptr;
CTAB *gcmdp;
C1TAB *ctbr, *glbr;

static CTAB set_glob_tab[] = {
    { "CONSOLE", &sim_set_console, 0 },
    { "BREAK", &brk_cmd, SSH_ST },
    { "TELNET", &sim_set_telnet, 0 },                   /* deprecated */
    { "NOTELNET", &sim_set_notelnet, 0 },               /* deprecated */
    { "LOG", &sim_set_logon, 0 },                       /* deprecated */
    { "NOLOG", &sim_set_logoff, 0 },                    /* deprecated */
    { "DEBUG", &sim_set_debon, 0 },                     /* deprecated */
    { "NODEBUG", &sim_set_deboff, 0 },                  /* deprecated */
    { NULL, NULL, 0 }
    };

static C1TAB set_dev_tab[] = {
    { "OCTAL", &set_dev_radix, 8 },
    { "DECIMAL", &set_dev_radix, 10 },
    { "HEX", &set_dev_radix, 16 },
    { "ENABLED", &set_dev_enbdis, 1 },
    { "DISABLED", &set_dev_enbdis, 0 },
    { "DEBUG", &set_dev_debug, 1 },
    { "NODEBUG", &set_dev_debug, 0 },
    { NULL, NULL, 0 }
    };

static C1TAB set_unit_tab[] = {
    { "ENABLED", &set_unit_enbdis, 1 },
    { "DISABLED", &set_unit_enbdis, 0 },
    { NULL, NULL, 0 }
    };

GET_SWITCHES (cptr);                                    /* get switches */
if (*cptr == 0) return SCPE_2FARG;                      /* must be more */
cptr = get_glyph (cptr, gbuf, 0);                       /* get glob/dev/unit */

if (dptr = find_dev (gbuf)) {                           /* device match? */
    uptr = dptr->units;                                 /* first unit */
    ctbr = set_dev_tab;                                 /* global table */
    lvl = MTAB_VDV;                                     /* device match */
    }
else if (dptr = find_unit (gbuf, &uptr)) {              /* unit match? */
    if (uptr == NULL) return SCPE_NXUN;                 /* invalid unit */
    ctbr = set_unit_tab;                                /* global table */
    lvl = MTAB_VUN;                                     /* unit match */
    }
else if (gcmdp = find_ctab (set_glob_tab, gbuf))        /* global? */
    return gcmdp->action (gcmdp->arg, cptr);            /* do the rest */
else return SCPE_NXDEV;                                 /* no match */
if (*cptr == 0) return SCPE_2FARG;                      /* must be more */

while (*cptr != 0) {                                    /* do all mods */
    cptr = get_glyph (svptr = cptr, gbuf, ',');         /* get modifier */
    if (cvptr = strchr (gbuf, '=')) *cvptr++ = 0;       /* = value? */
    for (mptr = dptr->modifiers; mptr && (mptr->mask != 0); mptr++) {
        if ((mptr->mstring) &&                          /* match string */
            (MATCH_CMD (gbuf, mptr->mstring) == 0)) {   /* matches option? */
            if (mptr->mask & MTAB_XTD) {                /* extended? */
                if ((lvl & mptr->mask) == 0) return SCPE_ARG;
                if ((lvl & MTAB_VUN) && (uptr->flags & UNIT_DIS))
                    return SCPE_UDIS;                   /* unit disabled? */
                if (mptr->valid) {                      /* validation rtn? */
                    if (cvptr && (mptr->mask & MTAB_NC))
                        get_glyph_nc (svptr, gbuf, ',');
                    r = mptr->valid (uptr, mptr->match, cvptr, mptr->desc);
                    if (r != SCPE_OK) return r;
                    }
                else if (!mptr->desc) break;            /* value desc? */
                else if (mptr->mask & MTAB_VAL) {       /* take a value? */
                    if (!cvptr) return SCPE_MISVAL;     /* none? error */
                    r = dep_reg (0, cvptr, (REG *) mptr->desc, 0);
                    if (r != SCPE_OK) return r;
                    }
                else if (cvptr) return SCPE_ARG;        /* = value? */
                else *((int32 *) mptr->desc) = mptr->match;
                }                                       /* end if xtd */
            else {                                      /* old style */
                if (cvptr) return SCPE_ARG;             /* = value? */
                if (uptr->flags & UNIT_DIS)             /* disabled? */
                     return SCPE_UDIS;
                if ((mptr->valid) && ((r = mptr->valid
                    (uptr, mptr->match, cvptr, mptr->desc))
                    != SCPE_OK)) return r;              /* invalid? */
                uptr->flags = (uptr->flags & ~(mptr->mask)) |
                    (mptr->match & mptr->mask);         /* set new value */
                }                                       /* end else xtd */
            break;                                      /* terminate for */
            }                                           /* end if match */
        }                                               /* end for */
    if (!mptr || (mptr->mask == 0)) {                   /* no match? */
        if (glbr = find_c1tab (ctbr, gbuf)) {           /* global match? */
            r = glbr->action (dptr, uptr, glbr->arg, cvptr);    /* do global */
            if (r != SCPE_OK) return r;
            }
        else if (!dptr->modifiers) return SCPE_NOPARAM; /* no modifiers? */
        else return SCPE_NXPAR;
        }                                               /* end if no mat */
    }                                                   /* end while */
return SCPE_OK;                                         /* done all */
}

/* Match CTAB/CTAB1 name */

CTAB *find_ctab (CTAB *tab, char *gbuf)
{
for (; tab->name != NULL; tab++) {
    if (MATCH_CMD (gbuf, tab->name) == 0) return tab;
    }
return NULL;
}

C1TAB *find_c1tab (C1TAB *tab, char *gbuf)
{
for (; tab->name != NULL; tab++) {
    if (MATCH_CMD (gbuf, tab->name) == 0) return tab;
    }
return NULL;
}

/* Set device data radix routine */

t_stat set_dev_radix (DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr)
{
if (cptr) return SCPE_ARG;
dptr->dradix = flag & 037;
return SCPE_OK;
}

/* Set device enabled/disabled routine */

t_stat set_dev_enbdis (DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr)
{
UNIT *up;
uint32 i;

if (cptr) return SCPE_ARG;
if ((dptr->flags & DEV_DISABLE) == 0) return SCPE_NOFNC;/* allowed? */
if (flag) {                                             /* enable? */
    if ((dptr->flags & DEV_DIS) == 0)                   /* already enb? ok */
        return SCPE_OK;
    dptr->flags = dptr->flags & ~DEV_DIS;               /* no, enable */
    }
else {
    if (dptr->flags & DEV_DIS) return SCPE_OK;          /* already dsb? ok */
    for (i = 0; i < dptr->numunits; i++) {              /* check units */
        up = (dptr->units) + i;                         /* att or active? */
        if ((up->flags & UNIT_ATT) || sim_is_active (up))
            return SCPE_NOFNC;                          /* can't do it */
        }
    dptr->flags = dptr->flags | DEV_DIS;                /* disable */
    }
if (dptr->reset) return dptr->reset (dptr);             /* reset device */
else return SCPE_OK;
}

/* Set unit enabled/disabled routine */

t_stat set_unit_enbdis (DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr)
{
if (cptr) return SCPE_ARG;
if (!(uptr->flags & UNIT_DISABLE)) return SCPE_NOFNC;   /* allowed? */
if (flag) uptr->flags = uptr->flags & ~UNIT_DIS;        /* enb? enable */
else {
    if ((uptr->flags & UNIT_ATT) ||                     /* dsb */
        sim_is_active (uptr)) return SCPE_NOFNC;        /* more tests */
    uptr->flags = uptr->flags | UNIT_DIS;               /* disable */
    }
return SCPE_OK;
}

/* Set device debug enabled/disabled routine */

t_stat set_dev_debug (DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr)
{
char gbuf[CBUFSIZE];
DEBTAB *dep;

if ((dptr->flags & DEV_DEBUG) == 0) return SCPE_NOFNC;
if (cptr == NULL) {                                     /* no arguments? */
    dptr->dctrl = flag;                                 /* disable/enable w/o table */
    if (flag && dptr->debflags) {                       /* enable with table? */
        for (dep = dptr->debflags; dep->name != NULL; dep++)
            dptr->dctrl = dptr->dctrl | dep->mask;      /* set all */
        }
    return SCPE_OK;
    }
if (dptr->debflags == NULL) return SCPE_ARG;            /* must have table */
while (*cptr) {
    cptr = get_glyph (cptr, gbuf, ';');                 /* get debug flag */
    for (dep = dptr->debflags; dep->name != NULL; dep++) {
        if (strcmp (dep->name, gbuf) == 0) {            /* match? */
            if (flag) dptr->dctrl = dptr->dctrl | dep->mask;
            else dptr->dctrl = dptr->dctrl & ~dep->mask;
            break;
            }
        }                                               /* end for */
    if (dep->mask == 0) return SCPE_ARG;                /* no match? */
    }                                                   /* end while */
return SCPE_OK;
}

/* Show command */

t_stat show_cmd (int32 flag, char *cptr)
{
t_stat r;

cptr = get_sim_opt (CMD_OPT_SW|CMD_OPT_OF, cptr, &r);   /* get sw, ofile */
if (!cptr) return r;                                    /* error? */
if (sim_ofile) {                                        /* output file? */
    r = show_cmd_fi (sim_ofile, flag, cptr);            /* do show */
    fclose (sim_ofile);
    }
else {
    r = show_cmd_fi (stdout, flag, cptr);               /* no, stdout, log */
    if (sim_log) show_cmd_fi (sim_log, flag, cptr);
    }
return r;
}

t_stat show_cmd_fi (FILE *ofile, int32 flag, char *cptr)
{
int32 lvl;
char gbuf[CBUFSIZE], *cvptr;
DEVICE *dptr;
UNIT *uptr;
MTAB *mptr;
SHTAB *shtb, *shptr;

static SHTAB show_glob_tab[] = {
    { "CONFIGURATION", &show_config, 0 },
    { "DEVICES", &show_config, 1 },
    { "QUEUE", &show_queue, 0 },
    { "TIME", &show_time, 0 },
    { "MODIFIERS", &show_mod_names, 0 },
    { "NAMES", &show_log_names, 0 },
    { "VERSION", &show_version, 1 },
    { "CONSOLE", &sim_show_console, 0 },
    { "BREAK", &show_break, 0 },
    { "LOG", &sim_show_log, 0 },                        /* deprecated */
    { "TELNET", &sim_show_telnet, 0 },                  /* deprecated */
    { "DEBUG", &sim_show_debug, 0 },                    /* deprecated */
    { NULL, NULL, 0 }
    };

static SHTAB show_dev_tab[] = {
    { "RADIX", &show_dev_radix, 0 },
    { "DEBUG", &show_dev_debug, 0 },
    { "MODIFIERS", &show_dev_modifiers, 0 },
    { "NAMES", &show_dev_logicals, 0 },
    { NULL, NULL, 0 }
    };

static SHTAB show_unit_tab[] = {
    { NULL, NULL, 0 }
    };

GET_SWITCHES (cptr);                                    /* get switches */
if (*cptr == 0) return SCPE_2FARG;                      /* must be more */
cptr = get_glyph (cptr, gbuf, 0);                       /* get next glyph */
if (shptr = find_shtab (show_glob_tab, gbuf))           /* global? */
    return shptr->action (ofile, NULL, NULL, shptr->arg, cptr);

if (dptr = find_dev (gbuf)) {                           /* device match? */
    uptr = dptr->units;                                 /* first unit */
    shtb = show_dev_tab;                                /* global table */
    lvl = MTAB_VDV;                                     /* device match */
    }
else if (dptr = find_unit (gbuf, &uptr)) {              /* unit match? */
    if (uptr == NULL) return SCPE_NXUN;                 /* invalid unit */
    if (uptr->flags & UNIT_DIS) return SCPE_UDIS;       /* disabled? */
    shtb = show_unit_tab;                               /* global table */
    lvl = MTAB_VUN;                                     /* unit match */
    }
else return SCPE_NXDEV;                                 /* no match */

if (*cptr == 0) {                                       /* now eol? */
    return (lvl == MTAB_VDV)?
        show_device (ofile, dptr, 0):
        show_unit (ofile, dptr, uptr, -1);
    }
if (dptr->modifiers == NULL) return SCPE_NOPARAM;       /* any modifiers? */

while (*cptr != 0) {                                    /* do all mods */
    cptr = get_glyph (cptr, gbuf, ',');                 /* get modifier */
    if (cvptr = strchr (gbuf, '=')) *cvptr++ = 0;       /* = value? */
    for (mptr = dptr->modifiers; mptr->mask != 0; mptr++) {
        if (((mptr->mask & MTAB_XTD)?                   /* right level? */
            (mptr->mask & lvl): (MTAB_VUN & lvl)) && 
            ((mptr->disp && mptr->pstring &&            /* named disp? */
            (MATCH_CMD (gbuf, mptr->pstring) == 0)) ||
            ((mptr->mask & MTAB_VAL) &&                 /* named value? */
            mptr->mstring &&
            (MATCH_CMD (gbuf, mptr->mstring) == 0)))) {
            if (cvptr && !(mptr->mask & MTAB_SHP)) return SCPE_ARG;
            show_one_mod (ofile, dptr, uptr, mptr, cvptr, 1);
            break;
            }                                           /* end if */
        }                                               /* end for */
    if (mptr->mask == 0) {                              /* no match? */
        if (shptr = find_shtab (shtb, gbuf))            /* global match? */
            shptr->action (ofile, dptr, uptr, shptr->arg, cptr);
        else return SCPE_ARG;
        }                                               /* end if */
    }                                                   /* end while */
return SCPE_OK;
}

SHTAB *find_shtab (SHTAB *tab, char *gbuf)
{
for (; tab->name != NULL; tab++) {
    if (MATCH_CMD (gbuf, tab->name) == 0) return tab;
    }
return NULL;
}

/* Show device and unit */

t_stat show_device (FILE *st, DEVICE *dptr, int32 flag)
{
uint32 j, udbl, ucnt;
UNIT *uptr;

fprintf (st, "%s", sim_dname (dptr));                   /* print dev name */
if (qdisable (dptr)) {                                  /* disabled? */
    fprintf (st, ", disabled\n");
    return SCPE_OK;
    }
for (j = ucnt = udbl = 0; j < dptr->numunits; j++) {    /* count units */
    uptr = dptr->units + j;
    if (uptr->flags & UNIT_DISABLE) udbl++;
    if (!(uptr->flags & UNIT_DIS)) ucnt++;
    }
show_all_mods (st, dptr, dptr->units, MTAB_VDV);        /* show dev mods */
if (dptr->numunits == 0) fprintf (st, "\n");
else {
    if (udbl && (ucnt == 0)) fprintf (st, ", all units disabled\n");
    else if (ucnt > 1) fprintf (st, ", %d units\n", ucnt);
    else if (flag) fprintf (st, "\n");
    }
if (flag) return SCPE_OK;                               /* dev only? */
for (j = 0; j < dptr->numunits; j++) {                  /* loop thru units */
    uptr = dptr->units + j;
    if ((uptr->flags & UNIT_DIS) == 0)
        show_unit (st, dptr, uptr, ucnt);
    }
return SCPE_OK;
}

t_stat show_unit (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag)
{
int32 u = uptr - dptr->units;

if (flag > 1) fprintf (st, "  %s%d", sim_dname (dptr), u);
else if (flag < 0) fprintf (st, "%s%d", sim_dname (dptr), u);
if (uptr->flags & UNIT_FIX) {
    fprintf (st, ", ");
    fprint_capac (st, dptr, uptr);
    }
if (uptr->flags & UNIT_ATT) {
    fprintf (st, ", attached to %s", uptr->filename);
    if (uptr->flags & UNIT_RO) fprintf (st, ", read only");
    }
else if (uptr->flags & UNIT_ATTABLE)
    fprintf (st, ", not attached");
show_all_mods (st, dptr, uptr, MTAB_VUN);               /* show unit mods */ 
fprintf (st, "\n");
return SCPE_OK;
}

void fprint_capac (FILE *st, DEVICE *dptr, UNIT *uptr)
{
t_addr kval = (uptr->flags & UNIT_BINK)? 1024: 1000;
t_addr mval = kval * kval;
t_addr psize = uptr->capac;
char scale, width;

if ((dptr->dwidth / dptr->aincr) > 8) width = 'W';
else width = 'B';
if (uptr->capac < (kval * 10)) scale = 0;
else if (uptr->capac < (mval * 10)) {
    scale = 'K';
    psize = psize / kval;
    }
else {
    scale = 'M';
    psize = psize / mval;
    }
fprint_val (st, (t_value) psize, 10, T_ADDR_W, PV_LEFT);
if (scale) fputc (scale, st);
fputc (width, st);
return;
}

/* Show <global name> processors  */

t_stat show_version (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr)
{
int32 vmaj = SIM_MAJOR, vmin = SIM_MINOR, vpat = SIM_PATCH;

if (cptr && (*cptr != 0)) return SCPE_2MARG;
fprintf (st, "%s simulator V%d.%d-%d", sim_name, vmaj, vmin, vpat);
if (flag) fprintf (st, " [%s, %s, %s]", sim_si64, sim_sa64, sim_snet);
fprintf (st, "\n");
return SCPE_OK;
}

t_stat show_config (FILE *st, DEVICE *dnotused, UNIT *unotused, int32 flag, char *cptr)
{
int32 i;
DEVICE *dptr;

if (cptr && (*cptr != 0)) return SCPE_2MARG;
fprintf (st, "%s simulator configuration\n\n", sim_name);
for (i = 0; (dptr = sim_devices[i]) != NULL; i++)
    show_device (st, dptr, flag);
return SCPE_OK;
}

t_stat show_log_names (FILE *st, DEVICE *dnotused, UNIT *unotused, int32 flag, char *cptr)
{
int32 i;
DEVICE *dptr;

if (cptr && (*cptr != 0)) return SCPE_2MARG;
for (i = 0; (dptr = sim_devices[i]) != NULL; i++)
    show_dev_logicals (st, dptr, NULL, 1, cptr);
return SCPE_OK;
}

t_stat show_dev_logicals (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr)
{
if (dptr->lname) fprintf (st, "%s -> %s\n", dptr->lname, dptr->name);
else if (!flag) fputs ("no logical name assigned\n", st);
return SCPE_OK;
}

t_stat show_queue (FILE *st, DEVICE *dnotused, UNIT *unotused, int32 flag, char *cptr)
{
DEVICE *dptr;
UNIT *uptr;
int32 accum;

if (cptr && (*cptr != 0)) return SCPE_2MARG;
if (sim_clock_queue == NULL) {
    fprintf (st, "%s event queue empty, time = %.0f\n",
        sim_name, sim_time);
    return SCPE_OK;
    }
fprintf (st, "%s event queue status, time = %.0f\n",
     sim_name, sim_time);
accum = 0;
for (uptr = sim_clock_queue; uptr != NULL; uptr = uptr->next) {
    if (uptr == &sim_step_unit) fprintf (st, "  Step timer");
    else if ((dptr = find_dev_from_unit (uptr)) != NULL) {
        fprintf (st, "  %s", sim_dname (dptr));
        if (dptr->numunits > 1) fprintf (st, " unit %d",
            (int32) (uptr - dptr->units));
        }
    else fprintf (st, "  Unknown");
    fprintf (st, " at %d\n", accum + uptr->time);
    accum = accum + uptr->time;
    }
return SCPE_OK;
}

t_stat show_time (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr)
{
if (cptr && (*cptr != 0)) return SCPE_2MARG;
fprintf (st, "Time:\t%.0f\n", sim_time);
return SCPE_OK;
}

t_stat show_break (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr)
{
t_stat r;

if (cptr && (*cptr != 0)) r = ssh_break (st, cptr, 1);  /* more? */
else r = sim_brk_showall (st, sim_switches);
return r;
}

t_stat show_dev_radix (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr)
{
fprintf (st, "Radix=%d\n", dptr->dradix);
return SCPE_OK;
}

t_stat show_dev_debug (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr)
{
int32 any = 0;
DEBTAB *dep;

if (dptr->flags & DEV_DEBUG) {
    if (dptr->dctrl == 0) fputs ("Debugging disabled", st);
    else if (dptr->debflags == NULL) fputs ("Debugging enabled", st);
    else {
        fputs ("Debug=", st);
        for (dep = dptr->debflags; dep->name != NULL; dep++) {
            if (dptr->dctrl & dep->mask) {
                if (any) fputc (';', st);
                fputs (dep->name, st);
                any = 1;
                }
            }
        }
    fputc ('\n', st);
    return SCPE_OK;
    }
else return SCPE_NOFNC;
}

/* Show modifiers */

t_stat show_mod_names (FILE *st, DEVICE *dnotused, UNIT *unotused, int32 flag, char *cptr)
{
int32 i;
DEVICE *dptr;

if (cptr && (*cptr != 0)) return SCPE_2MARG;            /* now eol? */
for (i = 0; (dptr = sim_devices[i]) != NULL; i++) 
    show_dev_modifiers (st, dptr, NULL, flag, cptr);
return SCPE_OK;
}

t_stat show_dev_modifiers (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, char *cptr)
{
int any, enb;
MTAB *mptr;
DEBTAB *dep;

any = enb = 0;
if (dptr->modifiers) {
    for (mptr = dptr->modifiers; mptr->mask != 0; mptr++) {
        if (mptr->mstring) {
            if (strcmp (mptr->mstring, "ENABLED") == 0) enb = 1;
            if (any++) fprintf (st, ", %s", mptr->mstring);
            else fprintf (st, "%s\t%s", sim_dname (dptr), mptr->mstring);
            }
        }
    }
if (dptr->flags & DEV_DEBUG) {
    if (any++) fprintf (st, ", DEBUG, NODEBUG");
    else fprintf (st, "%s\tDEBUG, NODEBUG", sim_dname (dptr));
    }
if (!enb && (dptr->flags & DEV_DISABLE)) {
    if (any++) fprintf (st, ", ENABLED, DISABLED");
    else fprintf (st, "%s\tENABLED, DISABLED", sim_dname (dptr));
    }
if (any) fprintf (st, "\n");
if ((dptr->flags & DEV_DEBUG) && dptr->debflags) {
    fprintf (st, "%s\tDEBUG=", sim_dname (dptr));
    for (dep = dptr->debflags; dep->name != NULL; dep++)
        fprintf (st, "%s%s", ((dep == dptr->debflags) ? "" : ","), dep->name);
    fprintf (st, "\n");
    }
return SCPE_OK;
}

t_stat show_all_mods (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag)
{
MTAB *mptr;

if (dptr->modifiers == NULL) return SCPE_OK;
for (mptr = dptr->modifiers; mptr->mask != 0; mptr++) {
    if (mptr->pstring && ((mptr->mask & MTAB_XTD)?
        ((mptr->mask & flag) && !(mptr->mask & MTAB_NMO)): 
        ((MTAB_VUN & flag) && ((uptr->flags & mptr->mask) == mptr->match)))) {
        fputs (", ", st);
        show_one_mod (st, dptr, uptr, mptr, NULL, 0);
        }
    }
return SCPE_OK;
}

t_stat show_one_mod (FILE *st, DEVICE *dptr, UNIT *uptr, MTAB *mptr,
    char *cptr, int32 flag)
{
t_value val;

if (mptr->disp) mptr->disp (st, uptr, mptr->match, cptr? cptr: mptr->desc);
else if ((mptr->mask & MTAB_XTD) && (mptr->mask & MTAB_VAL)) {
    REG *rptr = (REG *) mptr->desc;
    fprintf (st, "%s=", mptr->pstring);
    val = get_rval (rptr, 0);
    fprint_val (st, val, rptr->radix, rptr->width,
        rptr->flags & REG_FMT);
    }
else fputs (mptr->pstring, st);
if (flag && !((mptr->mask & MTAB_XTD) &&
    (mptr->mask & MTAB_NMO))) fputc ('\n', st);
return SCPE_OK;
}

/* Breakpoint commands */

t_stat brk_cmd (int32 flg, char *cptr)
{
GET_SWITCHES (cptr);                                    /* get switches */
return ssh_break (NULL, cptr, flg);                     /* call common code */
}

t_stat ssh_break (FILE *st, char *cptr, int32 flg)
{
char gbuf[CBUFSIZE], *tptr, *t1ptr, *aptr;
DEVICE *dptr = sim_dflt_dev;
UNIT *uptr = dptr->units;
t_stat r;
t_addr lo, hi, max = uptr->capac - 1;
int32 cnt;

if (*cptr == 0) return SCPE_2FARG;
if (sim_brk_types == 0) return SCPE_NOFNC;
if ((dptr == NULL) || (uptr == NULL)) return SCPE_IERR;
if (aptr = strchr (cptr, ';')) {                        /* ;action? */
    if (flg != SSH_ST) return SCPE_ARG;                 /* only on SET */
    *aptr++ = 0;                                        /* separate strings */
    }
while (*cptr) {
    cptr = get_glyph (cptr, gbuf, ',');
    tptr = get_range (dptr, gbuf, &lo, &hi, dptr->aradix, max, 0);
    if (tptr == NULL) return SCPE_ARG;
    if (*tptr == '[') {
        cnt = (int32) strtotv (tptr + 1, &t1ptr, 10);
        if ((tptr == t1ptr) || (*t1ptr != ']') ||
            (flg != SSH_ST)) return SCPE_ARG;
        tptr = t1ptr + 1;
        }
    else cnt = 0;
    if (*tptr != 0) return SCPE_ARG;
    if ((lo == 0) && (hi == max)) {
        if (flg == SSH_CL) sim_brk_clrall (sim_switches);
        else if (flg == SSH_SH) sim_brk_showall (st, sim_switches);
        else return SCPE_ARG;
        }
    else {      
        for ( ; lo <= hi; lo = lo + 1) {
            switch (flg) {

            case SSH_ST:
                r = sim_brk_set (lo, sim_switches, cnt, aptr);
                break;

            case SSH_CL:
                r = sim_brk_clr (lo, sim_switches);
                break;

            case SSH_SH:
                r = sim_brk_show (st, lo, sim_switches);
                break;

            default:
                return SCPE_ARG;
                }

            if (r != SCPE_OK) return r;
            }
        }
    }
return SCPE_OK;
}

/* Reset command and routines

   re[set]              reset all devices
   re[set] all          reset all devices
   re[set] device       reset specific device
*/

t_stat reset_cmd (int32 flag, char *cptr)
{
char gbuf[CBUFSIZE];
DEVICE *dptr;

GET_SWITCHES (cptr);                                    /* get switches */
if (*cptr == 0) return (reset_all (0));                 /* reset(cr) */
cptr = get_glyph (cptr, gbuf, 0);                       /* get next glyph */
if (*cptr != 0) return SCPE_2MARG;                      /* now eol? */
if (strcmp (gbuf, "ALL") == 0) return (reset_all (0));
dptr = find_dev (gbuf);                                 /* locate device */
if (dptr == NULL) return SCPE_NXDEV;                    /* found it? */
if (dptr->reset != NULL) return dptr->reset (dptr);
else return SCPE_OK;
}

/* Reset devices start..end

   Inputs:
        start   =       number of starting device
   Outputs:
        status  =       error status
*/

t_stat reset_all (uint32 start)
{
DEVICE *dptr;
uint32 i;
t_stat reason;

for (i = 0; i < start; i++) {
    if (sim_devices[i] == NULL) return SCPE_IERR;
    }
for (i = start; (dptr = sim_devices[i]) != NULL; i++) {
    if (dptr->reset != NULL) {
        reason = dptr->reset (dptr);
        if (reason != SCPE_OK) return reason;
        }
    }
return SCPE_OK;
}

/* Reset to powerup state

   Inputs:
        start   =       number of starting device
   Outputs:
        status  =       error status
*/

t_stat reset_all_p (uint32 start)
{
t_stat r;
int32 old_sw = sim_switches;

sim_switches = SWMASK ('P');
r = reset_all (start);
sim_switches = old_sw;
return r;
}

/* Load and dump commands

   lo[ad] filename {arg}        load specified file
   du[mp] filename {arg}        dump to specified file
*/

t_stat load_cmd (int32 flag, char *cptr)
{
char gbuf[CBUFSIZE];
FILE *loadfile;
t_stat reason;

GET_SWITCHES (cptr);                                    /* get switches */
if (*cptr == 0) return SCPE_2FARG;                      /* must be more */
cptr = get_glyph_nc (cptr, gbuf, 0);                    /* get file name */
loadfile = sim_fopen (gbuf, flag? "wb": "rb");          /* open for wr/rd */
if (loadfile == NULL) return SCPE_OPENERR;
GET_SWITCHES (cptr);                                    /* get switches */
reason = sim_load (loadfile, cptr, gbuf, flag);         /* load or dump */
fclose (loadfile);
return reason;
}

/* Attach command

   at[tach] unit file   attach specified unit to file
*/

t_stat attach_cmd (int32 flag, char *cptr)
{
char gbuf[CBUFSIZE];
DEVICE *dptr;
UNIT *uptr;
t_stat r;

GET_SWITCHES (cptr);                                    /* get switches */
if (*cptr == 0) return SCPE_2FARG;                      /* must be more */
cptr = get_glyph (cptr, gbuf, 0);                       /* get next glyph */
GET_SWITCHES (cptr);                                    /* get switches */
if (*cptr == 0) return SCPE_2FARG;                      /* now eol? */
dptr = find_unit (gbuf, &uptr);                         /* locate unit */
if (dptr == NULL) return SCPE_NXDEV;                    /* found dev? */
if (uptr == NULL) return SCPE_NXUN;                     /* valid unit? */
if (uptr->flags & UNIT_ATT) {                           /* already attached? */
    r = scp_detach_unit (dptr, uptr);                   /* detach it */
    if (r != SCPE_OK) return r;                         /* error? */
    }
sim_trim_endspc (cptr);                                 /* trim trailing spc */
return scp_attach_unit (dptr, uptr, cptr);              /* attach */
}

/* Call device-specific or file-oriented attach unit routine */

t_stat scp_attach_unit (DEVICE *dptr, UNIT *uptr, char *cptr)
{
if (dptr->attach != NULL)                               /* device routine? */
    return dptr->attach (uptr, cptr);                   /* call it */
return attach_unit (uptr, cptr);                        /* no, std routine */
}

/* Attach unit to file */

t_stat attach_unit (UNIT *uptr, char *cptr)
{
DEVICE *dptr;

if (uptr->flags & UNIT_DIS) return SCPE_UDIS;           /* disabled? */
if (!(uptr->flags & UNIT_ATTABLE)) return SCPE_NOATT;   /* not attachable? */
if ((dptr = find_dev_from_unit (uptr)) == NULL) return SCPE_NOATT;
if (dptr->flags & DEV_RAWONLY) return SCPE_NOFNC;       /* raw mode only? */
uptr->filename = (char *) calloc (CBUFSIZE, sizeof (char)); /* alloc name buf */
if (uptr->filename == NULL) return SCPE_MEM;
strncpy (uptr->filename, cptr, CBUFSIZE);               /* save name */
if (sim_switches & SWMASK ('R')) {                      /* read only? */
    if ((uptr->flags & UNIT_ROABLE) == 0)               /* allowed? */
        return attach_err (uptr, SCPE_NORO);            /* no, error */
    uptr->fileref = sim_fopen (cptr, "rb");             /* open rd only */
    if (uptr->fileref == NULL)                          /* open fail? */
        return attach_err (uptr, SCPE_OPENERR);         /* yes, error */
    uptr->flags = uptr->flags | UNIT_RO;                /* set rd only */
    if (!sim_quiet) printf ("%s: unit is read only\n", sim_dname (dptr));
    }
else {                                                  /* normal */
    uptr->fileref = sim_fopen (cptr, "rb+");            /* open r/w */
    if (uptr->fileref == NULL) {                        /* open fail? */
        if ((errno == EROFS) || (errno == EACCES)) {    /* read only? */
            if ((uptr->flags & UNIT_ROABLE) == 0)       /* allowed? */
                return attach_err (uptr, SCPE_NORO);    /* no error */
            uptr->fileref = sim_fopen (cptr, "rb");     /* open rd only */
            if (uptr->fileref == NULL)                  /* open fail? */
                return attach_err (uptr, SCPE_OPENERR); /* yes, error */
            uptr->flags = uptr->flags | UNIT_RO;        /* set rd only */
            if (!sim_quiet) printf ("%s: unit is read only\n", sim_dname (dptr));
            }
        else {                                          /* doesn't exist */
            if (sim_switches & SWMASK ('E'))            /* must exist? */
                return attach_err (uptr, SCPE_OPENERR); /* yes, error */
            uptr->fileref = sim_fopen (cptr, "wb+");    /* open new file */
            if (uptr->fileref == NULL)                  /* open fail? */
                return attach_err (uptr, SCPE_OPENERR); /* yes, error */
            if (!sim_quiet) printf ("%s: creating new file\n", sim_dname (dptr));
            }
        }                                               /* end if null */
    }                                                   /* end else */
if (uptr->flags & UNIT_BUFABLE) {                       /* buffer? */
    uint32 cap = ((uint32) uptr->capac) / dptr->aincr;  /* effective size */
    if (uptr->flags & UNIT_MUSTBUF)                     /* dyn alloc? */
        uptr->filebuf = calloc (cap, SZ_D (dptr));      /* allocate */
    if (uptr->filebuf == NULL)                          /* no buffer? */
        return attach_err (uptr, SCPE_MEM);             /* error */
    if (!sim_quiet) printf ("%s: buffering file in memory\n", sim_dname (dptr));
    uptr->hwmark = sim_fread (uptr->filebuf,            /* read file */
        SZ_D (dptr), cap, uptr->fileref);
    uptr->flags = uptr->flags | UNIT_BUF;               /* set buffered */
    }
uptr->flags = uptr->flags | UNIT_ATT;
uptr->pos = 0;
return SCPE_OK;
}

t_stat attach_err (UNIT *uptr, t_stat stat)
{
free (uptr->filename);
uptr->filename = NULL;
return stat;
}

/* Detach command

   det[ach] all         detach all units
   det[ach] unit        detach specified unit
*/

t_stat detach_cmd (int32 flag, char *cptr)
{
char gbuf[CBUFSIZE];
DEVICE *dptr;
UNIT *uptr;

GET_SWITCHES (cptr);                                    /* get switches */
if (*cptr == 0) return SCPE_2FARG;                      /* must be more */
cptr = get_glyph (cptr, gbuf, 0);                       /* get next glyph */
if (*cptr != 0) return SCPE_2MARG;                      /* now eol? */
if (strcmp (gbuf, "ALL") == 0) return (detach_all (0, FALSE));
dptr = find_unit (gbuf, &uptr);                         /* locate unit */
if (dptr == NULL) return SCPE_NXDEV;                    /* found dev? */
if (uptr == NULL) return SCPE_NXUN;                     /* valid unit? */
return scp_detach_unit (dptr, uptr);                    /* detach */
}

/* Detach devices start..end

   Inputs:
        start   =       number of starting device
        shutdown =      TRUE if simulator shutting down
   Outputs:
        status  =       error status

   Note that during shutdown, detach routines for non-attachable devices
   will be called.  These routines can implement simulator shutdown.
*/

t_stat detach_all (int32 start, t_bool shutdown)
{
uint32 i, j;
DEVICE *dptr;
UNIT *uptr;
t_stat r;

if ((start < 0) || (start > 1)) return SCPE_IERR;
for (i = start; (dptr = sim_devices[i]) != NULL; i++) { /* loop thru dev */
    for (j = 0; j < dptr->numunits; j++) {              /* loop thru units */
        uptr = (dptr->units) + j;
        if ((uptr->flags & UNIT_ATT) ||                 /* attached? */
            (shutdown && dptr->detach &&                /* shutdown, spec rtn, */
            !(uptr->flags & UNIT_ATTABLE))) {           /* !attachable? */
            r = scp_detach_unit (dptr, uptr);           /* detach unit */
            if (r != SCPE_OK) return r;
            }
        }
    }
return SCPE_OK;
}

/* Call device-specific or file-oriented detach unit routine */

t_stat scp_detach_unit (DEVICE *dptr, UNIT *uptr)
{
if (dptr->detach != NULL) return dptr->detach (uptr);   /* device routine? */
return detach_unit (uptr);                              /* no, standard */
}

/* Detach unit from file */

t_stat detach_unit (UNIT *uptr)
{
DEVICE *dptr;

if (uptr == NULL) return SCPE_IERR;
if (!(uptr->flags & UNIT_ATTABLE)) return SCPE_NOATT;   /* attachable? */
if (!(uptr->flags & UNIT_ATT)) return SCPE_OK;          /* attached? */
if ((dptr = find_dev_from_unit (uptr)) == NULL) return SCPE_OK;
if (uptr->flags & UNIT_BUF) {
    uint32 cap = (uptr->hwmark + dptr->aincr - 1) / dptr->aincr;
    if (uptr->hwmark && ((uptr->flags & UNIT_RO) == 0)) {
        if (!sim_quiet) printf ("%s: writing buffer to file\n", sim_dname (dptr));
        rewind (uptr->fileref);
        sim_fwrite (uptr->filebuf, SZ_D (dptr), cap, uptr->fileref);
        if (ferror (uptr->fileref)) perror ("I/O error");
        }
    if (uptr->flags & UNIT_MUSTBUF) {                   /* dyn alloc? */
        free (uptr->filebuf);                           /* free buf */
        uptr->filebuf = NULL;
        }
    uptr->flags = uptr->flags & ~UNIT_BUF;
    }
uptr->flags = uptr->flags & ~(UNIT_ATT | UNIT_RO);
free (uptr->filename);
uptr->filename = NULL;
if (fclose (uptr->fileref) == EOF) return SCPE_IOERR;
return SCPE_OK;
}

/* Assign command

   as[sign] device name assign logical name to device
*/

t_stat assign_cmd (int32 flag, char *cptr)
{
char gbuf[CBUFSIZE];
DEVICE *dptr;

GET_SWITCHES (cptr);                                    /* get switches */
if (*cptr == 0) return SCPE_2FARG;                      /* must be more */
cptr = get_glyph (cptr, gbuf, 0);                       /* get next glyph */
GET_SWITCHES (cptr);                                    /* get switches */
if (*cptr == 0) return SCPE_2FARG;                      /* now eol? */
dptr = find_dev (gbuf);                                 /* locate device */
if (dptr == NULL) return SCPE_NXDEV;                    /* found dev? */
cptr = get_glyph (cptr, gbuf, 0);                       /* get next glyph */
if (*cptr != 0) return SCPE_2MARG;                      /* must be eol */
if (find_dev (gbuf)) return SCPE_ARG;                   /* name in use */
deassign_device (dptr);                                 /* release current */
return assign_device (dptr, gbuf);
}

t_stat assign_device (DEVICE *dptr, char *cptr)
{
dptr->lname = (char *) calloc (CBUFSIZE, sizeof (char));
if (dptr->lname == NULL) return SCPE_MEM;
strncpy (dptr->lname, cptr, CBUFSIZE);
return SCPE_OK;
}

/* Deassign command

   dea[ssign] device    deassign logical name
*/

t_stat deassign_cmd (int32 flag, char *cptr)
{
char gbuf[CBUFSIZE];
DEVICE *dptr;

GET_SWITCHES (cptr);                                    /* get switches */
if (*cptr == 0) return SCPE_2FARG;                      /* must be more */
cptr = get_glyph (cptr, gbuf, 0);                       /* get next glyph */
if (*cptr != 0) return SCPE_2MARG;                      /* now eol? */
dptr = find_dev (gbuf);                                 /* locate device */
if (dptr == NULL) return SCPE_NXDEV;                    /* found dev? */
return deassign_device (dptr);
}

t_stat deassign_device (DEVICE *dptr)
{
if (dptr->lname) free (dptr->lname);
dptr->lname = NULL;
return SCPE_OK;
}

/* Get device display name */

char *sim_dname (DEVICE *dptr)
{
return (dptr->lname? dptr->lname: dptr->name);
}

/* Save command

   sa[ve] filename              save state to specified file
*/

t_stat save_cmd (int32 flag, char *cptr)
{
FILE *sfile;
t_stat r;
GET_SWITCHES (cptr);                                    /* get switches */
if (*cptr == 0) return SCPE_2FARG;                      /* must be more */
sim_trim_endspc (cptr);
if ((sfile = sim_fopen (cptr, "wb")) == NULL) return SCPE_OPENERR;
r = sim_save (sfile);
fclose (sfile);
return r;
}

t_stat sim_save (FILE *sfile)
{
void *mbuf;
int32 l, t;
uint32 i, j;
t_addr k, high;
t_value val;
t_stat r;
t_bool zeroflg;
size_t sz;
DEVICE *dptr;
UNIT *uptr;
REG *rptr;

#define WRITE_I(xx) sim_fwrite (&(xx), sizeof (xx), 1, sfile)

fprintf (sfile, "%s\n%s\n%s\n%s\n%s\n%.0f\n",
    save_vercur,                                        /* [V2.5] save format */
    sim_name,                                           /* sim name */
    sim_si64, sim_sa64, sim_snet,                       /* [V3.5] options */
    sim_time);                                          /* [V3.2] sim time */
WRITE_I (sim_rtime);                                    /* [V2.6] sim rel time */

for (i = 0; (dptr = sim_devices[i]) != NULL; i++) {     /* loop thru devices */
    fputs (dptr->name, sfile);                          /* device name */
    fputc ('\n', sfile);
    if (dptr->lname) fputs (dptr->lname, sfile);        /* [V3.0] logical name */
    fputc ('\n', sfile);
    WRITE_I (dptr->flags);                              /* [V2.10] flags */
    for (j = 0; j < dptr->numunits; j++) {
        uptr = dptr->units + j;
        t = sim_is_active (uptr);
        WRITE_I (j);                                    /* unit number */
        WRITE_I (t);                                    /* activation time */
        WRITE_I (uptr->u3);                             /* unit specific */
        WRITE_I (uptr->u4);
        WRITE_I (uptr->u5);                             /* [V3.0] more unit */
        WRITE_I (uptr->u6);
        WRITE_I (uptr->flags);                          /* [V2.10] flags */
        WRITE_I (uptr->capac);                          /* [V3.5] capacity */
        if (uptr->flags & UNIT_ATT) fputs (uptr->filename, sfile);
        fputc ('\n', sfile);
        if (((uptr->flags & (UNIT_FIX + UNIT_ATTABLE)) == UNIT_FIX) &&
             (dptr->examine != NULL) &&
             ((high = uptr->capac) != 0)) {             /* memory-like unit? */
            WRITE_I (high);                             /* [V2.5] write size */
            sz = SZ_D (dptr);
            if ((mbuf = calloc (SRBSIZ, sz)) == NULL) {
                fclose (sfile);
                return SCPE_MEM;
                }
            for (k = 0; k < high; ) {                   /* loop thru mem */
                zeroflg = TRUE;
                for (l = 0; (l < SRBSIZ) && (k < high); l++,
                     k = k + (dptr->aincr)) {           /* check for 0 block */
                    r = dptr->examine (&val, k, uptr, SIM_SW_REST);
                    if (r != SCPE_OK) return r;
                    if (val) zeroflg = FALSE;
                    SZ_STORE (sz, val, mbuf, l);
                    }                                   /* end for l */
                if (zeroflg) {                          /* all zero's? */
                    l = -l;                             /* invert block count */
                    WRITE_I (l);                        /* write only count */
                    }
                else {
                    WRITE_I (l);                        /* block count */
                    sim_fwrite (mbuf, sz, l, sfile);
                    }
                }                                       /* end for k */
            free (mbuf);                                /* dealloc buffer */
            }                                           /* end if mem */
        else {                                          /* no memory */
            high = 0;                                   /* write 0 */
            WRITE_I (high);
            }                                           /* end else mem */
        }                                               /* end unit loop */
    t = -1;                                             /* end units */
    WRITE_I (t);                                        /* write marker */
    for (rptr = dptr->registers; (rptr != NULL) &&      /* loop thru regs */
         (rptr->name != NULL); rptr++) {
        fputs (rptr->name, sfile);                      /* name */
        fputc ('\n', sfile);
        WRITE_I (rptr->depth);                          /* [V2.10] depth */
        for (j = 0; j < rptr->depth; j++) {             /* loop thru values */
            val = get_rval (rptr, j);                   /* get value */
            WRITE_I (val);                              /* store */
            }
        }
    fputc ('\n', sfile);                                /* end registers */
    }
fputc ('\n', sfile);                                    /* end devices */
return (ferror (sfile))? SCPE_IOERR: SCPE_OK;           /* error during save? */
}

/* Restore command

   re[store] filename           restore state from specified file
*/

t_stat restore_cmd (int32 flag, char *cptr)
{
FILE *rfile;
t_stat r;

GET_SWITCHES (cptr);                                    /* get switches */
if (*cptr == 0) return SCPE_2FARG;                      /* must be more */
sim_trim_endspc (cptr);
if ((rfile = sim_fopen (cptr, "rb")) == NULL) return SCPE_OPENERR;
r = sim_rest (rfile);
fclose (rfile);
return r;
}

t_stat sim_rest (FILE *rfile)
{
char buf[CBUFSIZE];
void *mbuf;
int32 j, blkcnt, limit, unitno, time, flg;
uint32 us, depth;
t_addr k, high, old_capac;
t_value val, mask;
t_stat r;
size_t sz;
t_bool v35, v32;
DEVICE *dptr;
UNIT *uptr;
REG *rptr;

#define READ_S(xx) if (read_line ((xx), CBUFSIZE, rfile) == NULL) \
    return SCPE_IOERR;
#define READ_I(xx) if (sim_fread (&xx, sizeof (xx), 1, rfile) == 0) \
    return SCPE_IOERR;

READ_S (buf);                                           /* [V2.5+] read version */
v35 = v32 = FALSE;
if (strcmp (buf, save_vercur) == 0) v35 = v32 = TRUE;   /* version 3.5? */      
else if (strcmp (buf, save_ver32) == 0) v32 = TRUE;     /* version 3.2? */
else if (strcmp (buf, save_ver30) != 0) {               /* version 3.0? */
    printf ("Invalid file version: %s\n", buf);
    return SCPE_INCOMP;
    }
READ_S (buf);                                           /* read sim name */
if (strcmp (buf, sim_name)) {                           /* name match? */
    printf ("Wrong system type: %s\n", buf);
    return SCPE_INCOMP;
    }
if (v35) {                                              /* [V3.5+] options */
    READ_S (buf);                                       /* integer size */
    if (strcmp (buf, sim_si64) != 0) {
        printf ("Incompatible integer size, save file = %s\n", buf);
        return SCPE_INCOMP;
        }
    READ_S (buf);                                       /* address size */
    if (strcmp (buf, sim_sa64) != 0) {
        printf ("Incompatible address size, save file = %s\n", buf);
        return SCPE_INCOMP;
        }
    READ_S (buf);                                       /* Ethernet */
    }
if (v32) {                                              /* [V3.2+] time as string */
    READ_S (buf);
    sscanf (buf, "%lf", &sim_time);
    }
else READ_I (sim_time);                                 /* sim time */
READ_I (sim_rtime);                                     /* [V2.6+] sim rel time */

for ( ;; ) {                                            /* device loop */
    READ_S (buf);                                       /* read device name */
    if (buf[0] == 0) break;                             /* last? */
    if ((dptr = find_dev (buf)) == NULL) {              /* locate device */
        printf ("Invalid device name: %s\n", buf);
        return SCPE_INCOMP;
        }
    READ_S (buf);                                       /* [V3.0+] logical name */
    deassign_device (dptr);                             /* delete old name */
    if ((buf[0] != 0) && 
        ((r = assign_device (dptr, buf)) != SCPE_OK)) return r;
    READ_I (flg);                                       /* [V2.10+] ctlr flags */
    if (!v32) flg = ((flg & DEV_UFMASK_31) << (DEV_V_UF - DEV_V_UF_31)) |
        (flg & ~DEV_UFMASK_31);                         /* [V3.2+] flags moved */
    dptr->flags = (dptr->flags & ~DEV_RFLAGS) |         /* restore ctlr flags */
         (flg & DEV_RFLAGS);
    for ( ;; ) {                                        /* unit loop */
        READ_I (unitno);                                /* unit number */
        if (unitno < 0) break;                          /* end units? */
        if ((uint32) unitno >= dptr->numunits) {        /* too big? */
            printf ("Invalid unit number: %s%d\n", sim_dname (dptr), unitno);
            return SCPE_INCOMP;
            }
        READ_I (time);                                  /* event time */
        uptr = (dptr->units) + unitno;
        sim_cancel (uptr);
        if (time > 0) sim_activate (uptr, time - 1);
        READ_I (uptr->u3);                              /* device specific */
        READ_I (uptr->u4);
        READ_I (uptr->u5);                              /* [V3.0+] more dev spec */
        READ_I (uptr->u6);
        READ_I (flg);                                   /* [V2.10+] unit flags */
        old_capac = uptr->capac;                        /* save current capacity */
        if (v35) {                                      /* [V3.5+] capacity */
            READ_I (uptr->capac);
            }
        if (!v32) flg = ((flg & UNIT_UFMASK_31) << (UNIT_V_UF - UNIT_V_UF_31)) |
            (flg & ~UNIT_UFMASK_31);                    /* [V3.2+] flags moved */
        uptr->flags = (uptr->flags & ~UNIT_RFLAGS) |
            (flg & UNIT_RFLAGS);                        /* restore */
        READ_S (buf);                                   /* attached file */
        if ((uptr->flags & UNIT_ATTABLE) &&             /* if attachable and */
            (!(dptr->flags & DEV_NET) ||                /* not net dev or */
             !(uptr->flags & UNIT_ATT) ||               /* not currently att */
             (buf[0] == 0))) {                          /* or will not be att */
            sim_switches = SIM_SW_REST;                 /* att-det/rest */
            r = scp_detach_unit (dptr, uptr);           /* detach old */
            if (r != SCPE_OK) return r;
            if (buf[0] != 0) {                          /* any file? */
                uptr->flags = uptr->flags & ~UNIT_DIS;
                if (flg & UNIT_RO)                      /* [V2.10+] saved flgs & RO? */
                    sim_switches |= SWMASK ('R');       /* RO attach */
                r = scp_attach_unit (dptr, uptr, buf);
                if (r != SCPE_OK) return r;
                }
            }
        READ_I (high);                                  /* memory capacity */
        if (high > 0) {                                 /* [V2.5+] any memory? */
            if (((uptr->flags & (UNIT_FIX + UNIT_ATTABLE)) != UNIT_FIX) ||
                 (dptr->deposit == NULL)) {
                printf ("Can't restore memory: %s%d\n", sim_dname (dptr), unitno);
                return SCPE_INCOMP;
                }
            if (high != old_capac) {
                if ((dptr->flags & DEV_DYNM) &&
                    ((dptr->msize == NULL) ||
                     (dptr->msize (uptr, (int32) high, NULL, NULL) != SCPE_OK))) {
                    printf ("Can't change memory size: %s%d\n",
                        sim_dname (dptr), unitno);
                    uptr->capac = old_capac;
                    return SCPE_INCOMP;
                    }
                uptr->capac = high;
                printf ("Memory size changed: %s%d = ", sim_dname (dptr), unitno);
                fprint_capac (stdout, dptr, uptr);
                printf ("\n");
                }
            sz = SZ_D (dptr);                           /* allocate buffer */
            if ((mbuf = calloc (SRBSIZ, sz)) == NULL)
                return SCPE_MEM;
            for (k = 0; k < high; ) {                   /* loop thru mem */
                READ_I (blkcnt);                        /* block count */
                if (blkcnt < 0) limit = -blkcnt;        /* compressed? */
                else limit = sim_fread (mbuf, sz, blkcnt, rfile);
                if (limit <= 0) return SCPE_IOERR;      /* invalid or err? */
                for (j = 0; j < limit; j++, k = k + (dptr->aincr)) {
                    if (blkcnt < 0) val = 0;            /* compressed? */
                    else SZ_LOAD (sz, val, mbuf, j);    /* saved value */
                    r = dptr->deposit (val, k, uptr, SIM_SW_REST);
                    if (r != SCPE_OK) return r;
                    }                                   /* end for j */
                }                                       /* end for k */
            free (mbuf);                                /* dealloc buffer */
            }                                           /* end if high */
        }                                               /* end unit loop */
    for ( ;; ) {                                        /* register loop */
        READ_S (buf);                                   /* read reg name */
        if (buf[0] == 0) break;                         /* last? */
        READ_I (depth);                                 /* [V2.10+] depth */
        if ((rptr = find_reg (buf, NULL, dptr)) == NULL) {
            printf ("Invalid register name: %s %s\n", sim_dname (dptr), buf);
            for (us = 0; us < depth; us++) {            /* skip values */
                READ_I (val);
                }
            continue;
            }
        if (depth != rptr->depth)                       /* [V2.10+] mismatch? */
            printf ("Register depth mismatch: %s %s, file = %d, sim = %d\n",
                sim_dname (dptr), buf, depth, rptr->depth);
        mask = width_mask[rptr->width];                 /* get mask */
        for (us = 0; us < depth; us++) {                /* loop thru values */
            READ_I (val);                               /* read value */
            if (val > mask)                             /* value ok? */
                printf ("Invalid register value: %s %s\n", sim_dname (dptr), buf);
            else if (us < rptr->depth)                  /* in range? */
                put_rval (rptr, us, val);
            }
        }
    }                                                   /* end device loop */
return SCPE_OK;
}

/* Run, go, cont, step commands

   ru[n] [new PC]       reset and start simulation
   go [new PC]          start simulation
   co[nt]               start simulation
   s[tep] [step limit]  start simulation for 'limit' instructions
   b[oot] device        bootstrap from device and start simulation
*/

t_stat run_cmd (int32 flag, char *cptr)
{
char *tptr, gbuf[CBUFSIZE];
uint32 i, j;
int32 unitno;
t_value pcv;
t_stat r;
DEVICE *dptr;
UNIT *uptr;
void int_handler (int signal);

GET_SWITCHES (cptr);                                    /* get switches */
sim_step = 0;
if ((flag == RU_RUN) || (flag == RU_GO)) {              /* run or go */
    if (*cptr != 0) {                                   /* argument? */
        cptr = get_glyph (cptr, gbuf, 0);               /* get next glyph */
        if (*cptr != 0) return SCPE_2MARG;              /* should be end */
        if (sim_vm_parse_addr)                          /* address parser? */
            pcv = sim_vm_parse_addr (sim_dflt_dev, gbuf, &tptr);
        else pcv = strtotv (gbuf, &tptr, sim_PC->radix);/* parse PC */
        if ((tptr == gbuf) || (*tptr != 0) ||           /* error? */
            (pcv > width_mask[sim_PC->width])) return SCPE_ARG;
        put_rval (sim_PC, 0, pcv);
        }
    if ((flag == RU_RUN) &&                             /* run? */
        ((r = run_boot_prep ()) != SCPE_OK)) return r;  /* reset sim */
    }

else if (flag == RU_STEP) {                             /* step */
   if (*cptr != 0) {                                    /* argument? */
        cptr = get_glyph (cptr, gbuf, 0);               /* get next glyph */
        if (*cptr != 0) return SCPE_2MARG;              /* should be end */
        sim_step = (int32) get_uint (gbuf, 10, INT_MAX, &r);
        if ((r != SCPE_OK) || (sim_step <= 0))          /* error? */
            return SCPE_ARG;
        }
    else sim_step = 1;
    }

else if (flag == RU_BOOT) {                             /* boot */
    if (*cptr == 0) return SCPE_2FARG;                  /* must be more */
    cptr = get_glyph (cptr, gbuf, 0);                   /* get next glyph */
    if (*cptr != 0) return SCPE_2MARG;                  /* should be end */
    dptr = find_unit (gbuf, &uptr);                     /* locate unit */
    if (dptr == NULL) return SCPE_NXDEV;                /* found dev? */
    if (uptr == NULL) return SCPE_NXUN;                 /* valid unit? */
    if (dptr->boot == NULL) return SCPE_NOFNC;          /* can it boot? */
    if (uptr->flags & UNIT_DIS) return SCPE_UDIS;       /* disabled? */
    if ((uptr->flags & UNIT_ATTABLE) &&                 /* if attable, att? */
        !(uptr->flags & UNIT_ATT)) return SCPE_UNATT;
    unitno = (int32) (uptr - dptr->units);              /* recover unit# */
    if ((r = run_boot_prep ()) != SCPE_OK) return r;    /* reset sim */
    if ((r = dptr->boot (unitno, dptr)) != SCPE_OK)     /* boot device */
        return r;
    }

else if (flag != RU_CONT) return SCPE_IERR;             /* must be cont */

for (i = 1; (dptr = sim_devices[i]) != NULL; i++) {     /* reposition all */
    for (j = 0; j < dptr->numunits; j++) {              /* seq devices */
        uptr = dptr->units + j;
        if ((uptr->flags & (UNIT_ATT + UNIT_SEQ)) ==
            (UNIT_ATT + UNIT_SEQ))
            sim_fseek (uptr->fileref, uptr->pos, SEEK_SET);
        }
    }
stop_cpu = 0;
if (signal (SIGINT, int_handler) == SIG_ERR) {          /* set WRU */
    return SCPE_SIGERR;
    }
if (sim_ttrun () != SCPE_OK) {                          /* set console mode */
    sim_ttcmd ();
    return SCPE_TTYERR;
    }
if ((r = sim_check_console (30)) != SCPE_OK) {          /* check console, error? */
    sim_ttcmd ();
    return r;
    }
if (sim_step) sim_activate (&sim_step_unit, sim_step);  /* set step timer */
sim_is_running = 1;                                     /* flag running */
sim_brk_clract ();                                      /* defang actions */
r = sim_instr();

sim_is_running = 0;                                     /* flag idle */
sim_ttcmd ();                                           /* restore console */
signal (SIGINT, SIG_DFL);                               /* cancel WRU */
sim_cancel (&sim_step_unit);                            /* cancel step timer */
if (sim_clock_queue != NULL) {                          /* update sim time */
    UPDATE_SIM_TIME (sim_clock_queue->time);
    }
else {
    UPDATE_SIM_TIME (noqueue_time);
    }
if (sim_log) fflush (sim_log);                          /* flush console log */
if (sim_deb) fflush (sim_deb);                          /* flush debug log */
for (i = 1; (dptr = sim_devices[i]) != NULL; i++) {     /* flush attached files */
    for (j = 0; j < dptr->numunits; j++) {              /* if not buffered in mem */
        uptr = dptr->units + j;
        if ((uptr->flags & UNIT_ATT) &&                 /* attached, */
            !(uptr->flags & UNIT_BUF) &&                /* not buffered, */
            (uptr->fileref) &&                          /* real file, */
            !(uptr->flags & UNIT_RAW) &&                /* not raw, */
            !(uptr->flags & UNIT_RO))                   /* not read only? */
            fflush (uptr->fileref);
        }
    }
#if defined (VMS)
printf ("\n");
#endif
fprint_stopped (stdout, r);                             /* print msg */
if (sim_log) fprint_stopped (sim_log, r);               /* log if enabled */
return SCPE_OK;
}

/* Common setup for RUN or BOOT */

t_stat run_boot_prep (void)
{
sim_interval = 0;                                       /* reset queue */
sim_time = sim_rtime = 0;
noqueue_time = 0;
sim_clock_queue = NULL;
return reset_all_p (0);
}

/* Print stopped message */

void fprint_stopped_gen (FILE *st, t_stat v, REG *pc, DEVICE *dptr)
{
int32 i;
t_stat r = 0;
t_addr k;
t_value pcval;

if (v >= SCPE_BASE) fprintf (st, "\n%s, %s: ",
    scp_error_messages[v - SCPE_BASE], pc->name);
else fprintf (st, "\n%s, %s: ", sim_stop_messages[v], pc->name);
pcval = get_rval (pc, 0);
if (sim_vm_fprint_addr) sim_vm_fprint_addr (st, dptr, (t_addr) pcval);
else fprint_val (st, pcval, pc->radix, pc->width,
    pc->flags & REG_FMT);
if ((dptr != NULL) && (dptr->examine != NULL)) {
    for (i = 0; i < sim_emax; i++) sim_eval[i] = 0;
    for (i = 0, k = (t_addr) pcval; i < sim_emax; i++, k = k + dptr->aincr) {
        if ((r = dptr->examine (&sim_eval[i], k, dptr->units,
                SWMASK ('V'))) != SCPE_OK) break;
        }
    if ((r == SCPE_OK) || (i > 0)) {
        fprintf (st, " (");
        if (fprint_sym (st, (t_addr) pcval, sim_eval, NULL, SWMASK('M')|SIM_SW_STOP) > 0)
            fprint_val (st, sim_eval[0], dptr->dradix, dptr->dwidth, PV_RZRO);
        fprintf (st, ")");
        }
    }
fprintf (st, "\n");
return;
}

void fprint_stopped (FILE *st, t_stat v)
{
fprint_stopped_gen (st, v, sim_PC, sim_dflt_dev);
return;
}

/* Unit service for step timeout, originally scheduled by STEP n command
   Return step timeout SCP code, will cause simulation to stop */

t_stat step_svc (UNIT *uptr)
{
return SCPE_STEP;
}

/* Cancel scheduled step service */

t_stat sim_cancel_step (void)
{
return sim_cancel (&sim_step_unit);
}

/* Signal handler for ^C signal - set stop simulation flag */

void int_handler (int sig)
{
stop_cpu = 1;
return;
}

/* Examine/deposit commands

   ex[amine] [modifiers] list           examine
   de[posit] [modifiers] list val       deposit
   ie[xamine] [modifiers] list          interactive examine
   id[eposit] [modifiers] list          interactive deposit

   modifiers
        @filename                       output file
        -letter(s)                      switches
        devname'n                       device name and unit number
        [{&|^}value]{=|==|!|!=|>|>=|<|<=} value search specification

   list                                 list of addresses and registers
        addr[:addr|-addr]               address range
        ALL                             all addresses
        register[:register|-register]   register range
        STATE                           all registers
*/

t_stat exdep_cmd (int32 flag, char *cptr)
{
char gbuf[CBUFSIZE], *gptr, *tptr;
int32 opt;
t_addr low, high;
t_stat reason;
DEVICE *tdptr;
REG *lowr, *highr;
FILE *ofile;

opt = CMD_OPT_SW|CMD_OPT_SCH|CMD_OPT_DFT;               /* options for all */
if (flag == EX_E) opt = opt | CMD_OPT_OF;               /* extra for EX */
cptr = get_sim_opt (opt, cptr, &reason);                /* get cmd options */
if (!cptr) return reason;                               /* error? */
if (*cptr == 0) return SCPE_2FARG;                      /* must be more */
if (sim_dfunit == NULL) return SCPE_NXUN;               /* got a unit? */
cptr = get_glyph (cptr, gbuf, 0);                       /* get list */
if ((flag == EX_D) && (*cptr == 0)) return SCPE_2FARG;  /* deposit needs more */
ofile = sim_ofile? sim_ofile: stdout;                   /* no ofile? use stdout */

for (gptr = gbuf, reason = SCPE_OK;
    (*gptr != 0) && (reason == SCPE_OK); gptr = tptr) {
    tdptr = sim_dfdev;                                  /* working dptr */
    if (strncmp (gptr, "STATE", strlen ("STATE")) == 0) {
        tptr = gptr + strlen ("STATE");
        if (*tptr && (*tptr++ != ',')) return SCPE_ARG;
        if ((lowr = sim_dfdev->registers) == NULL) return SCPE_NXREG;
        for (highr = lowr; highr->name != NULL; highr++) ;
        sim_switches = sim_switches | SIM_SW_HIDE;
        reason = exdep_reg_loop (ofile, sim_schptr, flag, cptr,
            lowr, --highr, 0, 0);
        continue;
        }

    if ((lowr = find_reg (gptr, &tptr, tdptr)) ||
        (lowr = find_reg_glob (gptr, &tptr, &tdptr))) {
        low = high = 0;
        if ((*tptr == '-') || (*tptr == ':')) {
            highr = find_reg (tptr + 1, &tptr, tdptr);
            if (highr == NULL) return SCPE_NXREG;
            }
        else {
            highr = lowr;
            if (*tptr == '[') {
                if (lowr->depth <= 1) return SCPE_ARG;
                tptr = get_range (NULL, tptr + 1, &low, &high,
                    10, lowr->depth - 1, ']');
                if (tptr == NULL) return SCPE_ARG;
                }
            }
        if (*tptr && (*tptr++ != ',')) return SCPE_ARG;
        reason = exdep_reg_loop (ofile, sim_schptr, flag, cptr,
            lowr, highr, (uint32) low, (uint32) high);
        continue;
        }

    tptr = get_range (sim_dfdev, gptr, &low, &high, sim_dfdev->aradix,
        (((sim_dfunit->capac == 0) || (flag == EX_E))? 0:
        sim_dfunit->capac - sim_dfdev->aincr), 0);
    if (tptr == NULL) return SCPE_ARG;
    if (*tptr && (*tptr++ != ',')) return SCPE_ARG;
    reason = exdep_addr_loop (ofile, sim_schptr, flag, cptr, low, high,
        sim_dfdev, sim_dfunit);
    }                                                   /* end for */
if (sim_ofile) fclose (sim_ofile);                      /* close output file */
return reason;
}

/* Loop controllers for examine/deposit

   exdep_reg_loop       examine/deposit range of registers
   exdep_addr_loop      examine/deposit range of addresses
*/

t_stat exdep_reg_loop (FILE *ofile, SCHTAB *schptr, int32 flag, char *cptr, 
    REG *lowr, REG *highr, uint32 lows, uint32 highs)
{
t_stat reason;
uint32 idx;
t_value val;
REG *rptr;

if ((lowr == NULL) || (highr == NULL)) return SCPE_IERR;
if (lowr > highr) return SCPE_ARG;
for (rptr = lowr; rptr <= highr; rptr++) {
    if ((sim_switches & SIM_SW_HIDE) &&
        (rptr->flags & REG_HIDDEN)) continue;
    for (idx = lows; idx <= highs; idx++) {
        if (idx >= rptr->depth) return SCPE_SUB;
        val = get_rval (rptr, idx);
        if (schptr && !test_search (val, schptr)) continue;
        if (flag != EX_D) {
            reason = ex_reg (ofile, val, flag, rptr, idx);
            if (reason != SCPE_OK) return reason;
            if (sim_log && (ofile == stdout))
                ex_reg (sim_log, val, flag, rptr, idx);
            }
        if (flag != EX_E) {
            reason = dep_reg (flag, cptr, rptr, idx);
            if (reason != SCPE_OK) return reason;
            }
        }
    }
return SCPE_OK;
}

t_stat exdep_addr_loop (FILE *ofile, SCHTAB *schptr, int32 flag, char *cptr,
    t_addr low, t_addr high, DEVICE *dptr, UNIT *uptr)
{
t_addr i, mask;
t_stat reason;

if (uptr->flags & UNIT_DIS) return SCPE_UDIS;           /* disabled? */
mask = (t_addr) width_mask[dptr->awidth];
if ((low > mask) || (high > mask) || (low > high)) return SCPE_ARG;
for (i = low; i <= high; ) {                            /* all paths must incr!! */
    reason = get_aval (i, dptr, uptr);                  /* get data */
    if (reason != SCPE_OK) return reason;               /* return if error */
    if (schptr && !test_search (sim_eval[0], schptr))
        i = i + dptr->aincr;                            /* sch fails, incr */
    else {                                              /* no sch or success */
        if (flag != EX_D) {                             /* ex, ie, or id? */
            reason = ex_addr (ofile, flag, i, dptr, uptr);
            if (reason > SCPE_OK) return reason;
            if (sim_log && (ofile == stdout))
                ex_addr (sim_log, flag, i, dptr, uptr);
            }
        else reason = 1 - dptr->aincr;                  /* no, dflt incr */
        if (flag != EX_E) {                             /* ie, id, or d? */
            reason = dep_addr (flag, cptr, i, dptr, uptr, reason);
            if (reason > SCPE_OK) return reason;
            }
        i = i + (1 - reason);                           /* incr */
        }
    }
return SCPE_OK;
}

/* Examine register routine

   Inputs:
        ofile   =       output stream
        val     =       current register value
        flag    =       type of ex/mod command (ex, iex, idep)
        rptr    =       pointer to register descriptor
        idx     =       index
   Outputs:
        return  =       error status
*/

t_stat ex_reg (FILE *ofile, t_value val, int32 flag, REG *rptr, uint32 idx)
{
int32 rdx;

if (rptr == NULL) return SCPE_IERR;
if (rptr->depth > 1) fprintf (ofile, "%s[%d]:\t", rptr->name, idx);
else fprintf (ofile, "%s:\t", rptr->name);
if (!(flag & EX_E)) return SCPE_OK;
GET_RADIX (rdx, rptr->radix);
if ((rptr->flags & REG_VMAD) && sim_vm_fprint_addr)
    sim_vm_fprint_addr (ofile, sim_dflt_dev, (t_addr) val);
else if (!(rptr->flags & REG_VMIO) ||
    (fprint_sym (ofile, rdx, &val, NULL, sim_switches | SIM_SW_REG) > 0))
        fprint_val (ofile, val, rdx, rptr->width, rptr->flags & REG_FMT);
if (flag & EX_I) fprintf (ofile, "\t");
else fprintf (ofile, "\n");
return SCPE_OK;
}

/* Get register value

   Inputs:
        rptr    =       pointer to register descriptor
        idx     =       index
   Outputs:
        return  =       register value
*/

t_value get_rval (REG *rptr, uint32 idx)
{
size_t sz;
t_value val;
UNIT *uptr;

sz = SZ_R (rptr);
if ((rptr->depth > 1) && (rptr->flags & REG_CIRC)) {
    idx = idx + rptr->qptr;
    if (idx >= rptr->depth) idx = idx - rptr->depth;
	}
if ((rptr->depth > 1) && (rptr->flags & REG_UNIT)) {
    uptr = ((UNIT *) rptr->loc) + idx;
#if defined (USE_INT64)
    if (sz <= sizeof (uint32)) val = *((uint32 *) uptr);
    else val = *((t_uint64 *) uptr);
#else
    val = *((uint32 *) uptr);
#endif
    }
else if (((rptr->depth > 1) || (rptr->flags & REG_FIT)) &&
    (sz == sizeof (uint8)))
    val = *(((uint8 *) rptr->loc) + idx);
else if (((rptr->depth > 1) || (rptr->flags & REG_FIT)) &&
    (sz == sizeof (uint16)))
    val = *(((uint16 *) rptr->loc) + idx);
#if defined (USE_INT64)
else if (sz <= sizeof (uint32))
     val = *(((uint32 *) rptr->loc) + idx);
else val = *(((t_uint64 *) rptr->loc) + idx);
#else
else val = *(((uint32 *) rptr->loc) + idx);
#endif
val = (val >> rptr->offset) & width_mask[rptr->width];
return val;
}

/* Deposit register routine

   Inputs:
        flag    =       type of deposit (normal/interactive)
        cptr    =       pointer to input string
        rptr    =       pointer to register descriptor
        idx     =       index
   Outputs:
        return  =       error status
*/

t_stat dep_reg (int32 flag, char *cptr, REG *rptr, uint32 idx)
{
t_stat r;
t_value val, mask;
int32 rdx;
char *tptr, gbuf[CBUFSIZE];

if ((cptr == NULL) || (rptr == NULL)) return SCPE_IERR;
if (rptr->flags & REG_RO) return SCPE_RO;
if (flag & EX_I) {
    cptr = read_line (gbuf, CBUFSIZE, stdin);
    if (sim_log) fprintf (sim_log, (cptr? "%s\n": "\n"), cptr);
    if (cptr == NULL) return 1;                         /* force exit */
    if (*cptr == 0) return SCPE_OK;                     /* success */
    }
mask = width_mask[rptr->width];
GET_RADIX (rdx, rptr->radix);
if ((rptr->flags & REG_VMAD) && sim_vm_parse_addr) {    /* address form? */
    val = sim_vm_parse_addr (sim_dflt_dev, cptr, &tptr);
    if ((tptr == cptr) || (*tptr != 0) ||
        (val > mask)) return SCPE_ARG;
    }
else if (!(rptr->flags & REG_VMIO) ||                   /* dont use sym? */
    (parse_sym (cptr, rdx, NULL, &val, sim_switches | SIM_SW_REG) > SCPE_OK)) {
    val = get_uint (cptr, rdx, mask, &r);
    if (r != SCPE_OK) return SCPE_ARG;
    }
if ((rptr->flags & REG_NZ) && (val == 0)) return SCPE_ARG;
put_rval (rptr, idx, val);
return SCPE_OK;
}

/* Put register value

   Inputs:
        rptr    =       pointer to register descriptor
        idx     =       index
        val     =       new value
        mask    =       mask
   Outputs:
        none
*/

void put_rval (REG *rptr, uint32 idx, t_value val)
{
size_t sz;
t_value mask;
UNIT *uptr;

#define PUT_RVAL(sz,rp,id,v,m) \
    *(((sz *) rp->loc) + id) = \
            (*(((sz *) rp->loc) + id) & \
            ~((m) << (rp)->offset)) | ((v) << (rp)->offset)

if (rptr == sim_PC) sim_brk_npc (0);
sz = SZ_R (rptr);
mask = width_mask[rptr->width];
if ((rptr->depth > 1) && (rptr->flags & REG_CIRC)) {
    idx = idx + rptr->qptr;
    if (idx >= rptr->depth) idx = idx - rptr->depth;
    }
if ((rptr->depth > 1) && (rptr->flags & REG_UNIT)) {
    uptr = ((UNIT *) rptr->loc) + idx;
#if defined (USE_INT64)
    if (sz <= sizeof (uint32))
        *((uint32 *) uptr) = (*((uint32 *) uptr) &
        ~(((uint32) mask) << rptr->offset)) | 
        (((uint32) val) << rptr->offset);
    else *((t_uint64 *) uptr) = (*((t_uint64 *) uptr)
        & ~(mask << rptr->offset)) | (val << rptr->offset);
#else
    *((uint32 *) uptr) = (*((uint32 *) uptr) &
        ~(((uint32) mask) << rptr->offset)) | 
        (((uint32) val) << rptr->offset);
#endif
    }
else if (((rptr->depth > 1) || (rptr->flags & REG_FIT)) &&
    (sz == sizeof (uint8)))
    PUT_RVAL (uint8, rptr, idx, (uint32) val, (uint32) mask);
else if (((rptr->depth > 1) || (rptr->flags & REG_FIT)) &&
    (sz == sizeof (uint16)))
    PUT_RVAL (uint16, rptr, idx, (uint32) val, (uint32) mask);
#if defined (USE_INT64)
else if (sz <= sizeof (uint32))
    PUT_RVAL (uint32, rptr, idx, (int32) val, (uint32) mask);
else PUT_RVAL (t_uint64, rptr, idx, val, mask);
#else
else PUT_RVAL (uint32, rptr, idx, val, mask);
#endif
return;
}

/* Examine address routine

   Inputs: (sim_eval is an implicit argument)
        ofile   =       output stream
        flag    =       type of ex/mod command (ex, iex, idep)
        addr    =       address to examine
        dptr    =       pointer to device
        uptr    =       pointer to unit
   Outputs:
        return  =       if > 0, error status
                        if <= 0,-number of extra addr units retired
*/

t_stat ex_addr (FILE *ofile, int32 flag, t_addr addr, DEVICE *dptr, UNIT *uptr)
{
t_stat reason;
int32 rdx;

if (sim_vm_fprint_addr) sim_vm_fprint_addr (ofile, dptr, addr);
else fprint_val (ofile, addr, dptr->aradix, dptr->awidth, PV_LEFT);
fprintf (ofile, ":\t");
if (!(flag & EX_E)) return (1 - dptr->aincr);

GET_RADIX (rdx, dptr->dradix);
if ((reason = fprint_sym (ofile, addr, sim_eval, uptr, sim_switches)) > 0) {
    fprint_val (ofile, sim_eval[0], rdx, dptr->dwidth, PV_RZRO);
    reason = 1 - dptr->aincr;
    }
if (flag & EX_I) fprintf (ofile, "\t");
else fprintf (ofile, "\n");
return reason;
}

/* Get address routine

   Inputs:
        flag    =       type of ex/mod command (ex, iex, idep)
        addr    =       address to examine
        dptr    =       pointer to device
        uptr    =       pointer to unit
   Outputs: (sim_eval is an implicit output)
        return  =       error status
*/

t_stat get_aval (t_addr addr, DEVICE *dptr, UNIT *uptr)
{
int32 i;
t_value mask;
t_addr j, loc;
size_t sz;
t_stat reason = SCPE_OK;

if ((dptr == NULL) || (uptr == NULL)) return SCPE_IERR;
mask = width_mask[dptr->dwidth];
for (i = 0; i < sim_emax; i++) sim_eval[i] = 0;
for (i = 0, j = addr; i < sim_emax; i++, j = j + dptr->aincr) {
    if (dptr->examine != NULL) {
        reason = dptr->examine (&sim_eval[i], j, uptr, sim_switches);
        if (reason != SCPE_OK) break;
        }
    else {
        if (!(uptr->flags & UNIT_ATT)) return SCPE_UNATT;
        if (uptr->flags & UNIT_RAW) return SCPE_NOFNC;
        if ((uptr->flags & UNIT_FIX) && (j >= uptr->capac)) {
            reason = SCPE_NXM;
            break;
            }
        sz = SZ_D (dptr);
        loc = j / dptr->aincr;
        if (uptr->flags & UNIT_BUF) {
            SZ_LOAD (sz, sim_eval[i], uptr->filebuf, loc);
            }
        else {
            sim_fseek (uptr->fileref, sz * loc, SEEK_SET);
            sim_fread (&sim_eval[i], sz, 1, uptr->fileref);
            if ((feof (uptr->fileref)) &&
               !(uptr->flags & UNIT_FIX)) {
                reason = SCPE_EOF;
                break;
                }
            else if (ferror (uptr->fileref)) {
                clearerr (uptr->fileref);
                reason = SCPE_IOERR;
                break;
                }
            }
        }
    sim_eval[i] = sim_eval[i] & mask;
    }
if ((reason != SCPE_OK) && (i == 0)) return reason;
return SCPE_OK;
}

/* Deposit address routine

   Inputs:
        flag    =       type of deposit (normal/interactive)
        cptr    =       pointer to input string
        addr    =       address to examine
        dptr    =       pointer to device
        uptr    =       pointer to unit
        dfltinc =       value to return on cr input
   Outputs:
        return  =       if > 0, error status
                        if <= 0, -number of extra address units retired
*/

t_stat dep_addr (int32 flag, char *cptr, t_addr addr, DEVICE *dptr,
    UNIT *uptr, int32 dfltinc)
{
int32 i, count, rdx;
t_addr j, loc;
t_stat r, reason;
t_value mask;
size_t sz;
char gbuf[CBUFSIZE];

if (dptr == NULL) return SCPE_IERR;
if (flag & EX_I) {
    cptr = read_line (gbuf, CBUFSIZE, stdin);
    if (sim_log) fprintf (sim_log, (cptr? "%s\n": "\n"), cptr);
    if (cptr == NULL) return 1;                         /* force exit */
    if (*cptr == 0) return dfltinc;                     /* success */
    }
if (uptr->flags & UNIT_RO) return SCPE_RO;              /* read only? */
mask = width_mask[dptr->dwidth];

GET_RADIX (rdx, dptr->dradix);
if ((reason = parse_sym (cptr, addr, uptr, sim_eval, sim_switches)) > 0) {
    sim_eval[0] = get_uint (cptr, rdx, mask, &reason);
    if (reason != SCPE_OK) return reason;
    }
count = (1 - reason + (dptr->aincr - 1)) / dptr->aincr;

for (i = 0, j = addr; i < count; i++, j = j + dptr->aincr) {
    sim_eval[i] = sim_eval[i] & mask;
    if (dptr->deposit != NULL) {
        r = dptr->deposit (sim_eval[i], j, uptr, sim_switches);
        if (r != SCPE_OK) return r;
        }
    else {
        if (!(uptr->flags & UNIT_ATT)) return SCPE_UNATT;
        if (uptr->flags & UNIT_RAW) return SCPE_NOFNC;
        if ((uptr->flags & UNIT_FIX) && (j >= uptr->capac))
            return SCPE_NXM;
        sz = SZ_D (dptr);
        loc = j / dptr->aincr;
        if (uptr->flags & UNIT_BUF) {
            SZ_STORE (sz, sim_eval[i], uptr->filebuf, loc);
            if (loc >= uptr->hwmark) uptr->hwmark = (uint32) loc + 1;
            }
        else {
            sim_fseek (uptr->fileref, sz * loc, SEEK_SET);
            sim_fwrite (&sim_eval[i], sz, 1, uptr->fileref);
            if (ferror (uptr->fileref)) {
                clearerr (uptr->fileref);
                return SCPE_IOERR;
                }
            }
        }
    }
return reason;
}

/* Evaluate command */

t_stat eval_cmd (int32 flg, char *cptr)
{
DEVICE *dptr = sim_dflt_dev;
int32 i, rdx, a, lim;
t_stat r;

GET_SWITCHES (cptr);
GET_RADIX (rdx, dptr->dradix);
for (i = 0; i < sim_emax; i++) sim_eval[i] = 0;
if (*cptr == 0) return SCPE_2FARG;
if ((r = parse_sym (cptr, 0, dptr->units, sim_eval, sim_switches)) > 0) {
    sim_eval[0] = get_uint (cptr, rdx, width_mask[dptr->dwidth], &r);
    if (r != SCPE_OK) return r;
    }
lim = 1 - r;
for (i = a = 0; a < lim; ) {
    printf ("%d:\t", a);
    if ((r = fprint_sym (stdout, a, &sim_eval[i], dptr->units, sim_switches)) > 0)
        r = fprint_val (stdout, sim_eval[i], rdx, dptr->dwidth, PV_RZRO);
    printf ("\n");
    if (sim_log) {
        fprintf (sim_log, "%d\t", i);
        if ((r = fprint_sym (sim_log, a, &sim_eval[i], dptr->units, sim_switches)) > 0)
            r = fprint_val (sim_log, sim_eval[i], rdx, dptr->dwidth, PV_RZRO);
        fprintf (sim_log, "\n");
        }
    if (r < 0) a = a + 1 - r;
    else a = a + dptr->aincr;
    i = a / dptr->aincr;
    }
return SCPE_OK;
}

/* String processing routines

   read_line            read line

   Inputs:
        cptr    =       pointer to buffer
        size    =       maximum size
        stream  =       pointer to input stream
   Outputs:
        optr    =       pointer to first non-blank character
                        NULL if EOF
*/

char *read_line (char *cptr, int32 size, FILE *stream)
{
char *tptr;

#if !defined (HAVE_READLINE)
cptr = fgets (cptr, size, stream);                      /* get cmd line */

#else
if (stream != stdin) cptr = fgets (cptr, size, stream);
else {
    char *tmpc = readline (NULL);
    if (tmpc == NULL) cptr = NULL;
    else {
        strncpy (cptr, tmpc, size);
        free (tmpc);
        }
    }
#endif

if (cptr == NULL) {
    clearerr (stream);                                  /* clear error */
    return NULL;                                        /* ignore EOF */
    }
for (tptr = cptr; tptr < (cptr + size); tptr++) {       /* remove cr or nl */
    if ((*tptr == '\n') || (*tptr == '\r') ||
        (tptr == (cptr + size - 1))) {                  /* str max length? */
        *tptr = 0;                                      /* terminate */
        break;
        }
    }
while (isspace (*cptr)) cptr++;                         /* trim leading spc */
if (*cptr == ';') *cptr = 0;                            /* ignore comment */

#if defined (HAVE_READLINE)
add_history (cptr);

#endif
return cptr;
}

/* get_glyph            get next glyph (force upper case)
   get_glyph_nc         get next glyph (no conversion)
   get_glyph_gen        get next glyph (general case)

   Inputs:
        iptr    =       pointer to input string
        optr    =       pointer to output string
        mchar   =       optional end of glyph character
        flag    =       TRUE for convert to upper case (_gen only)
   Outputs
        result  =       pointer to next character in input string
*/

char *get_glyph_gen (char *iptr, char *optr, char mchar, t_bool uc)
{
while ((isspace (*iptr) == 0) && (*iptr != 0) && (*iptr != mchar)) {
    if (islower (*iptr) && uc) *optr = toupper (*iptr);
    else *optr = *iptr;
    iptr++; optr++;
    }
*optr = 0;
if (mchar && (*iptr == mchar)) iptr++;                  /* skip terminator */
while (isspace (*iptr)) iptr++;                         /* absorb spaces */
return iptr;
}

char *get_glyph (char *iptr, char *optr, char mchar)
{
return get_glyph_gen (iptr, optr, mchar, TRUE);
}

char *get_glyph_nc (char *iptr, char *optr, char mchar)
{
return get_glyph_gen (iptr, optr, mchar, FALSE);
}

/* Trim trailing spaces from a string

    Inputs:
        cptr    =       pointer to string
    Outputs:
        cptr    =       pointer to string
*/

char *sim_trim_endspc (char *cptr)
{
char *tptr;

tptr = cptr + strlen (cptr);
while ((--tptr >= cptr) && isspace (*tptr)) *tptr = 0;
return cptr;
}

/* get_yn               yes/no question

   Inputs:
        cptr    =       pointer to question
        deflt   =       default answer
   Outputs:
        result  =       true if yes, false if no
*/

t_stat get_yn (char *ques, t_stat deflt)
{
char cbuf[CBUFSIZE], *cptr;

printf ("%s ", ques);
cptr = read_line (cbuf, CBUFSIZE, stdin);
if ((cptr == NULL) || (*cptr == 0)) return deflt;
if ((*cptr == 'Y') || (*cptr == 'y')) return TRUE;
return FALSE;
}

/* get_uint             unsigned number

   Inputs:
        cptr    =       pointer to input string
        radix   =       input radix
        max     =       maximum acceptable value
        *status =       pointer to error status
   Outputs:
        val     =       value
*/

t_value get_uint (char *cptr, uint32 radix, t_value max, t_stat *status)
{
t_value val;
char *tptr;

*status = SCPE_OK;
val = strtotv (cptr, &tptr, radix);
if ((cptr == tptr) || (val > max)) *status = SCPE_ARG;
else {
    while (isspace (*tptr)) tptr++;
    if (*tptr != 0) *status = SCPE_ARG;
    }
return val;
}

/* get_range            range specification

   Inputs:
        dptr    =       pointer to device (NULL if none)
        cptr    =       pointer to input string
        *lo     =       pointer to low result
        *hi     =       pointer to high result
        aradix  =       radix
        max     =       default high value
        term    =       terminating character, 0 if none
   Outputs:
        tptr    =       input pointer after processing
                        NULL if error
*/

char *get_range (DEVICE *dptr, char *cptr, t_addr *lo, t_addr *hi,
    uint32 rdx, t_addr max, char term)
{
char *tptr;

if (max && strncmp (cptr, "ALL", strlen ("ALL")) == 0) { /* ALL? */
    tptr = cptr + strlen ("ALL");
    *lo = 0;
    *hi = max;
    }
else {
    if (dptr && sim_vm_parse_addr)                      /* get low */
        *lo = sim_vm_parse_addr (dptr, cptr, &tptr);
    else *lo = (t_addr) strtotv (cptr, &tptr, rdx);
    if (cptr == tptr) return NULL;                      /* error? */
    if ((*tptr == '-') || (*tptr == ':')) {             /* range? */
        cptr = tptr + 1;
        if (dptr && sim_vm_parse_addr)                  /* get high */
            *hi = sim_vm_parse_addr (dptr, cptr, &tptr);
        else *hi = (t_addr) strtotv (cptr, &tptr, rdx);
        if (cptr == tptr) return NULL;
        if (*lo > *hi) return NULL;
        }
    else if (*tptr == '/') {                            /* relative? */
        cptr = tptr + 1;
        *hi = (t_addr) strtotv (cptr, &tptr, rdx);      /* get high */
        if ((cptr == tptr) || (*hi == 0)) return NULL;
        *hi = *lo + *hi - 1;
        }
    else *hi = *lo;
    }
if (term && (*tptr++ != term)) return NULL;
return tptr;
}

/* get_ipaddr           IP address:port

   Inputs:
        cptr    =       pointer to input string
   Outputs:
        ipa     =       pointer to IP address (may be NULL), 0 = none
        ipp     =       pointer to IP port (may be NULL)
        result  =       status
*/

t_stat get_ipaddr (char *cptr, uint32 *ipa, uint32 *ipp)
{
char gbuf[CBUFSIZE];
char *addrp, *portp, *octetp;
uint32 i, addr, port, octet;
t_stat r;

if ((cptr == NULL) || (*cptr == 0) || (ipa == NULL) || (ipp == NULL))
    return SCPE_ARG;
strncpy (gbuf, cptr, CBUFSIZE);
addrp = gbuf;                                           /* default addr */
if (portp = strchr (gbuf, ':')) *portp++ = '.';         /* x:y? */
else if (strchr (gbuf, '.')) portp = NULL;              /* x.y...? */
else {
    portp = gbuf;                                       /* port only */
    addrp = NULL;                                       /* x is port */
    }
if (portp) {                                            /* port string? */
    port = (int32) get_uint (portp, 10, 65535, &r);
    if ((r != SCPE_OK) || (port == 0)) return SCPE_ARG;
    *ipp = port;
    }
else *ipp = 0;
if (addrp) {
    for (i = addr = 0; i < 4; i++) {
        octetp = strchr (addrp, '.');
        if (octetp == NULL) return SCPE_ARG;
        *octetp++ = 0;
        octet = (int32) get_uint (addrp, 10, 255, &r);
        if (r != SCPE_OK) return SCPE_ARG;
        addr = (addr << 8) | octet;
        addrp = octetp;
        }
    if (((addr & 0377) == 0) || ((addr & 0377) == 255))
        return SCPE_ARG;
    *ipa = addr;
    }
else *ipa = 0;
return SCPE_OK;   
}

/* Find_device          find device matching input string

   Inputs:
        cptr    =       pointer to input string
   Outputs:
        result  =       pointer to device
*/

DEVICE *find_dev (char *cptr)
{
int32 i;
DEVICE *dptr;

for (i = 0; (dptr = sim_devices[i]) != NULL; i++) {
    if ((strcmp (cptr, dptr->name) == 0) ||
        (dptr->lname &&
        (strcmp (cptr, dptr->lname) == 0))) return dptr;
    }
return NULL;
}

/* Find_unit            find unit matching input string

   Inputs:
        cptr    =       pointer to input string
        uptr    =       pointer to unit pointer
   Outputs:
        result  =       pointer to device (null if no dev)
        *iptr   =       pointer to unit (null if nx unit)
*/

DEVICE *find_unit (char *cptr, UNIT **uptr)
{
uint32 i, u;
char *nptr, *tptr;
t_stat r;
DEVICE *dptr;

if (uptr == NULL) return NULL;                          /* arg error? */
if (dptr = find_dev (cptr)) {                           /* exact match? */
    if (qdisable (dptr)) return NULL;                   /* disabled? */
    *uptr = dptr->units;                                /* unit 0 */
    return dptr;
    }

for (i = 0; (dptr = sim_devices[i]) != NULL; i++) {     /* base + unit#? */
    if (dptr->numunits &&                               /* any units? */
        (((nptr = dptr->name) &&
          (strncmp (cptr, nptr, strlen (nptr)) == 0)) ||
         ((nptr = dptr->lname) &&
          (strncmp (cptr, nptr, strlen (nptr)) == 0)))) {
        tptr = cptr + strlen (nptr);
        if (isdigit (*tptr)) {
            if (qdisable (dptr)) return NULL;           /* disabled? */
                u = (uint32) get_uint (tptr, 10, dptr->numunits - 1, &r);
            if (r != SCPE_OK) *uptr = NULL;             /* error? */
            else *uptr = dptr->units + u;
            return dptr;
            }
        }
    }
return NULL;
}

/* Find_dev_from_unit   find device for unit

   Inputs:
        uptr    =       pointer to unit
   Outputs:
        result  =       pointer to device
*/

DEVICE *find_dev_from_unit (UNIT *uptr)
{
DEVICE *dptr;
uint32 i, j;

if (uptr == NULL) return NULL;
for (i = 0; (dptr = sim_devices[i]) != NULL; i++) {
    for (j = 0; j < dptr->numunits; j++) {
        if (uptr == (dptr->units + j)) return dptr;
        }
    }
return NULL;
}

/* Test for disabled device */

t_bool qdisable (DEVICE *dptr)
{
return (dptr->flags & DEV_DIS? TRUE: FALSE);
}

/* find_reg_glob        find globally unique register

   Inputs:
        cptr    =       pointer to input string
        optr    =       pointer to output pointer (can be null)
        gdptr   =       pointer to global device
   Outputs:
        result  =       pointer to register, NULL if error
        *optr   =       pointer to next character in input string
        *gdptr  =       pointer to device where found
*/

REG *find_reg_glob (char *cptr, char **optr, DEVICE **gdptr)
{
int32 i;
DEVICE *dptr;
REG *rptr, *srptr = NULL;

for (i = 0; (dptr = sim_devices[i]) != 0; i++) {        /* all dev */
    if (dptr->flags & DEV_DIS) continue;                /* skip disabled */
    if (rptr = find_reg (cptr, optr, dptr)) {           /* found? */
        if (srptr) return NULL;                         /* ambig? err */
        srptr = rptr;                                   /* save reg */
        *gdptr = dptr;                                  /* save unit */
        }
    }
return srptr;
}

/* find_reg             find register matching input string

   Inputs:
        cptr    =       pointer to input string
        optr    =       pointer to output pointer (can be null)
        dptr    =       pointer to device
   Outputs:
        result  =       pointer to register, NULL if error
        *optr   =       pointer to next character in input string
*/

REG *find_reg (char *cptr, char **optr, DEVICE *dptr)
{
char *tptr;
REG *rptr;
uint32 slnt;

if ((cptr == NULL) || (dptr == NULL) ||
    (dptr->registers == NULL)) return NULL;
tptr = cptr;
do {
    tptr++;
    } while (isalnum (*tptr) || (*tptr == '*') || (*tptr == '_'));
slnt = tptr - cptr;
for (rptr = dptr->registers; rptr->name != NULL; rptr++) {
    if ((slnt == strlen (rptr->name)) &&
        (strncmp (cptr, rptr->name, slnt) == 0)) {
        if (optr != NULL) *optr = tptr;
        return rptr;
        }
    }
return NULL;
}

/* get_switches         get switches from input string

   Inputs:
        cptr    =       pointer to input string
   Outputs:
        sw      =       switch bit mask
                        0 if no switches, -1 if error
*/

int32 get_switches (char *cptr)
{
int32 sw;

if (*cptr != '-') return 0;
sw = 0;
for (cptr++; (isspace (*cptr) == 0) && (*cptr != 0); cptr++) {
    if (isalpha (*cptr) == 0) return -1;
    sw = sw | SWMASK (toupper (*cptr));
    }
return sw;
}

/* get_sim_sw           accumulate sim_switches

   Inputs:
        cptr    =       pointer to input string
   Outputs:
        ptr     =       pointer to first non-string glyph
                        NULL if error
*/

char *get_sim_sw (char *cptr)
{
int32 lsw;
char gbuf[CBUFSIZE];

while (*cptr == '-') {                                  /* while switches */
    cptr = get_glyph (cptr, gbuf, 0);                   /* get switch glyph */
    lsw = get_switches (gbuf);                          /* parse */
    if (lsw <= 0) return NULL;                          /* invalid? */
    sim_switches = sim_switches | lsw;                  /* accumulate */
    }
return cptr;
}

/* get_sim_opt          get simulator command options

   Inputs:
        opt     =       command options
        cptr    =       pointer to input string
   Outputs:
        ptr     =       pointer to next glypsh, NULL if error
        *stat   =       error status
*/

char *get_sim_opt (int32 opt, char *cptr, t_stat *st)
{
int32 t;
char *svptr, gbuf[CBUFSIZE];
DEVICE *tdptr;
UNIT *tuptr;

sim_switches = 0;                                       /* no switches */
sim_ofile = NULL;                                       /* no output file */
sim_schptr = NULL;                                      /* no search */
sim_stab.logic = SCH_OR;                                /* default search params */
sim_stab.boolop = SCH_GE;
sim_stab.mask = 0;
sim_stab.comp = 0;
sim_dfdev = sim_dflt_dev;
sim_dfunit = sim_dfdev->units;
*st = SCPE_OK;
while (*cptr) {                                         /* loop through modifiers */
    svptr = cptr;                                       /* save current position */
    if ((opt & CMD_OPT_OF) && (*cptr == '@')) {         /* output file spec? */
        if (sim_ofile) {                                /* already got one? */
            fclose (sim_ofile);                         /* one per customer */
            *st = SCPE_ARG;
            return NULL;
			}
        cptr = get_glyph_nc (cptr + 1, gbuf, 0);
        sim_ofile = sim_fopen (gbuf, "a");              /* open for append */
        if (sim_ofile) continue;                        /* if ok, look for more */
        *st = SCPE_OPENERR;                             /* open failed */
        return NULL;
        }
    cptr = get_glyph (cptr, gbuf, 0);
    if ((t = get_switches (gbuf)) != 0) {               /* try for switches */
        if (t < 0) {                                    /* err if bad switch */
            *st = SCPE_INVSW;
            return NULL;
            }
        sim_switches = sim_switches | t;                /* or in new switches */
        }
    else if ((opt & CMD_OPT_SCH) &&                     /* if allowed, */
        get_search (gbuf, sim_dfdev->dradix, &sim_stab))        /* try for search */
        sim_schptr = &sim_stab;                         /* set search */
    else if ((opt & CMD_OPT_DFT) &&                     /* if allowed, */
        (tdptr = find_unit (gbuf, &tuptr)) &&           /* try for default */
        (tuptr != NULL)) {
        sim_dfdev = tdptr;                              /* set as default */
        sim_dfunit = tuptr;
        }
    else return svptr;                                  /* not rec, break out */
    }
return cptr;
}

/* Match file extension

   Inputs:
        fnam    =       file name
        ext     =       extension, without period
   Outputs:
        cp      =       pointer to final '.' if match, NULL if not
*/

char *match_ext (char *fnam, char *ext)
{
char *pptr, *fptr, *eptr;

if ((fnam == NULL) || (ext == NULL)) return NULL;
pptr = strrchr (fnam, '.');
if (pptr) {
    for (fptr = pptr + 1, eptr = ext; *fptr; fptr++, eptr++) {
        if (toupper (*fptr) != toupper (*eptr)) return NULL;
		}
    if (*eptr) return NULL;
    }
return pptr;
}

/* Get search specification

   Inputs:
        cptr    =       pointer to input string
        radix   =       radix for numbers
        schptr =        pointer to search table
   Outputs:
        return =        NULL if error
                        schptr if valid search specification
*/

SCHTAB *get_search (char *cptr, int32 radix, SCHTAB *schptr)
{
int32 c, logop, cmpop;
t_value logval, cmpval;
char *sptr, *tptr;
const char logstr[] = "|&^", cmpstr[] = "=!><";

logval = cmpval = 0;
if (*cptr == 0) return NULL;                            /* check for clause */
for (logop = cmpop = -1; c = *cptr++; ) {               /* loop thru clauses */
    if (sptr = strchr (logstr, c)) {                    /* check for mask */
        logop = sptr - logstr;
        logval = strtotv (cptr, &tptr, radix);
        if (cptr == tptr) return NULL;
        cptr = tptr;
        }
    else if (sptr = strchr (cmpstr, c)) {               /* check for boolop */
        cmpop = sptr - cmpstr;
        if (*cptr == '=') {
            cmpop = cmpop + strlen (cmpstr);
            cptr++;
            }
        cmpval = strtotv (cptr, &tptr, radix);
        if (cptr == tptr) return NULL;
        cptr = tptr;
        }
    else return NULL;
    }                                                   /* end for */
if (logop >= 0) {
    schptr->logic = logop;
    schptr->mask = logval;
    }
if (cmpop >= 0) {
    schptr->boolop = cmpop;
    schptr->comp = cmpval;
    }
return schptr;
}

/* Test value against search specification

   Inputs:
        val     =       value to test
        schptr =        pointer to search table
   Outputs:
        return =        1 if value passes search criteria, 0 if not
*/

int32 test_search (t_value val, SCHTAB *schptr)
{
if (schptr == NULL) return 0;

switch (schptr->logic) {                                /* case on logical */

    case SCH_OR:
        val = val | schptr->mask;
        break;

    case SCH_AND:
        val = val & schptr->mask;
        break;

    case SCH_XOR:
        val = val ^ schptr->mask;
        break;
        }

switch (schptr->boolop) {                                       /* case on comparison */

    case SCH_E: case SCH_EE:
        return (val == schptr->comp);

    case SCH_N: case SCH_NE:
        return (val != schptr->comp);

    case SCH_G:
        return (val > schptr->comp);

    case SCH_GE:
        return (val >= schptr->comp);

    case SCH_L:
        return (val < schptr->comp);

    case SCH_LE:
        return (val <= schptr->comp);
        }

return 0;
}

/* Radix independent input/output package

   strtotv - general radix input routine

   Inputs:
        inptr   =       string to convert
        endptr  =       pointer to first unconverted character
        radix   =       radix for input
   Outputs:
        value   =       converted value

   On an error, the endptr will equal the inptr.
*/

t_value strtotv (char *inptr, char **endptr, uint32 radix)
{
int32 nodigit;
t_value val;
uint32 c, digit;

*endptr = inptr;                                        /* assume fails */
if ((radix < 2) || (radix > 36)) return 0;
while (isspace (*inptr)) inptr++;                       /* bypass white space */
val = 0;
nodigit = 1;
for (c = *inptr; isalnum(c); c = *++inptr) {            /* loop through char */
    if (islower (c)) c = toupper (c);
    if (isdigit (c)) digit = c - (uint32) '0';          /* digit? */
    else digit = c + 10 - (uint32) 'A';                 /* no, letter */
    if (digit >= radix) return 0;                       /* valid in radix? */
    val = (val * radix) + digit;                        /* add to value */
    nodigit = 0;
    }
if (nodigit) return 0;                                  /* no digits? */
*endptr = inptr;                                        /* result pointer */
return val;
}

/* fprint_val - general radix printing routine

   Inputs:
        stream  =       stream designator
        val     =       value to print
        radix   =       radix to print
        width   =       width to print
        format  =       leading zeroes format
   Outputs:
        status  =       error status
*/

t_stat fprint_val (FILE *stream, t_value val, uint32 radix,
    uint32 width, uint32 format)
{
#define MAX_WIDTH ((int) (CHAR_BIT * sizeof (t_value)))
t_value owtest, wtest;
int32 d, digit, ndigits;
char dbuf[MAX_WIDTH + 1];

for (d = 0; d < MAX_WIDTH; d++) dbuf[d] = (format == PV_RZRO)? '0': ' ';
dbuf[MAX_WIDTH] = 0;
d = MAX_WIDTH;
do {
    d = d - 1;
    digit = (int32) (val % radix);
    val = val / radix;
    dbuf[d] = (digit <= 9)? '0' + digit: 'A' + (digit - 10);
    } while ((d > 0) && (val != 0));

if (format != PV_LEFT) {
    wtest = owtest = radix;
    ndigits = 1;
    while ((wtest < width_mask[width]) && (wtest >= owtest)) {
        owtest = wtest;
        wtest = wtest * radix;
        ndigits = ndigits + 1;
        }
    if ((MAX_WIDTH - ndigits) < d) d = MAX_WIDTH - ndigits;
    }
if (fputs (&dbuf[d], stream) == EOF) return SCPE_IOERR;
return SCPE_OK;
}

/* Event queue package

        sim_activate            add entry to event queue
        sim_cancel              remove entry from event queue
        sim_process_event       process entries on event queue
        sim_is_active           see if entry is on event queue
        sim_atime               return absolute time for an entry
        sim_gtime               return global time
        sim_qcount              return event queue entry count

   Asynchronous events are set up by queueing a unit data structure
   to the event queue with a timeout (in simulator units, relative
   to the current time).  Each simulator 'times' these events by
   counting down interval counter sim_interval.  When this reaches
   zero the simulator calls sim_process_event to process the event
   and to see if further events need to be processed, or sim_interval
   reset to count the next one.

   The event queue is maintained in clock order; entry timeouts are
   RELATIVE to the time in the previous entry.

   sim_process_event - process event

   Inputs:
        none
   Outputs:
        reason  =       reason code returned by any event processor,
                        or 0 (SCPE_OK) if no exceptions
*/

t_stat sim_process_event (void)
{
UNIT *uptr;
t_stat reason;

if (stop_cpu) return SCPE_STOP;                         /* stop CPU? */
if (sim_clock_queue == NULL) {                          /* queue empty? */
    UPDATE_SIM_TIME (noqueue_time);                     /* update sim time */
    sim_interval = noqueue_time = NOQUEUE_WAIT;         /* flag queue empty */
    return SCPE_OK;
    }
UPDATE_SIM_TIME (sim_clock_queue->time);                /* update sim time */
do {
    uptr = sim_clock_queue;                             /* get first */
    sim_clock_queue = uptr->next;                       /* remove first */
    uptr->next = NULL;                                  /* hygiene */
    uptr->time = 0;
    if (sim_clock_queue != NULL) sim_interval = sim_clock_queue->time;
    else sim_interval = noqueue_time = NOQUEUE_WAIT;
    if (uptr->action != NULL) reason = uptr->action (uptr);
    else reason = SCPE_OK;
    } while ((reason == SCPE_OK) && (sim_interval == 0));

/* Empty queue forces sim_interval != 0 */

return reason;
}

/* sim_activate - activate (queue) event

   Inputs:
        uptr    =       pointer to unit
        event_time =    relative timeout
   Outputs:
        reason  =       result (SCPE_OK if ok)
*/

t_stat sim_activate (UNIT *uptr, int32 event_time)
{
UNIT *cptr, *prvptr;
int32 accum;

if (event_time < 0) return SCPE_IERR;
if (sim_is_active (uptr)) return SCPE_OK;               /* already active? */
if (sim_clock_queue == NULL) { UPDATE_SIM_TIME (noqueue_time); }
else  { UPDATE_SIM_TIME (sim_clock_queue->time); }      /* update sim time */

prvptr = NULL;
accum = 0;
for (cptr = sim_clock_queue; cptr != NULL; cptr = cptr->next) {
    if (event_time < (accum + cptr->time)) break;
    accum = accum + cptr->time;
    prvptr = cptr;
    }
if (prvptr == NULL) {                                   /* insert at head */
    cptr = uptr->next = sim_clock_queue;
    sim_clock_queue = uptr;
    }
else {
    cptr = uptr->next = prvptr->next;                   /* insert at prvptr */
    prvptr->next = uptr;
    }
uptr->time = event_time - accum;
if (cptr != NULL) cptr->time = cptr->time - uptr->time;
sim_interval = sim_clock_queue->time;
return SCPE_OK;
}

/* sim_activate_abs - activate (queue) event even if event already scheduled

   Inputs:
        uptr    =       pointer to unit
        event_time =    relative timeout
   Outputs:
        reason  =       result (SCPE_OK if ok)
*/

t_stat sim_activate_abs (UNIT *uptr, int32 event_time)
{
sim_cancel (uptr);
return sim_activate (uptr, event_time);
}

/* sim_cancel - cancel (dequeue) event

   Inputs:
        uptr    =       pointer to unit
   Outputs:
        reason  =       result (SCPE_OK if ok)

*/

t_stat sim_cancel (UNIT *uptr)
{
UNIT *cptr, *nptr;

if (sim_clock_queue == NULL) return SCPE_OK;
UPDATE_SIM_TIME (sim_clock_queue->time);                /* update sim time */
nptr = NULL;
if (sim_clock_queue == uptr) nptr = sim_clock_queue = uptr->next;
else {
    for (cptr = sim_clock_queue; cptr != NULL; cptr = cptr->next) {
        if (cptr->next == uptr) {
            nptr = cptr->next = uptr->next;
            break;                                      /* end queue scan */
            }
        }
    }
if (nptr != NULL) nptr->time = nptr->time + uptr->time;
uptr->next = NULL;                                      /* hygiene */
uptr->time = 0;
if (sim_clock_queue != NULL) sim_interval = sim_clock_queue->time;
else sim_interval = noqueue_time = NOQUEUE_WAIT;
return SCPE_OK;
}

/* sim_is_active - test for entry in queue, return activation time

   Inputs:
        uptr    =       pointer to unit
   Outputs:
        result =        absolute activation time + 1, 0 if inactive
*/

int32 sim_is_active (UNIT *uptr)
{
UNIT *cptr;
int32 accum;

accum = 0;
for (cptr = sim_clock_queue; cptr != NULL; cptr = cptr->next) {
    accum = accum + cptr->time;
    if (cptr == uptr) return accum + 1;
    }
return 0;
}

/* sim_gtime - return global time
   sim_grtime - return global time with rollover

   Inputs: none
   Outputs:
        time    =       global time
*/

double sim_gtime (void)
{
if (sim_clock_queue == NULL) { UPDATE_SIM_TIME (noqueue_time); }
else  { UPDATE_SIM_TIME (sim_clock_queue->time); }
return sim_time;
}

uint32 sim_grtime (void)
{
if (sim_clock_queue == NULL) { UPDATE_SIM_TIME (noqueue_time); }
else  { UPDATE_SIM_TIME (sim_clock_queue->time); }
return sim_rtime;
}

/* sim_qcount - return queue entry count

   Inputs: none
   Outputs:
        count   =       number of entries on the queue
*/

int32 sim_qcount (void)
{
int32 cnt;
UNIT *uptr;

cnt = 0;
for (uptr = sim_clock_queue; uptr != NULL; uptr = uptr->next) cnt++;
return cnt;
}

/* Breakpoint package.  This module replaces the VM-implemented one
   instruction breakpoint capability.

   Breakpoints are stored in table sim_brk_tab, which is ordered by address for
   efficient binary searching.  A breakpoint consists of a four entry structure:

        addr                    address of the breakpoint
        type                    types of breakpoints set on the address
                                a bit mask representing letters A-Z
        cnt                     number of iterations before breakp is taken
        action                  pointer command string to be executed
                                when break is taken

   sim_brk_summ is a summary of the types of breakpoints that are currently set (it
   is the bitwise OR of all the type fields).  A simulator need only check for
   a breakpoint of type X if bit SWMASK('X') is set in sim_brk_sum.

   The package contains the following public routines:

        sim_brk_init            initialize
        sim_brk_set             set breakpoint
        sim_brk_clr             clear breakpoint
        sim_brk_clrall          clear all breakpoints
        sim_brk_show            show breakpoint
        sim_brk_showall         show all breakpoints
        sim_brk_test            test for breakpoint
        sim_brk_npc             PC has been changed
        sim_brk_getact          get next action
        sim_brk_clract          clear pending actions

   Initialize breakpoint system.
*/

t_stat sim_brk_init (void)
{
sim_brk_lnt = SIM_BRK_INILNT;
sim_brk_tab = (BRKTAB *) calloc (sim_brk_lnt, sizeof (BRKTAB));
if (sim_brk_tab == NULL) return SCPE_MEM;
sim_brk_ent = sim_brk_ins = 0;
sim_brk_act = NULL;
sim_brk_npc (0);
return SCPE_OK;
}

/* Search for a breakpoint in the sorted breakpoint table */

BRKTAB *sim_brk_fnd (t_addr loc)
{
int32 lo, hi, p;
BRKTAB *bp;

if (sim_brk_ent == 0) {                                 /* table empty? */
    sim_brk_ins = 0;                                    /* insrt at head */
    return NULL;                                        /* sch fails */
    }
lo = 0;                                                 /* initial bounds */
hi = sim_brk_ent - 1;
do {
    p = (lo + hi) >> 1;                                 /* probe */
    bp = sim_brk_tab + p;                               /* table addr */
    if (loc == bp->addr) return bp;                     /* match? */
    else if (loc < bp->addr) hi = p - 1;                /* go down? p is upper */
    else lo = p + 1;                                    /* go up? p is lower */
    } while (lo <= hi);
if (loc < bp->addr) sim_brk_ins = p;                    /* insrt before or */
else sim_brk_ins = p + 1;                               /* after last sch */
return NULL;
}

/* Insert a breakpoint */

BRKTAB *sim_brk_new (t_addr loc)
{
int32 i, t;
BRKTAB *bp, *newp;

if (sim_brk_ins < 0) return NULL;
if (sim_brk_ent >= sim_brk_lnt) {                       /* out of space? */
    t = sim_brk_lnt + SIM_BRK_INILNT;                   /* new size */
    newp = (BRKTAB *) calloc (t, sizeof (BRKTAB));      /* new table */
    if (newp == NULL) return NULL;                      /* can't extend */
    for (i = 0; i < sim_brk_lnt; i++)                   /* copy table */
        *(newp + i) = *(sim_brk_tab + i);
    free (sim_brk_tab);                                 /* free old table */
    sim_brk_tab = newp;                                 /* new base, lnt */
    sim_brk_lnt = t;
    }
if (sim_brk_ins != sim_brk_ent) {                       /* move needed? */
    for (bp = sim_brk_tab + sim_brk_ent;
         bp > sim_brk_tab + sim_brk_ins; bp--)
        *bp = *(bp - 1);
    }
bp = sim_brk_tab + sim_brk_ins;
bp->addr = loc;
bp->typ = 0;
bp->cnt = 0;
bp->act = NULL;
sim_brk_ent = sim_brk_ent + 1;
return bp;
}

/* Set a breakpoint of type sw */

t_stat sim_brk_set (t_addr loc, int32 sw, int32 ncnt, char *act)
{
BRKTAB *bp;

if (sw == 0) sw = sim_brk_dflt;
if ((sim_brk_types & sw) == 0) return SCPE_NOFNC;
bp = sim_brk_fnd (loc);                                 /* present? */
if (!bp) bp = sim_brk_new (loc);                        /* no, allocate */
if (!bp) return SCPE_MEM;                               /* still no? mem err */
bp->typ = sw;                                           /* set type */
bp->cnt = ncnt;                                         /* set count */
if ((bp->act != NULL) && (act != NULL)) {               /* replace old action? */
    free (bp->act);                                     /* deallocate */
    bp->act = NULL;                                     /* now no action */
    }
if ((act != NULL) && (*act != 0)) {                     /* new action? */
    char *newp = (char *) calloc (CBUFSIZE, sizeof (char)); /* alloc buf */
    if (newp == NULL) return SCPE_MEM;                  /* mem err? */
    strncpy (newp, act, CBUFSIZE);                      /* copy action */
    bp->act = newp;                                     /* set pointer */
    }
sim_brk_summ = sim_brk_summ | sw;
return SCPE_OK;
}

/* Clear a breakpoint */

t_stat sim_brk_clr (t_addr loc, int32 sw)
{
BRKTAB *bp = sim_brk_fnd (loc);

if (!bp) return SCPE_OK;                                /* not there? ok */
if (sw == 0) sw = SIM_BRK_ALLTYP;
bp->typ = bp->typ & ~sw;
if (bp->typ) return SCPE_OK;                            /* clear all types? */
if (bp->act != NULL) free (bp->act);                    /* deallocate action */
for ( ; bp < (sim_brk_tab + sim_brk_ent - 1); bp++)     /* erase entry */
    *bp = *(bp + 1);
sim_brk_ent = sim_brk_ent - 1;                          /* decrement count */
sim_brk_summ = 0;                                       /* recalc summary */
for (bp = sim_brk_tab; bp < (sim_brk_tab + sim_brk_ent); bp++)
    sim_brk_summ = sim_brk_summ | bp->typ;
return SCPE_OK;
}

/* Clear all breakpoints */

t_stat sim_brk_clrall (int32 sw)
{
BRKTAB *bp;

if (sw == 0) sw = SIM_BRK_ALLTYP;
for (bp = sim_brk_tab; bp < (sim_brk_tab + sim_brk_ent); ) {
    if (bp->typ & sw) sim_brk_clr (bp->addr, sw);
    else bp++;
    }
return SCPE_OK;
}

/* Show a breakpoint */

t_stat sim_brk_show (FILE *st, t_addr loc, int32 sw)
{
BRKTAB *bp = sim_brk_fnd (loc);
DEVICE *dptr;
int32 i, any;

if (sw == 0) sw = SIM_BRK_ALLTYP;
if (!bp || (!(bp->typ & sw))) return SCPE_OK;
dptr = sim_dflt_dev;
if (dptr == NULL) return SCPE_OK;
if (sim_vm_fprint_addr) sim_vm_fprint_addr (st, dptr, loc);
else fprint_val (st, loc, dptr->aradix, dptr->awidth, PV_LEFT);
fprintf (st, ":\t");
for (i = any = 0; i < 26; i++) {
    if ((bp->typ >> i) & 1) {
        if (any) fprintf (st, ", ");
        fputc (i + 'A', st);
        any = 1;
        }
    }
if (bp->cnt > 0) fprintf (st, " [%d]", bp->cnt);
if (bp->act != NULL) fprintf (st, "; %s", bp->act);
fprintf (st, "\n");
return SCPE_OK;
}

/* Show all breakpoints */

t_stat sim_brk_showall (FILE *st, int32 sw)
{
BRKTAB *bp;

if (sw == 0) sw = SIM_BRK_ALLTYP;
for (bp = sim_brk_tab; bp < (sim_brk_tab + sim_brk_ent); bp++) {
    if (bp->typ & sw) sim_brk_show (st, bp->addr, sw);
    }
return SCPE_OK;
}

/* Test for breakpoint */

uint32 sim_brk_test (t_addr loc, uint32 btyp)
{
BRKTAB *bp;
uint32 spc = (btyp >> SIM_BKPT_V_SPC) & (SIM_BKPT_N_SPC - 1);

if ((bp = sim_brk_fnd (loc)) && (btyp & bp->typ)) {     /* in table, type match? */
    if ((sim_brk_pend[spc] && (loc == sim_brk_ploc[spc])) || /* previous location? */
        (--bp->cnt > 0)) return 0;                      /* count > 0? */
    bp->cnt = 0;                                        /* reset count */
    sim_brk_ploc[spc] = loc;                            /* save location */
    sim_brk_pend[spc] = TRUE;                           /* don't do twice */
    sim_brk_act = bp->act;                              /* set up actions */
    return (btyp & bp->typ);
    }
sim_brk_pend[spc] = FALSE;
return 0;
}

/* Get next pending action, if any */

char *sim_brk_getact (char *buf, int32 size)
{
char *ep;
size_t lnt;

if (sim_brk_act == NULL) return NULL;                   /* any action? */
while (isspace (*sim_brk_act)) sim_brk_act++;           /* skip spaces */
if (*sim_brk_act == 0) return (sim_brk_act = NULL);     /* now empty? */
if (ep = strchr (sim_brk_act, ';')) {                   /* cmd delimiter? */
    lnt = ep - sim_brk_act;                             /* cmd length */
    memcpy (buf, sim_brk_act, lnt + 1);                 /* copy with ; */
    buf[lnt] = 0;                                       /* erase ; */
    sim_brk_act = sim_brk_act + lnt + 1;                /* adv ptr */
    }
else {
    strncpy (buf, sim_brk_act, size);                   /* copy action */
    sim_brk_act = NULL;                                 /* no more */
    }
return buf;
}

/* Clear pending actions */

void sim_brk_clract (void)
{
sim_brk_act = NULL;
}

/* New PC */

void sim_brk_npc (uint32 cnt)
{
uint32 i;

if ((cnt == 0) || (cnt > SIM_BKPT_N_SPC)) cnt = SIM_BKPT_N_SPC;
for (i = 0; i < cnt; i++) {
    sim_brk_pend[i] = FALSE;
    sim_brk_ploc[i] = 0;
    }
return;
}

/* Clear breakpoint space */

void sim_brk_clrspc (uint32 spc)
{
if (spc < SIM_BKPT_N_SPC) {
    sim_brk_pend[spc] = FALSE;
    sim_brk_ploc[spc] = 0;
    }
return;
}

/* Debug printout routines, from Dave Hittner */

const char* debug_bstates = "01_^";
const char* debug_fmt     = "DBG> %s %s: ";
int32 debug_unterm  = 0;

/* Finds debug phrase matching bitmask from from device DEBTAB table */

static char* get_dbg_verb (uint32 dbits, DEVICE* dptr)
{
static char* debtab_none    = "DEBTAB_ISNULL";
static char* debtab_nomatch = "DEBTAB_NOMATCH";
int32 offset = 0;

if (dptr->debflags == 0) return debtab_none;

/* Find matching words for bitmask */

while (dptr->debflags[offset].name && (offset < 32)) {
    if (dptr->debflags[offset].mask & dbits)
        return dptr->debflags[offset].name;
    offset++;
    }
return debtab_nomatch;
}

/* Prints standard debug prefix unless previous call unterminated */

static void sim_debug_prefix (uint32 dbits, DEVICE* dptr)
{
if (!debug_unterm) {
    char* debug_type = get_dbg_verb (dbits, dptr);
    fprintf(sim_deb, debug_fmt, dptr->name, debug_type);
    }
}

/* Prints state of a register: bit translation + state (0,1,_,^)
   indicating the state and transition of the bit. States:
   0=steady(0->0), 1=steady(1->1), _=falling(1->0), ^=rising(0->1) */

void sim_debug_u16(uint32 dbits, DEVICE* dptr, const char* const* bitdefs,
    uint16 before, uint16 after, int terminate)
{
if (sim_deb && (dptr->dctrl & dbits)) {
    int32 i;

    sim_debug_prefix(dbits, dptr);                      /* print prefix if required */
    for (i = 15; i >= 0; i--) {                         /* print xlation, transition */
        int off = ((after >> i) & 1) + (((before ^ after) >> i) & 1) * 2;
        fprintf(sim_deb, "%s%c ", bitdefs[i], debug_bstates[off]);
        }
    if (terminate) fprintf(sim_deb, "\r\n");
    debug_unterm = terminate ? 0 : 1;                   /* set unterm for next */
    }
}

#if defined (_WIN32)
#define vsnprintf _vsnprintf
#endif
#if defined (__DECC) && defined (__VMS)
#define NO_vsnprintf
#endif
#if defined( NO_vsnprintf)
#define STACKBUFSIZE 16384
#else
#define STACKBUFSIZE 2048
#endif

/* Inline debugging - will print debug message if debug file is
   set and the bitmask matches the current device debug options.
   Extra returns are added for un*x systems, since the output
   device is set into 'raw' mode when the cpu is booted,
   and the extra returns don't hurt any other systems. */

void sim_debug (uint32 dbits, DEVICE* dptr, const char* fmt, ...)
{
if (sim_deb && (dptr->dctrl & dbits)) {

    char stackbuf[STACKBUFSIZE];
    int32 bufsize = sizeof(stackbuf);
    char *buf = stackbuf;
    va_list arglist;
    int32 i, j, len;

    buf[bufsize-1] = '\0';
    sim_debug_prefix(dbits, dptr);                      /* print prefix if required */

    while (1) {                                         /* format passed string, args */
        va_start (arglist, fmt);
#if defined(NO_vsnprintf)
#if defined(HAS_vsprintf_void)

/* Note, this could blow beyond the buffer, and we couldn't tell */
/* That is a limitation of the C runtime library available on this platform */

        vsprintf (buf, fmt, arglist);
        for (len = 0; len < bufsize-1; len++)
            if (buf[len] == 0) break;
#else
        len = vsprintf (buf, fmt, arglist);
#endif                                                  /* HAS_vsprintf_void */
#else                                                   /* NO_vsnprintf */
#if defined(HAS_vsnprintf_void)
        vsnprintf (buf, bufsize-1, fmt, arglist);
        for (len = 0; len < bufsize-1; len++)
            if (buf[len] == 0) break;
#else
        len = vsnprintf (buf, bufsize-1, fmt, arglist);
#endif                                                  /* HAS_vsnprintf_void */
#endif                                                  /* NO_vsnprintf */
        va_end (arglist);

/* If it didn't fit into the buffer, then grow it and try again */

        if ((len < 0) || (len >= bufsize-1)) {
            if (buf != stackbuf) free (buf);
            bufsize = bufsize * 2;
            buf = (char *) malloc (bufsize);
            if (buf == NULL) return;                    /* out of memory */
            buf[bufsize-1] = '\0';
            continue;
            }
        break;
        }

/* Output the formatted data expanding newlines where they exist */

    for (i = j = 0; i < len; ++i) {
        if ('\n' == buf[i]) {
            if (i > j) fwrite (&buf[j], 1, i-j, sim_deb);
            j = i;
            fputc('\r', sim_deb);
            }
        }
    if (i > j) fwrite (&buf[j], 1, i-j, sim_deb);

/* Set unterminated flag for next time */

    debug_unterm = (len && (buf[len-1]=='\n')) ? 0 : 1;
    if (buf != stackbuf) free (buf);
    }
return;
}
