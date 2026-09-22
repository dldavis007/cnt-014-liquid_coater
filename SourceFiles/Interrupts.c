#include "Interrupts.h"
#include "mc9s12a128.h"
#include "Controller.h"
#include "mco.h"
#include "mcohw.h"
#include "nodecfg.h"
#include "procimg.h"
#include "Subroutines.h"
#include <stdlib.h>
#include <math.h>



//#define diag

char Seconds = 60;
unsigned long  StateTime = 0;
char prevDir;
char currentDir;

int Tick = RTI_One_Sec;
unsigned int TC0_RCVD_Data;
unsigned int posTick;
unsigned int negTick;


char Gen_Flags;

unsigned int Timer1;
unsigned int Timer2;
unsigned int FocusZoomTimer;
unsigned int AdvanceTimer;
unsigned int HdSpeedTimer;
int Update_Menu_Timer;
int HeadRampTimer;
unsigned int MenuTimer = RTI_One_Sec * 2;
unsigned int CamIndexTimer;
unsigned int BlowerTimer;

unsigned int CamStopTimer;
unsigned int CamPIDTimer;
extern int Rotate;
//unsigned int PrevCamPIDTimer;

extern struct menu_var MaxLADist;
extern struct menu_var CameraTurnTime;
int CamPosition;
char CamHome;
unsigned int CamTurnTimer;

extern int CamDegTimer;
extern int HeaterTimer;

extern char HeadSpdOut;
extern char iHeadSpd;
extern char CurrentLight;
extern char lightval;

extern float RDR_Ratio;
extern struct menu_var MachineSize;
extern struct menu_var MotorPol;

extern unsigned char Sense_Detected;

unsigned int HeadTime;
unsigned int OldHeadTime;
float HeadSpeed;

unsigned int LAMoveTimer;
unsigned long LAMovingTimer;
unsigned long Temp_LAMovingTimer;

char fast_inc=0;
unsigned int IncSpeedUpTimer;
char IncSpeedUpCntr;

char test=1;

void DUMMY_ENTRY ( void )
{

}

void CANRxISR ( void )
{
  	unsigned char length, index;
	unsigned char rxdata[8];
    UNSIGNED32 Identifier;
	CAN_MSG *pReceiveBuf;
	
    Identifier   = (UNSIGNED32)CANRXIDR0;
    Identifier <<= 8;
    Identifier  |= (UNSIGNED32) CANRXIDR1;
    Identifier >>= 5;
	
	length = (CANRXDLR & 0x0f);
	for ( index = 0; index < length; index++ )
		*(UNSIGNED8 *)(pReceiveBuf->BUF+index) = *(&CANRXDSR0 + index);   	/* Get received data */
	CANRFLG = 0x01;	  				   						  				/* Clear RXF */

    pReceiveBuf->ID = Identifier;
    pReceiveBuf->LEN = length;

}	

void IRQ_Int_Handler ( void )
{

}

void TC0_Int_Handler ( void )
{
	 
 	 TFLG1 = 0x01;	   		//clear interrupt 
	 
	 if ( lightval > 90 )
	 {
	     TC0 = TC0 + TC_416us;
		 TCTL2 = ( TCTL2 & ~0x03 ) | 0x03;
		 CFORC = 0x01;
	 }
}

void TC3_Int_Handler ( void )
{
 	 TFLG1 = 0x08;	   		//clear interrupt
	 Sense_Detected = 1;
}

void TC4_Int_Handler ( void )
{
 	 TFLG1 = 0x10;	   		 				 //clear interrupt
	 
	 //Bus Clock 24MHz / 32 = 750K
	 //1000 RPM is 166.666Hz
	 //TC4 Clock / Input freq = TC4 Value
	 //750k / 166Hz = 4518
	 HeadTime = TC4 - OldHeadTime;
	 OldHeadTime = TC4; 
	 HeadSpeed = 750000/HeadTime*60;
	 if (HeadSpeed < 1000)
	     HeadSpeed = 0;
	 HdSpeedTimer = RTI_One_Sec * .25;

	#ifdef diag 
	 //Stuff for the CNT-024
	HeadSpeed=8000;
	#endif			  	
}

void TC5_Int_Handler ( void )
{
 	 TFLG1 = 0x20;	   		//clear interrupt
 	 TC5 = TC5 + TC_1ms;
	 MCOHW_TimerISR();
}

