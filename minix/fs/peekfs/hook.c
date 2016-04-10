#include <minix/peekfs.h>
#include <minix/com.h>
#include <minix/ipc.h>
#include <minix/type.h>
#include <time.h>
#include "component.h"
#include "root.h"

static int proxctl_hook(struct inode *node);

/*====================================================================*
 *        init_hook                                                   *
 *====================================================================*/
static void init_hook(void)
{
  sef_llvm_metadata_client_init();
  root_init();
}

/*====================================================================*
 *        lookup_hook                                                 *
 *====================================================================*/
static int lookup_hook(struct inode *parent, char *name,
  cbdata_t cbdata)
{
/* Path name resolution hook, for a specific parent and name pair.
 * If needed, update our own view of the system first; after that,
 * determine whether we need to (re)generate certain files.
 */
  int r;

  if (get_root_inode() == parent) {
	/* We are in the /prox root. */
	r = root_lookup(parent, name);
  } else {
	/* We are in a component's directory or sub-directory. */
	r = component_lookup ? component_lookup(parent, name) : ENOSYS;
  }
  return r;
}

/*====================================================================*
 *        getdents_hook                                               *
 *====================================================================*/
static int getdents_hook(struct inode *dir, cbdata_t cbdata)
{
/* Directory entry retrieval hook, for potentially all files in a
 * directory. Make sure that all files that are supposed to be
 * returned, are actually part of the virtual tree.
 */
  int r;

  if (get_root_inode() == dir) {
	/* We are in the /prox root. */
	r = root_getdents(dir);
  } else {
	/* We are in a component's directory or sub-directory. */
	r = component_getdents ? component_getdents(dir) : ENOSYS;
  }
  return r;
}

/*====================================================================*
 *        read_hook                                                   *
 *====================================================================*/
static ssize_t read_hook(struct inode *node, char *ptr, size_t len, off_t off,
  cbdata_t cbdata)
{
  /* This hook will be called every time a regular file is read. We use
   * it to dynamically generate the contents of our file.
   */

  /* Currently no regfile reading is allowed in the peekfs root. */
  assert(get_parent_inode(node) != get_root_inode());

  return component_read ? component_read(node, ptr, len, off) : ENOSYS;
}

/*====================================================================*
 *        write_hook                                                  *
 *====================================================================*/
static ssize_t write_hook(struct inode *node, char *ptr, size_t len, off_t off,
  cbdata_t cbdata)
{
  /* This hook will be called every time a regular file is written. We
   * use it to dynamically generate the contents of our file.
   */
  int r;

  /* Currently no regfile writing is allowed in the peekfs root. */
  assert(get_parent_inode(node) != get_root_inode());

  r = component_write ? component_write(node, ptr, len, off) : ENOSYS;
  if(r >= 0 && peekfs_ctl->new_req) {
	return proxctl_hook(node);
  }
  return r;
}

/*====================================================================*
 *        rdlink_hook                                                 *
 *====================================================================*/
static int rdlink_hook(struct inode *node, char *path_buf, size_t size,
  cbdata_t cbdata)
{
  /* This hook will be called every time a symbolic link target is
   * retrieved.
   */

  /* Currently no symlink is allowed in the peekfs root. */
  assert(get_parent_inode(node) != get_root_inode());

  return component_readlink ? component_readlink(node, path_buf, size) : ENOSYS;
}

/*====================================================================*
 *        trunc_hook                                                  *
 *====================================================================*/
static int trunc_hook(struct inode *node, off_t size, cbdata_t cbdata)
{
  /* This hook will be called every time a regular file is truncated. */

  /* Currently no regfile truncation is allowed in the peekfs root. */
  assert(get_parent_inode(node) != get_root_inode());

  return component_truncate ? component_truncate(node, size) : ENOSYS;
}

/*====================================================================*
 *        slink_hook                                                  *
 *====================================================================*/
static int slink_hook(struct inode *node, char *lname, struct inode_stat *stat,
  char *target, cbdata_t cbdata)
{
  /* This hook will be called every time a symlink is created. */

  /* Currently no symlink support is implemented in the peekfs root. */
  if (node == get_root_inode()) return ENOSYS;

  return component_symlink ? component_symlink(node, lname, target) : ENOSYS;
}

/* The hook functions that will be called by VTreeFS. */
struct fs_hooks hooks = {
	.init_hook	= init_hook,
	.lookup_hook	= lookup_hook,
	.getdents_hook	= getdents_hook,
	.read_hook	= read_hook,
	.write_hook	= write_hook,
	.trunc_hook	= trunc_hook,
	.rdlink_hook	= rdlink_hook,
	.slink_hook	= slink_hook,
};

/*====================================================================*
 *        proxctl_hook                                                *
 *====================================================================*/
static int proxctl_hook(struct inode *node)
{
  /* This hook will be called every time a write is issued for the regular
   * file _PATH_PEEK_CTL.
   */
  int r;
  peekfs_ctl->new_req = 0;

  switch(peekfs_ctl->req_type) {
	case PEEK_CTL_MOUNT:
		r = root_mount_user(&peekfs_ctl->req.mount);
	break;
	case PEEK_CTL_UMOUNT:
		r = root_umount_user(&peekfs_ctl->req.umount);
	break;
	default:
		r = ENOSYS;
	break;
  }

  return r;
}

