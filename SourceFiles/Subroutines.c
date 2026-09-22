/*******************************************
201-CNT-014 Head Control Unit
24-48 Coater, Rev. D board

*******************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Subroutines.h"
#include "Subroutines1.h"
#include "mc9s12a128.h"
#include "Interrupts.h"
#include "mcohw.h"
#include "mco.h" // added in Rev 4.29 for Reset_Max33011() function
#include "EEProm.h"
#include "string.h"
#include "Packets.h"
#include "MenuSerialize.h"

char save_serial_flag=0;
//Enum strings have a maximum length of 100 chars including the Null
const char enum_NULL_str[]="";
const char enum_off_on_str[]="OFF, ON";
const char enum_12_24_str[]="12,24";
const char enum_stop_retract_str[]="   STOP, EXTEND,RETRACT";
const char enum_base_iso_str[]="BASE, ISO, OFF";
const char enum_lin_act_str[]="CLEANER,LIN ACT";
const char enum_type_str[]="LIQUID,   FBE";
const char enum_neg_pos_str[]="NEG,POS";
const char enum_alpha_str[]=" ,A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,0,1,2,3,4,5,6,7,8,9,.,<,>,;,:,@,(,),-,-"; //last char is the cursor char, do not count for max
const char enum_number_str[]="0,1,2,3,4,5,6,7,8,9,-";  //last char is the cursor char, do not count for max


char Variable_flag;
char String_Var_ptr;
char Multi_Var_ptr;
char UpdateArrayVar;


extern char Gen_Flags;
extern unsigned int TC0_RCVD_Data;

extern CAN_MSG gTxNMT;

CAN_MSG gTxMsg;

extern unsigned int Timer1;
extern unsigned int Timer2;
extern int Update_Menu_Timer;

char CurrentLight = 1;

char UpdateMenu;

char State = FinishState;
char Cycle_Complete;

float LAPos;

extern unsigned int MenuTimer;
extern unsigned long  StateTime;
extern unsigned int BlowerTimer;
extern unsigned int FocusZoomTimer;
extern unsigned int AdvanceTimer;

int CamDegree;
char DispDegree;
extern int CamPosition;
extern char CamHome;

int OldPumpSpeed;


int CamDegXmtd;
int CamDegTimer;			

extern unsigned int CamIndexTimer;

char MoveCmdXmtd;
char HeadSpdOut;
char iHeadSpd;

char lightval;

int HeaterTimer;

int StrokeNum, StartPumpFlag;

unsigned cam_add1;
unsigned cam_add2;

unsigned ran_num;  //used to create unique camera address  

// signed: these are tri-state (0/1/-1) and the -1 compare must hold on both compilers
char CursorDownFlag;
char CursorUpFlag;
char SelectFlag;
char CamAddressXmitd;

float RDR_Ratio;
char ghostState;	//used in switch statement of ghost band sequence	
//extern UNSIGNED8 gTPDONr;

int Rotate;
int Sense_Direction;
unsigned char Sense_Detected;
unsigned char CameraSpeed=1;

char Stop_Detected;

extern char fast_inc;
extern unsigned int IncSpeedUpTimer;

char Store_Flag = 0;


//The following Menu Variables are saved in EEPROM
struct menu_var  LightLevel1 = {
	   10,1,0,10,0,2,"10",enum_NULL_str,NULL,0,0
};

struct menu_var  LightLevel2 = {
	   10,1,0,10,0,2,"10",enum_NULL_str,NULL,0,0
};

struct menu_var PumpSpd= {
	   100,1,0,100,0,3,"100",enum_NULL_str,NULL,0,0
};

struct menu_var HeadSpd= {
	   100,1,0,100,0,3,"100",enum_NULL_str,NULL,0,0
};

struct menu_var CameraSpd= {
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0
};

struct menu_var CameraTrip= {
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0
};

struct menu_var CameraTurnTime= {
	   0.5,0.1,0.0,9.9,1,3,"0.5",enum_NULL_str,NULL,0,0
};

struct menu_var MaxLADist= {
	   12,1,0,24,0,2,"12",enum_NULL_str,NULL,0,0
};

struct menu_var MotorPol= {
	   1,1,1,2,0,3,"NEG",enum_neg_pos_str,NULL,0,0
};

struct menu_var TYPE= {
	   1,1,1,2,0,6,"LIQUID",enum_type_str,NULL,0,0
};

struct menu_var LA_TYPE= {
	   2,1,1,2,0,7,"LIN ACT",enum_lin_act_str,NULL,0,0
};

struct menu_var SecondStrokes= {
	   10,1,0,10,0,2,"10",enum_NULL_str,NULL,0,0
};

struct menu_var FirstStrokes= {
	   5,1,0,10,0,2," 5",enum_NULL_str,NULL,0,0
};

struct menu_var CleanOutStrokes= {
	   5,1,0,10,0,2," 5",enum_NULL_str,NULL,0,0
};

struct menu_var  RetractTime = {
	   0,1,0,10,0,2," 0",enum_NULL_str,NULL,0,0
};

struct menu_var HtrISOOnOff= {
	   1,1,1,2,0,3,"OFF",enum_off_on_str,NULL,0,0
};

struct menu_var HtrISOSetPnt= {
	   120,1,32,199,0,3,"120",enum_NULL_str,NULL,0,0
};

struct menu_var HtrBaseOnOff= {
	   1,1,1,2,0,3,"OFF",enum_off_on_str,NULL,0,0
};

struct menu_var HtrBaseSetPnt= {
	   120,1,32,199,0,3,"120",enum_NULL_str,NULL,0,0
};

struct menu_var HtrHoseOnOff= {
	   2,1,1,2,0,3,"OFF",enum_off_on_str,NULL,0,0
};

struct menu_var HtrHoseSetPnt= {
	   120,1,32,199,0,3,"120",enum_NULL_str,NULL,0,0
};

struct menu_var PGainISO= {
	   10,1,1,20,0,2,"10",enum_NULL_str,NULL,0,0
};

struct menu_var IGainISO= {
	   0.03,0.01,0.01,0.20,2,4,"0.03",enum_NULL_str,NULL,0,0
};

struct menu_var IMaxISO= {
	   2500,100,100,5000,0,4,"2500",enum_NULL_str,NULL,0,0
};

struct menu_var PGainBase= {
	   10,1,1,20,0,2,"10",enum_NULL_str,NULL,0,0
};

struct menu_var IGainBase= {
	   0.03,0.01,0.01,0.20,2,4,"0.03",enum_NULL_str,NULL,0,0
};
struct menu_var IMaxBase= {
	   2500,100,100,5000,0,4,"2500",enum_NULL_str,NULL,0,0
};

struct menu_var MachineSize= {
	   2,1,1,2,0,2,"24",enum_12_24_str,NULL,0,0
}; //"12" OR "24"

struct menu_var  FirstStrokePmpSpd[10] = {
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0
};

struct menu_var  FirstStrokeLASpd[10] = {
	   50,1,20,100,0,3," 50",enum_NULL_str,&FirstStrokePmpSpd[0],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&FirstStrokePmpSpd[1],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&FirstStrokePmpSpd[2],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&FirstStrokePmpSpd[3],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&FirstStrokePmpSpd[4],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&FirstStrokePmpSpd[5],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&FirstStrokePmpSpd[6],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&FirstStrokePmpSpd[7],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&FirstStrokePmpSpd[8],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&FirstStrokePmpSpd[9],0,0
};

struct menu_var  FirstStrokeLen[10] = {
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&FirstStrokeLASpd[0],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&FirstStrokeLASpd[1],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&FirstStrokeLASpd[2],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&FirstStrokeLASpd[3],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&FirstStrokeLASpd[4],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&FirstStrokeLASpd[5],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&FirstStrokeLASpd[6],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&FirstStrokeLASpd[7],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&FirstStrokeLASpd[8],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&FirstStrokeLASpd[9],0,0
};	  

struct menu_var FirstStrokeCtr[10]  = {
	   5,1,1,24,0,2," 5",enum_NULL_str,&FirstStrokeLen[0],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&FirstStrokeLen[1],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&FirstStrokeLen[2],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&FirstStrokeLen[3],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&FirstStrokeLen[4],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&FirstStrokeLen[5],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&FirstStrokeLen[6],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&FirstStrokeLen[7],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&FirstStrokeLen[8],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&FirstStrokeLen[9],0,0
};

struct menu_var  SecondStrokePmpSpd[10] = {
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0,
	   50,1,0,100,0,3," 50",enum_NULL_str,NULL,0,0
};

struct menu_var  SecondStrokeLASpd[10] = {
	   50,1,20,100,0,3," 50",enum_NULL_str,&SecondStrokePmpSpd[0],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&SecondStrokePmpSpd[1],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&SecondStrokePmpSpd[2],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&SecondStrokePmpSpd[3],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&SecondStrokePmpSpd[4],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&SecondStrokePmpSpd[5],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&SecondStrokePmpSpd[6],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&SecondStrokePmpSpd[7],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&SecondStrokePmpSpd[8],0,0,
	   50,1,20,100,0,3," 50",enum_NULL_str,&SecondStrokePmpSpd[9],0,0
};

struct menu_var  SecondStrokeLen[10] = {
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&SecondStrokeLASpd[0],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&SecondStrokeLASpd[1],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&SecondStrokeLASpd[2],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&SecondStrokeLASpd[3],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&SecondStrokeLASpd[4],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&SecondStrokeLASpd[5],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&SecondStrokeLASpd[6],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&SecondStrokeLASpd[7],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&SecondStrokeLASpd[8],0,0,
	   2,0.1,0.5,24,1,4," 2.0",enum_NULL_str,&SecondStrokeLASpd[9],0,0
};

struct menu_var SecondStrokeCtr[10]  = {
	   5,1,1,24,0,2," 5",enum_NULL_str,&SecondStrokeLen[0],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&SecondStrokeLen[1],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&SecondStrokeLen[2],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&SecondStrokeLen[3],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&SecondStrokeLen[4],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&SecondStrokeLen[5],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&SecondStrokeLen[6],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&SecondStrokeLen[7],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&SecondStrokeLen[8],0,0,
	   5,1,1,24,0,2," 5",enum_NULL_str,&SecondStrokeLen[9],0,0
};

//The following Menu Variables are NOT saved in EEPROM
struct menu_var  disp_add1 = {
	   1,0,1,46,0,-4,"4AF2",enum_alpha_str,NULL,0,0
}; //Display only;
struct menu_var  disp_add2 = {
	   1,0,1,46,0,-4,"4AF3",enum_alpha_str,NULL,0,0
}; //Display only;

struct menu_var  ZoomSpeed1 = {
	   100,1,0,100,0,3,"100",enum_NULL_str,NULL,0,0
}; //0-100
struct menu_var  FocusSpeed1 = {
	   100,1,0,100,0,3,"100",enum_NULL_str,NULL,0,0
}; //0-100
struct menu_var  ZoomSpeed2 = {
	   100,1,0,100,0,3,"100",enum_NULL_str,NULL,0,0
}; //0-100
struct menu_var  FocusSpeed2 = {
	   100,1,0,100,0,3,"100",enum_NULL_str,NULL,0,0
}; //0-100
struct menu_var  AdvanceTime = {
	   1,1,0,100,0,3,"  1",enum_NULL_str,NULL,0,0
}; //1-5
struct menu_var  CamTag1 = {
	   1,1,1,46,0,-11,"COATING CAM",enum_alpha_str,NULL,0,0
}; //"COATER CAM "
struct menu_var  CamTag2 = {
	   1,1,1,46,0,-11,"INSPECT CAM",enum_alpha_str,NULL,0,0
}; //"COATER CAM "



struct menu_var  NullVar;

//saved seperately at 0x0b00
char cam_addx1[2];      //unique camera address from ran_num
char cam_addx2[2];      //unique camera address from ran_num

//saved seperately at 0x0b10
struct menu_var SerialNum = {
	   1,0,1,10,0,-6,"------",enum_number_str,NULL,0,0
};

//The following Menu Variables are NOT saved in EEPROM
struct menu_var PumpOnOff= {
	   1,1,1,2,0,3,"OFF",enum_off_on_str,NULL,0,0
};

struct menu_var HeadOnOff= {
	   1,1,1,2,0,3,"OFF",enum_off_on_str,NULL,0,0
};

struct menu_var Rev = {
	   1,0,1,46,0,-4,Revision,enum_alpha_str,NULL,0,0
};
struct menu_var HtrISOTemp = {
	   32,0,0,300,0,3," 32",enum_NULL_str,NULL,0,0
}; //0-100

struct menu_var HtrBaseTemp = {
	   32,0,0,300,0,3," 32",enum_NULL_str,NULL,0,0
}; //0-100



struct MenuStruct const Menuc[MenuSize] = {
                                        0,0,0,0,
                                        "    LIQUID COATER   ",
                                        " SETTINGS           ",
                                        " INSP CAM SETTINGS  ",
										" DIAGNOSTICS        ",
                                        " HEATERS            ",
                                        " CAMERAS            ",
                                        " PURGE MATERIAL     ",
                                        " STATUS             ",
                                        " DEFAULTS           ",
                                        " BLOWER             ",
                                        " ADVANCE FILM       ",
										" EXIT               ",                                       
                                        0,0,0,0,0,0,0,0,0,0,0,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullVar,
                                        &NullFunction,
                                        &NullFunction,
                                        &NullFunction,
                                        &NullFunction,
                                        &NullFunction,
										&startGhost,
                                        &NullFunction,
                                        &NullFunction,
                                        &BlowerOn,
                                        &Advance,
                                        &ExitMenu,
                                        
                                        
                                            0,0,0,1,
                                            "    SETTINGS MENU   ",
											" MACHINE SIZE       ",
											" 1ST STROKES        ",
											" 2ND STROKES        ",
											" 1ST SETUP          ",
                                            " 2ND SETUP          ",
											" CLEANOUT STROKES   ",
                                            " PUMP SPEED         ",
                                            " LIN. ACT. DIST.    ",
                                            " L.A. TYPE          ",
                                            " EXIT               ",
                                            "                    ",
                                            18,18,18,0,0,18,17,18,13,0,0,
                                            &MachineSize,
                                            &FirstStrokes,
                                            &SecondStrokes,
                                            &NullVar,
                                            &NullVar,
                                            &CleanOutStrokes,
                                            &PumpSpd,
                                            &MaxLADist,
                                            &LA_TYPE,
                                            &NullVar,
                                            &NullVar,
                                            &StdVarFunction,
                                            &StdVarFunction,
                                            &StdVarFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &StdVarFunction,
                                            &StdVarFunction,
                                            &StdVarFunction,
                                            &StdVarFunction,
                                            &ExitMenu,
                                            &NullFunction,
                                     		
                            
                                      		    0,0,1,4,
                                      			" LN CTR LEN SPD PMP ",
                                      			" 1   5 24.0 100 100 ",
                                      			" 2   5  4.0 100 100 ",
                                      			" 3   5  4.0 100 100 ",
                                      			" 4   5  4.0 100 100 ",
                                      			" 5   5  4.0 100 100 ",
                                      			" 6   5  4.0 100 100 ",
                                      			" 7   5  4.0 100 100 ",
                                      			" 8   5  4.0 100 100 ",
                                      			" 9   5  4.0 100 100 ",
                                      			" 10  5  4.0 100 100 ",
                                      			" EXIT               ",
                                      			4,4,4,4,4,4,4,4,4,4,0,
                                      			&FirstStrokeCtr[0],
                                      			&FirstStrokeCtr[1],
                                      			&FirstStrokeCtr[2],
                                      			&FirstStrokeCtr[3],
                                      			&FirstStrokeCtr[4],
                                      			&FirstStrokeCtr[5],
                                      			&FirstStrokeCtr[6],
                                      			&FirstStrokeCtr[7],
                                      			&FirstStrokeCtr[8],
                                      			&FirstStrokeCtr[9],
                                      			&NullVar,
                                      			&ArrayVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&ExitMenu,
                            
                                      		    0,0,1,5,
                                      			" LN CTR LEN SPD PMP ",
                                      			" 1  24 24.9 100 100 ",
                                      			" 2   5  4.0 100 100 ",
                                      			" 3   5  4.0 100 100 ",
                                      			" 4   5  4.0 100 100 ",
                                      			" 5   5  4.0 100 100 ",
                                      			" 6   5  4.0 100 100 ",
                                      			" 7   5  4.0 100 100 ",
                                      			" 8   5  4.0 100 100 ",
                                      			" 9   5  4.0 100 100 ",
                                      			" 10  5  4.0 100 100 ",
                                      			" EXIT               ",
                                      			4,4,4,4,4,4,4,4,4,4,0,
                                      			&SecondStrokeCtr[0],
                                      			&SecondStrokeCtr[1],
                                      			&SecondStrokeCtr[2],
                                      			&SecondStrokeCtr[3],
                                      			&SecondStrokeCtr[4],
                                      			&SecondStrokeCtr[5],
                                      			&SecondStrokeCtr[6],
                                      			&SecondStrokeCtr[7],
                                      			&SecondStrokeCtr[8],
                                      			&SecondStrokeCtr[9],
                                      			&NullVar,
                                      			&ArrayVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&ExitMenu,
                            							  
                                            0,0,0,2,
                                            "  INSP CAM SETTINGS ",
                                            " CAMERA SPEED       ",
                                            " CAMERA TRIP PT     ",
                                            " MOTOR POL          ",
                                            " CAM TURN TIME      ",
                                            " EXIT               ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            17,17,17,17,0,0,0,0,0,0,0,
                                            &CameraSpd,
                                            &CameraTrip,
                                            &MotorPol,
                                            &CameraTurnTime,
                                            &NullVar,
                                            &NullVar,//&TYPE,
                                            &NullVar,//&EncoderRes,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &StdVarFunction,
											&StdVarFunction,
											&StdVarFunction,
                                            &StdVarFunction,
                                            &ExitMenu,
                                            &NullFunction,
                                            &NullFunction,//&StdVarFunction,
                                            &NullFunction,//&ExitMenu,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
											
											0,0,0,3,
                                            "   DIAGNOSTIC MENU  ",
                                            " RETRACT LIN. ACT.  ",
                                            " EXTEND LIN. ACT.   ",
                                            " PUMP               ",
                                            " HEAD               ",
                                            " EXIT               ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            0,0,16,16,0,0,0,0,0,0,0,
                                            &NullVar,
                                            &NullVar,
                                            &PumpOnOff,
                                            &HeadOnOff,
                                            &NullVar,
                                            &NullVar,//&TYPE,
                                            &NullVar,//&EncoderRes,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &RetractLA,
											&ExtendLA,
											&StdVarFunction,
                                            &StdVarFunction,
                                            &ExitMenu,
                                            &NullFunction,
                                            &NullFunction,//&StdVarFunction,
                                            &NullFunction,//&ExitMenu,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
 
                                                                                         
                                  			0,0,0,4,
                                  			"      HEATERS       ",
                                  			" BASE HEATER        ",
                                  			" SETPOINT TEMP      ",
                                  			" CURRENT TEMP       ",
                                  			" ISO HEATER         ",
                                  			" SETPOINT TEMP      ",
                                  			" CURRENT TEMP       ",
                                  			" SETUP              ",
                                  			" EXIT               ",
                                  			"                    ",
                                  			"                    ",
                                  			"                    ",
                                  			16,16,16,16,16,16,0,0,0,0,0,
                                  			&HtrBaseOnOff,
                                  			&HtrBaseSetPnt,
                                  			&HtrBaseTemp,//HtrBaseTemp,
                                  			&HtrISOOnOff,
                                  			&HtrISOSetPnt,
                                  			&HtrISOTemp,//HtrISOTemp,
                                  			&NullVar,
                                  			&NullVar,
                                  			&NullVar,
                                  			&NullVar,
                                  			&NullVar,
                                  			&StdVarFunction,
                                  			&StdVarFunction,
                                  			&NullFunction,
                                  			&StdVarFunction,
                                  			&StdVarFunction,
                                  			&NullFunction,
                                  			&NullFunction,
                                  			&ExitMenu,
                                  			&NullFunction,
                                  			&NullFunction,
                                  			&NullFunction,
                                  			
                            
                                      		    0,0,4,7,
                                      			"    HEATER SETUP    ",
                                      			" PGAIN BASE         ",
                                      			" IGAIN BASE         ",
                                      			" ILIMIT BASE        ",
                                      			" PGAIN ISO          ",
                                      			" IGAIN ISO          ",
                                      			" ILIMIT ISO         ",
                                      			" EXIT               ",
                                      			"                    ",
                                      			"                    ",
                                      			"                    ",
                                      			"                    ",
                                      			17,15,15,17,15,15,0,0,0,0,0,
                                      			&PGainBase,
                                      			&IGainBase,
                                      			&IMaxBase,
                                      			&PGainISO,
                                      			&IGainISO,
                                      			&IMaxISO,
                                      			&NullVar,
                                      			&NullVar,
                                      			&NullVar,
                                      			&NullVar,
                                      			&NullVar,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&StdVarFunction,
                                      			&ExitMenu,
                                      			&NullFunction,
                                      			&NullFunction,
                                      			&NullFunction,
                                      			&NullFunction,
                            
                                            0,0,0,5,
                                            "       CAMERAS      ",
                                            " CAMERA 1           ",
                                            " CAMERA 2           ",
                                            " EXIT               ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            15,15,0,0,0,0,0,0,0,0,0,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullFunction,
                                            &NullFunction,
                                            &ExitMenu,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            
                                            
                                                0,0,5,1,
                                                "       CAMERA-1     ",
    											" SETTINGS           ",
                                                " DIAGNOSTICS        ",
												" STATUS             ",
                                                " EXIT               ",
                                                "                    ",
                                                "                    ",
                                                "                    ",
                                                "                    ",
                                                "                    ",
                                                "                    ",
                                                "                    ",
                                                0,0,0,0,0,0,0,0,0,0,0,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullFunction,
                                                &NullFunction,
                                                &NullFunction,
                                                &ExitMenu,
                                                &NullFunction,
                                                &NullFunction,
                                                &NullFunction,
                                                &NullFunction,
                                                &NullFunction,
                                                &NullFunction,
                                                &NullFunction,
                                         
                                
                                                    0,5,1,1,
                                                    "   CAM-1 SETTINGS   ",
        											" LIGHTING           ",
        											" ZOOM SPEED         ",
        											" FOCUS SPEED        ",
        											" FILM ADVANCE       ",
                                                    " TAG 1              ",
                                                    " EXIT               ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    17,16,16,16,8,0,0,0,0,0,0,
                                                    &LightLevel1,
                                                    &ZoomSpeed1,//
                                                    &FocusSpeed1,//
                                                    &AdvanceTime,
                                                    &CamTag1,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullFunction,
                                                    &StdVarFunction,
                                                    &StdVarFunction,
                                                    &StdVarFunction,
                                                    &StdVarFunction,
                                                    &ExitMenu,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                     
                                    
                                                    0,5,1,2,
                                                    " CAM-1 DIAGNOSTICS  ",
                                                    " ADVANCE FILM       ",
                                                    " ZOOM IN            ",
        											" ZOOM OUT           ",
        											" FOCUS FAR          ",
        											" FOCUS NEAR         ",
                                                    " EXIT               ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    12,0,0,0,0,0,0,0,0,0,0,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &Advance,
                                                    &ZoomInFunct1,
                                                    &ZoomOutFunct1,
        											&FocusFarFunct1,
                                                    &FocusNearFunct1,
        											&ExitMenu,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                     
                                                    
                                                    0,5,1,3,
                                                    "  CAM-1 STATUS MENU ",
                                                    " SOFTWARE REV       ",
                                                    " CAMERA ID          ",
                                                    " SERIAL NUM         ",
                                                    " EXIT               ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    16,16,14,0,0,0,0,0,0,0,0,
                                                    &Rev,
                                                    &disp_add1,
                                                    &SerialNum,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &ExitMenu,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    
                                                    
                                                0,0,5,2,
                                                "      CAMERA-2      ",
    											" SETTINGS           ",
                                                " DIAGNOSTICS        ",
												" STATUS             ",
                                                " EXIT               ",
                                                "                    ",
                                                "                    ",
                                                "                    ",
                                                "                    ",
                                                "                    ",
                                                "                    ",
                                                "                    ",
                                                0,0,0,0,0,0,0,0,0,0,0,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullVar,
                                                &NullFunction,
                                                &NullFunction,
                                                &NullFunction,
                                                &ExitMenu,
                                                &NullFunction,
                                                &NullFunction,
                                                &NullFunction,
                                                &NullFunction,
                                                &NullFunction,
                                                &NullFunction,
                                                &NullFunction,
                                         
                                
                                                    0,5,2,1,
                                                    "   CAM-2 SETTINGS   ",
        											" LIGHTING           ",
        											" ZOOM SPEED         ",
        											" FOCUS SPEED        ",
        											" FILM ADVANCE       ",
                                                    " TAG 2              ",
                                                    " EXIT               ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    17,16,16,16,8,0,0,0,0,0,0,
                                                    &LightLevel2,
                                                    &ZoomSpeed2,
                                                    &FocusSpeed2,
                                                    &AdvanceTime,
                                                    &CamTag2,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullFunction,
                                                    &StdVarFunction,
                                                    &StdVarFunction,
                                                    &StdVarFunction,
                                                    &StdVarFunction,
                                                    &ExitMenu,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                             
                                    
                                                    0,5,2,2,
                                                    "  CAM-2 DIAGNOSTICS ",
                                                    " ADVANCE FILM       ",
                                                    " ZOOM IN            ",
        											" ZOOM OUT           ",
        											" FOCUS FAR          ",
        											" FOCUS NEAR         ",
                                                    " EXIT               ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    12,0,0,0,0,0,0,0,0,0,0,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &Advance,
                                                    &ZoomInFunct2,
                                                    &ZoomOutFunct2,
        											&FocusFarFunct2,
                                                    &FocusNearFunct2,
        											&ExitMenu,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                     
                                                    
                                                    0,5,2,3,
                                                    " CAM-2 STATUS MENU  ",
                                                    " SOFTWARE REV       ",
                                                    " CAMERA ID          ",
                                                    " SERIAL NUM         ",
                                                    " EXIT               ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    "                    ",
                                                    16,16,14,0,0,0,0,0,0,0,0,
                                                    &Rev,
                                                    &disp_add2,
                                                    &SerialNum,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullVar,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &ExitMenu,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    &NullFunction,
                                                    
                                                    
                                            0,0,0,7,
                                            "     STATUS MENU    ",
                                            " SOFTWARE REV       ",
                                            " CAMERA 1 ID        ",
                                            " CAMERA 2 ID        ",
                                            " SERIAL NUM         ",
                                            " EXIT               ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            16,16,16,14,0,0,0,0,0,0,0,
                                            &Rev,
                                            &disp_add1,
                                            &disp_add2,
                                            &SerialNum,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &StdVarFunction,
                                            &ExitMenu,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            
                                            
                                            0,0,0,8,
                                            "     DEFAULTS       ",
                                            " EXIT               ",
                                            " RESTORE DEFAULTS   ",
                                            "                    ",
                                            "      WARNING       ",
                                            "YOU WILL LOSE ALL   ",
                                            "OF THE CURRENT      ",
                                            "SETTINGS WHEN       ",
                                            "RESTORING DEFAULTS  ",
                                            "                    ",
                                            "                    ",
                                            "                    ",
                                            0,0,0,0,0,0,0,0,0,0,0,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &NullVar,
                                            &ExitMenu,
                                            &RestoreDefaults,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction,
                                            &NullFunction
 
};


struct MenuStack MenuStackc[MenuStackSize] = {0,0,0,0,1,0};

char StackPointer;

char Menu[12][21];

char VariableFlag;

char StrokeFlag;
char StrokePos;




/***************************************************************************/ 

