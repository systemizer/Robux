
/* result bits from security env */
#define AUTH 0x1

/* end result bits from security env */

void login(void);

struct LoginToken {
	char *username;
	char *password;
};
