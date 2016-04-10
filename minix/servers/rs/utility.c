/* This file contains some utility routines for RS.
 *
 * Changes:
 *   Nov 22, 2009: Created    (Cristiano Giuffrida)
 */

#include "inc.h"

#include <assert.h>
#include <minix/sched.h>
#include "kernel/const.h"
#include "kernel/config.h"
#include "kernel/proc.h"

#include <sys/reboot.h>

#define PRINT_SEP() printf("---------------------------------------------------------------------------------\n")

/*===========================================================================*
 *				 init_service				     *
 *===========================================================================*/
int init_service(struct rproc *rp, int type, int flags)
{
  int r;
  message m;
  endpoint_t old_endpoint;

  rp->r_flags |= RS_INITIALIZING;              /* now initializing */
  getticks(&rp->r_alive_tm);
  rp->r_check_tm = rp->r_alive_tm + 1;         /* expect reply within period */

  /* In case of RS initialization, we are done. */
  if(rp->r_priv.s_flags & ROOT_SYS_PROC) {
      return OK;
  }

  /* Determine the old endpoint if this is a new instance. */
  old_endpoint = NONE;
  if(rp->r_old_rp) {
      old_endpoint = rp->r_upd.state_endpoint;
  }
  else if(rp->r_prev_rp) {
      old_endpoint = rp->r_prev_rp->r_pub->endpoint;
  }

  /* Check flags. */
  if(rp->r_pub->sys_flags & SF_USE_SCRIPT) {
      flags |= SEF_INIT_SCRIPT_RESTART;
  }

  /* Send initialization message. */
  m.m_type = RS_INIT;
  m.m_rs_init.type = (short) type;
  m.m_rs_init.flags = flags;
  m.m_rs_init.rproctab_gid = rinit.rproctab_gid;
  m.m_rs_init.old_endpoint = old_endpoint;
  m.m_rs_init.restarts = (short) rp->r_restarts+1;
  m.m_rs_init.buff_addr = rp->r_map_prealloc_addr;
  m.m_rs_init.buff_len  = rp->r_map_prealloc_len;
  rp->r_map_prealloc_addr = 0;
  rp->r_map_prealloc_len = 0;
  r = rs_asynsend(rp, &m, 0);

  return r;
}

/*===========================================================================*
 *				 fi_service				     *
 *===========================================================================*/
int fi_service(struct rproc *rp, int type)
{
  message m;

  /* Send fault injection message. */
  m.m_type = COMMON_REQ_FI_CTL;
  m.m_lsys_fi_ctl.subtype = type;
  /* This is not a reply to a sendrec rather this is headed 
     towards the target process */
  return rs_asynsend(rp, &m, 1);	
}

/*===========================================================================*
 *			      fill_send_mask                                 *
 *===========================================================================*/
void fill_send_mask(send_mask, set_bits)
sys_map_t *send_mask;		/* the send mask to fill in */
int set_bits;			/* TRUE sets all bits, FALSE clears all bits */
{
/* Fill in a send mask. */
  int i;

  for (i = 0; i < NR_SYS_PROCS; i++) {
	if (set_bits)
		set_sys_bit(*send_mask, i);
	else
		unset_sys_bit(*send_mask, i);
  }
}

/*===========================================================================*
 *			      fill_call_mask                                 *
 *===========================================================================*/
void fill_call_mask(calls, tot_nr_calls, call_mask, call_base, is_init)
int *calls;                     /* the unordered set of calls */
int tot_nr_calls;               /* the total number of calls */
bitchunk_t *call_mask;          /* the call mask to fill in */
int call_base;                  /* the base offset for the calls */
int is_init;                    /* set when initializing a call mask */
{
/* Fill a call mask from an unordered set of calls. */
  int i;
  int call_mask_size, nr_calls;

  call_mask_size = BITMAP_CHUNKS(tot_nr_calls);

  /* Count the number of calls to fill in. */
  nr_calls = 0;
  for(i=0; calls[i] != NULL_C; i++) {
      nr_calls++;
  }

  /* See if all calls are allowed and call mask must be completely filled. */
  if(nr_calls == 1 && calls[0] == ALL_C) {
      for(i=0; i < call_mask_size; i++) {
          call_mask[i] = (~0);
      }
  }
  else {
      /* When initializing, reset the mask first. */
      if(is_init) {
          for(i=0; i < call_mask_size; i++) {
              call_mask[i] = 0;
          }
      }
      /* Enter calls bit by bit. */
      for(i=0; i < nr_calls; i++) {
          SET_BIT(call_mask, calls[i] - call_base);
      }
  }
}

