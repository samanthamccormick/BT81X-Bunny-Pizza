#include <Arduino.h>
#include "SPI.h"
#if !defined(__SAM3X8E__)
//#include "EEPROM.h"
#endif
#define VERBOSE             0
    #include <GD3.h>



#define BOARD_FTDI_80x      0
#define BOARD_FTDI_81X      1
#define BOARD_EVITA_0       2


#define BOARD               1   // board, from above
#define STORAGE             0   // Want SD storage?
#define CALIBRATION         1   // Want touchscreen?

// EVITA_0 has no storage or calibration
#if (BOARD == BOARD_EVITA_0)
// #undef STORAGE
// #define STORAGE          0
#undef CALIBRATION
#define CALIBRATION         0
#endif

// FTDI boards do not have storage
#if (BOARD == BOARD_FTDI_80x)
#undef STORAGE
#define STORAGE             1
#endif


#ifdef DUMPDEV
#include <assert.h>
#include "transports/dump.h"
#endif


byte ft8xx_model;

#include "wiring.h"


static GDTransport GDTR;

GDClass GD;


// The GD3 has a tiny configuration EEPROM - AT24C01D
// It is programmed at manufacturing time with the setup
// commands for the connected panel. The SCL,SDA lines
// are connected to thye FT81x GPIO0, GPIO1 signals.
// This is a read-only driver for it.  A single method
// 'read()' initializes the RAM and reads all 128 bytes
// into an array.

class ConfigRam {
private:
uint8_t gpio, gpio_dir, sda;
void set_SDA(byte n)
{
        if (sda != n) {
                GDTR.__wr16(REG_GPIO_DIR, gpio_dir | (0x03 - n)); // Drive SCL, SDA low
                sda = n;
        }
}

void set_SCL(byte n)
{
        GDTR.__wr16(REG_GPIO, gpio | (n << 1));
}

int get_SDA(void)
{
        return GDTR.__rd16(REG_GPIO) & 1;
}

void i2c_start(void)
{
        set_SDA(1);
        set_SCL(1);
        set_SDA(0);
        set_SCL(0);
}

void i2c_stop(void)
{
        set_SDA(0);
        set_SCL(1);
        set_SDA(1);
        set_SCL(1);
}

int i2c_rx1()
{
        set_SDA(1);
        set_SCL(1);
        byte r = get_SDA();
        set_SCL(0);
        return r;
}

void i2c_tx1(byte b)
{
        set_SDA(b);
        set_SCL(1);
        set_SCL(0);
}

int i2c_tx(byte x)
{
        for (byte i = 0; i < 8; i++, x <<= 1)
                i2c_tx1(x >> 7);
        return i2c_rx1();
}

int i2c_rx(int nak)
{
        byte r = 0;
        for (byte i = 0; i < 8; i++)
                r = (r << 1) | i2c_rx1();
        i2c_tx1(nak);
        return r;
}

public:
void read(byte *v)
{
        GDTR.__end();
        gpio = GDTR.__rd16(REG_GPIO) & ~3;
        gpio_dir = GDTR.__rd16(REG_GPIO_DIR) & ~3;
        sda = 2;

        // 2-wire software reset
        i2c_start();
        i2c_rx(1);
        i2c_start();
        i2c_stop();

        int ADDR = 0xa0;

        i2c_start();
        if (i2c_tx(ADDR))
                return;
        if (i2c_tx(0))
                return;

        i2c_start();
        if (i2c_tx(ADDR | 1))
                return;
        for (int i = 0; i < 128; i++) {
                *v++ = i2c_rx(i == 127);
                // Serial1.println(v[-1], DEC);
        }
        i2c_stop();
        GDTR.resume();
}
};

void GDClass::flush(void)
{
        GDTR.flush();
}

void GDClass::swap(void) {
        Display();
        cmd_swap();
        cmd_loadidentity();
        cmd_dlstart();
        GDTR.flush();
#ifdef DUMPDEV
        GDTR.swap();
#endif
}

uint32_t GDClass::measure_freq(void)
{
        unsigned long t0 = GDTR.rd32(REG_CLOCK);
        delayMicroseconds(15625);
        unsigned long t1 = GDTR.rd32(REG_CLOCK);
        // Serial1.println((t1 - t0) << 6);
        return (t1 - t0) << 6;
}

#define LOW_FREQ_BOUND  58800000UL
//#define LOW_FREQ_BOUND  47040000UL
// #define LOW_FREQ_BOUND  32040000UL

void GDClass::tune(void)
{
        uint32_t f;
        for (byte i = 0; (i < 31) && ((f = measure_freq()) < LOW_FREQ_BOUND); i++) {
                GDTR.wr(REG_TRIM, i);
        }
        GDTR.wr32(REG_FREQUENCY, f);
}


