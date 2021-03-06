#include "syslib.h"
#include <assert.h>
#include <minix/sysutil.h>
#include <minix/rs.h>
#include <minix/timers.h>
#include <minix/endpoint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "hypermem.h"

/* Self variables. */
#define SEF_SELF_NAME_MAXLEN 20
char sef_self_name[SEF_SELF_NAME_MAXLEN];
char *sef_self_name_p = sef_self_name;
endpoint_t sef_self_endpoint = NONE;
endpoint_t sef_self_proc_nr;
int sef_self_priv_flags;
int sef_self_init_flags;
int sef_self_receiving;

/* Extern variables. */
EXTERN int sef_lu_state;
EXTERN int __sef_st_before_receive_enabled;
EXTERN __attribute__((weak)) int __vm_init_fresh;

/* ltckpt recovery static library functions */
EXTERN __attribute__((weak)) int ltckpt_is_req_rehandling();
EXTERN __attribute__((weak)) void ltckpt_deactivate_req_rehandling();

/* Debug. */
#if SEF_INIT_DEBUG || SEF_LU_DEBUG || SEF_PING_DEBUG || SEF_SIGNAL_DEBUG || SEF_METADATA_DEBUG
#define SEF_DEBUG_HEADER_MAXLEN 50
static int sef_debug_init = 0;
static time_t sef_debug_boottime = 0;
static u32_t sef_debug_system_hz = 0;
static time_t sef_debug_time_sec = 0;
static time_t sef_debug_time_us = 0;
static char sef_debug_header_buff[SEF_DEBUG_HEADER_MAXLEN];
static void sef_debug_refresh_params(void);
char* sef_debug_header(void);
#endif

/* SEF Init prototypes. */
EXTERN int do_sef_rs_init(endpoint_t old_endpoint);
EXTERN int do_sef_init_request(message *m_ptr);

/* SEF Ping prototypes. */
EXTERN int do_sef_ping_request(message *m_ptr);

/* SEF Live update prototypes. */
EXTERN void do_sef_lu_before_receive(void);
EXTERN int do_sef_lu_request(message *m_ptr);

/* SEF Signal prototypes. */
EXTERN int do_sef_signal_request(message *m_ptr);

/* State transfer prototypes. */
EXTERN void do_sef_st_before_receive(void);

/* SEF GCOV prototypes. */
#ifdef USE_COVERAGE
EXTERN int do_sef_gcov_request(message *m_ptr);
#endif

/* SEF Fault Injection prototypes. */
EXTERN int do_sef_fi_request(message *m_ptr);

/* SEF Metadata prototypes. */
EXTERN int do_sef_metadata_request(message *m_ptr);

int sef_init_done = 0;

/*===========================================================================*
 *				sef_startup				     *
 *===========================================================================*/
void sef_startup()
{
/* SEF startup interface for system services. */
  int r, status;
  endpoint_t old_endpoint;
  int priv_flags;
  int init_flags;
  int sys_upd_flags = 0;

  sef_init_done = 0;

  /* Get information about self. */
  r = sys_whoami(&sef_self_endpoint, sef_self_name, SEF_SELF_NAME_MAXLEN,
      &priv_flags, &init_flags);
  if ( r != OK) {
      panic("sef_startup: sys_whoami failed: %d\n", r);
  }

  sef_self_proc_nr = _ENDPOINT_P(sef_self_endpoint);
  sef_self_priv_flags = priv_flags;
  sef_self_init_flags = init_flags;
  sef_lu_state = SEF_LU_STATE_NULL;
  old_endpoint = NONE;
  if(init_flags & SEF_LU_NOMMAP) {
      sys_upd_flags |= SF_VM_NOMMAP;
  }

#if USE_LIVEUPDATE
  /* RS may wake up with the wrong endpoint, perfom the update in that case. */
  if((sef_self_priv_flags & ROOT_SYS_PROC) && sef_self_endpoint != RS_PROC_NR) {
      r = vm_update(RS_PROC_NR, sef_self_endpoint, sys_upd_flags);
      if(r != OK) {
          panic("unable to update RS from instance %d to %d: %d",
              RS_PROC_NR, sef_self_endpoint, r);
      }
      old_endpoint = sef_self_endpoint;
      sef_self_endpoint = RS_PROC_NR;
  }
#endif /* USE_LIVEUPDATE */

#if INTERCEPT_SEF_INIT_REQUESTS
  /* Intercept SEF Init requests. */
  if(sef_self_priv_flags & ROOT_SYS_PROC) {
      /* RS initialization is special. */
      if((r = do_sef_rs_init(old_endpoint)) != OK) {
          panic("RS unable to complete init: %d", r);
      }
  }
  else if(sef_self_endpoint == VM_PROC_NR && __vm_init_fresh) {
      /* VM handles fresh initialization by RS later */
  } else {
      message m;

      /* Wait for an initialization message from RS. We need this to learn the
       * initialization type and parameters. When restarting after a crash, we
       * may get some spurious IPC messages from RS (e.g. update request) that
       * were originally meant to be delivered to the old instance. We discard
       * these messages and block till a proper initialization request arrives.
       */
      do {
          r = ipc_receive(RS_PROC_NR, &m, &status);
          if(r != OK) {
              panic("unable to ipc_receive from RS: %d", r);
          }
      } while(!IS_SEF_INIT_REQUEST(&m, status));

      /* Process initialization request for this system service. */
      if((r = do_sef_init_request(&m)) != OK) {
          panic("unable to process init request: %d", r);
      }
  }
#endif
  if((r = do_sef_fi_init(&sef_fi_callbacks_servers)) != 0) {
      panic("unable to startup if: %d", r);
  }

  /* (Re)initialize SEF variables. */
  sef_self_priv_flags = priv_flags;
  sef_self_init_flags = init_flags;
  sef_lu_state = SEF_LU_STATE_NULL;

  { int one = 1;
  memcpy(&sef_init_done, &one, sizeof(one));
  }
}

