#ifndef JOS_INC_SECURITY_H
#define JOS_INC_SECURITY_H


#define NAME_LEN 32
#define PASS_LEN 128

#define COMMENT_LEN 256
#define PATH_LEN 256

// Define as typedefs for JOS
// and #define for the fs format util
// to override sys/types.h typedefs
#ifndef FS_FORMAT_UTIL
typedef uint16_t uid_t;
typedef uint16_t gid_t;
typedef uint16_t fsperm_t;
#else
#define uid_t uint16_t
#define gid_t uint16_t
#define fsperm_t uint16_t
#endif

#define FSP_A_X 0001
#define FSP_A_W 0002
#define FSP_A_R 0004

#define FSP_G_X 0010
#define FSP_G_W 0020
#define FSP_G_R 0040

#define FSP_O_X 0100
#define FSP_O_W 0200
#define FSP_O_R 0400


#define DEFAULT_FILE_CREATE_PERM       FSP_O_W | FSP_O_R | FSP_G_R | FSP_A_R | FSP_A_X | FSP_G_X | FSP_O_X
#define DEFAULT_DIR_CREATE_PERM       FSP_O_W | FSP_O_R | FSP_O_X | FSP_G_R | FSP_G_W | FSP_A_R | FSP_O_X | FSP_G_X | FSP_A_X

struct user_info
{
	char ui_name[NAME_LEN];
	char ui_pass[PASS_LEN]; // Security server zeroes this before returning
	uid_t ui_uid;
	gid_t ui_gid;
	char ui_comment[COMMENT_LEN];
	char ui_home[PATH_LEN];
	char ui_shell[PATH_LEN];
};



#endif
