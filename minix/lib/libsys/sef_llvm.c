#include "syslib.h"
#include "sef_llvm_inc.h"
#include "kernel/proc.h"
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <minix/sysutil.h>

/* Stack refs definitions. */
extern char **environ;
extern char **env_argv;
extern int env_argc;
extern char *sef_self_name_p;
extern endpoint_t sef_self_endpoint;

#define sef_llvm_stack_refs_save_one(P, T, R) { *((T*)P) = R; P += sizeof(T); }
#define sef_llvm_stack_refs_restore_one(P, T, R) { R = *((T*)P); P += sizeof(T); }

/*===========================================================================*
 *      	              sef_llvm_magic_enabled                         *
 *===========================================================================*/
int sef_llvm_magic_enabled()
{
    extern void __attribute__((weak)) magic_init();
    if (!magic_init)
        return 0;
    return 1;
}

/*===========================================================================*
 *      	                sef_llvm_real_brk                            *
 *===========================================================================*/
int sef_llvm_real_brk(char *newbrk)
{
    extern int __attribute__((weak)) _magic_real_brk(char*);
    if (!_magic_real_brk)
        return brk(newbrk);
    return _magic_real_brk(newbrk);
}

/*===========================================================================*
 *      	              sef_llvm_state_cleanup                         *
 *===========================================================================*/
int sef_llvm_state_cleanup()
{
    return OK;
}

/*===========================================================================*
 *      	                sef_llvm_dump_eval                           *
 *===========================================================================*/
void sef_llvm_dump_eval(char *expr)
{
    extern void __attribute__((weak)) _magic_dump_eval_bool(char*);
    if (!_magic_dump_eval_bool)
        return;
    return _magic_dump_eval_bool(expr);
}

/*===========================================================================*
 *      	               sef_llvm_eval_bool                            *
 *===========================================================================*/
int sef_llvm_eval_bool(char *expr, char *result)
{
    extern int __attribute__((weak)) magic_eval_bool(char*, char*);
    if (!magic_eval_bool)
        return 0;
    return magic_eval_bool(expr, result);
}

/*===========================================================================*
 *      	            sef_llvm_state_table_addr                        *
 *===========================================================================*/
void *sef_llvm_state_table_addr()
{
    extern void* __attribute__((weak)) _magic_vars_addr(void);
    if (!_magic_vars_addr)
        return NULL;
    return _magic_vars_addr();
}

/*===========================================================================*
 *      	            sef_llvm_state_table_size                        *
 *===========================================================================*/
size_t sef_llvm_state_table_size()
{
    extern size_t __attribute__((weak)) _magic_vars_size(void);
    if (!_magic_vars_size)
        return 0;
    return _magic_vars_size();
}

/*===========================================================================*
 *      	            sef_llvm_stack_refs_save                         *
 *===========================================================================*/
void sef_llvm_stack_refs_save(char *stack_buff)
{
    extern void __attribute__((weak)) st_stack_refs_save_restore(char*, int);
    char *p = stack_buff;

    sef_llvm_stack_refs_save_one(p, char**, environ);
    sef_llvm_stack_refs_save_one(p, char**, env_argv);
    sef_llvm_stack_refs_save_one(p, int, env_argc);

    if (st_stack_refs_save_restore)
        st_stack_refs_save_restore(p, 1);
}

/*===========================================================================*
 *      	           sef_llvm_stack_refs_restore                       *
 *===========================================================================*/
void sef_llvm_stack_refs_restore(char *stack_buff)
{
    extern void __attribute__((weak)) st_stack_refs_save_restore(char*, int);
    char *p = stack_buff;

    sef_llvm_stack_refs_restore_one(p, char**, environ);
    sef_llvm_stack_refs_restore_one(p, char**, env_argv);
    sef_llvm_stack_refs_restore_one(p, int, env_argc);

    if (st_stack_refs_save_restore)
        st_stack_refs_save_restore(p, 0);
}

/*===========================================================================*
 *      	            sef_llvm_state_transfer                          *
 *===========================================================================*/
