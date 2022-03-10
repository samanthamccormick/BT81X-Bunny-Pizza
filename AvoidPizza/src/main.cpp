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
cl_timer last_touch(100);
cl_timer eaten_blueberry(5000);
cl_timer eaten_strawberry(5000);
cl_timer last_blueberry(10000);
cl_timer last_strawberry(10000);
int bunny_x = 0; //One sprite
int bunny_y = 160;
int counter = 0;
int maxDistance = 20;
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
	float yspeed = random(0.5,1); //pizzas start at different speeds above the screen
	int z = random(10,16); //Each pizza is a random pizza for the duration of its fall
	int alpha = 255;
public:
	float x = random(0,290);
	float y = random(-200,-50); //Start off the screen
	
	void reset(){
		alpha=255;
		y=random(-200,-50); //Start off the screen
		x=random(0,290);
		yspeed += random(0.5,1.5);
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
public:
	int alpha = 255;
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
		else{
			y=190;
			yspeed=0;
			alpha-=1;
		}
		if (alpha<5){
		alpha = 0;
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
public:
	int alpha = 255;
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
		if (alpha<5){
			alpha = 0;
		}
	}

	void show1(){
	GD.ColorA(alpha);
	GD.Vertex2ii(x,y,z);
	}
};
const int numOflevelups=1; //when I increase this to 2, bunny animation speed doubles
levelUp* levelup[numOflevelups];

const int numOfleveldowns=2;
levelDown* leveldown[numOfleveldowns];

const int numOfDrops= 30;
Drop* drops[numOfDrops]; //Drop d -> one pizza. Array makes more than one pizza fall

boolean collide(float x1, float y1,float x2,float y2){ //say pizza is (x1,y1) and bunny is (x2,y2)
	//Centre coordinate calculations
	float pizzaCentreX = x1 + BBQ1_WIDTH/2;
	float pizzaCentreY = y1 + BBQ1_HEIGHT/2;
	float bunnyCentreX = x2 + BUNNY1_WIDTH/2;
	float bunnyCentreY = y2 + BUNNY1_HEIGHT/2;
	float distance =hypot(bunnyCentreX-pizzaCentreX,bunnyCentreY-pizzaCentreY);
	if (distance < maxDistance){
		return true;	
	}
	return false;
}

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
GD.Tag(BUTTON_GOAL); GD.cmd_text((GD.w/2)-1,180+1,26,OPT_CENTER, "PRESS TO PLAY");
GD.ColorA(255);
GD.Tag(BUTTON_INSTRUCT); GD.cmd_text((GD.w/2)-1,200+1,26,OPT_CENTER,"HOW TO PLAY");
GD.ColorA(alpha2);
alpha2-=1;
GD.ColorRGB(0x6b1f53);
GD.Tag(BUTTON_GOAL); GD.cmd_text(GD.w/2,180,26,OPT_CENTER, "PRESS TO PLAY");
GD.ColorA(255);
GD.Tag(BUTTON_INSTRUCT); GD.cmd_text(GD.w/2,200,26,OPT_CENTER,"HOW TO PLAY");
goal_time = millis();
GD.RestoreContext();
for (int i=0; i<numOfDrops;i++){
drops[i]->reset();
}
for (int i=0; i<numOflevelups;i++){
levelup[i]->reset();
}
for (int i=0; i<numOfleveldowns;i++){
leveldown[i]->reset();
}
}

void drawBackButton() {
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
GD.cmd_text((GD.w/2),(GD.h/2),26,OPT_CENTER, "Avoid the falling pizzas to save Hoppy.");
GD.ColorRGB(0xff78ae);
GD.Tag(BUTTON_GOAL); GD.cmd_text((GD.w/2)-1,180+1,26,OPT_CENTER, "PRESS TO PLAY");
GD.ColorRGB(0x6b1f53);
GD.Tag(BUTTON_GOAL); GD.cmd_text(GD.w/2,180,26,OPT_CENTER, "PRESS TO PLAY");
drawBackButton();
GD.RestoreContext();
}

