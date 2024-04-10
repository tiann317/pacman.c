#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
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

	int true = 1;
	if (setsockopt(sockFD, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int)) < 0) {
    	perror("Setsockopt");
		close(sockFD);
    	exit(1);
	}

	struct sockaddr_in servAddr; 
	servAddr.sin_family = AF_INET; 
	servAddr.sin_port = htons(PORT); 
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(servAddr.sin_zero, '\0', sizeof servAddr.sin_zero);

	if (bind(sockFD, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
		close(sockFD);
		perror("bind");
		exit(1);
	} 

	if (listen(sockFD, 5) == -1) {
		close(sockFD);
		perror("listen");
		exit(1);
	} else {
		printf("Server listening...\n");
	}

	int clientFD = accept(sockFD, NULL, NULL);
	if (clientFD < 0) {
		perror("accept");
		close(sockFD);
		exit(1);
	}

    read(clientFD, &p1.magic, sizeof(uint32_t));
    read(clientFD, &p1.ptype, sizeof(uint32_t));
    read(clientFD, &p1.datasize, sizeof(uint32_t));
	p1.data = malloc(sizeof(uint8_t)*p1.datasize);
	if (p1.data == NULL) {
		printf("Error with malloc\n");
		return 1;
	}	
	recv(sockFD, p1.data, sizeof(p1.data), 0);

//	read(clientFD, p1.data, sizeof(uint8_t)*p1.datasize);	
    printf("magic: %x\nptype: %x\ndatasize: %d\n",\
	 p1.magic, p1.ptype, p1.datasize);

	for (uint8_t* ptr = p1.data; ptr < p1.data + p1.datasize; ++ptr) {
		printf("%d ", *ptr);
	}
	close(clientFD);
	close(sockFD);
	
	return 0; 
}