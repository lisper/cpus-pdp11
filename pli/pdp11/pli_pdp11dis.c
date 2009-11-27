/* pli_pdp11dis.c */
/*
 * pdp11 disassembler
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "vpi_user.h"
#include "cv_vpi_user.h"

PLI_INT32 pli_pdp11dis(void);
static void register_my_systfs(void);

char *instnam_tab[10]; 
int last_evh;

struct state_s {
    int inited;
} state[10];

static int getadd_inst_id(vpiHandle mhref)
{
    register int i;
    char *chp;
 
    chp = vpi_get_str(vpiFullName, mhref);
    //vpi_printf("getadd_inst_id() %s\n", chp);

    for (i = 1; i <= last_evh; i++) {
        if (strcmp(instnam_tab[i], chp) == 0)
            return(i);
    }

    instnam_tab[++last_evh] = malloc(strlen(chp) + 1);
    strcpy(instnam_tab[last_evh], chp);

    //vpi_printf("getadd_inst_id() done %d\n", last_evh);
    return(last_evh);
} 

extern void make_isn_table(void);
extern void dis(unsigned short, unsigned short, unsigned short);

static void
do_pdp11dis_setup(struct state_s *s)
{
    make_isn_table();
}

/*
 *
 */
PLI_INT32 pli_pdp11dis(void)
{
    vpiHandle href, iter, mhref;
    vpiHandle pcref, isnref, isn1ref, isn2ref;
    unsigned short pc, m0, m1, m2;
    struct state_s *s;
    int numargs, inst_id;
    s_vpi_value tmpval;

    href = vpi_handle(vpiSysTfCall, NULL); 
    if (href == NULL) {
        vpi_printf("** ERR: $pli_pdp11dis PLI 2.0 can't get systf call handle\n");
        return(0);
    }

    mhref = vpi_handle(vpiScope, href);

    if (vpi_get(vpiType, mhref) != vpiModule)
        mhref = vpi_handle(vpiModule, mhref); 

    inst_id = getadd_inst_id(mhref);

    //vpi_printf("pli_pdp11dis: inst_id %d\n", inst_id);
    s = &state[inst_id];

    if (!s->inited) {
        s->inited = 1;
        do_pdp11dis_setup(s);
    }

    iter = vpi_iterate(vpiArgument, href);

    numargs = vpi_get(vpiSize, iter);

    /* data_bus[15:0], pdp11dis_dior, pdp11dis_diow, pdp11dis_cs[1:0], pdp11dis_da[2:0] */
    pcref = vpi_scan(iter);
    isnref = vpi_scan(iter);
    isn1ref = vpi_scan(iter);
    isn2ref = vpi_scan(iter);

    if (pcref == NULL || isnref == NULL || isn1ref == NULL || isn2ref == NULL)
    {
        vpi_printf("**ERR: $pli_pdp11dis bad args\n");
        return(0);
    }

    tmpval.format = vpiIntVal; 
    vpi_get_value(pcref, &tmpval);
    pc = tmpval.value.integer;

    tmpval.format = vpiIntVal; 
    vpi_get_value(isnref, &tmpval);
    m0 = tmpval.value.integer;

    tmpval.format = vpiIntVal; 
    vpi_get_value(isn1ref, &tmpval);
    m1 = tmpval.value.integer;

    tmpval.format = vpiIntVal; 
    vpi_get_value(isn2ref, &tmpval);
    m2 = tmpval.value.integer;

    printf("f2: pc=%o, isn=", pc);

    dis(m0, m1, m2);

    return(0);
}

/* Template functin table for added user systf tasks and functions.
   See file vpi_user.h for structure definition
   Note only vpi_register_systf and vpi_ or tf_ utility routines that 
   do not access the simulation data base may be called from these routines
*/ 

/* all routines are called to register system tasks */
/* called just after all PLI 1.0 tf_ veriusertfs table routines are set up */
/* before source is read */ 
void (*pdp11dis_vlog_startup_routines[]) () =
{
 register_my_systfs, 
 0
};

/*
 * register all vpi_ PLI 2.0 style user system tasks and functions
 */
static void register_my_systfs(void)
{
    p_vpi_systf_data systf_data_p;

    /* use predefined table form - could fill systf_data_list dynamically */
    static s_vpi_systf_data systf_data_list[] = {
        { vpiSysTask, 0, "$pli_pdp11dis", pli_pdp11dis, NULL, NULL, NULL },
        { 0, 0, NULL, NULL, NULL, NULL, NULL }
    };

    systf_data_p = &(systf_data_list[0]);
    while (systf_data_p->type != 0) vpi_register_systf(systf_data_p++);
}

/* dummy +loadvpi= boostrap routine - mimics old style exec all routines */
/* in standard PLI vlog_startup_routines table */
void pdp11dis_vpi_compat_bootstrap(void)
{
    int i;

    for (i = 0;; i++) 
    {
        if (pdp11dis_vlog_startup_routines[i] == NULL) break; 
        pdp11dis_vlog_startup_routines[i]();
    }
}

void vpi_compat_bootstrap(void)
{
    pdp11dis_vpi_compat_bootstrap();
}

#ifndef BUILD_ALL
void __stack_chk_fail_local(void) {}
#endif


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
