//TODO: fix the new map representation with values

#include <ncurses.h>
#include <pthread.h>
#include <unistd.h> 
#include <getopt.h>
#include <stdlib.h> 
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>

#define right   'd'
#define left    'a'
#define up      'w'
#define down    's'
#define quit    'q'

#define PLAYERNAME_LEN  256
#define PORT            8080
#define FPS_TIMEOUT     70000
#define height          30
#define width           40
#define lheight         15
#define lwidth          820
//#define   Wall        0xff
//#define   Food        0xaa
//#define   Player      0x22

chtype pac_head = 'o';
chtype wall = '#';
chtype food = '.';
chtype empt = ' ';

typedef struct point {
    int x;
    int y;
} Point;

typedef struct rect {
    struct point max;
    struct point min;
} Boundary;

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

Point loghead;
Point dsplhead;         //displayble head is the logical head with offsets
Boundary fullscr;
Boundary boundary;
Boundary quarter;

int pl_count;
int is_server = -1; //server flag
int sd = -1;        //server fd in client mode
uint8_t dir8t;
unsigned int score = 0;
int dir_x;
char *name;   
bool FoodExists = TRUE;      
char arr1[lheight][lwidth];
char arr2[lheight][lwidth];
char arr3[lheight][lwidth];
char arr4[lheight][lwidth];
char map[height][width];

static void gen_init_pos(char arr[lheight][lwidth]) {
    for (int i = 0; i < lheight; i++) {
        for (int j = 0; j < lwidth; j++) {
            if (arr[i][j] == 0) {
                loghead.x = j;
                loghead.y = i;
                break;
            }
        } break;
    }
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
                    if (arr[k][m] == 1 || arr[k][m] == 3)
                    wall_counter++;
                }
            }
            while (wall_counter > 1) {
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

    gen_init_pos(arr);
    for (int i = 0; i < lheight; i++) {
        for (int j = 0; j < lwidth; j++) {
            if (arr[i][j] == 0) {
                arr[i][j] = '.';
            } else if (arr[i][j] == 2) {
                arr[i][j] = 'o';
            } else {
                arr[i][j] = '#';                
            }
        }
    }
}

static void logic_quarter(char arr[height][width]) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (arr[i][j] == '.') {
                mvaddch(boundary.min.y + i, boundary.min.x + j, food);
            } else if (arr[i][j] == ' ') {
                mvaddch(boundary.min.y + i, boundary.min.x + j, empt);
            } else {
                mvaddch(boundary.min.y + i, boundary.min.x + j, wall);
            }   
        }
        mvaddch(i, 0, empt);
    }
}

void expand_map(char *arr) {
    int k = 0;
    for (int i = 0; i < lheight; i++) {
        for (int j  = 0; j < lwidth; j++) {
            map[i][j] = arr[k++];
        }
    }
}

void client_connect(struct sockaddr_in addr) {
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sd, &addr, sizeof(struct sockaddr_in)) < 0) {
		perror("server fd");
		close(sd);
		exit(1);
    }
    Package connect;
	connect.magic = 0xabcdfe01;
    connect.ptype = 0x01;
    connect.datasize = strlen(name)+1;
	write(sd, &connect, sizeof(connect));
	write(sd, name, connect.datasize);

    Package getmap;
	read(sd, &getmap, sizeof(getmap));
	char one_dim_map[getmap.datasize];
	read(sd, &one_dim_map, getmap.datasize);
    expand_map(one_dim_map);

    Package client_ready;
    client_ready.magic = 0xabcdfe01;
    client_ready.ptype = 0x02;
    client_ready.datasize = 0;
	write(sd, &client_ready, sizeof(client_ready));

	Info *p = malloc(sizeof(Info));
    Package sg_info_cl;
    if (!read(sd, &sg_info_cl, sizeof(sg_info_cl))) {
    	perror("read struct info");
    	exit(1);
	}
	if (!read(sd, p, sg_info_cl.datasize)) {
    	perror("read Info struct");
    	exit(1);
	}
	p->players = malloc(sizeof(Player) * p->pl_count);

    Package sg_players;
	read(sd, &sg_players, sizeof(sg_players));
	read(sd, p->players, sg_players.datasize);
	
	for (size_t i = 0; i < p->pl_count; i++) {
		p->players[i].player_name = malloc(p->players[i].player_name_len);
		read(sd, p->players[i].player_name, p->players[i].player_name_len);		
	}
}
//in progress
void *client_handler();

