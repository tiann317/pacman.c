#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <signal.h>
#define PLAYERNAME_LEN 256
#define PORT 8080
#define BUFF 300

const char* playername = "Petya";
int sockFD;

typedef struct Package {
    uint32_t magic;
    uint32_t ptype;
    uint32_t datasize;
    uint8_t* data;
} Package;

Package p1;


void sigint_handler(int sig) {
	puts("exiting...");
	close(sockFD);
	exit(0);
}

int main(void) 
{
	signal(SIGINT, sigint_handler);
	sockFD = socket(AF_INET, SOCK_STREAM, 0);
	if (sockFD < 0) {
		perror("Socket");
		close(sockFD);
		exit(1);
	}

	const int enable = 1;
	int true = 1;

	if (setsockopt(sockFD, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int)) < 0) {
    	perror("Setsockopt");
		close(sockFD);
    	exit(1);
	}

	struct sockaddr_in servAddr; 
	servAddr.sin_family = AF_INET; 
	servAddr.sin_port = htonl(PORT); 
	servAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockFD, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
		close(sockFD);
		perror("bind");
		exit(1);
	} 

	if (listen(sockFD, 5) == -1) {
		close(sockFD);
		perror("listen");
		exit(1);
	}

	int clientFD = accept(sockFD, NULL, NULL);
	if (clientFD < 0) {
		perror("accept");
		close(sockFD);
		exit(1);
	}

    read(clientFD, &p1.magic, sizeof(uint32_t)));
    read(clientFD, &p1.ptype, sizeof(uint32_t));
    read(clientFD, &p1.datasize, sizeof(uint32_t));
    printf("magic: %d\nptype: %d\ndatasize: %d\n", p1.magic, p1.ptype, p1.datasize);

	close(clientFD);
	close(sockFD);
	
	return 0; 
}