// external declaration for the process image array
extern UNSIGNED8 gProcImg[];

/**************************************************************************/



void InitPorts ( void )
{


	DDRA = DDRA_Init;
    DDRB = DDRB_Init;
	DDRE = DDRE_Init;
	DDRJ = DDRJ_Init;
    DDRM = DDRM_Init;
    DDRP = DDRP_Init;
    DDRS = DDRS_Init;
    DDRT = DDRT_Init;
	
    ATD0DIEN = ATD0DIEN_Init;
    ATD0CTL2 = ATD0CTL2_Init;
    ATD0CTL3 = ATD0CTL3_Init;
    ATD0CTL5 = ATD0CTL5_Init;
    
    
    PUCR = PUCR_Init;
	PERM = PERM_Init;
    PPSM = PPSM_Init;
    
    PORTA = PORTA_Init;
    PORTB = PORTB_Init;
	PORTE = PORTE_Init;	
	PTJ = PTJ_Init;
    PTM = PTM_Init;
	PTP = PTP_Init;
	PTS = PTS_Init;
    PTT = PTT_Init;
	
	PPSJ = PPSJ_Init;  		 		//Port J pulldowns
	
	PERS = PERS_Init;				//Port S pulldowns
	WOMS = WOMS_Init;				//Port S bit 3 (TXD1) Wired-OR
}

