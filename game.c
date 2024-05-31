//todo: 
//network bytorder
//players should stuch on collision
//propper game ending
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
#define FOOD                0xaa
#define WALL                0xff
#define PLAYER              0x22

int self_id;                      //client self identification
uint32_t fps;
uint8_t temp_key;
int pl_count, score;
char *name;                     //temporary storage for names -n
char *name_checker;
int is_server = -1;                         
int sd, cd, maxfd, ret;                           
int ready_clients;
bool done = false;
uint32_t pollhit;
char arr1[lheight][lwidth] = {0};
char arr2[lheight][lwidth] = {0};
char arr3[lheight][lwidth] = {0};
char arr4[lheight][lwidth] = {0};
char map[height][width] = {0};
chtype wall = '#';
chtype food = '.';
chtype empt = ' ';
chtype head = 'o';

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
Pacman pacmans[MAX_PACMAN_COUNT] = {0};

void find_player_location(char arr[lheight][lwidth]) {
    while (pacmans[0].coords.x == 0) {
        int rand_height = rand()%14;
        int rand_width = rand()%19;
        if (arr[rand_height][rand_width] == 0xffffffaa) {
            arr[rand_height][rand_width] = PLAYER;
            pacmans[0].coords.x = rand_width;
            pacmans[0].coords.y = rand_height;
            break;
        } else {
            continue;
        }
    }
}
static void genarr1(char arr[lheight][lwidth]) {
    for (int i = 0; i < lheight; i++) {
        for (int j = 0; j < lwidth; j++) {
            arr[i][j] = FOOD;
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
                arr[k][fix_rnd_x] = WALL;
            for (int m = start_rnd_x; m < end_rnd_x; m++) 
                arr[fix_rnd_y][m] = WALL;
        }   
    }
    find_player_location(arr);

}

void gen_map(char arr1[lheight][lwidth],\
 char arr2[lheight][lwidth], char arr3[lheight][lwidth],\
  char arr4[lheight][lwidth], char map[height][width]) {
    
    for (int i = 0; i < lheight; i++) {
        for (int j = 0; j < lwidth; j++) {
            if (arr1[i][j] == 0xffffffaa || arr1[i][j] == 0xffffff22) {
                arr1[i][j] = '.';
            } else if (arr1[i][j] == 0xffffffff) {
                arr1[i][j] = '#';
            }
        }
    }

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
        map[height/2 + i][width/2 + j] = arr4[i][j];
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
        default:
            return key; 
    }
}

void set_dir(uint8_t key, size_t i) {
        if (key == UP) {
            pacmans[i].direction = key;
        } else if (key == DOWN) {
            pacmans[i].direction = key;
        } else if (key == LEFT) {
            pacmans[i].direction = key;
        } else if (key == RIGHT) {
            pacmans[i].direction = key;
        } else if (key == QUIT) {
            pacmans[i].direction = key;
        }
}

void *server_input(void *none) {
    while (TRUE) { 
        temp_key = getch();
        if (transform_key(temp_key) != pacmans[0].direction && \
        (transform_key(temp_key) == UP || transform_key(temp_key) == DOWN || \
        transform_key(temp_key) == RIGHT || transform_key(temp_key) == LEFT)) {

            set_dir(transform_key(temp_key), 0);
            for (size_t i = 1; i < pl_count; ++i) {
                Package key;
                key.magic = mgc;
                key.ptype = 0x00;
                key.datasize = 1;                
                write(pacmans[i].sd, &key, sizeof(key));
                write(pacmans[i].sd, &pacmans[0].direction, 1);                
            }
        }     
    }
}

void *client_input(void *none) {
    while (TRUE) { 
        temp_key = getch();
        if (transform_key(temp_key) != pacmans[self_id].direction && \
        (transform_key(temp_key) == UP || transform_key(temp_key) == DOWN || \
        transform_key(temp_key) == RIGHT || transform_key(temp_key) == LEFT)) {
            Package key;
            set_dir(transform_key(temp_key), self_id);            
            key.magic = mgc;
            key.ptype = 0x00;
            key.datasize = 1;
            write(sd, &key, sizeof(key));
            write(sd, &pacmans[self_id].direction, 1);
        }     
    }
}