/*===========================================================================*
 *			     srv_to_string_gen				     *
 *===========================================================================*/
char* srv_to_string_gen(struct rproc *rp, int is_verbose)
{
  struct rprocpub *rpub;
  int slot_nr;
  char *srv_string;
  static char srv_string_pool[3][RS_MAX_LABEL_LEN + 256];
  static int srv_string_pool_index = 0;

  rpub = rp->r_pub;
  slot_nr = rp - rproc;
  srv_string = srv_string_pool[srv_string_pool_index];
  srv_string_pool_index = (srv_string_pool_index + 1) % 3;

#define srv_str(cmd) ((cmd) == NULL || (cmd)[0] == '\0' ? "_" : (cmd))
#define srv_active_str(rp) ((rp)->r_flags & RS_ACTIVE ? "*" : " ")
#define srv_version_str(rp) ((rp)->r_new_rp || (rp)->r_next_rp ? "-" : \
    ((rp)->r_old_rp || (rp)->r_prev_rp ? "+" : " "))

  if(is_verbose) {
      sprintf(srv_string, "service '%s'%s%s(slot %d, ep %d, pid %d, cmd %s, script %s, proc %s, major %d, flags 0x%03x, sys_flags 0x%02x)",
          rpub->label, srv_active_str(rp), srv_version_str(rp),
          slot_nr, rpub->endpoint, rp->r_pid, srv_str(rp->r_cmd),
          srv_str(rp->r_script), srv_str(rpub->proc_name), rpub->dev_nr,
          rp->r_flags, rpub->sys_flags);
  }
  else {
      sprintf(srv_string, "service '%s'%s%s(slot %d, ep %d, pid %d)",
          rpub->label, srv_active_str(rp), srv_version_str(rp),
          slot_nr, rpub->endpoint, rp->r_pid);
  }

#undef srv_str
#undef srv_active_str
#undef srv_version_str

  return srv_string;
}

/*===========================================================================*
 *			     srv_upd_to_string				     *
 *===========================================================================*/
char* srv_upd_to_string(struct rprocupd *rpupd)
{
   static char srv_upd_string[256];
   struct rprocpub *rpub, *next_rpub, *prev_rpub;
   rpub = rpupd->rp ? rpupd->rp->r_pub : NULL;
   next_rpub = rpupd->next_rpupd && rpupd->next_rpupd->rp ? rpupd->next_rpupd->rp->r_pub : NULL;
   prev_rpub = rpupd->prev_rpupd && rpupd->prev_rpupd->rp ? rpupd->prev_rpupd->rp->r_pub : NULL;

#define srv_ep(RPUB) (RPUB ? (RPUB)->endpoint : -1)
#define srv_upd_luflag_c(F) (rpupd->lu_flags & F ? '1' : '0')
#define srv_upd_iflag_c(F) (rpupd->init_flags & F ? '1' : '0')

   sprintf(srv_upd_string, "update (lu_flags(SAMPUNDRV)=%c%c%c%c%c%c%c%c%c, init_flags=(FCTD)=%c%c%c%c, state %d (%s), tm %lu, maxtime %lu, endpoint %d, state_data_gid %d, prev_ep %d, next_ep %d)",
       srv_upd_luflag_c(SEF_LU_SELF), srv_upd_luflag_c(SEF_LU_ASR),
       srv_upd_luflag_c(SEF_LU_MULTI), srv_upd_luflag_c(SEF_LU_PREPARE_ONLY),
       srv_upd_luflag_c(SEF_LU_UNSAFE), srv_upd_luflag_c(SEF_LU_NOMMAP),
       srv_upd_luflag_c(SEF_LU_DETACHED), srv_upd_luflag_c(SEF_LU_INCLUDES_RS),
       srv_upd_luflag_c(SEF_LU_INCLUDES_VM), srv_upd_iflag_c(SEF_INIT_FAIL),
       srv_upd_iflag_c(SEF_INIT_CRASH), srv_upd_iflag_c(SEF_INIT_TIMEOUT),
       srv_upd_iflag_c(SEF_INIT_DEFCB), rpupd->prepare_state, 
       rpupd->prepare_state_data.eval_addr ? rpupd->prepare_state_data.eval_addr : "", rpupd->prepare_tm,
       rpupd->prepare_maxtime, srv_ep(rpub), rpupd->prepare_state_data_gid,
       srv_ep(prev_rpub), srv_ep(next_rpub));

   return srv_upd_string;
}

