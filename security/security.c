/*
 * This file is the security server process.
 *
 * This process is responsible for managing the /passwd file.
 *
 * It is the only way to get information about users and to 
 * verify passwords for users.
 */




#include <inc/lib.h>
#include <inc/security.h>
#include <security/secipc.h>
#include <inc/mmu.h>

// Put the buffers to pass in IPC at a fixed location below UTOP but
// above the range owned by malloc
struct user_info *reply_buf  = (struct user_info *)0x10000000;
union secipc_buffer *request_buf = (union secipc_buffer *)0x10001000;


// Read all of /passwd into memory and return a pointer to it in ret
// Put the size of the file into the int referred to by sizeret
void 
read_passwd(char **ret, uint32_t *sizeret)
{
 	int fd = open("passwd", O_RDONLY);
	if(fd < 0)
		*ret = NULL;
	else
	{
		// Get file size
		struct Stat stat;
		fstat(fd, &stat);
		uint32_t size = stat.st_size;

		// Make room for the file, plus null byte
		*ret = malloc(size+1);

		// Read file in
		int red = 0;
		while(red != size)
		{
			red += read(fd, (*ret)+red, size-red);
		}

		// Set null byte and close
		(*ret)[size] = '\0';
		close(fd);

		*sizeret = size;
	}
}

// Count users in a passwd file by counting newlines
// Returns the number of users in the password file
int
count_users(char *passwd, uint32_t size)
{
	int i;
	int ret = 0;
	for(i = 0; i < size; i++)
	{
		if(passwd[i] == '\n')
			ret += 1;
	}

	return ret;
}

// Load and parse the passwd file.
// Set arrptr to the beginning of an array of pointers to user info
// structures, one for each user correctly read from disk.
void
get_user_arr(struct user_info **arrptr, uint32_t *count_ret)
{
	char *passwd;
	uint32_t size;
	read_passwd(&passwd, &size);
	if(passwd == NULL)
		panic("security: cannot read passwd\n");


	int usernum = count_users(passwd, size);

	*arrptr = (struct user_info *) malloc(sizeof(struct user_info) * usernum);
	
	// Store each user's line
	char **lines = (char**)malloc(usernum * sizeof(char*));
	const char *cur = passwd;
	const char *end = strchr(cur, '\n');
	int i = 0;
	while(end != NULL)
	{
		lines[i] = (char*)malloc(end - cur + 1);
		memmove(lines[i], cur, end-cur);
		lines[i][end-cur] = '\0';

		i++;
		cur = end+1;
		end = strchr(cur, '\n');
	}

	// Parse each user's line for data
	
	struct user_info *ret = *arrptr;
	int retindex = 0;
	for(i = 0; i<usernum; i++)
	{
		cur = lines[i];

		end = strchr(cur, ':');
		// Skip line on errors
		if(end == NULL)
		{
			cprintf("passwd: no name\n");
			continue;
		}
		strncpy(ret[retindex].ui_name, cur, MIN(NAME_LEN, end-cur));

		cur = end+1;
		end = strchr(cur, ':');
		if(end == NULL)
		{
			cprintf("passwd: no pass\n");
			continue;
		}
		strncpy(ret[retindex].ui_pass, cur, MIN(PASS_LEN, end-cur));

		char tmp_buf[20];
		
		cur = end+1;
		end = strchr(cur, ':');
		if(end == NULL)
		{
			cprintf("passwd: no uid\n");
			continue;
		}
		memset(tmp_buf, 0, 20);
		strncpy(tmp_buf, cur, MIN(19, end-cur));
		ret[retindex].ui_uid = atoi(tmp_buf);

		cur = end+1;
		end = strchr(cur, ':');
		if(end == NULL)
		{
			cprintf("passwd: no gid\n");
			continue;
		}
		memset(tmp_buf, 0, 20);
		strncpy(tmp_buf, cur, MIN(19, end-cur));
		ret[retindex].ui_gid = atoi(tmp_buf);

		cur = end+1;
		end = strchr(cur, ':');
		if(end == NULL)
		{
			cprintf("passwd: no comment\n");
			continue;
		}
		strncpy(ret[retindex].ui_comment, cur, MIN(COMMENT_LEN, end-cur));

		cur = end+1;
		end = strchr(cur, ':');
		if(end == NULL)
		{
			cprintf("passwd: no home\n");
			continue;
		}
		strncpy(ret[retindex].ui_home, cur, MIN(NAME_LEN, end-cur));
		
		cur = end+1;
		strncpy(ret[retindex].ui_shell, cur, PATH_LEN);

		retindex++;
	}

	*count_ret = retindex;

	for(i = 0; i<usernum; i++)
	{
		free(lines[i]);
	}

	free(lines);
	free(passwd);
}


// Main server receive loop
void 
recv_loop()
{
	envid_t from;
	int req;
	while(1)
	{
		req = ipc_recv(&from, request_buf, NULL);

		struct user_info *user_arr;
		struct user_info *req_user = NULL;
		uint32_t user_count;
		get_user_arr(&user_arr, &user_count);


		int i;
		switch(req)
		{
			// Scan user array for matching username
			// Break if found and set req_user
			case NAME2INFO:
				for(i = 0; i < user_count; i++)
				{
					if(strncmp(user_arr[i].ui_name, 
								     request_buf->name_req.name,
										 NAME_LEN) == 0)
					{
						req_user = &user_arr[i];
						break;
					}
				}
				break;
			// Scan user array for matching uid
			case UID2INFO:
				for(i = 0; i < user_count; i++)
				{
					if(user_arr[i].ui_uid == request_buf->uid_req.uid)
					{
						req_user = &user_arr[i];
						break;
					}
				}
				break;
			case VERIFYPASS:
				for(i = 0; i < user_count; i++)
				{
					if(user_arr[i].ui_uid == request_buf->verify_req.uid)
					{
						req_user = &user_arr[i];
						break;
					}
				}
				break;
		}

		switch(req)
		{
			case NAME2INFO:

				
				if(req_user == NULL)
				{
					ipc_send(from, -E_USER_NOT_FOUND, NULL, 0);
					break;
				}

				if(sys_page_alloc(0, reply_buf, PTE_P | PTE_U | PTE_W) < 0)
					panic("security: failed to alloc reply page\n");
				
				// Copy user info into send buf, but clear out password field
				memmove(reply_buf, req_user, sizeof(struct user_info));
				memset(reply_buf->ui_pass, 0, PASS_LEN);
				ipc_send(from, 0, reply_buf, PTE_P | PTE_U | PTE_W);

				break;
			case UID2INFO:


				if(req_user == NULL)
				{
					ipc_send(from, -E_USER_NOT_FOUND, NULL, 0);
					break;
				}
				
				if(sys_page_alloc(0, reply_buf, PTE_P | PTE_U | PTE_W) < 0)
					panic("security: failed to alloc reply page\n");

				// Copy user info into send buf, but clear out password field
				memmove(reply_buf, req_user, sizeof(struct user_info));
				memset(reply_buf->ui_pass, 0, PASS_LEN);
				ipc_send(from, 0, reply_buf, PTE_P | PTE_U | PTE_W);
				
				break;
			case VERIFYPASS:

				if(req_user == NULL)
				{
					ipc_send(from, -E_USER_NOT_FOUND, NULL, 0);
					break;
				}

				// Return true for now
				ipc_send(from, 0, NULL, 0);

				break;
			default:
				ipc_send(from, -E_INVAL, NULL, 0);
		}

		free(user_arr);
	}
}


void 
umain(int argc, char *argv[])
{
	binaryname="security";

	cprintf("Security server running\n");

	recv_loop();

	exit();
}
