#include "kernel/kernel.h"
#include <assert.h>

#include "hypermem-lowlevel.h"

/*===========================================================================*
 *                              sef_fi_callbacks_kernel                     *
 *===========================================================================*/
/* just a stub for now on ARM */
const struct sef_fi_callbacks sef_fi_callbacks_kernel = {
    .state_size          = 0,
    .hypermem_connect    = NULL,
    .hypermem_disconnect = NULL,
    .hypermem_get_cr3    = NULL,
    .hypermem_read       = NULL,
    .hypermem_write      = NULL,
};
