//#define QEMU_HYPERMEM
#include "sef_fi.h"
#include "hypermem.h"

extern __attribute__((weak)) int edfi_ctl_process_request(void *ctl_request);

__attribute__((weak)) edfi_onfault_t edfi_onfault_p;
__attribute__((weak)) edfi_onfdp_t edfi_onfdp_p;
__attribute__((weak)) char *edfi_module_name = NULL;
__attribute__((weak)) edfi_context_t *edfi_context = NULL;
__attribute__((weak)) int edfi_faultinjection_enabled = 0;
__attribute__((weak)) int edfi_inject_bb = -1;
__attribute__((weak)) int inside_trusted_compute_base = -1;

static const struct sef_fi_callbacks *stored_callbacks;

/* Miscellanous
 *===========================================================================*
 *                            hypermem_notify_stop                           *
 *===========================================================================*
 * The notify stop is to be used by RS to notify hypermem when a process has
 * stopped unnaturally, either crash or shutdown
 */
int hypermem_notify_stop(const char *str, int signo)
{
    char buff[64];

    if (signo == 0) {
        /* Since normally there is no signal for 0 then the process exited normally */
        snprintf(buff, sizeof(buff), "rs-NORMAL: down sent for %s", str);
    }
    else {
        /* We were called as part of the signal delivery routine for the target */
        snprintf(buff, sizeof(buff), "rs-CRASH: signal [%d] sent for %s", signo, str);
    }

    hypermem_printstr(buff);
    return 0;
}

/*===========================================================================*
 *                            hypermem_printstr		                     *
 *===========================================================================*
 * Wrapper of hypermem_print for external consumption by ltckpt. Unlike the
 * others, it is declared in <minix/syslib.h> and does not use hypermem-related
 * types in its signature.
 */
int hypermem_printstr(const char *msg) {
    char statebuf[HYPERMEM_STATE_SIZE_MAX];
    struct hypermem_session session = {
	.callbacks = stored_callbacks,
	.state = statebuf,
    };

    if (!stored_callbacks) return -1;
    assert(sizeof(statebuf) >= stored_callbacks->state_size);

    if (hypermem_connect(&session) < 0) return -1;
    hypermem_print(&session, msg);
    hypermem_disconnect(&session);
    return 0;
}

/*===========================================================================*
 *                            hypermem_release_pagetable                     *
 *===========================================================================*
 * Wrapper of hypermem_set_cr3 for external consumption by vm. Unlike the
 * others, it is declared in <minix/syslib.h> and does not use hypermem-related
 * types in its signature.
 */
int hypermem_release_pagetable(uint32_t physaddr) {
    char statebuf[HYPERMEM_STATE_SIZE_MAX];
    struct hypermem_session session = {
	.callbacks = stored_callbacks,
	.state = statebuf,
    };

    if (!stored_callbacks) return -1;
    assert(sizeof(statebuf) >= stored_callbacks->state_size);

    if (hypermem_connect(&session) < 0) return -1;
    hypermem_release_cr3(&session, physaddr);
    hypermem_disconnect(&session);
    return 0;
}

/*===========================================================================*
 *                            hypermem_shutdown		                     *
 *===========================================================================*
 * Wrapper of hypermem_quit for external consumption by kernel. Unlike the
 * others, it is declared in <minix/syslib.h> and does not use hypermem-related
 * types in its signature.
 */
int hypermem_shutdown(void) {
    char statebuf[HYPERMEM_STATE_SIZE_MAX];
    struct hypermem_session session = {
	.callbacks = stored_callbacks,
	.state = statebuf,
    };

    if (!stored_callbacks) return -1;
    assert(sizeof(statebuf) >= stored_callbacks->state_size);

    if (hypermem_connect(&session) < 0) return -1;
    hypermem_quit(&session);
    hypermem_disconnect(&session);
    return 0;
}

/*===========================================================================*
 *                        edfi_module_alter_run_name                         *
 *===========================================================================*
 * This is used to get a unique id for the process from its name and command 
 * command line arguments
 */
static void edfi_module_alter_run_name() 
{
    static char fname[256];
    char *ptr;

    int i;
    if (env_argc > 1) {
        sprintf(fname, "%s@", edfi_module_name);
        for (i = 1; i < env_argc; ++i) {
            ptr = fname + strlen(fname);
            sprintf(ptr, "[%s]", env_argv[i]);
        }
        edfi_module_name = fname;
    }

    return;
}