/*===========================================================================*
 *			     rs_asynsend				     *
 *===========================================================================*/
int rs_asynsend(struct rproc *rp, message *m_ptr, int no_reply)
{
  struct rprocpub *rpub;
  int r;

  rpub = rp->r_pub;

  if(no_reply) {
      r = asynsend3(rpub->endpoint, m_ptr, AMF_NOREPLY);
  }
  else {
      r = asynsend(rpub->endpoint, m_ptr);
  }

  if(rs_verbose)
      printf("RS: %s being asynsent to with message type %d, noreply=%d, result=%d\n",
          srv_to_string(rp), m_ptr->m_type, no_reply, r);

  return r;
}

/*===========================================================================*
 *			     rs_receive_ticks				     *
 *===========================================================================*/
int rs_receive_ticks(endpoint_t src, message *m_ptr,
    int *status_ptr, int ticks)
{
  int r = ENOTREADY;
  int status;

  while(ticks>=0) {
      r = ipc_receivenb(src, m_ptr, &status);
      if(status_ptr) {
          *status_ptr = status;
      }
      if(r != ENOTREADY || ticks == 0) {
          break;
      }
      tickdelay(1);
      ticks--;
  }

  return r;
}

/*===========================================================================*
 *				reply					     *
 *===========================================================================*/
void reply(who, rp, m_ptr)
endpoint_t who;                        	/* replyee */
struct rproc *rp;                       /* replyee slot (if any) */
message *m_ptr;                         /* reply message */
{
  int r;				/* send status */

  /* No need to actually reply to RS */
  if(who == RS_PROC_NR) {
      return;
  }

  if(rs_verbose && rp)
      printf("RS: %s being replied to with message type %d\n", srv_to_string(rp), m_ptr->m_type);

  r = ipc_sendnb(who, m_ptr);		/* send the message */
  if (r != OK)
      printf("RS: unable to send reply to %d: %d\n", who, r);
}

/*===========================================================================*
 *			      late_reply				     *
 *===========================================================================*/
void late_reply(rp, code)
struct rproc *rp;				/* pointer to process slot */
int code;					/* status code */
{
/* If a caller is waiting for a reply, unblock it. */
  if(rp->r_flags & RS_LATEREPLY) {
      message m;
      m.m_type = code;
      if(rs_verbose)
          printf("RS: %s late reply %d to %d for request %d\n",
              srv_to_string(rp), code, rp->r_caller, rp->r_caller_request);

      reply(rp->r_caller, NULL, &m);
      rp->r_flags &= ~RS_LATEREPLY;
  }
}

/*===========================================================================*
 *				rs_isokendpt			 	     *
 *===========================================================================*/
int rs_isokendpt(endpoint_t endpoint, int *proc)
{
	*proc = _ENDPOINT_P(endpoint);
	if(*proc < -NR_TASKS || *proc >= NR_PROCS)
		return EINVAL;

	return OK;
}

/*===========================================================================*
 *				sched_init_proc			 	     *
 *===========================================================================*/
int sched_init_proc(struct rproc *rp)
{
  int s;
  int is_usr_proc;

  /* Make sure user processes have no scheduler. PM deals with them. */
  is_usr_proc = !(rp->r_priv.s_flags & SYS_PROC);
  if(is_usr_proc) assert(rp->r_scheduler == NONE);
  if(!is_usr_proc) assert(rp->r_scheduler != NONE);

  /* Start scheduling for the given process. */
  if ((s = sched_start(rp->r_scheduler, rp->r_pub->endpoint, 
      RS_PROC_NR, rp->r_priority, rp->r_quantum, rp->r_cpu,
      &rp->r_scheduler)) != OK) {
      return s;
  }

  return s;
}

/*===========================================================================*
 *				update_sig_mgrs			 	     *
 *===========================================================================*/
