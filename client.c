#include <stdio.h> 
#include <stdlib.h> 
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#define lheight 15
#define lwidth 20
#define PORT 8080
#define PLAYERNAME_LEN 256

typedef struct player {
  uint32_t start_x;
  uint32_t start_y;
  uint32_t start_direction;
  uint32_t player_name_len;
  uint8_t* player_name;
} Player;

typedef struct Info {
	uint32_t frame_timeout;
	uint32_t pl_count;
	Player* players;
} Info;

typedef struct Package {
    uint32_t magic;
    uint32_t ptype;
    uint32_t datasize;
} Package;

Package p1;
Package p2;
Package p3;
Package p4;

int main(void) { 
	char map[lheight][lwidth];

	int SockFD = socket(AF_INET, SOCK_STREAM, 0); 
	if (SockFD == -1) {
		perror("Socket");
		close(SockFD);
		exit(1);
	}

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



	char playername[] = "Vasya";
	p1.magic = 0xabcdfe01;
    p1.ptype = 0x01;
    p1.datasize = sizeof(playername);
	write(SockFD, &p1, sizeof(p1));
	write(SockFD, &playername, p1.datasize);

	read(SockFD, &p2, sizeof(p2));
	char pkgdata[p2.datasize];
	read(SockFD, &pkgdata, p2.datasize);

 	int k = 0;
    for (int i = 0; i < lheight; i++) {
        for (int j  = 0; j < lwidth; j++) {
            map[i][j] = pkgdata[k++];
        }
    }

    for (int i = 0; i < lheight; i++) {
        for (int j  = 0; j < lwidth; j++) {
            printf("%2c", map[i][j]);            
        }
        printf("\n");
    }

	p3.magic = 0xabcdfe01;
    p3.ptype = 0x02;
    p3.datasize = 0;
	write(SockFD, &p3, sizeof(p3));

	read(SockFD, &p4, sizeof(p4));
	Info *p = malloc(p4.datasize);
	if (p == NULL) {
		perror("malloc 1");
		exit(1);
	}


	p->players = malloc(sizeof(Info) * p->pl_count);
	if (p->players == NULL) {
		perror("malloc 2");
		exit(2);
	}


	for (size_t i = 0; i < p->pl_count; i++) {
		p->players[i].player_name = malloc(p->players[i].player_name_len);
		if (p->players[i].player_name == NULL) {
			perror("malloc i");
			exit(3);		
		}
	}


	read(SockFD, p, sizeof(p));
	printf("pl cnt %d\n", p->pl_count);
	for (size_t i = 0; i < p->pl_count; i++)
		printf("player name: %s\n", p->players[i].player_name);
	
    shutdown(SockFD, SHUT_RDWR);
    close(SockFD);
    return 0;
}