void GDClass::begin(uint8_t options) {
#if STORAGE && defined(ARDUINO)
        GDTR.begin0();

        if (options & GD_STORAGE) {
                GDTR.ios();
                SD.begin(SD_PIN);
        }
#endif

        GDTR.begin1();
#if 0
        Serial1.println("ID REGISTER:");
        Serial1.println(GDTR.rd(REG_ID), HEX);
#endif

#if (BOARD == BOARD_FTDI_80x)
        GDTR.wr(REG_PCLK_POL, 1);
        GDTR.wr(REG_PCLK, 2);

// Avoid inverted colours on alternative-FT800 boards
//#if PROTO == 1
//  GDTR.wr(REG_ROTATE, 1);
//  GDTR.wr(REG_SWIZZLE, 0);
//#endif
//    GDTR.wr(REG_ROTATE, 1);
// Avoid inverted colours on alternative-FT800 boards

#endif
        GDTR.wr(REG_PWM_DUTY, 0);
        GDTR.wr(REG_GPIO_DIR, 0x83);
        GDTR.wr(REG_GPIO, GDTR.rd(REG_GPIO) | 0x80);


#if (BOARD == BOARD_FTDI_81X)
        ConfigRam cr;
        byte v8[128] = {0};
        cr.read(v8);
        if ((v8[1] == 0xff) && (v8[2] == 0x01)) {
                options &= ~(GD_TRIM | GD_CALIBRATE);
                if (v8[3] & 2) {
                        GDTR.__end();
                        GDTR.hostcmd(0x44); // switch to external crystal
                        GDTR.resume();
                }
                copyram(v8 + 4, 124);
                finish();
        } else {

//https://github.com/MikroElektronika/mikromedia_HMI/blob/master/Mikromedia_HMI_Example/50/Mikromedia_HMI_50C_Demo_Code/mikroC%20PRO%20for%20FT90X/Mikromedia_HMI_50C_Demo_driver.c

                if (FT_810_ENABLE == 1) {
                        Serial1.println("FT810");
                        GDTR.wr32(REG_ROTATE, 1);
                        GD.wr32(REG_HCYCLE, 1056); // 900 //548
                        GD.wr32(REG_HOFFSET, 47); // 43
                        GD.wr32(REG_HSIZE, 800);
                        GD.wr32(REG_HSYNC0, 0);
                        GD.wr32(REG_HSYNC1, 10); // 41
                        GD.wr32(REG_VCYCLE, 525); // 500
                        GD.wr32(REG_VOFFSET, 23); // 12
                        GD.wr32(REG_VSIZE, 480);
                        GD.wr32(REG_VSYNC0, 0);
                        GD.wr32(REG_VSYNC1, 10);
                        GD.wr32(REG_PCLK, 2);
                        GD.wr32(REG_PCLK_POL, 1);
                        GD.wr32(REG_DITHER, 0); // 1
                        GD.wr32(REG_CSPREAD, 0); // 1
                }

                if (FT_811_ENABLE == 1) {
                        // MODIFICADO POR MI by LIGHTCALAMAR
                        Serial1.println("FT811");
                        GDTR.wr32(REG_ROTATE, 1);
                        GD.wr32(REG_HCYCLE, 1056); // 900 //548
                        GD.wr32(REG_HOFFSET, 46); // 43
                        GD.wr32(REG_HSIZE, 800);
                        GD.wr32(REG_HSYNC0, 0);
                        GD.wr32(REG_HSYNC1, 10); // 41
                        GD.wr32(REG_VCYCLE, 525); // 500
                        GD.wr32(REG_VOFFSET, 23); // 12
                        GD.wr32(REG_VSIZE, 480);
                        GD.wr32(REG_VSYNC0, 0);
                        GD.wr32(REG_VSYNC1, 10);
                        GD.wr32(REG_PCLK, 2);
                        GD.wr32(REG_PCLK_POL, 1);
                        GD.wr32(REG_DITHER, 0); // 1
                        GD.wr32(REG_CSPREAD, 0); // 1
                        // HASTA AQUI EL CODIGO SUPERIOR
                }

                if (FT_812_ENABLE == 1) {
                        Serial1.println("FT812");
                        GDTR.wr32(REG_ROTATE, 1);
                        GD.wr32(REG_HCYCLE, 1056); // 900 //548
                        GD.wr32(REG_HOFFSET, 47); // 43
                        GD.wr32(REG_HSIZE, 800);
                        GD.wr32(REG_HSYNC0, 0);
                        GD.wr32(REG_HSYNC1, 10); // 41
                        GD.wr32(REG_VCYCLE, 525); // 500
                        GD.wr32(REG_VOFFSET, 23); // 12
                        GD.wr32(REG_VSIZE, 480);
                        GD.wr32(REG_VSYNC0, 0);
                        GD.wr32(REG_VSYNC1, 10);
                        GD.wr32(REG_PCLK, 2);
                        GD.wr32(REG_PCLK_POL, 1);
                        GD.wr32(REG_DITHER, 0); // 1
                        GD.wr32(REG_CSPREAD, 0); // 1
                }

                if (FT_813_ENABLE == 1) {
                        Serial.println("FT813");
                        GDTR.wr32(REG_ROTATE, 2);

                        // Riverdi
                        // GD.wr32(REG_HCYCLE, 1056); // 900 //548
                        // GD.wr32(REG_HCYCLE, 1056); // 900 //548
                        // GD.wr32(REG_HOFFSET, 47); // 43
                        // GD.wr32(REG_HSIZE, 800);
                        // GD.wr32(REG_HSYNC0, 0);
                        // GD.wr32(REG_HSYNC1, 10); // 41
                        // GD.wr32(REG_VCYCLE, 525); // 500
                        // GD.wr32(REG_VOFFSET, 23); // 12
                        // GD.wr32(REG_VSIZE, 480);
                        // GD.wr32(REG_VSYNC0, 0);
                        // GD.wr32(REG_VSYNC1, 10);

                        // GD.wr32(REG_PCLK, 0);

                        // GD.wr32(REG_PCLK_POL, 1);
                        // GD.wr32(REG_DITHER, 0); // 1
                        // GD.wr32(REG_CSPREAD, 0); // 1


                        // Matrix Orbital
                        GD.wr32(REG_HSIZE, 800);                        
                        GD.wr32(REG_VSIZE, 480);    

                        GD.wr32(REG_HCYCLE, 928); // 900 //548
                        GD.wr32(REG_HOFFSET, 88); // 43
                        GD.wr32(REG_HSYNC0, 0);
                        GD.wr32(REG_HSYNC1, 48); // 41

                        GD.wr32(REG_VCYCLE, 525); // 500
                        GD.wr32(REG_VOFFSET, 32); // 12
                        GD.wr32(REG_VSYNC0, 0);
                        GD.wr32(REG_VSYNC1, 3);

                        GD.wr32(REG_PCLK, 0);                           
                        GD.wr32(REG_SWIZZLE, 0);                        
                        GD.wr32(REG_PCLK_POL, 1);                       
                        GD.wr32(REG_CSPREAD, 0); // 1                   
                    
                        GD.wr32(REG_DITHER, 1); // 1        

                        GD.wr32(REG_PWM_DUTY, 5);            

                        // GD.Clear();            
                        // GD.swap();

                }

                //CHANGE: Added bootup for BT_815_ENABLE
                if (BT_815_ENABLE == 1){
                        Serial.println("BT815");
                        GDTR.wr32(REG_ROTATE, 1);

						/* ///---- 7 inch
                        GD.wr32(REG_HCYCLE, 1056); // 900 //548
                        GD.wr32(REG_HOFFSET, 47); // 43
                        GD.wr32(REG_HSYNC0, 0);
                        GD.wr32(REG_HSYNC1, 10); // 41
                        GD.wr32(REG_VCYCLE, 525); // 500
                        GD.wr32(REG_VOFFSET, 23); // 12
                        GD.wr32(REG_VSYNC0, 0);
                        GD.wr32(REG_VSYNC1, 10);
                        GD.wr32(REG_SWIZZLE, 0);
                        GD.wr32(REG_PCLK_POL, 1);
                        GD.wr32(REG_DITHER, 0); // 1
                        GD.wr32(REG_CSPREAD, 0); // 1
                        GD.wr32(REG_HSIZE, 800);
                        GD.wr32(REG_VSIZE, 480);
                        GD.wr32(REG_PCLK, 2);

						/**/

						///---- 3.5 inch
						GD.wr32(REG_HCYCLE, 408); // 900 //548
                        GD.wr32(REG_HOFFSET, 70); // 43
                        GD.wr32(REG_HSYNC0, 0);
                        GD.wr32(REG_HSYNC1, 10); // 41
                        GD.wr32(REG_VCYCLE, 263); // 500
                        GD.wr32(REG_VOFFSET, 13); // 12
                        GD.wr32(REG_VSYNC0, 0);
                        GD.wr32(REG_VSYNC1, 2);
                        GD.wr32(REG_SWIZZLE, 2);
                        GD.wr32(REG_PCLK_POL, 1);
                        GD.wr32(REG_DITHER, 1); // 1
                        GD.wr32(REG_CSPREAD, 0); // 1
                        GD.wr32(REG_HSIZE, 320);
                        GD.wr32(REG_VSIZE, 240);
                        GD.wr32(REG_PCLK, 6);
                }
                /* End of change */


        }
#endif
        if (0) {
                GDTR.wr16(REG_HCYCLE, 928);
                GDTR.wr16(REG_HOFFSET, 88);
                GDTR.wr16(REG_HSIZE, 800);
                GDTR.wr16(REG_HSYNC0, 0);
                GDTR.wr16(REG_HSYNC1, 48);

                GDTR.wr16(REG_VCYCLE, 525);
                GDTR.wr16(REG_VOFFSET, 32);
                GDTR.wr16(REG_VSIZE, 480);
                GDTR.wr16(REG_VSYNC0, 0);
                GDTR.wr16(REG_VSYNC1, 3);

                GDTR.wr16(REG_CSPREAD, 0);
                GDTR.wr16(REG_DITHER, 1);
                GDTR.wr16(REG_PCLK_POL, 1);
                GDTR.wr16(REG_PCLK, 2);

//GD.wr32(REG_HCYCLE, 900);//548
//GD.wr32(REG_HOFFSET, 43);
//GD.wr32(REG_HSIZE, 800);
//GD.wr32(REG_HSYNC0, 0);
//GD.wr32(REG_HSYNC1, 41);

//GD.wr32(REG_VCYCLE, 500);
//GD.wr32(REG_VOFFSET, 12);
//GD.wr32(REG_VSIZE, 480);
//GD.wr32(REG_VSYNC0, 0);
//GD.wr32(REG_VSYNC1, 10);

//GD.wr32(REG_DITHER, 1);
//GD.wr32(REG_PCLK_POL, 1);//1
//GD.wr32(REG_PCLK, 1);//5
//GD.wr(REG_ROTATE, 0);
//GD.wr(REG_SWIZZLE, 0);//3 for GD2

        }

#if (BOARD == BOARD_EVITA_0)
        GDTR.wr16(REG_HCYCLE,  1344);
        GDTR.wr16(REG_HSIZE,   1024);
        GDTR.wr16(REG_HSYNC0,  0   );
        GDTR.wr16(REG_HSYNC1,  136 );
        GDTR.wr16(REG_HOFFSET, 136+160);
        GDTR.wr16(REG_VCYCLE,  806 );
        GDTR.wr16(REG_VSIZE,   768 );
        GDTR.wr16(REG_VSYNC0,  0   );
        GDTR.wr16(REG_VSYNC1,  6    );
        GDTR.wr16(REG_VOFFSET, 6+29  );
        GDTR.wr16(REG_CSPREAD, 0   );
        GDTR.wr16(REG_PCLK_POL,0   );
        GDTR.wr16(REG_PCLK,    1   );
        GDTR.wr(REG_GPIO, GDTR.rd(REG_GPIO) | 0x10);
#endif

        w = GDTR.rd32(REG_HSIZE);
        h = GDTR.rd32(REG_VSIZE);
        // w = 480, h = 272;
        Clear(); swap();
        Clear(); swap();
        Clear(); swap();
        cmd_regwrite(REG_PWM_DUTY, 128);
        // GD.wr32(REG_PWM_DUTY, 128);
        GD.flush();
// Serial1.println("STOP"); for(;;);

        //if (CALIBRATION & (options & GD_CALIBRATE)) {

//#if !defined(ARDUINO) && !defined(__DUE__)
/*
        if ((EEPROM.read(10) != 0x7f)) {

                self_calibrate();
                uint8_t reg_read;// = GDTR.rd(REG_CMD_READ);
                uint8_t reg_write;// = GDTR.rd(REG_CMD_WRITE);
                uint8_t diff = 1;
                while(diff){
                  reg_read = GDTR.rd(REG_CMD_READ);
                  reg_write = GDTR.rd(REG_CMD_WRITE);
                  diff = reg_read - reg_write;
                  delay(100);
                }

                // -for (int i = 0; i < 24; i++) Serial1.println(GDTR.rd(REG_TOUCH_TRANSFORM_A + i), HEX);
                for (int i = 0; i < 24; i++)
                {
                //  Serial.print(GDTR.rd(REG_TOUCH_TRANSFORM_A + i), HEX);
                //  Serial.print("\t\t- \t");
                  EEPROM.write(11 + i, GDTR.rd(REG_TOUCH_TRANSFORM_A + i));
                  //Serial.println(EEPROM.read(11+i), HEX);
                }

                EEPROM.write(10, 0x7d); // is written!

        } else {
//uint8_t temp = 12;
            //    for (int j = 0; j < 24; j++)
              //  {
        //              uint8_t temp = EEPROM.read(11);
        //              GDTR.wr(REG_TOUCH_TRANSFORM_A + 0, temp);

                      //uint8_t temp = EEPROM.read(11);
                //      Serial.println(temp, HEX);
                //}


  //  }
//#endif
  Serial.println("I got here 5");
#ifdef __DUE__
*/
                // The Due has no persistent storage. So instead use a "canned"
                // calibration.

                // self_calibrate();
                // for (int i = 0; i < 24; i++)
                //   Serial1.println(GDTR.rd(REG_TOUCH_TRANSFORM_A + i), HEX);
				/*
                static const byte canned_calibration[24] = {
                        0x66, 0x90, 0xFF, 0xFF, 0xAA, 0xFC, 0xFF, 0xFF,
                        0x91, 0x7C, 0x14, 0x03, 0x38, 0x00, 0x00, 0x00,
                        0x89, 0x85, 0xff, 0xff, 0x4a, 0x75, 0xe1, 0x01
                };
				*/
			
// A3
// FF
// FF
// 5A
// FF
// FF
// FF
// 84
// 36
// 40
// 1
// AE
// 0
// 0
// 0
// 75
// 98
// FF
// FF
// DE
// 27
// F5
// 0
                static const byte canned_calibration[24] = {
                        0xEF, 0xA1, 0xFF, 0xFF, 0x46, 0x1, 0x0, 0x0, 0xDC, 0x77, 0x3E, 0x1, 0xE8, 0xFF, 0xFF, 0xFF, 0xF7, 0x9B, 0xFF, 0xFF, 0x5F, 0xA7, 0xED, 0x0
                };
				for (int i = 0; i < 24; i++)
                        GDTR.wr(REG_TOUCH_TRANSFORM_A + i, canned_calibration[i]);
//#endif


  //      }
        GDTR.wr16(REG_TOUCH_RZTHRESH, 1200);

        rseed = 0x77777777;

        if ((BOARD == BOARD_FTDI_81X) && (options & GD_TRIM)) {
                tune();
        }
        
        GD.wr32(REG_PCLK, 2);       
}

