#include <Arduino.h>
#include <list>
#include <SPI.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include "GD3.h"
#include "cl_plc_objects.h"
#include "clcs_hmi_objects.h"
#include "clcs_hmi_structs.h"
#include "default_assets.h"

static uint32_t hit_time2 = millis() - 5000;
static uint32_t goal_time = millis()-2000;
static uint32_t leveldown_time = millis() - 2000;
static uint32_t levelup_time = millis() - 2000;
int bunny_x = 0; //One sprite
int bunny_y = 160;
int counter = 0;
boolean flag_fade=false;
int alpha2=0;
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
	float y = random(-600,-10);
	
	void reset(){
		alpha=255;
		y=random(-600,-10);
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
	//static int z=random(10,18); DO NOT place this here! Place it in before the access specifier to achieve different falling pizza bitmaps
	GD.Vertex2ii(x,y,z);
	}
};

class levelUp{
	float yspeed = random(1,3); 
	int z = 20;
	int alpha = 255;
public:
	float x = random(0,290);
	float y = random(-600,-10);
	
	void reset(){
		alpha=255;
		y=random(-600,-10);
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
		if (alpha<5){
			y=-100;
		}
	}

	void show1(){
	GD.ColorA(alpha);
	GD.Vertex2ii(x,y,z);
	}
};

class levelDown{
	float yspeed = random(1,3); 
	int z = 21;
	int alpha = 255;
public:
	float x = random(0,290);
	float y = random(-600,-10);
	
	void reset(){
		alpha=255;
		y=random(-600,-10);
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
		if (alpha<5){
			y=-100;
		}
	}

	void show1(){
	GD.ColorA(alpha);
	GD.Vertex2ii(x,y,z);
	}
};
const int numOflevelups=1;
levelUp* levelup[numOflevelups];

const int numOfleveldowns=2;
levelDown* leveldown[numOfleveldowns];

const int numOfDrops=23;
Drop* drops[numOfDrops]; //Drop d -> one pizza. Array makes more than one pizza fall

boolean collide(float x1, float y1,float x2,float y2){ //say pizza is (x1,y1) and bunny is (x2,y2)
	//Centre coordinate calculations
	float pizzaCentreX = x1 + BBQ1_WIDTH/2;
	float pizzaCentreY = y1 + BBQ1_HEIGHT/2;
	float bunnyCentreX = x2 + BUNNY1_WIDTH/2;
	float bunnyCentreY = y2 + BUNNY1_HEIGHT/2;
	float distance =hypot(bunnyCentreX-pizzaCentreX,bunnyCentreY-pizzaCentreY);
	if (distance < 25){
		return true;	
	}
	return false;
}

/*void boxCollide(int x, int y, int bitmap_width){
	if (x < 0) {
		x = (x-1)%340;
		GD.Vertex2ii(0, y, (x/8)%3);
	}
	if (x > 320 - bitmap_width) {
		x = (x+1)%340;
		GD.Vertex2ii(320 - bitmap_width, y,(x/8)%3);
	}
}*/

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

void drawMenu(){
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
GD.ColorA(alpha2);
alpha2-=2;
GD.ColorRGB(0xff78ae);
GD.Tag(BUTTON_GOAL); GD.cmd_text((GD.w/2)-1,180+1,22,OPT_CENTER, "PRESS TO PLAY");
GD.ColorA(255);
GD.Tag(BUTTON_INSTRUCT); GD.cmd_text((GD.w/2)-1,200+1,22,OPT_CENTER,"HOW TO PLAY");
GD.ColorA(alpha2);
alpha2-=1;
GD.ColorRGB(0x6b1f53);
GD.Tag(BUTTON_GOAL); GD.cmd_text(GD.w/2,180,22,OPT_CENTER, "PRESS TO PLAY");
GD.ColorA(255);
GD.Tag(BUTTON_INSTRUCT); GD.cmd_text(GD.w/2,200,22,OPT_CENTER,"HOW TO PLAY");
goal_time = millis();
GD.RestoreContext();
for (int i=0; i<numOfDrops;i++){
drops[i]->reset();
}
}

void drawBackButton() {
GD.SaveContext();
GD.ColorRGB(0xff78ae);
GD.Tag(BUTTON_MENU); GD.cmd_text((GD.w/2)-1,200+1,20,OPT_CENTER, "BACK"); //GO back button
GD.ColorRGB(0x6b1f53);
GD.Tag(BUTTON_MENU); GD.cmd_text(GD.w/2,200,20,OPT_CENTER,"BACK");
GD.RestoreContext();
}

