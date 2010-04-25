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

PLI_INT32 pli_test_calltf(PLI_BYTE8 *user_data)
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
    vpi_free_object(href);

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

    vpi_free_object(aref);
    vpi_free_object(bref);

    return 0;
}

PLI_INT32 pli_test_compiletf(PLI_BYTE8 *user_data)
{
    vpiHandle href, iter;
    vpiHandle aref, bref;

    href = vpi_handle(vpiSysTfCall, NULL); 
    if (href == NULL) {
        vpi_printf("** ERR: $pli_test PLI 2.0 can't get systf call handle\n");
        return(0);
    }

    iter = vpi_iterate(vpiArgument, href);

    aref = vpi_scan(iter);
    bref = vpi_scan(iter);

    vpi_free_object(iter);

    return 0;
}

PLI_INT32 pli_test_startofsim(s_cb_data *callback_data)
{
    vpi_printf("$pli_test start\n");
    return(0);
}

static void register_my_systfs(void)
{
  s_vpi_systf_data tf_data;
  s_cb_data        cb_data_s;
  vpiHandle        callback_handle;

  tf_data.type        = vpiSysTask;
  tf_data.sysfunctype = 0;
  tf_data.tfname      = "$pli_test";
  tf_data.calltf      = pli_test_calltf;
  tf_data.compiletf   = pli_test_compiletf;
  tf_data.sizetf      = NULL;
  tf_data.user_data   = NULL;
  vpi_register_systf(&tf_data);

  cb_data_s.reason    = cbStartOfSimulation;
  cb_data_s.cb_rtn    = pli_test_startofsim;
  cb_data_s.obj       = NULL;
  cb_data_s.time      = NULL;
  cb_data_s.value     = NULL;
  cb_data_s.user_data = NULL;
  callback_handle = vpi_register_cb(&cb_data_s);
  vpi_free_object(callback_handle);
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
