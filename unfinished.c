//todo: rewrite it in Rust pog
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h> 
#include <getopt.h>
#include <stdlib.h> 
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <string.h>
#include <time.h>
#define lheight             15
#define lwidth              20
#define height              30
#define width               40
#define PLAYERNAME_LEN      256
#define MAX_PACMAN_COUNT    4
#define PORT                8080
#define FPS_TIMEOUT         300000
#define mgc                 0xabcdfe01

uint32_t fps;
uint8_t temp_key;
int pl_count, score;
char *name;                     //temporary storage for names -n
int is_server = -1;                         
int sd, cd, maxfd, ret;                           
int ready_clients;
int cnt = 1;
char arr1[lheight][lwidth] = {0};
char arr2[lheight][lwidth] = {0};
char arr3[lheight][lwidth] = {0};
char arr4[lheight][lwidth] = {0};
char map[height][width] = {0};
chtype wall = '#';
chtype food = '.';
chtype empt = ' ';

enum DIRECTION {
    UP,
    RIGHT, 
    DOWN,
    LEFT,
    QUIT
};

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    Point max;
    Point min;
} Boundary;

typedef struct {
	uint32_t start_x;
	uint32_t start_y;
	uint32_t start_direction;
	uint8_t *player_name;
	uint32_t player_name_len;
} __attribute__((packed)) Player;

typedef struct {
	uint32_t frame_timeout;
	Player *players;
	uint32_t pl_count;
} __attribute__((packed)) Info;

typedef struct {
    uint32_t magic;
    uint32_t ptype;
    uint32_t datasize;
} __attribute__((packed)) Package;

typedef struct {
    uint8_t direction;
    char *player_name;
} __attribute__((packed)) Id;

typedef struct {
    char *name;
    int namelen;
    uint8_t direction;
    Point coords;
    Point offset_coords;
    int score;
    bool is_connected;
    int sd;
    chtype head;
} Pacman;

Boundary fullscr;
Boundary boundary;
Boundary quarter;
struct pollfd fds[MAX_PACMAN_COUNT + 3] = {0}; //stdin, stdout, stderr
Pacman pacmans[MAX_PACMAN_COUNT] = {0};