boolean GDClass::calibrateScreen(void)
{
  
  self_calibrate();

  uint8_t reg_read;// = GDTR.rd(REG_CMD_READ);
	uint8_t reg_write;// = GDTR.rd(REG_CMD_WRITE);
	uint8_t diff = 1;
	while(diff){
		reg_read = GDTR.rd(REG_CMD_READ);
		reg_write = GDTR.rd(REG_CMD_WRITE);
		diff = reg_read - reg_write;
		delay(100);
	}


  Serial.println("Calibration Values");
  for (int i = 0; i < 24; i++)
  {
	  Serial.print("0x");
	  Serial.print(GDTR.rd(REG_TOUCH_TRANSFORM_A + i), HEX);
	  Serial.print(", ");
	  
  }
  Serial.println();
//   uint8_t temp;
//   for (int i = 0; i < 24; i++)
//   {
//     temp = GDTR.rd(REG_TOUCH_TRANSFORM_A+i);
//     EEPROM.put(11+i, temp);
//   }

//           //EEPROM.write(11 + i, GDTR.rd(REG_TOUCH_TRANSFORM_A + i));
//   temp = 0x7d;
//   EEPROM.put(10, temp); // is written!
//   return EEPROM.read(10);

}

void GDClass::storage(void) {
        // GDTR.__end();
        // SD.begin(SD_PIN);
        // GDTR.resume();
}

