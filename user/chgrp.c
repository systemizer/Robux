#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	binaryname = "chgrp";
	if (argc<3) {
		printf("not enough arguments\n");
		exit();
	}

	int fdnum,r,i;
	char *gid_str = argv[1];
	gid_t gid;

	if (strlen(gid_str)>4)
		printf("not a valid gid:%s. Please use at most 4 digits",gid_str);
	gid = (gid_t)atoi(gid_str);

	for (i=2; i<argc; i++) {
		fdnum = open(argv[i],O_RDONLY);		
		if (fdnum<0)
			printf("can't open %s: %e\n",argv[i],fdnum);
		else {			       			     
			if ((r=fchgrp(fdnum,gid))<0)
				printf("chmod failed: %e\n", r);	     
		}
		close(fdnum);
	}
}


