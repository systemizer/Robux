#include <inc/lib.h>


void 
do_security()
{
	cprintf("it has built!");
}


void 
umain(int argc, char *argv[])
{
	binaryname="login";

	do_security();

	exit();
}


/* #include <inc/stdio.h> */
/* #include <inc/string.h> */
/* #include <inc/error.h> */

/* #include <inc/env.h> */

/* #include <kern/pmap.h> */
/* #include <kern/login.h> */

/* #include <security/security.h> */

/* struct LoginToken token; */

/* // authenticate takes a username and password and  */
/* // passes it to security, which will check if the */
/* // combination is valid. authenticate returns the  */
/* // userid if valid, else return -1 */
/* static int */
/* authenticate(void)  */
/* { */
/* 	envid_t from_envid; */
/* 	envid_t result; */
/* 	//cprintf("%s\n",token.username); */
/* 	//cprintf("%s\n",token.password); */
/* 	envid_t sc_envid = ipc_find_env(ENV_TYPE_SC); */
/* 	if (!sc_envid) */
/* 		panic("The security server could not be found"); */
/* 	ipc_send(sc_envid,SCREQ_AUTH,&token,PTE_P|PTE_U);		 */
/* 	result =  ipc_recv(&from_envid,NULL,NULL); */
/* 	while (from_envid!=sc_envid) { */
/* 		result = ipc_recv(&from_envid,NULL,NULL); */
/* 	} */
/* 	return result & AUTH; */

/* } */

/* //prompts the user for a username and password. */
/* //users authenticate to log the user into the */
/* //system. On success, spawn a new environment */
/* //with the returned userid */
/* static void  */
/* prompt_login(void) */
/* { */
/* 	char *buf; */
/* 	buf = readline("username: "); */
/* 	strcpy(token.username,buf); */
/* 	buf= readline("password: "); */
/* 	strcpy(token.password,buf); */
/* } */

/* int */
/* login_init(void)  */
/* { */
/* 	struct Page *upage  = page_alloc(ALLOC_ZERO); */
/* 	if (!upage) */
/* 		return -E_NO_MEM; */
/* 	upage->pp_ref++; */
/* 	token.username = page2kva(upage); */

/* 	struct Page *ppage  = page_alloc(ALLOC_ZERO); */
/* 	if (!ppage) */
/* 		return -E_NO_MEM; */
/* 	ppage->pp_ref++; */
/* 	token.password = page2kva(ppage); */

/* 	return 0; */
/* } */
			
	

/* void */
/* login(void) */
/* { */
/* 	login_init(); */
/* 	while (1)  */
/* 	{		 */
/* 		prompt_login(); */
/* 		if (authenticate()) { */
/* 			cprintf("authentication complete! Going to user environment"); */
/* 			//start shell here */
/* 		} */
/* 	} */
		
/* } */
