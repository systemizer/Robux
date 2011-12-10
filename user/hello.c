// hello, world
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	printf("hello, world\n");
	printf("i am environment %08x\n", thisenv->env_id);
}
