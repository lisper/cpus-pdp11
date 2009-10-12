/* pli_ram.c */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "vpi_user.h"
#include "cv_vpi_user.h"

typedef unsigned short u16;

static char last_ce_bit;
static char last_we_bit;
static unsigned last_addr;

static vpiHandle do_aref;
static int mem_init;
u16 *M;

static char *instnam_tab[10]; 
static int last_evh;

void do_mem_preload(char *fn)
{
    FILE *f;
    f = fopen(fn, "r");
    if (f) {
        unsigned int a, v;
        while (fscanf(f, "%o %o\n", &a, &v) == 2) {
            vpi_printf("[%06o] <- %06o\n", a, v);
            M[a/2] = v;
        }
        fclose(f);
    }
}
void do_mem_init(void)
{
    int i;
    M = (u16 *)malloc(1024*1024*2);
    mem_init = 1;

    for (i = 0; i < 32768; i++)
        M[i] = 0;

    if (0) do_mem_preload("test1.mem");
}

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

    vpi_printf("pli_ram: adding instance %d, %s\n", last_evh+1, chp);

    instnam_tab[++last_evh] = malloc(strlen(chp) + 1);
    strcpy(instnam_tab[last_evh], chp);

    return(last_evh);
} 

PLI_INT32 pli_ram(void)
{
    vpiHandle href, iter, mhref;
    vpiHandle clkref, resetref, aref, diref, doref, ceref, weref, byteopref;
    s_vpi_value tmpval, outval;
    int numargs, inst_id;

    if (!mem_init) {
        do_mem_init();
    }

    href = vpi_handle(vpiSysTfCall, NULL); 
    if (href == NULL) {
        vpi_printf("** ERR: $pli_ram PLI 2.0 can't get systf call handle\n");
        return(0);
    }

    mhref = vpi_handle(vpiScope, href);

    if (vpi_get(vpiType, mhref) != vpiModule)
        mhref = vpi_handle(vpiModule, mhref); 

    inst_id = getadd_inst_id(mhref);

    //vpi_printf("pli_ram: inst_id %d\n", inst_id);

    iter = vpi_iterate(vpiArgument, href);

    numargs = vpi_get(vpiSize, iter);

    /* clk, reset, a, di, do, ce, we */
    clkref = vpi_scan(iter);
    resetref = vpi_scan(iter);
    aref = vpi_scan(iter);
    diref = vpi_scan(iter);
    doref = vpi_scan(iter);
    ceref = vpi_scan(iter);
    weref = vpi_scan(iter);
    byteopref = vpi_scan(iter);

    vpi_free_object(mhref);
    vpi_free_object(href);
    vpi_free_object(iter);

    if (clkref == NULL || resetref == NULL || aref == NULL ||
	diref == NULL || doref == NULL || ceref == NULL ||
	weref == NULL || byteopref == NULL)
    {
        vpi_printf("**ERR: $pli_ram bad args\n");
        return(0);
    }

    /* */
    char clk_bit, reset_bit;
    char di_bits[17], do_bits[17];
    char ce_bit, we_bit, byteop_bit;
    unsigned int a, datai;

    tmpval.format = vpiBinStrVal; 
    vpi_get_value(clkref, &tmpval);
    clk_bit = tmpval.value.str[0];

    tmpval.format = vpiBinStrVal; 
    vpi_get_value(resetref, &tmpval);
    reset_bit = tmpval.value.str[0];

    tmpval.format = vpiIntVal; 
    vpi_get_value(aref, &tmpval);
    a = tmpval.value.integer;

    tmpval.format = vpiBinStrVal; 
    vpi_get_value(diref, &tmpval);
    strcpy(di_bits, tmpval.value.str);

    tmpval.format = vpiIntVal; 
    vpi_get_value(diref, &tmpval);
    datai = tmpval.value.integer;

    tmpval.format = vpiBinStrVal; 
    vpi_get_value(doref, &tmpval);
    strcpy(do_bits, tmpval.value.str);

    tmpval.format = vpiBinStrVal; 
    vpi_get_value(ceref, &tmpval);
    ce_bit = tmpval.value.str[0];

    tmpval.format = vpiBinStrVal; 
    vpi_get_value(weref, &tmpval);
    we_bit = tmpval.value.str[0];

    tmpval.format = vpiBinStrVal; 
    vpi_get_value(byteopref, &tmpval);
    byteop_bit =  tmpval.value.str[0];


    /* */
    int read_start, read_stop, write_start, write_stop, addr_change;

    read_start = 0;
    read_stop = 0;
    write_start = 0;
    write_stop = 0;
    addr_change = 0;

    if (a != last_addr)
        addr_change = 1;

    if (we_bit != last_we_bit || addr_change) {
        if (we_bit == '0') write_start = 1;
        if (we_bit == '1') write_stop = 1;
    }

    if (ce_bit != last_ce_bit || write_stop || addr_change) {
        if (ce_bit == '0') read_start = 1;
        if (ce_bit == '1') read_stop = 1;
    }

    last_ce_bit = ce_bit;
    last_we_bit = we_bit;
    last_addr = a;

    if (0) vpi_printf("pli_ram: clk %c ce %c we %c\n", clk_bit, ce_bit, we_bit);

    if (reset_bit == '1') vpi_printf("pli_ram: reset\n");

    if (0) {
        if (read_start) vpi_printf("pli_ram: read start\n");
        if (read_stop) vpi_printf("pli_ram: read stop\n");
        if (write_start) vpi_printf("pli_ram: write start\n");
        if (write_stop) vpi_printf("pli_ram: write stop\n");
    }

    /* */
    if (write_start) {
        vpi_printf("pli_ram: write %o <- %o\n", a, datai);

	if (byteop_bit != '1') {
            M[a/2] = datai;
	} else {
            if (a & 1)
                M[a/2] = (M[a/2] & 0x00ff) | ((datai & 0xff) << 8);
            else
                M[a/2] = (M[a/2] & 0xff00) | ((datai & 0xff) << 0);
        }
    }

    if (read_start) {
        u16 value;

	if (byteop_bit != '1') {
            value = M[a/2];
	} else {
            if (a & 1)
                value = (M[a/2] >> 8) & 0x00ff;
            else
                value = (M[a/2] >> 0) & 0x00ff;
        }

        vpi_printf("pli_ram: read %o -> %o\n", a, value);

#ifdef __CVER__
        if (do_aref == 0)
            do_aref = vpi_put_value(doref, NULL, NULL, vpiAddDriver);
#else
        do_aref = doref;
#endif

        outval.format = vpiIntVal;
        outval.value.integer = value;

        vpi_put_value(do_aref, &outval, NULL, vpiNoDelay);
    }

    if (read_stop) {
        outval.format = vpiBinStrVal;
//        outval.value.str = "16'bzzzzzzzzzzzzzzzz";
        outval.value.str = "16'b0000000000000000";
        if (do_aref)
            vpi_put_value(do_aref, &outval, NULL, vpiNoDelay);
    }

    return(0);
}



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
