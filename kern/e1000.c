#include <kern/e1000.h>
#include <inc/types.h>
#include <kern/pmap.h>
#include <inc/error.h>
#include <inc/string.h>


// LAB 6: Your driver code here

// Location of DMA region
// Initialized in kern/pci.c
volatile uint32_t *e1000;

volatile struct tx_desc tx_desc_arr[E1000_NUM_DESC] __attribute__ ((aligned(4096)));

volatile struct rx_desc rx_desc_arr[E1000_NUM_DESC] __attribute__ ((aligned(4096)));

struct Page *rbufs[E1000_NUM_DESC];


void
e1000_init_tx_desc_arr(void)
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

void
e1000_init_rx_desc_arr(void)
{
	// Zero out array
	memset((void*)rx_desc_arr, 0, sizeof(struct rx_desc)*E1000_NUM_DESC);

	// Allocate physical pages for each descriptor, and set the corresponding
	// descriptor to use that physical page
	int i;
	for(i=0; i<E1000_NUM_DESC; i++)
	{
		rbufs[i] = page_alloc(ALLOC_ZERO);
		rbufs[i]->pp_ref++;

		rx_desc_arr[i].addr = page2pa(rbufs[i]);
	}
}



/*
 * Send a packet on the E1000
 *
 * Addr must be the virtual address of a buffer
 * containing length bytes to be send.
 *
 * [addr, addr+length) must not cross a page boundary
 * Length must be <= PGSIZE
 *
 * Returns 0 on success
 * -E_INVAL if page boundary conditions are not respected
 */
int 
e1000_send_packet(void *addr, uint16_t length)
{
	volatile struct tx_desc *tail = &tx_desc_arr[e1000[E1000_INDEX(E1000_TDT)]];

	if((uint32_t)addr / PGSIZE != ((uint32_t)addr+length)/PGSIZE)
		return -E_INVAL;

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


/*
 * Receive a packet on the E1000
 *
 * Addr must be the virtual address of a buffer that can 
 * contain at least PGSIZE bytes
 *
 * Length is a pointer to a short that will hold the length
 * of the received packet
 *
 * The packet will be copied into this buffer.
 *
 * Returns 0 on success
 * -E_NET_NOT_RECV if no packets are waiting
 */
int 
e1000_recv_packet(void *addr, uint16_t *length)
{
	uint16_t index = (e1000[E1000_INDEX(E1000_RDT)]+1) % E1000_NUM_DESC;
	volatile struct rx_desc *next_desc = &rx_desc_arr[index];

	if(!(next_desc->status & RXD_STATUS_DD))
		return -E_NET_NOT_RECV;

	if(!(next_desc->status & RXD_STATUS_EOP))
		panic("Received a jumbo frame (we cannot handle jumbo frames)\n");


	*length = next_desc->length;
	memmove(addr, page2kva(rbufs[index]), next_desc->length);

	cprintf("Recv packet of len %d\n", *length);
	next_desc->status = 0;

	e1000[E1000_INDEX(E1000_RDT)] = index;

	return 0;
}
