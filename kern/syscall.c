/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/spinlock.h>
#include <kern/sched.h>


// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	// LAB 3: Your code here.
	user_mem_assert(curenv, s, len, PTE_U);


	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	env_destroy(e);
	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
//	-E_NO_MEM on memory exhaustion.
static envid_t
sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.

	// LAB 4: Your code here.
	int ret;
	struct Env *newEnv;
	struct Env *thisEnv = curenv;
	if ((ret = env_alloc(&newEnv, curenv->env_id)) < 0)
		return ret;

	// Set status
	newEnv->env_status = ENV_NOT_RUNNABLE;

	// Copy over registers, but set eax to 0
	memmove(&newEnv->env_tf, &curenv->env_tf, sizeof(struct Trapframe));
	newEnv->env_tf.tf_regs.reg_eax = 0;

	return newEnv->env_id;
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
	// Hint: Use the 'envid2env' function from kern/env.c to translate an
	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.
	struct Env *env;
	
	int ret = envid2env(envid, &env, 1);
	if(ret < 0)
		return ret;

	if(status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE)
		return -E_INVAL;

	env->env_status = status;

	return 0;

}

// Set envid's trap frame to 'tf'.
// tf is modified to make sure that user environments always run at code
// protection level 3 (CPL 3) with interrupts enabled.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	// LAB 5: Your code here.
	// Remember to check whether the user has supplied us with a good
	// address!
	panic("sys_env_set_trapframe not implemented");
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	// LAB 4: Your code here.
	struct Env *env;
	int ret = envid2env(envid, &env, 1);
	if(ret < 0)
		return ret;

	env->env_pgfault_upcall = func;

	return 0;
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_SYSCALL in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 4: Your code here.
	if (!( (perm & PTE_P) &&
			  (perm & PTE_U) &&
			 !(perm & ~(PTE_P | PTE_U | PTE_AVAIL | PTE_W))))
	{
		return -E_INVAL;
	}

	struct Env *env;
	int ret = envid2env(envid, &env, 1);
	if(ret < 0)
		return ret;

	if((uint32_t)va >= UTOP || ((uint32_t)va) % PGSIZE != 0)
		return -E_INVAL;

	struct Page *page = page_alloc(1);
	if(!page)
		return -E_NO_MEM;

	ret = page_insert(env->env_pgdir, page, va, perm);
	if(ret < 0)
		return ret;

	return 0;
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
	     envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.
	
	// Lab 4 your code here
	struct Env *srcenv;
	struct Env *dstenv;
	int ret;
	ret = envid2env(srcenvid, &srcenv, 1);
	if(ret < 0)
		return ret;
	ret = envid2env(dstenvid, &dstenv, 1);
	if(ret < 0)
		return ret;

	if((uint32_t)srcva > UTOP || (uint32_t)srcva % PGSIZE != 0)
		return -E_INVAL;
	if((uint32_t)dstva > UTOP || (uint32_t)dstva % PGSIZE != 0)
		return -E_INVAL;

	pte_t *srcpte;
	struct Page *srcpage = page_lookup(srcenv->env_pgdir, srcva, &srcpte);
	if(!srcpage)
		return -E_INVAL;


	if(!( (perm & PTE_P) &&
			  (perm & PTE_U) &&
			 !(perm & ~(PTE_SYSCALL))))
	{
		return -E_INVAL;
	}

	if((perm & PTE_W) && !(*srcpte & PTE_W))
		return -E_INVAL;

	return page_insert(dstenv->env_pgdir, srcpage, dstva, perm);
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().
	
	// LAB 4: Your code here.
	struct Env *env;
	int ret = envid2env(envid, &env, 1);
	if(ret < 0)
		return ret;

	if((uint32_t) va > UTOP || (uint32_t) va % PGSIZE != 0)
		return -E_INVAL;

	page_remove(env->env_pgdir, va);

	return 0;

}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in the
//		current environment's address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	// LAB 4: Your code here.
	struct Env *env;
	int r;
	uint32_t srcint = (uint32_t)srcva;
	struct Page *srcpage;

	// Make sure env exists
	if ((r = envid2env(envid, &env, 0) < 0))
		return r;

	int map_page = 0; // By default, we do not map a page

	// First check that our input makes sense
	if (srcint < UTOP)
	{
		// Check if srcva < UTOP but not page-aligned
		if(srcint % PGSIZE != 0)
		{
			return -E_INVAL;
		}

		// Check that permissions are appropriate
		if (!( (perm & PTE_P) &&
			  	 (perm & PTE_U) &&
			 		!(perm & ~(PTE_P | PTE_U | PTE_AVAIL | PTE_W))))
		{
			return -E_INVAL;
		}

		// Make sure that srcva is mapped in current env
		pte_t *pte;
		if ((srcpage = page_lookup(curenv->env_pgdir, srcva, &pte)) == NULL)
		{
			return -E_INVAL;
		}
		
		// Ensure that source and perm are either both RO or W
		if ((perm & PTE_W) != (*pte & PTE_W))
		{
			return -E_INVAL;
		}

		// Page mapping checks out, we are mapping a page if 
		// they want it
		map_page = 1;

		
	}

	// Make sure env receiving
	if (!env->env_ipc_recving)
	{
		// CHALLENGE: If the env is not receiving, set up
		// send fields in our env and sleep for response.
		// Note: The receiver now has the responsibility to
		// map its own page based off of what we have entered in
		// our struct (the va is guaranteed to be valid if it is < UTOP)
		// It must also set our EAX to the error code for the mapping.
		curenv->env_ipc_send_to = envid;
		curenv->env_ipc_send_value = value;
		curenv->env_ipc_send_srcva = srcva;
		curenv->env_ipc_send_perm = perm;


		// Sleep and yield
		curenv->env_status = ENV_NOT_RUNNABLE;
		sched_yield();
	}

	// The env is receiving, set their fields
	
	// Only map the page if our checks passed AND the destination VA
	// is < UTOP
	if(map_page && (uint32_t) env->env_ipc_dstva < UTOP)
	{
		// Map the page
		r = page_insert(env->env_pgdir, srcpage, env->env_ipc_dstva, perm);
		if(r < 0)
			return r;

		// Update perm
		env->env_ipc_perm = perm;
	}


	// If we made it here, there were no errors, update fields
	env->env_ipc_recving = 0;
	env->env_ipc_from = curenv->env_id;
	env->env_ipc_value = value;
	env->env_status = ENV_RUNNABLE;
	env->env_tf.tf_regs.reg_eax = 0;

	return 0;
}