void InitInterrupts ( void )
{	
	CRGINT = CRGINT_Init;	 		//enable RTI
	RTICTL = RTICTL_Init;
    
    PPSP = PPSP_Init;		  		//rising edge
    PIEP = PIEP_Init;		  		//enable KW interrupts 
    
	TSCR1 = TSCR1_Init;				//enable input capture
    TIOS = TIOS_Init;				//enable output compares
	TIE = TIE_Init;
	TSCR2 = TSCR2_Init;
	TCTL3 = TCTL3_Init;
	TCTL4 = TCTL4_Init;

 	TIE |= TIE_C4I;
}

void InitPLL ( void )
{	
	REFDV = REFDV_Init;
	SYNR = SYNR_Init;
	while ( !(CRGFLG & 0x08) );
	CLKSEL |= 0x80;
}

void PWMInit ( void )
{
	PWMPOL = PWMPOL_Init;
    PWMCLK = PWMCLK_Init;
    PWMPRCLK = PWMPRCLK_Init;
    
    PWMPER0 = PWMPER0_Init;
    PWMPER1 = PWMPER1_Init;
    PWMPER2 = PWMPER2_Init;
    PWMPER3 = PWMPER3_Init;
    PWMPER4 = PWMPER4_Init;
    PWMPER5 = PWMPER5_Init;
    PWMPER6 = PWMPER6_Init;
    PWMPER7 = PWMPER7_Init;
    
    PWMDTY0 = PWMDTY0_Init;
    PWMDTY1 = PWMDTY1_Init;
    PWMDTY2 = PWMDTY2_Init;
    PWMDTY3 = PWMDTY3_Init;
    PWMDTY4 = PWMDTY4_Init;
    PWMDTY5 = PWMDTY5_Init;
    PWMDTY6 = PWMDTY6_Init;
    PWMDTY7 = PWMDTY7_Init;

    
    PWMSCLA = PWMSCLA_Init;
    PWMSCLB = PWMSCLB_Init;
    PWME = PWME_Init;
    
}

