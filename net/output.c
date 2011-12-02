#include "ns.h"

extern union Nsipc nsipcbuf;

char data[PGSIZE] __attribute__ ((aligned(PGSIZE)));

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	int r;
	envid_t from_id;
	while((r = ipc_recv(&from_id, data, NULL)) >= 0)
	{
		if(r != NSREQ_OUTPUT || from_id != ns_envid)
			continue;

	  struct jif_pkt *packet = (struct jif_pkt*) data;
		if((r = sys_net_send_packet(&packet->jp_data, packet->jp_len)) < 0)
			cprintf("Error sending packet: %e\n", r);
	}

	cprintf("net output: ipc_recv failed: %e\n", r);
}
