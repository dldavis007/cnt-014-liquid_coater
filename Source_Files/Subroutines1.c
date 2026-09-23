/*******************************************
201-CNT-014 Head Control Unit
24-48 Coater, Rev. D board

*******************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "Subroutines.h"
#include "Subroutines1.h"
#include "mc9s12a128.h"
#include "Interrupts.h"
#include "mcohw.h"
#include "EEProm.h"
#include "string.h"

int SenseLevel;
char Stop_flag=0;

extern char save_serial_flag;
extern char Gen_Flags;
extern unsigned int TC0_RCVD_Data;
extern CAN_MSG gTxNMT;
extern unsigned int Timer1;
extern unsigned int Timer2;
extern int Update_Menu_Timer;
extern unsigned int MenuTimer;
extern unsigned long  StateTime;
extern unsigned int BlowerTimer;
extern unsigned int FocusZoomTimer;
extern unsigned int AdvanceTimer;
extern unsigned int CamTurnTimer;
extern unsigned int CamStopTimer;
extern unsigned int CamPIDTimer;
unsigned int PrevCamPIDTime;

int inspectPIDChange = 0;

//float CamPIDTimeGap = 0.5;

char curr_PWM;
char min_effective_pwm;
char usePID = 1;


char CurrentCamPWM;
char PrevCamPWM;

extern struct menu_var CameraTurnTime;
extern char CamHome;
extern unsigned int CamIndexTimer;
extern struct menu_var MotorPol;
extern struct menu_var TYPE;


//saved seperately at 0x0b00
extern char cam_addx1[];      //unique camera address from ran_num
extern char cam_addx2[];      //unique camera address from ran_num

extern struct menu_var SerialNum;
extern int StrokeNum, StartPumpFlag;

extern struct menu_var FirstStrokes;
extern struct menu_var PumpSpd;
extern struct menu_var HeadSpd;
extern struct menu_var SecondStrokePmpSpd[10];
extern struct menu_var SecondStrokeLASpd[10];
extern struct menu_var SecondStrokeLen[10];
extern struct menu_var SecondStrokeCtr[10];
extern struct menu_var FirstStrokePmpSpd[10];
extern struct menu_var FirstStrokeLASpd[10];
extern struct menu_var FirstStrokeLen[10];
extern struct menu_var FirstStrokeCtr[10];
extern struct menu_var SecondStrokes;
extern struct menu_var PumpOnOff;
extern struct menu_var CleanOutStrokes;
extern struct menu_var HtrISOOnOff;
extern struct menu_var HtrISOSetPnt;
extern struct menu_var HtrBaseOnOff;
extern struct menu_var HtrBaseSetPnt;
extern struct menu_var HtrHoseOnOff;
extern struct menu_var HtrHoseSetPnt;
extern struct menu_var PGainISO;
extern struct menu_var LA_TYPE;
extern struct menu_var IGainISO;
extern struct menu_var IMaxISO;
extern struct menu_var PGainBase;
extern struct menu_var IGainBase;
extern struct menu_var IMaxBase;
extern struct menu_var HtrISOTemp;
extern struct menu_var HtrBaseTemp;
extern struct menu_var CameraSpd;
extern struct menu_var CameraTrip;
extern struct menu_var PumpOnOff;
extern struct menu_var HeadOnOff;
extern struct menu_var RetractTime;
extern struct menu_var Rev;
extern struct menu_var LightLevel1;
extern struct menu_var LightLevel2;
extern struct menu_var disp_add1;
extern struct menu_var disp_add2;
extern struct menu_var CamTag1;
extern struct menu_var CamTag2;

extern float LAPos;
extern char MoveCmdXmtd;
extern int OldPumpSpeed;
extern struct menu_var MaxLADist;
extern struct menu_var MachineSize;
extern char StackPointer;
extern struct MenuStack MenuStackc[];
extern char Menu[12][21];
extern signed char CursorDownFlag;
extern signed char CursorUpFlag;
extern signed char SelectFlag;
extern char CamAddressXmitd;

extern char UpdateMenu;
extern char Variable_flag;
extern char StrokeFlag;
extern char StrokePos;
extern int HeaterTimer;
extern char DispDegree;
extern int CamPosition;
extern char CamHome;

extern unsigned char CameraSpeed;
extern int Rotate;
extern int Sense_Direction;
extern unsigned char Sense_Detected;
extern unsigned char CameraSpeed;

extern float RDR_Ratio;
extern int CamDegTimer;
extern int CamDegree;
int PrevCamPos = 0;
extern int CamDegXmtd;
extern char HeadSpdOut;

extern unsigned cam_add1;
extern unsigned cam_add2;

extern unsigned ran_num;  //used to create unique camera address  

extern char CurrentLight;
extern char lightval;

extern CAN_MSG gTxMsg;

extern char State;
extern char Cycle_Complete;

extern char ghostState;	//used in switch statement of ghost band sequence	

extern char Stop_Detected;
extern float HeadSpeed;
extern char iHeadSpd;

char LAError;
char OddStroke;
char odd_even_counter;

extern unsigned int LAMoveTimer;
extern unsigned long LAMovingTimer;
extern unsigned long Temp_LAMovingTimer;

char pressed_first;

struct PID inspect_cam_PID ={
	1,0.0,0.0,0,0,0,0
};

float min_dt = 0.05;
float inspectPIDStrength = 0.22;
char inspectCamDirection = 0; // 0 = stopped, 1= PWMDTY0 == 0 and PWMDTY2 == speed, 2 = opposite

float Cam_speed;

extern char Store_Flag;

/***************************************************************************/ 

// external declaration for the process image array
extern UNSIGNED8 gProcImg[];

/**************************************************************************/




void Save_Camera_Add1 ( void )
{
    char *EEpromPtr;
    
    EEpromPtr = &cam_addx1[0];

 	EEWrite ( 2, EEpromPtr, (int *)(0x0300 + EE_begin) );
}

void Save_Camera_Add2 ( void )
{
    char *EEpromPtr;
    
    EEpromPtr = &cam_addx2[0];

 	EEWrite ( 2, EEpromPtr, (int *)(0x0302 + EE_begin) );
}


//retreive Serial Number from EEProm
void Load_Serial_Num ( void )
{
    char tempstr[130];
    char *VarEEPROMPntr2;
	
    VarEEPROMPntr2 = (char *)(0x0310 + EE_begin);
    
    if ( *VarEEPROMPntr2 >= '0' && *VarEEPROMPntr2 <= '9')
	{
    	SerialNum.str_value[0] = *VarEEPROMPntr2;
        SerialNum.str_value[1] = *(VarEEPROMPntr2 + 1);   
        SerialNum.str_value[2] = *(VarEEPROMPntr2 + 2);   
        SerialNum.str_value[3] = *(VarEEPROMPntr2 + 3);   
        SerialNum.str_value[4] = *(VarEEPROMPntr2 + 4);   
        SerialNum.str_value[5] = *(VarEEPROMPntr2 + 5); 
		SerialNum.str_value[6] = 0;
	}
	else{		   
		
		save_serial_flag=1;   
	}  
}

void Save_Serial_Num ( void )
{
    char *EEpromPtr;
    
    EEpromPtr = &SerialNum.str_value[0];

 	EEWrite ( 7, EEpromPtr, (int *)(0x0310 + EE_begin) );
}

