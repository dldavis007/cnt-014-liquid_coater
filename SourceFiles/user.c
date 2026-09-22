/**************************************************************************
MODULE:    USER
CONTAINS:  MicroCANopen Object Dictionary and Process Image implementation
COPYRIGHT: Embedded Systems Academy, Inc. 2002-2005.
           All rights reserved. www.microcanopen.com
           This software was written in accordance to the guidelines at
           www.esacademy.com/software/softwarestyleguide.pdf
DISCLAIM:  Read and understand our disclaimer before using this code!
           www.esacademy.com/disclaim.htm
LICENSE:   THIS IS THE EDUCATIONAL VERSION OF MICROCANOPEN
           See file license_educational.txt or
           www.microcanopen.com/license_educational.txt
           A commercial MicroCANopen license is available at
           www.CANopenStore.com
VERSION:   2.10, ESA 12-JAN-05
           $LastChangedDate: 2005-01-12 13:53:59 -0700 (Wed, 12 Jan 2005) $
           $LastChangedRevision: 48 $
***************************************************************************/ 

#include <string.h>

#include "Controller.h"
#include "mco.h"
#include "mcohw.h"
#include "mc9s12a128.h"
#include "subroutines.h"

// ensure the number of tpdos and rpdos is correct
#if (NR_OF_RPDOS != 8)
  #if (NR_OF_TPDOS != 8)
#error This example is for 8 TPDOs and 8 RPDOs only
  #endif
#endif

// global variables

// This structure holds all node specific configuration
UNSIGNED8 gProcImg[PROCIMG_SIZE];
extern UNSIGNED8 setFilters[];
char clearProc = 1;
// Table with SDO Responses for read requests to OD
// Each Row has 8 Bytes:
// Command Specifier for SDO Response (1 byte)
//   bits 2+3 contain: '4' � {number of data bytes}
// Object Dictionary Index (2 bytes, low first)
// Object Dictionary Subindex (1 byte)
// Data (4 bytes, lowest bytes first)
UNSIGNED8 MEM_CONST SDOResponseTable[] = {

  // [1000h,00]: Device Type
  SDOREPLY(0x1000, 0x00, 4, OD_DEVICE_TYPE),

#ifdef OD_SERIAL
  // [1018h,00]: Identity Object, Number of Entries = 4
  SDOREPLY(0x1018, 0x00, 1, 0x00000004L),
#else
  // [1018h,00]: Identity Object, Number of Entries = 3
  SDOREPLY(0x1018, 0x00, 1, 0x00000003L),
#endif

  // [1018h,01]: Identity Object, Vendor ID
  SDOREPLY(0x1018, 0x01, 4, OD_VENDOR_ID),

  // [1018h,02]: Identity Object, Product Code
  SDOREPLY(0x1018, 0x02, 4, OD_PRODUCT_CODE),

  // [1018h,03]: Identity Object, Revision
  SDOREPLY(0x1018, 0x03, 4, OD_REVISION),

#ifdef OD_SERIAL
  // [1018h,04]: Identity Object, Serial
  SDOREPLY(0x1018, 0x04, 4, OD_SERIAL),
#endif

  // [2018h,00]: MicroCANopen Identity Object, Number of Entries = 3
  SDOREPLY(0x2018, 0x00, 1, 0x00000003L),

  // [2018h,01]: MicroCANopen Identity Object, Vendor ID = 01455341, ESA Inc.
  SDOREPLY(0x2018, 0x01, 4, 0x01455341L),

  // [2018h,02]: MicroCANopen Identity Object, Product Code = "MCOP"
  SDOREPLY4(0x2018, 0x02, 4, 'P', 'O', 'C', 'M'),

  // [2018h,03]: MicroCANopen Identity Object, Revision = 1.20
  SDOREPLY(0x2018, 0x03, 4, 0x00010020L),

  // [2100h,00]: MicroCANopen Identity Object, Number of Entries = 3
  SDOREPLY(0x2100, 0x00, 1, 0x00000004L),

  // [2100h,01]: MicroCANopen Identity Object, Vendor ID = 01455341, ESA Inc.
  SDOREPLY4(0x2100, 0x01, 4, 'L', 'I', 'Q', 'U'),

  // [2100h,02]: MicroCANopen Identity Object, Product Code = "MCOP"
  SDOREPLY4(0x2100, 0x02, 4, 'I', 'D', ' ', 'C'),

  // [2100h,03]: MicroCANopen Identity Object, Revision = 1.20
  SDOREPLY4(0x2100, 0x03, 4, 'O', 'A', 'T', 'E'),

  // [2100h,04]: MicroCANopen Identity Object, Revision = 1.20
  SDOREPLY4(0x2100, 0x04, 4, 'R', ' ', ' ', 0x32),

#ifdef PDO_IN_OD
  // NOTE: These entries must be added manually. The parameters must match
  // the parameters used to call the functions MCO_InitRPDO and MCO_InitTPDO.

  // These entries are necessary to be fully CANopen compliant.
  // Suppported in commercial version of MicroCANopen available from
  // www.CANopenStore.com

  // Warning: This version is not fully CANopen compliant - PDO_IN_OD must not be defined
  #error Warning: This version of MicroCANopen has a limited Object Dictionary! Un-define PDO_IN_OD to confirm!
#endif // PDO_IN_OD

  // End-of-table marker
  SDOREPLY(0xFFFF, 0xFF, 0xFF, 0xFFFFFFFFL)
};

