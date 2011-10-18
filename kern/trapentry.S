/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps


.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

TRAPHANDLER_NOEC(trap0,0)
TRAPHANDLER_NOEC(trap1,1)
TRAPHANDLER_NOEC(trap2,2)
TRAPHANDLER_NOEC(trap3,3)
TRAPHANDLER_NOEC(trap4,4)
TRAPHANDLER_NOEC(trap5,5)
TRAPHANDLER_NOEC(trap6,6)
TRAPHANDLER_NOEC(trap7,7)
TRAPHANDLER(trap8,8)
TRAPHANDLER_NOEC(trap9,9)
TRAPHANDLER(trap10,10)
TRAPHANDLER(trap11,11)
TRAPHANDLER(trap12,12)
TRAPHANDLER(trap13,13)
TRAPHANDLER(trap14,14)
TRAPHANDLER(trap15,15)
TRAPHANDLER_NOEC(trap16,16)
TRAPHANDLER_NOEC(trap17,17)
TRAPHANDLER_NOEC(trap18,18)
TRAPHANDLER_NOEC(trap19,19)
TRAPHANDLER_NOEC(trap20,20)
TRAPHANDLER_NOEC(trap21,21)
TRAPHANDLER_NOEC(trap22,22)
TRAPHANDLER_NOEC(trap23,23)
TRAPHANDLER_NOEC(trap24,24)
TRAPHANDLER_NOEC(trap25,25)
TRAPHANDLER_NOEC(trap26,26)
TRAPHANDLER_NOEC(trap27,27)
TRAPHANDLER_NOEC(trap28,28)
TRAPHANDLER_NOEC(trap29,29)
TRAPHANDLER_NOEC(trap30,30)
TRAPHANDLER_NOEC(trap31,31)

TRAPHANDLER_NOEC(trap_sysc,T_SYSCALL)

/*
	Data passed in is:
	eax - syscall number
	edx, ecx, ebx, edi - args 1-4
	esi - return pc
	ebp - return esp

	This code is the kernel side of the sysenter challenge
	Since the two values we need to feed back to sysexit,
	the return pc and return esp, are in callee save registers,
	they do not need to be saved.

	I just push all arguments to syscall and call the
	function.

	When it returns, I load the stack and return pointers
	in the correct locations and call sysexit.
*/
.globl sysenter_handler
sysenter_handler:

pushl $0
pushl %edi
pushl %ebx
pushl %ecx
pushl %edx
pushl %eax
call syscall

/* 
	Get our stack pointer in ecx
	and our return addr in edx
*/
movl %ebp, %ecx
movl %esi, %edx

sysexit


/*
 * Lab 3: Your code here for _alltraps
 */

_alltraps:
	pushl %ds
	pushl %es
	pushal
	movl $GD_KD, %eax
	movw %ax, %ds
	movw %ax, %es
	pushl %esp
	call trap

