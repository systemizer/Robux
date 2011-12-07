#include <inc/lib.h>


uid_t
getuid()
{
	return sys_get_env_user_id();
}

gid_t
getgid()
{
	return sys_get_env_group_id();
}
