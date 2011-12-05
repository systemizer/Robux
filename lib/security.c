#include <inc/env.h>
#include <security/secipc.h>
#include <inc/lib.h>
#include <inc/mmu.h>

static envid_t secenv = 0;


static union secipc_buffer *sec_send = (union secipc_buffer *)0x10000000;
static struct user_info *sec_resp = (struct user_info *)0x10001000;


void
alloc_send()
{
	sys_page_alloc(0, sec_send, PTE_W | PTE_U | PTE_P);
}

void
send_req(uint32_t num)
{
	ipc_send(secenv, num, sec_send, PTE_P | PTE_U | PTE_W);
}

int
wait_resp()
{
	envid_t from = 0;
	int r = 0;
	while(from != secenv)
	{
		r = ipc_recv(&from, sec_resp, NULL);
		if(r < 0)
			break;
	}

	return r;
}

void
format_text(char *to, char *from, int len)
{
	memmove(to, from, MIN(strlen(from)+1, len-1));
	to[len-1] = '\0';
}

int get_user_by_id(uid_t uid, struct user_info *user_info)
{
	if(secenv == 0)
		secenv = ipc_find_env(ENV_TYPE_SECURITY);

	alloc_send();
	sec_send->uid_req.uid = uid;
	send_req(UID2INFO);
	
	int r = wait_resp();
	if(r < 0)
		return r;

	memmove(user_info, sec_resp, sizeof(user_info)); 
	sys_page_unmap(0, sec_resp);

	return 0;
}

int get_user_by_name(char *name, struct user_info *user_info)
{
	if(secenv == 0)
		secenv = ipc_find_env(ENV_TYPE_SECURITY);

	alloc_send();
	format_text(sec_send->name_req.name, name, NAME_LEN);
	send_req(NAME2INFO);
	
	int r = wait_resp();
	if(r < 0)
		return r;

	memmove(user_info, sec_resp, sizeof(user_info));
	sys_page_unmap(0, sec_resp);

	return 0;
}
int verify_password(uid_t uid, char *pass)
{
	if(secenv == 0)
		secenv = ipc_find_env(ENV_TYPE_SECURITY);

	alloc_send();
	sec_send->verify_req.uid = uid;
	format_text(sec_send->verify_req.pass, pass, PASS_LEN);
	send_req(VERIFYPASS);

	int r = wait_resp();
	return r;
}