#ifdef PROCIMG_IN_OD
  // Table with Object Dictionary entries to process data.

  // These entries are necessary to be fully CANopen compliant.
  // Suppported in commercial version of MicroCANopen available from
  // www.CANopenStore.com

  // Warning: This version is not fully CANopen compliant - PROCIMG_IN_OD must not be defined
  #error Warning: This version of MicroCANopen has a limited Object Dictionary! Un-define PROCIMG_IN_OD to confirm!
#endif // PROCIMG_IN_OD


/**************************************************************************
DOES:    This function is called if a fatal error occurred. 
         Error codes of mcohwxxx.c are in the range of 0x8000 to 0x87FF.
         Error codes of mco.c are in the range of 0x8800 to 0x8FFF. 
         All other error codes may be used by the application.
RETURNS: nothing
**************************************************************************/
void MCOUSER_FatalError
  (
  UNSIGNED16 ErrCode  // the error code
  )
{
  //display blinking pattern on led
  //david, update for Rev 4.29, replaced error_state with InitCANOpen()
  InitCANOpen();
}

/**************************************************************************
DOES:    Call-back function for reset application.
         Starts the watchdog and waits until watchdog causes a reset.
RETURNS: nothing
**************************************************************************/
void MCOUSER_ResetApplication
  (
  void
  )
{
    COPCTL = 0x01;				//enable COP 
	while (1);					//wait for reset

  //MCOUSER_ResetCommunication();
}

/**************************************************************************
DOES:    This function both resets and initializes both the CAN interface
         and the CANopen protocol stack. It is called from within the
         CANopen protocol stack, if a NMT master message was received that
         demanded "Reset Communication".
         This function should call MCO_Init and MCO_InitTPDO/MCO_InitRPDO.
RETURNS: nothing
**************************************************************************/
void MCOUSER_ResetCommunication
  (
  void
  )
{
UNSIGNED8 i;

 INTR_OFF();

  // Initialize Process Variables
  if(clearProc == 1){ // Only clear gProcImg on startup.
  for (i = 0; i < PROCIMG_SIZE; i++)
  {
    gProcImg[i] = 0;
  }
  }
  clearProc = 0;

  for(i = 0; i < 8; i++){
      setFilters[i] = 0x80;
  }
  //david update rev 4.29, add Intr_On()
  INTR_ON();

  // 125kbit, Node 1, No heartbeat
  MCO_Init(125,NODE_ID,0); 
  
  
  //Initiate Menu/Trigger
  // RPDO1, ID ($NODEID+0x200), 1 bytes
  MCO_InitRPDO(1,0,8,OUT_digi_0); //Changed to 8 bytes and user 

  // RPDO2, ID ($NODEID+0x300), 2 bytes, Button box data 
  MCO_InitRPDO(2,0x180,3,OUT_digi_1); 

  // RPDO3, ID ($NODEID+0x400), 3 bytes, Temperature data
  MCO_InitRPDO(3,0x1fe,6,OUT_ana_0); 

  // RPDO4, ID ($NODEID+0x500), 1 bytes, Actuator Moving
  MCO_InitRPDO(4,0x1ea,1,OUT_digi_7); 

  //camera address is received on out_digi_7 and out_digi_7
  // RPDO5, default ID (), 2 bytes
  MCO_InitRPDO(5,0x421,2,OUT_digi_8); 

  // command from controller is received on out_digi_5, out...6,7 have data with command    
  // RPDO6, default ID (), 5 bytes
  MCO_InitRPDO(6,0x521,5,OUT_digi_10);       //command RPDO 

  // Purge Unit moving    
  // RPDO7, default ID (), 1 bytes
  MCO_InitRPDO(7,0x1e1,1,IN_digi_31);       //command RPDO 

  // TPDO1, default ID ($NODEID+0x180), 0ms event, 100ms inhibit, 1 bytes, Trigger
  MCO_InitTPDO(1,0x200,0,100,1,IN_digi_0); 

  // TPDO2, default ID ($NODEID+0x280), 0ms event, 100ms inhibit, 1 bytes, Pump motors
  MCO_InitTPDO(2,0x37c,0,100,1,IN_digi_1); 

  // TPDO3, default ID ($NODEID+0x380), 0ms event, 100ms inhibit, 2 bytes, actuator
  MCO_InitTPDO(3,0x36a,0,100,2,IN_digi_12); 

  //TPDO4, default ID ($NODEID+0x480), 0ms event, 100ms inhibit, 8 bytes, Heater
  MCO_InitTPDO(4,0x37e,0,100,8,IN_digi_15); 

  //TPDO5, default ID ($NODEID+0x580), 0ms event, 100ms inhibit, 8 bytes, Heater
  MCO_InitTPDO(5,0x47e,0,100,8,IN_digi_23); 
  
  //TPDO6, default ID ($NODEID+0x380), 0ms event, 100ms inhibit, 2 bytes, actuator
  MCO_InitTPDO(6,0x46a,0,100,2,IN_digi_32);

}

