#ifndef Subroutines_H
#define Subroutines_H

#define Revision "4.34"


//#pragma paged_function InitPorts InitInterrupts InitPLL InitSCI InitSPI
//#pragma paged_function PWMInit AtoDInit InitCANopen
//#pragma paged_function IncVariable DecVariable CkCntrLength Select DeSelect LoadMenu
//#pragma paged_function InsertCursor ClearTitler Display DisplayTitler PositionDisplay
//#pragma paged_function Load_Variables Save_Variables Load_Camera_Add
#pragma nonpaged_function NullFunction BlowerOn CamIndex StdVarFunction Advance
//getVariable
//#pragma paged_function StdVarFunc_String Next_Char_In_String_Var FindMenu
//#pragma paged_function next_variable getstrval getvalue Advance 
#pragma nonpaged_function ZoomInFunct1 ZoomOutFunct1 FocusFarFunct1 FocusNearFunct1
#pragma nonpaged_function ZoomInFunct2 ZoomOutFunct2 FocusFarFunct2 FocusNearFunct2
#pragma nonpaged_function ExtendLA RetractLA startGhost ExitMenu 
//incvar decvar  
#pragma nonpaged_function RestoreDefaults 
//CursorUp CursorDown UpdateArrayVariables
#pragma nonpaged_function ArrayVarFunction 
//ResetProc


extern struct menu_var InternalExternalCameraSetting;
extern struct menu_var Cam1Enable;
extern struct menu_var Cam2Enable;
#define STR_VALUE_LEN 12

struct menu_var{
float value;            //actual value of the variable, or pointer to enum list if an enum type variable
float inc;              //increment/decrement value, may be int or float, must be 1 for enum type variable
float min;              //minimum value of variable, may be int or float, must point to first enum in list for enum type variable
float max;              //maximum value of variable, may be int or float, must point to last enum in list for enum type variable
char dec_pos;            //decimal position, zero if variable is an int or an enum
signed char len_str;			//length of str_value, pads left with spaces
char str_value[STR_VALUE_LEN];     //string equivalent of value, or current enum pointed to by value
const char *str_enum;         //pointer to comma separated enum list, must be NULL for non-enum variable types
struct menu_var *next_var; //pointer to then next variable if more than one per line
};


void InitPorts ( void );
void InitInterrupts ( void );
void InitPLL ( void );
void InitSCI ( void );
void InitSPI ( void );

#define _SCI
void PWMInit ( void );
void AtoDInit ( void );
void InitCANOpen ( void );
int NullFunction ( void );
int BlowerOn ( void );
int CamIndex ( void );
int StdVarFunction ( void );
struct menu_var *getVariable( void ); 
void StdVarFunc_String ( struct menu_var *var );
void Next_Char_In_String_Var ( struct menu_var *var ); 
char FindMenu ( void );
void next_variable ( struct menu_var *var );
char * getstrval (struct menu_var* var);
float getvalue (struct menu_var* var, char index);
int Advance(void);
int ZoomInFunct1 ( void );
int ZoomOutFunct1 ( void );
int FocusNearFunct1 ( void );
int FocusFarFunct1 ( void );
int ZoomInFunct2 ( void );
int ZoomOutFunct2 ( void );
int FocusNearFunct2 ( void );
int FocusFarFunct2 ( void );
int ExtendLA ( void );
int RetractLA ( void );
char *incvar ( struct menu_var *var );
char *decvar ( struct menu_var *var );
int startGhost(void);
int ExitMenu ( void );
void CursorUp( void );
void CursorDown( void );
void IncVariable ( void );
void DecVariable ( void );
void CkCntrLength ( struct menu_var *var );
void Select ( void );
void DeSelect ( void );
void LoadMenu ( char Index[] );
void InsertCursor ( void );
void ClearTitler ( void );
void Display ( char buff[] );
void DisplayTitler ( void );
void PositionDisplay ( void );
void Load_Variables ( void );
void Save_Variables ( void );
int RestoreDefaults ( void );
void Load_Camera_Add ( void );
void UpdateArrayVariables ( struct menu_var *var, char num );
int ArrayVarFunction ( void );
int ResetProc(void);

//PLL OFF
//#define Tbus .543
//#define BusClk 1.8432

