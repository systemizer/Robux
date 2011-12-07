#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	struct user_info info;

	int r = get_user_by_id(1000, &info);

	if(r != 0)
		panic("Failed to get user info\n");


	cprintf("Test: %s (%d) info: %s, shell: %s, home: %s\n", info.ui_name, info.ui_uid, info.ui_comment, info.ui_shell, info.ui_home);

	r = get_user_by_name("rmcqueen", &info);
	if(r != 0)
		panic("Failed to get 2\n");
	cprintf("Test2: %d\n", info.ui_uid);

	r = verify_password(1000, "hello");
	cprintf("Pass: %d\n", r);
}

