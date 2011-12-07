// Fork a binary tree of processes and display their structure.

#include <inc/lib.h>

#define DEPTH 3

void forktree(const char *cur);

void
forkchild(const char *cur, char branch)
{
	char nxt[DEPTH+1];

	if (strlen(cur) >= DEPTH)
		return;

	snprintf(nxt, DEPTH+1, "%s%c", cur, branch);
	int r;
	if ((r = fork()) == 0) {
		forktree(nxt);
		exit();
	}
	wait(r);
}

void
forktree(const char *cur)
{
	printf("%04x: I am '%s'\n", sys_getenvid(), cur);

	forkchild(cur, '0');
	forkchild(cur, '1');
}

void
umain(int argc, char **argv)
{
	forktree("");
}

