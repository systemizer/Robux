#include "ns.h"

extern union Nsipc nsipcbuf;

#define NBUFS 16
char data[NBUFS*PGSIZE] __attribute__((aligned(PGSIZE)));

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";	

	
	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	

	// Dynamically allocate NBUFS+1 pages for buffers.
	// Use the extra page to align the remaining pages on a PGSIZE boundary
	
	int r;
	int buf = 0;

	memset(data, 0, NBUFS*PGSIZE);

	struct jif_pkt *packet = (struct jif_pkt*) data;
retry:
	while((r = sys_net_recv_packet(&packet->jp_data)) >= 0)
	{
		packet->jp_len = r;
		ipc_send(ns_envid, NSREQ_INPUT, packet, PTE_P | PTE_U | PTE_W);

		//cprintf("0x%08x %d\n", packet, buf);
		buf = (buf + 1) % NBUFS;
	  packet = (struct jif_pkt*) &data[PGSIZE*buf];
	}
	if(r == -E_NET_NOT_RECV)
	{
		goto retry;
	}

	cprintf("0x%08x %d %x\n", packet, buf, PGSIZE*buf);
	cprintf("net input: sys_net_recv_packet failed: %e\n", r);
}