void MoveLA (float Pos, int Spd, int Current )
{
   LAMoveTimer = LAMoveTime; 
   
   LAPos = Pos;
   
   if(LA_TYPE.value==2)
   {
      if ( !gProcImg[IN_digi_12] && !gProcImg[IN_digi_12+1] ) 
        Temp_LAMovingTimer = LAMovingTime * 26 * 100.0/30; 
      else 
	    Temp_LAMovingTimer = (RTI_One_Sec * 3) + (LAMovingTime * abs(gProcImg[IN_digi_12]-Pos*10)/10 * 100.0/Spd);
   
	  CRGINT &= ~0x80; //temporarily disable RTI Interrupts
	  LAMovingTimer = Temp_LAMovingTimer;  //This is a long variable, it requires 2 writes (4 bytes)
	  CRGINT |= 0x80; //re-enable RTI Interrupts
    
	
	  gProcImg[IN_digi_12] = Pos*10;
	  gProcImg[IN_digi_12+1] = Spd;
	  if ( gProcImg[IN_digi_12+2] & ~0x01 != Current & ~0x01)  // make sure it is set to current (ignoring bit 0)
	    gProcImg[IN_digi_12+2] = Current;
	  gProcImg[IN_digi_12+2] = gProcImg[IN_digi_12+2] ^ 0x01;  // toggle bit 0 to make sure message is sent (data changed)	
   }
   else
   {
   	   if ( !gProcImg[IN_digi_32] && !gProcImg[IN_digi_32+1] ) 
        Temp_LAMovingTimer = LAMovingTime * 26 * 100.0/30; 
       else 
	    Temp_LAMovingTimer = (RTI_One_Sec * 3) + (LAMovingTime * abs(gProcImg[IN_digi_32]-Pos*10)/10 * 100.0/Spd);
   
	  CRGINT &= ~0x80; //temporarily disable RTI Interrupts
	  LAMovingTimer = Temp_LAMovingTimer;  //This is a long variable, it requires 2 writes (4 bytes)
	  CRGINT |= 0x80; //re-enable RTI Interrupts
    
	
	  gProcImg[IN_digi_32] = Pos*10;
	  gProcImg[IN_digi_32+1] = Spd;
	  if ( gProcImg[IN_digi_32+2] & ~0x01 != Current & ~0x01)  // make sure it is set to current (ignoring bit 0)
	    gProcImg[IN_digi_32+2] = Current;
	  gProcImg[IN_digi_32+2] = gProcImg[IN_digi_32+2] ^ 0x01;  // toggle bit 0 to make sure message is sent (data changed)
   	   
   }	
}

int ATDGetLevel ( char ATD_Num )
{
 	ATD0CTL5=ATD0CTL5_Init | ATD_Num;
	while ( !(ATD0STAT0 & 0x80) );
	
	return (ATD0DR0+ATD0DR1+ATD0DR2+ATD0DR3)/4; 				
}

int SecondCoatSeq ( int Start )
{
/*
Second

If no First
Start by going to retracted position and start pumps, then extend to extended position

If Center
Start by moving to next position (extend or retract, depending on odd_even_counter)
*/
	float Pos,Len,Cntr;
	int LASpeed,PumpSpeed;
    char tempstr[25];
	
    if ( Start ) // Start == 1 if first starting, else 0 for coating
	{
	    StrokeNum = 0;
		LAError=0;
		if ( !FirstStrokes.value )
		{
		    odd_even_counter = 0;
			MoveLA ( SecondStrokeCtr[0].value  - ( SecondStrokeLen[0].value/2 ), 80, LACurrent );//Move to near end of first stroke
			MoveCmdXmtd = 1;			
			OldPumpSpeed = PumpSpd.value; //Save Pump Value if no First Strokes
		}
	}
	else if ( gProcImg[OUT_digi_7] )
	{   //If moving, return without doing anything
		CRGINT &= ~0x80; //temporarily disable RTI Interrupts
		Temp_LAMovingTimer = LAMovingTimer; //This is a long and may be affected by RTI
		CRGINT |= 0x80; //re-enable RTI Interrupts
		if (!Temp_LAMovingTimer)
		{
		    LAError=1;
		 	Display ("Proc:Moving Too Long");
		}
		MoveCmdXmtd = 0;
	    return 0;
	}
	else if ( MoveCmdXmtd  )
	{   //Wait until moving if move command just sent
		if (!LAMoveTimer)
		{
		    LAError=1;
		 	Display ("Proc:Not Moving");
		}
		return 0;
	}
	else 
	{   //If not Start and not Moving, initiate next movement
    	if ( StrokeNum >= SecondStrokes.value || StrokeNum >= 10 )
		{   //If done with full cycles, turn off pump
			strncpy(PumpOnOff.str_value,"OFF",PumpOnOff.len_str);
			getvalue(&PumpOnOff,0);
			PumpSpd.value = OldPumpSpeed;
			getstrval( &PumpSpd );
		    return 1;
		}
    	Cntr = SecondStrokeCtr[StrokeNum].value;
    	Len = SecondStrokeLen[StrokeNum].value;
		LASpeed = SecondStrokeLASpd[StrokeNum].value;
		PumpSpeed = SecondStrokePmpSpd[StrokeNum].value;
	    if ( StrokeNum == 0  )
		{
			if ( odd_even_counter & 1 )
			    Pos = Cntr - ( Len/2 );
			else
		        Pos = Cntr + ( Len/2 );
				
			if ( !FirstStrokes.value )
			{
				strncpy(PumpOnOff.str_value," ON",PumpOnOff.len_str);
    			getvalue(&PumpOnOff,0);
    			Pos = Cntr + ( Len/2 );
			    odd_even_counter = 0; //Start a zero if no First Strokes
			}	
		}
	    else if ( odd_even_counter & 1 )
		{   //Odd strokes
		    Pos = Cntr - ( Len/2 );
		}
		else
		{   //Even strokes
		    Pos = Cntr + ( Len/2 );
		}
		//Set Pump speed, LAPos for most strokes
		PumpSpd.value = PumpSpeed;
		getstrval( &PumpSpd );
		//Send move command
		MoveLA ( Pos, LASpeed, LACurrent );
		MoveCmdXmtd = 1;
		StrokeNum++;
		odd_even_counter++;
		sprintf (tempstr,"Proc:SecondStrokes %d",StrokeNum);
		//DispRatioTimer = DispRatioTime;//Restart Display Ration Timer so Stroke message doesn't get overwritten 
        Display ( tempstr );
		
	}
	return 0;
}


int CleanCoatSeq ( int Start )
{
/*
Clean

Start by extending

If odd number of strokes, stop in center after moving from retracted position
If even number of strokes, stop in center after moving from extended position

Stop in center
*/
	float Pos,Len,Cntr;
	int LASpeed;
	char tempstr[25];
	
    if ( Start ) // Start == 1 if first starting, else 0 for coating
	{
		LAError=0;
	    StrokeNum = 0;
	}
	else if ( gProcImg[OUT_digi_7] )
	{   //If moving, return without doing anything
		CRGINT &= ~0x80; //temporarily disable RTI Interrupts
		Temp_LAMovingTimer = LAMovingTimer; //This is a long and may be affected by RTI
		CRGINT |= 0x80; //re-enable RTI Interrupts
		if (!Temp_LAMovingTimer)
		{
		    LAError=1;  //Error if moving too long
		 	Display ("Proc:Moving Too Long");
		}
		MoveCmdXmtd = 0;
	    return 0;
	}
	else if ( MoveCmdXmtd  )
	{   //Wait until moving if move command just sent
	    if (!LAMoveTimer)
		{
		    LAError=1;  //Error if not moving in time
		 	Display ("Proc:Not Moving");
		}
		return 0;
	}
	else 
	{   //If not Start and not Moving, initiate next movement
    	if (SecondStrokes.value)//if second strokes were use, use their settings for Cleanout
		{
		    Len = SecondStrokeLen[(int)SecondStrokes.value-1].value;
			LASpeed = SecondStrokeLASpd[(int)SecondStrokes.value-1].value;
		    Cntr = SecondStrokeCtr[(int)SecondStrokes.value-1].value;
		}
		else
		{
		    Len = FirstStrokeLen[(int)FirstStrokes.value-1].value;
			LASpeed = FirstStrokeLASpd[(int)FirstStrokes.value-1].value;
			Cntr = FirstStrokeCtr[(int)FirstStrokes.value-1].value;
		}
		
		if ( StrokeNum >= CleanOutStrokes.value || StrokeNum >= 10 )
		{   //If done with cleanout cycles
		    return 1;
		}
    	
	    else if ( odd_even_counter & 1 )
		{   //Odd strokes
		    Pos = Cntr - ( Len/2 );
		}
		else
		{   //Even strokes
		    Pos = Cntr + ( Len/2 );
		}
		//Send move command
		MoveLA ( Pos, LASpeed, LACurrent );
		MoveCmdXmtd = 1;
		StrokeNum++;
		odd_even_counter++;
		sprintf (tempstr,"Proc:Cleanout %d",StrokeNum);
        Display ( tempstr );
	}
	return 0;
}