int volatile have_handled_message = 0;	/* currently working on a request message? */
static volatile message handled_message;		/* if so, this one */
static volatile int handled_message_status;
static int auto_set_handled_message = 1;  /* set handled message in sef_receive */
static int one = 1, zero = 0;

/*===========================================================================*
 *                         sef_handle_message                                *
 *===========================================================================*/
void sef_handle_message(message *m_ptr, int receive_status)
{
#ifdef _MINIX_DISTRIBUTED_RECOVERY
	if (ltckpt_is_req_rehandling && ltckpt_is_req_rehandling()) {
		return;
	}
#endif
	if(!m_ptr) { memcpy(&have_handled_message, &zero, sizeof(one)); return; }
	memcpy(&have_handled_message, &one, sizeof(one));
//	have_handled_message = 1;
	memcpy(&handled_message, m_ptr, sizeof(handled_message));
//	handled_message_status = receive_status;
	memcpy(&handled_message_status, &receive_status, sizeof(receive_status));
}

/*===========================================================================*
 *                         sef_get_current_message                           *
 *===========================================================================*/
int sef_get_current_message(message *m_ptr, int *replyable_bool)
{
	if(!have_handled_message) return 0;
	memcpy(m_ptr, &handled_message, sizeof(handled_message));
	if(replyable_bool) {
		*replyable_bool = 0;
		if (IPC_STATUS_FLAGS_TEST(handled_message_status, IPC_FLG_REPLY_OK)) {
			*replyable_bool = 1;
		}
	}
	return 1;
}

/*===========================================================================*
 *                         sef_get_current_message                           *
 *===========================================================================*/
void sef_cb_auto_message_handler(int activate)
{
	auto_set_handled_message = activate;
}

/*===========================================================================*
 *				sef_receive_status			     *
 *===========================================================================*/
