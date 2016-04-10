/* Function prototypes. */

/* Structs used in prototypes must be declared as such first. */
struct rproc;
struct rprocupd;

/* exec.c */
int srv_execve(int proc_e, char *exec, size_t exec_len, char *argv[],
	char **env); /* depends on VM and PM */

/* main.c */
int main(void);

/* request.c */
int do_up(message *m); /* depends on VM, PM, VFS, DS, devman and PCI */
int do_down(message *m); /* depends on VM, PM, DS, devman and PCI */
int do_refresh(message *m); /* depends on PM */
int do_restart(message *m); /* depends on VM, PM and DS */
int do_clone(message *m); /* depends on VM, PM and DS */
int do_unclone(message *m); /* depends on VM, PM and DS */
int do_edit(message *m); /* depends on VM, PM and DS */
int do_shutdown(message *m); /* depends on PM */
void do_period(message *m); /* depends on VM, PM and DS */
int do_init_ready(message *m); /* depends on VM, PM and DS */
int do_update(message *m); /* depends on VM, PM and DS */
int do_upd_ready(message *m); /* depends on VM, PM and DS */
void do_sigchld(void); /* depends on VM, PM and DS */
int do_getsysinfo(message *m);
int do_lookup(message *m);
int do_sysctl(message *m); /* depends on VM, PM and DS */
int do_fi(message *m); /* depends on PM */

/* manager.c */
int check_call_permission(endpoint_t caller, int call, struct rproc
	*rp); /* depends on PM */
int copy_rs_start(endpoint_t src_e, char *src_rs_start, struct rs_start
	*rs_start);
int copy_label(endpoint_t src_e, char *src_label, size_t src_len, char
	*dst_label, size_t dst_len);
int init_state_data(endpoint_t src_e, int prepare_state,
	struct rs_state_data *src_rs_state_data,
	struct rs_state_data *dst_rs_state_data); /* depends on DS */
void build_cmd_dep(struct rproc *rp);
#define kill_service(rp, errstr, err) \
	kill_service_debug(__FILE__, __LINE__, rp, errstr, err)
int kill_service_debug(char *file, int line, struct rproc *rp, char
	*errstr, int err);
#define crash_service(rp) \
	crash_service_debug(__FILE__, __LINE__, rp)
int crash_service_debug(char *file, int line, struct rproc *rp);
#define cleanup_service(rp) \
	cleanup_service_debug(__FILE__, __LINE__, rp)
#define cleanup_service_now(rp) \
	do { struct rproc *rpt = rp; cleanup_service(rpt); cleanup_service(rpt); } while(0)
void cleanup_service_debug(char *file, int line,
	struct rproc *rp); /* depends on VM, PM and DS */
#define detach_service(rp) \
	detach_service_debug(__FILE__, __LINE__, rp)
void detach_service_debug(char *file, int line,
	struct rproc *rp); /* depends on DS */
int create_service(struct rproc *rp); /* depends on VM, PM and DS */
int clone_service(struct rproc *rp, int instance_flag, int init_flags); /* depends on VM, PM and DS */
int publish_service(struct rproc *rp); /* depends on PM, VFS, DS, devman and PCI */
int unpublish_service(struct rproc *rp); /* depends on DS, devman and PCI */
int run_service(struct rproc *rp, int init_type, int init_flags);
int start_service(struct rproc *rp, int init_flags); /* depends on VM, PM, VFS, DS and devman */
void stop_service(struct rproc *rp,int how);
void activate_service(struct rproc *rp, struct rproc *ex_rp);
void terminate_service(struct rproc *rp); /* depends on VM, PM, VFS, DS, devman and PCI */
void restart_service(struct rproc *rp); /* depends on VM, PM and DS */
void inherit_service_defaults(struct rproc *def_rp,
	struct rproc *rp);
void get_service_instances(struct rproc *rp, struct rproc ***rps,
	int *length);
int read_exec(struct rproc *rp);
void share_exec(struct rproc *rp_src, struct rproc *rp_dst);
void free_exec(struct rproc *rp);
int init_slot(struct rproc *rp, struct rs_start *rs_start,
	endpoint_t source);
int edit_slot(struct rproc *rp, struct rs_start *rs_start,
	endpoint_t source);