int update_sig_mgrs(struct rproc *rp, endpoint_t sig_mgr,
	endpoint_t bak_sig_mgr)
{
  int r;
  struct rprocpub *rpub;

  rpub = rp->r_pub;

  if(rs_verbose)
      printf("RS: %s updates signal managers: %d%s / %d\n", srv_to_string(rp),
          sig_mgr == SELF ? rpub->endpoint : sig_mgr,
          sig_mgr == SELF ? "(SELF)" : "",
          bak_sig_mgr == NONE ? -1 : bak_sig_mgr);

  /* Synch privilege structure with the kernel. */
  if ((r = sys_getpriv(&rp->r_priv, rpub->endpoint)) != OK) {
      printf("unable to synch privilege structure: %d", r);
      return r;
  }

  /* Set signal managers. */
  rp->r_priv.s_sig_mgr = sig_mgr;
  rp->r_priv.s_bak_sig_mgr = bak_sig_mgr;

  /* Update privilege structure. */
  r = sys_privctl(rpub->endpoint, SYS_PRIV_UPDATE_SYS, &rp->r_priv);
  if(r != OK) {
      printf("unable to update privilege structure: %d", r);
      return r;
  }

  return OK;
}

/*===========================================================================*
 *				rs_is_idle			 	     *
 *===========================================================================*/
int rs_is_idle()
{
  int slot_nr;
  struct rproc *rp;
  for (slot_nr = 0; slot_nr < NR_SYS_PROCS; slot_nr++) {
      rp = &rproc[slot_nr];
      if (!(rp->r_flags & RS_IN_USE)) {
          continue;
      }
      if(!RS_SRV_IS_IDLE(rp)) {
          return 0;
      }
  }
  return 1;
}

int is_ready(endpoint_t ep)
{
  struct proc *p;
  static struct proc proctab[NR_TASKS + NR_PROCS];
  int result;

  if ((result = sys_getproctab(proctab)) != OK) {
      panic("RS: couldn't get copy of process table: %d\n", result);
  }
  p = &proctab[NR_TASKS+_ENDPOINT_P(ep)];
  assert(!RTS_ISSET(p, RTS_SLOT_FREE));
  if(p->p_rts_flags == RTS_RECEIVING && p->p_getfrom_e == ANY) {
  	printf("RS: %d (%s) is idle\n", ep, p->p_name);
	return 1;
  }
  printf("RS: %d (%s) is not idle; p_rts_flags %x, p->p_getfrom_e %d\n", ep, p->p_name, p->p_rts_flags, p->p_getfrom_e);
  return 0;
}

/*===========================================================================*
 *				rs_idle_period				     *
 *===========================================================================*/
void rs_idle_period() /* depends on VM, PM and DS */
{
  struct rproc *rp;
  struct rprocpub *rpub;
  int r;

  /* we can't be in vulnerable state while shutting down, because in this
   * situation rs_is_idle == 0 and user processes wouldn't exit in
   * vulnerable state
   */
  if (shutting_down) rs_set_vulnerable(0);

  /* Not much to do when RS is not idle. */
  if(!rs_is_idle()) {
      return;
  }

  /* Cleanup dead services. */
  for (rp=BEG_RPROC_ADDR; rp<END_RPROC_ADDR; rp++) {
      if((rp->r_flags & (RS_IN_USE|RS_DEAD)) == (RS_IN_USE|RS_DEAD)) {
          cleanup_service(rp); /* depends on VM, PM and DS */
      }
  }

  /* Create missing replicas when necessary. */
  for (rp=BEG_RPROC_ADDR; rp<END_RPROC_ADDR; rp++) {
      rpub = rp->r_pub;
      if((rp->r_flags & RS_ACTIVE) && (rpub->sys_flags & SF_USE_REPL) && rp->r_next_rp == NULL) {
          if(rpub->endpoint == VM_PROC_NR && (rp->r_old_rp || rp->r_new_rp)) {
              /* Only one replica at the time for VM. */
              continue;
          }
          if ((rp->r_flags & RS_EXITING) || (rp->r_pub->sys_flags & SF_NORESTART)) {
              /* Don't create a replica if it won't be restarted on failure. */
              continue;
	  }
          if ((r = clone_service(rp, RST_SYS_PROC, 0)) != OK) { /* depends on VM, PM and DS */
              printf("RS: warning: unable to clone %s (error %d)\n",
                  srv_to_string(rp), r);
          }
      }
  }

  /* with all the replica's ready and cleanup done, the system is no longer
   * in a vulnerable state
   */
  rs_set_vulnerable(0);
}

