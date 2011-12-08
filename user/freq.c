#include <inc/lib.h>


void
umain(int argc, char **argv)
{

	uint32_t data[256];
	memset(data, 0, sizeof(uint32_t)*256);
	
	int r;
	char buf[1];
	while((r = read(0, &buf, 1)) == 1)
	{
		data[(uint32_t)buf[0]] += 1;
	}


	int i;

	int max = 0;
	for(i = 0; i < 256; i++)
	{
		max = MAX(max, data[i]);
	}

	const int max_width = 50;
	int perblock = 1;
	if(max / perblock > max_width)
	{
		perblock = 5;
		while(max / perblock > max_width)
			perblock += 5;
	}

	printf("Frequence diagram (# = %d):\n", perblock);

	for(i = 0; i < 256; i++)
	{
		if(data[i] == 0)
			continue;

		if(i >= ' ' && i <= '~')
			printf(" '%c' = %8d : ", (char) i, data[i]);
		else
			printf("0x%02x = %8d : ", (char) i, data[i]);

		int j;
		for(j = 0; j * perblock < data[i]; j++)
			printf("#");

		printf("\n");
	}

	exit();
	
}
