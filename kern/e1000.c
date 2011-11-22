#include <kern/e1000.h>
#include <inc/types.h>
#include <kern/pmap.h>
#include <inc/string.h>


// LAB 6: Your driver code here

// Location of DMA region
// Initialized in kern/pci.c
volatile uint32_t *e1000;

volatile struct tx_desc tx_desc_arr[E1000_NUM_DESC] __attribute__ ((aligned(4096)));


void
e1000_init_tx_desc_arr()
{
	// Zero out array
	memset((void*)tx_desc_arr, 0, sizeof(struct tx_desc)*E1000_NUM_DESC);

	// Set DD on all descriptors so we can just check if it is set in
	// send
	int i;
	for(i=0; i<E1000_NUM_DESC; i++)
	{
		tx_desc_arr[i].status |= TXD_STATUS_DD;
	}
}


int 
e1000_send_packet(void *addr, uint16_t length)
{
	volatile struct tx_desc *tail = &tx_desc_arr[e1000[E1000_INDEX(E1000_TDT)]];

retry:
	if(!(tail->status & TXD_STATUS_DD))
		goto retry;

	memset((void*)tail, 0, sizeof(struct tx_desc));

	tail->addr = PADDR(addr);
	tail->length = length, 42;
	tail->cmd = TXD_CMD_RS | TXD_CMD_EOP;

	e1000[E1000_INDEX(E1000_TDT)] = (e1000[E1000_INDEX(E1000_TDT)] + 1) % E1000_NUM_DESC;

	return 0;
}
