#include <inc/lib.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/error.h>
#include <inc/env.h>
#include <inc/security.h>

void login(void);
int login_init(void);
static void prompt_login(void);
static int authenticate(void);

 
char *username;
char *password;

// authenticate takes a username and password and
// passes it to security, which will check if the
// combination is valid. authenticate returns the
// userid if valid, else return -1
static int
authenticate(void)
{
	envid_t from_envid;
	envid_t result;
	cprintf("%s\n",username);
	cprintf("%s\n",password);
	/* envid_t sc_envid = ipc_find_env(ENV_TYPE_SC); */
	/* if (!sc_envid) */
	/* 	panic("The security server could not be found"); */
	/* ipc_send(sc_envid,SCREQ_AUTH,&token,PTE_P|PTE_U); */
	/* result =  ipc_recv(&from_envid,NULL,NULL); */
	/* while (from_envid!=sc_envid) { */
	/* 	result = ipc_recv(&from_envid,NULL,NULL); */
	/* } */
	/* return result & AUTH; */
	return 0;

}

//prompts the user for a username and password.
//users authenticate to log the user into the
//system. On success, spawn a new environment
//with the returned userid
static void
prompt_login(void)
{
	char *buf;
	username = readline("username: ");
	cprintf("username - %s\n",buf);
	strcpy(username,buf);
	buf= readline("password: ");
	cprintf("password - %s",buf);
	strcpy(password,buf);
}

int
login_init(void)
{
	int r;
	/*
	if ((r=sys_page_alloc(0,username,PTE_U|PTE_W|PTE_P))<0)
		return r;
	if ((r=sys_page_alloc(0,password,PTE_U|PTE_W|PTE_P))<0)
		return r;
		*/

	username = (char*)malloc(NAME_LEN);
	password = (char*)malloc(PASS_LEN);

	return 0;
}
			
	

void
login(void)
{
	int r;
	cprintf("initializing environment\n");
	if ((r=login_init())<0)
		panic("Error: failed to initialize login environment\n");

		       
	// Wait for the console to calm down
	int i;
	for (i = 0; i < 100; i++)
		sys_yield();

	char * buf;
	while (1)
	{
		buf = readline("username: ");
		strncpy(username,buf,NAME_LEN);
		buf = readline("password: ");
		strncpy(password,buf,PASS_LEN);
		cprintf("password - %s",password);
		cprintf("username - %s",username);
	}
		
}

void 
umain(int argc, char *argv[])
{

	binaryname="login";


	close(0);
	close(1);
	opencons();
	opencons();

	login();
	exit();
}
