#include <Arduino.h>
#include <SPI.h>
#include <iostream>
#include "GD3.h"
#include "cl_plc_objects.h"
#include "default_assets.h"

/* Random time range for potion & strawberry timers */
int potion_random = random(5000, 50000); 
int potion2_random = random(5000, 50000);
int straw_random = random(5000, 50000);
int straw2_random = random(5000, 50000);

static uint32_t hit_time = millis() - 5000;
static uint32_t hit_time2 = millis() - 5000;

int counter = 0;
boolean flag_fade=false;

enum
{
	PAGE_START,
	PAGE_PLAY,
	PAGE_MENU,
	PAGE_INFO,
	PAGE_GOAL,
	PAGE_GAMEOVER,
	PAGE_YOUWIN
};

uint8_t page_display = PAGE_MENU; //Starting display page

class Drop{
	float yspeed = random(1,3); //pizzas start at different speeds above the screen
	int z = random(10,16); //Each pizza is a random pizza for the duration of its fall
	int alpha = 255;
public:
	float x = random(0,290);
	float y = random(-200,-10);
	
	void reset(){
		alpha=255;
		y=random(-200,-10);
		x=random(0,290);
		yspeed = random(0.5,1.5);
	}
	void fall(){
		if (y<190){
		y += yspeed;
		yspeed += 0.002;}
		else if (y>=190){
			y=190;
			yspeed=0;
			alpha-=1;
		}
		if (alpha<5){ //This works
			reset();
		}
	}

	void show1(){
	
	GD.ColorA(alpha);
	GD.Vertex2ii(x,y,z);
	}
};
enum
{
	WAITING_STRAWBERRY,
	FALLING_STRAWBERRY,
	LANDED_STRAWBERRY,
	DESTROY_STRAWBERRY
};

class strawBerry
{
	float yspeed = random(0.5,1.5);
	int z = 20;
	int alpha = 255;

public:
	float x = random(0, 290);
	float y = random(-200, -10);
	float y_start = -200;
	int state = WAITING_STRAWBERRY; //starting state
	bool flag_start = false;
	bool flag_destroy = false;

	uint32_t falling_start_time;
	uint32_t landed_start_time;

	void process()
	{
		switch (state)
		{
			case WAITING_STRAWBERRY:
				if (flag_start)
				{
					flag_start = false;
					state = FALLING_STRAWBERRY; //describing falling state
					falling_start_time = millis();
					x = random(0, 290);
					y_start = -200;
					y = y_start;
					alpha = 255;
				}
				break;
			case FALLING_STRAWBERRY:
				y = y_start + (millis() - falling_start_time)/10 + yspeed;
				if (y > 190)
				{
					y = 190;
					state = LANDED_STRAWBERRY;
					landed_start_time = millis();
				}
				if (flag_destroy)
				{
					flag_destroy = false;
					state = DESTROY_STRAWBERRY;
				}
				break;
			case LANDED_STRAWBERRY:
				alpha = 255 - (millis() - landed_start_time) / 10;
				if (alpha < 5 || flag_destroy)
				{
					flag_destroy = false;
					state = DESTROY_STRAWBERRY;
				}
				break;
			case DESTROY_STRAWBERRY:
				break;
		}
	}

	void draw()
	{
		switch (state)
		{
			case WAITING_STRAWBERRY:
				break;
			case FALLING_STRAWBERRY:
				GD.ColorA(255);
				GD.Vertex2ii(x, y, z);
				break;
			case LANDED_STRAWBERRY:
				GD.ColorA(alpha);
				GD.Vertex2ii(x, y, z);
				break;
			case DESTROY_STRAWBERRY:
				break;
		}
	}

	void start()
	{
		flag_start = true;
	}
	void destroy()
	{
		flag_destroy = true;
	}
};

enum
{
	WAITING_POTION,
	FALLING_POTION,
	LANDED_POTION,
	DESTROY_POTION
};

