//#include <GD3.h>
#ifndef CLCSHMISTRUCTS
#define CLCSHMISTRUCTS
#include <stdint.h>
#define SHADOW 3
#define RADIUS 5

char buff[10];

enum passwordscreentype {PASS_USER,PASS_MAINT,PASS_USER_NEW_1, PASS_USER_NEW_2, PASS_MAINT_1, PASS_MAINT_2, PASS_FAULT_SCREEN};

enum HMIcolours {
	BLACK,
	WHITE,
	BLUE,
	D_BLUE,
	RED,
	GREEN,
	YELLOW,
	L_GREY,
	D_GREY,
	ORANGE,
	B_BLUE,
	TEAL,
	C_SHADOW,
	D_RED,
	L_TEAL,
	VL_GREY,
	VD_GREY

};

uint32_t gc(uint8_t _colour)
{
      switch (_colour)
      {
                case BLACK  : return 0x000000;
                case WHITE  : return 0xFFFFFF;
                case BLUE   : return 0x3498DB;
                case D_BLUE : return 0x215968;
                case RED    : return 0xE74C3C;
                case D_RED  : return 0xC0392B;
                case GREEN  : return 0x2ECC71;
                case YELLOW : return 0xF1C40F;
                case L_GREY : return 0xBDC3C7;
                case D_GREY : return 0x95A5A6;
                case ORANGE : return 0xE67E22;
                case B_BLUE : return 0x00B0F0;
                case TEAL   : return 0x106055; //0x1FBBA6
                case C_SHADOW : return 0xd7d7d7;
                case L_TEAL : return 0x18baa4;
                case VL_GREY : return 0xECF0F1;
                case VD_GREY : return 0x3b3c3e;
				default : return 0x000000;
      }
}


//  DRAW SHADOWED RECTANGLE
//  function draws a shadowed rectangle (no rounded edges) with an offset SHADOW in the X and Y coordinates. This rectangle also has a border of 1px
//    param[x] - X offset from the upper left of the screen
//    param[y] - Y offset from the upper left of the screen.
//    param[w] - the width of the rectangle. This value does not include the shadow.
//    param[h] - the heigth of the rectangle. This value does not include the shadow.
//    param[colour] - the colour of the rectangle.


// void gd_switch_shape(uint8_t _shape)
// {
//   //  if(_shape != GD_draw_shape || _shape == LINES || _shape == LINE_STRIP)
//   //  {
//           GD.Begin(_shape);
//   //        GD_draw_shape = _shape;
//         //  Serial.print("u - ");
//   //  }
//   //  Serial.print(GD_draw_shape);
//   //  Serial.print("\t");
//   //  Serial.println(_shape);
// }

//  DRAW RECTANGLE
//  function draws a rounded rectangle. This does not have a border
//    param[x] - X offset from the upper left of the screen
//    param[y] - Y offset from the upper left of the screen.
//    param[w] - the width of the rectangle.
//    param[h] - the heigth of the rectangle.
//    param[colour] - the colour of the rounded rectangle.

void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t colour)
{

        GD.Begin(RECTS);
        GD.ColorRGB(gc(colour));
        GD.Vertex2f(x, y);
        GD.Vertex2f(x+w, y+h);

}

void drawShadowed(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t colour)
{
        drawRect(x+SHADOW, y+SHADOW, w-2*SHADOW, h, C_SHADOW);
        drawRect(x, y, w, h, D_GREY);
        drawRect(x+1, y+1, w-2, h-2, colour);
}

//  DRAW ROUNDED RECTANGLE
//  function draws a rounded rectangle. This does not have a border
//    param[x] - X offset from the upper left of the screen
//    param[y] - Y offset from the upper left of the screen.
//    param[w] - the width of the rectangle.
//    param[h] - the heigth of the rectangle.
//    param[colour] - the colour of the rounded rectangle.

void drawRoundedRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t colour)
{
        GD.LineWidth(RADIUS*16);
        drawRect(x+RADIUS, y+RADIUS, w-2*RADIUS, h-2*RADIUS, colour);
        GD.LineWidth(1*16);

}