void drawInstruct()
{
GD.ClearColorRGB(0xffffff); 
GD.Clear();
GD.SaveContext();
GD.ColorRGB(0xff78ae);
GD.cmd_text(GD.w/2,(GD.h/2)-30,20,OPT_CENTER, "Move Hoppy left or right by touching the screen. It's raining pizza!");
GD.cmd_text((GD.w/2),(GD.h/2),20,OPT_CENTER, "Avoid the falling pizzas to save Hoppy.");
GD.ColorRGB(0xff78ae);
GD.Tag(BUTTON_GOAL); GD.cmd_text((GD.w/2)-1,180+1,20,OPT_CENTER, "PRESS TO PLAY");
GD.ColorRGB(0x6b1f53);
GD.Tag(BUTTON_GOAL); GD.cmd_text(GD.w/2,180,20,OPT_CENTER, "PRESS TO PLAY");
drawBackButton();
GD.RestoreContext();
}

void returnMenu(){
GD.SaveContext();
GD.ColorRGB(0x000000);
GD.cmd_loadidentity();
GD.cmd_scale(F16(1), F16(1));
GD.cmd_setmatrix();
GD.Tag(BUTTON_MENU); 
GD.cmd_text(4, 70, 20, OPT_CENTERY, "Menu");
GD.RestoreContext();
}

void Score(){
GD.SaveContext();
GD.ColorRGB(0x000000);
GD.cmd_loadidentity();
GD.cmd_scale(F16(1), F16(1));
GD.cmd_setmatrix();
char string[50];
sprintf(string, "Pizzas avoided: %d",counter);
GD.cmd_text(4, 20, 20, OPT_CENTERY, string);
GD.RestoreContext();
}

void background(){
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
GD.cmd_text((GD.w/2),(GD.h/2)+10,21,OPT_CENTER, "Hoppy can survive the winter!");
GD.ColorRGB(0x6b1f53);
GD.cmd_text((GD.w/2)-1,(GD.h/2)+11,21,OPT_CENTER, "Hoppy can survive the winter!");
GD.ColorA(alpha2);
alpha2-=1;
GD.ColorRGB(0xff78ae);
GD.Tag(BUTTON_GOAL); GD.cmd_text((GD.w/2)-1,180+1,20,OPT_CENTER, "THE NEXT DAY...");
GD.ColorRGB(0x6b1f53);
GD.Tag(BUTTON_GOAL); GD.cmd_text(GD.w/2,180,20,OPT_CENTER, "THE NEXT DAY...");	
counter = 0;
goal_time = millis() - 2000;
for (int i=0; i<numOfDrops;i++){
drops[i]->reset();
}
GD.RestoreContext();
}

void LevelOneGoal(){
GD.ClearColorRGB(0xffffff);
GD.Clear();
GD.SaveContext();
GD.ColorRGB(0xff0066); 
GD.cmd_text((GD.w/2)-3,(GD.h/2)-40,30, OPT_CENTER, "GOAL");
GD.cmd_text((GD.w/2)+3,(GD.h/2)-40,30, OPT_CENTER, "GOAL");
GD.cmd_text((GD.w/2),(GD.h/2)-43,30, OPT_CENTER, "GOAL");
GD.cmd_text((GD.w/2),(GD.h/2)-37,30, OPT_CENTER, "GOAL");
GD.ColorRGB(0x00ffae);
GD.cmd_text(GD.w/2,(GD.h/2)-40,30,OPT_CENTER, "GOAL");
GD.ColorRGB(0xff1fe9);
GD.cmd_text((GD.w/2),(GD.h/2)+10,21,OPT_CENTER, "Hoppy will be crushed under the pizzas!");
GD.cmd_text((GD.w/2),(GD.h/2)+30,21,OPT_CENTER, "Avoid 100 pizzas to pass.");
GD.ColorRGB(0xffb0f7);
GD.cmd_text((GD.w/2)-1,(GD.h/2)+11,21,OPT_CENTER, "Hoppy will be crushed under the pizzas!");
GD.cmd_text((GD.w/2)-1,(GD.h/2)+31,21,OPT_CENTER, "Avoid 100 pizzas to pass.");
GD.RestoreContext(); 
}

