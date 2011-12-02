// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).
	

	// LAB 4: Your code here.
	pde_t pde = vpt[(uint32_t)addr>>12];
	if(!((pde & PTE_COW) &&
		   (err & FEC_WR)))
	{
		panic("Unrecoverable page fault at 0x%08x\n", addr);
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.
	
	
	// LAB 4: Your code here.
	envid_t myid = sys_getenvid();
	sys_page_alloc(myid, PFTEMP, PTE_P | PTE_W | PTE_U);
	memmove((void*)PFTEMP, (void*)ROUNDDOWN(addr, PGSIZE), PGSIZE);
	sys_page_map(myid, (void*)PFTEMP, myid, (void*)ROUNDDOWN(addr, PGSIZE), PTE_P | PTE_W | PTE_U);

}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	envid_t myid = sys_getenvid();
	int perm = vpt[pn] & PTE_SYSCALL;
	if(perm & PTE_COW || perm & PTE_W)
	{
		perm |= PTE_COW;
		perm &= ~PTE_W;
	}

	int ret = sys_page_map(myid, (void*)(pn*PGSIZE), envid, (void*)(pn*PGSIZE), perm);
	if(ret < 0)
		return ret;
	ret = sys_page_map(myid, (void*)(pn*PGSIZE), myid, (void*)(pn*PGSIZE), perm);
	if(ret < 0)
		return ret;

	return 0;
}


//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	int ret;
	envid_t myid = sys_getenvid();
	set_pgfault_handler(pgfault);
	
	envid_t newid = sys_exofork();
	if(newid < 0)
		panic("Failed to exofork in fork\n");
	if(newid != 0)
	{
		// In parent, set up child
		int i;
		for(i=0; i < UTOP >> 12; i++)
		{
			
			if(i << 12 == UXSTACKTOP - PGSIZE)
				continue;
			if(!vpd[i>>10] & PTE_P)
				continue;


			// Get the pte permissions combined with pde
			pte_t pte = vpt[i];
			pte &= PTE_SYSCALL & (vpd[i>>10] | PTE_COW); // Fixed: OR in COW
			if(pte & PTE_P && pte & (PTE_W | PTE_COW) && (pte & PTE_SHARE == 0))
			{
				ret = duppage(newid, i);
				if(ret < 0)
					panic("Failed to duppage in parent fork: %e at i = %d\n", ret, i);
			}
			else if(pte & PTE_P && pte & PTE_SHARE)
			{
				ret = sys_page_map(myid, (void*)(i*PGSIZE), newid, (void*)(i*PGSIZE), pte & PTE_SYSCALL);
				if(ret < 0)
					panic("Failed to copy [W] page mapping to child: %e\n", ret);
			}
			else if(pte & PTE_P)
			{
				ret = sys_page_map(myid, (void*)(i*PGSIZE), newid, (void*)(i*PGSIZE), pte & PTE_SYSCALL);
				if(ret < 0)
					panic("Failed to copy page mapping to child: %e\n", ret);
			}
		}

		ret = sys_env_set_pgfault_upcall(newid, thisenv->env_pgfault_upcall);
		if(ret < 0)
			panic("Failed to set upcall in parent fork\n");

		sys_page_alloc(newid, (void*)(UXSTACKTOP-PGSIZE), PTE_P | PTE_U | PTE_W);

		sys_env_set_status(newid, ENV_RUNNABLE);

		return newid;
	}
	else
	{
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