void *server_handler(struct sockaddr_in *addr) {
    int sd, cd;
    sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
		perror("Socket");
		close(sd);
		exit(1);
	}

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
		printf("Server listening...\n");
	}

	cd = accept(sd, NULL, NULL);
	if (cd < 0) {
		perror("accept");
		close(sd);
		exit(1);
	}
    Package connect;
	read(cd, &connect, sizeof(connect));
	char name1[PLAYERNAME_LEN];
	read(cd, &name1, connect.datasize);
    if (strcmp(name, name1) == 0) {
        close(cd);
    }
    Package sendmap;
	sendmap.magic = 0xabcdfe01;
	sendmap.ptype = 0x10;
	sendmap.datasize = sizeof(arr1);

	write(cd, &sendmap, sizeof(sendmap));
	write(cd, &arr1, sendmap.datasize);
    Package client_ready;
	read(cd, &client_ready, sizeof(client_ready));

	Info *ptr = malloc(sizeof(Info));
	if (ptr == NULL) {
		perror("malloc 1");
		exit(1);
	}

	ptr->frame_timeout = 30000;
	ptr->pl_count = pl_count;

	ptr->players = malloc(sizeof(Player) * ptr->pl_count);
	if (ptr->players == NULL) {
		perror("malloc 2");
		exit(2);
	}
//  by this time server should already posses all the playernames and their fd's
	ptr->players[0].player_name_len = strlen(name) + 1;
	ptr->players[1].player_name_len = strlen(name1) + 1;
//	ptr->players[2].player_name_len = strlen(playernameV) + 1;
//	ptr->players[3].player_name_len = strlen(playernameD) + 1;

	ptr->players[0].player_name = malloc(strlen(name) + 1);
	ptr->players[1].player_name = malloc(strlen(name1) + 1);
//	ptr->players[2].player_name = malloc(strlen(playernameV) + 1);
//	ptr->players[3].player_name = malloc(strlen(playernameD) + 1);
	
	for (int i = 0; i < pl_count; i++) {
		if (ptr->players[i].player_name == NULL) {
			perror("malloc i");
			exit(-1);
		}
	}

	memcpy(ptr->players[0].player_name, name, strlen(name)+1);
	memcpy(ptr->players[1].player_name, name1, strlen(name1)+1);
//	memcpy(ptr->players[2].player_name, playernameV, strlen(playernameV)+1);
//	memcpy(ptr->players[3].player_name, playernameD, strlen(playernameD)+1);

	for (int i = 0; i < pl_count; i++)
	ptr->players[i].start_direction = 1;
	for (int i = 0; i < pl_count; i++)
	ptr->players[i].start_x = rand()%10;
	for (int i = 0; i < pl_count; i++)
	ptr->players[i].start_y = rand()%15;

    Package sg_info;    //sg == start game package
	sg_info.magic = 0xabcdfe01;
	sg_info.ptype = 0x20;
	sg_info.datasize = sizeof(Info);
	write(cd, &sg_info, sizeof(sg_info));
	write(cd, ptr, sg_info.datasize);

    Package sg_players;
	sg_players.magic = 0xabcdfe01;
	sg_players.ptype = 0x20;
	sg_players.datasize = sizeof(Player) * pl_count;	

	write(cd, &sg_players, sizeof(sg_players));
	write(cd, ptr->players, sg_players.datasize);
	
	for (int i = 0; i < pl_count; i++) {
		write(cd, ptr->players[i].player_name, ptr->players[i].player_name_len);
	}
}

void *input() {
    for (;;) {
        int temp = getch();
        if (temp == quit) {
            dir_x = temp;
        } else if (temp == up || temp == KEY_UP) {
            dir8t = 0;
            dir_x = temp;
        } else if (temp == down || temp == KEY_DOWN) {
            dir8t = 2;
            dir_x = temp;
        } else if (temp == right || temp == KEY_RIGHT) {
            dir8t = 1;
            dir_x = temp;
        } else if (temp == left || temp == KEY_LEFT) {
            dir8t = 3;
            dir_x = temp;
        }
//      recv() dir_y;
    }  
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

void init() {
    initscr();
    noecho();
    keypad(stdscr, TRUE);
    getmaxyx(stdscr, fullscr.max.y, fullscr.max.x); 

    boundary.max.x = fullscr.max.x/2 + 20;
    boundary.min.x = fullscr.max.x/2 - 20;
    boundary.max.y = fullscr.max.y/2 + 15;
    boundary.min.y = fullscr.max.y/2 - 15;

    quarter.min.x = boundary.min.x;
    quarter.min.y = boundary.min.y; 
    quarter.max.y = boundary.min.y + 15;
    quarter.max.x = boundary.min.x + 20;
}

bool check_food() {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (map[i][j] == '.') {
                FoodExists = TRUE;
                return TRUE;
            }
        }
    }
    FoodExists = FALSE;
    return FALSE;
}

