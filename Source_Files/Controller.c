#include <stdio.h>

#include "Controller.h"
#include "mc9s12a128.h"
#include "EEProm.h"
#include "Subroutines.h"
#include "Subroutines1.h"
#include "mco.h"



void main (void)
{
    InitPorts ();
	InitInterrupts ();
	InitPLL ();
  	PWMInit ();
	AtoDInit ();
	
    EEInit ();


  	// end of initialization, enable all interrupts
  	INTR_ON();
	
    Load_Camera_Add();     //get camera address from memory, do NOT use default value
    //important to load_camera_add() before Load_Variables(), EEProm is not available for
    //brief period after save_variables at end of load_variables()
	
	//Skip over this function call to change the serial number
	Load_Serial_Num ();

	Load_Variables ();
	
	
	
	InitCANOpen ();
	
	COPCTL = 0x47;				//enable COP
	
	while (1)
	{
	    ARMCOP = 0x55;
		ARMCOP = 0xAA;
	    doevents ();
	}
	 
}






