#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <paths.h>
#include <limits.h>
#include <signal.h>
#include <minix/rs.h>
#include <minix/ipc.h>
#include <minix/sef.h>
#include <minix/prox.h>

#ifndef OK
#define OK 0
#endif

#define DEBUG 0

static endpoint_t prox_ep;
static char* prox_buff;

static void prox_refresh_ep(void)
{
    int r = minix_rs_lookup(PROX_LABEL, &prox_ep);
    if(r < 0) {
        printf("libprox: cannot talk to " PROX_LABEL ": %d\n", r);
        exit(1);
    }
}

static void prox_sendrec(message *m_ptr)
{
    int r;
    r = ipc_sendrec(prox_ep, m_ptr);
    if(r != OK) {
        prox_refresh_ep();
        r = ipc_sendrec(prox_ep, m_ptr);
    }
    if(r != OK) {
        printf("libprox: cannot sendrec to " PROX_LABEL "\n");
        exit(1);
    }
}

static void prox_handler(int signum)
{
    message m;

#if DEBUG
    printf("libprox: Received signal %d, communicating with " PROX_LABEL " %d\n", signum, prox_ep);
#endif

    m.m_type = COMMON_REQ_UPCALL_REPLY;
    m.m_lsys_upcall_reply.buff = (vir_bytes) prox_buff;
    m.m_lsys_upcall_reply.result = ENOSYS;
    prox_sendrec(&m);
    if(m.m_type != SEF_METADATA_REQUEST_TYPE) {
        printf("libprox: Warning: " PROX_LABEL " user request failed!\n");
        return;
    }
    /* Process the request. */
    m.m_lsys_metadata.status = sef_llvm_metadata_process_request(&m, prox_buff);

    /* Send an answer. */
    prox_sendrec(&m);
}

void prox_init(void)
{
    prox_buff = (char*) malloc(sef_llvm_metadata_msg_size());
    signal(SIGUPCALL, prox_handler);
    prox_refresh_ep();
}

