//handler for magic and ptype's
//define ptype's


#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#define PTYPE_S 0x00
#define PTYPE_B 0xffffffff
#define MAGIC 0xabcdfe01
#define PLAYERNAME_LEN 256
#define PLAYER_COUNT 4
#define PORT 8080
#define BUFF 300
#define lheight 15
#define lwidth 20
#define FPS 70000

int sockFD;

typedef struct player {
	uint32_t start_x;
	uint32_t start_y;
	uint32_t start_direction;
	uint8_t *player_name;
	uint32_t player_name_len;
} __attribute__((packed)) Player;

typedef struct Info {
	uint32_t frame_timeout;
	Player *players;
	uint32_t pl_count;
} __attribute__((packed)) Info;

typedef struct package {
	uint32_t magic;
	uint32_t ptype;
	uint32_t datasize;
} __attribute__((packed)) Package;

Package p1;
Package p2;
Package p3;
Package p4;
Package p5;

void sigint_handler(int sig) {
	puts("exiting...");
	close(sockFD);
	exit(0);
}

static void genarr1(char arr[lheight][lwidth]) {
    for (int i = 0; i < lheight; i++) {
        for (int j = 0; j < lwidth; j++) {
            arr[i][j] = rand()%2;
        }
    }

    int wall_counter = 0;
    int offset_i, offset_j;
    
    for (int i = 0; i < lheight - 1; i++) {
        for (int j = 0; j < lwidth - 1; j++) {
            
            for (int k = i; k < i+3; k++) {
                for (int m = j; m < j+3; m++) {
                    if (arr[k][m] == 1)
                    wall_counter++;
                }
            }
            
            while (wall_counter > 3) {
                offset_i = rand()%3;
                offset_j = rand()%3;
                if (arr[i + offset_i][j + offset_j] == 1) {
                    arr[i + offset_i][j + offset_j] = 0;
                    wall_counter--;
                } else {
                    continue;
                }                    
            }
            wall_counter = 0;
        }
    }

        for (int i = 0; i < lheight; i++) {
        for (int j = 0; j < lwidth; j++) {
            if (arr[i][j] == 0) {
                arr[i][j] = '.';
            } else {
                arr[i][j] = '#';
            }
        }
    }
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
	read(clientFD, &p1, sizeof(p1));
	char name[PLAYERNAME_LEN];
	read(clientFD, &name, p1.datasize);
	char arr1[lheight][lwidth];
	genarr1(arr1);
	p2.magic = MAGIC;
	p2.ptype = 0x10;
	p2.datasize = sizeof(arr1);

	write(clientFD, &p2, sizeof(p2));
	write(clientFD, &arr1, p2.datasize);
	read(clientFD, &p3, sizeof(p3));

	char *playername = "Petya";
	char *playernameD = "Dasha";
	char *playernameV = "Vasya";
	char *playernameM = "Masha";

	Info *ptr = malloc(sizeof(Info));
	if (ptr == NULL) {
		perror("malloc 1");
		exit(1);
	}

	ptr->frame_timeout = FPS;
	ptr->pl_count = PLAYER_COUNT;

	ptr->players = malloc(sizeof(Player) * ptr->pl_count);
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

	for (int i = 0; i < PLAYER_COUNT; i++)
		printf("Player[%d] name length: %d\n", i, ptr->players[i].player_name_len);
	
	for (int i = 0; i < PLAYER_COUNT; i++) {
		if (ptr->players[i].player_name == NULL) {
			perror("malloc i");
			exit(-1);
		}
	}

	memcpy(ptr->players[0].player_name, playername, strlen(playername)+1);
	memcpy(ptr->players[1].player_name, playernameM, strlen(playernameM)+1);
	memcpy(ptr->players[2].player_name, playernameV, strlen(playernameV)+1);
	memcpy(ptr->players[3].player_name, playernameD, strlen(playernameD)+1);

	for (int i = 0; i < 4; i++)
	ptr->players[i].start_direction = 1;
	for (int i = 0; i < 4; i++)
	ptr->players[i].start_x = 23;
	for (int i = 0; i < 4; i++)
	ptr->players[i].start_y = 12;

	p4.magic = MAGIC;
	p4.ptype = 0x20;
	p4.datasize = sizeof(Info);
	write(clientFD, &p4, sizeof(p4));
	write(clientFD, ptr, p4.datasize);

	p5.magic = 0xabcdfe01;
	p5.ptype = 0x20;
	p5.datasize = sizeof(Player) * PLAYER_COUNT;	

	write(clientFD, &p5, sizeof(p5));
	write(clientFD, ptr->players, p5.datasize);

	p5.magic = MAGIC;
	p5.ptype = 0x20;
	p5.datasize = sizeof(Player) * PLAYER_COUNT;
	
	for (int i = 0; i < PLAYER_COUNT; i++) {
		write(clientFD, ptr->players[i].player_name, ptr->players[i].player_name_len);
	}
/*	for (;;) {
		uint32_t magic;
		uint32_t ptype;
		uint32_t datasize;
		uint8_t button;
		read(clientFD, &magic, sizeof(magic));
		if (magic != MAGIC) return 1;
		read(clientFD, &ptype, sizeof(ptype));
		if (ptype != PTYPE_S) return 1;
		read(clientFD, &datasize, sizeof(datasize));
		read(clientFD, &button, datasize);	
		printf("dir from client %d\n", button);
	}*/


	close(clientFD);
	close(sockFD);
	
	return 0; 
}
