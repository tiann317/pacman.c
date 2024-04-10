#include <stdio.h> 
#include <stdlib.h> 
#include <arpa/inet.h>
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
	int sockD = socket(AF_INET, SOCK_STREAM, 0); 

	int true = 1;
	if (setsockopt(sockD, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int)) == -1) {
		perror("setsockopt");
		close(sockD);
		exit(1);
	}

	struct sockaddr_in servAddr; 
	servAddr.sin_family = AF_INET; 
	servAddr.sin_port = htonl(PORT); 
	servAddr.sin_addr.s_addr = INADDR_ANY; 
	int connectStatus = connect(sockD, (struct sockaddr*)&servAddr, sizeof(servAddr)); 

	if (connectStatus == -1) { 
		perror("connect");
		close(sockD);
		exit(1);
	} 

    p1.ptype = 0x01;
    p1.magic = 0xabcdfe01;
    p1.datasize = PLAYERNAME_LEN;
    p1.data = playername;

    write(sockD, &p1.magic, sizeof(htonl(p1.magic)));
    write(sockD, &p1.ptype, sizeof(htonl(p1.ptype)));
    write(sockD, &p1.datasize, sizeof(htonl(p1.datasize)));

    close(connectStatus);
    shutdown(sockD, SHUT_RDWR);
    close(sockD);
    return 0;
}
