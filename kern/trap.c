#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>

#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/env.h>
#include <kern/syscall.h>

static struct Taskstate ts;

/* For debugging, so print_trapframe can distinguish between printing
 * a saved trapframe and printing the current trapframe and print some
 * additional information in the latter case.
 */
static struct Trapframe *last_tf;

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256] = { { 0 } };
struct Pseudodesc idt_pd = {
	sizeof(idt) - 1, (uint32_t) idt
};


static const char *trapname(int trapno)
{
	static const char * const excnames[] = {
		"Divide error",
		"Debug",
		"Non-Maskable Interrupt",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack Fault",
		"General Protection",
		"Page Fault",
		"(unknown trap)",
		"x87 FPU Floating-Point Error",
		"Alignment Check",
		"Machine-Check",
		"SIMD Floating-Point Exception"
	};

	if (trapno < sizeof(excnames)/sizeof(excnames[0]))
		return excnames[trapno];
	if (trapno == T_SYSCALL)
		return "System call";
	return "(unknown trap)";
}


void
trap_init(void)
{
	extern struct Segdesc gdt[];

	// LAB 3: Your code here.
  SETGATE(idt[0], 0, GD_KT, trap0, 0)
  SETGATE(idt[1], 0, GD_KT, trap1, 0)
  SETGATE(idt[2], 0, GD_KT, trap2, 0)
  SETGATE(idt[3], 1, GD_KT, trap3, 3)
  SETGATE(idt[4], 1, GD_KT, trap4, 0)
  SETGATE(idt[5], 0, GD_KT, trap5, 0)
  SETGATE(idt[6], 0, GD_KT, trap6, 0)
  SETGATE(idt[7], 0, GD_KT, trap7, 0)
  SETGATE(idt[8], 0, GD_KT, trap8, 0)
  SETGATE(idt[9], 0, GD_KT, trap9, 0)
  SETGATE(idt[10], 0, GD_KT, trap10, 0)
  SETGATE(idt[11], 0, GD_KT, trap11, 0)
  SETGATE(idt[12], 0, GD_KT, trap12, 0)
  SETGATE(idt[13], 0, GD_KT, trap13, 0)
  SETGATE(idt[14], 0, GD_KT, trap14, 0)
  SETGATE(idt[15], 0, GD_KT, trap15, 0)
  SETGATE(idt[16], 0, GD_KT, trap16, 0)
  SETGATE(idt[17], 1, GD_KT, trap17, 0)
  SETGATE(idt[18], 1, GD_KT, trap18, 0)
  SETGATE(idt[19], 1, GD_KT, trap19, 0)
  SETGATE(idt[20], 1, GD_KT, trap20, 0)
  SETGATE(idt[21], 1, GD_KT, trap21, 0)
  SETGATE(idt[22], 1, GD_KT, trap22, 0)
  SETGATE(idt[23], 1, GD_KT, trap23, 0)
  SETGATE(idt[24], 1, GD_KT, trap24, 0)
  SETGATE(idt[25], 1, GD_KT, trap25, 0)
  SETGATE(idt[26], 1, GD_KT, trap26, 0)
  SETGATE(idt[27], 1, GD_KT, trap27, 0)
  SETGATE(idt[28], 1, GD_KT, trap28, 0)
  SETGATE(idt[29], 1, GD_KT, trap29, 0)
  SETGATE(idt[30], 1, GD_KT, trap30, 0)
  SETGATE(idt[31], 1, GD_KT, trap31, 0)
  
	// Set up syscall gate
	SETGATE(idt[T_SYSCALL], 1, GD_KT, trap_sysc, 3)

	// Set up syscall MSR if the processor supports sysenter
	if (cpu_get_features() & CPUID_FLAG_SEP)
	{
		wrmsr(SYSENTER_CS, GD_KT, 0);
		wrmsr(SYSENTER_ESP, KSTACKTOP, 0);
		wrmsr(SYSENTER_EIP, sysenter_handler, 0);
	}

	
	// Per-CPU setup 
	trap_init_percpu();
}

// Initialize and load the per-CPU TSS and IDT
void
trap_init_percpu(void)
{
	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.
	ts.ts_esp0 = KSTACKTOP;
	ts.ts_ss0 = GD_KD;

	// Initialize the TSS field of the gdt.
	gdt[GD_TSS0 >> 3] = SEG16(STS_T32A, (uint32_t) (&ts),
					sizeof(struct Taskstate), 0);
	gdt[GD_TSS0 >> 3].sd_s = 0;

	// Load the TSS
	ltr(GD_TSS0);

	// Load the IDT
	lidt(&idt_pd);
}