class Potion
{
	float yspeed = random(0.5,1.5); 
	int z = 21;
public:
	int alpha = 255;
	float x = random(0,290);
	float y = random(-200,-10);
	float y_start = -200;
	int state = WAITING_POTION;
	bool flag_start = false;
	bool flag_destroy = false;
	uint32_t falling_start_time;
	uint32_t landed_start_time;

	void process()
	{
		switch(state)
		{
			case WAITING_POTION:
				if (flag_start)
				{
					flag_start = false;
					state = FALLING_POTION;
					falling_start_time = millis();
					x = random(0,290);
					y_start = -200;
					y = y_start;
					alpha = 255;
				}
				break;
			case FALLING_POTION:
				y = y_start + (millis()-falling_start_time)/10 + yspeed;
				if (y > 190)
				{
					y = 190;
					state = LANDED_POTION;
					landed_start_time = millis();
				}
				if (flag_destroy)
				{
					flag_destroy = false;
					state = DESTROY_POTION;
				}
				break;
			case LANDED_POTION:
				alpha = 255 - (millis() - landed_start_time)/10;
				if (alpha < 5 || flag_destroy)
				{
					flag_destroy = false;
					state = DESTROY_POTION;
				}
				break;
			case DESTROY_POTION:
				break;
		}
	}
	void draw()
	{
		switch (state)
		{
			case WAITING_POTION:
				break;
			case FALLING_POTION:
				GD.ColorA(255);
				GD.Vertex2ii(x,y,z);
				break;
			case LANDED_POTION:
				GD.ColorA(alpha);
				GD.Vertex2ii(x,y,z);
				break;
			case DESTROY_POTION:
				break;
		}
	}
	void start()
	{
		flag_start = true;
	}
	void destroy()
	{
		flag_destroy = true;
	}
};

strawBerry *strawberry;
Potion *potion;
const int numOfDrops=15;
Drop* drops[numOfDrops]; //Drop d -> one pizza. Array makes more than one pizza fall
int maxDistance = 20;

boolean collide(float x1, float y1,float x2,float y2)  //say pizza is (x1,y1) and bunny is (x2,y2)
{ 
	//Centre coordinate calculations
	float pizzaCentreX = x1 + BBQ1_WIDTH/2;
	float pizzaCentreY = y1 + BBQ1_HEIGHT/2;
	float bunnyCentreX = x2 + BUNNY1_WIDTH/2;
	float bunnyCentreY = y2 + BUNNY1_HEIGHT/2;
	float distance =hypot(bunnyCentreX-pizzaCentreX,bunnyCentreY-pizzaCentreY);
	if (distance < 35)
	{
		return true;	
	}
	return false;
}
/*Page Drawings and Tag Definitions*/
void drawMenu();
void drawInstruct();
void drawBackButton();
void returnMenu();


#define BUTTON_MENU 1
#define BUTTON_INSTRUCT 2
#define BUTTON_GOAL 3
#define BUTTON_PLAY2 4
#define BUTTON_GAMEOVER 5
#define BUTTON_YOUWIN 6


static uint32_t goal_time = millis()-2000;
int collided = 0;
int alpha=0;

void cmd_text( int16_t x,
int16_t y,
int16_t font,
uint16_t options,
const char* s );

