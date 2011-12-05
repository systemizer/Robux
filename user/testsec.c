#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	struct user_info info;

	int r = get_user_by_id(0, &info);

	if(r != 0)
		panic("Failed to get user info\n");


	cprintf("Test: %x\n", info.ui_name[0]);
}

