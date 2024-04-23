//TODO: fix the new map representation with values

#include <ncurses.h>
#include <pthread.h>
#include <unistd.h> 
#include <stdlib.h> 
#include <time.h>

#define right   'd'
#define left    'a'
#define up      'w'
#define down    's'
#define quit    'q'

#define FPS_TIMEOUT 70000
#define height      30
#define width       40
#define lheight     15
#define lwidth      20
#define Wall        0xff
#define	Food        0xaa
#define Player      0x22

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

Point loghead;
Point dsplhead;         //displayble head is the logical head but with offsets
Boundary fullscr;
Boundary boundary;
Boundary quarter;

uint8_t dir8t;
unsigned int score = 0;
int dir_x;
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

void *input(void *none) {
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

void game_ends() {
    bool FoodExists = FALSE;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (map[i][j] == '.') {
                FoodExists = TRUE;
                continue;
            }
        }
    }
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

int main(void) {
    init();
    genarr1(arr1);
    gen_map(arr1, arr2, arr3, arr4, map);
    dsplhead.x = loghead.x + quarter.min.x;
    dsplhead.y = loghead.y + quarter.min.y;
    pthread_t pid;
    void *retval;
    pthread_create(&pid, NULL, input, NULL);
    render();
    return 0;
}
