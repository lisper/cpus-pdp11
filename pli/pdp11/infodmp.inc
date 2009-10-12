/*
 * expr and node info dump routines include
 */

/*
 * routine to print contents of exprinfo record 
 */
#ifdef __STDC__
int prt_exprinfo(p_tfexprinfo pinfo) {
#else
int prt_exprinfo(pinfo)
 p_tfexprinfo pinfo;
{
#endif
 register int gi;
 char *exprtyp_tonam();
 char s1[512]; 
 
 io_printf("**** exprinfo dump ****\n");
 io_printf(" expr_type: %s\n", exprtyp_tonam(s1, (int) pinfo->expr_type));
 io_printf(" expr_ngroups: %d\n", pinfo->expr_ngroups);
 if (pinfo->expr_type == TF_READONLYREAL
  || pinfo->expr_type == TF_READWRITEREAL)
   io_printf(" expr_value: %g", pinfo->real_value); 
 else
  {
   for (gi = 0; gi < pinfo->expr_ngroups; gi++) 
    {
     io_printf(" group %d: aval=%x, bval=%x\n", gi,
      pinfo->expr_value_p[gi].avalbits, pinfo->expr_value_p[gi].bvalbits);
    }
  }
 io_printf(" expr_vec_size: %d\n", pinfo->expr_vec_size);
 io_printf(" expr_sign: %d\n", pinfo->expr_sign);
 /* think next 2 unused? */
 io_printf(" expr_lhs_select: %d\n", pinfo->expr_lhs_select);
 io_printf(" expr_lhs_select: %d\n", pinfo->expr_rhs_select);
 io_printf("**** end exprinfo ****\n");
}

/*
 * convert an expr_type expr info field to a name
 */
char *exprtyp_tonam(s, xtyp)
 char *s;
 int xtyp;
{
 switch (xtyp) {
  case TF_NULLPARAM: strcpy(s, "tf_nullparam"); break;
  case TF_STRING: strcpy(s, "tf_string"); break;
  case TF_READONLY: strcpy(s, "tf_readonly"); break;
  case TF_READWRITE: strcpy(s, "tf_readwrite"); break;
  case TF_RWBITSELECT: strcpy(s, "tf_rwbitselect"); break;
  case TF_RWPARTSELECT: strcpy(s, "tf_rwpartselect"); break;
  case TF_RWMEMSELECT: strcpy(s, "tf_rwmemselect"); break;
  case TF_READONLYREAL: strcpy(s, "tf_readonlyreal"); break;
  case TF_READWRITEREAL: strcpy(s, "tf_readwritereal"); break;
  default: 
   tf_message(ERR_INTERNAL, "", "",
   "unknown exprinfo expr_type value %d", xtyp);
 }
 return(s);
}

/*
 * routine to print contents of nodeinfo record 
 */
#ifdef __STDC__
int prt_nodeinfo(p_tfnodeinfo pinfo) {
#else
int prt_nodeinfo(pinfo)
 p_tfnodeinfo pinfo;
{
#endif
 register int gi;
 char *nodetyp_tonam();
 char s1[512]; 
 
 if (pinfo->node_type == TF_NULL_NODE)
  {
   io_printf("**** nodeinfo dump for tf_null_node (no contents) ****\n");
   return;
  }

 if (pinfo->node_symbol == NULL) strcpy(s1, "<empty>");
 else strcpy(s1, pinfo->node_symbol);
 io_printf("**** nodeinfo dump for symbol %s ****\n", s1);
 io_printf(" node_type: %s\n", nodetyp_tonam(s1, (int) pinfo->node_type));
 io_printf(" node_ngroups: %d\n", pinfo->node_ngroups);
 /* notice for now in Cver t_strengthval never for strength vectors value */
 /* bits stored in t_vecval (also for scalars) */
 /* letter to sent to IEEE committee for clarification */
 if (pinfo->node_type == TF_REAL_NODE)
   io_printf(" node_value: %g", *pinfo->node_value.real_val_p); 
 else
  {
   for (gi = 0; gi < pinfo->node_ngroups; gi++) 
    {
     io_printf(" group %d: aval=%x, bval=%x\n", gi,
      pinfo->node_value.vecval_p[gi].avalbits,
      pinfo->node_value.vecval_p[gi].bvalbits);
    }
  }
 io_printf(" node_vec_size: %d\n", pinfo->node_vec_size);
 io_printf(" node_sign: %d\n", pinfo->node_sign);
 io_printf(" node_ms_index: %d\n", pinfo->node_ms_index);
 io_printf(" node_ls_index: %d\n", pinfo->node_ls_index);
 io_printf(" node_mem_size: %d\n", pinfo->node_mem_size);
 io_printf(" node_lhs_element: %d\n", pinfo->node_lhs_element);
 io_printf(" node_rhs_element: %d\n", pinfo->node_rhs_element);
 io_printf("**** end nodeinfo ****\n");
}

/*
 * convert a node_type node info field type to a name
 */
char *nodetyp_tonam(s, ntyp)
 char *s;
 int ntyp;
{
 switch (ntyp) {
  case TF_NULL_NODE: strcpy(s, "tf_null_node"); break;
  case TF_REG_NODE: strcpy(s, "tf_reg_node"); break;
  case TF_INTEGER_NODE: strcpy(s, "tf_integer_node"); break;
  case TF_REAL_NODE: strcpy(s, "tf_real_node"); break;
  case TF_TIME_NODE: strcpy(s, "tf_time_node"); break;
  case TF_NETVECTOR_NODE: strcpy(s, "tf_netvector_node"); break;
  case TF_NETSCALAR_NODE: strcpy(s, "tf_netscalar_node"); break;
  case TF_MEMORY_NODE: strcpy(s, "tf_memory_node"); break;
  default: 
   tf_message(ERR_INTERNAL, "", "",
   "unknown nodeinfo node_type value %d", ntyp);
 }
 return(s);
}

char *prt_tfreason(s, reason)
 char *s;
 int reason;
{
 switch (reason) {
  case REASON_CHECKTF: strcpy(s, "reason_checktf");
  case REASON_SIZETF: strcpy(s, "reason_sizetf"); break;
  case REASON_CALLTF: strcpy(s, "reason_calltf"); break;
  /* not supported by Cver */
  case REASON_SAVE: strcpy(s, "reason_save"); break;
  /* not supported by Cver */
  case REASON_RESTART: strcpy(s, "reason_restart"); break;
  /* not supported by Cver */
  case REASON_DISABLE: strcpy(s, "reason_disable"); break;
  case REASON_PARAMVC: strcpy(s, "reason_paramvc"); break;
  case REASON_SYNCH: strcpy(s, "reason_synch"); break;
  case REASON_FINISH: strcpy(s, "reason_finish"); break;
  case REASON_REACTIVATE: strcpy(s, "reason_reactivate"); break;
  case REASON_ROSYNCH: strcpy(s, "reason_rosynch"); break;
  /* not supported by Cver */
  case REASON_PARAMDRC: strcpy(s, "reason_paramdrc"); break;
  case REASON_ENDOFCOMPILE: strcpy(s, "reason_endofcompile"); break;
  /* not supported by Cver */
  case REASON_SCOPE: strcpy(s, "reason_scope"); break;
  case REASON_INTERACTIVE: strcpy(s, "reason_interactive"); break;
  case REASON_RESET: strcpy(s, "reason_reset"); break;
  case REASON_ENDOFRESET: strcpy(s, "reason_endofreset"); break;
  /* not supported by Cver */
  case REASON_FORCE: strcpy(s, "reason_force"); break;
  /* not supported by Cver */
  case REASON_RELEASE: strcpy(s, "reason_release"); break;
  /* not supported by Cver */
  case REASON_STARTOFSAVE: strcpy(s, "reason_startofsave"); break;
  /* not supported by Cver */
  case REASON_STARTOFRESTART: strcpy(s, "reason_startofrestart"); break;
  default: 
   tf_message(ERR_INTERNAL, "", "", "unknown misctf call reason %d", reason);
 }
 return(s);
}
