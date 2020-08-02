//============================================================================
// Name        : game.cpp
// Author      : alex
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include<fstream>
#include<string>
#include<sstream>
#include "GPIO.h"
#include "bitmaps.h"
#include<stdio.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <sys/timeb.h>
#include <chrono>
#include"oled_driver.h"

using namespace std;
using namespace exploringBB;

#define GHOSTS 4

#define INVISIBLE 0
#define VISIBLE 1

/* Timer */
uint64_t millis()
{
    uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::
                  now().time_since_epoch()).count();
    return ms;
}


/* ANALOG */
#define LDR_PATH "/sys/bus/iio/devices/iio:device0/in_voltage"
int readAnalog(int number){
// returns the input as an int
stringstream ss;
ss << LDR_PATH << number << "_raw";
fstream fs;
fs.open(ss.str().c_str(), fstream::in);
fs >> number;
fs.close();
return number;
}

void system_init()
{
    /* Initialize I2C bus and connect to the I2C Device */
    if(init_i2c_dev(I2C_DEV2_PATH, SSD1306_OLED_ADDR) == 0)
    {
        printf("(Main)i2c-2: Bus Connected to SSD1306\r\n");
    }
    else
    {
        printf("(Main)i2c-2: OOPS! Something Went Wrong\r\n");
        exit(1);
    }

    /* Run SDD1306 Initialization Sequence */
    display_Init_seq();

    /* Clear display */
    clearDisplay();
}


class Object
{
  private:
	int x, y; /* position on the screen */
	int w,h; //  obj size
	int dx, dy; /* movement vector */
	bool move;

 public:
	void set_x(int x_val)
    {
		x = x_val;
    }

	int get_x()
	{
		return x;
	}

	void set_y(int y_val)
    {
		y = y_val;
    }

	int get_y()
	{
		return y;
	}

	void set_h(int h_val)
    {
		h = h_val;
    }

	int get_h()
	{
		return h;
	}

	void set_w(int w_val)
    {
		w = w_val;
    }

	int get_w()
	{
		return w;
	}

	void set_dx(int dx_val)
    {
		dx = dx_val;
    }

	int get_dx()
	{
		return dx;
	}

	void set_dy(int dy_val)
    {
		dy = dy_val;
    }

	int get_dy()
	{
		return dy;
	}

	void set_move(int move_val)
    {
		move = move_val;
    }

	int get_move()
	{
		return move;
	}

} boy, ghost[GHOSTS], coin;

//inisilise starting position and sizes of game elemements
static void init_game()
{
	for(int i = 0; i < GHOSTS; i++)
	{
		ghost[i].set_x(rand() % 100);
		ghost[i].set_y (rand() % 40);
		ghost[i].set_w (16);
		ghost[i].set_h (12);
		ghost[i].set_dy (2);
		ghost[i].set_dx (2);
		ghost[i].set_move (true);
	}

	boy.set_x(5);
	boy.set_y (5);
	boy.set_w (16);
	boy.set_h (14);
	boy.set_dy (3);
	boy.set_dx (3);
	boy.set_move (true);

	coin.set_x(rand() % 100);
	coin.set_y (rand() % 40);
	coin.set_w (8);
	coin.set_h (8);
	coin.set_dy (0);
	coin.set_dx (0);
	coin.set_move (true);
}

//if return value is 1 collision occured. if return is 0, no collision.
int check_collision( Object  a,  Object  b)
{
	int left_a, left_b;
	int right_a, right_b;
	int top_a, top_b;
	int bottom_a, bottom_b;

	left_a = a.get_x();
	right_a = a.get_x() + a.get_w();
	top_a = a.get_y();
	bottom_a = a.get_y() + a.get_h();

	left_b = b.get_x();
	right_b = b.get_x() + b.get_w();
	top_b = b.get_y();
	bottom_b = b.get_y() + b.get_h();

	if (left_a > right_b) {
		return 0;
	}

	if (right_a < left_b) {
		return 0;
	}

	if (top_a > bottom_b) {
		return 0;
	}

	if (bottom_a < top_b) {
		return 0;
	}
	return 1;
}

static void move_ghost()
{
	for(int i=0; i <GHOSTS; i++)
	{
		/* Move the ghost by its motion vector. */
		ghost[i].set_x(ghost[i].get_x() + ghost[i].get_dx());
		ghost[i].set_y(ghost[i].get_y() + ghost[i].get_dy());

		/* Turn the ghost around if it hits the edge of the screen. */
		if (ghost[i].get_x() < 0 || ghost[i].get_x() > (SCREEN_WIDTH - 16))
		{
			ghost[i].set_dx(-ghost[i].get_dx());
		}

		if (ghost[i].get_y() < 0 || ghost[i].get_y() > SCREEN_HEIGHT - 16)
		{
			ghost[i].set_dy(-ghost[i].get_dy());
		}

	}
}

void game_start_screen()
{
	clearDisplay();
	setTextSize(2);
	setTextColor(WHITE);
    setCursor(5,7);
    print_str("GOLD&GHOST");
	setTextSize(1);
	setCursor(40,45);
	print_str("press A");
	Display();
}

void game_end_screen( int score)
{
	clearDisplay();
	setTextSize(2);
	setTextColor(WHITE);
    setCursor(10,5);
    print_str("GAME OVER");
	setTextSize(1);
	setCursor(25,30);
	print_str("score: ");
	setCursor(80,30);
	printNumber(score, DEC);
	setCursor(11,50);
	print_str("press A to restart");
	Display();
}

