
#include "fs.h"


bool va_is_accessed(void *va);
int va_get_pte(void *va);


// LAB 5 CHALLENGE
// Maximum number of blocks in the cache before 
// we begin evicting them
#define BLOCK_CACHE_MAX 16

// Structure for an entry in the block cache.
struct BlockCacheEntry
{
	// Block number, -1 if unallocated
	int blockno;
	struct BlockCacheEntry *next;
	struct BlockCacheEntry *prev;
};

// Keep a static number of block cache entries for each place
// in the cache
struct BlockCacheEntry block_entries[BLOCK_CACHE_MAX];

// Mark all cache entries free if any only if they have not
// been initialized
void
init_block_entries()
{

	int i;
	for (i = 0; i < BLOCK_CACHE_MAX; i++)
	{
		struct BlockCacheEntry *ent = &block_entries[i];
		ent->blockno = -1;
	}

}

struct BlockCacheEntry*
alloc_block_entry(int blockno)
{
	assert(blockno >= 0);

	int i;
	for (i = 0; i < BLOCK_CACHE_MAX; i++)
	{
		struct BlockCacheEntry *ent = &block_entries[i];
		if(ent->blockno == -1)
		{
			ent->blockno = blockno;
			return ent;
		}
	}

	panic("Ran out of entries for the block cache\nThis is a kernel bug\n");
	return NULL;
}

void
free_block_entry(struct BlockCacheEntry *ent)
{
	ent->blockno = -1;
}


// Block cache structure.
// The cache is a doubly-linked list in approximate LRU ordering
struct BlockCacheList
{
	struct BlockCacheEntry *head;
	struct BlockCacheEntry *tail;
	int size;
} cache = {NULL, NULL, 0};


// Insert a new block number into the block cache
// It is a fatal error to call this function while the cache is full
// You must first check and evict a block if necessary 
// before calling this function
void
block_cache_insert(struct BlockCacheEntry *ent)
{

	assert(cache.size != BLOCK_CACHE_MAX);

	// Do not manage the superblock
	// This causes a lot of problems.
	assert(ent->blockno != 1);

	//cprintf("Inserting block %d into cache\n", ent->blockno);

  if (!cache.head)
	{
		// First entry in cache
		cache.head = cache.tail = ent;
		ent->next = ent->prev = NULL;
	}
	else
	{
		// Append to list in constant time
		
		// tail -> NULL, tail <- ent -> NULL
		ent->prev = cache.tail;
		ent->next = NULL;

		// tail -><- ent -> NULL
		cache.tail->next = ent;

		// This is the new tail
		cache.tail = ent;
	}

	cache.size++;
}

// Unlink the given entry from the list, decrementing the count
void 
block_cache_remove(struct BlockCacheEntry *ent)
{

	// Relink prev and next to each other
	if (ent->prev)
		ent->prev->next = ent->next;
	if (ent->next)
		ent->next->prev = ent->prev;

	// Correct the head and tail if ent was one of those
	if (cache.head == ent)
		cache.head = ent->next;
	if (cache.tail == ent)
		cache.tail = ent->prev;

	cache.size--;
}

struct BlockCacheEntry * 
block_cache_find(int blockno)
{
	struct BlockCacheEntry *ret;
	for (ret = cache.head; ret != NULL; ret++)
	{
		if (ret->blockno == blockno)
			return ret;
	}

	return NULL;
}

// Rotate the cache
void 
block_cache_rotate()
{

	struct BlockCacheEntry *super = NULL;

	struct BlockCacheEntry *ent = cache.head;
	while (ent != NULL)
	{

		// Skip the superblock, since that is accessed by diskaddr
		if (ent->blockno == 1)
		{
			super = ent;
			ent = ent->next;
			continue;
		}

		// If the block was accessed since last rotate
		// move it to the back of the list and reset the accessed bit.
		// Then start the rotation over from the new head
		void *addr = diskaddr(ent->blockno);
		if(va_is_accessed(addr))
		{
			//cprintf("Rotating block %d to end\n", ent->blockno);
			block_cache_remove(ent);
			block_cache_insert(ent);
			int r = sys_page_map(sys_getenvid(), ROUNDDOWN(addr, PGSIZE),
								   sys_getenvid(), ROUNDDOWN(addr, PGSIZE),
									 va_get_pte(addr) & PTE_SYSCALL & ~PTE_A);

			ent = cache.head;
		}
		// Block was not accessed, continue
		else
		{
			ent = ent->next;
		}
	}

	// Always rotate the superblock to the end so it is not evicted
	if(super)
	{
		block_cache_remove(super);
		block_cache_insert(super);
	}
}

// Evict the first block in the cache
void 
block_cache_evict()
{

	assert(cache.head);

	struct BlockCacheEntry *ent = cache.head;
	//cprintf("Evicting block %d\n", ent->blockno);

	// Remove the block from the cache
	block_cache_remove(ent);

	// Flush the block if it is dirty
	if(va_is_dirty(diskaddr(ent->blockno)))
	{
		flush_block(diskaddr(ent->blockno));
	}

	// Unmap the page
	sys_page_unmap(sys_getenvid(), diskaddr(ent->blockno));

	// Free the block entry
	free_block_entry(ent);

}



