#include <inc/lib.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>


#define MAXPENDING 10

void die(const char *msg, int error_code)
{
	cprintf(msg, error_code);
	exit();
}


void
handle_client(int clientsock)
{
	// Fork
	// Return immediately if parent to continue accepting,
	// otherwise spawn the shell
	int r = fork();
	if(r == 0)
	{
		dup(clientsock, 0);
		dup(clientsock, 1);
		r = spawnl("login", "-t",  NULL);
		wait(r);
		close(clientsock);
	}

	close(clientsock);
}

void
do_listen(short port)
{
	int serversock, clientsock;
	struct sockaddr_in serveraddr, clientaddr;
	unsigned int clientlen;

	serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(serversock < 0)
		die("Failed to create socket: %e\n", serversock);

	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(port);

	int r = bind(serversock, (struct sockaddr*) &serveraddr, sizeof(serveraddr));
	if(r < 0)
		die("Failed to bind: %e\n", r);

	r = listen(serversock, MAXPENDING);
	if(r < 0)
		die("Failed to listen: %e\n", r);

	printf("Telnet server running on port %d\n", port);
	while(1) 
	{
		clientsock = accept(serversock, (struct sockaddr *) &clientaddr, &clientlen);
		if(clientsock < 0)
			die("Failed to accept client connection: %e\n", clientsock);

		printf("Telnet: Client connected from %s\n", inet_ntoa(clientaddr.sin_addr));
		handle_client(clientsock);
	}
	

	close(serversock);

}


void
umain(int argc, char *argv[])
{
	short port = 23;
	if(argc == 2)
		port = atoi(argv[1]);
	else if(argc > 2)
	{
		printf("Usage: telnet [port]\n");
	}

	//close(0);
	//close(1);
	//opencons();
	//opencons();

	do_listen(port);

}