void drawMenu()
{
	GD.ClearColorRGB(0xffffff); 
	GD.Clear();
	GD.SaveContext();
	GD.Begin(BITMAPS);
	GD.BitmapHandle(BUNNYPIZZAMENU_HANDLE);
	GD.Vertex2f(0,0);
	GD.ColorRGB(0xff0066);
	GD.cmd_text((GD.w/2)-3,GD.h/2,31, OPT_CENTER, "BUNNY PIZZA");
	GD.cmd_text((GD.w/2)+3,GD.h/2,31, OPT_CENTER, "BUNNY PIZZA");
	GD.cmd_text((GD.w/2),(GD.h/2)-3,31, OPT_CENTER, "BUNNY PIZZA");
	GD.cmd_text((GD.w/2),(GD.h/2)+3,31, OPT_CENTER, "BUNNY PIZZA");
	GD.ColorRGB(0x00ffae);
	GD.cmd_text(GD.w/2,GD.h/2,31, OPT_CENTER, "BUNNY PIZZA");
	GD.ColorA(alpha);
	alpha-=2;
	GD.ColorRGB(0xff78ae);
	GD.Tag(BUTTON_GOAL); GD.cmd_text((GD.w/2)-1,180+1,26,OPT_CENTER, "PRESS TO PLAY");
	GD.ColorA(255);
	GD.Tag(BUTTON_INSTRUCT); GD.cmd_text((GD.w/2)-1,200+1,26,OPT_CENTER,"HOW TO PLAY");
	GD.ColorA(alpha);
	alpha-=1;
	GD.ColorRGB(0x6b1f53);
	GD.Tag(BUTTON_GOAL); GD.cmd_text(GD.w/2,180,26,OPT_CENTER, "PRESS TO PLAY");
	GD.ColorA(255);
	GD.Tag(BUTTON_INSTRUCT); GD.cmd_text(GD.w/2,200,26,OPT_CENTER,"HOW TO PLAY");
	goal_time = millis();
}

void drawBackButton() 
{
	GD.SaveContext();
	GD.ColorRGB(0xff78ae);
	GD.Tag(BUTTON_MENU); GD.cmd_text((GD.w/2)-1,200+1,26,OPT_CENTER, "BACK"); //GO back button
	GD.ColorRGB(0x6b1f53);
	GD.Tag(BUTTON_MENU); GD.cmd_text(GD.w/2,200,26,OPT_CENTER,"BACK");
	GD.RestoreContext();
}

void drawInstruct()
{
	GD.ClearColorRGB(0xffffff); 
	GD.Clear();
	GD.SaveContext();
	GD.ColorRGB(0xff78ae);
	GD.cmd_text(GD.w/2,(GD.h/2)-30,26,OPT_CENTER, "Move Hoppy left or right by touching the screen. It's raining pizza!");
	GD.cmd_text((GD.w/2),(GD.h/2),26,OPT_CENTER, "Collect as many as you can so that Hoppy can survive");
	GD.cmd_text((GD.w/2),(GD.h/2)+30,26,OPT_CENTER,"the harsh winter ahead.");
	GD.ColorRGB(0xff78ae);
	GD.Tag(BUTTON_GOAL); GD.cmd_text((GD.w/2)-1,180+1,26,OPT_CENTER, "PRESS TO PLAY");
	GD.ColorRGB(0x6b1f53);
	GD.Tag(BUTTON_GOAL); GD.cmd_text(GD.w/2,180,26,OPT_CENTER, "PRESS TO PLAY");
	drawBackButton();
	GD.RestoreContext();
}

void returnMenu()
{
	GD.SaveContext();
	GD.ColorRGB(0x000000);
	GD.cmd_loadidentity();
	GD.cmd_scale(F16(0.7), F16(0.7));
	GD.cmd_setmatrix();
	GD.Tag(BUTTON_MENU); 
	GD.cmd_text(4, 70, 26, OPT_CENTERY, "Menu");
	GD.RestoreContext();
}

void Score()
{
	GD.SaveContext();
	GD.ColorRGB(0x000000);
	GD.cmd_loadidentity();
	GD.cmd_scale(F16(0.7), F16(0.7));
	GD.cmd_setmatrix();
	char string[50];
	sprintf(string, "Pizzas collected: %d",counter);
	GD.cmd_text(4, 20, 26, OPT_CENTERY, string);
	GD.RestoreContext();
}

void background()
{
	GD.cmd_loadidentity();
	GD.cmd_scale(F16(0.4), F16(0.4));
	GD.cmd_setmatrix();
	GD.Vertex2ii(0,0,GRASSY_PLAINS_BY_THEODENN_HANDLE);
}

