#ifndef HYPERMEM_H
#define HYPERMEM_H

#include <stddef.h>
#include <sys/types.h>

#include <minix/hypermem-api.h>

/* hypermem-lowlevel.c */
extern const struct sef_fi_callbacks sef_fi_callbacks_servers;

/* hypermem-api.c */
struct hypermem_session {
    const struct sef_fi_callbacks *callbacks;
    void *state;
};

int hypermem_connect(const struct hypermem_session *session);
void hypermem_disconnect(const struct hypermem_session *session);

void hypermem_edfi_context_set(const struct hypermem_session *session,
    const char *name, const void *context, ptrdiff_t ptroffset);
void hypermem_edfi_dump_stats(const struct hypermem_session *session,
    const char *message);
void hypermem_edfi_dump_stats_module(const struct hypermem_session *session,
    const char *name, const char *message);
int hypermem_edfi_faultindex_get(const struct hypermem_session *session,
    const char *name);
void hypermem_fault(const struct hypermem_session *session, const char *name,
    unsigned bbindex);
int hypermem_nop(const struct hypermem_session *session);
void hypermem_print(const struct hypermem_session *session, const char *str);
void hypermem_quit(const struct hypermem_session *session);
void hypermem_release_cr3(const struct hypermem_session *session,
    uint32_t cr3);
void hypermem_set_cr3(const struct hypermem_session *session);

#endif /* !defined(HYPERMEM_H) */