void
print_trapframe(struct Trapframe *tf)
{
	cprintf("TRAP frame at %p\n", tf);
	print_regs(&tf->tf_regs);
	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);
	cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
	// If this trap was a page fault that just happened
	// (so %cr2 is meaningful), print the faulting linear address.
	if (tf == last_tf && tf->tf_trapno == T_PGFLT)
		cprintf("  cr2  0x%08x\n", rcr2());
	cprintf("  err  0x%08x", tf->tf_err);
	// For page faults, print decoded fault error code:
	// U/K=fault occurred in user/kernel mode
	// W/R=a write/read caused the fault
	// PR=a protection violation caused the fault (NP=page not present).
	if (tf->tf_trapno == T_PGFLT)
		cprintf(" [%s, %s, %s]\n",
			tf->tf_err & 4 ? "user" : "kernel",
			tf->tf_err & 2 ? "write" : "read",
			tf->tf_err & 1 ? "protection" : "not-present");
	else
		cprintf("\n");
	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  flag 0x%08x\n", tf->tf_eflags);
	cprintf("  esp  0x%08x\n", tf->tf_esp);
	cprintf("  ss   0x----%04x\n", tf->tf_ss);
}

void
print_regs(struct PushRegs *regs)
{
	cprintf("  edi  0x%08x\n", regs->reg_edi);
	cprintf("  esi  0x%08x\n", regs->reg_esi);
	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
	cprintf("  oesp 0x%08x\n", regs->reg_oesp);
	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
	cprintf("  edx  0x%08x\n", regs->reg_edx);
	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
	cprintf("  eax  0x%08x\n", regs->reg_eax);
}

static void
trap_dispatch(struct Trapframe *tf)
{
	// Handle processor exceptions.
	// LAB 3: Your code here.
	
	if (tf->tf_trapno == T_PGFLT)
	{
		page_fault_handler(tf);
		return;
	}
	else if (tf->tf_trapno == T_BRKPT)
	{
		monitor(tf);
		return;
	}
	else if (tf->tf_trapno == T_SYSCALL)
	{
		tf->tf_regs.reg_eax = syscall(tf->tf_regs.reg_eax, 
				    											tf->tf_regs.reg_edx, 
																	tf->tf_regs.reg_ecx, 
																	tf->tf_regs.reg_ebx, 
																	tf->tf_regs.reg_edi, 
																	tf->tf_regs.reg_esi);
		return;
	}

	// Unexpected trap: The user process or the kernel has a bug.
	print_trapframe(tf);
	if (tf->tf_cs == GD_KT)
		panic("unhandled trap in kernel");
	else {
		env_destroy(curenv);
		return;
	}
}

void
trap(struct Trapframe *tf)
{
	// The environment may have set DF and some versions
	// of GCC rely on DF being clear
	asm volatile("cld" ::: "cc");

	// Check that interrupts are disabled.  If this assertion
	// fails, DO NOT be tempted to fix it by inserting a "cli" in
	// the interrupt path.
	assert(!(read_eflags() & FL_IF));

	cprintf("Incoming TRAP frame at %p\n", tf);

	if ((tf->tf_cs & 3) == 3) {
		// Trapped from user mode.
		// Copy trap frame (which is currently on the stack)
		// into 'curenv->env_tf', so that running the environment
		// will restart at the trap point.
		assert(curenv);
		curenv->env_tf = *tf;
		// The trapframe on the stack should be ignored from here on.
		tf = &curenv->env_tf;
	}

	// Record that tf is the last real trapframe so
	// print_trapframe can print some additional information.
	last_tf = tf;

	// Dispatch based on what type of trap occurred
	trap_dispatch(tf);

	// Return to the current environment, which should be running.
	assert(curenv && curenv->env_status == ENV_RUNNING);
	env_run(curenv);
}


void
page_fault_handler(struct Trapframe *tf)
{
	uint32_t fault_va;

	// Read processor's CR2 register to find the faulting address
	fault_va = rcr2();

	// Handle kernel-mode page faults.

	// LAB 3: Your code here.
	// Fault if lower 3 bits (CPL) are 0
	if(!(tf->tf_cs & 3))
	{
		panic("kernel fault va %08x ip %08x\n", fault_va, tf->tf_eip);
	}

	// We've already handled kernel-mode exceptions, so if we get here,
	// the page fault happened in user mode.

	// Destroy the environment that caused the fault.
	cprintf("[%08x] user fault va %08x ip %08x\n",
		curenv->env_id, fault_va, tf->tf_eip);
	print_trapframe(tf);
	env_destroy(curenv);
}

