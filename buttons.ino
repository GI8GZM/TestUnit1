/*-----------------------------------------------------------------------------------
SWR / POWER METER + IC7300 C-IV CONTROLLER

Swr/PowerMeter (basic) - https://github.com/GI8GZM/PowerSwrMeter
Swr/PowerMeter + IC7300 C-IV Controller - https://github.com/GI8GZM/PowerMeter-CIVController

� Copyright 2018-2020  Roger Mawhinney, GI8GZM.
No publication without acknowledgement to author
-------------------------------------------------------------------------------------*/

/*-----  button functions -----------------------------------------------*/

/*------------------------------ touchnetPwr() ---------------------------------
short touch (1) - switches to dBm display. any touch goes back to netPwr
long touch (2) - toggles between normal and basic mode
*/
void netPwrButton(int tStat)
{
	// short touch - display power dBm
	if (tStat == 1)
	{
		eraseFrame(netPower);
		restoreFrame(dBm);
	}
}


/*------------------------ dBmButton()------------------------------------
dBm displayed by netPower short touch
short touch restores to netPwr (Watts) display
*/
void dbmButton(int tStat)
{
	eraseFrame(dBm);									// erases and disables  netPwr frame
	fr[netPower].bgColour = BG_COLOUR;
	restoreFrame(netPower);
}

/*-----------------------------peakPwrButton() --------------------------------------------------
short touch(1) - swap from Peak average power to PEP
long touch(2) - calibrate mode
*/
void peakPwrButton(int tStat)
{
	// toggle between pep and peak average power
	if (tStat == 1)
		lab[peakPower].stat = !lab[peakPower].stat;
	if (lab[peakPower].stat)
		strcpy(lab[peakPower].txt, "Avg Peak");
	else
		strcpy(lab[peakPower].txt, "Pep");
	restoreFrame(peakPower);

	// toggle normal(civ) mode and calibrate mode
	if (tStat == 2)
	{

		lab[peakPower].stat = !lab[peakPower].stat;
		// lab[peakPower].stat = true for normal mode
		if (!lab[peakPower].stat)
		{
			// calibrate
			avgSamples = optAvgCal.val;
			copyFrame(calFrame);
			drawDisplay();
		}
		else
		{
			// normal mode
			avgSamples = optAvgDefault.val;
			initDisplay();
		}
	}
}

/*---------------------- swrButton ------------------------------
short touch - swap from one decimal to two decmal places
long touch - reboot
*/
void swrButton(int tStat)
{
	// short touch - change decimals & font
	if (tStat == 1)
	{
		if (val[vswr].decs == 1)
		{
			val[vswr].font = FONT24;
			val[vswr].decs = 2;
		}
		else
		{
			val[vswr].font = FONT28;
			val[vswr].decs = 1;
		}
		restoreFrame(vswr);
	}

	//reboot;
	if (tStat == 2)
	{
		// unset isEEInit flag to copy defualts to EEPROM
		//EEPROM.write(0, false);

		CPU_RESTART;									
	}
}

/*--------------------meterButton() -------------------------
swap currMeter and newMeter
*/
void meterButton(int currMeter, int newMeter)
{
	{
		int x = fr[currMeter].x;
		int y = fr[currMeter].y;

		fr[currMeter].x = fr[newMeter].x;
		fr[currMeter].y = fr[newMeter].y;
		fr[newMeter].x = x;
		fr[newMeter].y = y;
		// swap meters
		restoreFrame(currMeter);
		drawMeterScale(currMeter);
		restoreFrame(newMeter);
		drawMeterScale(newMeter);
	}

}


/*------------------------------ optionsButton ----------------------
toggles between calibrate mode and ci-v mode
tStat = 0 (don't toggle), 1 (short touch), 2 (long Touch)
Calibrate -Plot antenna rms volts (oscilloscope) against Vf.
Use curve fitting to get constants.
*/
void optionssButton(int tStat)
{
	// short touch - cycle through averaging options
	if (tStat == 1)
	{
		if (avgSamples == optAvgCal.val)
			avgSamples = optAvgDefault.val;
		else if (avgSamples == optAvgDefault.val)
			avgSamples = optAvgAlt.val;
		else if (avgSamples == optAvgAlt.val)
			avgSamples = optAvgCal.val;
		avgOptionsLabel();
	}

	//  long touch - set averaging options
	if (tStat == 2)
		setAvgSamples();
}

/*---------------------- optionsLabel() -------------------------------
update options label with averaging status
averaging (1-100: fast - slow)
*/
void avgOptionsLabel()
{
	if (avgSamples != optAvgDefault.val)
	{
		// invert avgOptions button
		lab[avgOptions].colour = BUTTON_BG;
		fr[avgOptions].bgColour = BUTTON_FG;
	}
	else
	{
		lab[avgOptions].colour = BUTTON_FG;
		fr[avgOptions].bgColour = BUTTON_BG;
	}

	// display avgOptions on button
	sprintf(lab[avgOptions].txt, "Average: %d", avgSamples);
	restoreFrame(avgOptions);
}

