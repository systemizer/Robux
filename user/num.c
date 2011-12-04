#include <inc/lib.h>

#define BUF_SIZE 2048

int bol = 1;
int line = 0;

int buf_index = 0;
char buffer[BUF_SIZE];

void my_flush()
{
	int r = 0;
	while(r != buf_index)
		r += write(1, buffer + r, buf_index - r);
	buf_index = 0;
}

void
my_putch(int c, void *empty)
{
	buffer[buf_index++] = c;

	if(buf_index == BUF_SIZE)
		my_flush();

}

void
num(int f, const char *s)
{
	long n;
	int r;
	char c;

	while ((n = read(f, &c, 1)) > 0) {
		if (bol) {
			printfmt(my_putch, NULL, "%5d ", ++line);
		//	printf("%5d ", ++line);
			bol = 0;
		}
	  //if ((r = write(1, &c, 1)) != 1)
		//  panic("write error copying %s: %e", s, r);
 	  my_putch(c, NULL);
		if (c == '\n')
			bol = 1;
	}
	if (n < 0)
		panic("error reading %s: %e", s, n);

	my_flush();
}

void
umain(int argc, char **argv)
{
	int f, i;

	binaryname = "num";
	if (argc == 1)
		num(0, "<stdin>");
	else
		for (i = 1; i < argc; i++) {
			f = open(argv[i], O_RDONLY);
			if (f < 0)
				panic("can't open %s: %e", argv[i], f);
			else {
				num(f, argv[i]);
				close(f);
			}
		}
	exit();
}