int FirstCoatSeq ( int Start )
{
/*First
Start by going to retracted position and start pumps, then extend to extended position
*/
	float Pos,Len,Cntr;
	int LASpeed,PumpSpeed;
    char tempstr[25];
	
    if ( Start ) // Start == 1 if first starting, else 0 for coating
	{
		StrokeNum = 0;
		LAError=0;
		odd_even_counter = 0;
		MoveLA ( FirstStrokeCtr[0].value  - ( FirstStrokeLen[0].value/2 ), 80, LACurrent );//Move to near end of first stroke
		MoveCmdXmtd = 1;
		OldPumpSpeed = PumpSpd.value;
	}
	else if ( gProcImg[OUT_digi_7] )
	{   //If moving, return without doing anything
		CRGINT &= ~0x80; //temporarily disable RTI Interrupts
		Temp_LAMovingTimer = LAMovingTimer; //This is a long and may be affected by RTI
		CRGINT |= 0x80; //re-enable RTI Interrupts
    	if (!Temp_LAMovingTimer)
		{
		    LAError=1;  //Error if moving too long
		 	Display ("Proc:Moving Too Long");
		}
		MoveCmdXmtd = 0;
	    return 0;
	}
	else if ( MoveCmdXmtd  )
	{   //Wait until moving if move command just sent
		if (!LAMoveTimer)
		{
		    LAError=1;  //Error if not moving in time
		 	Display ("Proc:Not Moving");
		}
		return 0;
	}
	else 
	{
		if ( StrokeNum >= FirstStrokes.value || StrokeNum >= 10 ) //See if finished with First Strokes
		{   //If done with Center cycles, turn off pump 
			if  ( !SecondStrokes.value ) //See if there are Second Strokes
			{   //If no Second Strokes, turn pump off
			    strncpy(PumpOnOff.str_value,"OFF",PumpOnOff.len_str);
				getvalue(&PumpOnOff,0);
				PumpSpd.value = OldPumpSpeed;
				getstrval( &PumpSpd );
			}
		    return 1;
		}
    	Cntr = FirstStrokeCtr[StrokeNum].value;	 	 //Get the center position
		Len =  FirstStrokeLen[StrokeNum].value;
		LASpeed =  FirstStrokeLASpd[StrokeNum].value;
		PumpSpeed = FirstStrokePmpSpd[StrokeNum].value;
	    if ( StrokeNum == 0  )//Start pump at start of first stroke
		{
			strncpy(PumpOnOff.str_value," ON",PumpOnOff.len_str);
			getvalue(&PumpOnOff,0);
		    Pos = Cntr + ( Len/2 );//First stroke ends at far end
		    odd_even_counter = 0;
		}
	    else if ( odd_even_counter & 1 )
		{   //Odd strokes
		    Pos = Cntr - ( Len/2 );
		}
		else
		{   //Even strokes
		    Pos = Cntr + ( Len/2 );
		}
		//Set Pump speed
		PumpSpd.value = PumpSpeed;
		getstrval( &PumpSpd );
		//Send move command
		MoveLA ( Pos, LASpeed, LACurrent );
		MoveCmdXmtd = 1;
		StrokeNum++;
		odd_even_counter++;
		sprintf (tempstr,"Proc:FirstStrokes %d",StrokeNum);
//		DispRatioTimer = DispRatioTime;//Restart Display Ration Timer so Stroke message doesn't get overwritten 
        Display ( tempstr );
	}
	
	return 0;
}



extern char *RamAddress;
extern char RamData;
extern char RamCkSum;