#define RECV_LIMIT 16

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	// LAB 4: Your code here.
	
		if((uint32_t)dstva < UTOP)
	{
		if((uint32_t)dstva % PGSIZE != 0)
			return -E_INVAL;
		curenv->env_ipc_dstva = dstva;
	}
	else
	{
		curenv->env_ipc_dstva = (void*)0xFFFFFFFF;
	}
	
	
	// LAB 4 CHALLENGE: Check if another env has queued a send to us
	// If they do, receive it and return, short circuiting on the first
	// received
	
	// i keeps track of the last index received from
	static int i = 0;
	static int recvs = RECV_LIMIT;
	// k is the counting index, 
	int k = i;
	// j is the stop index, when j == k we end
	int j;

	// Loop over all envs circularly starting with the last one we received
	// from
	// Every RECV_LIMIT receives from the same env, start one after the
	// last one we received from.
	// This should eliminate livelock when one env is repeatedly queueing
	// messages to this env faster than we receive, but it still provides
	// good performance when we only want to receive from a small number of
	// envs
	int shift_j = 1;
	for(j = k-1; k != j ; k = (k+1)%NENV)
	{
		// We need to shift j up 1 to that envs[i] is checked
		// This solves the problem of finding the end of the ring
		if(shift_j)
		{
			j = (j+1) % NENV;
			shift_j = 0;
		}

		struct Env *env = &envs[k];
		if(env->env_status == ENV_NOT_RUNNABLE &&
			 env->env_ipc_send_to == curenv->env_id)
		{
			// This environment has a send waiting for us!
			curenv->env_ipc_value = env->env_ipc_send_value;
			curenv->env_ipc_from = env->env_id;

			// Map the page iff they sent one AND we want one
			if((uint32_t)env->env_ipc_send_srcva < UTOP &&
					(uint32_t)curenv->env_ipc_dstva < UTOP)
			{
				int r;
				struct Page *page = page_lookup(env->env_pgdir, 
															env->env_ipc_send_srcva, NULL);
				r = page_insert(curenv->env_pgdir, page, curenv->env_ipc_dstva, 
																			env->env_ipc_send_perm);
				if(r < 0)
				{
					// Error mapping, we need to make the send call receive the
					// error and ignore it here as if it never happened
					env->env_ipc_send_to = 0;
					env->env_tf.tf_regs.reg_eax = r;
					env->env_status = ENV_RUNNABLE;
					continue;
				}

				curenv->env_ipc_perm = env->env_ipc_send_perm;
			}
			else
			{
				// If we do not map, clear perm
				curenv->env_ipc_perm = 0;
			}

			// We have received it, check that we do not need to increment i
			if(k == i)
			{
				if (--recvs == 0)
				{
					// If we have received RECV_LIMIT messages in a row from the same
					// env, start checking one past that env next time to prevent
					// livelock
					recvs = RECV_LIMIT;
					k++;
				}
			}
			else
			{
				recvs = RECV_LIMIT;
			}

			i = k;

			// Queued send has been received, ready the other env and return
			env->env_ipc_send_to = 0;
			env->env_tf.tf_regs.reg_eax = 0;
			env->env_status = ENV_RUNNABLE;
			return 0;
		}
	}
	


	curenv->env_ipc_recving = 1;
	curenv->env_status = ENV_NOT_RUNNABLE;
	sched_yield();

	
	return 0;
}