void returnMenu(){
GD.SaveContext();
GD.ColorRGB(0x000000);
GD.cmd_loadidentity();
GD.cmd_scale(F16(0.7), F16(0.7));
GD.cmd_setmatrix();
GD.Tag(BUTTON_MENU); 
GD.cmd_text(4, 70, 26, OPT_CENTERY, "Menu");
GD.RestoreContext();
}

void Score(){
GD.SaveContext();
GD.ColorRGB(0x000000);
GD.cmd_loadidentity();
GD.cmd_scale(F16(0.7), F16(0.7));
GD.cmd_setmatrix();
char string[50];
sprintf(string, "Pizzas avoided: %d",counter);
GD.cmd_text(4, 20, 26, OPT_CENTERY, string);
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
GD.cmd_text((GD.w/2),(GD.h/2)+10,26,OPT_CENTER, "Hoppy can survive the winter!");
GD.ColorRGB(0x6b1f53);
GD.cmd_text((GD.w/2)-1,(GD.h/2)+11,26,OPT_CENTER, "Hoppy can survive the winter!");
GD.ColorA(alpha2);
alpha2-=1;
GD.ColorRGB(0xff78ae);
GD.Tag(BUTTON_GOAL); GD.cmd_text((GD.w/2)-1,180+1,26,OPT_CENTER, "THE NEXT DAY...");
GD.ColorRGB(0x6b1f53);
GD.Tag(BUTTON_GOAL); GD.cmd_text(GD.w/2,180,26,OPT_CENTER, "THE NEXT DAY...");	
counter = 0;
goal_time = millis() - 2000;
for (int i=0; i<numOfDrops;i++){
	drops[i]->reset();
}
for (int i=0; i<numOflevelups;i++){
	levelup[i]->reset();
}
for (int i=0; i<numOfleveldowns;i++){
	leveldown[i]->reset();
}
GD.RestoreContext();
}

void LevelOneGoal(){
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
GD.cmd_text((GD.w/2),(GD.h/2)+10,26,OPT_CENTER, "Hoppy will be crushed under the pizzas!");
GD.cmd_text((GD.w/2),(GD.h/2)+30,26,OPT_CENTER, "Avoid 200 pizzas to pass.");
GD.ColorRGB(0xffb0f7);
GD.cmd_text((GD.w/2)-1,(GD.h/2)+11,26,OPT_CENTER, "Hoppy will be crushed under the pizzas!");
GD.cmd_text((GD.w/2)-1,(GD.h/2)+31,26,OPT_CENTER, "Avoid 200 pizzas to pass.");
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
GD.Tag(BUTTON_GOAL); GD.cmd_text((GD.w/2)-1,180+1,26,OPT_CENTER, "REVIVE HOPPY");
GD.ColorRGB(0x6b1f53);
GD.Tag(BUTTON_GOAL); GD.cmd_text(GD.w/2,180,26,OPT_CENTER, "REVIVE HOPPY");	
GD.ColorA(255);
GD.ColorRGB(0xff78ae);
GD.Tag(BUTTON_MENU); GD.cmd_text((GD.w/2)-1,180+19,26,OPT_CENTER, "QUIT");
GD.ColorRGB(0x6b1f53);
GD.Tag(BUTTON_MENU); GD.cmd_text(GD.w/2,180+20,26,OPT_CENTER, "QUIT");	
GD.RestoreContext();
counter = 0;
//goal_time = millis() - 2000;
for (int i=0; i<numOfDrops;i++){
drops[i]->reset();}
for (int i=0; i<numOflevelups;i++){
levelup[i]->reset();
}
for (int i=0; i<numOfleveldowns;i++){
leveldown[i]->reset();
}
}
bool bunnyDirection=true;
bool flag_hit=false;
uint32_t last_display_update=0;
boolean levelup_flag = false;
boolean leveldown_flag = false;
boolean fruit_flag;



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

	if (bunny_x < 1 && GD.inputs.x > 0 && GD.inputs.x<160) { //1 instead of 0 to mitigate glitch
		bunny_x = 1;
		GD.Vertex2ii(bunny_x, bunny_y, (millis()/20)%3);
	}
	if (bunny_x < 0 && GD.inputs.x < 0) {
		bunny_x = 0;
		GD.Vertex2ii(bunny_x,bunny_y,BUNNY9_HANDLE);
	}

	if (bunny_x > (320-BUNNY9_WIDTH/1.5) && GD.inputs.x > 160 && GD.inputs.x < 320){
		bunny_x = (320-BUNNY9_WIDTH/1.5);
		GD.Vertex2ii(bunny_x, bunny_y, 4+(millis()/20)%3);
	}
	if (bunny_x > (320-BUNNY9_WIDTH/1.5) && GD.inputs.x < 0){
		bunny_x = (320-BUNNY9_WIDTH/1.5);
		GD.Vertex2ii(bunny_x,bunny_y,BUNNY10_HANDLE);
	}

}
boolean flagb;
boolean flags;