bool GDClass::self_calibrate(void) {
        cmd_dlstart();
        Clear();
//  Serial1.print("ID REGISTER: ");
//  Serial1.println(GDTR.rd(REG_ID), HEX);

#if (FT_810_ENABLE==1)
// Serial1.println("FT810");
        cmd_text(GD.w / 2, (GD.h / 2)+(2*(GD.h / 20)), 27, OPT_CENTERX, "FT810 en Teensy 3.2");
#endif

#if (FT_811_ENABLE==1)
// Serial1.println("FT811");
        cmd_text(GD.w / 2, (GD.h / 2)+(2*(GD.h / 20)), 27, OPT_CENTERX, "FT811 en Teensy 3.2");
#endif

#if (FT_812_ENABLE==1)
// Serial1.println("FT812");
        cmd_text(GD.w / 2, (GD.h / 2)+(2*(GD.h / 20)), 27, OPT_CENTERX, "FT812 en Teensy 3.2");
#endif

#if (FT_813_ENABLE==1)
        //Serial1.println("FT813");
        cmd_text(GD.w / 2, (GD.h / 2)+(2*(GD.h / 20)), 27, OPT_CENTERX, "CLCS Screen");
#endif

/* CHANGE: Added BT815 text */
#if (BT_815_ENABLE==1)
        cmd_text(GD.w / 2, (GD.h / 2)+(2*(GD.h / 20)), 27, OPT_CENTERX, "CLCS Screen");
#endif
/* End of change */

        cmd_text(GD.w / 2, GD.h / 2, 27, OPT_CENTERX, "Press on the dot to calibrate");
        //cmd_number((GD.w / 2)-(GD.w / 20), (GD.h / 2)+(GD.h / 20), 27, 0, GD.w);
        //cmd_number((GD.w / 2)+(GD.w / 80), (GD.h / 2)+(GD.h / 20), 27, 0, GD.h);
        //cmd_text(GD.w / 2, (GD.h / 2)+(GD.h / 20), 27, OPT_CENTERX, "Centerline (Windsor) Ltd");
        cmd_calibrate();
        finish();
        cmd_loadidentity();
        cmd_dlstart();
        GDTR.flush();
        return true;
}

void GDClass::seed(uint16_t n) {
        rseed = n ? n : 7;
}

uint16_t GDClass::random() {
        rseed ^= rseed << 13;
        rseed ^= rseed >> 17;
        rseed ^= rseed << 5;
        return rseed;
}

uint16_t GDClass::random(uint16_t n) {
        uint32_t p = random();
        return (p * n) >> 16;
}


// >>> [int(65535*math.sin(math.pi * 2 * i / 1024)) for i in range(257)]
static const uint16_t sintab[257] = {
        0, 402, 804, 1206, 1608, 2010, 2412, 2813, 3215, 3617, 4018, 4419, 4821, 5221, 5622, 6023, 6423, 6823, 7223, 7622, 8022, 8421, 8819, 9218, 9615, 10013, 10410, 10807, 11203, 11599, 11995, 12390, 12785, 13179, 13573, 13966, 14358, 14750, 15142, 15533, 15923, 16313, 16702, 17091, 17479, 17866, 18252, 18638, 19023, 19408, 19791, 20174, 20557, 20938, 21319, 21699, 22078, 22456, 22833, 23210, 23585, 23960, 24334, 24707, 25079, 25450, 25820, 26189, 26557, 26924, 27290, 27655, 28019, 28382, 28744, 29105, 29465, 29823, 30181, 30537, 30892, 31247, 31599, 31951, 32302, 32651, 32999, 33346, 33691, 34035, 34378, 34720, 35061, 35400, 35737, 36074, 36409, 36742, 37075, 37406, 37735, 38063, 38390, 38715, 39039, 39361, 39682, 40001, 40319, 40635, 40950, 41263, 41574, 41885, 42193, 42500, 42805, 43109, 43411, 43711, 44010, 44307, 44603, 44896, 45189, 45479, 45768, 46055, 46340, 46623, 46905, 47185, 47463, 47739, 48014, 48287, 48558, 48827, 49094, 49360, 49623, 49885, 50145, 50403, 50659, 50913, 51165, 51415, 51664, 51910, 52155, 52397, 52638, 52876, 53113, 53347, 53580, 53810, 54039, 54265, 54490, 54712, 54933, 55151, 55367, 55581, 55793, 56003, 56211, 56416, 56620, 56821, 57021, 57218, 57413, 57606, 57796, 57985, 58171, 58355, 58537, 58717, 58894, 59069, 59242, 59413, 59582, 59748, 59912, 60074, 60234, 60391, 60546, 60699, 60849, 60997, 61143, 61287, 61428, 61567, 61704, 61838, 61970, 62100, 62227, 62352, 62474, 62595, 62713, 62828, 62941, 63052, 63161, 63267, 63370, 63472, 63570, 63667, 63761, 63853, 63942, 64029, 64114, 64196, 64275, 64353, 64427, 64500, 64570, 64637, 64702, 64765, 64825, 64883, 64938, 64991, 65042, 65090, 65135, 65178, 65219, 65257, 65293, 65326, 65357, 65385, 65411, 65435, 65456, 65474, 65490, 65504, 65515, 65523, 65530, 65533, 65535
};

int16_t GDClass::rsin(int16_t r, uint16_t th) {
        th >>= 6; // angle 0-123
        // return int(r * sin((2 * M_PI) * th / 1024.));
        int th4 = th & 511;
        if (th4 & 256)
                th4 = 512 - th4; // 256->256 257->255, etc
        uint16_t s = pgm_read_word_near(sintab + th4);
        int16_t p = ((uint32_t)s * r) >> 16;
        if (th & 512)
                p = -p;
        return p;
}

int16_t GDClass::rcos(int16_t r, uint16_t th) {
        return rsin(r, th + 0x4000);
}

void GDClass::polar(int &x, int &y, int16_t r, uint16_t th) {
        x = (int)(-GD.rsin(r, th));
        y = (int)( GD.rcos(r, th));
}

// >>> [int(round(1024 * math.atan(i / 256.) / math.pi)) for i in range(256)]
static const uint8_t atan8[] = {
        0,1,3,4,5,6,8,9,10,11,13,14,15,17,18,19,20,22,23,24,25,27,28,29,30,32,33,34,36,37,38,39,41,42,43,44,46,47,48,49,51,52,53,54,55,57,58,59,60,62,63,64,65,67,68,69,70,71,73,74,75,76,77,79,80,81,82,83,85,86,87,88,89,91,92,93,94,95,96,98,99,100,101,102,103,104,106,107,108,109,110,111,112,114,115,116,117,118,119,120,121,122,124,125,126,127,128,129,130,131,132,133,134,135,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,177,178,179,180,181,182,183,184,185,186,187,188,188,189,190,191,192,193,194,195,195,196,197,198,199,200,201,201,202,203,204,205,206,206,207,208,209,210,211,211,212,213,214,215,215,216,217,218,219,219,220,221,222,222,223,224,225,225,226,227,228,228,229,230,231,231,232,233,234,234,235,236,236,237,238,239,239,240,241,241,242,243,243,244,245,245,246,247,248,248,249,250,250,251,251,252,253,253,254,255,255
};

uint16_t GDClass::atan2(int16_t y, int16_t x)
{
        uint16_t a;
        uint16_t xx = 0;

        /* These values are tricky. So pretend they are not */
        if (x == -32768)
                x++;
        if (y == -32768)
                y++;

        if ((x <= 0) ^ (y > 0)) {
                int16_t t; t = x; x = y; y = t;
                xx ^= 0x4000;
        }
        if (x <= 0) {
                x = -x;
        } else {
                xx ^= 0x8000;
        }
        y = abs(y);
        if (x > y) {
                int16_t t; t = x; x = y; y = t;
                xx ^= 0x3fff;
        }
        while ((x | y) & 0xff80) {
                x >>= 1;
                y >>= 1;
        }
        if (y == 0) {
                a = 0;
        } else if (x == y) {
                a = 0x2000;
        } else {
                // assert(x <= y);
                int r = ((x << 8) / y);
                // assert(0 <= r);
                // assert(r < 256);
                a = pgm_read_byte(atan8 + r) << 5;
        }
        a ^= xx;
        return a;
}

void GDClass::align(byte n) {
        while ((n++) & 3)
                GDTR.cmdbyte(0);
}

