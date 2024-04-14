#include <stdio.h> 
#include <stdlib.h> 
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#define lheight 15
#define lwidth 20
#define PORT 8080
#define PLAYERNAME_LEN

typedef struct player {
  uint32_t start_x;
  uint32_t start_y;
  uint32_t start_direction;
  uint8_t player_name[PLAYERNAME_LEN];
} Player;

typedef struct Package {
    uint32_t magic;
    uint32_t ptype;
    uint32_t datasize;
} Package;

Package p1;
Package p2;
Package p3;
Package p4;
Package p5;

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

	printf("magic %x\n", p4.magic);
	printf("ptype %d\n", p4.ptype);	
	printf("amount of players %d\n", p4.datasize);

	typedef struct Info {
		uint32_t frame_timeout;
		uint32_t pl_count;
		Player players[p4.datasize];
	} Info;

	Info info;
	read(SockFD, &p5, sizeof(p5));
	read(SockFD, &info, p5.datasize);
	printf("FPS %d\n", info.frame_timeout);
	for (int i = 0; i < p4.datasize; i++) 
		printf("Startx player[%d]: %d\n", i, info.players[i].start_x);
	
	for (int i = 0; i < p4.datasize; i++) 
		printf("Starty player[%d]: %d\n", i, info.players[i].start_y);
	
	for (int i = 0; i < p4.datasize; i++) 
		printf("dir player[%d]: %d\n", i, info.players[i].start_direction);

	for (int i = 0; i < p4.datasize; i++) 
		printf("Startx player[%d]: %s\n", i, info.players[i].player_name);

    shutdown(SockFD, SHUT_RDWR);
    close(SockFD);
    return 0;
}
