#ifndef _EDFI_HYPER_H
#define _EDFI_HYPER_H
#include <sys/types.h>
#include <sys/mman.h>
#include <minix/vm.h>
#include <minix/mmio.h>
#include <assert.h>
#include <minix/sysutil.h>
#include <string.h>

#include "syslib.h"

#define SHUTDOWN_DOWN                           1
#define SHUTDOWN_SIGNAL                         2

#ifdef _QEMU_HYPERMEM_
#define fi_printf(f_, ...)                                          \
    do {                                                            \
        if (((int)SELF) > LAST_SPECIAL_PROC_NR)                     \
            printf((f_), __VA_ARGS__);                              \
    } while(0)
#else
#define fi_printf(f_, ...)                                          \
    do { } while(0)
#endif


int hypermem_notify_stop(const char *str, int reason);
int do_sef_fi_request(message *m_ptr);

/* Extern variables */
EXTERN int env_argc;
EXTERN char **env_argv;

/* this can be redefined to force pointer size to 32 or 64 bit */
#define POINTER(type) type*
#ifndef PACKED
#  define PACKED __attribute__((packed))
#endif

#define EDFI_DFLIB_PATH_MAX                     (512)
#define EDFI_CANARY_VALUE                       0xff0a0011


/* EDFI context definitions. */
typedef unsigned long long exec_count;
typedef struct {
    POINTER(char) name;
    POINTER(int) bb_num_injected;
    POINTER(int) bb_num_candidates;
} fault_type_stats;

typedef struct {
    float fault_prob;
    unsigned long fault_prob_randmax;
    int min_fdp_interval;
    int faulty_bb_index;
    unsigned int min_fault_time_interval;
    unsigned int max_time;
    unsigned long long max_faults;
    unsigned int rand_seed;
    char dflib_path[EDFI_DFLIB_PATH_MAX];
} PACKED edfi_context_conf_t;

typedef struct {
    unsigned int canary_value1;
    int fault_fdp_count;
    unsigned long long fault_time;
    unsigned long long start_time;
    unsigned long long total_faults;
    POINTER(fault_type_stats) fault_type_stats;
    POINTER(exec_count) bb_num_executions; /* canaries in first and last elements */
    POINTER(int) bb_num_faults;
    unsigned int num_bbs;
    unsigned int num_fault_types;
    int no_svr_thread;
    POINTER(char) output_dir;
    int num_requests_on_start;
    int verbosity;
    edfi_context_conf_t c;
    unsigned int canary_value2;
} PACKED edfi_context_t;

#undef POINTER

enum edfi_context_t_fields {
    EDFI_CONTEXT_FIELD_CANARY_VALUE1,
    EDFI_CONTEXT_FIELD_FAULT_FDP_COUNT,
    EDFI_CONTEXT_FIELD_FAULT_TIME,
    EDFI_CONTEXT_FIELD_START_TIME,
    EDFI_CONTEXT_FIELD_TOTAL_FAULTS,
    EDFI_CONTEXT_FIELD_FAULT_TYPE_STATS,
    EDFI_CONTEXT_FIELD_BB_NUM_EXECUTIONS,
    EDFI_CONTEXT_FIELD_BB_NUM_FAULTS,
    EDFI_CONTEXT_FIELD_NUM_BBS,
    EDFI_CONTEXT_FIELD_NUM_FAULT_TYPES,
    EDFI_CONTEXT_FIELD_NO_SVR_THREAD,
    EDFI_CONTEXT_FIELD_OUTPUT_DIR,
    EDFI_CONTEXT_FIELD_NUM_REQUESTS_ON_START,
    EDFI_CONTEXT_FIELD_VERBOSITY,
    EDFI_CONTEXT_FIELD_C,
    EDFI_CONTEXT_FIELD_CANARY_VALUE2
};


/* DF handlers. */
typedef void (*edfi_onstart_t)(char *params);
typedef int (*edfi_onfdp_t)(int);
typedef void (*edfi_onfault_t)(int bb_index);
typedef void (*edfi_onstop_t)(void);


void edfi_ctl_init();


#endif /* _EDFI_HYPER_H */
