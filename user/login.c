#include <inc/lib.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/error.h>
#include <inc/env.h>
#include <inc/security.h>

void login(void);
void login_init(void);

char *username;
char *password;


void
login_init(void)
{
	username = (char*)malloc(NAME_LEN);
	password = (char*)malloc(PASS_LEN);
	if ((!username) || (!password))
		panic("login initialization failed");
}
			       
void
login(void)
{
	int r,uid;
	struct user_info info;
	char * buf;

	cprintf("initializing login environment\n");
	login_init();
		       
	// Wait for the console to calm down
	int i;
	for (i = 0; i < 100; i++)
		sys_yield();

	while (1)
	{
		buf = readline_full("\nusername:",1);
		strncpy(username,buf,NAME_LEN);
		buf = readline("\npassword:");
		strncpy(password,buf,PASS_LEN);
		uid = get_user_by_name(username, &info);

		if(uid != 0) {
			cprintf("No user exists: %s\n",username);
			continue;
		}
		r = verify_password(uid,password);
		cprintf("Pass: %d\n",r);
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