void GDClass::cH(uint16_t v) {
        GDTR.cmdbyte(v & 0xff);
        GDTR.cmdbyte((v >> 8) & 0xff);
}

void GDClass::ch(int16_t v) {
        cH((uint16_t)v);
}

void GDClass::cI(uint32_t v) {
        GDTR.cmd32(v);
}

void GDClass::cFFFFFF(byte v) {
        union {
                uint32_t c;
                uint8_t b[4];
        };
        b[0] = v;
        b[1] = 0xff;
        b[2] = 0xff;
        b[3] = 0xff;
        GDTR.cmd32(c);
}

void GDClass::ci(int32_t v) {
        cI((uint32_t) v);
}

void GDClass::cs(const char *s) {
        int count = 0;
        while (*s) {
                char c = *s++;
                GDTR.cmdbyte(c);
                count++;
        }
        GDTR.cmdbyte(0);
        align(count + 1);
}

void GDClass::copy(const uint8_t *src, int count) {
        byte a = count & 3;
        while (count--) {
                GDTR.cmdbyte(pgm_read_byte_near(src));
                src++;
        }
        align(a);
}

void GDClass::copyram(byte *src, int count) {
        byte a = count & 3;
        GDTR.cmd_n(src, count);
        align(a);
}

void GDClass::AlphaFunc(byte func, byte ref) {
        cI((9UL << 24) | ((func & 7L) << 8) | ((ref & 255L) << 0));
}
void GDClass::Begin(byte prim) {
        cI((31UL << 24) | prim);
}

//CHANGE: Function for the extended formats, I don't think I need to add this
void GDClass::BitmapExtendedFormat(uint16_t format){
        cI((46UL << 24) | format);
}

void GDClass::BitmapHandle(byte handle) {
        cI((5UL << 24) | handle);
}

//CHECK: Do I need to change this function to support ASTC?
void GDClass::BitmapLayout(byte format, uint16_t linestride, uint16_t height) {
        // cI((7UL << 24) | ((format & 31L) << 19) | ((linestride & 1023L) << 9) | ((height & 511L) << 0));
        union {
                uint32_t c;
                uint8_t b[4];
        };
        b[0] = height;
        b[1] = (1 & (height >> 8)) | (linestride << 1);
        b[2] = (7 & (linestride >> 7)) | (format << 3);
        b[3] = 7;
        cI(c);
}
void GDClass::BitmapSize(byte filter, byte wrapx, byte wrapy, uint16_t width, uint16_t height) {
        byte fxy = (filter << 2) | (wrapx << 1) | (wrapy);
        // cI((8UL << 24) | ((uint32_t)fxy << 18) | ((width & 511L) << 9) | ((height & 511L) << 0));
        union {
                uint32_t c;
                uint8_t b[4];
        };
        b[0] = height;
        b[1] = (1 & (height >> 8)) | (width << 1);
        b[2] = (3 & (width >> 7)) | (fxy << 2);
        b[3] = 8;
        cI(c);
        if (ft8xx_model) {
                b[0] = ((width >> 9) << 2) | (3 & (height >> 9));
                b[3] = 0x29;
                cI(c);
        }
}

//CHECK: I don't think I need to change this, as the extra bits were already reserved and this function accounts for that
void GDClass::BitmapSource(uint32_t addr) {
        cI((1UL << 24) | ((addr & 1048575L) << 0));
}
void GDClass::BitmapTransformA(int32_t a) {
        cI((21UL << 24) | ((a & 131071L) << 0));
}
void GDClass::BitmapTransformB(int32_t b) {
        cI((22UL << 24) | ((b & 131071L) << 0));
}
void GDClass::BitmapTransformC(int32_t c) {
        cI((23UL << 24) | ((c & 16777215L) << 0));
}
void GDClass::BitmapTransformD(int32_t d) {
        cI((24UL << 24) | ((d & 131071L) << 0));
}
void GDClass::BitmapTransformE(int32_t e) {
        cI((25UL << 24) | ((e & 131071L) << 0));
}
void GDClass::BitmapTransformF(int32_t f) {
        cI((26UL << 24) | ((f & 16777215L) << 0));
}
void GDClass::BlendFunc(byte src, byte dst) {
        cI((11UL << 24) | ((src & 7L) << 3) | ((dst & 7L) << 0));
}
void GDClass::Call(uint16_t dest) {
        cI((29UL << 24) | ((dest & 2047L) << 0));
}
void GDClass::Cell(byte cell) {
        cI((6UL << 24) | ((cell & 127L) << 0));
}
void GDClass::ClearColorA(byte alpha) {
        cI((15UL << 24) | ((alpha & 255L) << 0));
}
void GDClass::ClearColorRGB(byte red, byte green, byte blue) {
        cI((2UL << 24) | ((red & 255L) << 16) | ((green & 255L) << 8) | ((blue & 255L) << 0));
}
void GDClass::ClearColorRGB(uint32_t rgb) {
        cI((2UL << 24) | (rgb & 0xffffffL));
}
void GDClass::Clear(byte c, byte s, byte t) {
        byte m = (c << 2) | (s << 1) | t;
        cI((38UL << 24) | m);
}
void GDClass::Clear(void) {
        cI((38UL << 24) | 7);
}
void GDClass::ClearStencil(byte s) {
        cI((17UL << 24) | ((s & 255L) << 0));
}
void GDClass::ClearTag(byte s) {
        cI((18UL << 24) | ((s & 255L) << 0));
}
void GDClass::ColorA(byte alpha) {
        cI((16UL << 24) | ((alpha & 255L) << 0));
}
void GDClass::ColorMask(byte r, byte g, byte b, byte a) {
        cI((32UL << 24) | ((r & 1L) << 3) | ((g & 1L) << 2) | ((b & 1L) << 1) | ((a & 1L) << 0));
}
void GDClass::ColorRGB(byte red, byte green, byte blue) {
        // cI((4UL << 24) | ((red & 255L) << 16) | ((green & 255L) << 8) | ((blue & 255L) << 0));
        union {
                uint32_t c;
                uint8_t b[4];
        };
        b[0] = blue;
        b[1] = green;
        b[2] = red;
        b[3] = 4;
        cI(c);
}
void GDClass::ColorRGB(uint32_t rgb) {
        cI((4UL << 24) | (rgb & 0xffffffL));
}
void GDClass::Display(void) {
        cI((0UL << 24));
}
void GDClass::End(void) {
        cI((33UL << 24));
}
void GDClass::Jump(uint16_t dest) {
        cI((30UL << 24) | ((dest & 2047L) << 0));
}
void GDClass::LineWidth(uint16_t width) {
        cI((14UL << 24) | ((width & 4095L) << 0));
}
void GDClass::Macro(byte m) {
        cI((37UL << 24) | ((m & 1L) << 0));
}
void GDClass::PointSize(uint16_t size) {
        cI((13UL << 24) | ((size & 8191L) << 0));
}
void GDClass::RestoreContext(void) {
        cI((35UL << 24));
}
void GDClass::Return(void) {
        cI((36UL << 24));
}
void GDClass::SaveContext(void) {
        cI((34UL << 24));
}
void GDClass::ScissorSize(uint16_t width, uint16_t height) {
        if (ft8xx_model == 0)
                cI((28UL << 24) | ((width & 1023L) << 10) | ((height & 1023L) << 0));
        else
                cI((28UL << 24) | ((width & 4095L) << 12) | ((height & 4095L) << 0));
}
void GDClass::ScissorXY(uint16_t x, uint16_t y) {
        if (ft8xx_model == 0)
                cI((27UL << 24) | ((x & 511L) << 9) | ((y & 511L) << 0));
        else
                cI((27UL << 24) | ((x & 2047L) << 11) | ((y & 2047L) << 0));
}
void GDClass::StencilFunc(byte func, byte ref, byte mask) {
        cI((10UL << 24) | ((func & 7L) << 16) | ((ref & 255L) << 8) | ((mask & 255L) << 0));
}
void GDClass::StencilMask(byte mask) {
        cI((19UL << 24) | ((mask & 255L) << 0));
}
void GDClass::StencilOp(byte sfail, byte spass) {
        cI((12UL << 24) | ((sfail & 7L) << 3) | ((spass & 7L) << 0));
}
void GDClass::TagMask(byte mask) {
        cI((20UL << 24) | ((mask & 1L) << 0));
}
void GDClass::Tag(byte s) {
        cI((3UL << 24) | ((s & 255L) << 0));
}
void GDClass::Vertex2f(int16_t x, int16_t y) {
        // x = int(16 * x);
        // y = int(16 * y);
        cI((1UL << 30) | ((x & 32767L) << 15) | ((y & 32767L) << 0));
}
void GDClass::Vertex2ii(uint16_t x, uint16_t y, byte handle, byte cell) {
//   cI((2UL << 30) | ((x & 511L) << 21) | ((y & 511L) << 12) | ((handle & 31L) << 7) | ((cell & 127L) << 0));
        union {
                uint32_t c;
                uint8_t b[4];
        };
        b[0] = cell | ((handle & 1) << 7);
        b[1] = (handle >> 1) | (y << 4);
        b[2] = (y >> 4) | (x << 5);
        b[3] = (2 << 6) | (x >> 3);
        cI(c);
}
void GDClass::VertexFormat(byte frac) {
        cI((39UL << 24) | (((frac) & 7) << 0));
}
void GDClass::BitmapLayoutH(byte linestride, byte height) {
        cI((40 << 24) | (((linestride) & 3) << 2) | (((height) & 3) << 0));
}
void GDClass::BitmapSizeH(byte width, byte height) {
        cI((41UL << 24) | (((width) & 3) << 2) | (((height) & 3) << 0));
}
void GDClass::PaletteSource(uint32_t addr) {
        cI((42UL << 24) | (((addr) & 4194303UL) << 0));
}
void GDClass::VertexTranslateX(uint32_t x) {
        cI((43UL << 24) | (((x) & 131071UL) << 0));
}
void GDClass::VertexTranslateY(uint32_t y) {
        cI((44UL << 24) | (((y) & 131071UL) << 0));
}
void GDClass::Nop(void) {
        cI((45UL << 24));
}

