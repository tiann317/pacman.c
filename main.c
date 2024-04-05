// +---------+
// |    |    |
// |    |    |
// +----------+
// |    |    |
// |    |    |
// +---------+ 
//TODO: add random border to one distibuted quarter
//TODO: add 2 dim arrays representation of the fields
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h> 
#include <stdlib.h> 
#include <time.h>
#define hight 30
#define width 40
#define lhight 15
#define lwidth 20

chtype pac_head = 'o';
chtype wall = '#';
int dir_x;
char arr[hight][width];
char larr[lhight][lwidth] = {
    {'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'.','#','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'.','#','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'.','#','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'.','#','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'.','#','#','#','#','#','#','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'.','#','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'.','#','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'},
};

typedef struct point {
    int x;
    int y;
} Point;

typedef struct rect {
    struct point max;
    struct point min;
} Boundary;

Point head;
Boundary fullscr;
Boundary boundary;
Boundary quarter;

static void fill(void) {
    for (int i = boundary.min.x; i < boundary.max.x; i++) {
        for (int j = boundary.min.y; j < boundary.max.y - 1; j++) {
            arr[i][j] = '.';
            mvaddch(i, j, arr[j][i]);
        }
        mvaddch(0, i, ' ');
    }
}

static void logic_quarter(void) {
    for (int i = quarter.min.y; i < quarter.max.y; i++) {
        for (int j = quarter.min.x; j < quarter.max.x - 1; j++)
            mvaddch(i, j, larr[j][i]);  
        mvaddch(0, i, ' ');
    }
}

static void bound_print(void) {
        for (int i = boundary.min.x - 2; i < boundary.max.x + 2; i++) {
            mvaddch(boundary.min.y - 1, i, wall);
            mvaddch(boundary.max.y , i, wall);
        }
        for (int j = boundary.min.y - 2; j < boundary.max.y + 2; j++)
        {
            mvaddch(j, boundary.min.x - 1, wall);
            mvaddch(j, boundary.max.x, wall);
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
//    srand(time(NULL));
    
    getmaxyx(stdscr, fullscr.max.y, fullscr.max.x);   
    boundary.max.x = fullscr.max.x/2 + 20;
    boundary.min.x = fullscr.max.x/2 - 20;
    boundary.max.y = fullscr.max.y/2 + 15;
    boundary.min.y = fullscr.max.y/2 - 15;

    quarter.min.x = boundary.min.x;
    quarter.min.y = boundary.min.y; 
    quarter.max.y = boundary.min.y + 15;
    quarter.max.x = boundary.min.x + 20;
    
    head.x = boundary.min.x + boundary.max.x/2;
    head.y = boundary.min.y + boundary.max.y/2;

    while(TRUE) {
        bound_print();
//        fill();
        logic_quarter();
        if (mvprintw(1, 0, "tlhc x,y %d %d; brhc x,y %d %d", quarter.min.x, quarter.min.y,\
        quarter.max.x, quarter.max.y) == ERR) {
            printw("Error! error %d", ERR);
        }
        mvaddch(head.y, head.x, pac_head);
        refresh();
        mvaddch(head.y, head.x, ' ');
        int tmp = dir_x; 
            switch (tmp) {
            case 's':
            case KEY_DOWN:
                pac_head = 'V';
                head.y = head.y >= boundary.max.y ? boundary.min.y : head.y; 
                head.y++;           
                usleep(150000);
                break;
            case 'w':
            case KEY_UP: 
                pac_head = '^';
                if (head.y <= boundary.min.y)
                    head.y = boundary.max.y;
                head.y--;
                usleep(150000);
                break;
            case 'a':
            case KEY_LEFT:
                pac_head = '<';
                if (head.x <= boundary.min.x) 
                    head.x = boundary.max.x;
                head.x--;
                break;
            case 'd':
            case KEY_RIGHT:
                pac_head = '>';
                head.x = head.x >= boundary.max.x ? boundary.min.x : head.x;
                head.x++;
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
