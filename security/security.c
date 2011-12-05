#include <inc/lib.h>
#include <inc/security.h>
#include <security/secipc.h>



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
