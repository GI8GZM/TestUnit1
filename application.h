/*-----------------------------------------------------------------------------------
SWR / POWER METER + IC7300 C-IV CONTROLLER

Swr/PowerMeter (basic) - https://github.com/GI8GZM/PowerSwrMeter
Swr/PowerMeter + IC7300 C-IV Controller - https://github.com/GI8GZM/PowerMeter-CIVController

© Copyright 2018-2020  Roger Mawhinney, GI8GZM.
No publication without acknowledgement to author
-------------------------------------------------------------------------------------*/

// connstants and defines for Powermeter

/*------  measure() constants -------------------------------*/
#define	SAMPLE_FREQ		5000						// effective ADC sampling frequency - hertz
#define MAXBUF			SAMPLE_FREQ
#define PEAK_HOLD		2000					// average Peak Pwr hold time (mSecs)
#define PEP_HOLD		250					// average pep hold time (mSecs)
#define PWR_THRESHOLD   .5    						// power on threshold watts
#define	FV_ZEROADJ      -0.0000 				    // ADC zero offset voltage
#define	RV_ZEROADJ      -0.0000				        // ADC zero offset voltage

///*------  measure() constants -------------------------------*/
// Direct power conversion constants
// forward power constants - new coupler
// 03/06/2020
#define	FWD_V_SPLIT_PWR         0.015				// split voltage, direct pwr conversion
#define	FWD_LO_MULT_PWR         0.0969				// LO pwrs = ln(v)*LO_MULT_PWR + LO_ADD_PWR
#define FWD_LO_ADD_PWR          0.8719
#define	FWD_HI_MULT2_PWR        10.384
#define FWD_HI_MULT1_PWR		6.3687
#define FWD_HI_ADD_PWR			0.4894				// HI pwr = v*v*HI_MULT2_PWR +v*HI_MULT1_PWR + HI_ADD_PWR

//// Direct power conversion constants
//// forward power constants - new coupler
//// 01/06/2020
//#define	FWD_V_SPLIT_PWR         0.0175				// split voltage, direct pwr conversion
//#define	FWD_LO_MULT_PWR         0.068				// LO pwrs = ln(v)*LO_MULT_PWR + LO_ADD_PWR
//#define FWD_LO_ADD_PWR          0.7459
//#define	FWD_HI_MULT2_PWR        11
//#define FWD_HI_MULT1_PWR       6
//#define FWD_HI_ADD_PWR         .45				// HI pwr = v*v*HI_MULT2_PWR +v*HI_MULT1_PWR + HI_ADD_PWR


/*----------Metro timers-----------------------------------------*/
Metro netPwrPkTimer = Metro(1000);					// peak power hold timer
Metro pepTimer = Metro(1000);				   		// pep hold timer


/*-------------------- expressions ------------------------------*/
// constant expression to convert BCD to decimal
constexpr int getBCD(int n)
{
	return (n / 16 * 10 + n % 16);
}

// constant expression to convert decimal to BCD
constexpr int putBCD(int n)
{
	return (n / 10 * 16 + n % 10);
}

// Frame positions --------------------------------------------------------------------------

// Default Frame position number.

enum buttons {
	button0, button1, button2, button3,
};
const int NUM_BUTTONS = button3 + 1;


//const int
//netPower = 0,			// net Pwr
//peakPower = 1,			// peak Pwr
//vswr = 2,				// VSWR frame
//dBm = 3,				// dBm
//fwdPower = 4,			// forward Pwr
//refPower = 5,			// reflected Pwr
//fwdVolts = 6,			// forward volts
//refVolts = 7,			// reflected volts
//
//netPwrMeter = 8,		// power meter
//swrMeter = 9,			// swr meter
//
//avgOptions = 10,		// options button frame
//samplesSlowOpt = 11,	// meaurement average - samples register size
//samplesMedOpt = 12,		// meaurement average - samples register size
//samplesFastOpt = 13,	// calibrate average - samples register size
//weighting = 14;         // weighting for exponential smoothing+

enum frames {
	netPower, peakPower, vswr, dBm,
	fwdPower, refPower, fwdVolts, refVolts,
	netPwrMeter, swrMeter,
	avgOptions, samplesSlowOpt, samplesMedOpt, samplesFastOpt, weighting,
};

const int NUM_FRAMES = weighting + 1;

// working frame array
frame fr[NUM_FRAMES];           // Copy either civFrame or Basic Frame or calFrame