int sef_llvm_state_transfer(sef_init_info_t *info)
{
    extern int __attribute__((weak)) _magic_state_transfer(sef_init_info_t *info);
    if (!_magic_state_transfer)
        return ENOSYS;
    return _magic_state_transfer(info);
}

/*===========================================================================*
 *      	        sef_llvm_add_special_mem_region                      *
 *===========================================================================*/
int sef_llvm_add_special_mem_region(void *addr, size_t len, const char* name)
{
    extern int __attribute__((weak)) st_add_special_mmapped_region(void *addr,
        size_t len, char* name);
    if (!st_add_special_mmapped_region)
        return 0;
    return st_add_special_mmapped_region(addr, len, (char*) name);
}

/*===========================================================================*
 *      	    sef_llvm_del_special_mem_region_by_addr                  *
 *===========================================================================*/
int sef_llvm_del_special_mem_region_by_addr(void *addr)
{
    extern int __attribute__((weak)) st_del_special_mmapped_region_by_addr(
        void *addr);
    if (!st_del_special_mmapped_region_by_addr)
        return 0;
    return st_del_special_mmapped_region_by_addr(addr);
}

/*===========================================================================*
 *      	    sef_llvm_ds_st_init                  *
 *===========================================================================*/
void sef_llvm_ds_st_init(void)
{
    extern void __attribute__((weak)) _magic_ds_st_init(void);
    if (!_magic_ds_st_init)
        return;
    _magic_ds_st_init();
}

/*===========================================================================*
 *      	    sef_llvm_ac_mmap                  *
 *===========================================================================*/
void* sef_llvm_ac_mmap(void *buf, size_t len, int prot, int flags, int fd, off_t offset, int ac_flags)
{
    int r;
    extern void* __attribute__((weak)) _magic_real_mmap(void*, size_t, int, int, int, off_t);
    if (!_magic_real_mmap || !(ac_flags & AC_NORELOC))
        return mmap(buf, len, prot, flags, fd, offset);

    /* Avoid regular dsentries for non-relocatable regions (e.g., DMA buffers). */
    buf = _magic_real_mmap(buf, len, prot, flags, fd, offset);
    if(buf == MAP_FAILED)
        return buf;
    r = sef_llvm_add_special_mem_region(buf, len, NULL);
    if(r < 0)
        printf("sef_llvm_add_special_mem_region failed: %d\n", r);
    return buf;
}

/*===========================================================================*
 *      	    sef_llvm_metadata_request                  *
 *===========================================================================*/
int sef_llvm_metadata_request(message *m_ptr)
{
    extern int __attribute__((weak)) _magic_metadata_request(message *m_ptr);
    if (!_magic_metadata_request) {
        m_ptr->m_lsys_metadata.status = ENOSYS;
        ipc_sendnb(m_ptr->m_source, m_ptr);
        return OK;
    }
    return _magic_metadata_request(m_ptr);
}

/*===========================================================================*
 *      	    sef_llvm_metadata_process_request                  *
 *===========================================================================*/
int sef_llvm_metadata_process_request(message *m_ptr, void *__m_data_ptr)
{
    extern int __attribute__((weak)) _magic_metadata_process_request(message *, void *);
    if (!_magic_metadata_process_request)
        return ENOSYS;
    return _magic_metadata_process_request(m_ptr, __m_data_ptr);
}

/*===========================================================================*
 *      	    sef_llvm_metadata_msg_size                  *
 *===========================================================================*/
size_t sef_llvm_metadata_msg_size(void)
{
    extern size_t __attribute__((weak)) _magic_metadata_msg_size(void);
    if (!_magic_metadata_msg_size)
        return 0;
    return _magic_metadata_msg_size();
}

/*===========================================================================*
 *      	    sef_llvm_metadata_buff_size                  *
 *===========================================================================*/
size_t sef_llvm_metadata_buff_size(void)
{
    extern size_t __attribute__((weak)) _magic_metadata_buff_size(void);
    if (!_magic_metadata_buff_size)
        return 0;
    return _magic_metadata_buff_size();
}