int sef_receive_status(endpoint_t src, message *m_ptr, int *status_ptr)
{
/* SEF receive() interface for system services. */
  int r, status, m_type, recovered;

  sef_self_receiving = TRUE;

  /* One of the places we know for sure we're not handling any request message.
   * If we return from here without picking up a message (e.g. error), it will
   * remain NULL.
   */
  sef_handle_message(NULL, 0);

  while(TRUE) {
      /* If the caller indicated that it no longer wants to receive a message,
       * return now.
       */
      if (!sef_self_receiving)
          return EINTR;

#if INTERCEPT_SEF_LU_REQUESTS
      /* Handle SEF Live update before receive events. */
      if(sef_lu_state != SEF_LU_STATE_NULL) {
          do_sef_lu_before_receive();
      }

      /* Handle State transfer before receive events. */
      if(__sef_st_before_receive_enabled) {
          do_sef_st_before_receive();
      }
#endif

      /* not handling any message while blocking on ipc_receive */
      sef_handle_message(NULL, 0);

#ifdef _MINIX_DISTRIBUTED_RECOVERY
      /* reprocessing after recovering */
      int recovered = FALSE;
      if (ltckpt_is_req_rehandling && ltckpt_deactivate_req_rehandling) {
	if (ltckpt_is_req_rehandling()) {
		if (1 == sef_get_current_message(m_ptr, NULL)) {
		     	recovered = TRUE;
	      		status = handled_message_status;
	      		if(status_ptr) *status_ptr = status;
	     		 r = OK;
			 printf("sef_get_current_message() worked.\n");

	  	}
		else {
			printf("sef_get_current_message() failed.\n");
		}
		ltckpt_deactivate_req_rehandling();  
      	}
      }
      if (!recovered) {
#endif
      	/* Receive and return in case of error. */
      	r = ipc_receive(src, m_ptr, &status);
  
      	if(status_ptr) *status_ptr = status;
      	if(r != OK) {
       	      return r;
      	}
#ifdef _MINIX_DISTRIBUTED_RECOVERY
      }
#endif
      m_type = m_ptr->m_type;
      if (is_ipc_notify(status)) {
          switch (m_ptr->m_source) {
              case SYSTEM:
                  m_type = SEF_SIGNAL_REQUEST_TYPE;
              break;
              case RS_PROC_NR:
                  m_type = SEF_PING_REQUEST_TYPE;
              break;
          }
      }

      switch(m_type) {

#if INTERCEPT_SEF_INIT_REQUESTS
      case SEF_INIT_REQUEST_TYPE:
          /* Intercept SEF Init requests. */
          if(IS_SEF_INIT_REQUEST(m_ptr, status)) {
	      sef_handle_message(m_ptr, status);
              /* Ignore spurious init requests. */
              if (m_ptr->m_rs_init.type != SEF_INIT_FRESH
                  || sef_self_endpoint != VM_PROC_NR)
              continue;
          }
      break;
#endif

#if INTERCEPT_SEF_PING_REQUESTS
      case SEF_PING_REQUEST_TYPE:
          /* Intercept SEF Ping requests. */
          if(IS_SEF_PING_REQUEST(m_ptr, status)) {
	      sef_handle_message(m_ptr, status);
              if(do_sef_ping_request(m_ptr) == OK) {
                  continue;
              }
          }
      break;
#endif

#if INTERCEPT_SEF_LU_REQUESTS
      case SEF_LU_REQUEST_TYPE:
          /* Intercept SEF Live update requests. */
          if(IS_SEF_LU_REQUEST(m_ptr, status)) {
	      sef_handle_message(m_ptr, status);
              if(do_sef_lu_request(m_ptr) == OK) {
                  continue;
              }
          }
      break;
#endif

#if INTERCEPT_SEF_SIGNAL_REQUESTS
      case SEF_SIGNAL_REQUEST_TYPE:
          /* Intercept SEF Signal requests. */
          if(IS_SEF_SIGNAL_REQUEST(m_ptr, status)) {
	      sef_handle_message(m_ptr, status);
              if(do_sef_signal_request(m_ptr) == OK) {
                  continue;
              }
          }
      break;
#endif

#if INTERCEPT_SEF_GCOV_REQUESTS && USE_COVERAGE
      case SEF_GCOV_REQUEST_TYPE:
          /* Intercept GCOV data requests (sent by VFS in vfs/gcov.c). */
          if(IS_SEF_GCOV_REQUEST(m_ptr, status)) {
	      sef_handle_message(m_ptr, status);
              if(do_sef_gcov_request(m_ptr) == OK) {
                  continue;
              }
          }
      break;
#endif

#if INTERCEPT_SEF_FI_REQUESTS
      case SEF_FI_REQUEST_TYPE:
          /* Intercept SEF Fault Injection requests. */
          if(IS_SEF_FI_REQUEST(m_ptr, status)) {
	      sef_handle_message(m_ptr, status);
              if(do_sef_fi_request(m_ptr) == OK) {
                  continue;
              }
          }
      break;
#endif

#if INTERCEPT_SEF_METADATA_REQUESTS
      case SEF_METADATA_REQUEST_TYPE:
          /* Intercept SEF Metadata requests. */
          if(IS_SEF_METADATA_REQUEST(m_ptr, status)) {
	      sef_handle_message(m_ptr, status);
              if(do_sef_metadata_request(m_ptr) == OK) {
                  continue;
              }
          }
      break;
#endif

      default:
      break;
      }

      /* If we get this far, this is not a valid SEF request, return and
       * let the caller deal with that.
       */
      break;
  }

  /* By default, we are now said to be handling this request message.
   * The component can override this behaviour by changing auto_set_handled_message,
   * e.g. because it might receive a reply message that isn't a request message
   * which SEF can't tell.
   */
  if(auto_set_handled_message) { sef_handle_message(m_ptr, status); }

  return r;
}