//  DUAL TOGGLE TEXT BUTTON
//  function draws a shadowed rectangle button with two lines of text
//          _______________
//          |   HEADING   |
//          |             |
//          |  TXT1/TXT2  |
//          |_____________|
//
//    param[x] - X offset from the upper left of the screen
//    param[y] - Y offset from the upper left of the screen.
//    param[w] - the width of the rectangle. This value does not include the shadow.
//    param[h] - the heigth of the rectangle. This value does not include the shadow.
//    param[colour1] - the colour of the button when in the LOW state
//    param[colour2] - the colour of the button when in the ACTIVE state
//    param[txt] - the header text of the button. This does not change with button status
//    param[txt1] - the text of the button in the LOW state
//    param[txt2] - the text of the button in the ACTIVE state
//
//    function[draw(_state)] draws the button in either the LOW or ACTIVE state determined by
//                           the value of _state input.

struct buttonDual {
        char tag;
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;
        uint8_t colour1;
        uint8_t colour2;
        char txt[10];
        char txt1[10];
        char txt2[10];

        void draw(char _state)
        {
                GD.Tag(tag);

                GD.Begin(LINES);
              	GD.ColorRGB(gc(_state == 0 ? colour1 : colour2));
              	GD.LineWidth(8*h);
              	GD.Vertex2f(x+(h/2), y+(h/2));
              	GD.Vertex2f(x+w-(h/2), y+(h/2));


              		GD.ColorRGB(gc(WHITE));
              		GD.LineWidth(8*(h-1));
              		GD.Vertex2f(x+(h/2), y+(h/2));
              		GD.Vertex2f(x+w-(h/2), y+(h/2));

              	  //if(_tag == _in->tag) drawRoundedRect(_in->x, _in->y, _in->w, _in->h, YELLOW);
              	  //else drawRoundedRect(_in->x, _in->y, _in->w, _in->h, _in->colour);
              	  GD.ColorRGB(gc(_state == 0 ? colour1 : colour2));


              	GD.LineWidth(16);

                GD.cmd_text(x+w/2, y+15, 27, OPT_CENTER, txt);
                if(_state == 0) GD.cmd_text(x+w/2, y+40, 29, OPT_CENTER, txt1);
                else GD.cmd_text(x+w/2, y+40, 29, OPT_CENTER, txt2);
        }
};

//  DUAL TOGGLE TEXT BUTTON
//  function draws a shadowed rectangle button with two lines of text
//          _______________
//          |   HEADING   |
//          |             |
//          |   12.345    |
//          |             |
//          |    Teach    |
//          |_____________|
//
//    param[tag] - the tag ID of the object getting drawn. Any touch input from the FT813 has an associated ID
//    param[x] - X offset from the upper left of the screen
//    param[y] - Y offset from the upper left of the screen.
//    param[w] - the width of the rectangle. This value does not include the shadow.
//    param[h] - the heigth of the rectangle. This value does not include the shadow.
//    param[colour] - the colour of the button when in the LOW state
//    param[txt] - the header text of the button. This field is used to describe the position that will get taught
//
//    function[draw(_tag, _value)] draws the button in either the LOW or ACTIVE pressstate determined by
//                           the value of _tag input. When _tag equals the tag of the object, it draws the
//                           the button as pressed. The value displayed uses sprintf to convert the float
//                           into a char array that can get printed on the screen.

struct buttonTeach {
        char tag;
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;
        uint8_t colour;
        char txt[20];

        void draw(char _tag, float _value, boolean _state)
        {
                GD.Tag(tag);

                if(_tag == tag) drawRoundedRect(x, y, w, h, YELLOW);
                else if(_state)
                {
                  drawRoundedRect(x, y, w, h, GREEN);
                  //drawRoundedRect(x+4, y+4, w-8, h-8, L_GREY);
                }
                else drawRoundedRect(x, y, w, h, colour);
                GD.ColorRGB(gc(WHITE));
                sprintf(buff, "%4.2f",_value);
                //  dtostrf(_value, 5, 2, buff);
                GD.cmd_text(x+w/2, y+h/2, 31, OPT_CENTER, buff);
                GD.cmd_text(x+w/2, y+20, 27, OPT_CENTER, txt);
                GD.cmd_text(x+w/2, y+h-20, 27, OPT_CENTER, "Press to Teach");
        }
};


struct num_field_highlight{
        char tag;
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;
        //uint8_t colour;