void send_all_with_exception(uint8_t key, int exception) {
    uint32_t namelen = pacmans[exception].namelen;
    char *sender_name = pacmans[exception].name;
    Package send_key;
    send_key.magic = mgc;
    send_key.ptype = 0xffffffff;
    send_key.datasize = 1;
    for (size_t i = 1; i < pl_count; ++i) {
        if (i != exception) {
            write(pacmans[i].sd, &send_key, sizeof(send_key));
            write(pacmans[i].sd, &key, 1);
            Package cli_name;
            cli_name.magic = mgc;
            cli_name.ptype = 0xffffffff;
            cli_name.datasize = namelen;
            write(pacmans[i].sd, &cli_name, sizeof(cli_name));
            write(pacmans[i].sd, sender_name, namelen);
        }
    }
}

void *broadcast(void *none) {                      //server
    struct pollfd fds[MAX_PACMAN_COUNT] = {0};
    for (size_t i = 1; i < pl_count; ++i) {
        fds[i].fd = pacmans[i].sd;
        fds[i].events = POLLIN;
    }
    //pl_count includes server
    while (TRUE) {
        ret = poll((struct pollfd *)&fds, pl_count, -1);  
        if (ret != -1 && ret != 0) {
            for (size_t i = 1; i < pl_count; ++i) {
                if (fds[i].revents & POLLIN) {
                    fds[i].revents = 0;
                    pollhit++;
                    Package cli_key;
                    read(pacmans[i].sd, &cli_key, sizeof(cli_key));
                    if (cli_key.magic == mgc && cli_key.ptype == 0x00) {
                        uint8_t key;
                        read(pacmans[i].sd, &key, 1);
                        pacmans[i].direction = key;
                        send_all_with_exception(key, i);                 
                    }
                } 
            }
        }        
    }     
}

void *socket_keys(void *none) {
    while (TRUE) {
        Package serv_key;
        uint8_t key;
        read(sd, &serv_key, sizeof(serv_key));
        if (serv_key.magic == mgc \
        && serv_key.ptype == 0x00) {        
            read(sd, &key, 1);
            pacmans[0].direction = key;
        } else if (serv_key.magic == mgc \
        && serv_key.ptype == 0xffffffff) { 
            read(sd, &key, 1);
            Package name;
            read(sd, &name, sizeof(name));
            if (name.magic == mgc && name.ptype == 0xffffffff) {
                char temp_name[name.datasize];                //im here 28.05
                read(sd, &temp_name, name.datasize);
                for (size_t i = 1; i < pl_count; ++i) {
                    if (strcmp(temp_name, pacmans[i].name) == 0) {
                        pacmans[i].direction = key;
                        break;
                    } else {
                        continue;
                    }
                }
            }
        }
    }
}

void assign_coords() {
    switch (ready_clients) {
        case 3:
            pacmans[1].coords.x = width - pacmans[0].coords.x;
            pacmans[1].coords.y = pacmans[0].coords.y;
            pacmans[2].coords.x = pacmans[0].coords.x;
            pacmans[2].coords.y = height - pacmans[0].coords.y;
            pacmans[3].coords.x = width - pacmans[0].coords.x;
            pacmans[3].coords.y = height - pacmans[0].coords.y;
            break;
        case 2:
            pacmans[1].coords.x = width - pacmans[0].coords.x;
            pacmans[1].coords.y = pacmans[0].coords.y;
            pacmans[2].coords.x = pacmans[0].coords.x;
            pacmans[2].coords.y = height - pacmans[0].coords.y;     
            break;
        case 1:
            pacmans[1].coords.x = width - pacmans[0].coords.x;
            pacmans[1].coords.y = pacmans[0].coords.y;
            break;
    }    
}

