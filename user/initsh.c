#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int i, r, x, want;

	printf("initsh: running sh\n");

	// being run directly from kernel, so no file descriptors open yet
	close(0);
	if ((r = opencons()) < 0)
		panic("opencons: %e", r);
	if (r != 0)
		panic("first opencons used fd %d", r);
	if ((r = dup(0, 1)) < 0)
		panic("dup: %e", r);
	while (1) {
		printf("init: starting sh\n");
		r = spawnl("/sh", "sh", (char*)0);
		if (r < 0) {
			printf("init: spawn sh: %e\n", r);
			continue;
		}
		wait(r);
	}
}