// If a call uses sysenter, it does not go through trap and lock
// the kernel lock.
// Check if we got one of those, and lock the kernel if we did.
// I had to make destroy env and yield use traps to work right with 
// locking
void syscall_cond_lock(uint32_t syscallno, int lock)
{
	if (!(cpu_get_features() & CPUID_FLAG_SEP))
		return;


	// NO LOCKING, DISABLE SYSENTER FOR NOW
	return;

#if 0
	switch(syscallno)
	{
		case SYS_cputs:
		case SYS_cgetc:
		case SYS_getenvid:
			if(lock)
			 lock_kernel();
			else
			 unlock_kernel();
		case SYS_env_destroy:
		case SYS_yield:
		default:
			break;
	}
#endif
}

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.
	
	// We need to lock the kernel for any calls we know use sysenter
	syscall_cond_lock(syscallno, 1);
	
	int32_t ret = 0;
	switch(syscallno)
	{
		case SYS_cputs:
			sys_cputs((char *)a1, a2);
			break;
		case SYS_cgetc:
			ret = sys_cgetc();
			break;
		case SYS_env_destroy:
			ret = sys_env_destroy(a1);
			break;
		case SYS_env_set_pgfault_upcall:
			ret = sys_env_set_pgfault_upcall((envid_t)a1, (void*)a2);
			break;
		case SYS_getenvid:
			ret = sys_getenvid();
			break;
		case SYS_yield:
			sys_yield();
			break;
		case SYS_exofork:
			ret = sys_exofork();
			break;
		case SYS_env_set_status:
			ret = sys_env_set_status((envid_t)a1, a2);
			break;
		case SYS_page_alloc:
			ret = sys_page_alloc((envid_t)a1, (void*)a2, a3);
			break;
		case SYS_page_map:
			ret = sys_page_map((envid_t)a1, (void*)a2, (envid_t)a3, (void*)a4, a5);
			break;
		case SYS_page_unmap:
			ret = sys_page_unmap((envid_t)a1, (void*)a2);
			break;
		case SYS_ipc_try_send:
			ret = sys_ipc_try_send((envid_t)a1, (uint32_t)a2, 
														 (void*)a3, (unsigned) a4);
			break;
		case SYS_ipc_recv:
			ret = sys_ipc_recv((void*)a1);
			break;
		default:
			ret = -E_INVAL;
			break;
	}

	syscall_cond_lock(syscallno, 0);
	return ret;
}

