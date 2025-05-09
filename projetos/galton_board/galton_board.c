#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "include/ssd1306.h"
#include "math.h"
#include "stdlib.h"     
#include "time.h"
#include <string.h>

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

#define MAX_BALLS 10
#define N 5
#define MAX_PEGS 15 // ((1+N) * N) / 2
#define H 64
#define W 128
#define MAX_HISTOGRAM_HEIGHT 12
#define HISTOGRAM_WIDTH 20

const double PI = 3.1415926535;
const double GRAVITY = 1.75;
const int HORIZONTAL_SPACE = 20;
const int VERTICAL_SPACE = 7;
const int START_COLUMN=W/2 - 1;
const int START_ROW=10;
const double DT = 0.35;
const int BALL_RADIUS=1;
const int PEG_RADIUS=2;
const int STOP_BALLS = 10000;
static int BALLS=0;

typedef struct Ball {
    int x, y, radius;
    double vx, vy;
} Ball;

typedef struct Peg {
    int x, y, radius;
} Peg;

typedef struct Gamemaster {
    int n_pegs;
    int n_balls;

    Peg pegs[MAX_PEGS];
    Ball balls[MAX_BALLS];

    int result[N+1];
} Gamemaster;

// Variável gamemaster global
static Gamemaster gamemaster;

int rand_mod(int mod) {
    int r = rand() % mod;

    return r;
}

int intceil(int a, int b) {
    return (a / b) + (a % b != 0);
}

void setup_ball( int x, int y, int radius) {
    int vx = (rand_mod(2) ? 1: -1) * rand_mod(2);
    vx *= 0.5;
    if(vx==0)
        vx=rand_mod(2) ? 1: -1;

    int x_offset = rand_mod(2) ? 1 : -1;
    Ball ball = {x + x_offset, y, radius, vx, 0};
    gamemaster.balls[gamemaster.n_balls] = ball;
    gamemaster.n_balls++;
}

void setup_peg( int x, int y, int radius) {
    Peg peg = {x, y, radius};
    gamemaster.pegs[gamemaster.n_pegs] = peg;
    gamemaster.n_pegs++;    
}

void setup_gamemaster() {
    gamemaster.n_balls=0;
    gamemaster.n_pegs=0;
    for(int i=0; i<=N; i++)
        gamemaster.result[i] = 0;

    // Inicializa cada peg
    for(int row=0; row<N; row++) {
        for(int column=0; column <= row; column++) {
            setup_peg((column+START_COLUMN-(intceil(row*HORIZONTAL_SPACE, 2))) + column*HORIZONTAL_SPACE, START_ROW+row*VERTICAL_SPACE, PEG_RADIUS);
        }
    }
}

void collision_handler() {
    for(int i=0; i<gamemaster.n_balls; i++) 
    {
        Ball *ball = &gamemaster.balls[i];
        
        // Checa cada uma das pegs para ver se houve colisão com uma bola
        for(int j=0; j<gamemaster.n_pegs; j++) 
        {
            Peg *peg = &gamemaster.pegs[j];

            int dx = ball->x - peg->x;
            int dy = ball->y - peg->y;

            int distance = dx*dx + dy*dy;
            int r_sum = ball->radius + peg->radius;
            

            double normal_x = dx / sqrt(distance);
            double normal_y = dy / sqrt(distance);

            if(distance <= r_sum*r_sum) 
            {
                double dot = -2*((normal_x * ball->vx) + (normal_y * ball->vy));
                if(dot > 0) 
                {
                    ball->x = peg->x + (normal_x * (distance+1));
                    ball->y = peg->y + (normal_y * (distance+1));

                    ball->vx = ball->vx + (normal_x * dot) * 0.4;
                    ball->vy = ball->vy + (normal_y * dot) * 0.5;
                }
            }
        }

        // Bola acertou as paredes?
        if(ball->x >= W-1 || ball->x <= 0) 
        {
            ball->vx = -ball->vx;
            if(ball->x >= W-1)
                ball->x = W-2;
            else
                ball->x = 1;
        }
        if(ball->y <= 0) 
        {
            ball->vy = -ball->vy;
            ball->y = 1;
        }
        if(ball->y >= H-1) 
        {
            int local = ball->x / (W / (N+1));
            for (int k = i; k < gamemaster.n_balls - 1; k++) 
            {
                gamemaster.balls[k] = gamemaster.balls[k + 1];
            }
            i--;
            gamemaster.n_balls--;

            gamemaster.result[local]++;
            BALLS++;
        }
    }
}

