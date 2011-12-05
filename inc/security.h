#ifndef JOS_SECURITY_H
#define JOS_SECURITY_H


#define NAME_LEN 32
#define PASS_LEN 32

#define COMMENT_LEN 256
#define PATH_LEN 256

typedef uint16_t uid_t;
typedef uint16_t gid_t;
typedef uint16_t fsperm_t;

#define FSP_A_X 0001
#define FSP_A_W 0002
#define FSP_A_R 0004

#define FSP_G_X 0010
#define FSP_G_W 0020
#define FSP_G_R 0040

#define FSP_O_X 0100
#define FSP_O_W 0200
#define FSP_O_R 0400


struct user_info
{
	char ui_name[NAME_LEN];
	uid_t ui_uid;
	gid_t ui_gid;
	char ui_comment[COMMENT_LEN];
	char ui_home[PATH_LEN];
	char ui_shell[PATH_LEN];
};



#endif