void TC7_Int_Handler ( void )
{

 	 TFLG1 = 0x80;	   		//clear interrupt 
	 //TIE &= ~TIE_C2I;		//disable TC2 Interrupt
	//PORTB ^= 0x01;
 	/*if(prevDir == (Encoder_Port & Encoder_Dir)){

	}*/


		//The circuit that sends in the encoder data switches between switching on a rising edge and a falling edge depending on the direction of the encoder.
		//This means whenever the direction changes we need to alter TCTL3 to check for the corresponding type.

	 //if (( TC0_RCVD_Data & TeleData_CCW || TC0_RCVD_Data & TeleData_CW ))// Make sure button is being pressed 
	 {
		if(Encoder_Port & Encoder_Dir){
			TCTL3 &= ~0xc0;
			TCTL3 |= 0x40;
			CamPosition -= 1;
		}
		else{
			TCTL3 &= ~0xc0;
			TCTL3 |= 0x80;
			CamPosition += 1;
		}
	 }
	 //prevDir = (Encoder_Port & Encoder_Dir);
	 
/*	 if ( CamPosition < 0 )	 
	     CamPosition = 359 * RDR_Ratio;
		 
	 if ( CamPosition > 360 * RDR_Ratio)	 
	     CamPosition = 0;
*/
}

void RTI_Int_Handler ( void )
{
 	 CRGFLG = 0x80;	   	//clear interrupt

	StateTime++;

	if ( !HeadRampTimer-- )
	{
	    HeadRampTimer = HeadRampTime;
		
    	if ( HeadSpdOut < iHeadSpd )
    	{ 
    	    //HeadSpdOut++;
			HeadSpdOut=100;
    	}
    	
    	if ( HeadSpdOut > iHeadSpd )
    	{ 
    	    //HeadSpdOut--;
			HeadSpdOut=0;
    	}
	}
	
#ifdef diag
	//Stuff for the CNT-024
	HeadSpeed=8000;		
#endif
	
    if ( AdvanceTimer )
    {
        if ( !--AdvanceTimer )       
        {   //stop focus and zoom
			PORTA &= ~0x40;
        }
    }

    if ( FocusZoomTimer )
    {
        if ( !--FocusZoomTimer )       
        {   //stop focus and zoom
            PWMDTY3 = 0;
            PWMDTY4 = 0;
            PWMDTY5 = 0;
            PWMDTY7 = 0;
			PORTA &= ~0x0f;
        }
    }

	if ( CamDegTimer )
	{
	    --CamDegTimer;
	}

	
	++CamPIDTimer;
	
    
    if ( Timer1 )
    {
        if ( !--Timer1 )
            Gen_Flags &= ~Gen_Flags_Timer1;
    }
    
    if ( Timer2 )
    {
        //if ( !--Timer2 )
            //Gen_Flags &= ~Gen_Flags_Timer2;
         --Timer2;
    }
    
	if ( CamIndexTimer )
	{
	    CamIndexTimer--;
	}
	
	if ( BlowerTimer )
	{
	    BlowerTimer--;
	}
	
	if ( IncSpeedUpTimer )
    {
		//IncSpeedUpTimer = RTI_One_Sec;
        IncSpeedUpTimer--;
    }

	if ( CamTurnTimer )
	{
		CamTurnTimer--;
		if( !CamTurnTimer ){
			CamStopTimer = 0.75*RTI_One_Sec;
		}
	}
	/*else if( (CameraTurnTime.value > 0)){
		PWMDTY0 = 0;
		PWMDTY2 = 0;
	}*/
	

	if ( CamStopTimer )
	{
		CamStopTimer--;
	}
    
	if ( MenuTimer )
	{
	    MenuTimer--;
        
        if ( !MenuTimer )
        {
            if ( IncSpeedUpTimer )
            {
                IncSpeedUpCntr++;
            }
            else
            {
                IncSpeedUpCntr=0;
                fast_inc=0;
            }
                
            if ( IncSpeedUpCntr > IncSpeedUpCnt )
            {
                IncSpeedUpCntr--;
                fast_inc=1;
            }
        }      
	}
	
	if ( LAMoveTimer )
	    LAMoveTimer--;
		
	if ( LAMovingTimer )
	    LAMovingTimer--;
		
	if ( HdSpeedTimer )
	{
	    HdSpeedTimer--;
	}
	else
	{
        if ( !test )
            HeadSpeed = 0;
		//HeadSpeed = 8000;	
	}
	
	if ( Update_Menu_Timer > 0 )
	{
	    Update_Menu_Timer--;
	}
	
    if ( !(Tick--) )
    {
        Tick = RTI_One_Sec;
        if ( !(Seconds--) )
        {
            Seconds = 60;
        
        }
		
    	if ( HeaterTimer )
    	    HeaterTimer--;
		else
		    HeaterTimer = HeaterTime;
    }	 
	
}

#include "vectors.h"
