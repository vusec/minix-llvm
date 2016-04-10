/* IPC filter definitions. */

#ifndef _MINIX_IPC_FILTER_H
#define _MINIX_IPC_FILTER_H

#include <minix/com.h>
#include <minix/config.h>

/* IPC filter constants. */
#define IPCF_MAX_ELEMENTS       NR_SYS_PROCS

/* IPC filter flags. */
#define IPCF_MATCH_M_SOURCE    0x1
#define IPCF_MATCH_M_TYPE      0x2
#define IPCF_EL_BLACKLIST      0x4
#define IPCF_EL_WHITELIST      0x8

/* IPC filter element macros. */
#define IPCF_EL_FLAG_C(E,F) (((E)->flags & F) ? '1' : '0')
#define IPCF_EL_PRINT(E,PRINTF) PRINTF("IPCF_EL=(flags(STBW)=%c%c%c%c, m_source=%d, m_type=%d)", \
    IPCF_EL_FLAG_C(E, IPCF_MATCH_M_SOURCE), IPCF_EL_FLAG_C(E, IPCF_MATCH_M_TYPE), \
    IPCF_EL_FLAG_C(E, IPCF_EL_BLACKLIST), IPCF_EL_FLAG_C(E, IPCF_EL_WHITELIST), \
    (E)->m_source, (E)->m_type);

struct ipc_filter_el_s {
    int flags;
    endpoint_t m_source;
    int m_type;
};
typedef struct ipc_filter_el_s ipc_filter_el_t;

#endif /* _MINIX_IPC_FILTER_H */
