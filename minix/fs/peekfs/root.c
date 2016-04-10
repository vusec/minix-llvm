#include "root.h"

#include <minix/ds.h>
#include <minix/sysinfo.h>
#include <minix/type.h>		/* typedef int endpoint_t */
#include <minix/ds.h>
#include <machine/archtypes.h>
#include <errno.h>
#include "component.h"
#include "../../servers/pm/mproc.h"
#include "kernel/proc.h"

#define MOUNT_USER_DEBUG        0
#define DSF_IS_LABEL(m)	(((m) & DSF_MASK_TYPE) == DSF_TYPE_LABEL)	/* is a label */

typedef struct data_store ixfer_data_store_t;
static ixfer_data_store_t ds_store[NR_DS_KEYS];

static struct inode_stat root_stat = DIR_STAT_INIT;
static struct inode_stat entry_stat = DIR_STAT_INIT;

static struct muproc {
  int in_use;
  char name[NAME_MAX];
  endpoint_t ep;
  pid_t pid;
} muproc[NR_MOUNTED_USER_PROCS];

/*====================================================================*
 *        root_init                                                   *
 *====================================================================*/
void root_init(void)
{
  /* No slot in use. */
  memset(muproc, 0, sizeof(muproc));
}

/*====================================================================*
 *        root_get_nr_idx_entries                                     *
 *====================================================================*/
int root_get_nr_idx_entries(void)
{
  return array_len(ds_store, struct data_store)
	+ array_len(muproc, struct muproc);
}

/*====================================================================*
 *        root_get_stat                                               *
 *====================================================================*/
struct inode_stat * root_get_stat(void)
{
  return &root_stat;
}

/*====================================================================*
 *        update_ds_store                                             *
 *====================================================================*/
static int root_update_ds_store(void)
{
  return getsysinfo(DS_PROC_NR, SI_DATA_STORE, ds_store, sizeof(ds_store));
}

/*====================================================================*
 *        user_pid_to_endpoint                                        *
 *====================================================================*/
static int user_pid_to_endpoint(pid_t pid, endpoint_t *ep)
{
  static struct mproc mproc[NR_PROCS];
  int i;

  getsysinfo(PM_PROC_NR, SI_PROC_TAB, mproc, sizeof(mproc));

  for (i=0; i<NR_PROCS; i++) {
	if(pid == mproc[i].mp_pid) {
		if(mproc[i].mp_flags & PRIV_PROC) {
			return EPERM;
		}
		*ep = mproc[i].mp_endpoint;
		return OK;
	}
  }

  return ENOENT;
}

/*====================================================================*
 *        root_refresh_entries                                        *
 *====================================================================*/
static void root_refresh_entries(void)
{
  int i, r, entries_nr;
  struct inode *node, *root;
  cbarray_t *curr_array, new_array;
  endpoint_t endpt;

  root = get_root_inode();

  for (i = 0; i < NR_DS_KEYS; ++i) {
	node = get_inode_by_index(root, i);

	/* If the process slot is not in use or is not a LABEL type,
	 * delete the associated inode (if there was one) and skip this
	 * entry.
	 */
	if ( ! (ds_store[i].flags & DSF_IN_USE && DSF_IS_LABEL(ds_store[i].flags)) ) {
		if (node != NULL) delete_inode(node);
		continue;
	}

	/* Handling of existing inodes. */
	if (node != NULL) {
		/* Refresh the component endpoint. */
		curr_array = (cbarray_t *) get_inode_extra(node);
		if ( (r=ds_retrieve_label_endpt(get_inode_name(node),
			&curr_array->component_e)) != OK ) {
			if ( r != ESRCH ) printf("PEEKFS: ds_retrieve_label_endpt() error [%d]\n", r);
			delete_inode(node);
		}
		continue;
	}

	/* Skip init by design. */
	if (! strcmp(ds_store[i].key, "init")) continue;

	/* Find the number of indexed entries. */
	if (!component_get_nr_idx_entries ||
		(component_get_nr_idx_entries(ds_store[i].u.u32, 0, &entries_nr) != OK)) {
		/* The error is EDEADSRCDST - skip this entry. */
		continue;
	}

	new_array.component_e = ds_store[i].u.u32;
	new_array.is_user = 0;

	if (cbarray_refresh_selement_key)
		cbarray_refresh_selement_key(&new_array, NULL);

	node = add_inode(root, ds_store[i].key, i, &entry_stat,
		entries_nr, (cbdata_t) 0);
	assert(node != NULL);
	memcpy(get_inode_extra(node), &new_array, sizeof(new_array));
  }
}

/*====================================================================*
 *        root_refresh_user_entries                                   *
 *====================================================================*/
