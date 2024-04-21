#include <stdio.h> 
#include <stdlib.h> 
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#define lheight 15
#define lwidth 20
#define PORT 8080
#define PLAYERNAME_LEN 256

enum MOVEMENTS {
	UP,		//0
	RIGHT,	//1
	DOWN,	//2
	LEFT,	//3
};

typedef struct player {
	uint32_t start_x;
	uint32_t start_y;
	uint32_t start_direction;
	uint8_t* player_name;
	uint32_t player_name_len;
} __attribute__((packed)) Player;

typedef struct Info {
	uint32_t frame_timeout;
	Player* players;
	uint32_t pl_count;
} __attribute__((packed)) Info;

typedef struct Package {
    uint32_t magic;
    uint32_t ptype;
    uint32_t datasize;
} __attribute__((packed)) Package;

Package p1;
Package p2;
Package p3;
Package p4;
Package p5;

int get_dir(void) {
	char button;
	scanf("%c", &button);
	switch (button) {
	case 'a':
		return LEFT;
	case 'w':
		return UP;
	case 's':
		return DOWN;
	case 'd':
		return RIGHT;
	}
}

void *input(int* SockFD) {
	int sd = *(int *)SockFD;
	for (;;) {
		Package p6;
		p6.magic = 0xabcdfe01;
		p6.ptype = 0x00;
		p6.datasize = 1;
		write(sd, &p6, sizeof(p6));
		uint8_t dir = get_dir();
		write(sd, &dir, p6.datasize);		
	}
	return NULL;
}

int main(void) { 
//	char map[lheight][lwidth];

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


/*	char playername[] = "Vasya";
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
    }*/

/*	p3.magic = 0xabcdfe01;
    p3.ptype = 0x02;
    p3.datasize = 0;
	write(SockFD, &p3, sizeof(p3));*/


	Info *p = malloc(sizeof(Info));

	if (!read(SockFD, &p4, sizeof(p4))) {
    	perror("read struct p4");
    	exit(1);
	}

	if (!read(SockFD, p, p4.datasize)) {
    	perror("read Info struct");
    	exit(1);
	}

    printf("FPS: %d; pl_count: %d\n", p->frame_timeout, p->pl_count);
	p->players = malloc(sizeof(Player) * p->pl_count);

	read(SockFD, &p5, sizeof(p5));
	read(SockFD, p->players, p5.datasize);
	
	for (size_t i = 0; i < p->pl_count; i++) {
		p->players[i].player_name = malloc(p->players[i].player_name_len);
		read(SockFD, p->players[i].player_name, p->players[i].player_name_len);		
	}
		
	for (size_t i = 0; i < p->pl_count; i++)
		printf("name[%ld]: %s\n", i, p->players[i].player_name);

	for (size_t i = 0; i < p->pl_count; i++)
		printf("name[%ld]: %d\n", i, p->players[i].start_x);

	pthread_t pid;
    void *retval;
    pthread_create(&pid, NULL, input, &SockFD);

    shutdown(SockFD, SHUT_RDWR);
    close(SockFD);
    return 0;
}