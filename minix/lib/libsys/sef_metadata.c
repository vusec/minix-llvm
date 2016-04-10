#include "syslib.h"

#include <minix/sysutil.h>

/* SEF Metadata prototypes for sef_receive(). */
int do_sef_metadata_request(message *m_ptr);

int do_sef_metadata_request(message *m_ptr)
{
    return sef_llvm_metadata_request(m_ptr);
}

