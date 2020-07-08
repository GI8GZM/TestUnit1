/*-----------------------------------------------------------------------------------
SWR / POWER METER + IC7300 C-IV CONTROLLER

Swr/PowerMeter (basic) - https://github.com/GI8GZM/PowerSwrMeter
Swr/PowerMeter + IC7300 C-IV Controller - https://github.com/GI8GZM/PowerMeter-CIVController

© Copyright 2018-2020  Roger Mawhinney, GI8GZM.
No publication without acknowledgement to author
-------------------------------------------------------------------------------------*/

//#ifdef TEENSY40
//#define SAMPLE_INTERVAL 200						    // ADC sample timer interval (microsecs)
//#define MAXBUF          10000						// maximum averaging buffer/samples size
//
//#else
//#define SAMPLE_INTERVAL 400						    // ADC sample timer interval (microsecs)
//#define MAXBUF          2000						// maximum averaging buffer/samples size
//
//#endif
//


#define		TEENSY40								// comment this line for default = Teensy 3.2


/*---------- Teensy pin assignments (use for wiring) --------------------------------------
board: referes to ILI9341 + Touch board   */
#define	DIM_PIN         3							// board: 8(LED) analog out pin for display LED & dimming
#define	TFT_DC_PIN      9                           // board: 5(DC) tft data pin
#define	TFT_CS_PIN      10							// board: 10(CS) TFT chip select pin
#define	TS_IRQ_PIN      2							// board: 14(T_IRQ) touch interrupt pin
#define	TS_CS_PIN       6							// board: 6(T_CS) Touch CS. *** do NOT use pin 8...  required for Serial3
#define TFT_SDI         11                          // board: 6(SDI), 12(T_DIN)     *** used in library, not referenced in code
#define TFT_SCK         13                          // board: 7(SCK), 10(T_CLK)     *** used in library, not referenced in code
#define TFT_SDO         12                          // board: 9(SDO), 13(T_DO)      *** used in library, not referenced in code

#define FWD_ADC_PIN     15                          // (A1) ACD input pin - forward power
#define REF_ADC_PIN     16                          // (A2) ACD input pin- reflected power
#define SERIAL_RX1      0                           // Serial1 RX pin
#define SERIAL_TX1      1                           // Serial1 Tx pin
#define SERIAL_RX3      7                           // Serial3 RX pin
#define SERIAL_TX3      8                           // serial3 Tx pin
#define	TEST_PIN        4							// high/low pulse output for timing



// display/touch screen
#define		TOUCH_REVERSED false 					// touchscreen, true = reversed, false = normal
#define     SCREEN_ROTATION 3						// rotation for tft and touchscreen

/*----------ILI9341 TFT display (320x240)-------------------------*/
ILI9341_t3	tft = ILI9341_t3(TFT_CS_PIN, TFT_DC_PIN);		// define tft device
#define		TFT_FULL 255							// tft display full brightness
#define		TFT_DIM	20								// tft display dim value
#define		TFT_OFF	0								// tft display off

/*----------XPT2046 touchscreen	-----------------------------*/
XPT2046_Touchscreen ts(TS_CS_PIN, TS_IRQ_PIN);		// allows touch screen interrupts
#if			TOUCH_REVERSED
int xMapL = 3850, xMapR = 500;                      // reversed touch screen mapping
int yMapT = 3800, yMapB = 300;
#else
int xMapL = 320, xMapR = 3850;                      // touch screen mapping
int yMapT = 300, yMapB = 3800;
#endif
#define		MAPX map(p.x, xMapL, xMapR, 0, 320)		// touch screen mapping expresssion
#define		MAPY map(p.y, yMapT, yMapB, 0, 240)

#define		SHORTTOUCH = 1;
#define		LONGTOUCH = 2;

#define		SPLASH_DELAY 1 * 1000					// splash screen delay, mSecs.. At power on, allow time for radio to boot


/* ---------------- Metro Timers -----------*/
Metro heartBeatTimer = Metro(250);			        // heartbeat timer
Metro longTouchTimer = Metro(750);			        // long touch timer
Metro dimTimer = Metro(15 * 60 * 1000);				// dimmer timer (mins)

 // set up default arguements for functions
void displayValue(int posn, float curr, bool isUpdate = false);
void displayLabel(int posn, char* = NULL);


// frame / button definitions-----------------------------------------------------------------------
#define RADIUS 5				// frame corner radius
#define LINE_COLOUR LIGHTGREY	// frame line colour

// structure definitions
struct frame {
	int x;							// top left corner - x coord
	int y;							// top left corner - y coord
	int w;							// horizontal width
	int h;							// vertical height
	int bgColour;					// frame background colour
	bool isOutLine;					// outline flg / don't display
	bool isTouch;					// frame enabled for touch
	bool isEnable;					// enable frame & CONTENTS
};

struct label {
	char txt[30];					// frame label text
	int colour;						// text colour
	ILI9341_t3_font_t  font;		// text font size
	char xJustify;					// 'L'eft, 'C'entre, 'R'ight
	char yJustify;					// 'T'op, 'M'iddle, 'B'ottom
	int stat;						// status, used for update label display, -1 for errors
};

struct value {
	float prevDispVal;				// previous display value
	int decs;						// decimals
	int colour;						// text colour
	ILI9341_t3_font_t  font;		// text font size
	bool isUpdate;					// true forces display update
};

struct meter {
	float sStart;					// start value for scale
	float sEnd;						// end value for scale
	int major;						// number major scale divisions
	int tColour;					// meter bar gradient top colour
	int bColour;					// meter bar bottom colour
	int pkWidth;					// peak indicator width
	int pkColour;					// peak indicator colour
	int pkPrevPosn;					// peak indicator prev position
};



/*---------- Teensy restart code (long press on Peak Power frame)--------*/
#define		CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define		CPU_RESTART_VAL 0x5FA0004
#define		CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);

/*----------- code to get Teeny type -----------------------------------*/
//uint32_t cpuid = ((*(volatile uint32_t*)0xE000ED00) & 0xfff0);
//
//uint16_t dev_id;
//if (cpuid == 0xC240) dev_id = 1;		//T3.1	(M4)
//else if (cpuid == 0xC600) dev_id = 2;	//TLC	(M0+)
//else if (cpuid == 0xC270) dev_id = 3;	//T4.0	(M7)
//else dev_id = 0;

#define     CPU_ID ((*(volatile uint32_t*)0xE000ED00) & 0xfff0);

