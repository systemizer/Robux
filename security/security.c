#include <inc/lib.h>
#include <inc/security.h>
#include <security/secipc.h>
#include <inc/mmu.h>

// TODO: Implement parsing the passwd file in an iterative style
// (iterate over user_info structs it contains).

struct user_info *reply_buf  = (struct user_info *)0x10000000;
union secipc_buffer *request_buf = (union secipc_buffer *)0x10001000;


void 
read_passwd(char **ret)
{
 	int fd = open("passwd", O_RDONLY);
	if(fd < 0)
		*ret = NULL;
	else
	{
		struct Stat stat;
		fstat(fd, &stat);
		uint32_t size = stat.st_size;

		*ret = malloc(size);

		int red = 0;
		while(red != size)
		{
			red += read(fd, ret+red, size-red);
		}
		close(fd);
	}
}


void 
recv_loop()
{
	envid_t from;
	int req;
	while(1)
	{
		req = ipc_recv(&from, request_buf, NULL);


		switch(req)
		{
			case NAME2INFO:
				if(sys_page_alloc(0, reply_buf, PTE_P | PTE_U | PTE_W) < 0)
					panic("security: failed to alloc reply page\n");
				memset(reply_buf, 0x7f, sizeof(struct user_info));
				ipc_send(from, 0, reply_buf, PTE_P | PTE_U | PTE_W);
				break;
			case UID2INFO:
				if(sys_page_alloc(0, reply_buf, PTE_P | PTE_U | PTE_W) < 0)
					panic("security: failed to alloc reply page\n");
				memset(reply_buf, 0x8f, sizeof(struct user_info));
				ipc_send(from, 0, reply_buf, PTE_P | PTE_U | PTE_W);
				break;
			case VERIFYPASS:
				ipc_send(from, 1337, NULL, 0);
				break;
			default:
				ipc_send(from, -E_INVAL, NULL, 0);
		}
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
