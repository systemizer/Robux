#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	struct user_info info;

	int r = get_user_by_id(0, &info);

	if(r != 0)
		panic("Failed to get user info\n");


	cprintf("Test: %x\n", (uint8_t)info.ui_name[0]);

	r = get_user_by_name("helloworld", &info);
	if(r != 0)
		panic("Failed to get 2\n");
	cprintf("Test2: %x\n", (uint8_t)info.ui_name[1]);

	r = verify_password(1000, "hello");
	cprintf("Pass: %d\n", r);
}