void gameOver(){
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
GD.ColorA(alpha2);
alpha2-=1;
GD.ColorRGB(0xff78ae);
GD.Tag(BUTTON_GOAL); GD.cmd_text((GD.w/2)-1,180+1,20,OPT_CENTER, "REVIVE HOPPY");
GD.ColorRGB(0x6b1f53);
GD.Tag(BUTTON_GOAL); GD.cmd_text(GD.w/2,180,20,OPT_CENTER, "REVIVE HOPPY");	
GD.ColorA(255);
GD.ColorRGB(0xff78ae);
GD.Tag(BUTTON_GOAL); GD.cmd_text((GD.w/2)-1,180+19,20,OPT_CENTER, "QUIT");
GD.ColorRGB(0x6b1f53);
GD.Tag(BUTTON_GOAL); GD.cmd_text(GD.w/2,180+20,20,OPT_CENTER, "QUIT");	
counter = 0;
goal_time = millis() - 2000;
for (int i=0; i<numOfDrops;i++){
drops[i]->reset();}
GD.RestoreContext();
}


void setup(){
pinMode(23, OUTPUT);
pinMode(12, OUTPUT);
digitalWrite(12, HIGH);
digitalWrite(23, HIGH);
Serial.begin(115200);
SPI.begin(5, 15, 14);
GD.begin();
LOAD_ASSETS();
for (int i=0; i<numOfDrops;i++){
	drops[i] = new Drop(); //Recalculate Drop class for each drop 
	}
for (int i=0; i<numOfleveldowns;i++){
	leveldown[i] = new levelDown(); //Recalculate Drop class for each drop 
	}
for (int i=0; i<numOflevelups;i++){
	levelup[i] = new levelUp(); //Recalculate Drop class for each drop 
	}
}

bool bunnyDirection=true;
bool flag_hit=false;
uint32_t last_display_update=0;

void loop()
{ 
GD.get_inputs();

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
	GD.cmd_scale(F16(1), F16(1));
	GD.cmd_setmatrix();
	for (int i=0; i<numOfDrops;i++){
	drops[i]->fall(); //speed and fall animation is simulated here
	drops[i]->show1(); //pizzas are drawn with this function
		if (collide(drops[i]->x, drops[i]->y, bunny_x,bunny_y)==true && drops[i]->y <190){
			
			page_display = PAGE_GAMEOVER;
		}	
		if (drops[i]->y > 190){
			counter += 1;
			hit_time2 = millis();
		}
	}
	if (millis() - leveldown_time < 20000){
	for (int i=0; i<numOfleveldowns;i++){
	leveldown[i]->fall(); //speed and fall animation is simulated here
	leveldown[i]->show1();
	}
	}

	if (millis() - levelup_time < 20000){
	for (int i=0; i<numOflevelups;i++){
	levelup[i]->fall(); //speed and fall animation is simulated here
	levelup[i]->show1();
	}
	}
/* 	if (millis() - hit_time < 200)
		{
		GD.ColorA(255);
		GD.cmd_text(100,100,31,OPT_CENTER,"1 POINT!"); 
		}
	if (millis() - hit_time2 < 200)
		{
		GD.ColorA(255);
		GD.ColorRGB(0x8a3e3e);
		GD.cmd_text(200,130,31,OPT_CENTER,"-1 POINT :(");
		} */


	if (counter == 50)
	{
		page_display = PAGE_YOUWIN;
	}
	GD.ColorRGB(0xffffff);
	
	
	GD.cmd_loadidentity();
	GD.cmd_scale(F16(1), F16(1));
	GD.cmd_setmatrix();

	GD.ColorA(255);
	//boxCollide(bunny_x,bunny_y,BUNNY1_WIDTH);

	if (GD.inputs.x>0 && GD.inputs.x<160)
	{	
		bunny_x = (bunny_x - 1)%340; //-1 is direction of bunny walk (bunny walks left)
		GD.Vertex2ii(bunny_x, bunny_y, (bunny_x/8)%3); //320-a -> bitmap sequence runs R to L. bunny_y -> constant y coordinate. 1+(a/2) -> starting from 1 instead of zero, also /2 is how many pixels
										//between changing the cell -> %3 is how many iterations or switches you are doing. . -> . -> . -> .
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

	returnMenu();
	break;

case PAGE_INFO:
	drawInstruct();
	break;

case PAGE_GOAL:
	if (millis() - goal_time < 4000){
	LevelOneGoal();
	}
	else {
	page_display = PAGE_PLAY;}
	break;

case PAGE_YOUWIN:
	youWin();
	break;

case PAGE_GAMEOVER:
	gameOver();
	break;

	}
	GD.swap();
	delay(10);
}