void GDClass::cmd_append(uint32_t ptr, uint32_t num) {
        cFFFFFF(0x1e);
        cI(ptr);
        cI(num);
}
void GDClass::cmd_bgcolor(uint32_t c) {
        cFFFFFF(0x09);
        cI(c);
}
void GDClass::cmd_button(int16_t x, int16_t y, uint16_t w, uint16_t h, byte font, uint16_t options, const char *s) {
        cFFFFFF(0x0d);
        ch(x);
        ch(y);
        ch(w);
        ch(h);
        ch(font);
        cH(options);
        cs(s);
}
void GDClass::cmd_calibrate(void) {
        cFFFFFF(0x15);
        cFFFFFF(0xff);
}
void GDClass::cmd_clock(int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t h, uint16_t m, uint16_t s, uint16_t ms) {
        cFFFFFF(0x14);
        ch(x);
        ch(y);
        ch(r);
        cH(options);
        cH(h);
        cH(m);
        cH(s);
        cH(ms);
}
void GDClass::cmd_coldstart(void) {
        cFFFFFF(0x32);
}
void GDClass::cmd_dial(int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t val) {
        cFFFFFF(0x2d);
        ch(x);
        ch(y);
        ch(r);
        cH(options);
        cH(val);
        cH(0);
}
void GDClass::cmd_dlstart(void) {
        cFFFFFF(0x00);
}
void GDClass::cmd_fgcolor(uint32_t c) {
        cFFFFFF(0x0a);
        cI(c);
}
void GDClass::cmd_gauge(int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t major, uint16_t minor, uint16_t val, uint16_t range) {
        cFFFFFF(0x13);
        ch(x);
        ch(y);
        ch(r);
        cH(options);
        cH(major);
        cH(minor);
        cH(val);
        cH(range);
}
void GDClass::cmd_getmatrix(void) {
        cFFFFFF(0x33);
        ci(0);
        ci(0);
        ci(0);
        ci(0);
        ci(0);
        ci(0);
}
void GDClass::cmd_getprops(uint32_t &ptr, uint32_t &w, uint32_t &h) {
        cFFFFFF(0x25);
        ptr = GDTR.getwp();
        cI(0);
        w = GDTR.getwp();
        cI(0);
        h = GDTR.getwp();
        cI(0);
}
void GDClass::cmd_getptr(void) {
        cFFFFFF(0x23);
        cI(0);
}
void GDClass::cmd_gradcolor(uint32_t c) {
        cFFFFFF(0x34);
        cI(c);
}
void GDClass::cmd_gradient(int16_t x0, int16_t y0, uint32_t rgb0, int16_t x1, int16_t y1, uint32_t rgb1) {
        cFFFFFF(0x0b);
        ch(x0);
        ch(y0);
        cI(rgb0);
        ch(x1);
        ch(y1);
        cI(rgb1);
}
void GDClass::cmd_inflate(uint32_t ptr) {
        cFFFFFF(0x22);
        cI(ptr);
}
void GDClass::cmd_interrupt(uint32_t ms) {
        cFFFFFF(0x02);
        cI(ms);
}
void GDClass::cmd_keys(int16_t x, int16_t y, int16_t w, int16_t h, byte font, uint16_t options, const char*s) {
        cFFFFFF(0x0e);
        ch(x);
        ch(y);
        ch(w);
        ch(h);
        ch(font);
        cH(options);
        cs(s);
}
void GDClass::cmd_loadidentity(void) {
        cFFFFFF(0x26);
}
void GDClass::cmd_loadimage(uint32_t ptr, int32_t options) {
        cFFFFFF(0x24);
        cI(ptr);
        cI(options);
}
void GDClass::cmd_memcpy(uint32_t dest, uint32_t src, uint32_t num) {
        cFFFFFF(0x1d);
        cI(dest);
        cI(src);
        cI(num);
}
void GDClass::cmd_memset(uint32_t ptr, byte value, uint32_t num) {
        cFFFFFF(0x1b);
        cI(ptr);
        cI((uint32_t)value);
        cI(num);
}
uint32_t GDClass::cmd_memcrc(uint32_t ptr, uint32_t num) {
        cFFFFFF(0x18);
        cI(ptr);
        cI(num);
        uint32_t r = GDTR.getwp();
        cI(0xFFFFFFFF);
        return r;
}
void GDClass::cmd_memwrite(uint32_t ptr, uint32_t num) {
        cFFFFFF(0x1a);
        cI(ptr);
        cI(num);
}
void GDClass::cmd_regwrite(uint32_t ptr, uint32_t val) {
        cFFFFFF(0x1a);
        cI(ptr);
        cI(4UL);
        cI(val);
}
void GDClass::cmd_number(int16_t x, int16_t y, byte font, uint16_t options, uint32_t n) {
        cFFFFFF(0x2e);
        ch(x);
        ch(y);
        ch(font);
        cH(options);
        ci(n);
}
void GDClass::cmd_progress(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t options, uint16_t val, uint16_t range) {
        cFFFFFF(0x0f);
        ch(x);
        ch(y);
        ch(w);
        ch(h);
        cH(options);
        cH(val);
        cH(range);
        cH(0);
}
void GDClass::cmd_regread(uint32_t ptr) {
        cFFFFFF(0x19);
        cI(ptr);
        cI(0);
}
void GDClass::cmd_rotate(int32_t a) {
        cFFFFFF(0x29);
        ci(a);
}
void GDClass::cmd_scale(int32_t sx, int32_t sy) {
        cFFFFFF(0x28);
        ci(sx);
        ci(sy);
}
void GDClass::cmd_screensaver(void) {
        cFFFFFF(0x2f);
}
void GDClass::cmd_scrollbar(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t options, uint16_t val, uint16_t size, uint16_t range) {
        cFFFFFF(0x11);
        ch(x);
        ch(y);
        ch(w);
        ch(h);
        cH(options);
        cH(val);
        cH(size);
        cH(range);
}
void GDClass::cmd_setfont(byte font, uint32_t ptr) {
        cFFFFFF(0x2b);
        cI(font);
        cI(ptr);
}
void GDClass::cmd_setmatrix(void) {
        cFFFFFF(0x2a);
}
void GDClass::cmd_sketch(int16_t x, int16_t y, uint16_t w, uint16_t h, uint32_t ptr, uint16_t format) {
        cFFFFFF(0x30);
        ch(x);
        ch(y);
        cH(w);
        cH(h);
        cI(ptr);
        cI(format);
}
void GDClass::cmd_slider(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t options, uint16_t val, uint16_t range) {
        cFFFFFF(0x10);
        ch(x);
        ch(y);
        ch(w);
        ch(h);
        cH(options);
        cH(val);
        cH(range);
        cH(0);
}
void GDClass::cmd_snapshot(uint32_t ptr) {
        cFFFFFF(0x1f);
        cI(ptr);
}
void GDClass::cmd_spinner(int16_t x, int16_t y, byte style, byte scale) {
        cFFFFFF(0x16);
        ch(x);
        ch(y);
        cH(style);
        cH(scale);
}
void GDClass::cmd_stop(void) {
        cFFFFFF(0x17);
}
void GDClass::cmd_swap(void) {
        cFFFFFF(0x01);
}
void GDClass::cmd_text(int16_t x, int16_t y, byte font, uint16_t options, const char *s) {
        cFFFFFF(0x0c);
        ch(x);
        ch(y);
        ch(font);
        cH(options);
        cs(s);
}
void GDClass::cmd_toggle(int16_t x, int16_t y, int16_t w, byte font, uint16_t options, uint16_t state, const char *s) {
        cFFFFFF(0x12);
        ch(x);
        ch(y);
        ch(w);
        ch(font);
        cH(options);
        cH(state);
        cs(s);
}
void GDClass::cmd_track(int16_t x, int16_t y, uint16_t w, uint16_t h, byte tag) {
        cFFFFFF(0x2c);
        ch(x);
        ch(y);
        ch(w);
        ch(h);
        ch(tag);
        ch(0);
}
void GDClass::cmd_translate(int32_t tx, int32_t ty) {
        cFFFFFF(0x27);
        ci(tx);
        ci(ty);
}
void GDClass::cmd_playvideo(int32_t options) {
        cFFFFFF(0x3a);
        cI(options);
}
void GDClass::cmd_romfont(uint32_t font, uint32_t romslot) {
        cFFFFFF(0x3f);
        cI(font);
        cI(romslot);
}
void GDClass::cmd_mediafifo(uint32_t ptr, uint32_t size) {
        cFFFFFF(0x39);
        cI(ptr);
        cI(size);
}
void GDClass::cmd_setbase(uint32_t b) {
        cFFFFFF(0x38);
        cI(b);
}
void GDClass::cmd_videoframe(uint32_t dst, uint32_t ptr) {
        cFFFFFF(0x41);
        cI(dst);
        cI(ptr);
}
void GDClass::cmd_snapshot2(uint32_t fmt, uint32_t ptr, int16_t x, int16_t y, int16_t w, int16_t h) {
        cFFFFFF(0x37);
        cI(fmt);
        cI(ptr);
        ch(x);
        ch(y);
        ch(w);
        ch(h);
}
void GDClass::cmd_setfont2(uint32_t font, uint32_t ptr, uint32_t firstchar) {
        cFFFFFF(0x3b);
        cI(font);
        cI(ptr);
        cI(firstchar);
}
void GDClass::cmd_setbitmap(uint32_t source, uint16_t fmt, uint16_t w, uint16_t h) {
        cFFFFFF(0x43);
        cI(source);
        ch(fmt);
        ch(w);
        ch(h);
        ch(0);
}