void AtoDInit ( void )
{
    ATD0DIEN = ATD0DIEN_Init;	  	 //Analog to Digital Registers
    ATD0CTL2 = ATD0CTL2_Init;
    ATD0CTL3 = ATD0CTL3_Init;
    ATD0CTL4 = ATD0CTL4_Init;
    ATD0CTL5 = ATD0CTL5_Init;
}    

extern int _textmode;


//c000
//#pragma abs_address:0xc000

int NullFunction ( void )
{
    return 0;
}

int BlowerOn ( void )
{
    BlowerTimer = BlowerTime;
	return 0;
}

int CamIndex ( void )
{
    CamIndexTimer = CamIndexTime;
	return 0;
}

//When a menu item is first select, this sets the Variable
int StdVarFunction ( void )
{
    int i;
	struct menu_var *var;
	float old_value; 
	
	i = FindMenu(); 
	var = Menuc[i].VarPntr[MenuStackc[StackPointer].CursorPos+MenuStackc[StackPointer].FirstLine-1];
	    //var is the 1st variable pointed to in the menu
	
	
	if ( !Variable_flag ) //Here the first time select is pushed
    {
		Variable_flag = 1;
		Multi_Var_ptr = 0;
		String_Var_ptr = 0;
		
        if (var->next_var)
		    Multi_Var_ptr = 1;
			
		if ( var->len_str < 0 )	//puts a cursor char in "String type" variables <<<<<<<<<<<<<<<<<<
		{
    		String_Var_ptr = 1;
			StdVarFunc_String(var);
		}
		else			   //non "String type" variables
		{
            LoadMenu ( MenuStackc[StackPointer].Index );
		}
    }
    else //Here if select had already been pushed, see if more variables or string variable, or end if needed
    {

	    var = getVariable();// Get the variable pointed to by Multi_Var_ptr
		 
		//see if "String type" before going to next multi var
		if (var->len_str < 0) //"String type"
		{
		    Next_Char_In_String_Var( var );  
		}
		else
		{
		    next_variable( var );
		}
    }


    DisplayTitler ();
    return 0;
}

