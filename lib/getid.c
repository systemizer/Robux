#include <inc/lib.h>

// Get the uid of the running process
uid_t
getuid()
{
	return sys_get_env_user_id();
}

// Get the gid of the running process
gid_t
getgid()
{
	return sys_get_env_group_id();
}
