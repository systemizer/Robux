#include <inc/lib.h>


int
atoi_octal(char *str)
{
	int end,i;
	int ret=0;
	int str_len = strlen(str);

	for (end=0;end<str_len;end++)
	{
		char c = str[end];
		switch(c)
		{
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '0':
			continue;
		default:
			i++;
			break;
		}
	}
	for(i=1; i <= end; i++)
	{
		int c = str[end-i] - '0';

		int mult;
		for(mult = 0; mult < (i-1); mult++)
			c *= 8;

		ret += c;
	}
	return ret;
}


void
umain(int argc, char **argv)
{
	binaryname = "chmod";
	if (argc<3) {
		printf("not enough arguments\n");
		exit();
	}

	int fdnum, r,i,perm_len,end;
	char *perm_str = argv[1];
	fsperm_t perm; 


	if (strlen(perm_str)>3) 
	{
		printf("invalid permission argument: %s\n",perm_str);
	} 
	else 
	{
		perm = (fsperm_t)atoi_octal(perm_str);       
		for (i=2; i<argc; i++) {
			fdnum = open(argv[i],O_RDONLY);		
			if (fdnum<0)
				printf("can't open %s: %e\n",argv[i],fdnum);
			else {			       			     
				if ((r=fchmod(fdnum,perm))<0)
					printf("chmod failed: %e\n", r);	     
			}
			close(fdnum);
		}
	}
}