//This gets the variable pointed to by the Menu/Cursor/Multi_Var_ptr
struct menu_var *getVariable( void ) 
{
     int k;
	 int j=1;
	 struct menu_var *var; 
	 
	 k = FindMenu();
	 var = Menuc[k].VarPntr[MenuStackc[StackPointer].CursorPos+MenuStackc[StackPointer].FirstLine-1];
	 while(j<Multi_Var_ptr)
	 {
	     var = var->next_var;
		 j++;
	 }
     return var;
}

//This displays the "String Type" variable with a '-' in the current "String_Var_ptr" location
void StdVarFunc_String ( struct menu_var *var ) 
{
	float old_value; 
	
    getvalue(var,String_Var_ptr-1);
    old_value = var->value;
    var->value = var->max+1; 	 //the last char in the enum list is used as a cursor
    getstrval(var);				 //put it in the string temporarily   
    LoadMenu ( MenuStackc[StackPointer].Index );
    
    update_menu_var_by_value(var, old_value); // Update the value member of the variable in the python GUI
    // var->value = old_value;
    // getstrval(var);
    // update_value_gui(var); //Update the value member of the variable in the python GUI; honestly not sure if this is needed here

}

// This points to the next char in "String Type" variables and puts in Menu, or goes to next variable
void Next_Char_In_String_Var ( struct menu_var *var ) 
{
    int i;
	
    if (++String_Var_ptr > abs(var->len_str)) //End of "String Type" variable here
    {
	    next_variable (var);
    }
    else
    {
	    StdVarFunc_String(var);
    }

}

char FindMenu ( void )
{
     int k;
     
     //Find out which menu we are on
     for ( k=0;k<MenuSize;k++ )
     {
        if ( Menuc[k].Index[0] == MenuStackc[StackPointer].Index[0] && Menuc[k].Index[1] == MenuStackc[StackPointer].Index[1] && 
             Menuc[k].Index[2] == MenuStackc[StackPointer].Index[2] && Menuc[k].Index[3] == MenuStackc[StackPointer].Index[3] )
        {
		    break;
		}
	 }
	 return k;
}

// This puts the next variable in the Menu and updates the pointers "Multi_Var_ptr", "String_Var_ptr", "Variable_flag"
void next_variable ( struct menu_var *var )
{
    if (var->next_var) // See if more linked variables
    {
        Multi_Var_ptr++;
        var = getVariable(); // Get the variable pointed to by Multi_Var_ptr
        if (var->len_str < 0)  // See if "String Type"
        {
            String_Var_ptr = 1;
            StdVarFunc_String(var); //Display "String Type"   ">-xxx"
        }
    	else //"Standard" numeric or enum variable here
    	{
            if (!Multi_Var_ptr)
                Variable_flag = 0; //This removes Variable Cursor when the menu is loaded
			
        	LoadMenu ( MenuStackc[StackPointer].Index );
        	if (!Multi_Var_ptr)
                InsertCursor ();
        }
	
    }
	else // No more linked variables, cursor back to beginning of line
	{
	    Variable_flag = 0;
		Multi_Var_ptr = 0;
		String_Var_ptr = 0;
		UpdateArrayVar = 0;
		LoadMenu ( MenuStackc[StackPointer].Index );
		InsertCursor ();
	}
    
}

//Creates a string from the value member or the enum member
//assigns the string to ->str_value, left pads with space so length = ->len_str
//also returns a pointer to the string
char * getstrval (struct menu_var* var)
{
    char tempstr[100];
    char *token;
    int i;

    if (strlen(var->str_enum) && var->len_str >= 0) //"Enum Type" variable here
    {
        strncpy(tempstr,var->str_enum,sizeof(tempstr));
        token = strtok(tempstr, ",");
        for (i=0;i<var->value-1;i++)
        {
            token = strtok(NULL, ",");
        }
        strncpy(var->str_value,token,var->len_str); //The enum string without leading spaces should be shorter than len(var->str_value)
		sprintf(tempstr,"%*s%s",10," ",var->str_value); //Use tempstr to pad with leading spaces
		strncpy(var->str_value,tempstr+strlen(tempstr)-abs(var->len_str),var->len_str); //put back in var->str_value
    }
    else if ( strlen(var->str_enum) && var->len_str < 0 ) //"String Type" variables here, get char (using value) from enum and put in string location indicated by String_Var_ptr 
	{
	    
		
		strncpy(tempstr,var->str_enum,sizeof(tempstr)); 
									   
        for (i=0;i<var->value;i++)   
        {							   
            if (i==0)
			   token = strtok(tempstr, ",");
			else
			   token = strtok(NULL, ","); 
        }
        var->str_value[String_Var_ptr-1]=*token;
		//sprintf(var->str_value,"%*s%s",10," ",token);
	}
	else //"Numeric Type" varaible
    {
        tempstr[0]=NULL;
		if (var->dec_pos)
		    sprintf(tempstr,"%*s%#.*f",10," ",var->dec_pos,var->value); //force decimal if float, us tempstr so it won't overflow var->str_value
        else
		    sprintf(tempstr,"%*s%.*f",10," ",var->dec_pos,var->value); //don't force decimal on int, us tempstr so it won't overflow var->str_value
			strncpy(var->str_value,tempstr+strlen(tempstr)-abs(var->len_str),var->len_str); //Put back in var->str_value
    }
    
	return var->str_value;
}

//Gets the value member corresponding to the str_value member
//-OR- gets the enum number corresponding to the str_value member
//-OR- gets the enum number of the char pointed to by index for "String type" variables
float getvalue (struct menu_var* var, char index)
{
    char tempstr[120];
    char *token;
    int i;

    if (strlen(var->str_enum) && var->len_str >= 0) //regular enum list
    {
        strncpy(tempstr,var->str_enum,sizeof(tempstr));

        for (i=1;i<=var->max;i++)
        {
            if (i==1)
                token = strtok(tempstr, ",");
            else
                token = strtok(NULL, ",");

			if ( !strcmp(var->str_value,token) )
			{
			    var->value = i;
				break;
			}
        }
	}
	else if (strlen(var->str_enum) && var->len_str < 0) //"String type" enum list
	{
        strncpy(tempstr,var->str_enum,sizeof(tempstr));

        for (i=1;i<=var->max;i++)
        {
            if (i==1)
                token = strtok(tempstr, ",");
            else
                token = strtok(NULL, ",");

			if ( *(var->str_value+index) == *token )
			{
			    var->value = i;
				break;
			}
        }

	}
	else if (strlen(var->str_enum) == 0) //"Numeric type" variable, get value of string
    {
        var->value=atof(var->str_value);
    }
	return var->value;
}


int Advance(void)       //function to advance camera film
{   //advance timer is set 1 - 5 * 0.5 seconds (0.5 to 2.5 seconds)
    AdvanceTimer =  AdvanceTime.value * RTI_One_Sec;
    PORTA |= 0x40;          //turns off in interrupt.c
    return 0;
}

int ZoomInFunct1 ( void )
{
    PWMDTY5 = ZoomSpeed1.value;
    PORTA &= ~0x04;
    FocusZoomTimer = FocusZoomTime;
    return 0;
}

int ZoomOutFunct1 ( void )
{
    PWMDTY5 = 100 - ZoomSpeed1.value;
    PORTA |= 0x04;
    FocusZoomTimer = FocusZoomTime;
    return 0;
}

int FocusFarFunct1 ( void )
{
    PWMDTY3 =  FocusSpeed1.value;
    PORTA &= ~0x08;
    FocusZoomTimer = FocusZoomTime;
    return 0;
}

