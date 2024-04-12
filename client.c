//TODO: fix the map representation where
//	wall = 0xff
//	food = 0xaa
//	player = 0x22
//TODO: implement package 0x00 (client press button)
//TODO: implement package 0xffffffff (server broadcasts each client's movements 
//to others)

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
#define lheight 15
#define lwidth 20

uint32_t frame_timeout;
uint32_t player_count;

char playername[] = "Petya";
int sockFD;

typedef struct player {
  uint32_t start_x;
  uint32_t start_y;
  uint32_t start_direction;
  uint32_t player_name_len;
  uint8_t player_name[];
} Player;

//Player players[player_count];

typedef struct Package {
    uint32_t magic;
    uint32_t ptype;
    uint32_t datasize;
} Package;

Package p1;
Package p2;
Package p3;

void sigint_handler(int sig) {
	puts("exiting...");
	close(sockFD);
	exit(0);
}

int main(void) 
{

	char arr1[lheight][lwidth] = {
    	{'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
		{'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    	{'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    	{'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    	{'#','#','#','#','#','#','#','#','#','#','.','.','.','.','#','#','#','#','#','#'},
    	{'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    	{'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    	{'#','.','.','.','.','.','.','.','.','.','o','.','.','.','.','.','.','.','.','.'},
    	{'#','#','#','#','.','#','#','#','#','#','.','#','#','#','#','.','#','#','#','#'},
    	{'#','#','#','#','.','#','#','#','#','#','.','#','#','#','#','.','#','#','#','#'},
    	{'#','#','#','#','.','#','#','#','#','#','.','#','#','#','#','.','#','#','#','#'},
    	{'#','#','#','#','.','#','#','#','#','#','.','#','#','#','#','.','#','#','#','#'},
    	{'#','#','#','#','.','#','#','#','#','#','.','#','#','#','#','.','#','#','#','#'},
    	{'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    	{'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
	};
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

	read(clientFD, &p1, sizeof(p1));
	char name[PLAYERNAME_LEN];
	read(clientFD, &name, p1.datasize);
	printf("%s", name);

	p2.magic = 0xabcdfe01;
	p2.ptype = 0x10;
	p2.datasize = sizeof(arr1);

	write(clientFD, &p2, sizeof(p2));
	write(clientFD, &arr1, p2.datasize);
	
	read(clientFD, &p3, sizeof(p3));

	printf("\n%x\n", p3.ptype);
	close(clientFD);
	close(sockFD);
	
	return 0; 
}