/*===========================================================================*
 *      	    sef_llvm_metadata_client_init                  *
 *===========================================================================*/
void sef_llvm_metadata_client_init(void)
{
    extern void __attribute__((weak)) _magic_metadata_client_init(void);
    if (!_magic_metadata_client_init)
        return;
    return _magic_metadata_client_init();
}

/*===========================================================================*
 *      	             sef_llvm_ltckpt_enabled                         *
 *===========================================================================*/
int sef_llvm_ltckpt_enabled()
{
    extern int __attribute__((weak)) ltckpt_mechanism_enabled(void);
    if (!sef_llvm_get_ltckpt_offset() || !ltckpt_mechanism_enabled())
        return 0;
    return 1;
}

/*===========================================================================*
 *      	            sef_llvm_ltckpt_get_offset                       *
 *===========================================================================*/
int sef_llvm_get_ltckpt_offset()
{
    extern int __attribute__((weak)) ltckpt_get_offset();
    if (!ltckpt_get_offset)
        return 0;
    return ltckpt_get_offset();
}

/*===========================================================================*
 *      	             sef_llvm_ltckpt_restart                         *
 *===========================================================================*/
int sef_llvm_ltckpt_restart(int type, sef_init_info_t *info)
{
    extern int __attribute__((weak)) ltckpt_restart(void *);
    int ret = 0;

    if(!sef_llvm_ltckpt_enabled())
    {
	char ist[100];
	int no_ist = 0;
	if (env_get_param("no_ist", ist, sizeof(ist)) == 0) {
		no_ist = 1;
	}
	if(!no_ist) {
		printf("%s:%d: sef_llvm_ltckpt_restart: doing identity state transfer\n", sef_self_name_p, sef_self_endpoint);
	        return sef_cb_init_identity_state_transfer(type, info);
	} else {
		printf("%s:%d: sef_llvm_ltckpt_restart: skipping identity state transfer\n", sef_self_name_p, sef_self_endpoint);
	        return OK;
	}
    }

    ret = ltckpt_restart(info);
    if (0 != ret)
        return ret;

#if 0
    message m;
    int replyable = 0;
    sef_get_current_message(&m, &replyable);

    /* recover */
    if (replyable) {
	/* reply error to source */
	m.m_type = ECRASH;
	ipc_send(m.m_source, &m);
    } else {
	/* can not reply error to source */
    }
#endif

    extern int __attribute__((weak)) ltckpt_recovery_dispatcher(void *);
    if(!ltckpt_recovery_dispatcher)
    {
        return ret;
    }
    return ltckpt_recovery_dispatcher(info);
}

/*===========================================================================*
 *      	             sef_llvm_is_user_endpoint                       *
 *===========================================================================*/
int sef_llvm_is_user_endpoint(endpoint_t e)
{
    struct proc lproc[NR_TASKS + NR_PROCS];
    int r, slot_init, slot_e;

    if((r = sys_getproctab(lproc)) != OK)
    {
        printf("Unable to access kernel process table.\n");
        return -1;
    }
    slot_init = INIT_PROC_NR + NR_TASKS;
    slot_e = _ENDPOINT_P(e) + NR_TASKS;

    if(lproc[slot_init].p_rts_flags & RTS_SLOT_FREE)
    {
        printf("Slot belonging to init is empty.\n");
	return -1;
    }
    if(lproc[slot_e].p_rts_flags & RTS_SLOT_FREE)
    {
        printf("sef_llvm : Slot belonging to specified endpoint (%d) is empty.\n", e);
        return -1;
    }
    printf("sef_llvm : endpoint : %d \tprocess name : %s\n", slot_e, lproc[slot_e].p_name);
    if (lproc[slot_init].p_priv == lproc[slot_e].p_priv)
    {
        // We should not dereference these pointers, as they refer to kernel memory
        return 1; // user process
    }
    return 0; // NOT a user process
}