void doevents ( void )
{
    int i,j;
		
	   //if the 2-Wire system is working, start updating the 2-Wire Stack


		if ( Store_Flag && State == FinishState) //To prevent timeout while exiting menu after changing settings during sequence -EBB
        {
            Store_Flag = 0;
            Save_Variables();
        }
	   	  
		    if ( ( TC0_RCVD_Data & TeleData_CamTog1 ) && CursorDownFlag == 0 )
                CursorDownFlag = 1;
            else if ( CursorDownFlag == -1 && !( TC0_RCVD_Data & TeleData_CamTog1 ) )
                CursorDownFlag = 0;
            
            if ( ( TC0_RCVD_Data &  TeleData_CamTog2 ) && CursorUpFlag == 0 )
                CursorUpFlag = 1;
            else if ( CursorUpFlag == -1 && !( TC0_RCVD_Data & TeleData_CamTog2 ) )
                CursorUpFlag = 0;
            
            if ( ( TC0_RCVD_Data & TeleData_PLCTrig ) && SelectFlag == 0 )
                SelectFlag = 1;
            else if ( SelectFlag == -1 && !( TC0_RCVD_Data & TeleData_PLCTrig ) )
                SelectFlag = 0;
			
		
		   if( MachineSize.value == 2 ) //24
	          gProcImg[IN_digi_22] = 0b00100101;  //Heater 1 (xxxxxx01) & 2 (xxxx01xx) = RTD 1,  Heater 3 (xx10xxxx) = RTD 2, RTD 3 not used
		   else
	          gProcImg[IN_digi_22] = 0b00011001;  //Heater 1 (xxxxxx01) & 3 (xx01xxxx) = RTD 1, Heater 2 (xxxx10xx) = RTD 2,  RTD 3 (not used)  
		   
           gProcImg[IN_digi_15] = ((HtrBaseSetPnt.value)-32)*5/9;		 //Heater 1 = Base Set Point
           if( HtrBaseOnOff.value == 1 )//off 
		   	  gProcImg[IN_digi_15] = 0;

           gProcImg[IN_digi_16] = ((HtrISOSetPnt.value)-32)*5/9;    	 //Heater 2 = ISO Set Point
           if ( HtrISOOnOff.value == 1 ) //off 
		   	  gProcImg[IN_digi_16] = 0;   

           gProcImg[IN_digi_17] = ((32)-32)*5/9;   	 
           if ( 1 ) gProcImg[IN_digi_17] = 0;	 			   		 //Heater 3 = Not Used 

           gProcImg[IN_digi_19] = PGainBase.value;			   	 	 //Heater 1 PID values (Base)
           gProcImg[IN_digi_20] = (char)(IGainBase.value)*100+1;
           gProcImg[IN_digi_21] = (IMaxBase.value);

           gProcImg[IN_digi_23] = PGainISO.value;			   		 //Heater 2 PID values (ISO)
           gProcImg[IN_digi_24] = (char)(IGainISO.value)*100+1;
           gProcImg[IN_digi_25] = IMaxISO.value;

           gProcImg[IN_digi_27] = (10);			   		 	 //Heater 3 PID values 
           gProcImg[IN_digi_28] = (char)(0.03)*100+1;
           gProcImg[IN_digi_29] = (20);


		   	   
            if ( TC0_RCVD_Data != ( gProcImg[OUT_digi_2]<<8 | gProcImg[OUT_digi_1] ) )
            {
                TC0_RCVD_Data = gProcImg[OUT_digi_2]<<8 | gProcImg[OUT_digi_1];
                if ( MenuTimer < MenuTime )   
                   MenuTimer = 0;
            }
            if ( gProcImg[OUT_digi_0] & 0x01 && !(Gen_Flags & Gen_Flags_Menu_Active) )
            {
            	Timer1 = RTI_One_Sec * .10;
            	while ( Timer1 );
                gProcImg[IN_digi_0] |= 0x01;
				
				i = MCO_ProcessStack();
            	Timer1 = RTI_One_Sec * .10;
            	while ( Timer1 );
                
                gProcImg[OUT_digi_0] &= ~0x01;
				
				StackPointer = 0;
                MenuStackc[StackPointer].Index[0] = 0;
                MenuStackc[StackPointer].Index[1] = 0;
                MenuStackc[StackPointer].Index[2] = 0;
                MenuStackc[StackPointer].Index[3] = 0;
                MenuStackc[StackPointer].CursorPos = 1;
                MenuStackc[StackPointer].FirstLine = 0;
                Gen_Flags |= Gen_Flags_Menu_Active;
                
                LoadMenu ( MenuStackc[StackPointer].Index );
                InsertCursor ();
                DisplayTitler ();			
                TC0_RCVD_Data &= ~0x07;  //Make sure up/down/select not active
    			CursorDownFlag = 0;
    			CursorUpFlag = 0;
    			SelectFlag = 0;

				//gProcImg[IN_digi_0] |= 0x01;
            }

           
            
	       if ( !MenuTimer )
		   {
			   MenuTimer = MenuTime;
			   
	   	   	   if ( Gen_Flags & Gen_Flags_Menu_Active )
			   {
        			//Limit the Center and Length setting according to MaxLADist and to each other
			         //Length has to be less than 2 times the center, etc...
					 int i;
			    	 //Check coating lengths while menu is active
			    	 for (i=0;i<10;i++)
					 {
				 	    CkCntrLength(FirstStrokeLen+i);
						CkCntrLength(SecondStrokeLen+i);
					 }
					
					if ( CursorDownFlag )
					{
           				CursorDown ();
						CursorDownFlag = -1;
					}
    				if ( CursorUpFlag )
					{
           				CursorUp ();
						CursorUpFlag = -1;
					}					
				
           			if ( SelectFlag )
					{
   					    MenuTimer = MenuTime * 2; //briefly disable select after menu is selected
           				Select ();          //select menu item at cursor position
						SelectFlag = -1;
					}					 
					
				    {
        			    char tempstr[4];
        			    sprintf (tempstr, "%3.0f", ( (float) ( gProcImg[OUT_ana_0] )  *  9 / 5 + 32 ) );
        				if ( atoi( tempstr ) != ( HtrBaseTemp.value ) )
						{
        				    
                            
							if ( MenuStackc[StackPointer].Index[0] == 0 && MenuStackc[StackPointer].Index[1] == 0 && 
        					     MenuStackc[StackPointer].Index[2] == 0 && MenuStackc[StackPointer].Index[3] == 4 &&
								 strcmp ( HtrBaseTemp.str_value, tempstr) && !HeaterTimer )
						    {
									 UpdateMenu = 1;
        							 HtrBaseTemp.value = atoi(tempstr);
									 strcpy ( HtrBaseTemp.str_value, tempstr);
							}
						}
        			    sprintf (tempstr, "%3.0f", ( (float)( gProcImg[OUT_ana_1] ) *  9 / 5 + 32 ) );
        				if ( atoi( tempstr ) != ( HtrISOTemp.value ) )
        				{
							
                            if ( MenuStackc[StackPointer].Index[0] == 0 && MenuStackc[StackPointer].Index[1] == 0 && 
        					     MenuStackc[StackPointer].Index[2] == 0 && MenuStackc[StackPointer].Index[3] == 4 &&
								 strcmp ( HtrISOTemp.str_value, tempstr) && !HeaterTimer )
							{
        				    	     UpdateMenu = 1;
        							 HtrISOTemp.value = atoi(tempstr);
									 strcpy ( HtrISOTemp.str_value, tempstr);
							}
						}
        			    sprintf (tempstr, "%3.0f", ( (float) ( gProcImg[OUT_ana_2] ) *  9 / 5 + 32 ) );
        			}
				   				   			   
				    if ( UpdateMenu )
				    {
                       LoadMenu ( MenuStackc[StackPointer].Index );
                       if ( !Variable_flag )
					       InsertCursor ();
                       DisplayTitler ();
					   UpdateMenu = 0;
				    }
				}			   
		    }		
			
			if ( ( MachineSize.value ) == 2 ) //24
			 	 RDR_Ratio = RDR24;
	   		else
			     RDR_Ratio = RDR12;
			

			//Time Calculation for dt in cam PID loop

			/*
			if (CamPIDTimer <= PrevCamPIDTimer) {
				// Normal case: no wraparound
				Cam_dt = (PrevCamPIDTimer - CamPIDTimer)/RTI_One_Sec; // In seconds the difference in time
			} else {
				// Wraparound case
				Cam_dt = ((65535 - PrevCamPIDTimer) + CamPIDTimer + 1)/RTI_One_Sec; // If we wraparound subtract from the unsigned int max
			}
			*/


			//PrevCamPIDTimer = CamPIDTimer;

			//CALCULATE DT for INSPECT CAM PID

			

			if(CamPIDTimer < PrevCamPIDTime){ // Fix wraparound for PID Timer
				CamPIDTimer = 65535 - PrevCamPIDTime;
				PrevCamPIDTime = 0;
			}
			

			inspect_cam_PID.dT = (CamPIDTimer - PrevCamPIDTime)/RTI_One_Sec; // Calculate dT for inspect_cam


			if(inspect_cam_PID.dT >= min_dt){ // Make sure enough time has passed so that the loops arent insanely short
				
				PrevCamPIDTime = CamPIDTimer;
				Cam_speed = GetRotationalSpeed(CamPosition,PrevCamPos,inspect_cam_PID.dT); 
				PrevCamPos = CamPosition;

 				if ((fabs(Cam_speed) < 0.1) && (CurrentCamPWM > 0) && (inspect_cam_PID.desired_value < 1)){
					CurrentCamPWM = 0; // If PWM is low and speed is desired to be 0 then set PWM to 0

				}
				else{//business as usual

					inspectPIDChange = (int)((PID_Loop(&inspect_cam_PID,Cam_speed)));

					if(CurrentCamPWM + inspectPIDChange > 100){ //Cap PWM at 100.
						CurrentCamPWM = 100;
					}
					else if(CurrentCamPWM + inspectPIDChange <= 0){ // Limit PWM at 0.
						CurrentCamPWM = 0;
					}
					else{ // Add PID change like normal
						CurrentCamPWM = CurrentCamPWM + (char)inspectPIDChange; 
					}
				}

			}


			


			if ( (TC0_RCVD_Data & TeleData_CCW) || (TC0_RCVD_Data & TeleData_CW) )
			{
				inspect_cam_PID.desired_value = (CameraSpd.value)*inspectPIDStrength;
				if(!(CameraSpd.value < 1) && inspect_cam_PID.desired_value < 1){
					inspect_cam_PID.desired_value = 1;
				}
				
			    //char temp_spd = CurrentCamPWM;
				//CurrentCamPWM = temp_spd;
				//char temp_spd = CurrentCamPWM;
				//CurrentCamPWM = AdjustPWMForMotion(CurrentCamPWM,Cam_speed);	
				

       	    	if(TC0_RCVD_Data & TeleData_CW && TC0_RCVD_Data & TeleData_CCW){								
       			    inspect_cam_PID.desired_value = 100*inspectPIDStrength;;	
					CamTurnTimer = 1*RTI_One_Sec;		
				}
				SenseLevel = ATDGetLevel (6);
							
				if (  SenseLevel > (1007.0/100) * (CameraTrip.value) && SenseLevel < 0x3f0 )  // (1024/100)*100% = 1024 full scale   
				{  	
						//inspectCamDirection = 0;
						inspect_cam_PID.desired_value = 0;
						//PWMDTY0 = 0;
						//PWMDTY2 = 0;					
						Rotate = 0;
					 	CamHome = 1;
						
						if ( TC0_RCVD_Data & TeleData_CW)
						{
						     Stop_flag=1;														
							 CamPosition = (360 - (STOPWIDTH)) * RDR_Ratio;														 
							 
						}
						else if( TC0_RCVD_Data & TeleData_CCW)
						{
							 Stop_flag=2;  	   
							 CamPosition=0;
						}	 
				}
				
				if(CamHome && CamPosition<= STOPMARGIN*RDR_Ratio)
				{
				 	Stop_flag=2;
					//inspectCamDirection = 0;
					//inspect_cam_PID.desired_value = 0;	   
					//PWMDTY0 = 0;
					//PWMDTY2 = 0;		
				}
				if(CamHome && CamPosition>=(((360 - STOPWIDTH)*RDR_Ratio)-STOPMARGIN*RDR_Ratio))
				{
				    Stop_flag=1;
					//inspectCamDirection = 0;
					//inspect_cam_PID.desired_value = 0;
					//PWMDTY0 = 0;
					//PWMDTY2 = 0;
				}				
						
				if ( (TC0_RCVD_Data & TeleData_CW)  && Stop_flag!=1 && pressed_first!=2 && (!CamStopTimer || (CameraTurnTime.value < 0.1) ))
				{
					 Stop_flag=0;					 
					 pressed_first=1;

					 if(!CamTurnTimer && !CamStopTimer){
						CamTurnTimer = CameraTurnTime.value*RTI_One_Sec;
					 }

					 if(MotorPol.value==2)
					 {
					  	    if( ( MachineSize.value ) == 2)  //24
					    	{
								inspectCamDirection = 1;
						     	//PWMDTY0 = 0;
						     	//PWMDTY2 = temp_spd;
							}
							else 
							{
    					     	inspectCamDirection = 2;
								//PWMDTY0 = temp_spd;    							
								//PWMDTY2 = 0;
							}				   	 
					 }
					 else
					 {
					  	    if( ( MachineSize.value ) == 2)  //24
					    	{
						     	inspectCamDirection = 2;
								//PWMDTY0 = temp_spd;
						    	//PWMDTY2 = 0;
							}
							else 
							{
								inspectCamDirection = 1;
    					     	//PWMDTY0 = 0;    							
								//PWMDTY2 = temp_spd;
							}
					 }    	
									 
					 Rotate=1;
				}
				else if(TC0_RCVD_Data & TeleData_CCW && Stop_flag!=2 && pressed_first!=1 && (!CamStopTimer  || (CameraTurnTime.value < 0.1) )) //had to be CW to get here
				{
					 Stop_flag=0;
					 pressed_first=2;
					 
					 if(!CamTurnTimer && !CamStopTimer){
						CamTurnTimer = CameraTurnTime.value*RTI_One_Sec;
					 }

					 if(MotorPol.value==2)
					 {
					  	    if( ( MachineSize.value ) == 2)  //24
					    	{
						     	inspectCamDirection = 2;
								//PWMDTY0 = temp_spd;
						    	//PWMDTY2 = 0;
							}
							else 
							{
								inspectCamDirection = 1;
    					     	//PWMDTY0 = 0;    							
								//PWMDTY2 = temp_spd;
							}				   	 
					 }
					 else
					 {
					  	    if( ( MachineSize.value ) == 2)  //24
					    	{
								inspectCamDirection = 1;
						     	//PWMDTY0 = 0;						    	
								//PWMDTY2 = temp_spd;
							}
							else 
							{
    					     	inspectCamDirection = 2;
								//PWMDTY0 = temp_spd;
    							//PWMDTY2 = 0;
							}
					 }    	   	
					 Rotate=-1;
				}
				else
				{
					Rotate=0;
					//inspectCamDirection = 0;
					inspect_cam_PID.desired_value = 0;
					//PWMDTY0 = 0;
					//PWMDTY2 = 0;
					//pressed_first=0;
					if ( (TC0_RCVD_Data & TeleData_CCW) || (TC0_RCVD_Data & TeleData_CW) ){ // If both buttons are held and then the buttopn pressed first is changed this allows for this change
						if((TC0_RCVD_Data & TeleData_CCW) && (TC0_RCVD_Data & TeleData_CW)){
							//NOTHING
						}
						else if((TC0_RCVD_Data & TeleData_CCW)){
							pressed_first = 2;
						}
						else if((TC0_RCVD_Data & TeleData_CW)){
							pressed_first = 1;
						}
					}
					else{
						pressed_first = 0;
					}

				}
								
									   
				DispDegree = 1;	

			
			}
			else 
			{
				Rotate=0;
				//inspectCamDirection = 0;
				inspect_cam_PID.desired_value = 0;
				//PWMDTY0 = 0;
				//PWMDTY2 = 0;
				pressed_first=0;

				//PrevCamPWM = 0;
			}

			switch(inspectCamDirection){
				case 1:
				if(Stop_flag != 1){
					PWMDTY2 = CurrentCamPWM;
					PWMDTY0 = 0;
				}
					else{
						PWMDTY0 = 0;
						PWMDTY2 = 0;
					}

				break;

				case 2:
					if(Stop_flag != 2){
						PWMDTY2 = 0;
						PWMDTY0 = CurrentCamPWM;
					}
					else{
						PWMDTY0 = 0;
						PWMDTY2 = 0;
					}

				break;
			}
			


			if ( DispDegree && !CamDegTimer )
            {


				
                char tmpstr[15];
                
				
				DispDegree = 0;
				CamDegTimer = CamDegTime;
				CamDegree = CamPosition/RDR_Ratio;
				
				CamDegree+=184;
				
				CamDegree = CamDegree%360;				
				
			    if ( CamDegXmtd != CamDegree )
                {
                   	CamDegXmtd = CamDegree;
                    if ( CamHome )
                        sprintf (tmpstr, "Icam:%3d",CamDegree);
                    else
                        sprintf (tmpstr, "Icam:---");
                    Display ( tmpstr );
                }
            }		

			if ( !strcmp(HeadOnOff.str_value," ON") ) //on
			{
			    if ( (int)HeadSpdOut < 100 ){			
				   HeadSpdOut += .25;
				   iHeadSpd = 100;
				}
			}
			else 
			{
			    if ( (int)HeadSpdOut != 0 ){				
				   HeadSpdOut -=.25;
				   iHeadSpd = 0;
				}
			}
			PWMDTY1 = HeadSpdOut;

			if ( PumpOnOff.value == 2 )//on
			{
			    gProcImg[IN_digi_1] = ( PumpSpd.value );
			}
			else
			{
			    gProcImg[IN_digi_1] = 0;
			}
			
			if ( BlowerTimer )
				HeadBlower_Port |= HeadBlower;
			else
			    HeadBlower_Port &= ~HeadBlower;
				
				
            if ( MenuStackc[StackPointer].Index[0] == 0 &&
                 MenuStackc[StackPointer].Index[1] == 0 &&
                 MenuStackc[StackPointer].Index[2] == 4 &&
                 MenuStackc[StackPointer].Index[3] == 1 &&
				 ( gProcImg[OUT_digi_8] != ( cam_add1  & 0x00FF ) ||
				 gProcImg[OUT_digi_9] != cam_add1>>8 ) )
			{
                if ( !CamAddressXmitd )
    			{
    			    gProcImg[OUT_digi_8] = cam_add1;
                    gProcImg[OUT_digi_9] = cam_add1>>8;			
    			    CurrentLight = 1;
    				
                    gTxMsg.ID = 0x421;
                    gTxMsg.LEN = 2; 
                    gTxMsg.BUF[0] = cam_add1;
                    gTxMsg.BUF[1] = cam_add1 >> 8;      
                    if (!MCOHW_PushMessage(&gTxMsg))
                    {
                        // failed to transmit
                        MCOUSER_FatalError(0x8801);
                    }
                     //! Transmit this without using the TPDO
    			}
    		    CamAddressXmitd = 1;
				
			}
            else if ( MenuStackc[StackPointer].Index[0] == 0 &&
                 MenuStackc[StackPointer].Index[1] == 0 &&
                 MenuStackc[StackPointer].Index[2] == 4 &&
                 MenuStackc[StackPointer].Index[3] == 2 &&
				 ( gProcImg[OUT_digi_8] != ( cam_add2 & 0x00FF ) ||
				 gProcImg[OUT_digi_9] != cam_add2>>8 ) )
			{
                if ( !CamAddressXmitd )
    			{
    			    gProcImg[OUT_digi_8] = cam_add2 & 0x00FF;
                    gProcImg[OUT_digi_9] = cam_add2>>8;			
    			    CurrentLight = 2;
    
                    gTxMsg.ID = 0x421;
                    gTxMsg.LEN = 2; 
                    gTxMsg.BUF[0] = cam_add2;
                    gTxMsg.BUF[1] = cam_add2 >> 8;      
                    if (!MCOHW_PushMessage(&gTxMsg))
                    {
                        // failed to transmit
                        MCOUSER_FatalError(0x8801);
                    }
                     //! Transmit this without using the TPDO
    			}
    		    CamAddressXmitd = 1;
				
			}
    		else
    		{
    		    CamAddressXmitd = 0;
    		}


            if ( CurrentLight == 2 )
            {
                //VideoSw_Port |= VideoSw;
                lightval =  ( LightLevel2.value ) * ( LightLevel2.value );
            }
            else
            {
                //VideoSw_Port &= ~VideoSw;
                lightval = ( LightLevel1.value ) * ( LightLevel1.value );
            }
				 
        	//Turn on the blower when PLC_Trig and Cam_Tog1 pressed
            if ( !(Gen_Flags & Gen_Flags_Menu_Active) && (TC0_RCVD_Data & TeleData_PLCTrig) && (TC0_RCVD_Data & TeleData_CamTog1) )
            {
              BlowerOn ();
            }

			CameraMain1 ();
			CameraMain2 ();
			
            if ( gProcImg[OUT_digi_0] & 0x02 )
			{
			    if ( VSEL_PORT & CAM_ON )
                {
			        if ( State == FinishState && !ghostState )
				        State = TrigState;
				}
                gProcImg[OUT_digi_0] &=  ~0x02;				
            }			
			
			if ( 1 ) // LA home etc.
			{
			    Cycle_Complete = 1;
			}
			
			//Coating routine, See State definitions for exact sequence!!!!!!
			switch (State)
            {
			    case TrigState:
				    LAError = 0;
					HeadSpeed = 0;
				    Cycle_Complete = 0;
                    Display ( "Proc:Coating" );
    				strncpy(HeadOnOff.str_value," ON",HeadOnOff.len_str);
					getvalue (&HeadOnOff,2);

                	gTxMsg.ID = 0x361; 		 //make sure purge unit (ghostcup) is retracted
	        		gTxMsg.LEN = 1;
	        		gTxMsg.BUF[0] = 2;
   	        		if (!MCOHW_PushMessage(&gTxMsg))
   	        		{
                        // failed to transmit
       	        		MCOUSER_FatalError(0x8801);
            		}
					if ( !gProcImg[OUT_digi_7] )//skip if moving 
					{
                	   	 StateTime = 0;
            			 State++;
					}
					else if ( StateTime > RTI_One_Sec * 5)
					{
			    	 	 State = ErrorState;		   	  //Actuator was moving too long
					}
			    break;

				case CkHeadRotation:
				    if ( StateTime > RTI_One_Sec * 5 )
			    	{
					    State = HeadErrorState;		   	  //Head didn't rotate
					}
				    if ( HeadSpeed > 5000 )
					{
					    StateTime = 0;
					    State++;
					}
				break;
					
				case PurgeRetract:
				    if ( StateTime > RTI_One_Sec * 2 )
					{
					    if ( !gProcImg[IN_digi_31] )
			    	        State = InitLAMove;		      //already retracted
    					else 
    					    State++;					  //retracting
						StateTime = 0;
					}
			    break;

				case PurgeRetractWait:
				    if ( StateTime > RTI_One_Sec * 20 )
			    	{
					    State = PurgeRetractWaitErrorState;		   	  //Didn't retract
					}
					if (  !(gProcImg[IN_digi_31]) )
					{
						State++;				   	   //Retracted
						StateTime = 0;
					}
			    break;
				
				case InitLAMove:  	//This movement is necessary because the LA will initially go home after reset
					if ( !gProcImg[OUT_digi_7] )//skip if moving
					{
						LAMoveTimer = LAMoveTime;
						if ( LAPos != MaxLADist.value )
						{
						    Display ( "Proc:Head Not Extended" );
							State = HomeState;
						}
						else if ( FirstStrokes.value )
						    State = StartCoatState; 
						else if( SecondStrokes.value )
							State = StartSecondCoatState;
						else if ( CleanOutStrokes.value )
						    State = StartCleanOutState;
						else
						    State = ErrorState;
					}
					
			    break;

				case StartCoatState:
					if ( !gProcImg[OUT_digi_7] )//skip if moving
					{
    				    if ( (FirstStrokes.value) )
    					{
    						FirstCoatSeq ( 1 );
    				    	StateTime = 0;
    				    	State++;
    					}
    					else
    					{
    				    	StateTime = 0;
    				    	State = StartSecondCoatState;
    					}
					}
					if (!LAMovingTimer)
					    State = ErrorState; //Moving too long
				break;

				case FirstCoatState:
				    
					if ( LAError )
					    State = ErrorState;
					if ( HeadSpeed < 5000 )
					    State = HeadErrorState;
					if ( FirstCoatSeq ( 0 ) )
						State++;	
				break;
				
				case StartSecondCoatState:
					if ( !gProcImg[OUT_digi_7] )//skip if moving
					{
    				    if ( (SecondStrokes.value) )
    					{
    						SecondCoatSeq ( 1 );
    				    	StateTime = 0;
    				    	State++;
    					}
    					else
    					{
    				    	StateTime = 0;
    				    	State = StartCleanOutState;
    					}
					}
				break;

				case SecondCoatState:
				    
					if ( LAError )
					    State = ErrorState;
					if ( HeadSpeed < 5000 )
					    State = HeadErrorState;
					if ( SecondCoatSeq ( 0 ) )
				        State++;	
				break;
				
				case StartCleanOutState:
					
					if (RetractTime.value)
					{
					   OldPumpSpeed = PumpSpd.value;
					   PumpSpd.value = 75;
					   getstrval( &PumpSpd );
					   strncpy(PumpOnOff.str_value," ON",PumpOnOff.len_str);
					   getvalue(&PumpOnOff,0);
					   
					}
					
					if ( !gProcImg[OUT_digi_7] )//skip if moving
					{
    					CleanCoatSeq ( 1 );
    			    	StateTime = 0;
                        PumpOnOff.value = 1;						
    			    	State++;
					}
				break;

				case CleanOutState:
				    if ( LAError ){
					    
						State = ErrorState;						
					}	
					if ( CleanCoatSeq( 0 ) ){
				    	
						State++;						
					}
					
				break;
				
				case HomeState:
    				strncpy(HeadOnOff.str_value,"OFF",HeadOnOff.len_str);
					getvalue (&HeadOnOff,1);
					HeadOnOff.value = 1;
					
					if ( !gProcImg[OUT_digi_7] )//skip if moving
					{
					    if (gProcImg[IN_digi_12] != 0){//Home
						   
							State++; 		   //Wasn't at home							
						}
						else{
						    
							State = StopState; //Was already there							
						}	
					}
				break;

			    case StopState:
				    StateTime = 0;
  					if ( !gProcImg[OUT_digi_7] )
					{  	 					   	//here when it stops moving
						MoveLA ( MaxLADist.value, 100, LACurrent );                       
						Update_Menu_Timer = 5 * RTI_One_Sec;
    					StateTime = 0;
					  	State = CoatingComplete; //Was already there
						
					}
					if (!LAMovingTimer){
					    
						State = ErrorState; //Moving too long
					}	
				break;

				case CoatingComplete:
				    if ( StateTime > 1 * RTI_One_Sec )
					{
					    Display ( "Proc:Coating Complete" );
						StateTime = 0;
						State++;
					}
				break;
				
				case StopWaitState:
				    if ( StateTime > 15 * RTI_One_Sec )
					{
					    State = 99;
					}
                    if ( Cycle_Complete )
                    {
                        Cycle_Complete = 0;
                        State++;
                    }
				break;
				
			    case FinishState:
				    State = FinishState;
				break;

				case ErrorState:
				    StateTime = 0;
                    Display ( "Warn:TIMEOUT ERROR" );
					Update_Menu_Timer = 5 * RTI_One_Sec;

    				strncpy(HeadOnOff.str_value,"OFF",HeadOnOff.len_str);
					getvalue (&HeadOnOff,1);
					HeadOnOff.value = 1;

                    PumpOnOff.value = 1;

					State = FinishState;
				break;
				
				case HeadErrorState:
				    StateTime = 0;
                    Display ( "Warn:HEAD ERROR" );
					Update_Menu_Timer = 5 * RTI_One_Sec;

    				strncpy(HeadOnOff.str_value,"OFF",HeadOnOff.len_str);
					getvalue (&HeadOnOff,1);
					HeadOnOff.value = 1;

                    PumpOnOff.value = 1;

					State++;
				break;
				
				case HdErrHomeState:
					if ( !gProcImg[OUT_digi_7] )//skip if moving
					{
					    if (gProcImg[IN_digi_12] != 0)//Home
						    State++; 		   //Wasn't at home
						else
						    State = HdErrStopState; //Was already there
					}
				break;

			    case HdErrStopState:
				    StateTime = 0;
  					if ( !gProcImg[OUT_digi_7] )//wait if moving
					{
                        MoveLA ( MaxLADist.value, 100, LACurrent );
						Update_Menu_Timer = 5 * RTI_One_Sec;
    					StateTime = 0;
     				    if (gProcImg[IN_digi_12] != MaxLADist.value )// LA Max
						    State++; 		   //Wasn't there
						else						    
						    State = FinishState; //Was already there
					}
				break;

				case PurgeRetractWaitErrorState:
				    StateTime = 0;
                    Display ( "Warn:PURGE TIMEOUT" );
					Update_Menu_Timer = 5 * RTI_One_Sec;

    				strncpy(HeadOnOff.str_value,"OFF",HeadOnOff.len_str);
					getvalue (&HeadOnOff,1);
					HeadOnOff.value = 1;

                    PumpOnOff.value = 1;

					State = FinishState;
				break;

			    default:
				    State = FinishState;
            	break;
            }


			throwGhost();		//call sequence to throw ghost band

       		// Operate on CANopen protocol stack
       		i = MCO_ProcessStack();
	   		//End MicroCanOpen Stack   
	   

}


