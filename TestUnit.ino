/*-----------------------------------------------------------------------------------
SWR / POWER METER + IC7300 C-IV CONTROLLER

Swr/PowerMeter (basic) - https://github.com/GI8GZM/PowerSwrMeter
Swr/PowerMeter + IC7300 C-IV Controller - https://github.com/GI8GZM/PowerMeter-CIVController

© Copyright 2018-2020  Roger Mawhinney, GI8GZM.
No publication without acknowledgement to author
-------------------------------------------------------------------------------------*/
/*
	Teensy 3.2
	ILI9341 TFT liquid crystal display, 240x320 dots, RGB colour.
	XPT2046 Touch screen,   Needs mapped to match TFT display

	15 May 2020 - project renamed to PowerMeter
*/



// libraries
#include <XPT2046_Touchscreen.h>					// touchscreen library
#include <ILI9341_t3.h>								// display (320x240)
#include <EEPROM.h>									// EEPROM
#include <ADC.h>									// analog - digital converter
#include <Metro.h>									// metro timers

// local header files - keep in this order
#include "general.h"								// general constants
#include "fontsColours.h"							// Teensy fonts

// power meter specific
#include "application.h"								// PowerMeter defines

/* global variables  */
//int		devId = 1;									// default device to Teensy 3.2

int		avgSamples;									// number of circular buffer samples
bool	isDim = false;								// dim flag, false = no dim

/*-------------------------------------------------------------------------------------------------------
  setup() - initialise functions
  runs once
*/
void setup()
{
	/*----------- code to get Teeny type -----------------------------------*/
	// get Teensy type (3.2 or 4.0)
	long cpuid = CPU_ID;								// see pwrMeter.h for code definition
	int dev_id=0;
	if (cpuid == 0xC240)
		dev_id = 1;									//T3.1	(M4)
	else if (cpuid == 0xC270)
		dev_id = 3;									//T4.0	(M7)

	pinMode(LED_BUILTIN, OUTPUT);						// diagnostic trigger
	pinMode(TEST_PIN, OUTPUT);							// set toggle pin for timing

	// set tft to off
	analogWrite(DIM_PIN, TFT_OFF);

	tft.begin();										// TFT begin
	tft.setRotation(SCREEN_ROTATION);					// horizontal, pins to right. Cursor 0,0 = TOP LEFT of screen

	ts.begin();											// touch screen begin
	ts.setRotation(SCREEN_ROTATION);							// set touch rotation

	// USB serial
	Serial.begin(19200);								// serial for debug
	delay(100);
	Serial.print("Teensy ID: ");
	Serial.println(dev_id);

	// initialise variables etc from EEPROM
	//clearEEPROM();									// clear EEPROM - diagnostic only
	initEEPROM();

	// set circular buffer default sample size
	avgSamples = optAvgDefault.val;

	// initialise ADC, set interrupt timer
	initADC();

	// display splash screen
	analogWrite(DIM_PIN, TFT_FULL);
	splashScreen();

	// draw screen etc
	initDisplay();
}

/*-----------------------------------------------------------------------------------------------
  loop() - main loop
  executes continuously
*/
void loop()
{
	//digitalWrite(TEST_PIN, !digitalRead(TEST_PIN));

	// blink indicator
	heartBeat();

	// measure & display power, swr etc - main function
	//measure();

	// dimmer timer - dim display if not active, touch to undim
	if (dimTimer.check())								// check Metro timer
		setDimmer();

	// check if screen has been touched
	if (ts.tirqTouched())
	{
		if (isDim)
			resetDimmer();								// reset dimmer
		else
			chkTouchFrame(NUM_FRAMES);					// check for touch or button press
	}
}