void update_balls() 
{
    // Atualiza cada bola atualmente na gamemaster
    for(int i=0; i<gamemaster.n_balls; i++) 
    {
        Ball *ball = &gamemaster.balls[i];
        
        // Atualiza a velocidade da bola
        ball->vy += GRAVITY*DT;
        
        // Atualiza a posição da bola
        ball->y += ball->vy*DT;

        double new_x = ball->vx*DT; 
        if(abs(new_x)<1.0) {
            new_x = rand_mod(2) ? 1: -1;
        }


        ball->x += new_x;
        collision_handler(gamemaster);
    }
}

void driver_i2c_init() 
{
    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void display_init(ssd1306_t *disp) 
{
    driver_i2c_init();

    disp->external_vcc=false;
    ssd1306_init(disp, 128, 64, 0x3C, I2C_PORT);
    ssd1306_clear(disp);
}

void draw_circle(ssd1306_t *disp, int cy, int cx, int radius) 
{
    ssd1306_draw_pixel(disp, cx, cy);
    for (int y = -radius; y <= radius; y++) 
    {
        int y_pos = cy + y;
        if (y_pos < 0 || y_pos >= H) continue;

        int dx = (int)sqrt(radius * radius - y * y);
        int x_start = cx - dx;
        int x_end = cx + dx;

        if (x_start < 0) x_start = 0;
        if (x_end >= W) x_end = W - 1;

        for (int x = x_start; x <= x_end; x++) 
        {
            ssd1306_draw_pixel(disp, x, y_pos);
        }
    }
}

void update_display(ssd1306_t *disp) 
{
    ssd1306_clear(disp);

    char balls_display[6];
    sprintf(balls_display, "%d", BALLS);

    ssd1306_draw_string(disp, 95, 0, 1, balls_display);

    for(int i=0; i<gamemaster.n_balls; i++) 
    {
        draw_circle(disp, gamemaster.balls[i].y, gamemaster.balls[i].x, gamemaster.balls[i].radius);
    } 
    
    for(int i=0; i<gamemaster.n_pegs; i++) 
    {
        draw_circle(disp, gamemaster.pegs[i].y, gamemaster.pegs[i].x, gamemaster.pegs[i].radius);
    } 

    int max_value = -1;
    for(int i = 0; i < N+1; i++)
        if(gamemaster.result[i] > max_value) max_value = gamemaster.result[i];

    int display_result[N+1];
    for(int i = 0; i < N+1; i++) display_result[i] = gamemaster.result[i];

    if(max_value > 18)
    {
        double scale_factor = 18.0/max_value;
        for(int i = 0; i < N+1; i++)
        {
            display_result[i] = (int) (gamemaster.result[i] * scale_factor);
        }
    }

    for(int i=0; i<N+1; i++)
    {
        for(int j = 2+(21*i); j <= 21+(21*i); j++) //grossura dos elementos do histograma
        {
            for(int k = 64; k > 63 - display_result[i]; k--) //altura dos elementos do histograma
            {
                ssd1306_draw_pixel(disp, j, k);
            }
        }
    } 

    ssd1306_show(disp);
}

int main()
{
    stdio_init_all();

    ssd1306_t disp;
    display_init(&disp);

    srand(time(NULL));
    setup_gamemaster();

    while (true) 
    {
        update_display(&disp);
        if(gamemaster.n_balls < MAX_BALLS)
            setup_ball(START_COLUMN, 1, BALL_RADIUS);

        update_balls();
    }
}