static void root_refresh_user_entries(void)
{
  int i, entries_nr;
  struct inode *node, *root;
  cbarray_t *curr_array, new_array;
  endpoint_t endpt;
  struct proc p;

  root = get_root_inode();

  for(i = 0; i < NR_MOUNTED_USER_PROCS; ++i) {
	node = get_inode_by_index(root, i+NR_DS_KEYS);

	/* If the process slot is not in use, delete the associated
	 * inode (if there was one) and skip this entry.
	 */
	if ( !muproc[i].in_use ) {
		if (node != NULL) delete_inode(node);
		continue;
	}

	/* Handling of existing inodes. */
	if (node != NULL) {
		/* Check if the user process endpoint still exists. */
		endpoint_t ep;
		curr_array = (cbarray_t *) get_inode_extra(node);
		if (curr_array->component_e != muproc[i].ep || sys_getproc(&p, muproc[i].ep) != OK) {
			muproc[i].in_use = 0;
			delete_inode(node);
		}
		if(ds_retrieve_label_endpt(muproc[i].name, &ep) == OK) {
			printf("PEEKFS: Mounted user process superseded by system process, cleaning up...\n");
			muproc[i].in_use = 0;
			delete_inode(node);
		}
		continue;
	}

	/* Find the number of indexed entries. */
	if (!component_get_nr_idx_entries
		|| (component_get_nr_idx_entries(muproc[i].ep, 1, &entries_nr) != OK)) {
		/* The error is EDEADSRCDST - skip this entry. */
		muproc[i].in_use = 0;
		continue;
	}

	new_array.component_e = muproc[i].ep;
	new_array.is_user = 1;

	if (cbarray_refresh_selement_key)
		cbarray_refresh_selement_key(&new_array, NULL);

	node = add_inode(root, muproc[i].name, i+NR_DS_KEYS, &entry_stat,
		entries_nr, (cbdata_t) 0);
	assert(node != NULL);
	memcpy(get_inode_extra(node), &new_array, sizeof(new_array));
  }
}

/*====================================================================*
 *        root_refresh                                                *
 *====================================================================*/
static void root_refresh(void)
{
  int r;

  if( (r = root_update_ds_store()) != OK ) {
	printf("PEEKFS: Couldn't talk to DS: %d.\n", r);
	return;
  }
  root_refresh_entries();
  root_refresh_user_entries();
}

/*====================================================================*
 *        root_lookup                                                 *
 *====================================================================*/
int root_lookup(struct inode *root, char *name)
{
  root_refresh();
  
  return OK;
}

/*====================================================================*
 *        root_getdents                                               *
 *====================================================================*/
int root_getdents(struct inode *root)
{
  root_refresh();
  
  return OK;
}

/*====================================================================*
 *        root_mount_user                                             *
 *====================================================================*/
int root_mount_user(struct peekfs_mount *mount)
{
  int i, r, slot = -1;
  endpoint_t ep;

#if MOUNT_USER_DEBUG
  printf("MOUNT: %s --> %d\n", mount->name, mount->pid);
#endif

  root_refresh();
  for(i=0;i<NR_MOUNTED_USER_PROCS;i++) {
	if(!muproc[i].in_use) {
		if(slot < 0) slot = i;
	}
	else if(!strcmp(mount->name, muproc[i].name)) {
#if MOUNT_USER_DEBUG
  printf("MOUNT: EEXIST\n");
#endif
		return EEXIST;
	}
  }
  if(slot < 0) {
#if MOUNT_USER_DEBUG
  printf("MOUNT: ENOMEM\n");
#endif
	return ENOMEM;
  }
  if(ds_retrieve_label_endpt(mount->name, &ep) == OK) {
#if MOUNT_USER_DEBUG
  printf("MOUNT: EEXIST2\n");
#endif
	return EEXIST;
  }
  if((r=user_pid_to_endpoint(mount->pid, &ep)) != OK) {
#if MOUNT_USER_DEBUG
  printf("MOUNT: user_pid_to_endpoint failed\n");
#endif
	return r;
  }

  muproc[slot].in_use = 1;
  muproc[slot].pid = mount->pid;
  muproc[slot].ep = ep;
  strcpy(muproc[slot].name, mount->name);

#if MOUNT_USER_DEBUG
  printf("MOUNT: Done for slot %d, endpoint %d\n", slot, ep);
#endif

  return OK;
}

/*====================================================================*
 *        root_umount_user                                             *
 *====================================================================*/
int root_umount_user(struct peekfs_umount *umount)
{
  int i;

#if MOUNT_USER_DEBUG
  printf("UMOUNT: %s\n", umount->name);
#endif

  for(i=0;i<NR_MOUNTED_USER_PROCS;i++) {
	if(!strcmp(umount->name, muproc[i].name)) {
		muproc[i].in_use = 0;
#if MOUNT_USER_DEBUG
                printf("MOUNT: Done for slot %d, endpoint %d\n", i, muproc[i].ep);
#endif
		return OK;
	}
  }

  return ENOENT;
}