void CameraMain1 ( void )
{
  int i;
	
	//light value is set in settings menu 0 - 100% duty
    //PWMDTY1 = atoi ( LightLevel1 ) * atoi (LightLevel1);

    ++ran_num;      //random number used for camera address

	    sprintf ( disp_add1.str_value, "%04X", cam_add1 );    //for video diplay of camera address
        
        if ( (gProcImg[OUT_digi_10] & 0x08) &&              //command to activate menu
            (gProcImg[OUT_digi_11] == (cam_add1 & 0x00FF)) &&         //lsb - old address
                (gProcImg[OUT_digi_12]<< 8 ==  (cam_add1 & 0xFF00)) &&
                 !(Gen_Flags & Gen_Flags_Menu_Active) )
        {
            gProcImg[OUT_digi_10] = 0x00;
            gProcImg[OUT_digi_11] = 0x00;
            gProcImg[OUT_digi_12] = 0x00;
            gProcImg[OUT_digi_8] = (cam_add1 & 0x00FF);
	        gProcImg[OUT_digi_9] = (cam_add1 & 0xFF00)>>8;
            VSEL_PORT |= CAM_ON;       //turn camera on, portA bit 0 high
			VideoSw_Port |= VideoSw;	 //Select Camera 1
            
            //turn off other cameras
			gTxMsg.ID = 0x421;
            gTxMsg.LEN = 2; 
            gTxMsg.BUF[0] = cam_add1;
            gTxMsg.BUF[1] = cam_add1 >> 8;      
            if (!MCOHW_PushMessage(&gTxMsg))
            {
            // failed to transmit
            MCOUSER_FatalError(0x8801);
            }
            //! Transmit this without using the TPDO
            
            Timer1 = RTI_One_Sec * .10;
            while ( Timer1 );
            gProcImg[IN_digi_0] |= 0x01;
            i = MCO_ProcessStack();
            Timer1 = RTI_One_Sec * .10;
            while ( Timer1 );
            
            MenuTimer = MenuTime * 2; //briefly disable up/down after menu is brought up
            
            StackPointer = 0;
            MenuStackc[StackPointer].Index[0] = 0;
            MenuStackc[StackPointer].Index[1] = 0;
            MenuStackc[StackPointer].Index[2] = 5;
            MenuStackc[StackPointer].Index[3] = 1;
            MenuStackc[StackPointer].CursorPos = 1;
            MenuStackc[StackPointer].FirstLine = 0;
            Gen_Flags |= Gen_Flags_Menu_Active;
            
			TC0_RCVD_Data &= ~0x07;  //Make sure up/down/select not active
			CursorDownFlag = 0;
			CursorUpFlag = 0;
			SelectFlag = 0;
			LoadMenu ( MenuStackc[StackPointer].Index );
            InsertCursor ();
            DisplayTitler ();
        }


    if ( gProcImg[OUT_digi_8] == (cam_add1 & 0x00FF) &&
        gProcImg[OUT_digi_9]<< 8 == (cam_add1 & 0xFF00) )    //compare
	{
    	char tempstr[18];
		gProcImg[OUT_digi_8] = gProcImg[OUT_digi_9] = 0;
		VSEL_PORT |= CAM_ON;       //turn camera on
		VideoSw_Port |= VideoSw;	 //Select Camera 1
		CurrentLight = 1;
		sprintf (tempstr,"Proc:%s",CamTag1.str_value); 
		Display ( tempstr );
	}
	else if ( ( gProcImg[OUT_digi_8] || gProcImg[OUT_digi_9] ) &&
	          ( gProcImg[OUT_digi_8] != (cam_add2 & 0x00FF) &&
                gProcImg[OUT_digi_9]<< 8 != (cam_add2 & 0xFF00) ) )
    {
    	VSEL_PORT &= ~CAM_ON;       //turn camera off, portA bit 0 low
    }

    if ( gProcImg[OUT_digi_10] & 0x01 )   //command to generate random address
    {
        srand(ran_num);               //seed the random number      
        cam_add1 = rand();
        cam_addx1[0] = cam_add1;     
        cam_addx1[1] = cam_add1>>8;
        Save_Camera_Add1();
    }

    //command to transmit address
    if ( gProcImg[OUT_digi_10] & 0x02)   //called by scan_camera in 2-wire
    {
        //make delay proportional to camera address so cameras report in ascending order
		Timer1 = cam_add1/100;
		while(Timer1);
        
        gTxMsg.ID = 0x2a1;
        gTxMsg.LEN = 2; 
        gTxMsg.BUF[0] = cam_add1;
        gTxMsg.BUF[1] = cam_add1 >> 8;      
        if (!MCOHW_PushMessage(&gTxMsg))
        {
            // failed to transmit
            MCOUSER_FatalError(0x8801);
        }
         //! Transmit this without using the TPDO
    }

     if ( (gProcImg[OUT_digi_10] & 0x04) &&              //command to store address 
            (gProcImg[OUT_digi_11] == (cam_add1 & 0x00FF)) &&         //lsb - old address
                ( (gProcImg[OUT_digi_12]<< 8) ==  (cam_add1 & 0xFF00)) ) //msb - old address
    {   //change to new address
        cam_add1 = gProcImg[OUT_digi_13] + (gProcImg[OUT_digi_14]<<8);
        //send new address            
        cam_addx1[0] = cam_add1;     
        cam_addx1[1] = cam_add1>>8;
        Save_Camera_Add1();
   }

}


