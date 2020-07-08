/*-----------------------------------------------------------------------------------
SWR / POWER METER + IC7300 C-IV CONTROLLER

Swr/PowerMeter (basic) - https://github.com/GI8GZM/PowerSwrMeter
Swr/PowerMeter + IC7300 C-IV Controller - https://github.com/GI8GZM/PowerMeter-CIVController

© Copyright 2018-2020  Roger Mawhinney, GI8GZM.
No publication without acknowledgement to author
-------------------------------------------------------------------------------------*/

/*---------------------------------  eePromInit() ---------------------------------------------------------
Initialises EEPROM to default values from frame.h
   Normal start up reads EEPROM into band data and global variables
   EEPROM(0) is initialised flag, 0=not, 1= done
------------------------------------------------------------------------------------------*/
void initEEPROM(void)
{
	// new EEPROM may be initialised to 0,1, 0xFFFF, or something else
	// check isEEInit to a pattern unlilkely to be in iniial EEPROM content

	int pattern = B0101001;										// random pattern
	int isEEInit = EEPROM.read(0);								// if true = already initialised at first boot, false = new processor, uninitialised EEPROM

	if (isEEInit == pattern)
	{
		// EEProm has been initialised.  Get values and set variables
		EEPROM.get(optAvgCal.eeAddr, optAvgCal);
		EEPROM.get(optAvgDefault.eeAddr, optAvgDefault);
		EEPROM.get(optAvgAlt.eeAddr, optAvgAlt);
		EEPROM.get(optAvgWeight.eeAddr, optAvgWeight);
	}
	else
	{
		// initialise EEPROM from default values.  Should only happen with blank eeprom
		clearEEPROM();

		EEPROM.put(optAvgCal.eeAddr, optAvgCal);
		EEPROM.put(optAvgDefault.eeAddr, optAvgDefault);
		EEPROM.put(optAvgAlt.eeAddr, optAvgAlt);
		EEPROM.put(optAvgWeight.eeAddr, optAvgWeight);

		// set EEPROMinitialised flag
		EEPROM.write(0, pattern);								// write isEEInit flag pattern, address 0
	}
}

void clearEEPROM()
{
	// clear eeprom to zeros
	for (int i = 0; i < EEPROM.length(); i++)
		EEPROM.write(i, 0);
}