/*------------------------------------------------------------------------------------------
 initDisplay()
	Initialises system to standard screen layout
	called by: setup() and Options()
	used to reset the screen layout
*/
void initDisplay()
{
	// tft display OFF
	analogWrite(DIM_PIN, TFT_OFF);

	copyFrame(basicFrame);
	val[netPower].font = FONT48;

	// draw enabled display frames labels and values
	drawDisplay();

	// empty tirqtouch buffer for first operation
	if (ts.tirqTouched())
		ts.touched();
}

/*------------------ drawDisplay -----------------------------------------------------------------------
	draw display
	displays only enabled frames
*/
void drawDisplay()
{
	// clear screen
	analogWrite(DIM_PIN, TFT_OFF);						// tft display OFF
	tft.fillScreen(BG_COLOUR);							// set tft background colour

	// draw enabled display frames, labels and values
	for (int i = 0; i < NUM_FRAMES; i++)			// do all frames, except for experimental freq meter
	{
		displayLabel(i);								// displays only enabled
		val[i].isUpdate = true;							// force values redisplay
	}

	// draw enabled meter scales
	drawMeterScale(netPwrMeter);
	drawMeterScale(swrMeter);

	// display samples / options button
	avgOptionsLabel();

	// turn screen on full bright
	analogWrite(DIM_PIN, TFT_FULL);
}

/*------------------------------------splashScreen() --------------------
displays startup messages, allows for screen calibration
*/
void splashScreen()
{
	char title[] = "";				// display current version
	char cpyRight[] = "Copyright 2020 - R Mawhinney, Gi8GZM";
	char line0[] = "SWR / Power Meter";
	char line2[] = "by GI8GZM";

	int y = 10;
	// display title
	tft.fillScreenVGradient(BG_COLOUR, BLUE);
	tft.setTextColor(WHITE);
	tft.setFont(FONT18);
	displayTextCentred(title, 10);

	// display copyright notice
	tft.setCursor(10, 40);
	tft.setFont(AwesomeF180_14);
	tft.write(121);
	tft.setCursor(30, 43);
	tft.setFont(FONT12);
	tft.print(cpyRight);

	// display remaining lines
	y = 100;
	tft.setFont(FONT20);
	displayTextCentred(line0, y);

	tft.setFont(FONT16);
	displayTextCentred(line2, y + 100);

	// hold display
	for (int i = 0; i < SPLASH_DELAY / 10; i++)
	{
		// do other things in here
		delay(10);
	}
}

/*--------------------------- copyFrame() ---------------------------------------
copies default frame setting (frame.h) to  frame pointer
-------------------------------------------------------------------------------*/
void copyFrame(frame* fPtr)
{
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		fr[i].x = fPtr[i].x;
		fr[i].y = fPtr[i].y;
		fr[i].w = fPtr[i].w;
		fr[i].h = fPtr[i].h;
		fr[i].bgColour = fPtr[i].bgColour;
		fr[i].isOutLine = fPtr[i].isOutLine;
		fr[i].isTouch = fPtr[i].isTouch;
		fr[i].isEnable = fPtr[i].isEnable;
	}
}

/*---------------------------------- resetDimmer() ---------------------
reset tft dimmer
*/
void resetDimmer()
{
	// reset dimmer flag, tft display and timer
	isDim = false;
	analogWrite(DIM_PIN, TFT_FULL);
	dimTimer.reset();
}

/*---------------------------- setDimmer() -----------------------------
tft display dimmer
*/
void setDimmer()
{
	// set flag and dim tft display
	isDim = true;
	analogWrite(DIM_PIN, TFT_DIM);
}

/*--------------------------- heartbeat() -----------------------------
heartbeat()  - timer, displays pulsing dot top left corner
*/
void heartBeat()
{
	static bool isHeartBeat;

	if (heartBeatTimer.check())
	{
		if (isHeartBeat) {
			tft.fillCircle(12, 18, 5, FG_COLOUR);
			heartBeatTimer.reset();
		}
		else
			tft.fillCircle(12, 18, 5, BG_COLOUR);

		// set/reset flag, toggle indicator on/off
		isHeartBeat = !isHeartBeat;
	}
}