#include <assert.h>
#include <string.h>

#include "syslib.h"

#include "hypermem.h"

/*===========================================================================*
 *                             hypermem_read                                 *
 *===========================================================================*
 * This function can be used to read a value from a hypermem address
 */
static hypermem_entry_t hypermem_read(const struct hypermem_session *session) {
    assert(session);
    assert(session->callbacks);
    assert(session->callbacks->hypermem_read);
    return session->callbacks->hypermem_read(session->state);
}

/*===========================================================================*
 *                               hypermem_write                              *
 *===========================================================================*/
static void hypermem_write(const struct hypermem_session *session,
                           hypermem_entry_t value) {
    assert(session);
    assert(session->callbacks);
    assert(session->callbacks->hypermem_write);
    session->callbacks->hypermem_write(session->state, value);
}

/*===========================================================================*
 *                             hypermem_write_string                        *
 *===========================================================================*/
static void hypermem_write_string(const struct hypermem_session *session,
	const char *str) {
	hypermem_entry_t buf;
	size_t chunk, len = strlen(str);

	hypermem_write(session, len);
	while (len > 0) {
		chunk = sizeof(hypermem_entry_t);
		if (chunk > len) chunk = len;
		buf = 0;
		memcpy(&buf, str, chunk);
		hypermem_write(session, buf);
		str += chunk;
		len -= chunk;
	}
}

/*===========================================================================*
 *                              hypermem_connect                             *
 *===========================================================================*/
int hypermem_connect(const struct hypermem_session *session) {
    assert(session);
    assert(session->callbacks);
    if (!(session->callbacks->hypermem_connect)) return -1;
    return session->callbacks->hypermem_connect(session->state);
}

/*===========================================================================*
 *                          hypermem_disconnect                              *
 *===========================================================================*/
void hypermem_disconnect(const struct hypermem_session *session) {
    assert(session);
    assert(session->callbacks);
    assert(session->callbacks->hypermem_disconnect);
    session->callbacks->hypermem_disconnect(session->state);
}

/*===========================================================================*
 *                       hypermem_edfi_context_set                           *
 *===========================================================================*
 * After startup it is advisable to set the proper context in hypermem so that
 * it can fetch data at any point from the running process without a need for
 * the process to be running.
 */
void hypermem_edfi_context_set(const struct hypermem_session *session,
    const char *name, const void *context, ptrdiff_t ptroffset) {
    hypermem_write(session, HYPERMEM_COMMAND_EDFI_CONTEXT_SET);
    hypermem_write(session, (hypermem_entry_t)context);
    hypermem_write(session, (hypermem_entry_t) ptroffset);
    hypermem_write_string(session, name);
}

/*===========================================================================*
 *                          hypermem_dump_stats                              *
 *===========================================================================*
 * Dump stats for all the hypermem registered processes
 */
void hypermem_edfi_dump_stats(const struct hypermem_session *session,
    const char *message) {
    hypermem_write(session, HYPERMEM_COMMAND_EDFI_DUMP_STATS);
    hypermem_write_string(session, message);
}

/*===========================================================================*
 *                      hypermem_dump_stats_module                           *
 *===========================================================================*
 * Dump stats for the current module.
 */
void hypermem_edfi_dump_stats_module(const struct hypermem_session *session,
    const char *name, const char *message) {
    hypermem_write(session, HYPERMEM_COMMAND_EDFI_DUMP_STATS_MODULE);
    hypermem_write_string(session, name);
    hypermem_write_string(session, message);
}

/*===========================================================================*
 *                    hypermem_edfi_faultindex_get                           *
 *===========================================================================*
 * When we use edfi this is used to ask the hypervisor which fault to enable
 */
int hypermem_edfi_faultindex_get(const struct hypermem_session *session,
    const char *name) {
    hypermem_write(session, HYPERMEM_COMMAND_EDFI_FAULTINDEX_GET);
    hypermem_write_string(session, name);
    return hypermem_read(session);
}

/*===========================================================================*
 *                            hypermem_fault                                 *
 *===========================================================================*
 * Hypermem call to inform the hypervisor that we have hit a fault
 */
void hypermem_fault(const struct hypermem_session *session, const char *name,
    unsigned bbindex) {
    hypermem_write(session, HYPERMEM_COMMAND_FAULT);
    hypermem_write(session, bbindex);
    hypermem_write_string(session, name);
}

/*===========================================================================*
 *                              hypermem_nop                                 *
 *===========================================================================*
 * The hypermem nop command, which can be used for testing purposes
 */
int hypermem_nop(const struct hypermem_session *session) {
    hypermem_write(session, HYPERMEM_COMMAND_NOP);
    return hypermem_read(session) == HYPERCALL_NOP_REPLY;
}

/*===========================================================================*
 *                              hypermem_print                               *
 *===========================================================================*
 * The print command can also be used for debugging and testing purposes, since
 * it send characters one by one to hypermem, thus not needing any pointer resolution.
 */
void hypermem_print(const struct hypermem_session *session, const char *str) {
    hypermem_write(session, HYPERMEM_COMMAND_PRINT);
    hypermem_write_string(session, str);
}

/*===========================================================================*
 *                              hypermem_quit                               *
 *===========================================================================*
 * The quit command shuts down QEMU instantly.
 */
void hypermem_quit(const struct hypermem_session *session) {
    hypermem_write(session, HYPERMEM_COMMAND_QUIT);
}

/*===========================================================================*
 *                            hypermem_release_cr3                           *
 *===========================================================================*
 * This function tells hypermem the specified page table is no longer valid and
 * any module associated with it should be forgotten.
 */
void hypermem_release_cr3(const struct hypermem_session *session,
                          uint32_t cr3) {
    hypermem_write(session, HYPERMEM_COMMAND_RELEASE_CR3);
    hypermem_write(session, (hypermem_entry_t)cr3);
}

/*===========================================================================*
 *                            hypermem_set_cr3                               *
 *===========================================================================*
 * This function is used to inform qemu (hypermem) of our own cr3 register 
 * value since we don't know for sur that hypermem will take over exactly from 
 * this process
 */
void hypermem_set_cr3(const struct hypermem_session *session) {
    uint32_t reg = session->callbacks->hypermem_get_cr3();
    hypermem_write(session, HYPERMEM_COMMAND_SET_CR3);
    hypermem_write(session, (hypermem_entry_t)reg);
}
