//TODO: compress the field by y axis (it looks like shit)
//TODO: add logical distribution of field by 4 parts
// +---------+
// |    |    |
// |    |    |
// +----------+
// |    |    |
// |    |    |
// +---------+ 
//TODO: add random border to one distibuted quarter
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h> 
#include <stdlib.h> 
#include <time.h>  

typedef enum {
    BACK = 0,
    FORE = 1,
} Pixel;

typedef struct point {
    int x;
    int y;
} Point;

typedef struct rect {
    struct point max;
    struct point min;
} Boundary;

int dir_x;
Point head;
Boundary fullscr;
Boundary boundary;

static void fill(void);

static void bound_print(void) {
        for (int i = boundary.min.x; i < boundary.max.x; i++) {
            mvprintw(boundary.min.y, i, "--");
            mvprintw(boundary.max.y, i, "--");
        }
        for (int j = boundary.min.y; j < boundary.max.y; j++)
        {
            mvprintw(j, boundary.min.x, "|");
            mvprintw(j, boundary.max.x, "|");
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
    srand(time(NULL));
    
    getmaxyx(stdscr, fullscr.max.y, fullscr.max.x);   
    boundary.max.x = fullscr.max.x/2 + 20;
    boundary.min.x = fullscr.max.x/2 - 20;
    boundary.max.y = fullscr.max.y/2 + 15;
    boundary.min.y = fullscr.max.y/2 - 15;
    head.x = boundary.max.x/2;
    head.y = boundary.max.y/2;
    chtype snake_head = 'o';
    chtype food = '+';
    while(TRUE) {
        bound_print();   
        if (mvprintw(1, 0, "max x %d; max y %d", boundary.max.x, boundary.max.y) == ERR) {
            printw("Error! error %d", ERR);
        }
        if (mvprintw(0, 0, "min x %d; min y %d", boundary.min.x, boundary.min.y) == ERR) {
            printw("Error! error %d", ERR);
        }
        for (int i = boundary.min.x + 1; i < boundary.max.x; i++) {
            for (int j = boundary.min.y + 1; j < boundary.max.y; j++)
            mvaddch(j, i, food);
        }
        mvaddch(head.y, head.x, snake_head);
        refresh();
        mvaddch(head.y, head.x, ' ');
        int tmp = dir_x; 
            switch (tmp) {
            case 's':
            case KEY_DOWN:
                snake_head = 'V';
                head.y = head.y >= boundary.max.y ? boundary.min.y : head.y; 
                head.y++;           
                usleep(150000);
                break;
            case 'w':
            case KEY_UP: 
                snake_head = '^';
                if (head.y <= boundary.min.y)
                    head.y = boundary.max.y;
                head.y--;
                usleep(150000);
                break;
            case 'a':
            case KEY_LEFT:
                snake_head = '<';
                if (head.x <= boundary.min.x) 
                    head.x = boundary.max.x;
                head.x--;
                break;
            case 'd':
            case KEY_RIGHT:
                snake_head = '>';
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
