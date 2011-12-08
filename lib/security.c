/*
 * This file implements the libjos side of communication with the
 * security server
 */

#include <inc/env.h>
#include <security/secipc.h>
#include <inc/lib.h>
#include <inc/mmu.h>

// Save envid of security environment here
static envid_t secenv = 0;

// Buffers in fixed position untouched by malloc
static union secipc_buffer *sec_send = (union secipc_buffer *)0x10000000;
static struct user_info *sec_resp = (struct user_info *)0x10001000;


// Allocate a new send buffer
void
alloc_send()
{
	sys_page_alloc(0, sec_send, PTE_W | PTE_U | PTE_P);
}

// Send the request with id = num and offer the sec_send buffer
void
send_req(uint32_t num)
{
	ipc_send(secenv, num, sec_send, PTE_P | PTE_U | PTE_W);
}

// Wait for a response from the security server, optionally 
// expecting a page
int
wait_resp(int page_map)
{
	envid_t from = 0;
	int r = 0;
	while(from != secenv)
	{
		if(page_map)
			r = ipc_recv(&from, sec_resp, NULL);
		else
			r = ipc_recv(&from, NULL, NULL);
		if(r < 0)
			break;
	}

	return r;
}

// Text format helper to prevent unterminated strings
void
format_text(char *to, char *from, int len)
{
	memmove(to, from, MIN(strlen(from)+1, len-1));
	to[len-1] = '\0';
}


// Save the user_info for the given uid in the given buffer.
int get_user_by_id(uid_t uid, struct user_info *user_info)
{
	if(secenv == 0)
		secenv = ipc_find_env(ENV_TYPE_SECURITY);

	alloc_send();
	sec_send->uid_req.uid = uid;
	send_req(UID2INFO);
	
	int r = wait_resp(1);
	if(r < 0)
		return r;

	memmove(user_info, sec_resp, sizeof(struct user_info)); 
	sys_page_unmap(0, sec_resp);

	return 0;
}

// Save the user_info for the user with the given name in the given buffer
int get_user_by_name(char *name, struct user_info *user_info)
{
	if(secenv == 0)
		secenv = ipc_find_env(ENV_TYPE_SECURITY);

	alloc_send();
	format_text(sec_send->name_req.name, name, NAME_LEN);
	send_req(NAME2INFO);
	
	int r = wait_resp(1);
	if(r < 0)
		return r;

	memmove(user_info, sec_resp, sizeof(struct user_info));
	sys_page_unmap(0, sec_resp);

	return 0;
}

// Verify a password for the given uid.
// Returns 0 on success, nonzero on failure.
int verify_password(uid_t uid, char *pass)
{
	if(secenv == 0)
		secenv = ipc_find_env(ENV_TYPE_SECURITY);

	alloc_send();
	sec_send->verify_req.uid = uid;
	format_text(sec_send->verify_req.pass, pass, PASS_LEN);
	send_req(VERIFYPASS);

	int r = wait_resp(0);
	return r;
}
