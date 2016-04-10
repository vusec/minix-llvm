#include <minix/peekfs.h>

/* This is not set as static so that the name of the struct doesn't get suffixed
 * with a huge id number when seen through peekfs. */
struct peekfs_conf conf = {
        .debugging_mode = 1,
        .raw_io = 0,
        .symlink_abs_path = 0,
        .allow_overflow = 0,
        .skip_byte_indices = 1,
        .max_array_indices = 2*NR_PROCS,
};

static struct peekfs_ctl ctl = {
        .new_req = 0,
        .req_type = 0,
        .req = {
                .mount = {0},
                .umount = {0}
        }
};

/* Used externally. */
struct peekfs_conf *peekfs_conf = &conf;
struct peekfs_ctl *peekfs_ctl = &ctl;
