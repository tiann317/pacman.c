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

#define MAX_PACMAN_COUNT    4
#define PLAYERNAME_LEN      256
#define PORT                8080
#define FPS_TIMEOUT         300000
#define height              30
#define width               40
#define lheight             15
#define lwidth              20

int pl_count, score;
int is_server = -1;                         
int sd, cd;                           
uint32_t fps;                  //global scope for client 
//bool FoodExists = TRUE;      
bool done = FALSE;
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

typedef struct package {
	uint32_t magic;
	uint32_t ptype;
	uint32_t datasize;
} __attribute__((packed)) Package;

typedef struct {
    uint8_t direction;
    char *player_name;
} __attribute__((packed)) Id;

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    Point max;
    Point min;
} Boundary;

typedef struct {
    char *name;
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
Pacman pacmans[MAX_PACMAN_COUNT] = {0};  
// pacman at index 0 in server mode - my pacman

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

void expand_map(char *arr) {
    int k = 0;
    for (int i = 0; i < lheight; i++) {
        for (int j  = 0; j < lwidth; j++) {
            arr1[i][j] = arr[k++];
        }
    }
    gen_map(arr1,arr2,arr3,arr4,map);
}


Info* client_connect(struct sockaddr_in addr) {
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sd, &addr, sizeof(struct sockaddr_in)) < 0) {
		perror("server fd");
		close(sd);
		exit(1);
    }

    Package connect;
	connect.magic = 0xabcdfe01;
    connect.ptype = 0x01;
    connect.datasize = strlen(pacmans[0].name) + 1;
	write(sd, &connect, sizeof(connect));
	write(sd, pacmans[0].name, connect.datasize);

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
    return p;
}


/*set_pacman_direction(char *name, uint8_t dir, Info *initial_info) {
    for (int i = 0; i < pl_count; i++) {
        if (strcmp(name, initial_info->players[i].player_name) == 0) {
            pacman[i].name = initial_info->players[i].player_name;
            pacman[i].direction = dir;
        }
    }
}
*/

Info* server_handler(struct sockaddr_in *addr) {
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
		mvprintw(0, 0, "Server listening...\n");
	}

	cd = accept(sd, NULL, NULL);
	if (cd < 0) {
		perror("accept");
		close(sd);
		exit(1);
	}

    Package connect;
	read(cd, &connect, sizeof(connect));
	if (connect.datasize > PLAYERNAME_LEN)
        close(cd);
    char name[PLAYERNAME_LEN];
	read(cd, &name, connect.datasize);
    if (strcmp(pacmans[0].name, name) == 0)
        close(cd);
    pacmans[1].name = name;

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

	ptr->frame_timeout = FPS_TIMEOUT;
	ptr->pl_count = pl_count;

	ptr->players = malloc(sizeof(Player) * ptr->pl_count);
	if (ptr->players == NULL) {
		perror("malloc 2");
		exit(2);
	}

    for (size_t i = 0; i < pl_count; ++i) {
        uint32_t namelen = strlen(pacmans[i].name) + 1;
        ptr->players[i].player_name_len = namelen;
        ptr->players[i].player_name = malloc(namelen);
	    memcpy(ptr->players[i].player_name, pacmans[i].name, namelen);            
    }
    
	for (size_t i = 0; i < pl_count; i++) {
	    ptr->players[i].start_direction = 1;        
        ptr->players[i].start_x = rand()%10;    
        ptr->players[i].start_y = rand()%15;
    }
        // positions for clients should be 
        // located symetrically to the servers position
        // player[0] == server; player[1] == 1st client connected, etc 

    Package sg_info;                            //sg == start game package
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
		write(cd, ptr->players[i].player_name, \
        ptr->players[i].player_name_len);
	}
    return ptr;
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

void server_input() {
    while (TRUE) {
        pacmans[0].direction = getch();
        pacmans[0].direction = transform_key(pacmans[0].direction);
        write(cd, &pacmans[0].direction, 1);
    }
}

void client_input() {
    while (TRUE) {
        pacmans[0].direction = getch();
        pacmans[0].direction = transform_key(pacmans[0].direction);
        write(sd, &pacmans[0].direction, 1);
    }
}

void client_keys() {
    while (TRUE) {
        read(cd, &pacmans[1].direction, 1);
    }
}