/*===========================================================================*
 *				sef_self				     *
 *===========================================================================*/
endpoint_t sef_self(void)
{
/* Return the process's own endpoint number. */

  if (sef_self_endpoint == NONE)
	panic("sef_self called before initialization");

  return sef_self_endpoint;
}

/*===========================================================================*
 *				sef_cancel				     *
 *===========================================================================*/
void sef_cancel(void)
{
/* Cancel receiving a message. This function be called from a callback invoked
 * from within sef_receive_status(), which will then return an EINTR error
 * code. In particular, this function can be used to exit from the main receive
 * loop when a signal handler causes the process to want to shut down.
 */

  sef_self_receiving = FALSE;
}

/*===========================================================================*
 *                              sef_getrndseed              		     *
 *===========================================================================*/
int sef_getrndseed(void)
{
    clock_t uptime;
    sys_times(SELF, NULL, NULL, &uptime, NULL);
    return (int) uptime;
}

/*===========================================================================*
 *      	                  sef_exit                                   *
 *===========================================================================*/
void sef_exit(int status)
{
/* System services use a special version of exit() that generates a
 * self-termination signal.
 */
  /* Ask the kernel to exit. */
  sys_exit();

  /* If everything else fails, hang. */
  printf("Warning: system service %d couldn't exit\n", sef_self_endpoint);
  for(;;) { }
}

#ifdef __weak_alias
__weak_alias(_exit, sef_exit);
__weak_alias(__exit, sef_exit);
#endif

/*===========================================================================*
 *                                sef_munmap                                 *
 *===========================================================================*/
int sef_munmap(void *addrstart, vir_bytes len, int type)
{
/* System services use a special version of munmap() to control implicit
 * munmaps as startup and allow for asynchronous mnmap for VM.
 */
  message m;
  m.m_type = type;
  m.VMUM_ADDR = addrstart;
  m.VMUM_LEN = len;
  if(sef_self_endpoint == VM_PROC_NR) {
      return asynsend3(SELF, &m, AMF_NOREPLY);
  }
  return _syscall(VM_PROC_NR, type, &m);
}

#if SEF_INIT_DEBUG || SEF_LU_DEBUG || SEF_PING_DEBUG || SEF_SIGNAL_DEBUG
/*===========================================================================*
 *                         sef_debug_refresh_params              	     *
 *===========================================================================*/
static void sef_debug_refresh_params(void)
{
/* Refresh SEF debug params. */
  clock_t uptime;

  /* Get boottime and system hz the first time. */
  if(!sef_debug_init) {
      if (sys_times(NONE, NULL, NULL, NULL, &sef_debug_boottime) != OK)
	  sef_debug_init = -1;
      else if (sys_getinfo(GET_HZ, &sef_debug_system_hz,
        sizeof(sef_debug_system_hz), 0, 0) != OK)
	  sef_debug_init = -1;
      else
	  sef_debug_init = 1;
  }

  /* Get uptime. */
  uptime = -1;
  if (sef_debug_init < 1 || sys_times(NONE, NULL, NULL, &uptime, NULL) != OK) {
      sef_debug_time_sec = 0;
      sef_debug_time_us = 0;
  }
  else {
      /* Compute current time. */
      sef_debug_time_sec = (time_t) (sef_debug_boottime
          + (uptime/sef_debug_system_hz));
      sef_debug_time_us = (uptime%sef_debug_system_hz)
          * 1000000/sef_debug_system_hz;
  }
}

/*===========================================================================*
 *                              sef_debug_header              		     *
 *===========================================================================*/
char* sef_debug_header(void)
{
/* Build and return a SEF debug header. */
  sef_debug_refresh_params();
  snprintf(sef_debug_header_buff, sizeof(sef_debug_header_buff),
      "%s: time = %ds %06dus", sef_self_name, (int) sef_debug_time_sec,
      (int) sef_debug_time_us);

  return sef_debug_header_buff;
}
#endif /*SEF_INIT_DEBUG || SEF_LU_DEBUG || SEF_PING_DEBUG || SEF_SIGNAL_DEBUG*/

