#include <stdio.h> 
#include <stdlib.h> 
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#define PLAYERNAME_LEN 256
#define PORT 8080
const char* playername = "Vasya";
typedef struct Package {
    uint32_t magic;
    uint32_t ptype;
    uint32_t datasize;
    uint8_t* data;
} Package;

Package p1;

int main(void) { 
	int SockFD = socket(AF_INET, SOCK_STREAM, 0); 
	if (SockFD == -1) {
		perror("Socket");
		close(SockFD);
		exit(1);
	}

	const int enable = 1;
	int true = 1;

	if (setsockopt(SockFD, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int)) < 0) {
    	perror("Setsockopt");
		close(SockFD);
    	exit(1);
	}

	struct sockaddr_in servAddr; 
	servAddr.sin_family = AF_INET; 
	servAddr.sin_port = htons(PORT); 
	servAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 

	if (connect(SockFD, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) { 
		perror("connect");
		close(SockFD);
		exit(1);
	} 

    p1.ptype = 0x01;
    p1.magic = 0xabcdfe01;
    p1.datasize = PLAYERNAME_LEN;

	//convert const char* to uint8_t*
	p1.data = (uint8_t*) malloc(sizeof(playername));
	if (p1.data == NULL) {
		printf("Error with malloc\n");
		return 1;
	}

    write(SockFD, &p1.magic, sizeof(htonl(p1.magic)));
    write(SockFD, &p1.ptype, sizeof(htonl(p1.ptype)));
    write(SockFD, &p1.datasize, sizeof(htonl(p1.datasize)));
	send(SockFD, p1.data, sizeof(uint8_t)*p1.datasize, 0);

    shutdown(SockFD, SHUT_RDWR);
    close(SockFD);
    return 0;
}