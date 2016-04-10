/* Prototypes and definitions for ProxFS. */

#ifndef _MINIX_PEEKFS_H
#define _MINIX_PEEKFS_H

#define _MINIX          1
#define _POSIX_SOURCE	1
#define _MINIX_SYSTEM	1
#define _SYSTEM		1

#include <minix/config.h>
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>

#include <minix/const.h>
#include <minix/type.h>
#include <minix/drivers.h>
#include <minix/syslib.h>
#include <minix/vtreefs.h>

#ifndef EXTERN_WEAK
#define EXTERN_WEAK extern __attribute__((weak))
#endif

/* Returns the length of the array A of type T. */
#define array_len(A, T) \
  ( sizeof( (A) ) / sizeof(T) )

/* Number of preallocated inodes. */
#define NR_INODES		1536

/* Maximum number of mounted user processes. */
#define NR_MOUNTED_USER_PROCS     32

#define PEEK_CTL_MOUNT   1
#define PEEK_CTL_UMOUNT  2

struct peekfs_mount
{
  pid_t pid;
  char name[NAME_MAX];
};

struct peekfs_umount
{
  char name[NAME_MAX];
};

/* Various file modes. */
#define REG_ALL_MODE	(S_IFREG | 0444)	/* world-readable regular */
#define DIR_ALL_MODE	(S_IFDIR | 0555)	/* world-accessible directory */
#define LNK_ALL_MODE	(S_IFLNK | 0777)	/* symbolic link */

#define REG_STAT_INIT			\
{					\
  REG_ALL_MODE,		/* mode */	\
  SUPER_USER,		/* uid */	\
  SUPER_USER,		/* gid */	\
  0,			/* size */	\
  NO_DEV,		/* dev */	\
}

static inline void INIT_REG_STAT(struct inode_stat *stat)
{
  stat->mode  = REG_ALL_MODE;
  stat->uid   = SUPER_USER;
  stat->gid   = SUPER_USER;
  stat->size  = 0;
  stat->dev   = NO_DEV;
}

#define DIR_STAT_INIT			\
{					\
  DIR_ALL_MODE,		/* mode */	\
  SUPER_USER,		/* uid */	\
  SUPER_USER,		/* gid */	\
  0,			/* size */	\
  NO_DEV,		/* dev */	\
}

static inline void INIT_DIR_STAT(struct inode_stat *stat)
{
  stat->mode  = DIR_ALL_MODE;
  stat->uid   = SUPER_USER;
  stat->gid   = SUPER_USER;
  stat->size  = 0;
  stat->dev   = NO_DEV;
}

static inline void INIT_STAT(struct inode_stat *stat)
{
  stat->uid   = SUPER_USER;
  stat->gid   = SUPER_USER;
  stat->size  = 0;
  stat->dev   = NO_DEV;
}

/* CB array definitions. */
typedef struct cbarray_s
{
  /* Component endpoint. */
  endpoint_t component_e;	/* typedef int endpoint_t */
  int is_user;

  /* Selement-related fields. */
  void *sentry_ptr;
  int sentry_flags;
  void *type_ptr;
  void *address;
  unsigned int dsentry_id;
} cbarray_t;

/* Configuration. */
struct peekfs_conf
{
  int debugging_mode;
  int raw_io;
  int symlink_abs_path;
  int allow_overflow;
  int skip_byte_indices;
  int max_array_indices;
};

struct peekfs_ctl_req {
  struct peekfs_mount mount;
  struct peekfs_umount umount;
};

struct peekfs_ctl
{
  int new_req;
  int req_type;
  struct peekfs_ctl_req req;
};

#endif /* _MINIX_PEEKFS_H */

