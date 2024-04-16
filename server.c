//TODO: fix the map representation where
//	wall = 0xff
//	food = 0xaa
//	player = 0x22
//TODO: async I/O poll/select to hancle multiple FDs
//TODO: implement package 0x00 (client press button)
//TODO: implement package 0xffffffff (server broadcasts
// each client's movements to others)
//
//fact: we pass amount of players and an array of structs
//			which max_index is the amount of players
//
//question 1: how to pass this in a single struct Info
//
//question 2: should all this structs be trnsferred in a
//			single struct Message.
//
//question 3: if so how should we handle the receivers
//			catchcng and handling the structs correctly
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#define PLAYERNAME_LEN 256
#define PLAYER_COUNT 4
#define PORT 8080
#define BUFF 300
#define lheight 15
#define lwidth 20
#define FPS 70000

char* playername = "Petya";
int sockFD;

typedef struct player {
  uint32_t start_x;
  uint32_t start_y;
  uint32_t start_direction;
  uint32_t player_name_len;
  uint8_t *player_name;
} Player;

typedef struct Info {
	uint32_t frame_timeout;
	uint32_t pl_count;
	Player *players;
} Info;

typedef struct package {
    uint32_t magic;
    uint32_t ptype;
    uint32_t datasize;
} Package;

Package p1;
Package p2;
Package p3;
Package p4;

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

	char *playernameD = "Dasha";
	char *playernameV = "Vasya";
	char *playernameM = "Masha";

	Info *ptr = malloc(sizeof(Info));
	ptr->players = malloc(sizeof(Player)*PLAYER_COUNT);
	ptr->players[0].player_name = malloc(sizeof(playername));
	ptr->players[1].player_name = malloc(sizeof(playernameM));
	ptr->players[2].player_name = malloc(sizeof(playernameV));
	ptr->players[3].player_name = malloc(sizeof(playernameD));

	ptr->players[0].player_name = playername;
	ptr->players[1].player_name = playernameM;
	ptr->players[2].player_name = playernameV;
	ptr->players[3].player_name = playernameD;

	ptr->players[0].player_name_len = sizeof(playername);
	ptr->players[1].player_name_len = sizeof(playernameM);
	ptr->players[2].player_name_len = sizeof(playernameV);
	ptr->players[3].player_name_len = sizeof(playernameD);

	for (int i = 0; i < 4; i++)
	ptr->players[i].start_direction = 1;
	for (int i = 0; i < 4; i++)
	ptr->players[i].start_x = 23;
	for (int i = 0; i < 4; i++)
	ptr->players[i].start_y = 12;

	ptr->frame_timeout = FPS;
	ptr->pl_count = PLAYER_COUNT;

	for (int i = 0; i < PLAYER_COUNT; i++)
	printf("name %s\n", ptr->players[i].player_name);

	printf("number of players %d\n", ptr->pl_count);

	p4.magic = 0xabcdfe01;
	p4.ptype = 0x20;
	p4.datasize = sizeof(Info);
	write(clientFD, &p4, sizeof(p4));
	write(clientFD, ptr, sizeof(ptr));

	close(clientFD);
	close(sockFD);
	
	return 0; 
}