void drawFruit(){
	GD.SaveContext();

	
	GD.cmd_loadidentity();
	GD.cmd_scale(F16(0.8), F16(0.8));
	GD.cmd_setmatrix();
/*  	if (last_blueberry.check()){
		flagb = true;
	}
	else {
		flagb = false;
	}
	if (flagb == true){  */
		for (int i=0; i<numOfleveldowns;i++){
		leveldown[i]->fall(); //speed and fall animation is simulated here
		leveldown[i]->show1(); //pizzas are drawn with this function
		last_blueberry.reset();
		if (collide(leveldown[i]->x, leveldown[i]->y, bunny_x,bunny_y)==true && leveldown[i]->y <190){
			leveldown[i]->alpha = 0;
			eaten_blueberry.reset();
			fruit_flag = false;
			}
		}
	
	GD.cmd_loadidentity();
	GD.cmd_scale(F16(0.8), F16(0.8));
	GD.cmd_setmatrix();
	maxDistance = 30;
/* 	if (last_strawberry.check()){
	flags = true;
	}	
	else {
		flags = false;
	}
	if (flags ==true){  */
		for (int i=0; i<numOflevelups;i++){
		levelup[i]->fall(); //speed and fall animation is simulated here
		levelup[i]->show1(); //pizzas are drawn with this function
		last_strawberry.reset(); //Time since last strawberry == 0;
		if (collide(levelup[i]->x, levelup[i]->y, bunny_x,bunny_y)==true && levelup[i]->y <190){
			levelup[i]->alpha = 0;
			eaten_strawberry.reset();
			fruit_flag = true;
			}
		}
	
	if (fruit_flag == false){
		if (!eaten_blueberry.check()){ //if timer is less than 10 s
			leveldown_flag = true;
		}	
		else { //else if timer is >= 10 s
			leveldown_flag = false;
		}
		GD.cmd_loadidentity();
		if (leveldown_flag == true){
			maxDistance = 1.2*20;
			bunny_y = 155;
			GD.cmd_scale(F16(1), F16(1));
		}
		if (leveldown_flag == false){
			maxDistance = 20;
			bunny_y = 160;
			GD.cmd_scale(F16(0.6), F16(0.6));
			}
	}
	
	if (fruit_flag == true){
		if (!eaten_strawberry.check()){ //if timer is less than 10 s
			levelup_flag = true;
		}	
		else {  					//else if timer is >= 10 s
			levelup_flag = false;
		}
		GD.cmd_loadidentity();
		if (levelup_flag == true){
			maxDistance = 0.4 * maxDistance;
			bunny_y = 165;
			GD.cmd_scale(F16(0.4), F16(0.4));
		}
		if (levelup_flag == false){
			maxDistance = 20;
			bunny_y = 160;
			GD.cmd_scale(F16(0.6), F16(0.6));
		}
	}

	GD.cmd_setmatrix();
	GD.ColorA(255);
	bunnyAnimation();
	GD.RestoreContext();
}
void loop()
{ 
GD.get_inputs();
cl_timer last_touch(1000);

if (GD.inputs.tag != 0)
{
	if (last_touch.check()){
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
	{GD.Clear();
	GD.Begin(BITMAPS);
	background();
	Score();
	GD.ColorRGB(0xffffff);//Must change the color to white, otherwise the previous color will affect all other drawings
	GD.cmd_loadidentity();
	GD.cmd_scale(F16(0.4), F16(0.4));
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

	drawFruit();
	
	if (counter == 200)
	{
		page_display = PAGE_YOUWIN;
	}
	GD.ColorRGB(0xffffff);
	returnMenu();
	break;
	} 

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







