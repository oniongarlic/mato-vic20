#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define SCREEN ((unsigned char*)0x1E00)
#define COLOR  ((unsigned char*)0x9600)
#define VIC_MEM (*(unsigned char*)0x9005)
#define BORDER (*(unsigned char*)0x900F)
#define ROM_CHARSET ((unsigned char*)0x8000)
#define CHARSET     ((unsigned char*)0x1800)
#define CHARSET_MEM 0x1800
#define KEYBOARD  (*(unsigned char*)0x00C5)

#pragma section( charset, 0)
#pragma region( charset, 0x1800, 0x2000, , , {charset} )
unsigned char vic_charset[2048];

#define WIDTH 22
#define HEIGHT 22

#define C_BG    0
#define C_WALL  6
#define C_HEAD  2
#define C_BODY  5
#define C_FOOD  2
#define C_PORT  3

unsigned char defborder;

#define MAX_LEN 80

unsigned char snake_x[MAX_LEN];
unsigned char snake_y[MAX_LEN];
unsigned char length = 5;
unsigned char lives = 4;
unsigned char foods = 0;
unsigned char tick = 0;

unsigned char dir = 3;

unsigned int score = 0;
unsigned int level = 1;

unsigned char food_x, food_y;

#define CH_HEAD_UP    170
#define CH_HEAD_DOWN  171
#define CH_HEAD_LEFT  172
#define CH_HEAD_RIGHT 173
#define CH_BODY       174

void handle_input();

void wait_frames(unsigned char frames) {
    clock_t start=clock();
    clock_t cur=start;
    while (cur-start < frames) {
      handle_input();
      cur = clock();
    }
}

unsigned char getkey()
{
    volatile unsigned char c=KEYBOARD;
    return c;
}

void set_charset(unsigned int addr) {
    unsigned char screen_bits = VIC_MEM & 0xF0;
    unsigned char char_bits = 14;

    VIC_MEM = screen_bits | char_bits;
}

void put_cell(unsigned char x, unsigned char y, unsigned char c, unsigned char col) {
    unsigned int i = y * WIDTH + x;
    SCREEN[i] = c;
    COLOR[i]  = col;
}

void print_num(unsigned int v, unsigned char x, unsigned char y) {
    put_cell(x,   y, '0' + (v / 1000), 5);
    put_cell(x+1, y, '0' + (v / 100), 5);
    put_cell(x+2, y, '0' + ((v / 10) % 10), 5);
    put_cell(x+3, y, '0' + (v % 10), 5);
}

void print_c(unsigned int v, unsigned char x, unsigned char y) {
    if (v>999)
     v=999;
    put_cell(x, y, '0' + (v / 100), 5);
    put_cell(x+1, y, '0' + ((v / 10) % 10), 5);
    put_cell(x+2, y, '0' + (v % 10), 5);
}

void copy_charset() {
    for (unsigned int i = 0; i < 2048; i++) {
        CHARSET[i] = ROM_CHARSET[i];
    }
}

void define_char(unsigned char code, unsigned char *data) {
    unsigned char *ptr = CHARSET + (code * 8);
    for (int i = 0; i < 8; i++) {
        ptr[i] = data[i];
    }
}

void init_charset() {
    unsigned char up[8] =   {60,66,129,165,129,129,189,126};
    unsigned char down[8] = {126,189,129,129,165,129,66,60};
    unsigned char left[8] = {62,65,147,131,131,147,65,62};
    unsigned char right[8]= {124,130,201,193,193,201,130,124};
    unsigned char body[8] = {60,126,231,195,195,231,126,60};

    define_char(CH_HEAD_UP, up);
    define_char(CH_HEAD_DOWN, down);
    define_char(CH_HEAD_LEFT, left);
    define_char(CH_HEAD_RIGHT, right);
    define_char(CH_BODY, body);

    set_charset(CHARSET_MEM);
}

void draw_portals() {
    put_cell(WIDTH/2, 0, 0x2A, C_PORT);
    put_cell(WIDTH/2, HEIGHT-1, 0x2A, C_PORT);
    put_cell(0, HEIGHT/2, 0x2A, C_PORT);
    put_cell(WIDTH-1, HEIGHT/2, 0x2A, C_PORT);
}

void draw_board() {
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        SCREEN[i] = ' ';
        COLOR[i] = C_BG;
    }

    // We use 22x22 so we can do all walls in one loop
    for (unsigned char i = 0; i < WIDTH; i++) {
        put_cell(i, 0, '#', C_WALL);
        put_cell(i, HEIGHT-1, '#', C_WALL);
        put_cell(0, i, '#', C_WALL);
        put_cell(WIDTH-1, i, '#', C_WALL);
    }

    draw_portals();

}

