/* pli_test.c */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#ifdef unix
#include <unistd.h>
#endif

#include "vpi_user.h"

#ifdef __CVER__
#include "cv_vpi_user.h"
#endif

#ifdef __MODELSIM__
#include "veriuser.h"
#endif

PLI_INT32 pli_test(void)
{
    vpiHandle href, iter;
    vpiHandle aref, bref;
    s_vpi_value tmpval, outval;
    int numargs;
    int a_bit, b_bit;

    href = vpi_handle(vpiSysTfCall, NULL); 
    if (href == NULL) {
        vpi_printf("** ERR: $pli_test PLI 2.0 can't get systf call handle\n");
        return(0);
    }

    iter = vpi_iterate(vpiArgument, href);

    aref = vpi_scan(iter);
    bref = vpi_scan(iter);

    vpi_free_object(iter);

    if (aref == NULL || bref == NULL) {
        vpi_printf("**ERR: $pli_test bad args\n");
        return(0);
    }

    tmpval.format = vpiBinStrVal; 
    vpi_get_value(aref, &tmpval);
    a_bit = tmpval.value.str[0];

    tmpval.format = vpiBinStrVal; 
    vpi_get_value(bref, &tmpval);
    b_bit = tmpval.value.str[0];

    outval.format = vpiBinStrVal;
    outval.value.str = b_bit ? "1'b0" : "1'b1";
    vpi_put_value(aref, &outval, NULL, vpiNoDelay);

    return(0);
}

static void register_my_systfs(void)
{
    p_vpi_systf_data systf_data_p;

    static s_vpi_systf_data systf_data_list[] = {
        { vpiSysTask, 0, "$pli_test", pli_test, NULL, NULL, NULL },
        { 0, 0, NULL, NULL, NULL, NULL, NULL }
    };

    systf_data_p = &(systf_data_list[0]);
    while (systf_data_p->type != 0) {
        vpi_register_systf(systf_data_p++);
    }
}

#ifdef unix
static void (*rk_vlog_startup_routines[]) () =
{
 register_my_systfs, 
 0
};

void rk_vpi_compat_bootstrap(void)
{
    int i;

    io_rk_debug = 0;
    rk_debug = 1;

    for (i = 0;; i++) {
        if (rk_vlog_startup_routines[i] == NULL)
            break; 
        rk_vlog_startup_routines[i]();
    }
}

void vpi_compat_bootstrap(void)
{
    rk_vpi_compat_bootstrap();
}

void __stack_chk_fail_local() {}
#endif

#ifdef __MODELSIM__
static void (*vlog_startup_routines[]) () =
{
 register_my_systfs, 
 0
};
#endif


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