//PLL ON 22.118	 3.6864MHz Crystal
//PLL loop components, high stability = 3300pF, .033uf, 2K 
//#define REFDV_Init 0
//#define SYNR_Init 5
//#define Tbus .0452
//#define BusClk 22.118
//#define OscClk 3.6864

//PLL ON 24.576	 3.6864MHz Crystal
//PLL loop components, high stability = .01uf, .1uf, 2K 
//#define REFDV_Init 2
//#define SYNR_Init 19
//#define Tbus .0407
//#define BusClk 24.576
//#define OscClk 3.6864

//PLL ON 24.000	 4.0MHz Crystal
//PLL loop components, high stability = .01uf, .1uf, 2K 
//#define REFDV_Init 2
//#define SYNR_Init 17
//#define Tbus .0417
//#define BusClk 24.000
//#define OscClk 4.0

//PLL ON 24.000	 8.0MHz Crystal
//PLL loop components, high stability = .01uf, .1uf, 2K 
#define REFDV_Init 5
#define SYNR_Init 17
#define Tbus .0417
#define BusClk 24.000
#define OscClk 8.0



#define ATD0DIEN_Init 0x00
#define ATD0CTL2_Init 0x80
#define ATD0CTL3_Init 0x23
#define ATD0CTL4_Init 0x45
#define ATD0CTL5_Init 0x80


#define PUCR_Init 0x91
#define PPSJ_Init 0x00
#define PPSP_Init 0x00
#define PIEP_Init 0x00

#define PERM_Init 0x00
#define PPSM_Init 0x00

//#define PERS_Init 0x00
#define PERS_Init 0xff

//#define WOMS_Init 0x08
#define WOMS_Init 0x00


#define DDRA_Init 0b11111111
#define DDRB_Init 0b11111111
#define DDRE_Init 0b11111100
#define DDRJ_Init 0b11000000
#define DDRM_Init 0b00111100
#define DDRP_Init 0b10111111
#define DDRS_Init 0b00001111
#define DDRT_Init 0b00000100


#define PORTA_Init 0b00000000
#define PORTB_Init 0b00000010
#define PORTE_Init 0b00000000
#define PTJ_Init 0b00000000
#define PTM_Init 0b00000000
#define PTP_Init 0b00000000
#define PTS_Init 0b00000000
#define PTT_Init 0b00000000


#define CRGINT_Init 0x80
#define RTICTL_Init 0x40

#define MODRR_Init 0x10



//SPI Control Register 1
#define SPI0CR1_Init 0xd0
//SPI Control Register 2
#define SPI0CR2_Init 0x00
//SPI Baud Rate Register  (24MHz / 2 = 12MHz)
#define SPI0BR_Init 0x00


#define SPISR_SPTEF 0x20
#define SPICR1_SPTIE 0x20
#define SPISR_SPIF 0X80

#define UpdateMainTime 250

#define TieInCamSet   0x04
#define TieInCamReset 0x08

#define PWME_Init 0b10111111
#define PWMPOL_Init 0xff
#define PWMCLK_Init 0b11111010   //Rev 4.16 increased inspect camera pwm (select unscaled clocks)
#define PWMPRCLK_Init 0x44		 //Rev 4.16 this was changed from divide by 64 to divide by 16, 1/4, see below

#define PWMPER0_Init 100
#define PWMPER1_Init 100
#define PWMPER2_Init 100
#define PWMPER3_Init 100
#define PWMPER4_Init 100
#define PWMPER5_Init 100
#define PWMPER6_Init 100
#define PWMPER7_Init 100

#define PWMDTY0_Init 0
#define PWMDTY1_Init 0
#define PWMDTY2_Init 0
#define PWMDTY3_Init 0
#define PWMDTY4_Init 0
#define PWMDTY5_Init 0
#define PWMDTY6_Init 0
#define PWMDTY7_Init 0

#define PWMSCLA_Init 0x30		 //Rev 4.16 this was changed from 12 to 48, times 4, see above
#define PWMSCLB_Init 0x30		 //Rev 4.16 this was changed from 12 to 48, times 4, see above





typedef struct MenuStruct 
{
char Index[4];
char Entry[12][21];
char Pos[11];
struct menu_var *VarPntr[11];
int (*FunctPtr[11])( void );
};

extern const struct MenuStruct Menuc[];