void youWin()
{
	GD.ClearColorRGB(0xffffff);
	GD.Clear();
	GD.SaveContext();
	GD.ColorA(255);
	GD.ColorRGB(0xffffff);
	GD.Begin(BITMAPS);
	GD.BitmapHandle(BUNNYPIZZAMENU_HANDLE);
	GD.Vertex2f(0,0);
	GD.ColorRGB(0xff0066); 
	GD.cmd_text((GD.w/2)-3,(GD.h/2)-20,31, OPT_CENTER, "YOU WIN!");
	GD.cmd_text((GD.w/2)+3,(GD.h/2)-20,31, OPT_CENTER, "YOU WIN!");
	GD.cmd_text((GD.w/2),(GD.h/2)-23,31, OPT_CENTER, "YOU WIN!");
	GD.cmd_text((GD.w/2),(GD.h/2)-17,31, OPT_CENTER, "YOU WIN!");
	GD.ColorRGB(0x00ffae);
	GD.cmd_text(GD.w/2,(GD.h/2)-20,31, OPT_CENTER,"YOU WIN!");
	GD.ColorRGB(0xff78ae);
	GD.cmd_text((GD.w/2),(GD.h/2)+10,26,OPT_CENTER, "Hoppy can survive the winter!");
	GD.ColorRGB(0x6b1f53);
	GD.cmd_text((GD.w/2)-1,(GD.h/2)+11,26,OPT_CENTER, "Hoppy can survive the winter!");
	GD.ColorA(alpha);
	alpha-=1;
	GD.ColorRGB(0xff78ae);
	GD.Tag(BUTTON_GOAL); GD.cmd_text((GD.w/2)-1,180+1,26,OPT_CENTER, "THE NEXT DAY...");
	GD.ColorRGB(0x6b1f53);
	GD.Tag(BUTTON_GOAL); GD.cmd_text(GD.w/2,180,26,OPT_CENTER, "THE NEXT DAY...");	
	counter = 0;
	goal_time = millis();
	GD.RestoreContext();
}

cl_timer eaten_potion(10000); //Duration of scale effect after potion
cl_timer eaten_strawberry(10000); //Duration of scale effect after strawberry
cl_timer strawberry_time(straw_random);
cl_timer second_strawberry(straw2_random);
cl_timer potion_time(potion_random);
cl_timer second_potion(potion2_random);

void LevelOneGoal()
{
	GD.SaveContext();
	GD.ClearColorRGB(0xffffff);
	GD.Clear();
	GD.ColorRGB(0xff0066); 
	GD.cmd_text((GD.w/2)-3,(GD.h/2)-40,30, OPT_CENTER, "GOAL");
	GD.cmd_text((GD.w/2)+3,(GD.h/2)-40,30, OPT_CENTER, "GOAL");
	GD.cmd_text((GD.w/2),(GD.h/2)-43,30, OPT_CENTER, "GOAL");
	GD.cmd_text((GD.w/2),(GD.h/2)-37,30, OPT_CENTER, "GOAL");
	GD.ColorRGB(0x00ffae);
	GD.cmd_text(GD.w/2,(GD.h/2)-40,30,OPT_CENTER, "GOAL");
	GD.ColorRGB(0xff1fe9);
	GD.cmd_text((GD.w/2),(GD.h/2)+10,26,OPT_CENTER, "Collect 60 pizzas. Be careful! If you reach");
	GD.cmd_text((GD.w/2),(GD.h/2)+30,26,OPT_CENTER, "-5 points, Hoppy will starve.");
	GD.ColorRGB(0xffb0f7);
	GD.cmd_text((GD.w/2)-1,(GD.h/2)+11,26,OPT_CENTER, "Collect 60 pizzas. Be careful! If you reach");
	GD.cmd_text((GD.w/2)-1,(GD.h/2)+31,26,OPT_CENTER, "-5 points, Hoppy will starve.");
	GD.RestoreContext(); 
	for (int i=0; i<numOfDrops;i++) //Resets pizzas
	{		
		drops[i]->reset();
	}
	strawberry->state = WAITING_STRAWBERRY;
	potion->state = WAITING_POTION;
	strawberry_time.reset();		//Resets time of fall for the special items
	second_strawberry.reset();
	potion_time.reset();
	second_potion.reset();
	strawberry->start();
	potion->start();
	collided = 0;	
}