void reset_border() {
    BORDER = defborder;
}

void flash_border() {
    for (int i = 0; i < 8; i++) {
        BORDER = (i & 1) ? 2 : 6;
        wait_frames(8);
    }
}

void flash_border_food() {
    for (int i = 0; i < 4; i++) {
        BORDER = (i & 1) ? 2 : 3;
        wait_frames(1);
    }
    BORDER = defborder;
}

void handle_input() {
   unsigned char c=getkey();

   if (c==64)
     return;

   if (c == 9 && dir != 1) dir = 0;
   if (c == 41 && dir != 0) dir = 1;
   if (c == 17 && dir != 3) dir = 2;
   if (c == 18 && dir != 2) dir = 3;
}

void print_stats() {
    print_num(level, 1, 22);
    print_num(score, 6, 22);
    //print_num(tick, 12, 22);
}

void game_over() {
    unsigned char c=0;

    flash_border();
    clrscr();
    gotoxy(7, 9);
    puts(p"game over");
    gotoxy(7, 12);
    puts(p"y for new game");
    reset_border();
    print_stats();
    while (c!=89) {
        c = getch();
        print_num(c, 4, 4);
        wait_frames(8);
    }
}

char check_overlap(unsigned char x, unsigned char y) {
    for (int i = 0; i < length; i++) {
        if (snake_x[i] == x && snake_y[i] == y) {
            return 1;
        }
    }
    return 0;
}

void spawn_food() {
    do {
     food_x = (rand() % (WIDTH-2)) + 1;
     food_y = (rand() % (HEIGHT-2)) + 1;
    } while (check_overlap(food_x, food_y)==1);
    put_cell(food_x, food_y, 0x24, C_FOOD);
}

void snake_eats() {
    score+=length;
    length++;
    if (length>MAX_LEN)
       length=MAX_LEN;
    spawn_food();
    foods++;
    if (foods>10) {
      level++;
      foods=0;
    }
    // flash_border_food();
    print_stats();
}

int move_snake() {
    unsigned char nx = snake_x[0];
    unsigned char ny = snake_y[0];

    if (dir == 0) ny--;
    if (dir == 1) ny++;
    if (dir == 2) nx--;
    if (dir == 3) nx++;

    // portals
    if (ny == 0 && nx == WIDTH/2) ny = HEIGHT-2;
    else if (ny == HEIGHT-1 && nx == WIDTH/2) ny = 1;
    else if (nx == 0 && ny == HEIGHT/2) nx = WIDTH-2;
    else if (nx == WIDTH-1 && ny == HEIGHT/2) nx = 1;

    if (SCREEN[ny * WIDTH + nx] == '#') {
        return 1;
    }

    if (check_overlap(nx, ny)==1)
        return 1;

    for (int i = length; i > 0; i--) {
        snake_x[i] = snake_x[i-1];
        snake_y[i] = snake_y[i-1];
    }

    snake_x[0] = nx;
    snake_y[0] = ny;

    if (tick>60) {
        tick=0;
        spawn_food();
    }

    if (nx == food_x && ny == food_y) {
        snake_eats();
        tick=0;
    } else {
        put_cell(snake_x[length], snake_y[length], ' ', C_BG);
    }
    tick++;
    print_num(60-tick, 12, 22);
    return 0;
}

void draw_snake() {
    unsigned char head_char;
    if (dir == 0) head_char = CH_HEAD_UP;
    else if (dir == 1) head_char = CH_HEAD_DOWN;
    else if (dir == 2) head_char = CH_HEAD_LEFT;
    else head_char = CH_HEAD_RIGHT;

    put_cell(snake_x[0], snake_y[0], head_char, C_HEAD);

    for (int i = 1; i < length; i++) {
        put_cell(snake_x[i], snake_y[i], CH_BODY, C_BODY);
    }
}

void init_snake()
{
    clrscr();
    draw_board();
    length=level+5;
    if (length>15)
      level=15;
    dir=3;
    for (int i = 0; i < length; i++) {
        snake_x[i] = length+2 - i;
        snake_y[i] = 8;
    }
    spawn_food();
    print_stats();
}

void game_loop()
{
    length=5;
    score=0;
    level=1;
    tick=0;

    init_snake();

    while (1) {
        unsigned char r;
        handle_input();
        r=move_snake();
        if (r==1) {
          lives--;
          if (lives==0)
            return;
          init_snake();
        }
        draw_snake();
        wait_frames(12);
    }
}

int main() {
    defborder = BORDER;
    copy_charset();
    init_charset();

    while (1) {
      game_loop();
      game_over();
      wait_frames(32);
    }

    return 0;
}

