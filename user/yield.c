// yield the processor to other environments

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int i;

	printf("Hello, I am environment %08x.\n", thisenv->env_id);
	for (i = 0; i < 5; i++) {
		sys_yield();
		printf("Back in environment %08x, iteration %d.\n",
			thisenv->env_id, i);
	}
	printf("All done in environment %08x.\n", thisenv->env_id);
}
