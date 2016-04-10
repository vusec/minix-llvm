#include "kernel/kernel.h"
#include "arch_proto.h"
#include <assert.h>

#include "../../hypermem-lowlevel.h"

struct hypermem_state {
    phys_bytes address;
};

/*===========================================================================*
 *                             hypermem_read                                 *
 *===========================================================================*
 * This function can be used to read a value from a hypermem address
 */
static hypermem_entry_t hypermem_read_impl(
    struct hypermem_state *state) {
    struct vir_addr src = { .proc_nr_e = NONE };
    struct vir_addr dst = { .proc_nr_e = KERNEL };
    hypermem_entry_t value;

    assert(state);

    src.offset = state->address;
    dst.offset = (vir_bytes) &value;
    virtual_copy_f(NULL, &src, &dst, sizeof(value), 0, 0);
    return value;
}

/*===========================================================================*
 *                               hypermem_write                              *
 *===========================================================================*/
static void hypermem_write_impl(struct hypermem_state *state,
                                hypermem_entry_t value) {
    struct vir_addr src = { .proc_nr_e = KERNEL };
    struct vir_addr dst = { .proc_nr_e = NONE };

    assert(state);

    src.offset = (vir_bytes) &value;
    dst.offset = state->address;
    virtual_copy_f(NULL, &src, &dst, sizeof(value), 0, 0);
}

/*===========================================================================*
 *                              hypermem_connect                             *
 *===========================================================================*/
static int hypermem_connect_impl(struct hypermem_state *state) {
    static int unavailable = 0;

    assert(state);

    if (unavailable) return -1;

    state->address = HYPERMEM_BASEADDR;
    state->address = hypermem_read_impl(state);
    if (state->address < HYPERMEM_BASEADDR ||
        state->address >= (HYPERMEM_BASEADDR + HYPERMEM_SIZE) ||
	state->address % sizeof(hypermem_entry_t) != 0) {
	unavailable = 1;
        return -1;
    }
    hypermem_write_impl(state, HYPERMEM_COMMAND_CONNECT);
    return 0;
}

/*===========================================================================*
 *                          hypermem_disconnect                              *
 *===========================================================================*/
static void hypermem_disconnect_impl(struct hypermem_state *state) {

    assert(state);

    hypermem_write_impl(state, HYPERMEM_COMMAND_DISCONNECT);
}

/*===========================================================================*
 *                          hypermem_get_cr3                                 *
 *===========================================================================*/
static uint32_t hypermem_get_cr3_impl(void) {
    return read_cr3();
}

/*===========================================================================*
 *                              sef_fi_callbacks_kernel                     *
 *===========================================================================*/
const struct sef_fi_callbacks sef_fi_callbacks_kernel = {
    .state_size          = sizeof(struct hypermem_state),
    .hypermem_connect    = hypermem_connect_impl,
    .hypermem_disconnect = hypermem_disconnect_impl,
    .hypermem_get_cr3    = hypermem_get_cr3_impl,
    .hypermem_read       = hypermem_read_impl,
    .hypermem_write      = hypermem_write_impl,
};
