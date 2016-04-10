#ifndef _PEEKFS_COMPONENT_H_
#define _PEEKFS_COMPONENT_H_

#include <minix/peekfs.h>

/* The following functions are implemented in the shared static library
 * -lmagic. */
EXTERN_WEAK int component_get_nr_idx_entries(endpoint_t component_e,
	int is_user, int *entries_nr);
EXTERN_WEAK int component_lookup(struct inode *parent, char *name);
EXTERN_WEAK int component_getdents(struct inode *dir);
EXTERN_WEAK ssize_t component_read(struct inode *node, char *ptr, size_t len,
	off_t off);
EXTERN_WEAK ssize_t component_write(struct inode *node, char *ptr, size_t len,
	off_t off);
EXTERN_WEAK int component_truncate(struct inode *node, off_t size);
EXTERN_WEAK int component_readlink(struct inode *node, char *path_buf,
	size_t size);
EXTERN_WEAK int component_symlink(struct inode *node, char *lname,
	char *target);
EXTERN_WEAK void cbarray_refresh_selement_key(cbarray_t *dst_cba,
	const void *src_selement);

#endif /* !_PEEKFS_COMPONENT_H_ */