/*===========================================================================*
 *			   print_services_status		 	     *
 *===========================================================================*/
void print_services_status()
{
  int slot_nr;
  struct rproc *rp;
  int num_services = 0;
  int num_service_instances = 0;
  int is_verbose = 1;

  PRINT_SEP();
  printf("Printing information about all the system service instances:\n");
  PRINT_SEP();
  for (slot_nr = 0; slot_nr < NR_SYS_PROCS; slot_nr++) {
      rp = &rproc[slot_nr];
      if (!(rp->r_flags & RS_IN_USE)) {
          continue;
      }
      if (rp->r_flags & RS_ACTIVE) {
          num_services++;
      }
      num_service_instances++;
      printf("%s\n", srv_to_string_gen(rp, is_verbose));
  }
  PRINT_SEP();
  printf("Found %d service instances, of which %d are active services\n",
  	  num_service_instances, num_services);
  PRINT_SEP();
}

/*===========================================================================*
 *			    print_update_status 		 	     *
 *===========================================================================*/
void print_update_status()
{
  struct rprocupd *prev_rpupd, *rpupd;
  int is_updating = RUPDATE_IS_UPDATING();
  int i;

#define rupdate_flag_c(F) (rupdate.flags & F ? '1' : '0')

  if(!is_updating && !RUPDATE_IS_UPD_SCHEDULED()) {
      PRINT_SEP();
      printf("No update is in progress or scheduled\n");
      PRINT_SEP();
      return;
  }

  PRINT_SEP();
  i = 1;
  printf("A %s-component update is %s, flags(UIRV)=%c%c%c%c:\n", RUPDATE_IS_UPD_MULTI() ? "multi" : "single",
      is_updating ? "in progress" : "scheduled",
      rupdate_flag_c(RS_UPDATING), rupdate_flag_c(RS_INITIALIZING),
      rupdate.rs_rpupd ? '1' : '0', rupdate.vm_rpupd ? '1' : '0');
  PRINT_SEP();
  RUPDATE_ITER(rupdate.first_rpupd, prev_rpupd, rpupd,
      printf("%d. %s %s %s\n", i++, srv_to_string(rpupd->rp),
          is_updating ? "updating with" : "scheduled for",
          srv_upd_to_string(rpupd));
  );
  PRINT_SEP();

#undef rupdate_flag_c
}

/*===========================================================================*
 *			    rs_set_vulnerable 			 	     *
 *===========================================================================*/
void rs_set_vulnerable(int vulnerable) {
	static int was_vulnerable;

	/* the system is in a vulnerable state if one or more replica's are
	 * missing because other servers are needed to create the new replica's
	 * and if they fail before or while this is done, the system will hang;
	 * when the system is vulnerable we tell the kernel to stop scheduling
	 * user processes as they would increase the risk of a bad state
	 */

	if (was_vulnerable == vulnerable) return;

	sys_privctl_set_rs_ready(!vulnerable);
	was_vulnerable = vulnerable;
}

/*===========================================================================*
 *			    rscheck_locked 			 	     *
 *===========================================================================*/
static void rscheck_locked(const char *file, int line) {
	int r;

	hypermem_printstr("RS: shutdown due to deadlock");
	printf("RS: %s:%d: deadlocked on sendrec, shutting down system\n",
		file, line);
	r = sys_abort(RB_NOSYNC);
	panic("RS: sys_abort failed: %d\n", r);
}

/*===========================================================================*
 *			    rscheck_err 			 	     *
 *===========================================================================*/
int rscheck_err(int r, const char *file, int line) {
	if (r == ELOCKED) rscheck_locked(file, line);
	return r;
}

/*===========================================================================*
 *			    rscheck_int 			 	     *
 *===========================================================================*/
int rscheck_int(int r, const char *file, int line) {
	if (r == -1 && -errno == ELOCKED) rscheck_locked(file, line);
	return r;
}

/*===========================================================================*
 *			    rscheck_ptr 			 	     *
 *===========================================================================*/
void *rscheck_ptr(void *r, const char *file, int line) {
	if (r == (char *) -1 && -errno == ELOCKED) rscheck_locked(file, line);
	return r;
}