int FocusNearFunct1 ( void )
{
    PWMDTY3 = 100 - FocusSpeed1.value;
    PORTA |= 0x08;
    FocusZoomTimer = FocusZoomTime;
    return 0;
}

int ZoomInFunct2 ( void )
{
    PWMDTY7 = ZoomSpeed2.value;
    PORTA &= ~0x01;
    FocusZoomTimer = FocusZoomTime;
    return 0;
}

int ZoomOutFunct2 ( void )
{
    PWMDTY7 = 100 - ZoomSpeed2.value;
    PORTA |= 0x01;
    FocusZoomTimer = FocusZoomTime;
    return 0;
}

int FocusNearFunct2 ( void )
{
    PWMDTY4 = 100 - FocusSpeed2.value;
    PORTA |= 0x02;
    FocusZoomTimer = FocusZoomTime;
    return 0;
}

int FocusFarFunct2 ( void )
{
    PWMDTY4 = FocusSpeed2.value;
    PORTA &= ~0x02;
    FocusZoomTimer = FocusZoomTime;
    return 0;
}



int ExtendLA ( void )
{
    StateTime = 0;
	MoveLA ( MaxLADist.value, 100, LACurrent );
	return 0;
}

int RetractLA ( void )
{
    StateTime = 0;
	MoveLA ( 0, 100, LACurrent );
    return 0;
}

//Increments a variable using the inc member
//Wraps around to min value if max value is exceeded
//assigns the equivalent string to the str_value member
//also returns a pointer to the string
char *incvar ( struct menu_var *var )
{
	getvalue(var,String_Var_ptr-1);  // String_Var_ptr points to the char with in var>str_value that is being modified
					 			   // the enum value is retrieved and put in var->value so "incvar" and "decvar" can use it
    IncSpeedUpTimer = MenuTimer * 1.9;
    
    if ( fast_inc )
        var->value = var->value + var->inc*10;
	else
        var->value = var->value + var->inc;
		
    if(var->value>var->max+var->inc/10){
        var->value=var->min;
    }
    return getstrval(var);
}

//Decrements a variable using the inc member
//Wraps around to max value if min value is exceeded
//assigns the equivalent string to the str_value member
//also returns a pointer to the string
char *decvar ( struct menu_var *var )
{
 	getvalue(var,String_Var_ptr-1);  // String_Var_ptr points to the char with in var>str_value that is being modified
					 			   // the enum value is retrieved and put in var->value so "incvar" and "decvar" can use it

    IncSpeedUpTimer = MenuTimer * 1.9;
    
    if ( fast_inc )
        var->value = var->value - var->inc*10;
	else
        var->value = var->value - var->inc;
		
    if(var->value<var->min-var->inc/10){
        var->value=var->max;
    }
    return getstrval(var);
}

int startGhost(void)
{
	if ( !ghostState && State == FinishState )
	    ghostState = 1;		//this will start ghost band sequence if not already triggered or in error or coating
    return 0;
}

int ExitMenu ( void )
{
    DeSelect ();
    return 0;
}

int RestoreDefaults ( void )
{
 	LightLevel1.str_value[0] = 0xFF;    
	gProcImg[OUT_digi_0] &= ~0x01;
	MCO_ProcessStack_Menu();
	Timer1 = RTI_One_Sec * .10;
    while ( Timer1 );
	Save_Variables();    
	ResetProc ();
	return 0;

}

//#pragma end_abs_address

void CursorUp( void )
{
    if ( Variable_flag )
    {
         IncVariable ();
		 UpdateArrayVariables (getVariable(),UpdateArrayVar);
    }
    else
    {
       MenuStackc[StackPointer].CursorPos--;
       LoadMenu ( MenuStackc[StackPointer].Index );
       InsertCursor ();
       DisplayTitler ();
    }
}

void CursorDown( void )
{
    if ( Variable_flag )
    {
         DecVariable ();
		 UpdateArrayVariables (getVariable(),UpdateArrayVar);
    }
    else
    {
       MenuStackc[StackPointer].CursorPos++;
       LoadMenu ( MenuStackc[StackPointer].Index );
       InsertCursor ();
       DisplayTitler ();
    }
}

void IncVariable ( void )
{
     incvar(getVariable());
     LoadMenu ( MenuStackc[StackPointer].Index );
     DisplayTitler ();
     update_value_gui(getVariable()); // sends new updated value to python GUI (on x200) across CAN
} 

void DecVariable ( void )
{
     decvar(getVariable());
     LoadMenu ( MenuStackc[StackPointer].Index );
     DisplayTitler ();
     update_value_gui(getVariable()); // sends new updated value to python GUI (on x200) across CAN
}
 

void CkCntrLength ( struct menu_var *var )
{
	float len,cntr,max;

    cntr = (var+10)->value;

    len = var->value;
	
	max = MaxLADist.value;
	
	// check for max center
	if ( cntr > ( max - 1 ) )
	{
	    cntr  =  max - 1;
		UpdateMenu = 1;
	}
	// check for over extend
	if ( ( len/2 + cntr ) >  max )
	{
	    len = ( max - cntr ) * 2;
		UpdateMenu = 1;
	}
	// check for over retract
	if ( ( cntr - len/2 ) < 0 )
	{
    	    len = cntr * 2;
			UpdateMenu = 1;
	}	
	if ( len == 0 )
	{
	    len = 1;
		UpdateMenu = 1;
	}
	
	(var+10)->value = cntr;
	
	var->value = len;
	
	getstrval(var);

}

void Select ( void )
{
     int k;
     
     //Save current menu pointers
     MenuStackc[StackPointer+1].Index[0]=MenuStackc[StackPointer].Index[1];
     MenuStackc[StackPointer+1].Index[1]=MenuStackc[StackPointer].Index[2];
     MenuStackc[StackPointer+1].Index[2]=MenuStackc[StackPointer].Index[3];
     MenuStackc[StackPointer+1].Index[3] = MenuStackc[StackPointer].CursorPos + MenuStackc[StackPointer].FirstLine;
     
     //look for selected menu 
     for ( k=0;k<MenuSize;k++ )
     {
        if ( Menuc[k].Index[0] == MenuStackc[StackPointer+1].Index[0] && Menuc[k].Index[1] == MenuStackc[StackPointer+1].Index[1] && 
             Menuc[k].Index[2] == MenuStackc[StackPointer+1].Index[2] && Menuc[k].Index[3] == MenuStackc[StackPointer+1].Index[3] )
        {
             //If found increment stackpointer and initialize cursor
             StackPointer++;
             //Cursor to menu entry 1
             MenuStackc[StackPointer].CursorPos = 1;
             MenuStackc[StackPointer].FirstLine = 0;
             LoadMenu ( MenuStackc[StackPointer].Index );
             InsertCursor ();
             DisplayTitler ();
             break;
        }
     } 

     if ( k >= MenuSize )
     {
         //Find out which menu we are on
         for ( k=0;k<MenuSize;k++ )
         {
            if ( Menuc[k].Index[0] == MenuStackc[StackPointer].Index[0] && Menuc[k].Index[1] == MenuStackc[StackPointer].Index[1] && 
                 Menuc[k].Index[2] == MenuStackc[StackPointer].Index[2] && Menuc[k].Index[3] == MenuStackc[StackPointer].Index[3] )
            {
                 //Execute function for that entry 
                 Menuc[k].FunctPtr[(MenuStackc[StackPointer].CursorPos + MenuStackc[StackPointer].FirstLine)-1]();
                 //LoadMenu ( MenuStackc[StackPointer].Index );
                 //DisplayMenu ();
                 break;
            }
        }
   }
}

void DeSelect ( void )
{
     int i,j;
     if (StackPointer )
     {
        StackPointer--;
        LoadMenu ( MenuStackc[StackPointer].Index );
        InsertCursor ();
        DisplayTitler ();
     }
     else
     {
        MenuStackc[StackPointer].Index[0] = 0;
        MenuStackc[StackPointer].Index[1] = 0;
        MenuStackc[StackPointer].Index[2] = 0;
        MenuStackc[StackPointer].Index[3] = 0;
        Gen_Flags &= ~Gen_Flags_Menu_Active;
		gProcImg[IN_digi_0] &= ~0x01;
        ClearTitler ();
		for (j=1;j<=NR_OF_TPDOS;j++)
		{
		    ARMCOP = 0x55;
			ARMCOP = 0xAA;
			i = MCO_ProcessStack_Menu();
		}
        //Save_Variables ();
        Store_Flag = 1;

     }
	 Variable_flag = 0;
	 String_Var_ptr = 0;
	 Multi_Var_ptr = 0;
	 UpdateArrayVar = 0;
}