        void draw(boolean _state, int16_t _num)
        {
              GD.Tag(tag);
              drawRoundedRect(x,y,w,h, BLACK);
              if(_state)
              {
                drawRoundedRect(x+1, y+1, w-2, h-2, B_BLUE);
                drawRoundedRect(x+4, y+4, w-8, h-8, WHITE);
              }
              else drawRoundedRect(x+2, y+2, w-4, h-4, WHITE);

              sprintf(buff, "%d",_num);
              GD.ColorRGB(gc(BLACK));
              GD.cmd_text(x+w/2, y+h/2, 29, OPT_CENTER, buff);


        }

};

//  Teach Header
//  function draws a shadowed rectangle button with two lines of text
//          _______________
//          |   HEADING   |
//          |             |
//          |   100ms     |
//          |_____________|
//
//    param[tag] - the tag ID of the object getting drawn. Any touch input from the FT813 has an associated ID
//    param[x] - X offset from the upper left of the screen
//    param[y] - Y offset from the upper left of the screen.
//    param[w] - the width of the rectangle. This value does not include the shadow.
//    param[h] - the heigth of the rectangle. This value does not include the shadow.
//    param[colour] - the colour of the button when in the LOW state
//    param[txt] - the header text of the button. This field is used to describe the position
//    param[unit] - this button uses the unit to define whether the float value drawn and the associated units are
//                  either ms or PSI. This changes the text as well as the significant digits displayed
//
//    function[draw(_value)] The value displayed uses sprintf to convert the float
//                           into a char array that can get printed on the screen.

struct teachHeader {
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;
        uint8_t colour;
        char txt[25];
        int8_t unit;

        void draw(int16_t _value)
        {
                GD.Tag(0);

                //GD.Begin(POINTS);
                GD.Begin(POINTS);
                GD.ColorRGB(gc(colour));

                GD.PointSize(RADIUS*16);
                GD.Vertex2f(x+RADIUS,y+RADIUS);
                GD.Vertex2f(x+w-RADIUS,y+RADIUS);
                //GD.Begin(RECTS);
                GD.Begin(RECTS);
                GD.Vertex2f(x, y+RADIUS);
                GD.Vertex2f(x+w, y+h);
                GD.Vertex2f(x+RADIUS, y);
                GD.Vertex2f(x+w-RADIUS, y+h);
                GD.ColorRGB(gc(WHITE));
                if(unit == 0) sprintf(buff, "%d ms", _value);
                else sprintf(buff, "%d PSI", _value);
                GD.cmd_text(x+w/2, y+60, 30, OPT_CENTER, buff);
                GD.cmd_text(x+w/2, y+20, 27, OPT_CENTER, txt);
        }
};

//  Buttom TEACH button (ie + -)
//  function draws a partially rounded button with either a plus or minus
//          ___________
//          |         |
//          |   +-    |
//          \         |
//           \________|
//
//    param[tag] - the tag ID of the object getting drawn. Any touch input from the FT813 has an associated ID
//    param[x] - X offset from the upper left of the screen
//    param[y] - Y offset from the upper left of the screen.
//    param[w] - the width of the rectangle. This value does not include the shadow.
//    param[h] - the heigth of the rectangle. This value does not include the shadow.
//    param[colour] - the colour of the button when in the LOW state
//    param[txt] - the header text of the button. This field is used to describe the position that will get taught
//    param[side] - determines which side type is getting drawn (left or right). Left is minus symbol. Right is the plus symbol
//
//    function[draw(_tag)] draws the button in either the LOW or ACTIVE pressstate determined by
//                           the value of _tag input. When _tag equals the tag of the object, it draws the
//                           the button as pressed.
struct buttonTeachbottom {
        char tag;
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;
        uint8_t colour;
        bool side;
        char txt[2];

        void draw(char _tag)
        {
                GD.Tag(tag);

                //GD.Begin(POINTS);
                GD.Begin(POINTS);
                if(_tag == tag) GD.ColorRGB(gc(YELLOW));
                else GD.ColorRGB(gc(colour));

                GD.PointSize(RADIUS*16);
                GD.Vertex2f(x+RADIUS,y+h-RADIUS);
                GD.Vertex2f(x+w-RADIUS,y+h-RADIUS);
                //GD.Begin(RECTS);
                GD.Begin(RECTS);
                GD.Vertex2f(x, y);
                GD.Vertex2f(x+w, y+h-RADIUS);
                if(side)
                {
                        GD.Vertex2f(x, y);
                        GD.Vertex2f(x+w-RADIUS, y+h);
                }
                else
                {
                        GD.Vertex2f(x+RADIUS, y);
                        GD.Vertex2f(x+w, y+h);
                }


                GD.ColorRGB(gc(WHITE));
                GD.cmd_text(x+w/2, y+h/2, 31, OPT_CENTER, txt);
        }
};