static void genarr1(char arr[lheight][lwidth]) {
    for (int i = 0; i < lheight; i++) {
        for (int j = 0; j < lwidth; j++) {
            arr[i][j] = 0;
        }
    }
    for (int i = 0; i < lheight - 1; i+=4) {
        for (int j = 0; j < lwidth - 1; j+=9) {  
            int start_rnd_y = i + 1 + rand()%5;
            int end_rnd_y = i + 2 + rand()%4;
            int start_rnd_x = j + 1 + rand()%9;
            int end_rnd_x = j + 2 + rand()%8;
            int fix_rnd_y = i + 1 + rand()%4;
            int fix_rnd_x = j + 1 + rand()%9;
                for (int k = start_rnd_y; k < end_rnd_y; k++) 
                arr[k][fix_rnd_x] = 1;
                for (int m = start_rnd_x; m < end_rnd_x; m++) 
                arr[fix_rnd_y][m] = 1;
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

uint8_t transform_key(uint8_t key) {
    switch (key) {
        case 119:
        case 3:
            return UP;
        case 100:
        case 5:
            return RIGHT;
        case 115:
        case 2:
            return DOWN;
        case 97:
        case 4:
            return LEFT;
        case 113:
            return QUIT; 
    }
}

void set_dir(uint8_t key, size_t i) {
        if (key == 'w' || key == 3) {
            pacmans[i].direction = transform_key(key);
        } else if (key == 's' || key == 2) {
            pacmans[i].direction = transform_key(key);
        } else if (key == 'a' || key == 5) {
            pacmans[i].direction = transform_key(key);
        } else if (key == 'd' || key == 4) {
            pacmans[i].direction = transform_key(key);
        } else if (key == 'q') {
            pacmans[i].direction = transform_key(key);
        }
}

void server_input() {
    while (TRUE) {
        Package key;
        key.magic = mgc;
        key.ptype = 0x00;
        key.datasize = 1;
        for (size_t i = 1; i < pl_count; ++i) 
            write(pacmans[i].sd, &key, sizeof(key));
        temp_key = getch();
        if (temp_key == pacmans[0].direction) {
            break;
        } else {
            continue;
        }
        set_dir(temp_key, 0);
        send_all(pacmans[0].direction);   
    }
}

void client_input() {
    while (TRUE) {
        Package key;
        key.magic = mgc;
        key.ptype = 0x00;
        key.datasize = 1;
        write(sd, &key, sizeof(key));
        temp_key = getch();
        if (temp_key == pacmans[0].direction) {
            break;
        } else {
            continue;
        }
        set_dir(temp_key, 0);
        write(sd, &pacmans[0].direction, 1);   
    }
}
/*void server_keys() {
    //this should be modified to catch ptype 0xffffffff
    while (TRUE) {
        Package s_key;
        read(sd, &s_key, sizeof(s_key));
        if (s_key.ptype == 0x00 && s_key.magic == mgc)              //server key
            read(sd, &pacmans[0].direction, 1);
        if (s_key.ptype == 0xffffffff && s_key.magic == mgc) {      //clients key broadcasted
            Id c_key;
            read(sd, &c_key, )
        }
    }
}*/


void send_all(uint8_t key) {
    for (size_t i = 1; i < pl_count; ++i)
    write(pacmans[i].sd, &key, 1);
}

void send_all_with_exception(uint8_t key, int exception) {
    for (size_t i = 1; i < pl_count; ++i) {
        if (i != exception)
        write(pacmans[i].sd, &key, 1);
    }
}

void server_handler(struct sockaddr_in *addr) {
    sd = socket(AF_INET, SOCK_STREAM, 0);
	int val = 1;
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) < 0) {
    	perror("Setsockopt");
		close(sd);
    	exit(1);
	}
	if (bind(sd, (struct sockaddr*)addr, sizeof(*addr)) < 0) {
		close(sd);
		perror("bind");
		exit(1);
	} 
	if (listen(sd, 5) == -1) {
		close(sd);
		perror("listen");
		exit(1);
	} else {
		mvprintw(0, 0, "Server listening...\n");
	}
    while (TRUE) {
        if (cnt == pl_count) break;
        if (cd = accept(sd, NULL, NULL)) {
            pacmans[cnt].sd = cd;
            
            Package connect;
            read(cd, &connect, sizeof(connect));
            if (connect.magic != mgc || connect.datasize > PLAYERNAME_LEN) {
                close(cd);
                continue;;
            } else {
                pacmans[cnt].namelen = connect.datasize;    
            }
            char tmpname[PLAYERNAME_LEN];
            read(cd, &tmpname, connect.datasize);
            pacmans[cnt].name = tmpname;
            
            Package sendmap;
            sendmap.magic = mgc;
            sendmap.ptype = 0x10;
            sendmap.datasize = sizeof(arr1);
	        write(cd, &sendmap, sizeof(sendmap));
	        write(cd, &arr1, sendmap.datasize);
            
            Package client_ready;
	        read(cd, &client_ready, sizeof(client_ready));            
            cnt++;
            ready_clients++;
            continue;
        }
    }

    Info *ptr = malloc(sizeof(Info));
	if (ptr == NULL) {
		perror("malloc 1");
		exit(1);
	}

	ptr->frame_timeout = FPS_TIMEOUT;
	ptr->pl_count = pl_count;

	ptr->players = malloc(sizeof(Player) * ptr->pl_count);
	if (ptr->players == NULL) {
		perror("malloc 2");
		exit(1);
	}

    for (size_t i = 0; i < pl_count; ++i) {
        pacmans[i].coords.x = rand()%35;
        pacmans[i].coords.y = rand()%25;
        pacmans[i].offset_coords.x = pacmans[i].coords.x + boundary.min.x;
        pacmans[i].offset_coords.y = pacmans[i].coords.y + boundary.min.y;
        pacmans[i].direction = rand()%4;
        pacmans[i].head = 'o';
        pacmans[i].is_connected = TRUE;
        pacmans[i].score = 0; 
        uint32_t namelen = pacmans[i].namelen;
        ptr->players[i].player_name_len = namelen;
        ptr->players[i].player_name = malloc(namelen);
	    memcpy(ptr->players[i].player_name, pacmans[i].name, namelen);
        ptr->players[i].start_direction = pacmans[i].direction;        
        ptr->players[i].start_x = pacmans[i].coords.x;    
        ptr->players[i].start_y = pacmans[i].coords.y;            
    }

    Package sg_info;                            //sg == start game package
	sg_info.magic = mgc;
	sg_info.ptype = 0x20;
	sg_info.datasize = sizeof(Info);

    for (size_t i = 1; i < pl_count; i++) {
        write(pacmans[i].sd, &sg_info, sizeof(sg_info));
        write(pacmans[i].sd, ptr, sg_info.datasize);
    }

    Package sg_players;
	sg_players.magic = mgc;
	sg_players.ptype = 0x20;
	sg_players.datasize = sizeof(Player) * pl_count;

    for (size_t i = 1; i < pl_count; i++) {
        write(pacmans[i].sd, &sg_players, sizeof(sg_players));
        write(pacmans[i].sd, ptr->players, sg_players.datasize);

    }

    for (size_t i = 1; i < pl_count; i++) {             //outer cycle for descriptors
        for (size_t j = 0; j < pl_count; ++j)           //inner cycle to send all array members
		    write(pacmans[i].sd, pacmans[j].name, pacmans[j].namelen);
    }
}

Info *client_connect(struct sockaddr_in addr) {
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sd, &addr, sizeof(struct sockaddr_in)) < 0) {
		perror("server fd");
		close(sd);
		exit(1);
    }
    Package connect;
    connect.magic = mgc;
    connect.ptype = 0x02;
    connect.datasize = strlen(name) + 1;
    write(sd, &connect, sizeof(connect));
    write(sd, name, connect.datasize);

    Package getmap;
	read(sd, &getmap, sizeof(getmap));
	char one_dim_map[getmap.datasize];
	read(sd, &one_dim_map, getmap.datasize);
    expand_map(one_dim_map);                    
                                                
    Package client_ready;
    client_ready.magic = mgc;
    client_ready.ptype = 0x02;
    client_ready.datasize = 0;
	write(sd, &client_ready, sizeof(client_ready));

    Package sg_info;                                //start game package header
    read(sd, &sg_info, sizeof(sg_info));
	
	Info *p = malloc(sizeof(Info));                 
    read(sd, p, sg_info.datasize);                  //start game package data
	p->players = malloc(sizeof(Player) * p->pl_count);

    Package sg_players;
	read(sd, &sg_players, sizeof(sg_players));
	read(sd, p->players, sg_players.datasize);
	
	for (size_t i = 0; i < p->pl_count; ++i) {
		p->players[i].player_name = malloc(p->players[i].player_name_len);
		read(sd, p->players[i].player_name, p->players[i].player_name_len);		
	}
    return p;
}

