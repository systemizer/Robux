#ifndef JOS_INC_SYSCALL_H
#define JOS_INC_SYSCALL_H

/* system call numbers */
enum {
	SYS_cputs = 0,
	SYS_cgetc,
	SYS_getenvid,
	SYS_env_destroy,
	SYS_page_alloc,
	SYS_page_map,
	SYS_page_unmap,
	SYS_exofork,
	SYS_env_set_status,
	SYS_env_set_trapframe,
	SYS_env_set_pgfault_upcall,
	SYS_yield,
	SYS_ipc_try_send,
	SYS_ipc_recv,
	SYS_time_msec,
	SYS_net_send_packet,
	SYS_net_recv_packet,
	SYS_get_mac_addr,
	SYS_get_env_user_id,
	SYS_set_user_id,
	SYS_get_env_group_id,
	SYS_set_group_id,
	NSYSCALLS
};

#endif /* !JOS_INC_SYSCALL_H */