struct backButton
{
    char tag;
    int16_t x;
    int16_t y;
    uint8_t bg;
    uint8_t fg;

    void draw(char _tag)
    {
          GD.Tag(tag);
          drawRect(x, y, 80, 80,  _tag == tag ? fg : bg);
          GD.ColorRGB(gc(_tag != tag ? fg : bg));

          GD.Begin(LINE_STRIP);
          GD.LineWidth(3*16);
          GD.Vertex2f(x+55, y+20);
          GD.Vertex2f(x+25, y+40);
          GD.Vertex2f(x+55, y+60);
          GD.LineWidth(1*16);
    }
};

//  Square Button
//  function draws a square button with visually centered text
//          _______________
//          |             |
//          |     txt     |
//          |_____________|
//
//    param[tag] - the tag ID of the object getting drawn. Any touch input from the FT813 has an associated ID
//    param[x] - X offset from the upper left of the screen
//    param[y] - Y offset from the upper left of the screen.
//    param[w] - the width of the rectangle. This value does not include the shadow.
//    param[h] - the heigth of the rectangle. This value does not include the shadow.
//    param[font] - the font number. This refers to which font array is getting used from the FT813
//    param[colour] - the colour of the button when in the LOW state
//    param[txt] - the header text of the button. This field is used to describe the position
//
//    function[draw(_tag)] draws the button in either the LOW or ACTIVE pressstate determined by
//                           the value of _tag input. When _tag equals the tag of the object, it draws the
//                           the button as pressed.
struct buttonSq {
        char tag;
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;
        uint8_t font;
        uint8_t colour;
        char txt[12];

        void draw(char _tag)
        {
                GD.Tag(tag);
                //GD.Begin(RECTS);
                GD.Begin(RECTS);
                if(_tag == tag) GD.ColorRGB(gc(YELLOW));
                else GD.ColorRGB(gc(colour));

                GD.Vertex2f(x, y);
                GD.Vertex2f(x+w-1, y+h-1);
                GD.ColorRGB(gc(WHITE));
                GD.cmd_text(x+w/2, y+h/2, font, OPT_CENTER, txt);

        }
        void load(char _tag)
        {
                GD.Tag(tag);
                //GD.Begin(RECTS);
                GD.Begin(RECTS);
                if(_tag == tag) GD.ColorRGB(gc(YELLOW));
                else GD.ColorRGB(gc(colour));

                GD.Vertex2f(x, y);
                GD.Vertex2f(x+w-1, y+h-1);
                GD.ColorRGB(gc(WHITE));
                GD.cmd_text(x+w/2, y+h/2, font, OPT_CENTER, txt);

        }
        void update(char _tag)
        {
          if(_tag == tag)
          {
                GD.Tag(tag);
                //GD.Begin(RECTS);
                GD.Begin(RECTS);
                if(_tag == tag) GD.ColorRGB(gc(YELLOW));
                else GD.ColorRGB(gc(colour));

                GD.Vertex2f(x, y);
                GD.Vertex2f(x+w-1, y+h-1);
                GD.ColorRGB(gc(WHITE));
                GD.cmd_text(x+w/2, y+h/2, font, OPT_CENTER, txt);
            }
        }
};

//  Rounded Button
//  function draws a square button with visually centered text
//            ____________
//           /            \   //
//          |     txt      | 
//           \____________/   //
//
//    param[tag] - the tag ID of the object getting drawn. Any touch input from the FT813 has an associated ID
//    param[x] - X offset from the upper left of the screen
//    param[y] - Y offset from the upper left of the screen.
//    param[w] - the width of the rectangle. This value does not include the shadow.
//    param[h] - the heigth of the rectangle. This value does not include the shadow.
//    param[font] - the font number. This refers to which font array is getting used from the FT813
//    param[colour] - the colour of the button when in the LOW state
//    param[txt] - the header text of the button. This field is used to describe the position
//
//    function[draw(_tag)] draws the button in either the LOW or ACTIVE pressstate determined by
//                           the value of _tag input. When _tag equals the tag of the object, it draws the
//                           the button as pressed.