void LoadMenu ( char Index[] )
{
 	int i,j,k,l;

    //find menu
	k = FindMenu();
    /*for ( k=0;k<MenuSize;k++ )
    {
        if ( Menuc[k].Index[0] == Index[0] && Menuc[k].Index[1] == Index[1] && 
             Menuc[k].Index[2] == Index[2] && Menuc[k].Index[3] == Index[3] )
             break;
    }*/ 
	
	
    //Get Menu Title
    for ( j=0;j<=20;j++ )
	{
        Menu[0][j] = Menuc[k].Entry[0][j];
	}
       
    //See if this entry is a valid menu entry, if not start cursor back at top
    if ( MenuStackc[StackPointer].CursorPos > 0 && Menuc[k].Entry[MenuStackc[StackPointer].CursorPos + MenuStackc[StackPointer].FirstLine][1] == ' ' )
    {
         MenuStackc[StackPointer].CursorPos = 1;
         MenuStackc[StackPointer].FirstLine = 0;
    }
        
    //Allow a maximum of 12 entries, if past 12, start cursor back at top
    if ( MenuStackc[StackPointer].CursorPos + MenuStackc[StackPointer].FirstLine >= 12 )
    {
         MenuStackc[StackPointer].CursorPos = 1;
         MenuStackc[StackPointer].FirstLine = 0;
    }
        
    //If cursor is too far down, check to see if there is another entry, if so, show it and set cursor to bottom entry.
    if ( MenuStackc[StackPointer].CursorPos == 9 )
    {
         if ( Menuc[k].Entry[9+MenuStackc[StackPointer].FirstLine][1] != ' ' )
         {
            MenuStackc[StackPointer].FirstLine++;
            MenuStackc[StackPointer].CursorPos = 8;
         }
         else
         {
             MenuStackc[StackPointer].CursorPos = 1;
             MenuStackc[StackPointer].FirstLine = 0;
         }
    }

    //If cursor is too far up, and the first entry is not the first line then move firstline up and set cursor to top entry
    if ( MenuStackc[StackPointer].CursorPos == 0 )
    {
         if ( MenuStackc[StackPointer].FirstLine )
         {
            MenuStackc[StackPointer].FirstLine--;
            MenuStackc[StackPointer].CursorPos = 1;
         }
         else
         {
             //Find last entry
             for ( i=11;i>0;i-- )
             {
                 if ( Menuc[k].Entry[i][1] != ' ' && Menuc[k].Entry[i][0] == ' ' )
                      break;
             }
             //If more than 8 entries, cursor will be at the bottom (8) and first line will be adjusted
             if ( i > 8 )
             {
                 MenuStackc[StackPointer].CursorPos = 8;
                 MenuStackc[StackPointer].FirstLine = i - 8;
             }
             //Otherwise the cursor will be on that entry with the firstline at 0
             else
             {
                 MenuStackc[StackPointer].CursorPos = i;
                 MenuStackc[StackPointer].FirstLine = 0;
             }
         }
    }

    //Load appropriate menu entries into Menu[i][j]
    for ( i=1;i<=8;i++ )
	{
	    for ( j=0;j<=20;j++ )
		{
            Menu[i][j] = Menuc[k].Entry[i+MenuStackc[StackPointer].FirstLine][j];
		}
	}
	
	//Load any variables
    for ( i=1;i<=8;i++ )
	{
        if ( Menuc[k].Pos[i+MenuStackc[StackPointer].FirstLine-1] )
        {
			 struct menu_var *var = Menuc[k].VarPntr[i+MenuStackc[StackPointer].FirstLine-1];
			 char next_flag = 1;
			 int var_cntr = 1;
			 
             j=0;    
			 do 
			 {	  
                 int l;
					 
				 
				 for (l=0;j<=20;j++,l++ )//up to 11 char variable "STR_VALUE_LEN"
                 {
    				 
                    if ( !Multi_Var_ptr && Variable_flag && i == MenuStackc[StackPointer].CursorPos && l==0 ) 
                        Menu[i][Menuc[k].Pos[i+MenuStackc[StackPointer].FirstLine-1]-1] = '>';

                    if ( Multi_Var_ptr == var_cntr && i == MenuStackc[StackPointer].CursorPos && l==0 )
					{
						 Menu[i][Menuc[k].Pos[i+MenuStackc[StackPointer].FirstLine-1]+j-1] = '>';
					}
                    
    				 if ( var->len_str >= 0 )
    				 {    
    					 getstrval ( var );
    				 }
    				 if ( *(var->str_value+l) ) //if current char of string isn't a NULL
                     {
                          Menu[i][Menuc[k].Pos[i+MenuStackc[StackPointer].FirstLine-1]+j] = *(var->str_value+l);
                     }
                     else
                     {
                         break;
                     }
                 }
				 if (var->next_var)
				 {
					 var = var->next_var;
					 Menu[i][Menuc[k].Pos[i+MenuStackc[StackPointer].FirstLine-1]+j] = ' ';
					 j++;
				  	 var_cntr++;
				 }
				 else
				 {
				     next_flag = 0;
				 }
			 }while(next_flag);
        }            
	}    
}

void InsertCursor ( void )
{
	//Display Cursor Pointer
    Menu[MenuStackc[StackPointer].CursorPos][0] = '>';
}

void ClearTitler ( void )
{
    int i;

	gTxMsg.ID = WIM_ID;
	gTxMsg.LEN = 8;
	gTxMsg.BUF[0] = 0;
	gTxMsg.BUF[1] = 0x02;
	gTxMsg.BUF[2] = 'C';
	gTxMsg.BUF[3] = 'l';
	gTxMsg.BUF[4] = 'r';
	gTxMsg.BUF[5] = ':';
	gTxMsg.BUF[6] = 0x03;
	gTxMsg.BUF[7] = 0;       //bogus place-holder character, RF will not transmit null

   	if (!MCOHW_PushMessage(&gTxMsg))
   	{
        // failed to transmit
       	MCOUSER_FatalError(0x8801);
    }
	//Timer1 = RTI_One_Sec * .05;
	Timer1 = RTI_One_Sec * .25;     //changed to match actuator
	while ( Timer1 );

}
	
void DisplayTitler ( void )
{
	int i,j,k,l;
	
    PositionDisplay ();

	gTxMsg.ID = WIM_ID;
	gTxMsg.LEN = 8;
	gTxMsg.BUF[0] = 0;
	gTxMsg.BUF[1] = 0x02;
	gTxMsg.BUF[2] = 'M';
	gTxMsg.BUF[3] = 'e';
	gTxMsg.BUF[4] = 'n';
	gTxMsg.BUF[5] = 'u';
	gTxMsg.BUF[6] = ':';
	gTxMsg.BUF[7] = 0;
	
   	if (!MCOHW_PushMessage(&gTxMsg))
   	{
        // failed to transmit
       	MCOUSER_FatalError(0x8801);
    }
	Timer1 = RTI_One_Sec * .05;
	while ( Timer1 );
	
	for ( i=0;i<=8;i++ )        //9 lines
	{
	    gTxMsg.ID = WIM_ID;
		gTxMsg.LEN = 7;
		gTxMsg.BUF[0] = 0;
		gTxMsg.BUF[6] = 0;
	    for ( j=0;j<=15;j+=5 )
		{
    	    ARMCOP = 0x55;
    		ARMCOP = 0xAA;
		 	for ( k=0;k<=4;k++ )
			{
		     	gTxMsg.BUF[k+1] = Menu[i][j+k];
			}
   	    	if (!MCOHW_PushMessage(&gTxMsg))
   			{
                // failed to transmit
       			MCOUSER_FatalError(0x8801);
    		}
			Timer1 = RTI_One_Sec * .006;
			while ( Timer1 );
			l = MCO_ProcessStack_Menu();
		}
	}

	gTxMsg.ID = WIM_ID;
	gTxMsg.LEN = 3;
	gTxMsg.BUF[0] = 0;
	gTxMsg.BUF[1] = 0x03;
	gTxMsg.BUF[2] = 0;

   	if (!MCOHW_PushMessage(&gTxMsg))
   	{
        // failed to transmit
       	MCOUSER_FatalError(0x8801);
    }
	Timer1 = RTI_One_Sec * .05;
	while ( Timer1 );
}

