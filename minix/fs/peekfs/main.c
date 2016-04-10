#include <minix/peekfs.h>

#include "root.h"

extern struct fs_hooks hooks;

/*====================================================================*
 *        main                                                        *
 *====================================================================*/
int main(void)
{

  run_vtreefs(&hooks, NR_INODES, sizeof(cbarray_t), root_get_stat(),
	root_get_nr_idx_entries(), sef_llvm_metadata_buff_size());

  return 0;
}