struct button_line {
        char tag;
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;
        uint8_t font;
        uint8_t colour;
        char* txt;
};

struct button {
        char tag;
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;
        uint8_t font;
        uint8_t colour;
        char txt[24];

        void draw(char _tag)
        {
                GD.Tag(tag);
                if(_tag == tag) drawRoundedRect(x, y, w, h, YELLOW);
                else drawRoundedRect(x, y, w, h, colour);
                GD.ColorRGB(gc(WHITE));
                GD.cmd_text(x+w/2, y+h/2, font, OPT_CENTER, txt);

        }
        void update(char _tag)
        {
                if(_tag == tag)
                {
                      GD.Tag(tag);
                      drawRoundedRect(x, y, w, h, YELLOW);
                      GD.ColorRGB(gc(WHITE));
                      GD.cmd_text(x+w/2, y+h/2, font, OPT_CENTER, txt);
                }
        }
        void load(char _tag)
        {
                GD.Tag(tag);
                if(_tag == tag) drawRoundedRect(x, y, w, h, YELLOW);
                else drawRoundedRect(x, y, w, h, colour);
                GD.ColorRGB(gc(WHITE));
                GD.cmd_text(x+w/2, y+h/2, font, OPT_CENTER, txt);
        }

};

void draw(const button_line *_in, char _tag)
{
  GD.Tag(_in->tag);

	GD.Begin(LINES);
	GD.ColorRGB(gc(_in->colour));
	GD.LineWidth(8*_in->h);
	GD.Vertex2f(_in->x+(_in->h/2), _in->y+(_in->h/2));
	GD.Vertex2f(_in->x+_in->w-(_in->h/2), _in->y+(_in->h/2));

	if(_tag != _in->tag)
	{
		GD.ColorRGB(gc(WHITE));
		GD.LineWidth(8*(_in->h-1));
		GD.Vertex2f(_in->x+(_in->h/2), _in->y+(_in->h/2));
		GD.Vertex2f(_in->x+_in->w-(_in->h/2), _in->y+(_in->h/2));

	  //if(_tag == _in->tag) drawRoundedRect(_in->x, _in->y, _in->w, _in->h, YELLOW);
	  //else drawRoundedRect(_in->x, _in->y, _in->w, _in->h, _in->colour);
	  GD.ColorRGB(gc(_in->colour));
	 // GD.cmd_text(_in->x+_in->w/2, _in->y+_in->h/2, _in->font, OPT_CENTER, _in->txt);
	}
	else
	{
		GD.ColorRGB(gc(WHITE));
	}
	GD.cmd_text(_in->x+_in->w/2, _in->y+_in->h/2, _in->font, OPT_CENTER, _in->txt);
	GD.LineWidth(16);
}

void update(const button_line *_in, char _tag)
{
  if(_tag == _in->tag)
  {
		GD.Tag(_in->tag);

		GD.Begin(LINES);
		GD.ColorRGB(gc(_in->colour));
		GD.LineWidth(8*_in->h);
		GD.Vertex2f(_in->x+(_in->h/2), _in->y+(_in->h/2));
		GD.Vertex2f(_in->x+_in->w-(_in->h/2), _in->y+(_in->h/2));


	  //if(_tag == _in->tag) drawRoundedRect(_in->x, _in->y, _in->w, _in->h, YELLOW);
	  //else drawRoundedRect(_in->x, _in->y, _in->w, _in->h, _in->colour);
	  GD.ColorRGB(gc(WHITE));
	  GD.cmd_text(_in->x+_in->w/2, _in->y+_in->h/2, _in->font, OPT_CENTER, _in->txt);
		GD.LineWidth(16);
  }
}

struct button2 {
        char tag;
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;
        uint8_t font;
        uint8_t colour1;
        uint8_t colour2;
        char txt[24];

        void draw(char _tag)
        {
                GD.Tag(tag);
                if(_tag == tag) drawRoundedRect(x, y, w, h, colour2);
                else drawRoundedRect(x, y, w, h, colour1);
                GD.ColorRGB(gc(WHITE));
                GD.cmd_text(x+w/2, y+h/2, font, OPT_CENTER, txt);

        }
};