int clone_slot(struct rproc *rp, struct rproc **clone_rpp);
void swap_slot(struct rproc **src_rpp, struct rproc **dst_rpp);
struct rproc* lookup_slot_by_label(char *label);
struct rproc* lookup_slot_by_pid(pid_t pid);
struct rproc* lookup_slot_by_dev_nr(dev_t dev_nr);
struct rproc* lookup_slot_by_flags(int flags);
int alloc_slot(struct rproc **rpp);
void free_slot(struct rproc *rp);
char *get_next_label(char *ptr, char *label, char *caller_label);
void add_forward_ipc(struct rproc *rp, struct priv *privp);
void add_backward_ipc(struct rproc *rp, struct priv *privp);
void init_privs(struct rproc *rp, struct priv *privp);
void end_srv_init(struct rproc *rp); /* depends on VM, PM and DS */

/* update.c */
void rupdate_clear_upds(void); /* depends on VM, PM and DS */
void rupdate_add_upd(struct rprocupd* rpupd);
void rupdate_set_new_upd_flags(struct rprocupd* rpupd);
void rupdate_upd_init(struct rprocupd* rpupd, struct rproc *rp);
void rupdate_upd_clear(struct rprocupd* rpupd);
void rupdate_upd_move(struct rproc* src_rp, struct rproc* dst_rp);
#define request_prepare_update_service(rp, state) \
	request_prepare_update_service_debug(__FILE__, __LINE__, rp, state)
void request_prepare_update_service_debug(char *file, int line,
	struct rproc *rp, int state);
int srv_update(endpoint_t src_e, endpoint_t dst_e, int sys_upd_flags); /* depends on VM */
int update_service(struct rproc **src_rpp,
	struct rproc **dst_rpp, int swap_flag, int sys_upd_flags); /* depends on VM */
void rollback_service(struct rproc **src_rpp,
	struct rproc **dst_rpp); /* depends on VM */
void update_period(message *m_ptr);
int start_update_prepare(int allow_retries); /* depends on VM, PM and DS */
struct rprocupd* start_update_prepare_next(void);
int start_update(void); /* depends on VM, PM and DS */
int start_srv_update(struct rprocupd *rpupd); /* depends on VM, PM and DS */
int complete_srv_update(struct rprocupd *rpupd); /* depends on VM, PM and DS */
void end_srv_update(struct rprocupd *rpupd, int result, int reply_flag);
int abort_update_proc(int reason); /* depends on VM, PM and DS */
#define end_update(result, reply_flag) \
	end_update_debug(__FILE__, __LINE__, result, reply_flag)
void end_update_debug(char *file, int line,
        int result, int reply_flag); /* depends on VM, PM and DS */

/* utility.c */
int init_service(struct rproc *rp, int type, int flags);
int fi_service(struct rproc *rp, int type);
void fill_send_mask(sys_map_t *send_mask, int set_bits);
void fill_call_mask( int *calls, int tot_nr_calls,
	bitchunk_t *call_mask, int call_base, int is_init);
#define srv_to_string(RP) srv_to_string_gen(RP, DEBUG)
char* srv_to_string_gen(struct rproc *rp, int is_verbose);
char* srv_upd_to_string(struct rprocupd *rpupd);
int rs_asynsend(struct rproc *rp, message *m_ptr, int no_reply);
int rs_receive_ticks(endpoint_t src, message *m_ptr,
    int *status_ptr, int ticks);
void reply(endpoint_t who, struct rproc *rp, message *m_ptr);
void late_reply(struct rproc *rp, int code);
int rs_isokendpt(endpoint_t endpoint, int *proc);
int sched_init_proc(struct rproc *rp);
int update_sig_mgrs(struct rproc *rp, endpoint_t sig_mgr,
	endpoint_t bak_sig_mgr);
int rs_is_idle(void);
void rs_idle_period(void); /* depends on VM, PM and DS */
void rs_set_vulnerable(int vulnerable);
void print_services_status(void);
void print_update_status(void);

#define RSCHECK_ERR(expr) (rscheck_err((expr), __FILE__, __LINE__))
#define RSCHECK_INT(expr) (errno = 0, rscheck_int((expr), __FILE__, __LINE__))
#define RSCHECK_PTR(expr) (errno = 0, rscheck_ptr((expr), __FILE__, __LINE__))

int rscheck_err(int r, const char *file, int line);
int rscheck_int(int r, const char *file, int line);
void *rscheck_ptr(void *r, const char *file, int line);

/* error.c */
char * init_strerror(int errnum);
char * lu_strerror(int errnum);