// Return the virtual address of this disk block.
void*
diskaddr(uint32_t blockno)
{
	if (blockno == 0 || (super && blockno >= super->s_nblocks))
		panic("bad block number %08x in diskaddr", blockno);
	return (char*) (DISKMAP + blockno * BLKSIZE);
}

// Is this virtual address mapped?
bool
va_is_mapped(void *va)
{
	return (vpd[PDX(va)] & PTE_P) && (vpt[PGNUM(va)] & PTE_P);
}

// Is this virtual address accessed?
bool
va_is_accessed(void *va)
{
	return (vpt[PGNUM(va)] & PTE_A) != 0;
}

// Is this virtual address dirty?
bool
va_is_dirty(void *va)
{
	return (vpt[PGNUM(va)] & PTE_D) != 0;
}

// Get the PTE for a virtual address
int 
va_get_pte(void *va)
{
	return vpt[PGNUM(va)];
}

// Fault any disk block that is read or written in to memory by
// loading it from disk.
// Hint: Use ide_read and BLKSECTS.
static void
bc_pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t blockno = ((uint32_t)addr - DISKMAP) / BLKSIZE;
	int r;

	// Rotate the blocks in the cache and evict if necessary
	// LAB 5 CHALLENGE
	block_cache_rotate();
	if(cache.size == BLOCK_CACHE_MAX)
	{
		block_cache_evict();
	}

	// Check that the fault was within the block cache region
	if (addr < (void*)DISKMAP || addr >= (void*)(DISKMAP + DISKSIZE))
		panic("page fault in FS: eip %08x, va %08x, err %04x",
		      utf->utf_eip, addr, utf->utf_err);

	// Sanity check the block number.
	if (super && blockno >= super->s_nblocks)
		panic("reading non-existent block %08x\n", blockno);

	// Allocate a page in the disk map region, read the contents
	// of the block from the disk into that page, and mark the
	// page not-dirty (since reading the data from disk will mark
	// the page dirty).
	//
	// LAB 5: Your code here
	
	// Alloc and insert the page for the block address
	sys_page_alloc(sys_getenvid(), ROUNDDOWN(addr, PGSIZE), PTE_P | PTE_W | PTE_U);

	// Insert this block number into the cache
	// LAB 5 CHALLENGE
	if(blockno != 1)
		block_cache_insert(alloc_block_entry(blockno));

	// Read the data into the page we allocated
	ide_read(blockno * BLKSECTS, ROUNDDOWN(addr, PGSIZE), BLKSECTS);


	// Check that the block we read was allocated. (exercise for
	// the reader: why do we do this *after* reading the block
	// in?)
	if (bitmap && block_is_free(blockno))
		panic("reading free block %08x\n", blockno);
}

// Flush the contents of the block containing VA out to disk if
// necessary, then clear the PTE_D bit using sys_page_map.
// If the block is not in the block cache or is not dirty, does
// nothing.
// Hint: Use va_is_mapped, va_is_dirty, and ide_write.
// Hint: Use the PTE_SYSCALL constant when calling sys_page_map.
// Hint: Don't forget to round addr down.
void
flush_block(void *addr)
{
	uint32_t blockno = ((uint32_t)addr - DISKMAP) / BLKSIZE;

	if (addr < (void*)DISKMAP || addr >= (void*)(DISKMAP + DISKSIZE))
		panic("flush_block of bad va %08x", addr);

	// LAB 5: Your code here.
	if (va_is_mapped(addr) && va_is_dirty(addr))
	{
		// Write the block to disk
		ide_write(blockno * BLKSECTS, ROUNDDOWN(addr, PGSIZE), BLKSECTS);
		// Reset dirty bit for page
		sys_page_map(sys_getenvid(), ROUNDDOWN(addr, PGSIZE),
							   sys_getenvid(), ROUNDDOWN(addr, PGSIZE),
								 va_get_pte(addr) & PTE_SYSCALL & ~PTE_D);
	}
}

// Test that the block cache works, by smashing the superblock and
// reading it back.
static void
check_bc(void)
{
	struct Super backup;

	// back up super block
	memmove(&backup, diskaddr(1), sizeof backup);

	// smash it
	strcpy(diskaddr(1), "OOPS!\n");
	flush_block(diskaddr(1));
	assert(va_is_mapped(diskaddr(1)));
	assert(!va_is_dirty(diskaddr(1)));

	// clear it out
	sys_page_unmap(0, diskaddr(1));
	assert(!va_is_mapped(diskaddr(1)));

	// read it back in
	assert(strcmp(diskaddr(1), "OOPS!\n") == 0);

	// fix it
	memmove(diskaddr(1), &backup, sizeof backup);
	flush_block(diskaddr(1));

	cprintf("block cache is good\n");
}

void
bc_init(void)
{
	set_pgfault_handler(bc_pgfault);
	init_block_entries();
	check_bc();
}