//  Rounded 2 Line Button
//  function draws a square button with visually centered text
//            ____________
//           /            \  //
//          |     txt1     |
//          |     txt2     |
//           \____________/  //
//
//    param[tag] - the tag ID of the object getting drawn. Any touch input from the FT813 has an associated ID
//    param[x] - X offset from the upper left of the screen
//    param[y] - Y offset from the upper left of the screen.
//    param[w] - the width of the rectangle. This value does not include the shadow.
//    param[h] - the heigth of the rectangle. This value does not include the shadow.
//    param[font] - the font number. This refers to which font array is getting used from the FT813
//    param[colour] - the colour of the button when in the LOW state
//    param[txt1] - the first line of text of the button.
//    param[txt2] - the second line of text of the button.
//
//    function[draw(_tag)] draws the button in either the LOW or ACTIVE pressstate determined by
//                           the value of _tag input. When _tag equals the tag of the object, it draws the
//                           the button as pressed.

struct button2line {
        char tag;
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;
        uint8_t font;
        uint8_t colour;
        char txt1[12];
        char txt2[24];

        void draw(char _tag)
        {
                GD.Tag(tag);
                GD.Begin(LINES);
                GD.ColorRGB(gc(colour));
                GD.LineWidth(8*h);
                GD.Vertex2f(x+(h/2), y+(h/2));
                GD.Vertex2f(x+w-(h/2), y+(h/2));

                if(_tag != tag)
                {
                  GD.ColorRGB(gc(WHITE));
                  GD.LineWidth(8*(h-1));
                  GD.Vertex2f(x+(h/2), y+(h/2));
                  GD.Vertex2f(x+w-(h/2), y+(h/2));

                  //if(_tag == _in->tag) drawRoundedRect(_in->x, _in->y, _in->w, _in->h, YELLOW);
                  //else drawRoundedRect(_in->x, _in->y, _in->w, _in->h, _in->colour);
                  GD.ColorRGB(gc(colour));
                 // GD.cmd_text(_in->x+_in->w/2, _in->y+_in->h/2, _in->font, OPT_CENTER, _in->txt);
                }
                else
                {
                  GD.ColorRGB(gc(WHITE));
                }
                GD.cmd_text(x+w/2, y-11+h/2, font, OPT_CENTER, txt1);
                GD.cmd_text(x+w/2, y+11+h/2, font, OPT_CENTER, txt2);
                GD.LineWidth(16);
        }
};

//  Run Indicator
//  function draws a square indicator field with two states (LOW or HIGH)
//          _______________
//          |             |
//          |     txt     |
//          |_____________|
//
//    param[tag] - the tag ID of the object getting drawn. Any touch input from the FT813 has an associated ID
//    param[x] - X offset from the upper left of the screen
//    param[y] - Y offset from the upper left of the screen.
//    param[w] - the width of the rectangle. This value does not include the shadow.
//    param[h] - the heigth of the rectangle. This value does not include the shadow.
//    param[font] - the font number. This refers to which font array is getting used from the FT813
//    param[colour] - the colour of the button when in the LOW state
//    param[txt] - the header text of the button. This field is used to describe the position
//
//    function[draw(_state)] draws the button in either the LOW or ACTIVE pressstate determined by
//                           the value of _state input.

struct drawRunIndicator
{
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;
        char txt[20];

        void draw(boolean _state)
        {
                GD.Tag(0);
              //  GD.Begin(RECTS);
              GD.Begin(RECTS);
                if(_state == 0)
                {
                        GD.ColorRGB(gc(BLACK));
                }
                else
                {
                        drawRect(x, y, w, h, B_BLUE);
                        GD.ColorRGB(gc(WHITE));
                }
                GD.cmd_text(x+w/2, y+h/2, 29, OPT_CENTER, txt);
        }

};
//  DRAW ROUNDED SHADOWED RECTANGLE
//  function draws a shadowed rectangle (no rounded edges) with an offset SHADOW in the X and Y coordinates. This rectangle also has a border of 1px
//    param[x] - X offset from the upper left of the screen
//    param[y] - Y offset from the upper left of the screen.
//    param[w] - the width of the rectangle. This value does not include the shadow.
//    param[h] - the heigth of the rectangle. This value does not include the shadow.
//    param[colour] - the colour of the rectangle.
void drawRoundedRectWithShadow(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t colour, uint8_t shadowColour, int shadowRadius)
{
        drawRoundedRect(x-shadowRadius, y-shadowRadius, w+2*shadowRadius, h+2*shadowRadius, shadowColour);
        drawRoundedRect(x, y, w, h, colour);
}