void gameOver()
{
	GD.ClearColorRGB(0x000000);
	GD.Clear();
	GD.SaveContext();
	GD.Begin(BITMAPS);
	GD.BitmapHandle(BUNNY_FLAT_HANDLE);
	GD.Vertex2f(0,16*60);
	GD.ColorRGB(0xd9dadb); 
	GD.cmd_text((GD.w/2)-3,(GD.h/2)-20,31, OPT_CENTER, "GAME OVER");
	GD.cmd_text((GD.w/2)+3,(GD.h/2)-20,31, OPT_CENTER, "GAME OVER");
	GD.cmd_text((GD.w/2),(GD.h/2)-23,31, OPT_CENTER, "GAME OVER");
	GD.cmd_text((GD.w/2),(GD.h/2)-17,31, OPT_CENTER, "GAME OVER");
	GD.ColorRGB(0x5a5d63);
	GD.cmd_text(GD.w/2,(GD.h/2)-20,31, OPT_CENTER, "GAME OVER");
	GD.ColorA(alpha);
	alpha-=1;
	GD.ColorRGB(0xff78ae);
	GD.Tag(BUTTON_GOAL); GD.cmd_text((GD.w/2)-1,180+1,26,OPT_CENTER, "REVIVE HOPPY");
	GD.ColorRGB(0x6b1f53);
	GD.Tag(BUTTON_GOAL); GD.cmd_text(GD.w/2,180,26,OPT_CENTER, "REVIVE HOPPY");	
	GD.ColorA(255);
	GD.ColorRGB(0xff78ae);
	GD.Tag(BUTTON_GOAL); GD.cmd_text((GD.w/2)-1,180+19,26,OPT_CENTER, "QUIT");
	GD.ColorRGB(0x6b1f53);
	GD.Tag(BUTTON_GOAL); GD.cmd_text(GD.w/2,180+20,26,OPT_CENTER, "QUIT");
	GD.RestoreContext();
	counter = 0;
	goal_time = millis();
}

bool bunnyDirection=true;
bool flag_hit=false;
uint32_t last_display_update=0;
boolean strawberry_flag = false;
boolean potion_flag = false;

void setup()
{
	pinMode(23, OUTPUT);
	pinMode(12, OUTPUT);
	digitalWrite(12, HIGH);
	digitalWrite(23, HIGH);
	Serial.begin(115200);
	SPI.begin(5, 15, 14);
	GD.begin();
	LOAD_ASSETS();
	for (int i=0; i<numOfDrops;i++)
	{
		drops[i] = new Drop(); //Recalculate Drop class for each drop 
	}
	potion = new Potion();
	strawberry = new strawBerry();
	randomSeed(analogRead(0));
	}

	int bunny_x = 0; //One sprite
	int bunny_y = 160;

	void bunnyAnimation(){
		if (GD.inputs.x>0 && GD.inputs.x<160)
		{
			bunny_x = (bunny_x - 1)%340; //-1 is direction of bunny walk (bunny walks left)
			GD.Vertex2ii(bunny_x, bunny_y, (bunny_x/8)%3); // /8 is how many pixels between changing the cell -> %3 is how many iterations or switches you are doing. . -> . -> . -> .
			bunnyDirection=false;
		}

		if (GD.inputs.x>160 && GD.inputs.x<320)
		{ 
			bunny_x = (bunny_x+1)%340; //+1 to change direction of bunny walk (bunny walks right)
			GD.Vertex2ii(bunny_x, bunny_y , 4+(bunny_x/8)%3); 
			bunnyDirection = true;
		}
		if (GD.inputs.x < 0)
		{
			if	(bunnyDirection==false)
				{
				GD.Vertex2ii(bunny_x,bunny_y,BUNNY9_HANDLE);
				}
			if	(bunnyDirection == true)
				{
				GD.Vertex2ii(bunny_x,bunny_y,BUNNY10_HANDLE);
				}
		}
			/*Screen border boundaries*/
		if (bunny_x < 1 && GD.inputs.x > 0 && GD.inputs.x<160)  //1 instead of 0 to mitigate glitch
		{
			bunny_x = 1;
			GD.Vertex2ii(bunny_x, bunny_y, (millis()/100)%3);
		}
		if (bunny_x > (320-BUNNY9_WIDTH/1.5) && GD.inputs.x > 160 && GD.inputs.x < 320)
		{
			bunny_x = (320-BUNNY9_WIDTH/1.5);
			GD.Vertex2ii(bunny_x, bunny_y, 4+(millis()/100)%3);
		}
}

