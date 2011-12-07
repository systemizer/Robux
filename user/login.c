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
		buf = readline("\nusername: ");
		strncpy(username,buf,NAME_LEN);
		buf = readline_full("password: ", 1);
		strncpy(password,buf,PASS_LEN);
		printf("\n");
		
		r = get_user_by_name(username, &info);

		if(r != 0) {
			cprintf("No user exists: %s\n", username);
			continue;
		}

		r = verify_password(info.ui_uid, password);

		if(r == 0)
		{
			const char *newarg[2];
			newarg[0] = info.ui_shell;
			newarg[1] = NULL;
			int pid = spawn_full(info.ui_shell, newarg, info.ui_uid, info.ui_gid);
			if(pid != 0)
				wait(pid);
		}
		else
		{
			cprintf("Bad password\n");
		}
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
