#include <inc/env.h>
#include <security/secipc.h>
#include <inc/lib.h>
#include <inc/mmu.h>

static envid_t secenv = 0;


static union secipc_buffer *sec_send = (union secipc_buffer *)0x10000000;
static struct user_info *sec_resp = (struct user_info *)0x10001000;

int get_user_by_id(uid_t uid, struct user_info *user_info)
{
	if(secenv == 0)
		secenv = ipc_find_env(ENV_TYPE_SECURITY);

	sys_page_alloc(0, sec_send, PTE_W | PTE_U | PTE_P);
	sec_send->uid_req.uid = uid;
	ipc_send(secenv, NAME2INFO, sec_send, PTE_P | PTE_U | PTE_W);
	
	envid_t from = 0;
	while(from != secenv)
	{
		int r = ipc_recv(&from, sec_resp, NULL);
		if(r < 0)
			return r;
	}

	memmove(user_info, sec_resp, sizeof(user_info)); 
	sys_page_unmap(0, sec_resp);

	return 0;
}

int get_user_by_name(char *name, struct user_info *user_info)
{
	if(secenv == 0)
		secenv = ipc_find_env(ENV_TYPE_SECURITY);

	return -1;
}
int verify_password(uid_t uid, char *pass)
{
	if(secenv == 0)
		secenv = ipc_find_env(ENV_TYPE_SECURITY);

	return -1;
}