// ------------------------------  basic (non civ) frame layout -------------------------------
frame basicFrame[] = {
  { 5, 10,		100, 100,	BG_COLOUR,	true,	true,	true},			// 0 - netPwr (default - netPower)
  { 110, 10,	100, 65,	BG_COLOUR,	true,	true,	true},			// 1 - peakPwr
  { 215, 10,	100, 65,	BG_COLOUR,	true,	true,	true},			// 2 - swr
  { 5, 10,		100, 100,	BG_COLOUR,	true,	true,	false},			// 3 - dBm

  { 5, 115,		155, 30,	BG_COLOUR,	true,	false,	false},			// 4 - fwdPwr (forward power)
  { 165, 115,	155, 30,	BG_COLOUR,	true,	false,	false},			// 5 - refPwr (reflected power)
  { 5, 155,		155, 50,	BG_COLOUR,	true,	false,	false},			// 6 - fwdVolts
  { 165, 155,	155, 50,	BG_COLOUR,	true,	false,	false},			// 7 - refVolts

	// meter position														 
  { 5, 110,		315, 50,	BG_COLOUR,	false,	true,	true},			// 8 - Pwr Meter
  { 5, 160,		315, 50,	BG_COLOUR,	false,	true,	true},			// 9 - SWR Meter

	// buttons
  { 215, 215,	105, 25,	BG_COLOUR,	true,	true,	true},			// 10 - avgOptions (avgOptions button)
  { 200, 20,	90, 40,		BG_COLOUR,		true,	false,	false},		// 11 - samplesDefault (averaging samples - default)
  { 200, 70,	90, 40,		BG_COLOUR,		true,	false,	false},		// 12 - samplesAltOpt	(averaging samples - alternate)
  { 200, 120,	90, 40,		BG_COLOUR,		true,	false,	false},		// 13 - samplesCalOpt	(averaging samples - calibrate mode)
  { 200, 170,	90, 40,		BG_COLOUR,		true,	false,	false},		// 14 -  weighting for expnential smoothing  

};


// calibration frame layout
frame calFrame[] = {
  { 5, 10,		100, 75,	BG_COLOUR,	true,	true,	true},			// 0 - netPwr (default - netPower)
  { 110, 10,	100, 65,	BG_COLOUR,	true,	true,	true},			// 1 - peakPwr
  { 215, 10,	100, 65,	BG_COLOUR,	true,	false,	true},			// 2 - swr
  { 5, 10,		100, 75,	BG_COLOUR,	true,	false,	false},			// 3 - dBm

  { 5, 115,		155, 30,	BG_COLOUR,	true,	false,	true},			// 4 - fwdPwr (forward power)
  { 165, 115,	155, 30,	BG_COLOUR,	true,	false,	true},			// 5 - refPwr (reflected power)
  { 5, 155,		155, 50,	BG_COLOUR,	true,	false,	true},			// 6 - fwdVolts
  { 165, 155,	155, 50,	BG_COLOUR,	true,	false,	true},			// 7 - refVolts

	// meter position														 
  { 5, 95,		315, 55,	BG_COLOUR,	false,	false,	false},			// 8 - Pwr Meter
  { 5, 95,		315, 55,	BG_COLOUR,	false,	false,	false},			// 9 - SWR Meter

	// buttons
  { 215, 215,	105, 25,	BG_COLOUR,	true,	true,	true},			// 10 - avgOptions (avgOptions button)
  { 200, 20,	90, 40,		BG_COLOUR,		true,	false,	false},		// 11 - samplesDefault (averaging samples - default)
  { 200, 70,	90, 40,		BG_COLOUR,		true,	false,	false},		// 12 - samplesAltOpt	(averaging samples - alternate)
  { 200, 120,	90, 40,		BG_COLOUR,		true,	false,	false},		// 13 - samplesCalOpt	(averaging samples - calibrate mode)
  { 200, 170,	90, 40,		BG_COLOUR,		true,	false,	false},		// 14 -  weighting for expnential smoothing  

};

// label --------------------------------------------------------------------------------------------------------
#define GAP 5						// gap from frame outline


label lab[] = {
  { "Watts",		FG_COLOUR,		FONT14,     'C', 'B', false,	},		// 0 - net Pwr frame
  { "Pep",			FG_COLOUR,		FONT14,		'C', 'B', false,	},		// 1 - peak Pwr	 lab.stat = 0 for peakpwr, .stat=1 for PEP
  { "vSWR",			FG_COLOUR,		FONT14,		'C', 'B', false,	},		// 2 - VSWR
  { "dBm",			FG_COLOUR,		FONT14,		'C', 'B', false,	},		// 3 - dBm
  { "Fwd Pwr",		YELLOW,			FONT10,		'L', 'M', false,	},		// 4 - forward Pwr frame
  { "Ref Pwr",		YELLOW,			FONT10,		'R', 'M', false,	},		// 5 - reflected Pwr
  { "Fwd Volts",	GREENYELLOW,	FONT10,		'R', 'M', false,	},		// 6 - Forward voltage
  { "Ref Volts",	GREENYELLOW,	FONT10,		'L', 'M', false,	},		// 7 - Reflected Voltage
  { "         Watts",	FG_COLOUR,	FONT8,		'L', 'B', false,	},		// 8 - Pwr Meter
  { "         vSWR ",	FG_COLOUR,	FONT8,		'L', 'B', false,	},		// 9 - SWR Meter
  { "Avg",			BUTTON_FG,		FONT12,		'C', 'M', false,	},		// 10 - avgOptions button
  { "",				CIV_COLOUR,		FONT14,		'L', 'M', false,	},		// 11 - default samples size
  { "",				CIV_COLOUR,		FONT14,		'R', 'M', false,	},		// 12 - alternate samples size
  { "",				CIV_COLOUR,		FONT14,		'R', 'M', false,	},		// 13 - calibrate samples size
  { "",				CIV_COLOUR,		FONT14,		'R', 'M', false,	},		// 14 - weighting


};

