/* ProcFS - hypermem.c - generator for the hypermem file */

#include "inc.h"

#if defined(__i386__)
#include "../../kernel/arch/i386/include/archconst.h"
#endif

/*
 * Generate the contents of /proc/hypermem.
 */
void
root_hypermem(void)
{
	vir_bytes vaddr;

	if (sys_gethypermem(&vaddr)) {
		printf("PROCFS: cannot get hypermem vaddr\n");
		return;
	}

	buf_printf("0x%lx\n", (long) vaddr);
}