void Display ( char buff[] )
{
	int i,j,k,l;

	gTxMsg.ID = WIM_ID;
	gTxMsg.LEN = 8;
	gTxMsg.BUF[0] = NODE_ID;      //not sent
	gTxMsg.BUF[1] = 0x02;       //starting w/ Rev 2.0, <STX> indicates beginning of command

    for (j=0;j<=5;j++)			 
    {
        gTxMsg.BUF[j+2]=buff[j];
    }
    
    if (!MCOHW_PushMessage(&gTxMsg))
    {
    // failed to transmit
        MCOUSER_FatalError(0x8801);
    }
    Timer1 = RTI_One_Sec * .05;
    while ( Timer1 );
    
	gTxMsg.BUF[0] = NODE_ID;

	for (i=6;i<strlen(buff);i+=7)   //0,8,16
	{
	    for (j=0;j<=6;j++)			 //0-6,8-14
		{
	        gTxMsg.BUF[j+1]=buff[i+j];
		}
		
    	if (!MCOHW_PushMessage(&gTxMsg))
       	{
            // failed to transmit
           	MCOUSER_FatalError(0x8801);
        }
    	Timer1 = RTI_One_Sec * .006;
    	while ( Timer1 );
		MCO_ProcessStack_Menu();
    	
	}
	gTxMsg.ID = WIM_ID;
	gTxMsg.LEN = 3;
	gTxMsg.BUF[0] = NODE_ID;
	gTxMsg.BUF[1] = 0x03;
	gTxMsg.BUF[2] = 0;

   	if (!MCOHW_PushMessage(&gTxMsg))
   	{
        // failed to transmit
       	MCOUSER_FatalError(0x8801);
    }
	Timer1 = RTI_One_Sec * 0.05;
	while ( Timer1 );

}

void PositionDisplay ( void )
{
	gTxMsg.ID = WIM_ID;
	gTxMsg.LEN = 8;
	gTxMsg.BUF[0] = 0;
	gTxMsg.BUF[1] = 0x02;
	gTxMsg.BUF[2] = 'H';
	gTxMsg.BUF[3] = 'o';
	gTxMsg.BUF[4] = 'm';
	gTxMsg.BUF[5] = 'e';
	gTxMsg.BUF[6] = ':';
	gTxMsg.BUF[7] = 0x03;
	
   	if (!MCOHW_PushMessage(&gTxMsg))
   	{
        // failed to transmit
       	MCOUSER_FatalError(0x8801);
    }
	Timer1 = RTI_One_Sec * .05;
	while ( Timer1 );
}

void Load_Variables ( void )
{
 	int EE_offset=0;
	char tempstr[130];
    char *cptr,*token;
	struct menu_var *var;

	if ( *(char *)EE_begin == 0xff )
	{
	    Save_Variables();
		return;
	}
		
    if (strlen((char *)EE_begin)>128)
	{
	    tempstr[0] = 0xff;
		EEWrite ( 1, tempstr,(int *)(EE_begin));//Put a 0xff in the beginning of EEPROM to force defaults
		ResetProc ();
		return;
	}
	
	strncpy(tempstr,(char *)EE_begin,sizeof(tempstr));
    token = strtok(tempstr, ",");

	for(cptr=(char *)&LightLevel1.str_value;  cptr<=(char *)&SecondStrokeCtr[9].str_value;  cptr=cptr+((char *)&LightLevel2.str_value - (char *)&LightLevel1.str_value))
	{
		if (strlen(token)>STR_VALUE_LEN)
    	{
	        tempstr[0] = 0xff;
			EEWrite ( 1, tempstr,(int *)(EE_begin));//Put a 0xff in the beginning of EEPROM to force defaults
    		ResetProc ();
			return;
    	}
		
		strncpy(cptr,token,STR_VALUE_LEN);
		token = strtok(NULL, ",");
		if ( token == NULL )
		{
		    EE_offset = EE_offset + 128;
			if (!*(char *)(EE_begin + EE_offset))
			    break; //Last variable loaded
            if (strlen((char *)(EE_begin + EE_offset))>128)
        	{
        	    tempstr[0] = 0xff;
        		EEWrite ( 1, tempstr,(int *)(EE_begin));//Put a 0xff in the beginning of EEPROM to force defaults
        		ResetProc ();
				return;
        	}
			strncpy(tempstr,(char *)(EE_begin + EE_offset),sizeof(tempstr));
			token = strtok(tempstr, ",");
		}
	}
		
	for(var=&LightLevel1; var<=&SecondStrokeCtr[9]; var=var+1)	
	{
	    getvalue(var,0);
	}	
}

void Save_Variables ( void )
{
	int offset=0,EE_offset=0;
	char *cptr;
    char tempstr[130];
	char NULL_char=0;
	char *NULL_ptr = &NULL_char;

	for(cptr=(char *)&LightLevel1.str_value;  cptr<=(char *)&SecondStrokeCtr[9].str_value;  cptr=cptr+((char *)&LightLevel2.str_value - (char *)&LightLevel1.str_value))
	{
	    if ( (offset + strlen(cptr) + 1) < 128 )
		    sprintf(tempstr+offset,"%s,",cptr); //puts a comma
		else
		    *(tempstr+offset-1)=NULL; //puts a null in place of the last comma
		offset = offset + strlen(cptr) + 1;
		
		if (offset>127) //current variable will go over the buffer limit
		{
			EEWrite ( 128, tempstr, (int *)(EE_begin + EE_offset));
			EE_offset = EE_offset + 128;
			sprintf(tempstr,"%s,",cptr); //starts back over by putting the current variable at beginning of tempstr
			offset = strlen(cptr) + 1;
		}
		
	}
	*(tempstr+offset-1)=NULL; //end the last string with a null
	EEWrite ( 128, tempstr, (int *)(EE_begin + EE_offset));
	EEWrite ( 1,NULL_ptr,(int *)(EE_begin + EE_offset + 128));//Put a Null in the first location of the next 128 byte block


	if(save_serial_flag==1){
		 
		Save_Serial_Num();
		save_serial_flag=0; 
	}
	

}                                                                                        


//retreive camera address from EEProm
void Load_Camera_Add ( void )
{
    char *VarEEPROMPntr2;
    VarEEPROMPntr2 = (char *)0x0b00;
    
    cam_addx1[0] = *VarEEPROMPntr2;
    cam_addx1[1] = *(VarEEPROMPntr2 + 1);   
    cam_add1 = cam_addx1[0] + (cam_addx1[1]<<8);               

    cam_addx2[0] = *(VarEEPROMPntr2 + 2);
    cam_addx2[1] = *(VarEEPROMPntr2 + 3);   
    cam_add2 = cam_addx2[0] + (cam_addx2[1]<<8);               
}

void UpdateArrayVariables ( struct menu_var *var, char num )
{
    char tempstr[12];
    int i=0;
    
	if (num)
	{
        var = getVariable();
        strncpy (tempstr,&var->str_value[0],sizeof(tempstr));
        while (i++ < num-1)
        {
            var++;
        	// strncpy (&var->str_value[0],tempstr,var->len_str);
        	// getvalue(var,0);
            // update_value_gui(var); //Update the value member of the variable in the python GUI
            update_menu_var_by_str(var, tempstr); // Update the value member of the variable in the python GUI
        }
		LoadMenu ( MenuStackc[StackPointer].Index );
		DisplayTitler ();
	}
}

//When a menu item is first select, this sets the UpdateArrayVar to cause 
//a variable array to be update
int ArrayVarFunction ( void )
{
    UpdateArrayVar = 10;
	return StdVarFunction();
}

int ResetProc (void){
	
	COPCTL = 0x01;
	while(1);
	return 0;
}


void InitCANOpen ( void )
{
    Reset_Max33011();
  	// Reset/Initialize CANopen communication
  	MCOUSER_ResetCommunication();
  	
}