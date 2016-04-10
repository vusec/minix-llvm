#include <assert.h>

#include "syslib.h"

#include "hypermem.h"

struct hypermem_state {
    volatile hypermem_entry_t *address;
    volatile hypermem_entry_t *mem_base;
};

/*===========================================================================*
 *                              get_hypermem_vaddr                           *
 *===========================================================================*/
static volatile hypermem_entry_t *get_hypermem_vaddr(void) {
#if defined(__i386__)
    vir_bytes hypermem_vaddr;
    sys_gethypermem(&hypermem_vaddr);
    return (volatile hypermem_entry_t *) hypermem_vaddr;
#else
    return NULL;
#endif /* defined(__i386__) */
}

/*===========================================================================*
 *                             hypermem_read                                 *
 *===========================================================================*
 * This function can be used to read a value from a hypermem address
 */
static hypermem_entry_t hypermem_read_impl(
    struct hypermem_state *state) {
    assert(state);
    return *state->address;
}

/*===========================================================================*
 *                               hypermem_write                              *
 *===========================================================================*/
static void hypermem_write_impl(struct hypermem_state *state,
                                hypermem_entry_t value) {
    assert(state);
    *state->address = value;
}

/*===========================================================================*
 *                              hypermem_connect                             *
 *===========================================================================*/
static int hypermem_connect_impl(struct hypermem_state *state) {
    static int unavailable = 0;
    hypermem_entry_t channel, channelidx;
    volatile hypermem_entry_t *hypermem_vaddr;

    assert(state);

    if (unavailable) return -1;

    /* The kernel will remap the hypermem memory interval for us at hypermem_vaddr */
    hypermem_vaddr = get_hypermem_vaddr();
    if (!hypermem_vaddr) {
	unavailable = 1;
	return -1;
    }
    state->address = state->mem_base = hypermem_vaddr;

    channel = hypermem_read_impl(state);
    if (channel < HYPERMEM_BASEADDR ||
        channel >= (HYPERMEM_BASEADDR + HYPERMEM_SIZE) ||
	channel % sizeof(hypermem_entry_t) != 0) {
	unavailable = 1;
        return -1;
    }
    channelidx = (channel - HYPERMEM_BASEADDR) / sizeof(hypermem_entry_t);
    state->address = state->mem_base + channelidx;
    hypermem_write_impl(state, HYPERMEM_COMMAND_CONNECT);
    return 0;
}

/*===========================================================================*
 *                          hypermem_disconnect                              *
 *===========================================================================*/
static void hypermem_disconnect_impl(struct hypermem_state *state) {
    hypermem_entry_t value;

    assert(state);

    hypermem_write_impl(state, HYPERMEM_COMMAND_DISCONNECT);
}

/*===========================================================================*
 *                          hypermem_get_cr3                                 *
 *===========================================================================*/
static uint32_t hypermem_get_cr3_impl(void) {
    int r;
    uint32_t reg;
    r = sys_vmctl_get_pdbr(SELF, &reg);
    if (r != OK) {
        printf("hypermem_get_cr3_impl: sys_vmctl_get_pdbr failed: %d\n", r);
	reg = 0;
    }
    return reg;
}

/*===========================================================================*
 *                              sef_fi_callbacks_servers                     *
 *===========================================================================*/
const struct sef_fi_callbacks sef_fi_callbacks_servers = {
    .state_size          = sizeof(struct hypermem_state),
    .hypermem_connect    = hypermem_connect_impl,
    .hypermem_disconnect = hypermem_disconnect_impl,
    .hypermem_get_cr3    = hypermem_get_cr3_impl,
    .hypermem_read       = hypermem_read_impl,
    .hypermem_write      = hypermem_write_impl,
};