void show_scre_life(int life, int score)
{
	drawRect(0,0,life,3,1);
	setTextSize(1);
	setCursor(110,0);
	printNumber(score, DEC);
	Display();
}


void game_startup(bool *start_game, int button_val, int *life, int *score)
{
	while(!start_game)
	{
		if(button_val)
	 	{
			*start_game = true;
	 		*life = 64;
	 		*score = 0;
	 		init_game();
	 		clearDisplay();
	 	}
	 }
}
void game_value_reset(bool *start_game, int *life, int *score)
{
	*start_game = true;
	*life = 64;
	*score = 0;
	init_game();
	clearDisplay();
}

int main()
{
	bool debug = false;
	cout << "BOY vs Ghost" << endl; // prints !!!Hello World!!!
	system_init();
	init_game();

	int ghosts = sizeof(ghost)/sizeof(ghost[0]);
	GPIO button_a(115);
	int x = 0;
	int y = 0;
	int a = 0;
	int life = 64;
	int score = 0;
	int score_prescaler = 0;
	bool start_game = false;

	uint64_t t_ms;
	bool boy_damage = false;

	game_start_screen();

	while(!start_game)
	{
		if(button_a.getValue())
		{
			game_value_reset(&start_game, &life, &score);
		}
	}


while(1)
{
	while(life > 0)
	{
		t_ms = millis();
		x = readAnalog(0);
		y = readAnalog(2);
		a = button_a.getValue();


		/*BOY*/
		if(t_ms % 1 == 0)
		{
			/*Clear screen*/
			clearDisplay();

			for(int i=0; i < ghosts; i++)
			{

				if(check_collision(ghost[i],boy))
					life-=2;
					boy_damage = true;
			}

			if(check_collision(coin,boy))
			{
				score++;
				coin.set_x(rand() % 100);
				coin.set_y (rand() % 40);

			}

			move_ghost();

			/* boy movements */
			/*x*/
			if(x > 2000)
			{
				if(boy.get_x() <= 0)
					boy.set_x(0);
				else
					boy.set_x(boy.get_x()- boy.get_dx());
//					boy.x-=boy.dx;
				boy.set_move (true);
			}

			if(x < 1300)
	   	   {
		   	   if(boy.get_x() >= 112)
					boy.set_x(112);
				else
					boy.set_x(boy.get_x()+ boy.get_dx());
				boy.set_move (true);
			}

	   		/*y*/
	   		if(y > 2000)
	   		{
	   			if(boy.get_y() >= 49)
	   				boy.set_y (49);
	   			else
	   				boy.set_y(boy.get_y() + boy.get_dy());
	   			boy.set_move (true);
	   		}

	   		if(y < 1300)
	   		{
	   			if(boy.get_y() <= 0)
	   				boy.set_y(0);
	   			else
	   				boy.set_y(boy.get_y() - boy.get_dy());
	   			boy.set_move (true);
	   		}


	   		/*Update screen*/

	   		if((boy.get_y()%2 == 0 || boy.get_x()%2 == 0) && boy.get_move())
	   		{
	   			drawBitmap(boy.get_x(), boy.get_y(),  r_leg_up, boy.get_w(), boy.get_h(), VISIBLE);
	   			boy.set_move(false);
	   		}

	   		else if (boy.get_move())
	   		{
	   			drawBitmap(boy.get_x(), boy.get_y(),  l_leg_up, boy.get_w(), boy.get_h(), VISIBLE);
	   			boy.set_move (false);
	   		}
	   		else if (boy_damage)
	   		{
	   			drawBitmap(boy.get_x(), boy.get_y(),  big_mounth, boy.get_w(), boy.get_h(), VISIBLE);
	   			boy_damage = false;
	   		}
	   		else
	   			drawBitmap(boy.get_x(), boy.get_y(),  stand, boy.get_w(), boy.get_h(), VISIBLE);


	   		for(int i=0; i < ghosts; i++)
	   		{
	   			if(ghost[i].get_move())
	   				drawBitmap(ghost[i].get_x(), ghost[i].get_y(),  ghost_small_m1, ghost[i].get_w(), ghost[i].get_h(), VISIBLE);
	   			else
	   				drawBitmap(ghost[i].get_x(), ghost[i].get_y(),  ghost_big_m2, ghost[i].get_w(), ghost[i].get_h(), VISIBLE);

	   			ghost[i].set_move(!ghost[i].get_move());
	   		}
	   		if(t_ms % 2 == 0)
	   			drawBitmap(coin.get_x(), coin.get_y(),  coin_bmp, coin.get_w(), coin.get_h(), VISIBLE);
	   		else
	   			drawBitmap(coin.get_x(), coin.get_y(),  coin__blink_bmp, coin.get_w(), coin.get_h(), VISIBLE);

		}

		show_scre_life(life, score);

		if(debug)
		{
			cout << "x: " << boy.get_x() << " y: " << boy.get_y() << " a: " << a << endl;
			cout << "life " << life << endl;
			cout << "score " << score << endl;
			cout << "t_ms: "<< t_ms << endl;
		}

	}

	start_game = false;

	game_end_screen(score);

	 while(!start_game)
	 	{
	 		if(button_a.getValue())
	 		{
	 			game_value_reset(&start_game, &life, &score);
	 		}
	 	}
}

	return 0;
}

