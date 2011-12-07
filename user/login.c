#include <inc/lib.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/error.h>
#include <inc/env.h>
#include <inc/security.h>

void login(int);
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
login(int forever)
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
		if(buf == NULL)
		{
			printf("Terminating login\n");
			exit();
		}
		strncpy(username,buf,NAME_LEN);
		username[NAME_LEN-1] = '\0';

		// Disable remote echo if not console
		if(!iscons(1))
		{
			char d1 = 0xFF;
			char d2 = 0xFD;
			char d3 = 0x2D;
			write(1, &d1, 1); 
			write(1, &d2, 1); 
			write(1, &d3, 1); 
		}

		buf = readline_full("password: ", 1);
		if(buf == NULL)
		{
			printf("Terminating login\n");
			exit();
		}

		// Enable remote echo if not console
		if(!iscons(1))
		{
			char d1 = 0xFF;
			char d2 = 0xFE;
			char d3 = 0x2D;
			write(1, &d1, 1); 
			write(1, &d2, 1); 
			write(1, &d3, 1); 
		}

		strncpy(password,buf,PASS_LEN);
		password[PASS_LEN-1] = '\0';
		printf("\n");
		
		r = get_user_by_name(username, &info);

		if(r != 0) {
			printf("No user exists: %s\n", username);
			continue;
		}

		r = verify_password(info.ui_uid, password);

		if(r == 0)
		{
			const char *newarg[3];
			newarg[0] = info.ui_shell;
			newarg[1] = "-i";
			newarg[2] = NULL;
			int pid = spawn_full(info.ui_shell, newarg, info.ui_uid, info.ui_gid);
			if(pid != 0)
			{
				wait(pid);
				if(!forever)
					exit();
			}
		}
		else
		{
			printf("Bad password\n");
		}
	}
		
}

void 
umain(int argc, char *argv[])
{

	binaryname="login";


	if(!isopen(0) && !isopen(1))
	{
		close(0);
		close(1);
		opencons();
		opencons();
	}

	if(argc == 2 && strcmp(argv[1], "-t") == 0)
		login(0);
	else
		login(1);
	exit();
}