typedef struct MenuStack 
{
char Index[4];
char CursorPos;
char FirstLine;
};

#define MenuSize 19
#define MenuStackSize 5

#define	VM0_Write	0x00
#define	VM1_Write	0x01
#define	HOS_Write	0x02
#define	VOS_Write	0x03
#define	DMM_Write	0x04
#define	DMAH_Write	0x05
#define	DMAL_Write	0x06
#define	DMDI_Write	0x07
#define	CMM_Write	0x08
#define	CMAH_Write	0x09
#define	CMAL_Write	0x0A
#define	CMDI_Write	0x0B
#define	OSDM_Write	0x0C
#define	RB0_Write	0x10
#define	RB1_Write	0x11
#define	RB2_Write	0x12
#define	RB3_Write	0x13
#define	RB4_Write	0x14
#define	RB5_Write	0x15
#define	RB6_Write	0x16
#define	RB7_Write	0x17
#define	RB8_Write	0x18
#define	RB9_Write	0x19
#define	RB10_Write	0x1A
#define	RB11_Write	0x1B
#define	RB12_Write	0x1C
#define	RB13_Write	0x1D
#define	RB14_Write	0x1E
#define	RB15_Write	0x1F
#define	OSDBL_Write	0x6C
		
#define	VM0_Read	0x80
#define	VM1_Read	0x81
#define	HOS_Read	0x82
#define	VOS_Read	0x83
#define	DMM_Read	0x84
#define	DMAH_Read	0x85
#define	DMAL_Read	0x86
#define	DMDI_Read	0x87
#define	CMM_Read	0x88
#define	CMAH_Read	0x89
#define	CMAL_Read	0x8A
#define	CMDI_Read	0x8B
#define	OSDM_Read	0x8C
#define	RB0_Read	0x90
#define	RB1_Read	0x91
#define	RB2_Read	0x92
#define	RB3_Read	0x93
#define	RB4_Read	0x94
#define	RB5_Read	0x95
#define	RB6_Read	0x96
#define	RB7_Read	0x97
#define	RB8_Read	0x98
#define	RB9_Read	0x99
#define	RB10_Read	0x9A
#define	RB11_Read	0x9B
#define	RB12_Read	0x9C
#define	RB13_Read	0x9D
#define	RB14_Read	0x9E
#define	RB15_Read	0x9F
#define	OSDBL_Read	0xEC
#define	STAT_Read	0xA0
#define	DMDO_Read	0xB0
#define	CMDO_Read	0xC0

#define HBridgeAtoD 0

// HBridgeSetPoint is 4.14V        848 = 4.14V / (5V / 1024) 
//#define HBridgeSetPoint 848

#define TrigState			   	   1
#define CkHeadRotation			   2
#define PurgeRetract			   3
#define PurgeRetractWait		   4
#define InitLAMove				   5
//#define InitLAMove1				   6
#define StartCoatState	   	   	   6
#define FirstCoatState			   7
#define StartSecondCoatState	   8
#define SecondCoatState			   9
#define StartCleanOutState		   10
#define CleanOutState		   	   11
#define HomeState				   12
//#define HomeState1				   13
#define StopState				   13
//#define StopState1				   14
#define CoatingComplete			   14
#define StopWaitState			   15
#define FinishState				   16   
#define ErrorState				   99
#define HeadErrorState			   100
#define HdErrHomeState			   101
//#define HdErrHomeState1			   102
#define HdErrStopState			   102
//#define HdErrStopState1			   104
#define PurgeRetractWaitErrorState 110

#define VideoSw_Port PORTA
#define VideoSw 0x20

#define HeadBlower_Port PTT
#define HeadBlower 0x04

#define Head_Sensor_Port PTT
#define Head_Sensor 0x10

#define CW 0
#define CCW 1

#define WIM_ID 0x310
#define TxPump 0x26c
#define TxLA 0x26a

#define RotateTimeOut 90

#define Low_Batt_Level 21

#define CamIndexTime (2 * RTI_One_Sec)
#define BlowerTime (4 * RTI_One_Sec)

#define Cam_Index_Port PORTA 
#define Cam_Index 0x40

#define LACurrent 100

#define CamDegTime RTI_One_Sec * .25

#define HeaterTime 3

#define VSEL_PORT PORTA
#define CAM_ON 0x10

#define NODE_ID 0x6e

#endif