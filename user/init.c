#include <inc/lib.h>

struct {
	char msg1[5000];
	char msg2[1000];
} data = {
	"this is initialized data",
	"so is this"
};

char bss[6000];

int
sum(const char *s, int n)
{
	int i, tot = 0;
	for (i = 0; i < n; i++)
		tot ^= i * s[i];
	return tot;
}

void
umain(int argc, char **argv)
{
	int i, r, x, want;
	char args[256];

	if(getuid() != 0)
	{
		printf("Only root may run init\n");
		exit();
	}

	printf("init: running\n");

	want = 0xf989e;
	if ((x = sum((char*)&data, sizeof data)) != want)
		printf("init: data is not initialized: got sum %08x wanted %08x\n",
			x, want);
	else
		printf("init: data seems okay\n");
	if ((x = sum(bss, sizeof bss)) != 0)
		printf("bss is not initialized: wanted sum 0 got %08x\n", x);
	else
		printf("init: bss seems okay\n");

	// output in one syscall per line to avoid output interleaving 
	strcat(args, "init: args:");
	for (i = 0; i < argc; i++) {
		strcat(args, " '");
		strcat(args, argv[i]);
		strcat(args, "'");
	}
	printf("%s\n", args);

	printf("init: running sh\n");

	// being run directly from kernel, so no file descriptors open yet
	close(0);
	if ((r = opencons()) < 0)
		panic("opencons: %e", r);
	if (r != 0)
		panic("first opencons used fd %d", r);
	if ((r = dup(0, 1)) < 0)
		panic("dup: %e", r);
	while (1) {
		printf("init: starting login\n");
		r = spawnl("/login", "login", "-t", (char*)0);
		if (r < 0) {
			printf("init: spawn sh: %e\n", r);
			continue;
		}
		wait(r);
	}
}
