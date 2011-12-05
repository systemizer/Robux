#include <inc/lib.h>



void 
do_security()
{
}


void 
umain(int argc, char *argv[])
{
	binaryname="security";

	cprintf("Security server running\n");

	do_security();

	exit();
}