void *server_handler(void *address) {
    struct sockaddr_in *addr = address;
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
        mvprintw(0,0,"Waiting players...");
    }

    int cnt = 1;
    while (TRUE) {
        if (cnt == pl_count) break;

        if ((cd = accept(sd, NULL, NULL)) > 0) {
            pacmans[cnt].sd = cd;
            Package connect;
            read(cd, &connect, sizeof(connect));
            if (connect.magic == mgc && connect.datasize < PLAYERNAME_LEN) {
                char tmpname[connect.datasize];
                pacmans[cnt].namelen = connect.datasize;
                read(cd, &tmpname, connect.datasize);
                pacmans[cnt].name = malloc(connect.datasize);
                strncpy(pacmans[cnt].name, tmpname, connect.datasize);

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
            }  
        }           
    }
    
    Info *ptr = malloc(sizeof(Info));
    fps = FPS_TIMEOUT;
	ptr->frame_timeout = FPS_TIMEOUT;
	ptr->pl_count = pl_count;
	ptr->players = malloc(sizeof(Player) * ptr->pl_count);

    assign_coords();
    for (size_t i = 0; i < pl_count; ++i) {
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
    return EXIT_SUCCESS;
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

Info *client_connect(struct sockaddr_in addr) {
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sd, (const struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
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

bool check_food() {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (map[i][j] == '.') {
                return FALSE;
            }
        }
    }
    return TRUE;
}

void move_player() {     
    for (size_t i = 0; i < pl_count; ++i) {
        mvaddch(pacmans[i].offset_coords.y, \
        pacmans[i].offset_coords.x, pacmans[i].head);
        refresh();
        switch (pacmans[i].direction) {
        case DOWN:
            if (map[pacmans[i].coords.y + 1][pacmans[i].coords.x] == '.' && pacmans[i].coords.y + 1 < height) {
                map[pacmans[i].coords.y + 1][pacmans[i].coords.x] = 'o';
                pacmans[i].coords.y++;
                pacmans[i].offset_coords.y++;
                pacmans[i].score++;
                map[pacmans[i].coords.y][pacmans[i].coords.x] = ' ';
            } else if (map[pacmans[i].coords.y + 1][pacmans[i].coords.x] == 'o' ||\
                map[pacmans[i].coords.y + 1][pacmans[i].coords.x] == '#' ||\
                (map[pacmans[i].coords.y + 1][pacmans[i].coords.x] != '#' \
                && pacmans[i].coords.y + 1 == height)) {
                pacmans[i].coords.y = pacmans[i].coords.y;
            } else {
                //map[pacmans[i].coords.y + 1][pacmans[i].coords.x] = 'o';
                pacmans[i].coords.y++;
                pacmans[i].offset_coords.y++;
            }    
            break;
        case UP: 
            if (map[pacmans[i].coords.y - 1][pacmans[i].coords.x] == '.' \
                && pacmans[i].coords.y - 1 >= 0) {
                map[pacmans[i].coords.y - 1][pacmans[i].coords.x] = 'o';
                pacmans[i].coords.y--;
                pacmans[i].offset_coords.y--;
                pacmans[i].score++;
                map[pacmans[i].coords.y][pacmans[i].coords.x] = ' ';
            } else if (map[pacmans[i].coords.y - 1][pacmans[i].coords.x] == 'o' ||\
                map[pacmans[i].coords.y - 1][pacmans[i].coords.x] == '#' ||\
                (map[pacmans[i].coords.y - 1][pacmans[i].coords.x] != '#' \
                && pacmans[i].coords.y - 1 == -1)) {
                pacmans[i].coords.y = pacmans[i].coords.y;
            } else {
                //map[pacmans[i].coords.y - 1][pacmans[i].coords.x] = 'o';
                pacmans[i].coords.y--;
                pacmans[i].offset_coords.y--;
            }
            break;
        case LEFT:
            if (map[pacmans[i].coords.y][pacmans[i].coords.x - 1] == '.' \
                && pacmans[i].coords.x - 1 >= 0) {
                map[pacmans[i].coords.y][pacmans[i].coords.x - 1] = 'o';
                pacmans[i].coords.x--;
                pacmans[i].offset_coords.x--;
                pacmans[i].score++;
                map[pacmans[i].coords.y][pacmans[i].coords.x] = ' ';
            } else if (map[pacmans[i].coords.y][pacmans[i].coords.x - 1] == 'o' ||\
                map[pacmans[i].coords.y][pacmans[i].coords.x - 1] == '#' ||\
                (map[pacmans[i].coords.y][pacmans[i].coords.x - 1] != '#' && \
                pacmans[i].coords.x - 1 == -1)) {
                pacmans[i].coords.x = pacmans[i].coords.x;
            } else {
                //map[pacmans[i].coords.y][pacmans[i].coords.x - 1] = 'o';
                pacmans[i].coords.x--;
                pacmans[i].offset_coords.x--;
            }
            break;
        case RIGHT:
            if (map[pacmans[i].coords.y][pacmans[i].coords.x + 1] == '.' \
                && pacmans[i].coords.x + 1 < width) {
                map[pacmans[i].coords.y][pacmans[i].coords.x + 1] = 'o';
                pacmans[i].coords.x++;
                pacmans[i].offset_coords.x++;
                pacmans[i].score++;
                map[pacmans[i].coords.y][pacmans[i].coords.x] = ' ';
            } else if (map[pacmans[i].coords.y][pacmans[i].coords.x + 1] == 'o' ||\
                map[pacmans[i].coords.y][pacmans[i].coords.x + 1] == '#' ||\
                (map[pacmans[i].coords.y][pacmans[i].coords.x + 1] != '#' \
                && pacmans[i].coords.x + 1 == width)) {
                pacmans[i].coords.x = pacmans[i].coords.x;
            } else {
                //map[pacmans[i].coords.y][pacmans[i].coords.x + 1] = 'o';
                pacmans[i].coords.x++;
                pacmans[i].offset_coords.x++;
                }
            break;         
        case QUIT:
            done = TRUE;
            break;  
        }
    }
    done = check_food();
    usleep(fps);
}

void print_map(char arr[height][width]) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (arr[i][j] == '.') {
                mvaddch(boundary.min.y + i, \
                boundary.min.x + j, food);
            } else if (arr[i][j] == ' ') {
                mvaddch(boundary.min.y + i, \
                boundary.min.x + j, empt);
            } else if (arr[i][j] == '#') {
                mvaddch(boundary.min.y + i, \
                boundary.min.x + j, wall);
            } else if (arr[i][j] == 'o') {
                mvaddch(boundary.min.y + i, \
                boundary.min.x + j, head);   
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
        pacmans[i].namelen = data->players[i].player_name_len;
        pacmans[i].name = malloc(pacmans[i].namelen);
        pacmans[i].name = (char *)data->players[i].player_name;    
        if (strcmp(name, pacmans[i].name) == 0) self_id = i;
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
    pthread_t netthread;
    pthread_t keyboard;
    switch (is_server) {
        case TRUE:  // server
            pacmans[0].name = name;
            pacmans[0].namelen = strlen(name) + 1;
            init();
            genarr1(arr1);     
            pthread_create(&registration, NULL, server_handler, &addr);
            pthread_join(registration, NULL);
            gen_map(arr1,arr2,arr3,arr4,map);

            pthread_create(&keyboard, NULL, server_input, NULL);
            pthread_create(&netthread, NULL, broadcast, NULL);
            break;
        case FALSE:  // client
            init();
            Info *ptr = client_connect(addr);
            pack_playerdata(ptr);

            pthread_create(&keyboard, NULL, client_input, NULL);
            pthread_create(&netthread, NULL, socket_keys, NULL);
            break;
        case -1:
            printf("Error\n");
            return EXIT_SUCCESS;
    }

    while (TRUE) {
        print_map(map);
        for (int i = 0, offset = 1; i < pl_count; i++) {
            mvprintw(offset++, 0, "%s score: %d", pacmans[i].name, pacmans[i].score);
        }
        if (done == FALSE) {
            move_player();          
        } else {
            break;
        }
    }

    endwin();
    return EXIT_SUCCESS;
}