// Test preemption by forking off a child process that just spins forever.
// Let it run for a couple time slices, then kill it.

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	envid_t env;

	printf("I am the parent.  Forking the child...\n");
	if ((env = fork()) == 0) {
		printf("I am the child.  Spinning...\n");
		while (1)
			/* do nothing */;
	}

	printf("I am the parent.  Running the child...\n");
	sys_yield();
	sys_yield();
	sys_yield();
	sys_yield();
	sys_yield();
	sys_yield();
	sys_yield();
	sys_yield();

	printf("I am the parent.  Killing the child...\n");
	sys_env_destroy(env);
}

