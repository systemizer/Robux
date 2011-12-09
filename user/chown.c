#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	binaryname = "chown";
	if (argc<3) {
		printf("not enough arguments\n");
		exit();
	}

	int fdnum,r,i;
	char *username = argv[1];
	struct user_info u_info;
	uid_t uid; 

	if ((r=get_user_by_name(username,&u_info))<0)
	{
		printf("could not find user %s",username);
		exit();
	}

	uid = u_info.ui_uid;
	for (i=2; i<argc; i++) {
		fdnum = open(argv[i],O_RDONLY);		
		if (fdnum<0)
			printf("can't open %s: %e\n",argv[i],fdnum);
		else {			       			     
			if ((r=fchown(fdnum,uid))<0)
				printf("chmod failed: %e\n", r);	     
		}
		close(fdnum);
	}
}