void GDClass::cmd_setrotate(uint32_t r) {
        cFFFFFF(0x36);
        cI(r);
}
void GDClass::cmd_videostart() {
        cFFFFFF(0x40);
}

/**CHANGE: Added flash functions */
void GDClass::cmd_flasherase(){
        cFFFFFF(0x44);
        GD.flush();
}

void GDClass::cmd_flashwrite(uint32_t ptr, uint32_t num, byte *src){
        cFFFFFF(0x45);
        cI(ptr);
        cI(num);
        while(num--){
                (*src++);
        }
}

void GDClass::cmd_flashread(uint32_t dest, uint32_t src, uint32_t num){
        cFFFFFF(0x46);
        cI(dest);
        cI(src);
        cI(num);
        GD.flush();
}

void GDClass::cmd_flashupdate(uint32_t dest, uint32_t src, uint32_t num){
        cFFFFFF(0x47);
        cI(dest);
        cI(src);
        cI(num);
        GD.flush();
}

void GDClass::cmd_flashdetach(){
        cFFFFFF(0x48);
}

void GDClass::cmd_flashattach(){
        cFFFFFF(0x49);
}

// uint32_t GDClass::cmd_memcrc(uint32_t ptr, uint32_t num) {
//         cFFFFFF(0x18);
//         cI(ptr);
//         cI(num);
//         uint32_t r = GDTR.getwp();
//         cI(0xFFFFFFFF);
//         return r;
// }

uint32_t GDClass::cmd_flashfast(){
        cFFFFFF(0x4a);
        uint32_t r = GDTR.getwp();
        cI(0xFFFFFFFF);
        return r;
}

void GDClass::cmd_flashspidesel(){
        cFFFFFF(0x4b);
}

void GDClass::cmd_flashspitx(uint32_t num){
        cFFFFFF(0x4c);
        cI(num);
}

void GDClass::cmd_flashspirx(uint32_t ptr, uint32_t num){
        cFFFFFF(0x4d);
        cI(ptr);
        cI(num);
}

void GDClass::cmd_clearcache(){
        cFFFFFF(0x4f);
}

void GDClass::cmd_flashsource(uint32_t ptr){
        cFFFFFF(0x4e);
        cI(ptr);
}
/* End of change */



byte GDClass::rd(uint32_t addr) {
        return GDTR.rd(addr);
}
void GDClass::wr(uint32_t addr, uint8_t v) {
        GDTR.wr(addr, v);
}
uint16_t GDClass::rd16(uint32_t addr) {
        return GDTR.rd16(addr);
}
void GDClass::wr16(uint32_t addr, uint16_t v) {
        GDTR.wr16(addr, v);
}
uint32_t GDClass::rd32(uint32_t addr) {
        return GDTR.rd32(addr);
}
void GDClass::wr32(uint32_t addr, uint32_t v) {
        GDTR.wr32(addr, v);
}
void GDClass::wr_n(uint32_t addr, byte *src, uint32_t n) {
        GDTR.wr_n(addr, src, n);
}
void GDClass::rd_n(byte *dst, uint32_t addr, uint16_t n) {
        GDTR.rd_n(dst, addr, n);
}