void server_keys() {
    while (TRUE) {
        read(sd, &pacmans[1].direction, 1);
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
}

static void print_map(char arr[height][width]) {
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


/*bool check_food() {
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
    }
}*/

void move_player() {
    for (size_t i = 0; i < pl_count; ++i) {
        mvaddch(pacmans[i].offset_coords.y, \
        pacmans[i].offset_coords.x, pacmans[i].head);
        refresh();
//        mvaddch(pacmans[i].offset_coords.y, \
//        pacmans[i].offset_coords.x, empt);
        switch (pacmans[i].direction) {
        case DOWN:
            pacmans[i].head = '^';
            if (map[pacmans[i].coords.y + 1][pacmans[i].coords.x] == '.' \
                && pacmans[i].coords.y + 1 < height) {
                pacmans[i].coords.y++;
                pacmans[i].offset_coords.y++;
                pacmans[i].score++;
                map[pacmans[i].coords.y][pacmans[i].coords.x] = ' ';
            } else if (map[pacmans[i].coords.y + 1][pacmans[i].coords.x] == '#' ||\
                (map[pacmans[i].coords.y + 1][pacmans[i].coords.x] != '#' \
                && pacmans[i].coords.y + 1 == height)) {
                pacmans[i].coords.y = pacmans[i].coords.y;
            } else {
                pacmans[i].coords.y++;
                pacmans[i].offset_coords.y++;
            }    
            break;
        case UP: 
            pacmans[i].head = 'v';
            if (map[pacmans[i].coords.y - 1][pacmans[i].coords.x] == '.' \
                && pacmans[i].coords.y - 1 >= 0) {
                pacmans[i].coords.y--;
                pacmans[i].offset_coords.y--;
                pacmans[i].score++;
                map[pacmans[i].coords.y][pacmans[i].coords.x] = ' ';
            } else if (map[pacmans[i].coords.y - 1][pacmans[i].coords.x] == '#' ||\
                (map[pacmans[i].coords.y - 1][pacmans[i].coords.x] != '#' \
                && pacmans[i].coords.y - 1 == -1)) {
                pacmans[i].coords.y = pacmans[i].coords.y;
            } else {
                pacmans[i].coords.y--;
                pacmans[i].offset_coords.y--;
            }
            break;
        case LEFT:
            pacmans[i].head = '>';
            if (map[pacmans[i].coords.y][pacmans[i].coords.x - 1] == '.' \
                && pacmans[i].coords.x - 1 >= 0) {
                pacmans[i].coords.x--;
                pacmans[i].offset_coords.x--;
                pacmans[i].score++;
                map[pacmans[i].coords.y][pacmans[i].coords.x] = ' ';
            } else if (map[pacmans[i].coords.y][pacmans[i].coords.x - 1] == '#' ||\
                (map[pacmans[i].coords.y][pacmans[i].coords.x - 1] != '#' && \
                pacmans[i].coords.x - 1 == -1)) {
                pacmans[i].coords.x = pacmans[i].coords.x;
            } else {
                pacmans[i].coords.x--;
                pacmans[i].offset_coords.x--;
            }
            break;
        case RIGHT:
            pacmans[i].head = '<';
            if (map[pacmans[i].coords.y][pacmans[i].coords.x + 1] == '.' \
                && pacmans[i].coords.x + 1 < width) {
                pacmans[i].coords.x++;
                pacmans[i].offset_coords.x++;
                pacmans[i].score++;
                map[pacmans[i].coords.y][pacmans[i].coords.x] = ' ';
            } else if (map[pacmans[i].coords.y][pacmans[i].coords.x + 1] == '#' ||\
                (map[pacmans[i].coords.y][pacmans[i].coords.x + 1] != '#' \
                && pacmans[i].coords.x + 1 == width)) {
                pacmans[i].coords.x = pacmans[i].coords.x;
            } else {
                pacmans[i].coords.x++;
                pacmans[i].offset_coords.x++;
                }
            break;         
        case QUIT:
            done = TRUE;
            break;  
        }
    }
    usleep(fps);        
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
        pacmans[i].sd = sd;
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
	        case 'a': addr.sin_addr.s_addr = inet_addr(optarg); break;
            case 'n': pacmans[0].name = optarg;
	    }
    }
    pthread_t net_thread;
    pthread_t keyboard_thread;
    switch (is_server) {
        case TRUE:  // server
            init();
            genarr1(arr1);
            gen_map(arr1,arr2,arr3,arr4,map);
            Info *p = server_handler(&addr);
            pack_playerdata(p);
            pthread_create(&keyboard_thread, NULL, server_input, NULL);
            pthread_create(&net_thread, NULL, client_keys, NULL);
            break;
        case FALSE:  // client
            init();
            Info *ptr = client_connect(addr);
            pack_playerdata(ptr);
            pthread_create(&keyboard_thread, NULL, client_input, NULL);
            pthread_create(&net_thread, NULL, server_keys, NULL);
            break;
        case -1:
            printf("Error\n");
            return 1;
    }

    while (TRUE) {
        print_map(map);
        if (done == FALSE) {
            move_player();          
        } else {
            break;
        }
    }
    endwin();
    return 0;
}