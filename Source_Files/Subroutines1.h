#ifndef Subroutines1_H
#define Subroutines1_H


typedef enum { false = 0, true = !false } bool;

#define RDR12EncoderRes 512.0 //256*2
#define RDR12SprocketGear 16.0 // Amount of teeth on sprocket attached to encoder
#define RDR12MainGear 74.0 // Amount of teeth on main gear attached to sprocket

#define RDR24EncoderRes 512.0
#define RDR24SprocketGear 24.0
#define RDR24MainGear   120.0


#define RDR24 ((RDR24MainGear/RDR24SprocketGear)*RDR24EncoderRes)/360.0
#define RDR12 ((RDR12MainGear/RDR12SprocketGear)*RDR12EncoderRes)/360.0

#define STOPMARGIN 6 //Degrees that RDR stops preemptively before hitting barstop.
#define STOPWIDTH 8 //Degrees from 180 that home position is initialized at.

//Old RDR values
//#define RDR24 7.111
//#define RDR12 6.578

struct PID{
float Kp;       //Kp     
float Ki;       //Ki
float Kd;       //Kd
float desired_value;    //Desired value the PID should reach
float previous_error;   //Previous error of PID in last loop
float Integral;         //Integral accumulated
float dT;               //Difference in time
};

//#pragma paged_function Save_Camera_Add1 Save_Camera_Add2 Load_Serial_Num Save_Serial_Num
#pragma nonpaged_function MoveLA Save_Serial_Num Save_Camera_Add1 Save_Camera_Add2
#pragma nonpaged_function ATDGetLevel SecondCoatSeq FirstCoatSeq throwGhost
//#pragma nonpaged_function CameraMain1 CameraMain2
//#pragma paged_function CameraMain2 throwGhost CleanCoatSeq


void Save_Camera_Add1 ( void );
void Save_Camera_Add2 ( void );
void Load_Serial_Num ( void );
void Save_Serial_Num ( void );
void MoveLA (float Pos, int Spd, int Current );
int ATDGetLevel ( char ATD_Num );
int SecondCoatSeq ( int Start );
int FirstCoatSeq ( int Start );
void doevents ( void );
int update_active_cam_address ( void );
void CameraMain1 ( void );
void CameraMain2 ( void );
void throwGhost(void);
int CleanCoatSeq ( int Start );

/* HD/SD trig request handling, split out of doevents() so the handshake can be
 * unit tested. trig_query_sent / trig_query_timer stay externed per-TU, the way
 * this revision declares its other globals. */
#define HDSD_SD 1
#define HDSD_HD 2

typedef enum {
	TRIG_IDLE = 0,		/* nothing to do this pass        */
	TRIG_WAITING,		/* query armed, still waiting     */
	TRIG_SENT,			/* query transmitted              */
	TRIG_STARTED,		/* coating sequence started       */
	TRIG_TIMEDOUT,		/* window closed, no answer       */
	TRIG_NOCAM,			/* no active camera to query      */
	TRIG_NOTIDLE		/* busy: the request was ignored  */
} TrigResult;

char HDSDMode ( void );
TrigResult HDTrigQueryStart ( void );
TrigResult HDTrigQueryService ( void );
TrigResult TrigRequest ( void );
//MUST UPDATE PREV ANGLE RIGHT AFTER
float GetRotationalSpeed(int current_angle, int previous_angle, float dt);
float PID_Loop(struct PID* pid, float actual_value);

#endif