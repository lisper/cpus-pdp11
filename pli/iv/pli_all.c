
void vpi_compat_bootstrap(void)
{
	extern void ide_vpi_compat_bootstrap(void);
	extern void pdp11dis_vpi_compat_bootstrap(void);
	extern void rk_vpi_compat_bootstrap(void);
	ide_vpi_compat_bootstrap();
	pdp11dis_vpi_compat_bootstrap();
	rk_vpi_compat_bootstrap();
}

static void register_my_systfs(void)
{
	vpi_compat_bootstrap();
}

void (*vlog_startup_routines[]) () =
{
 register_my_systfs, 
 0
};
