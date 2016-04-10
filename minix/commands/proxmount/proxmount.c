/* proxmount - mount/unmount user processes on peekfs.
 * Author: Cristiano Giuffrida
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <minix/paths.h>

#include <minix/peekfs.h>

void ctl_req(int fd, struct peekfs_ctl *ctl)
{
    int ret;

    ctl->new_req = 1;
    ret = write(fd, ctl, sizeof(struct peekfs_ctl));

    if (ret < 0) {
        printf("peekfs ctl failed:%d\n", ret);
        exit(1);
    }
}

void exit_usage()
{
    printf("Usage: proxmount [-u] <name> [<pid>]\n");
    exit(1);
}

int main(int argc, char **argv)
{
    int fd, ret;
    struct peekfs_ctl ctl = { 1, PEEK_CTL_MOUNT };
    int index = 1;
    char *name;
    pid_t *pid;

    /* Check arguments. */
    if(argc != 3) {
        exit_usage();
    }

    /* Get the request type. */
    if(!strcmp(argv[index], "-u")) {
        index++;
        ctl.req_type = PEEK_CTL_UMOUNT;
        name = ctl.req.umount.name;
        pid = NULL;
    }
    else {
        name = ctl.req.mount.name;
        pid = &ctl.req.mount.pid;
    }

    /* Get the name. */
    strncpy(name, argv[index++], NAME_MAX);

    /* Get the pid. */
    if(ctl.req_type == PEEK_CTL_MOUNT) {
        char *buff;
        errno=0;
        *pid = strtol(argv[index], &buff, 10);
        if(errno || strcmp(buff, "") || *pid <= 0) {
            printf("Error: bad pid\n");
            exit_usage();
            exit(1);
        }
    }

    fd = open(_PATH_PEEK_CTL, O_WRONLY);
    if (fd < 0) {
        printf("Can't open %s\n", _PATH_PEEK_CTL);
        exit(1);
    }

    ctl_req(fd, &ctl);

    close(fd);
    return 0;
}

