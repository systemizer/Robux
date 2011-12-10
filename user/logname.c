#include <inc/lib.h>



void
umain(int argc, char *argv[])
{
	binaryname = "logname";

	uid_t uid = getuid();
	struct user_info info;
	
	int r = get_user_by_id(uid, &info);
	if(r < 0)
		printf("Failed to get user for id %d: %e\n", uid, r);
	else
	{
		printf("%s\n", info.ui_name);
	}

	exit();
}