void game_ends() {
    check_food();
    if (FoodExists == FALSE) {
        clear();
        refresh();
        if (mvprintw(fullscr.max.y/2, fullscr.max.x/2, "Game Over") == ERR) {
            exit(ERR);
        }
        if (mvprintw(fullscr.max.y/2 + 1, fullscr.max.x/2, "Score: %d", score) == ERR) {
            exit(ERR);
        }
        if (mvprintw(fullscr.max.y/2 + 2, fullscr.max.x/2 - 4, "Press 'q' to leave") == ERR) {
            exit(ERR);
        }
        for(;;) {
            if (dir_x == quit) {
                endwin();
                exit(1);
            } else {
                continue;
            }
        }
    }
}

void render() {
    dsplhead.x = loghead.x + quarter.min.x;
    dsplhead.y = loghead.y + quarter.min.y;
    bool done = FALSE;
    while(TRUE) {
        logic_quarter(map);
        if (mvprintw(0, 0, "Score: %d", dir8t) == ERR)
            exit(ERR);
        mvaddch(dsplhead.y, dsplhead.x, pac_head);
        refresh();
        mvaddch(dsplhead.y, dsplhead.x, ' ');
        game_ends();
            switch (dir_x) {
            case down:
            case KEY_DOWN:
                pac_head = '^';
                if (map[loghead.y + 1][loghead.x] == '.' && loghead.y + 1 < height) {
                    loghead.y++;
                    dsplhead.y++;
                    score++;
                    map[loghead.y][loghead.x] = ' ';
                } else if (map[loghead.y + 1][loghead.x] == '#' ||\
                (map[loghead.y + 1][loghead.x] != '#' && loghead.y + 1 == height)) {
                    loghead.y = loghead.y;
                } else {
                    loghead.y++;
                    dsplhead.y++;
                }    
                break;
            case up:
            case KEY_UP: 
                pac_head = 'v';
                if (map[loghead.y - 1][loghead.x] == '.' && loghead.y - 1 >= 0) {
                    loghead.y--;
                    dsplhead.y--;
                    score++;
                    map[loghead.y][loghead.x] = ' ';
                } else if (map[loghead.y - 1][loghead.x] == '#' ||\
                (map[loghead.y - 1][loghead.x] != '#' && loghead.y - 1 == -1)) {
                    loghead.y = loghead.y;
                } else {
                    loghead.y--;
                    dsplhead.y--;
                }
                break;
            case left:
            case KEY_LEFT:
                pac_head = '>';
                if (map[loghead.y][loghead.x - 1] == '.' && loghead.x - 1 >= 0) {
                    loghead.x--;
                    dsplhead.x--;
                    score++;
                    map[loghead.y][loghead.x] = ' ';
                } else if (map[loghead.y][loghead.x - 1] == '#' ||\
                (map[loghead.y][loghead.x - 1] != '#' && loghead.x - 1 == -1)) {
                    loghead.x = loghead.x;
                } else {
                    loghead.x--;
                    dsplhead.x--;
                }
                break;
            case right:
            case KEY_RIGHT:
                pac_head = '<';
                if (map[loghead.y][loghead.x + 1] == '.' && loghead.x + 1 < width) {
                    loghead.x++;
                    dsplhead.x++;
                    score++;
                    map[loghead.y][loghead.x] = ' ';
                } else if (map[loghead.y][loghead.x + 1] == '#' ||\
                (map[loghead.y][loghead.x + 1] != '#' && loghead.x + 1 == width)) {
                    loghead.x = loghead.x;
                } else {
                    loghead.x++;
                    dsplhead.x++;
                }
                break;         
            case quit:
                done = TRUE;
                break;  
            }
        if (done) break;        
        usleep(FPS_TIMEOUT);
    }      
    endwin();
}

int main(int argc, char **argv) {
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
	        case 'a': addr.sin_addr.s_addr = inet_addr(optarg); break;
            case 'n': name = optarg;
	    }
    }
    pthread_t net_thread;
    switch (is_server) {
        case TRUE:  // server
            init();
            genarr1(arr1);
            gen_map(arr1,arr2,arr3,arr4,map);
            pthread_create(&net_thread, NULL, server_handler, &addr);
            break;
        case FALSE:  // client
            init();
            client_connect(addr);
            pthread_create(&net_thread, NULL, client_handler, NULL);
            break;
        case -1:
            printf("Error\n");
            return 1;
    }
    init();
    genarr1(arr1);
    gen_map(arr1, arr2, arr3, arr4, map);
    pthread_t keyboard_thread;
    pthread_create(&keyboard_thread, NULL, input, NULL);
    render();
    return 0;
}
