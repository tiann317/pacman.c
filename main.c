//TODO: score counter
//TODO: fix eating dots after first 1-2 iterations
//TODO: server/client
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h> 
#include <stdlib.h> 
#include <time.h>
#define height 30
#define width 40
#define lheight 15
#define lwidth 20

chtype pac_head = 'o';
chtype wall = '#';
chtype food = '.';
chtype empt = ' ';

int dir_x;
char arr1[lheight][lwidth] = {
    {'#','#','#','#','#','#','#','#','#','#','#','#','#','#','#','#','#','#','#','#'},
    {'#','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'#','.','.','.','.','.','.','.','.','.','.','.','.','#','.','.','.','.','.','.'},
    {'#','.','.','.','.','.','.','.','.','.','.','.','.','#','.','.','.','.','.','.'},
    {'#','.','.','.','#','.','.','.','.','.','.','.','.','#','.','.','.','.','.','.'},
    {'#','.','.','.','#','.','.','.','.','.','.','.','.','#','.','.','.','.','.','.'},
    {'#','.','.','.','#','.','.','.','.','.','.','.','.','#','.','.','#','#','#','#'},
    {'#','.','.','.','#','#','#','.','.','.','.','.','.','#','.','.','#','.','.','.'},
    {'#','.','.','.','#','.','.','.','.','.','.','.','.','#','.','.','#','.','.','.'},
    {'#','.','.','.','#','.','.','.','.','.','.','#','#','#','.','.','#','.','.','.'},
    {'#','.','.','.','#','.','.','.','#','#','#','#','.','.','.','.','#','.','.','.'},
    {'#','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','#','.','.','.'},
    {'#','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'#','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'#','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
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

static void *input(void *none) {
    for (;;) {
        dir_x = getch();
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

        if (mvprintw(0, 0, "bnd.min.x %d; mnd.min.y %d ", boundary.min.x, boundary.min.y) == ERR) {
            printw("Error! error %d", ERR);
        }
        if (mvprintw(1, 0, "bnd.max.x %d; mnd.max.y %d ", boundary.max.x, boundary.max.y) == ERR) {
            printw("Error! error %d", ERR);
        }
        mvaddch(dsplhead.y, dsplhead.x, pac_head);
        refresh();
        mvaddch(dsplhead.y, dsplhead.x, ' ');
        int tmp = dir_x; 
            switch (tmp) {
            case 's':
            case KEY_DOWN:
                pac_head = '^';
                if (map[loghead.y + 1][loghead.x] == '.') {
                    loghead.y++;
                    dsplhead.y++;
                    map[loghead.y][loghead.x] = ' ';
                } else if (map[loghead.y + 1][loghead.x] == '#') {
                    loghead.y = loghead.y;
                } else {
                    loghead.y++;
                    dsplhead.y++;
                }    
                usleep(150000);
                break;
            case 'w':
            case KEY_UP: 
                pac_head = 'v';
                if (map[loghead.y - 1][loghead.x] == '.') {
                    loghead.y--;
                    dsplhead.y--;
                    map[loghead.y][loghead.x] = ' ';
                } else if (map[loghead.y - 1][loghead.x] == '#') {
                    loghead.y = loghead.y;
                } else {
                    loghead.y--;
                    dsplhead.y--;
                }
                usleep(150000);
                break;
            case 'a':
            case KEY_LEFT:
                pac_head = '>';
                if (map[loghead.y][loghead.x - 1] == '.') {
                    loghead.x--;
                    dsplhead.x--;
                    map[loghead.y][loghead.x] = ' ';
                } else if (map[loghead.y][loghead.x - 1] == '#') {
                    loghead.x = loghead.x;
                } else {
                    loghead.x--;
                    dsplhead.x--;
                }
                break;
            case 'd':
            case KEY_RIGHT:
                pac_head = '<';
                if (map[loghead.y][loghead.x + 1] == '.') {
                    loghead.x++;
                    dsplhead.x++;
                    map[loghead.y][loghead.x] = ' ';
                } else if (map[loghead.y][loghead.x + 1] == '#') {
                    loghead.x = loghead.x;
                } else {
                    loghead.x++;
                    dsplhead.x++;
                }
                break;         
            case 'q':
                done = TRUE;
                break;  
            }
        if (done) break;        
        usleep(150000);
    }      
    endwin();
    return 0;
}
