#include <assert.h>

#include "syslib.h"

#include "hypermem.h"

/*===========================================================================*
 *                              sef_fi_callbacks_servers                     *
 *===========================================================================*/
/* not yet supported on ARM */
const struct sef_fi_callbacks sef_fi_callbacks_servers = {
    .state_size          = 0,
    .hypermem_connect    = NULL,
    .hypermem_disconnect = NULL,
    .hypermem_get_cr3    = NULL,
    .hypermem_read       = NULL,
    .hypermem_write      = NULL,
};