void expand_map(char *arr) {
    int k = 0;
    for (int i = 0; i < lheight; i++) {
        for (int j  = 0; j < lwidth; j++) {
            arr1[i][j] = arr[k++];
        }
    }
    gen_map(arr1,arr2,arr3,arr4,map);
}

void gen_map(char arr1[lheight][lwidth],\
 char arr2[lheight][lwidth], char arr3[lheight][lwidth],\
  char arr4[lheight][lwidth], char map[height][width]) {
    for (int i = 0; i < lheight; i++) {
        for (int j = 0; j < lwidth; j++) 
        arr2[i][j] = arr1[i][lwidth - 1 - j];        
    }   
    for (int i = 0; i < lheight; i++) {
        for (int j = 0; j < lwidth; j++) 
        arr3[i][j] = arr1[lheight - 1 - i][j];        
    } 
    for (int i = 0; i < lheight; i++) {
        for (int j = 0; j < lwidth; j++) 
        arr4[i][j] = arr1[lheight - 1 - i][lwidth - 1 - j];        
    }     
    for (int i = 0; i < height/2; i++) {
        for (int j = 0; j < width/2; j++)
        map[i][j] = arr1[i][j];
    }
    for (int i = 0; i < height/2; i++) {
        for (int j = 0; j < width/2; j++)
        map[height/2 + i][j] = arr3[i][j];
    }
    for (int i = 0; i < height/2; i++) {
        for (int j = 0; j < width/2; j++)
        map[i][width/2 + j] = arr2[i][j];
    }
    for (int i = 0; i < height/2; i++) {
        for (int j = 0; j < width/2; j++)
        map[height/2 + i][ width/2 + j] = arr4[i][j];
    }
}


   /*    fds[sd].fd = sd;
    fds[sd].events = POLLIN;
    maxfd = sd;

    while (maxfd != pl_count + 3) {
        ret = poll(&fds, maxfd + 1, -1);
        if (fds[sd].revents & POLLIN) {
            cd = accept(sd, NULL, NULL);
            fds[cd].fd = cd;
            fds[cd].events = POLLIN;
            if (cd > maxfd)
                maxfd = cd;
            if (--ret == 0)
                continue;
        }
        for (size_t i = 0; i <= maxfd; ++i) {
            if (fds[i].revents & POLLIN) {
                Package connect;
                read(fds[i].fd, &connect, sizeof(connect));
                ready_clients++;
                pacmans[ready_clients].sd = fds[i].fd;
                pacmans[ready_clients].namelen = connect.datasize;
                char tmpname[PLAYERNAME_LEN];
                read(fds[i].fd, &tmpname, connect.datasize);
                pacmans[ready_clients].name = tmpname; 
            }
        }    
    }*/ 
     
