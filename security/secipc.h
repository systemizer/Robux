/*
 * This file describes types that are used to send messages
 * to the security server using IPC.
 *
 * Only libjos should need this, user programs should use the 
 * interface declared in inc/lib.h
 */


#ifndef JOS_SECIPC_H
#define JOS_SECIPC_H

#include <inc/security.h>

// Types of messages the server supports
enum SECIPC_TYPE
{
	NAME2INFO = 1,
	UID2INFO,
	VERIFYPASS,
};

// Union of buffers for different IPC types
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
