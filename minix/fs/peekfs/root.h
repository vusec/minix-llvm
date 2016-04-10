#ifndef _PEEKFS_ROOT_H_
#define _PEEKFS_ROOT_H_

#include <minix/peekfs.h>

extern struct peekfs_conf *peekfs_conf;
extern struct peekfs_ctl *peekfs_ctl;

void root_init(void);
int root_get_nr_idx_entries(void);
struct inode_stat * root_get_stat(void);
int root_lookup(struct inode *root, char *name);
int root_getdents(struct inode *root);
int root_mount_user(struct peekfs_mount *mount);
int root_umount_user(struct peekfs_umount *umount);

#endif /* !_PEEKFS_ROOT_H_ */