static void edfi_minix_ctl_init()
{
    unsigned i;

    if (edfi_context->bb_num_executions) 
    {
        edfi_context->bb_num_executions[0] = EDFI_CANARY_VALUE;
#if 0
        for(i = 1; i <= edfi_context->num_bbs; i++) {
            edfi_context->bb_num_executions[i] = 0;
        }
#endif
        edfi_context->bb_num_executions[edfi_context->num_bbs + 1] = EDFI_CANARY_VALUE;
    }
}

extern __attribute__((weak)) int sef_fi_custom(void);

/* Functions that are called from sef
 *===========================================================================*
 *                            do_sef_fi_request                          *
 *===========================================================================*/
int do_sef_fi_request(message *m_ptr)
{
    /* See if we are simply asked to crash. */
    if (m_ptr->m_lsys_fi_ctl.subtype == RS_FI_CRASH) {
	inside_trusted_compute_base = 0; 
        panic("Crash!");
    }

    if (m_ptr->m_lsys_fi_ctl.subtype == RS_FI_CUSTOM) {
		if (sef_fi_custom) {
			return sef_fi_custom();
		} else {
			return ENOSYS;
		}
	}

#if SEF_FI_ALLOW_EDFI
    /* Forward the request to the EDFI fault injector, if linked in. */
    if(edfi_ctl_process_request)
        return edfi_ctl_process_request(m_ptr);
#endif

    return ENOSYS;
}

/*===========================================================================*
 *                            onfault_hyper                                  *
 *===========================================================================*/
static void onfault_hyper(int bb_index)
{
    static int exec_count = 3;
    char statebuf[HYPERMEM_STATE_SIZE_MAX];
    struct hypermem_session session = {
	.callbacks = stored_callbacks,
	.state = statebuf,
    };

    if (!stored_callbacks) return;
    assert(sizeof(statebuf) >= stored_callbacks->state_size);

    if (exec_count == 0)
        return;
    exec_count --;
    if (hypermem_connect(&session) == -1)
        return;

    hypermem_fault(&session, edfi_module_name, bb_index);
    hypermem_disconnect(&session);
    return;
}

/*===========================================================================*
 *                            do_sef_fi_init                          *
 *===========================================================================*/

int do_sef_fi_init(const struct sef_fi_callbacks *callbacks)
{
    char statebuf[HYPERMEM_STATE_SIZE_MAX];
    struct hypermem_session session = {
	.callbacks = callbacks,
	.state = statebuf,
    };
    char buff[64];
    int verbose = 0;
    /* Check to see whether we are linked against edfi */

    if (edfi_module_name == NULL) {
        return 0;
    }

    if (!stored_callbacks) stored_callbacks = callbacks;
    assert(stored_callbacks == callbacks);
    assert(sizeof(statebuf) >= callbacks->state_size);

    edfi_module_alter_run_name();
    /* Try and connect to the hypermem interface */
    if(hypermem_connect(&session) == -1) {
        /* Silently fail since there might not be a hypermem interface to connect to */
        return 0;
    }
    /* Inform the hypervisor who we are via a print message */
    snprintf(buff, sizeof(buff), "<EDFI> startup %s", edfi_module_name);
    hypermem_print(&session, buff);

    /* Inform the hypervisor of our cr3 unless we are kernel */
    hypermem_set_cr3(&session);
    hypermem_edfi_context_set(&session, edfi_module_name, edfi_context, 0);

    edfi_inject_bb = edfi_context->c.faulty_bb_index = hypermem_edfi_faultindex_get(&session, edfi_module_name);

    if ((edfi_faultinjection_enabled  == 0)
    && (edfi_context->c.faulty_bb_index != 0)) {
        edfi_faultinjection_enabled = 1;
        edfi_onfault_p = onfault_hyper;
    }
    edfi_minix_ctl_init();
    if (verbose)
        hypermem_edfi_dump_stats(&session, "do_sef_fi_init");
    else
        hypermem_edfi_dump_stats_module(&session, edfi_module_name, "do_sef_fi_init");

    hypermem_disconnect(&session);
    return 0;
}