void CameraMain2 ( void )
{
    int i;
	
	//light value is set in settings menu 0 - 100% duty
    //PWMDTY1 = atoi ( LightLevel2 ) * atoi (LightLevel2);

    ++ran_num;      //random number used for camera address

	    sprintf ( disp_add2.str_value, "%04X", cam_add2 );    //for video diplay of camera address
        
        if ( (gProcImg[OUT_digi_10] & 0x08) &&              //command to activate menu
            (gProcImg[OUT_digi_11] == (cam_add2 & 0x00FF)) &&         //lsb - old address
                (gProcImg[OUT_digi_12]<< 8 ==  (cam_add2 & 0xFF00)) &&
                 !(Gen_Flags & Gen_Flags_Menu_Active) )
        {
            gProcImg[OUT_digi_10] = 0x00;
            gProcImg[OUT_digi_11] = 0x00;
            gProcImg[OUT_digi_12] = 0x00;
            gProcImg[OUT_digi_8] = (cam_add2 & 0x00FF);
	        gProcImg[OUT_digi_9] = (cam_add2 & 0xFF00)>>8;
            VSEL_PORT |= CAM_ON;       //turn camera on, portA bit 0 high
			VideoSw_Port |= VideoSw;	 //Select Camera 1
            
            //turn off other cameras
			gTxMsg.ID = 0x421;
            gTxMsg.LEN = 2; 
            gTxMsg.BUF[0] = cam_add2;
            gTxMsg.BUF[1] = cam_add2 >> 8;      
            if (!MCOHW_PushMessage(&gTxMsg))
            {
            // failed to transmit
            MCOUSER_FatalError(0x8801);
            }
            //! Transmit this without using the TPDO
            
            Timer1 = RTI_One_Sec * .10;
            while ( Timer1 );
            gProcImg[IN_digi_0] |= 0x01;
            i = MCO_ProcessStack();
            Timer1 = RTI_One_Sec * .10;
            while ( Timer1 );
            
            MenuTimer = MenuTime * 2; //briefly disable up/down after menu is brought up
            
            StackPointer = 0;
            MenuStackc[StackPointer].Index[0] = 0;
            MenuStackc[StackPointer].Index[1] = 0;
            MenuStackc[StackPointer].Index[2] = 5;
            MenuStackc[StackPointer].Index[3] = 2;
            MenuStackc[StackPointer].CursorPos = 1;
            MenuStackc[StackPointer].FirstLine = 0;
            Gen_Flags |= Gen_Flags_Menu_Active;
            
			TC0_RCVD_Data &= ~0x07;  //Make sure up/down/select not active
			CursorDownFlag = 0;
			CursorUpFlag = 0;
			SelectFlag = 0;
			LoadMenu ( MenuStackc[StackPointer].Index );
            InsertCursor ();
            DisplayTitler ();
        }

    if ( gProcImg[OUT_digi_8] == (cam_add2 & 0x00FF) &&
        gProcImg[OUT_digi_9]<< 8 == (cam_add2 & 0xFF00) )    //compare
	{
    	char tempstr[18];
		gProcImg[OUT_digi_8] = gProcImg[OUT_digi_9] = 0;
		VSEL_PORT |= CAM_ON;       //turn camera on
		VideoSw_Port &= ~VideoSw;	 //Select Camera 2
		CurrentLight = 2;
		sprintf (tempstr,"Proc:%s",CamTag2.str_value); 
		Display ( tempstr );
	}
	else if ( ( gProcImg[OUT_digi_8] || gProcImg[OUT_digi_9] ) &&
	          ( gProcImg[OUT_digi_8] != (cam_add1 & 0x00FF) &&
                gProcImg[OUT_digi_9]<< 8 != (cam_add1 & 0xFF00) ) )
    {
    	VSEL_PORT &= ~CAM_ON;       //turn off Output
    }

    if ( gProcImg[OUT_digi_10] & 0x01 )   //command to generate random address
    {
        srand(ran_num);               //seed the random number      
        cam_add2 = rand();
        gProcImg[OUT_digi_10] = 0x00;
        cam_addx2[0] = cam_add2;     
        cam_addx2[1] = cam_add2>>8;
        Save_Camera_Add2();
    }

    //command to transmit address
    if ( gProcImg[OUT_digi_10] & 0x02)   //called by scan_camera in 2-wire
    {
        gProcImg[OUT_digi_10] = 0x00;             
    	//VSEL_PORT &= ~CAM_ON;           //camera off, portA bit 0 low

        //make delay proportional to camera address so cameras report in ascending order
		Timer1 = cam_add1/100;
		while(Timer1);
        
        gTxMsg.ID = 0x2a1;
        gTxMsg.LEN = 2; 
        gTxMsg.BUF[0] = cam_add2;
        gTxMsg.BUF[1] = cam_add2 >> 8;      
        if (!MCOHW_PushMessage(&gTxMsg))
        {
            // failed to transmit
            MCOUSER_FatalError(0x8801);
        }
         //! Transmit this without using the TPDO
    }

     if ( (gProcImg[OUT_digi_10] & 0x04) &&              //command to store address 
            (gProcImg[OUT_digi_11] == (cam_add1 & 0x00FF)) &&         //lsb - old address
                ( (gProcImg[OUT_digi_12]<< 8) ==  (cam_add2 & 0xFF00)) ) //msb - old address
    {   //change to new address
        cam_add2 = gProcImg[OUT_digi_13] + (gProcImg[OUT_digi_14]<<8);
        gProcImg[OUT_digi_10] = 0x00;             
        //send new address            
        cam_addx2[0] = cam_add2;     
        cam_addx2[1] = cam_add2>>8;
        Save_Camera_Add2();
   } 
}

