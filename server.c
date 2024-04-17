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
/*	read(clientFD, &p1, sizeof(p1));
	char name[PLAYERNAME_LEN];
	read(clientFD, &name, p1.datasize);

	p2.magic = 0xabcdfe01;
	p2.ptype = 0x10;
	p2.datasize = sizeof(arr1);

	write(clientFD, &p2, sizeof(p2));
	write(clientFD, &arr1, p2.datasize);
	read(clientFD, &p3, sizeof(p3));*/


	char *playernameD = "Dasha";
	char *playernameV = "Vasya";
	char *playernameM = "Masha";

	Info *ptr = malloc(sizeof(Info));
	if (ptr == NULL) {
		perror("malloc 1");
		exit(1);
	}

	ptr->players = malloc(sizeof(Player)*PLAYER_COUNT);
	if (ptr->players == NULL) {
		perror("malloc 2");
		exit(2);
	}

	ptr->players[0].player_name_len = strlen(playername) + 1;
	ptr->players[1].player_name_len = strlen(playernameM) + 1;
	ptr->players[2].player_name_len = strlen(playernameV) + 1;
	ptr->players[3].player_name_len = strlen(playernameD) + 1;

	ptr->players[0].player_name = malloc(strlen(playername) + 1);
	ptr->players[1].player_name = malloc(strlen(playernameM) + 1);
	ptr->players[2].player_name = malloc(strlen(playernameV) + 1);
	ptr->players[3].player_name = malloc(strlen(playernameD) + 1);

	for (int i = 0; i < PLAYER_COUNT; i++) {
		if (ptr->players[i].player_name == NULL) {
			perror("malloc i");
			exit(-1);
		}
	}

	strcpy(ptr->players[0].player_name, playername);
	strcpy(ptr->players[1].player_name, playernameM);
	strcpy(ptr->players[2].player_name, playernameV);
	strcpy(ptr->players[3].player_name, playernameD);

	for (int i = 0; i < 4; i++)
	ptr->players[i].start_direction = 1;
	for (int i = 0; i < 4; i++)
	ptr->players[i].start_x = 23;
	for (int i = 0; i < 4; i++)
	ptr->players[i].start_y = 12;

	ptr->frame_timeout = FPS;
	ptr->pl_count = PLAYER_COUNT;

	p4.magic = 0xabcdfe01;
	p4.ptype = 0x20;
	p4.datasize = sizeof(Info);
	write(clientFD, &p4, sizeof(p4));
	write(clientFD, ptr, p4.datasize);

	printf("Player 0 name length: %d\n", ptr->players[0].player_name_len);
	printf("Player 1 name length: %d\n", ptr->players[1].player_name_len);
	printf("Player 2 name length: %d\n", ptr->players[2].player_name_len);
	printf("Player 3 name length: %d\n", ptr->players[3].player_name_len);

	printf("Player 0 name: %s\n", ptr->players[0].player_name);
	printf("Player 1 name: %s\n", ptr->players[1].player_name);
	printf("Player 2 name: %s\n", ptr->players[2].player_name);
	printf("Player 3 name: %s\n", ptr->players[3].player_name);

	close(clientFD);
	close(sockFD);
	
	return 0; 
}