// value ---------------------------------------------------------------------------
value val[] = {
  { 0.0, 0,	 FG_COLOUR,		FONT40,	    true},		// 0 - netPwr
  { 0.0, 0,	 FG_COLOUR,		FONT32,	    true},		// 1 - peakPwr
  { 0.0, 1,	 ORANGE,		FONT28,	    true},		// 2 - swr
  { 0.0, 0,	 GREEN,			FONT48,	    true},		// 3 - dbm
  { 0.0, 2,	 ORANGE,		FONT18,	    true},		// 4 - fwdPwr
  { 0.0, 2,	 ORANGE,		FONT18,	    true},		// 5 - refPwr
  { 0.0, 4,	 ORANGE,		FONT20,	    true},		// 6 - fwdVolts
  { 0.0, 4,	 ORANGE,		FONT20,	    true},		// 7 - refVolts
  { 0.0, 0,	 ORANGE,		FONT18,	    true},		// 8 - netPwrMeter
  { 0.0, 0,	 ORANGE,		FONT18,	    true},		// 9 - swrMeter
  { 0.0, 0,	 BG_COLOUR,		FONT16,	    true},		// 10 - avgOptions button
  { 0.0, 0,	 CIV_COLOUR,	FONT18,	    true},		// 11 - measure samples size
  { 0.0, 0,	 CIV_COLOUR,	FONT18,	    true},		// 12 - measure samples size
  { 0.0, 0,	 CIV_COLOUR,	FONT18,	    true},		// 13 - calibrate samples size
  { 0.0, 2,	 CIV_COLOUR,	FONT18,	    true},		// 14 - weighting
};

// meter -----------------------------------------------------------------------
meter mtr[] = {
{ 0, 100,	4, FG_COLOUR, BG_COLOUR,   5, YELLOW, 0, },
{ 1.0, 4.0,	4, FG_COLOUR, BG_COLOUR,   10, ORANGE, 0, },
};


/* structure for options boxes */
struct optBox										// touch check bxes/circles co-ords
{
	int x;
	int y;
};
optBox		tb[30];									// tb[] is touch area co-ord

/*----------EEPROM Options for HF Bands----------------------------------------------------*/
// EEPROM Adresses + Increments
#define		EEINCR 16								// address increment for band options and parameters
#define		EEADDR_PARAM 10							// start address variable parameter


/*----------eeProm strucure for Variable Parameters------------------------------*/
struct option										// param structure definition
{
	int		val;									// variables value
	bool	isFlg;									// variable flag
	int		eeAddr;									// eeProm address
};

// averaging 1-100% (fast - slow)
option		optAvgCal = { 75, 1,	EEADDR_PARAM + 0x20 };			// number samples for averaging - default
option		optAvgDefault = { 5,	1,	EEADDR_PARAM + 0x30 };			// alternate samples number
option		optAvgAlt = { 25,	1,	EEADDR_PARAM + 0x40 };			// samples for averaging
option     optAvgWeight = { 500, 1, EEADDR_PARAM + 0x50 };			// weight (x1000) for exponential averaging
#define     SAMPLES_CHANGE 5									// % change samples amount


//
//
//#ifdef TEENSY40
//#define		SAMPLE_INTERVAL 10									// ADC sample timer interval (microsecs)
//#define     SAMPLES_CHANGE 500                                       // change samples amount
//param		samplesSlowPar = { 5000, 1,	EEADDR_PARAM + 0x20 };		// number samples for averaging - default
//param		samplesMedPar =	{ 2000,	1,	EEADDR_PARAM + 0x30 };		// alternate samples number
//param		samplesFastPar = { 100,	1,	EEADDR_PARAM + 0x40 };		// samples for averaging
//
//#else                                                             // TEENSY32
//#define		SAMPLE_INTERVAL 50									// ADC sample timer interval (microsecs)
//#define     SAMPLES_CHANGE 20                                     // change samples amount
//param		samplesSlowPar = { 500,	1,	EEADDR_PARAM + 0x20 };		// number samples for averaging - default
//param		samplesMedPar = { 100, 1, EEADDR_PARAM + 0x30 };		// alternate samples number
//param		samplesFastPar = { 1000,	1,	EEADDR_PARAM + 0x40 };	// samples for averaging
//
//#endif


