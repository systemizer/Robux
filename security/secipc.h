#ifndef JOS_SECIPC_H
#define JOS_SECIPC_H

#include <inc/security.h>

enum SECIPC_TYPE
{
	NAME2INFO = 1,
	UID2INFO,
	VERIFYPASS,
};

union secipc_buffer
{
	struct
	{
		char name[NAME_LEN];
	} name_req;

	struct
	{
		uid_t uid;
	} uid_req;

	struct
	{
		uid_t uid;
		char pass[PASS_LEN];
	} verify_req;

};

#endif