void GDClass::cmdbyte(uint8_t b) {
        GDTR.cmdbyte(b);
}
void GDClass::cmd32(uint32_t b) {
        GDTR.cmd32(b);
}
void GDClass::finish(void) {
        GDTR.finish();
}
void GDClass::get_accel(int &x, int &y, int &z) {
        static int f[3];

        for (byte i = 0; i < 3; i++) {
                int a = analogRead(A0 + i);
                int s = (-160 * (a - 376)) >> 6;
                f[i] = ((3 * f[i]) >> 2) + (s >> 2);
        }
        x = f[2];
        y = f[1];
        z = f[0];
}
void GDClass::get_inputs(void) {
        GDTR.finish();
        byte *bi = (byte*)&inputs;
#if defined(DUMPDEV)
        extern FILE* stimfile;
        if (stimfile) {
                byte tag;
                fscanf(stimfile, "%hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx",
                       &bi[0],
                       &bi[1],
                       &bi[2],
                       &bi[3],
                       &bi[4],
                       &bi[5],
                       &bi[6],
                       &bi[7],
                       &bi[8],
                       &bi[9],
                       &bi[10],
                       &bi[11],
                       &bi[12],
                       &bi[13],
                       &bi[14],
                       &bi[15],
                       &bi[16],
                       &bi[17]);
                GDTR.wr(REG_TAG, tag);
        } else {
                inputs.x = inputs.y = -32768;
        }
#else
        GDTR.rd_n(bi, REG_TRACKER, 4);
        GDTR.rd_n(bi + 4, REG_TOUCH_RZ, 13);
        GDTR.rd_n(bi + 17, REG_TAG, 1);
#ifdef DUMP_INPUTS
        for (size_t i = 0; i < sizeof(inputs); i++) {
                Serial1.print(bi[i], HEX);
                Serial1.print(" ");
        }
        Serial1.println();
#endif
#endif
}
void GDClass::bulkrd(uint32_t a) {
        GDTR.bulk(a);
}
void GDClass::resume(void) {
        GDTR.resume();
}
void GDClass::__end(void) {
#if !defined(DUMPDEV) && !defined(RASPBERRY_PI)
        GDTR.__end();
#endif
}
void GDClass::play(uint8_t instrument, uint8_t note) {
        wr16(REG_SOUND, (note << 8) | instrument);
        wr(REG_PLAY, 1);
}
void GDClass::sample(uint32_t start, uint32_t len, uint16_t freq, uint16_t format, int loop) {
        GD.wr32(REG_PLAYBACK_START, start);
        GD.wr32(REG_PLAYBACK_LENGTH, len);
        GD.wr16(REG_PLAYBACK_FREQ, freq);
        GD.wr(REG_PLAYBACK_FORMAT, format);
        GD.wr(REG_PLAYBACK_LOOP, loop);
        GD.wr(REG_PLAYBACK_PLAY, 1);
}
void GDClass::reset() {
        GDTR.__end();
        GDTR.wr(REG_CPURESET, 1);
        GDTR.wr(REG_CPURESET, 0);
        GDTR.resume();
}

// Load named file from storage
// returns 0 on failure (e.g. file not found), 1 on success

uint8_t GDClass::switchToFullMode(){
        uint8_t val;
        uint32_t err;

        // GD.wr_n(60000, blob, 4096);
        // GD.cmd_flashupdate(0, 60000, 4096);

        GD.cmd_flashdetach();
        GD.flush();
        delay(1000);
        val = GD.rd(REG_FLASH_STATUS);

        if (val != 1){
                Serial.println("Unable to detach");
                return 0;
        }

        GD.cmd_flashattach();
        GD.flush();
        delay(1000);
        val = GD.rd(REG_FLASH_STATUS);

        if (val != 2){
                Serial.println("Unable to reattach");
                return 0;
        }

        err = GD.cmd_flashfast();
        GD.flush();
        delay(1000);
        val = GD.rd(REG_FLASH_STATUS);

        if (val != 3){
                Serial.print("Unable to go into full mode: ");
                Serial.print(err, HEX);
                return 0;
        }
        Serial.print("Successfully entered full mode: ");
        Serial.println(err, HEX);
        return 1;
}

byte GDClass::load(const char *filename, void (*progress)(long, long))
{
// #if defined(RASPBERRY_PI) || defined(DUMPDEV)
// char full_name[2048]  = "sdcard/";
// strcat(full_name, filename);
// FILE *f = fopen(full_name, "rb");
// if (!f) {
// perror(full_name);
// exit(1);
// }
// byte buf[512];
// int n;
// while ((n = fread(buf, 1, 512, f)) > 0) {
// GDTR.cmd_n(buf, (n + 3) & ~3);
// }
// fclose(f);
// return 1;
// #else
// GD.__end();
// Reader r;
// if (r.openfile(filename)) {
// byte buf[512];
// while (r.offset < r.size) {
// uint16_t n = min(512U, r.size - r.offset);
// n = (n + 3) & ~3;   // force 32-bit alignment
// r.readsector(buf);

        // GD.resume();
        // if (progress)
        // (*progress)(r.offset, r.size);
        // GD.copyram(buf, n);
        // GDTR.stop();
        // }
        // GD.resume();
        // return 1;
        // }
        // GD.resume();
        // return 0;
// #endif
}

// Generated by mk_bsod.py. Blue screen with 'ERROR' text
static const uint8_t __bsod[32] = {
        0, 255, 255, 255, 96, 0, 0, 2, 7, 0, 0, 38, 12, 255, 255, 255, 240, 0,
        90, 0, 31, 0, 0, 6, 69, 82, 82, 79, 82, 0, 0, 0
};
static const uint8_t __bsod_badfile[32] = {
        12, 255, 255, 255, 240, 0, 148, 0, 29, 0, 0, 6, 67, 97, 110, 110, 111,
        116, 32, 111, 112, 101, 110, 32, 102, 105, 108, 101, 58, 0, 0, 0
};

// Fatal error alert.
// Show a blue screen with message.
// This method never returns.

void GDClass::alert(const char *message)
{
        begin(0);
        copy(__bsod, sizeof(__bsod));
        cmd_text(240, 176, 29, OPT_CENTER, message);
        swap();
        GD.finish();
        for (;; )
                ;
}

void GDClass::safeload(const char *filename)
{
        if (!load(filename)) {
                copy(__bsod, sizeof(__bsod));
                copy(__bsod_badfile, sizeof(__bsod_badfile));
                cmd_text(240, 190, 29, OPT_CENTER, filename);
                swap();
                for (;; )
                        ;
        }
}

#define REG_SCREENSHOT_EN    (ft8xx_model ? 0x302010UL : 0x102410UL) // Set to enable screenshot mode
#define REG_SCREENSHOT_Y     (ft8xx_model ? 0x302014UL : 0x102414UL) // Y line register
#define REG_SCREENSHOT_START (ft8xx_model ? 0x302018UL : 0x102418UL) // Screenshot start trigger
#define REG_SCREENSHOT_BUSY  (ft8xx_model ? 0x3020e8UL : 0x1024d8UL) // Screenshot ready flags
#define REG_SCREENSHOT_READ  (ft8xx_model ? 0x302174UL : 0x102554UL) // Set to enable readout
#define RAM_SCREENSHOT       (ft8xx_model ? 0x3c2000UL : 0x1C2000UL) // Screenshot readout buffer