static void print_map(char arr[height][width]) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (arr[i][j] == '.') {
                mvaddch(boundary.min.y + i, \
                boundary.min.x + j, food);
            } else if (arr[i][j] == ' ') {
                mvaddch(boundary.min.y + i, \
                boundary.min.x + j, empt);
            } else {
                mvaddch(boundary.min.y + i, \
                boundary.min.x + j, wall);
            }   
        }
        mvaddch(i, 0, empt);
    }
}

void init() {
    initscr();
    noecho();
    keypad(stdscr, TRUE);
    getmaxyx(stdscr, fullscr.max.y, fullscr.max.x); 
    boundary.max.x = fullscr.max.x/2 + 20;
    boundary.min.x = fullscr.max.x/2 - 20;
    boundary.max.y = fullscr.max.y/2 + 15;
    boundary.min.y = fullscr.max.y/2 - 15;
}

void pack_playerdata(Info* data) {
    pl_count = data->pl_count;
    fps = data->frame_timeout;
    for (size_t i = 0; i < pl_count; ++i) {
        pacmans[i].name = data->players[i].player_name;
        pacmans[i].is_connected = TRUE;
        pacmans[i].coords.x = data->players[i].start_x;
        pacmans[i].coords.y = data->players[i].start_y;
        pacmans[i].offset_coords.x = pacmans[i].coords.x + boundary.min.x;
        pacmans[i].offset_coords.y = pacmans[i].coords.y + boundary.min.y;        
        pacmans[i].direction = data->players[i].start_direction;
        pacmans[i].score = 0;
        pacmans[i].head = 'o';
    }
}

int main(int argc, char **argv) {
    srand(time(NULL));
	struct sockaddr_in addr; 
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(PORT); 
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(addr.sin_zero, '\0', sizeof addr.sin_zero);
    int opt;
    while ((opt = getopt(argc, argv, "hscp:k:a:n:")) != -1) {
	    switch (opt) {
	        case 'h':	printf("Help:\n");
			        	printf("\t-h prints this message\n");
				        printf("\t-s for server\n");
			    	    printf("\t-c for client\n");
    				    printf("\t-p <PORT>");
	    			    printf("\t-k <number of players>\n");
		    		    printf("\t-a <address>\n"); break;
            case 's': is_server = 1; break;
	        case 'c': is_server = 0; break;
	        case 'p': addr.sin_port = atoi(optarg); break; 
	        case 'k': pl_count = atoi(optarg); break;
	        case 'a': addr.sin_addr.s_addr = inet_addr(optarg); 
                break;
            case 'n': name = optarg;
	    }
    }
    pthread_t registration;
    switch (is_server) {
        case TRUE:  // server
            pacmans[0].name = name;
            pacmans[0].namelen = strlen(name) + 1;
            init();
            genarr1(arr1);
            gen_map(arr1,arr2,arr3,arr4,map);     
            pthread_create(&registration, NULL, server_handler, &addr);
            break;
        case FALSE:  // client
            init();
            Info *ptr = client_connect(addr);
            pack_playerdata(ptr);
            break;
        case -1:
            printf("Error\n");
            return 1;
    }

    while (TRUE) {
        refresh();
        print_map(map);
            for (size_t i = 0; i < 4; ++i) {
                mvprintw(1+i, 0, "pacmans[%ld].namelen %d", i, \
                pacmans[i].namelen);
                mvprintw(6+i, 0, "pacmans[%ld].sd %d", i, \
                pacmans[i].sd);
                mvprintw(11+i, 0, "pacmans[%ld].name %s", i, \
                pacmans[i].name);
                mvprintw(16+i, 0, "pacmans[%ld].dir %d", i, \
                pacmans[i].direction);
            }
        mvprintw(21, 0, "waiting ... %d/%d", cnt, pl_count);
        mvprintw(22, 0, "ready clients %d", ready_clients);        
    }

    endwin();
    return 0;
}