void throwGhost(void)
{

    switch( ghostState )
    {
        case 0:		    //not doing ghost band
            break;
			
		case 1:	  		//if moving try to retract
			if (gProcImg[IN_digi_31])
			{
                gTxMsg.ID = 0x361;
	        	gTxMsg.LEN = 1;
	        	gTxMsg.BUF[0] = 2;
   	        	if (!MCOHW_PushMessage(&gTxMsg))
   	        	{
                    // failed to transmit
       	        	MCOUSER_FatalError(0x8801);
            	}
	        	Timer1 = RTI_One_Sec * .25;
	        	while ( Timer1 );
				++ghostState;
			}
			else
			{
			    ghostState=4;
			}
            break;

        case 2:		    //wait until retracted
			Timer2 = 20 * RTI_One_Sec;
			++ghostState;
            break;

		case 3:		    //abort after Timer2 
			if( !Timer2 )
            {
			    ghostState = 19;
            }
			if ( !(gProcImg[IN_digi_31]) )
			{
                gTxMsg.ID = 0x361;
    	        gTxMsg.LEN = 1;
    	        gTxMsg.BUF[0] = 0;
       	        if (!MCOHW_PushMessage(&gTxMsg))
       	        {
                    // failed to transmit
           	        MCOUSER_FatalError(0x8801);
                }
    	        Timer1 = RTI_One_Sec * .25;
    	        while ( Timer1 );
			    ++ghostState;
			}
			break;
        
        case 4:		    //extend cup
            gTxMsg.ID = 0x361;
	        gTxMsg.LEN = 1;
	        gTxMsg.BUF[0] = 1;
   	        if (!MCOHW_PushMessage(&gTxMsg))
   	        {
                // failed to transmit
       	        MCOUSER_FatalError(0x8801);
            }
	        Timer1 = RTI_One_Sec * .25;
	        while ( Timer1 );
            Timer2 = 2 * RTI_One_Sec;
			Display ( "Proc:Extend Purge" );
            ++ghostState;
            break;
			
		case 5:		    //wait until moving, abort after Timer2 
			if( !Timer2 )
            {
			    ghostState = 19;
            }
			if ( (gProcImg[IN_digi_31]) )
			{
			   Timer2 = 20 * RTI_One_Sec;
			    ++ghostState;
			}
			break;
        
		case 6:		    //wait until not moving, abort after Timer2 
			if( !Timer2 )
            {
			    ghostState = 19;
            }
			if ( !(gProcImg[IN_digi_31]) )
			    ++ghostState;
			break;

        case 7:		    //turn on head
			//HeadOnOff.value=2;
			strncpy(HeadOnOff.str_value," ON",HeadOnOff.len_str);
			getvalue (&HeadOnOff,2);
			Timer2 = 2 * RTI_One_Sec;    
            ++ghostState; 
			break;
			
		case 8:		    //get head up to speed
			if( !Timer2 )
            {
			    ++ghostState;
            }
			break;
			
		case 9:		    //turn on pump
			strncpy(PumpOnOff.str_value," ON",PumpOnOff.len_str);
			getvalue(&PumpOnOff,0);
			PumpOnOff.value=2;
			Timer2 = 4 * RTI_One_Sec;
			Display ( "Proc:Purging" );
			++ghostState;
			break;
			
		case 10:		    //wait for pump
			if( !Timer2 )
            {
			    ++ghostState;
            }
			break;
			
		case 11:		    //turn off pump
			PumpOnOff.value=1;
			Timer2 = 15 * RTI_One_Sec;
            ++ghostState;
			break;
		
		case 12:		    //wait for pump to stop
			if( !Timer2 )
            {
			    ++ghostState;
            }
			break;

		case 13:		//turn off head
			strncpy(HeadOnOff.str_value,"OFF",HeadOnOff.len_str);
			getvalue (&HeadOnOff,1);
			HeadOnOff.value=1;
			Timer2 = 5 * RTI_One_Sec;
            ++ghostState;
			break;
    
		case 14:		//wait for head cone to empty
			if( !Timer2 )
            {
			    ++ghostState;
            }
			break;
			
		case 15:		//retract cup
            gTxMsg.ID = 0x361;
	        gTxMsg.LEN = 1;
	        gTxMsg.BUF[0] = 2;
   	        if (!MCOHW_PushMessage(&gTxMsg))
   	        {
                // failed to transmit
       	        MCOUSER_FatalError(0x8801);
            }
	        Timer1 = RTI_One_Sec * .25;
	        while ( Timer1 );
            Timer2 = 2 * RTI_One_Sec;
            Display ( "Proc:Retract Purge" );
			++ghostState;
            break;

		case 16:		    //wait until moving, abort after Timer2 
			if( !Timer2 )
            {
			    ghostState = 19;
            }
			if ( (gProcImg[IN_digi_31]) )
			{
			   Timer2 = 20 * RTI_One_Sec;
			    ++ghostState;
			}
			break;
        
		case 17:		    //wait until not moving, abort after Timer2 
			if( !Timer2 )
            {
			    ghostState = 19;
            }
			if ( !(gProcImg[IN_digi_31]) )
			    ++ghostState;
			break;

		case 18:	    //finished
			Display ( "Proc:Purge Complete" );
			ghostState = 0;			//reset to beginning
			break;

		case 19:	    //Error
			Display ( "Proc:Purge Error" );
			ghostState++;
			break;

		case 20:	    //Error
			ghostState = 20;   	  	//stay in error state until reset
			break;
    }
}


//MUST UPDATE PREV ANGLE RIGHT AFTER
float GetRotationalSpeed(int current_angle, int previous_angle, float dt) {
    int angle_diff = current_angle - previous_angle;
    // Calculate speed as the corrected angle difference over time
    return (fabs(angle_diff) / dt)/RDR_Ratio; // Return magnitude of speed
}


float PID_Loop(struct PID* pid, float actual_value) {
    float error, P, I, D, derivative, output, new_output;

	
    error = pid->desired_value - actual_value;


    // Proportional term
    P = pid->Kp * error;

    // Integral term
    pid->Integral += error * pid->dT;

    I = pid->Kd * pid->Integral;

    // Derivative term
    derivative = (error - pid->previous_error) / pid->dT;
    D = pid->Kd * derivative;

    // PID output
    output = (P + I + D);

    pid->previous_error = error;

	return output;
}