void drawWifiStrength(int16_t x, int16_t y, int _strength)
{
        //GD.Begin(RECTS);
        GD.Begin(RECTS);
        int strength[5] = {-110, -100,-86,-70,1};
        if(_strength < strength[0]) GD.ColorRGB(0xE74C3C);
        else if(_strength < strength[1]) GD.ColorRGB(0xE74C3C);
        else if(_strength < strength[2]) GD.ColorRGB(0xE67E22);
        else if(_strength < strength[4]) GD.ColorRGB(0x2ECC71);
        else GD.ColorRGB(0x2ECC71);
        for(int i = 0; i < 5; i++)
        {

                GD.Vertex2f(x+10*i, y);
                GD.Vertex2f(x+5+10*i, y-10-10*i);
                if(_strength < strength[i]) GD.ColorRGB(0xBDC3C7);
        }

}

struct keyboardIcon {
        char tag;
        int16_t x;
        int16_t y;
        uint8_t colour;

        void draw(char _tag)
        {
                GD.Tag(tag);
                GD.Begin(RECTS);
                if(_tag == tag) GD.ColorRGB(gc(YELLOW));
                else GD.ColorRGB(gc(colour));
                GD.Vertex2f(x, y);
                GD.Vertex2f(x+50-1, y+50-1);

                GD.Begin(BITMAPS);
                GD.ColorRGB(0xFFFFFF);
                GD.BitmapHandle(2);
                GD.Vertex2f(x+10, y+10);
        }
        void load(char _tag)
        {
                GD.Tag(tag);
                GD.Begin(RECTS);
                if(_tag == tag) GD.ColorRGB(gc(YELLOW));
                else GD.ColorRGB(gc(colour));
                GD.Vertex2f(x, y);
                GD.Vertex2f(x+50-1, y+50-1);

                GD.Begin(BITMAPS);
                GD.ColorRGB(0xFFFFFF);
                GD.BitmapHandle(2);
                GD.Vertex2f(x+10, y+10);
        }
        void update(char _tag)
        {
                if(_tag == tag)
                {
                      GD.Tag(tag);
                      GD.Begin(RECTS);
                      if(_tag == tag) GD.ColorRGB(gc(YELLOW));
                      else GD.ColorRGB(gc(colour));
                      GD.Vertex2f(x, y);
                      GD.Vertex2f(x+50-1, y+50-1);

                      GD.Begin(BITMAPS);
                      GD.ColorRGB(0xFFFFFF);
                      GD.BitmapHandle(2);
                      GD.Vertex2f(x+10, y+10);
                }

        }

};

void drawScalingPositionWarning(uint8_t type)
{
      int16_t offset_y = 210 + 140 * type;

      drawRoundedRect(20, offset_y, 440, 40, RED );
      GD.ColorRGB(0xFFFFFF);
      GD.cmd_text(90,offset_y+20, 28, OPT_CENTERY, "Positions taught to the same value");
      GD.Begin(BITMAPS);
      GD.ColorRGB(0xFFFFFF);
      GD.BitmapHandle(1);
      GD.Vertex2f(30, offset_y+5);

}

void drawBarHorizontal(uint16_t _x, uint16_t _y, uint16_t _w, uint16_t _h, uint16_t _val, uint16_t _min, uint16_t _max, uint32_t _colour)
{
  GD.Begin(LINES);
  uint16_t y = _y;
  GD.ColorRGB(gc(D_GREY));
  GD.LineWidth((_h/2)*16);
  GD.Vertex2f(_x+_h/2, y);
  GD.Vertex2f(_x+_w-_h/2, y);
  GD.ColorRGB(gc(WHITE));
  GD.LineWidth(((_h-2)/2)*16);
  GD.Vertex2f(_x+_h/2, y);
  GD.Vertex2f(_x+_w-_h/2, y);

  GD.ColorRGB(_colour);


  uint16_t val = map(_val, _min, _max, 0, _w-_h);
  GD.LineWidth(((_h-6)/2)*16);
  GD.Vertex2f(_x+_h/2, y);
  GD.Vertex2f(_x+_h/2+val, y);
  GD.LineWidth(1*16);
}
#endif