int numOfPotions = 0;
int numOfStrawberries = 0;

void drawFruit()
{
	GD.cmd_loadidentity();
	GD.cmd_scale(F16(1), F16(1));
	GD.cmd_setmatrix();
	maxDistance = 30;
	if (potion_time.check()) 
	{ 
		if(potion->state == WAITING_POTION && numOfPotions < 10)
		{
			potion->start(); //set flag_start = true to bring state to falling
			++numOfPotions;
		}
		if (potion->state == FALLING_POTION)
		{
			if (collide(potion->x, potion->y, bunny_x, bunny_y) == true && potion->y < 190)
			{	
				potion->destroy(); //sets flag destroy to true to bring state to destroy (before landed state)
				collided = 1;
				second_potion.reset();
				eaten_potion.reset();
			} 
				second_potion.reset();
		}

	if (potion->state == DESTROY_POTION && second_potion.check())
	{
		potion->state = WAITING_POTION;
		}
	
	potion->process(); // begin process by waiting
	potion->draw();	 //begin draw function
	}
	GD.cmd_loadidentity();
	GD.cmd_scale(F16(0.8), F16(0.8));
	GD.cmd_setmatrix();
	maxDistance = 30;
	if (strawberry_time.check()) //if 5 s passed since pizza landed
	{ 
		if(strawberry->state == WAITING_STRAWBERRY && numOfStrawberries < 4)
		{
			strawberry->start(); //set flag_start = true to bring state to falling
			++numOfStrawberries;
		}
		if (strawberry->state == FALLING_STRAWBERRY)
		{
			if (collide(strawberry->x, strawberry->y, bunny_x, bunny_y) == true && strawberry->y < 190)
			{	
				strawberry->destroy(); //sets flag destroy to true to bring state to destroy (before landed state)
				collided = 2;
				second_strawberry.reset();
				eaten_strawberry.reset();
			} 
				second_strawberry.reset();
		}
		if (strawberry->state == DESTROY_STRAWBERRY && second_strawberry.check())
		{
			strawberry->state = WAITING_STRAWBERRY;
		}
			strawberry->process(); 
			strawberry->draw();	
	}
	if (collided == 0)
	{
		maxDistance = 20;
		bunny_y = 160;
		GD.cmd_loadidentity();
		GD.cmd_scale(F16(0.6), F16(0.6));
		GD.cmd_setmatrix();
	}
	else if (collided == 1)
	{
		if (!eaten_potion.check()) 
		{ 
			potion_flag = true;
		}	
		else 
		{ 
			potion_flag = false;
		}
		
		if (potion_flag == true)
		{
			maxDistance = 1.2*20;
			bunny_y = 140;
			GD.cmd_loadidentity();
			GD.cmd_scale(F16(1), F16(1));
			GD.cmd_setmatrix();
		}

		if (potion_flag == false)
		{
			maxDistance = 20;
			bunny_y = 160;
			GD.cmd_loadidentity();
			GD.cmd_scale(F16(0.6), F16(0.6));
			GD.cmd_setmatrix();
		}
	}
	else if (collided == 2)
	{
		if (!eaten_strawberry.check())
		{ 
			strawberry_flag = true;
		}	
		else
		{  					
			strawberry_flag = false;
		}
		
		if (strawberry_flag == true)
		{
			maxDistance = 0.4 * maxDistance;
			bunny_y = 165;
			GD.cmd_loadidentity();
			GD.cmd_scale(F16(0.4), F16(0.4));
			GD.cmd_setmatrix();
		}
		if (strawberry_flag == false)
		{
			maxDistance = 20;
			bunny_y = 160;
			GD.cmd_loadidentity();
			GD.cmd_scale(F16(0.6), F16(0.6));
			GD.cmd_setmatrix();
		}
	}
	GD.ColorA(255);
	bunnyAnimation();
}

