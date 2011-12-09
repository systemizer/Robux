#include <inc/lib.h>

int flag[256];

void lsdir(const char*, const char*);
void ls1(const char*, bool, off_t, const char*);
void ls2(const char *prefix, struct Stat *f);

void
ls(const char *path, const char *prefix)
{
	int r;
	struct Stat st;

	if ((r = stat(path, &st)) < 0)
	{
		printf("stat %s: %e\n", path, r);
		return;
	}
	if (st.st_isdir && !flag['d'])
		lsdir(path, prefix);
	else
		ls2(0, &st); 
}

void
lsdir(const char *path, const char *prefix)
{
	int fd, n;
	struct File f;

	if ((fd = open(path, O_RDONLY)) < 0)
		panic("open %s: %e", path, fd);
	while ((n = readn(fd, &f, sizeof f)) == sizeof f)
		if (f.f_name[0])
		{
			struct Stat st;
			int r;
			if((r = stat(f.f_name, &st)) < 0)
			{
				memset(&st, 0, sizeof(st));
				strncpy(st.st_name, f.f_name, MAXNAMELEN);
				st.st_uid = 0xFFFF;
				st.st_gid = r;
				st.st_size = r;
			}
			ls2(prefix, &st);
		}
	if (n > 0)
		panic("short read in directory %s", path);
	if (n < 0)
		panic("error reading directory %s: %e", path, n);
}


void
ls2(const char *prefix, struct Stat *f)
{
	if(flag['l'])
	{
		printf("-%c%c%c%c%c%c%c%c%c ",
				(f->st_perm & FSP_O_R) ? 'r':'-',
				(f->st_perm & FSP_O_W) ? 'w':'-',
				(f->st_perm & FSP_O_X) ? 'x':'-',
				(f->st_perm & FSP_G_R) ? 'r':'-',
				(f->st_perm & FSP_G_W) ? 'w':'-',
				(f->st_perm & FSP_G_X) ? 'x':'-',
				(f->st_perm & FSP_A_R) ? 'r':'-',
				(f->st_perm & FSP_A_W) ? 'w':'-',
				(f->st_perm & FSP_A_X) ? 'x':'-');

		struct user_info info;
		int r;
		if(f->st_uid != 0xFFFF)
			r = get_user_by_id(f->st_uid, &info);
		else
		{
			r = 0;
			strcpy(info.ui_name, "?unknown?");
		}
		if(r < 0)
			printf("%15d ", f->st_uid);
		else
			printf("%15s ", info.ui_name); 

		printf("%5d %11d %s\n", f->st_gid, f->st_size, f->st_name);
	}
	else
	{
		char *sep;
		if(prefix[0] && prefix[strlen(prefix)-1] != '/')
			sep = "/";
		else
			sep = "";

		printf("%s%s%s\n", prefix, sep, f->st_name); 
	}
}

void
ls1(const char *prefix, bool isdir, off_t size, const char *name)
{
	const char *sep;

	if(flag['l'])
		printf("%11d %c ", size, isdir ? 'd' : '-');
	if(prefix) {
		if (prefix[0] && prefix[strlen(prefix)-1] != '/')
			sep = "/";
		else
			sep = "";
		printf("%s%s", prefix, sep);
	}
	printf("%s", name);
	if(flag['F'] && isdir)
		printf("/");
	printf("\n");
}

void
usage(void)
{
	printf("usage: ls [-dFl] [file...]\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int i;
	struct Argstate args;

	argstart(&argc, argv, &args);
	while ((i = argnext(&args)) >= 0)
		switch (i) {
		case 'd':
		case 'F':
		case 'l':
			flag[i]++;
			break;
		default:
			usage();
		}

	if (argc == 1)
		ls("/", "");
	else {
		for (i = 1; i < argc; i++)
			ls(argv[i], argv[i]);
	}
}

