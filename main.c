//TODO:     implement random-ish wall generator for arr1[][]
//          2nd local player
//          server/client communication
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h> 
#include <stdlib.h> 
#include <time.h>

#define RIGHT 'd'
#define LEFT 'a'
#define UP 'w'
#define DOWN 's'
#define QUIT 'q'

#define FPS_TIMEOUT 70000
#define height 30
#define width 40
#define lheight 15
#define lwidth 20

chtype pac_head = 'o';
chtype wall = '#';
chtype food = '.';
chtype empt = ' ';

const int name_len = 256;
unsigned int score = 0;
int dir_x;
char arr1[lheight][lwidth] = {
    {'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    {'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    {'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    {'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    {'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    {'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    {'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    {'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    {'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    {'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    {'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    {'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    {'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
    {'#','#','#','#','#','#','#','#','#','#','.','#','#','#','#','#','#','#','#','#'},
};
char arr2[lheight][lwidth];
char arr3[lheight][lwidth];
char arr4[lheight][lwidth];

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
        if (temp == QUIT) {
            dir_x = temp;
        } else if (temp == UP || temp == KEY_UP) {
            dir_x = temp;
        } else if (temp == DOWN || temp == KEY_DOWN) {
            dir_x = temp;
        } else if (temp == RIGHT || temp == KEY_RIGHT) {
            dir_x = temp;
        } else if (temp == LEFT || temp == KEY_LEFT) {
            dir_x = temp;
        }
//      recv() dir_y;
    }  
}

int main(void) {
    initscr();
    noecho();
    keypad(stdscr, TRUE);
    pthread_t pid;
    void *retval;
    pthread_create(&pid, NULL, input, NULL);
    bool done = FALSE;
    getmaxyx(stdscr, fullscr.max.y, fullscr.max.x); 

    boundary.max.x = fullscr.max.x/2 + 20;
    boundary.min.x = fullscr.max.x/2 - 20;
    boundary.max.y = fullscr.max.y/2 + 15;
    boundary.min.y = fullscr.max.y/2 - 15;

    quarter.min.x = boundary.min.x;
    quarter.min.y = boundary.min.y; 
    quarter.max.y = boundary.min.y + 15;
    quarter.max.x = boundary.min.x + 20;

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

    char map[height][width];
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

    loghead.x = lwidth/2;
    loghead.y = lheight/2;
    dsplhead.x = loghead.x + quarter.min.x;
    dsplhead.y = loghead.y + quarter.min.y;
    
    while(TRUE) {
        logic_quarter(map);
        if (mvprintw(0, 0, "log heady: %d; logheadx: %d", loghead.y, loghead.x) == ERR)
            printw("Error! error %d", ERR);
        mvaddch(dsplhead.y, dsplhead.x, pac_head);
        refresh();
        mvaddch(dsplhead.y, dsplhead.x, ' ');
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
                printw("Error! error %d", ERR);
            }
            if (mvprintw(fullscr.max.y/2 + 1, fullscr.max.x/2, "Score: %d", score) == ERR) {
                printw("Error! error %d", ERR);
            }
            if (mvprintw(fullscr.max.y/2 + 2, fullscr.max.x/2 - 4, "Press 'q' to leave") == ERR) {
                printw("Error! error %d", ERR);
            }
                for(;;) {
                    if (dir_x == QUIT) {
                        endwin();
                        return 0;
                    } else {
                        continue;
                    }
                }
        }
            switch (dir_x) {
            case DOWN:
            case KEY_DOWN:
                pac_head = '^';
                if (map[loghead.y + 1][loghead.x] == '.') {
                    loghead.y++;
                    dsplhead.y++;
                    score++;
                    map[loghead.y][loghead.x] = ' ';
                } else if (map[loghead.y + 1][loghead.x] == '#') {
                    loghead.y = loghead.y;
                } else if (map[loghead.y][loghead.x + 1] == ' ') {
                    loghead.y++;
                    dsplhead.y++;
                }    
                break;
            case UP:
            case KEY_UP: 
                pac_head = 'v';
                if (map[loghead.y - 1][loghead.x] == '.') {
                    loghead.y--;
                    dsplhead.y--;
                    score++;
                    map[loghead.y][loghead.x] = ' ';
                } else if (map[loghead.y - 1][loghead.x] == '#') {
                    loghead.y = loghead.y;
                } else if (map[loghead.y][loghead.x + 1] == ' ') {
                    loghead.y--;
                    dsplhead.y--;
                }
                break;
            case LEFT:
            case KEY_LEFT:
                pac_head = '>';
                if (map[loghead.y][loghead.x - 1] == '.') {
                    loghead.x--;
                    dsplhead.x--;
                    score++;
                    map[loghead.y][loghead.x] = ' ';
                } else if (map[loghead.y][loghead.x - 1] == '#') {
                    loghead.x = loghead.x;
                } else if (map[loghead.y][loghead.x + 1] == ' ') {
                    loghead.x--;
                    dsplhead.x--;
                }
                break;
            case RIGHT:
            case KEY_RIGHT:
                pac_head = '<';
                if (map[loghead.y][loghead.x + 1] == '.') {
                    loghead.x++;
                    dsplhead.x++;
                    score++;
                    map[loghead.y][loghead.x] = ' ';
                } else if (map[loghead.y][loghead.x + 1] == '#') {
                    loghead.x = loghead.x;
                } else if (map[loghead.y][loghead.x + 1] == ' ') {
                    loghead.x++;
                    dsplhead.x++;
                }
                break;         
            case QUIT:
                done = TRUE;
                break;  
            }
        if (done) break;        
        usleep(FPS_TIMEOUT);
    }      
    endwin();
    return 0;
}