cl_timer last_touch(100);

void loop()
{ 
	GD.get_inputs();
	cl_timer last_touch(1000); //time between touch triggers should be at least 1 second
	if (GD.inputs.tag != 0){ //If tag is being touched
		if (last_touch.check())
		{
			last_touch.reset();  
			switch(GD.inputs.tag)
			{
			case BUTTON_MENU:
			page_display = PAGE_MENU;
			break;

			case BUTTON_INSTRUCT:
			page_display = PAGE_INFO;
			break;

			case BUTTON_GOAL:
			page_display = PAGE_GOAL;
			break;
			
			case BUTTON_PLAY2:
			page_display = PAGE_PLAY;
			break;

			case BUTTON_GAMEOVER:
			page_display = PAGE_GAMEOVER;
			break;

			case BUTTON_YOUWIN:
			page_display = PAGE_YOUWIN;
			break;
			}
		}
}

	


switch (page_display)
{
	case PAGE_MENU:
		drawMenu();
		break;
	
	case PAGE_PLAY:
		GD.Clear();
		GD.Begin(BITMAPS);
		background();
		Score();
		GD.ColorRGB(0xffffff);//Must change the color to white, otherwise the previous color (green) will affect all other drawings
		GD.cmd_loadidentity();
		GD.cmd_scale(F16(0.4), F16(0.4));
		GD.cmd_setmatrix();
		for (int i=0; i<numOfDrops;i++)
		{
			drops[i]->fall(); //speed and fall animation is simulated here
			drops[i]->show1(); //pizzas are drawn with this function
			if (collide(drops[i]->x, drops[i]->y, bunny_x,bunny_y)==true && drops[i]->y <190)
			{
				drops[i]->reset();
				hit_time = millis();
				counter++;
			}	
			if (drops[i]->y > 190)
			{
				counter -= 1;
				hit_time2 = millis();
			}
		}

		if (millis() - hit_time < 200)
		{
			GD.ColorA(255);
			GD.cmd_text(100,100,31,OPT_CENTER,"1 POINT!"); 
		}
		if (millis() - hit_time2 < 200)
		{
			GD.ColorA(255);
			GD.ColorRGB(0x8a3e3e);
			GD.cmd_text(200,130,31,OPT_CENTER,"-1 POINT :(");
		}
		GD.ColorRGB(0xffffff);
		drawFruit();
		if (counter <= -5)
		{
			page_display = PAGE_GAMEOVER;
		}
			
		if (counter == 100)
		{
			page_display = PAGE_YOUWIN;
		}
		returnMenu(); //draw return to menu button
		break;

		case PAGE_INFO:
			drawInstruct();
			break;

		case PAGE_GOAL:
			if (millis() - goal_time < 4000)
			{
				LevelOneGoal();
			}
			else
			{
				page_display = PAGE_PLAY;
			}
			break;

		case PAGE_YOUWIN:
			youWin();
			bunny_x = 0;
			break;

		case PAGE_GAMEOVER:
			gameOver();
			bunny_x = 0;
			break;
		}
		GD.swap();
		delay(10);
}







