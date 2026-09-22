/**************************************************************************
MODULE:    MCOHW
CONTAINS:  Preliminary, limited hardware driver implementation for
           the 2-Wire Controller
           This version re-uses functions provided by
           www.esacademy.com/faq/progs
           If you need a full-featured 8XC591 MicroCANopen driver, contact
           support@esacademy.com
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
---------------------------------------------------------------------------
PRELIMINARY VERSION

Shortcomings:
Only supports up to 2 (out of 8 possible) receive filter
  => This version can only be used with a maximum of 2 RPDOs

Only supports a transmit queue of length "1"
If queue occupied, waits until it is clear
**************************************************************************/

#include "Controller.h"
#include "mc9s12a128.h"
#include "mcohw.h"
#include "Interrupts.h"
#include "Subroutines.h"


extern char Gen_Flags;
extern unsigned int Timer1;

// Global timer/conter variable, incremented every millisecond
UNSIGNED16 gTimCnt = 0;

// global can filter count
UNSIGNED8 gCANFilter = 0;

/**************************************************************************
DOES:    Sets one of the four screeners (acceptance filters) of
         the CAN controller.
CAUTION: Does assume the screeners are set-up to be used as "one
         long filter for standard messages"
RETURNS: nothing
**************************************************************************/
void set_screener_std
  (
  UNSIGNED8 Screener,  // 1 - 8, one of the 8 screeners
  UNSIGNED16 ID_Mask,   // Bit set: corresponding bit in ID is don't care
                  // Bit clear: corresponding bit in ID must match ID_Match
  UNSIGNED16 ID_Match  // ID_Match - Match/Code value for ID
  ) 
  //darrell
  //The following arguments are not used, this has been modified for 16 bit filters  			 			
  //UNSIGNED8 B1_Mask,   // Bit set: cor. bit in data byte 1 is don't care
                  // Bit clear: cor. bit in data byte 1 must match B1_Match
  //UNSIGNED8 B1_Match,  // Match/Code value for data byte 1
  //UNSIGNED8 B2_Mask,   // Bit set: cor. bit in data byte 2 is don't care
                  // Bit clear: cor. bit in data byte 2 must match B2_Match
  //UNSIGNED8 B2_Match   // Match/Code value for data byte 2
  //)
{
   // ensure 0 <= screener <= 7
  Screener -= 1;
  Screener &= 0x7;

  // Disable screener, match on any message
  //ID_Mask = 0xffff;
  
  switch (Screener)
  {
   	case 0:
	
      CANIDMR0 = (UNSIGNED8) (ID_Mask >> 3);
      //CANIDMR0 = (UNSIGNED8) (((ID_Mask & 0x07) << 5) | 0x1F);
	  
	  CANIDAR0 = (UNSIGNED8) (ID_Match >> 3);
	  //CANIDAR0 = (UNSIGNED8) (ID_Match & 0x07) << 5;
	break;
	
	case 1:
	
      CANIDMR1 = (UNSIGNED8) (ID_Mask >> 3);
      //CANIDMR1 = (UNSIGNED8) (((ID_Mask & 0x07) << 5) | 0x1F);
  
	  CANIDAR1 = (UNSIGNED8) (ID_Match >> 3);
	  //CANIDAR1 = (UNSIGNED8) (ID_Match & 0x07) << 5;
	break;
	  	
	case 2:
	
      CANIDMR2 = (UNSIGNED8) (ID_Mask >> 3);
      //CANIDMR2 = (UNSIGNED8) (((ID_Mask & 0x07) << 5) | 0x1F);
  
	  CANIDAR2 = (UNSIGNED8) (ID_Match >> 3);
	  //CANIDAR2 = (UNSIGNED8) (ID_Match & 0x07) << 5;
	break;
	  	
	case 3:
	
      CANIDMR3 = (UNSIGNED8) (ID_Mask >> 3);
      //CANIDMR3 = (UNSIGNED8) (((ID_Mask & 0x07) << 5) | 0x1F);
  
	  CANIDAR3 = (UNSIGNED8) (ID_Match >> 3);
	  //CANIDAR3 = (UNSIGNED8) (ID_Match & 0x07) << 5;
	break;		
	  	
	case 4:
	
      CANIDMR4 = (UNSIGNED8) (ID_Mask >> 3);
      //CANIDMR4 = (UNSIGNED8) (((ID_Mask & 0x07) << 5) | 0x1F);
  
	  CANIDAR4 = (UNSIGNED8) (ID_Match >> 3);
	  //CANIDAR4 = (UNSIGNED8) (ID_Match & 0x07) << 5;
	break;		
	  	
	case 5:
	
      CANIDMR5 = (UNSIGNED8) (ID_Mask >> 3);
      //CANIDMR5 = (UNSIGNED8) (((ID_Mask & 0x07) << 5) | 0x1F);
  
	  CANIDAR5 = (UNSIGNED8) (ID_Match >> 3);
	  //CANIDAR5 = (UNSIGNED8) (ID_Match & 0x07) << 5;
	break;		
	  	
	case 6:
	
      CANIDMR6 = (UNSIGNED8) (ID_Mask >> 3);
      //CANIDMR6 = (UNSIGNED8) (((ID_Mask & 0x07) << 5) | 0x1F);
  
	  CANIDAR6 = (UNSIGNED8) (ID_Match >> 3);
	  //CANIDAR6 = (UNSIGNED8) (ID_Match & 0x07) << 5;
	break;		
	  	
	case 7:
	
      CANIDMR7 = (UNSIGNED8) (ID_Mask >> 3);
      //CANIDMR7 = (UNSIGNED8) (((ID_Mask & 0x07) << 5) | 0x1F);
  
	  CANIDAR7 = (UNSIGNED8) (ID_Match >> 3);
	  //CANIDAR7 = (UNSIGNED8) (ID_Match & 0x07) << 5;
	break;		
  }
}


/**************************************************************************
DOES:    Sets one of the eight screeners (acceptance filters) of
         the CAN controller.
CAUTION: Does assume the screeners are set-up to be used as "eight
         short filters for upper 8 bits of short message"
RETURNS: nothing
**************************************************************************/
/*
void set_screener_std
  (
  UNSIGNED8 Screener,  // 1 - 4, one of the four screeners
  UNSIGNED8 ID_Mask,   // Bit set: corresponding bit in ID is don't care
                  // Bit clear: corresponding bit in ID must match ID_Match
  UNSIGNED8 ID_Match  // ID_Match - Match/Code value for ID
  ) 
  //darrell
  //The following arguments are not used, this has been modified for 16 bit filters  			 			
  //UNSIGNED8 B1_Mask,   // Bit set: cor. bit in data byte 1 is don't care
                  // Bit clear: cor. bit in data byte 1 must match B1_Match
  //UNSIGNED8 B1_Match,  // Match/Code value for data byte 1
  //UNSIGNED8 B2_Mask,   // Bit set: cor. bit in data byte 2 is don't care
                  // Bit clear: cor. bit in data byte 2 must match B2_Match
  //UNSIGNED8 B2_Match   // Match/Code value for data byte 2
  //)
{
   // ensure 0 <= screener <= 7
  Screener -= 1;
  Screener &= 0x7;

  switch (Screener)
  {
   	case 0:
      CANIDMR0 = (UNSIGNED8) (ID_Mask >> 3);
	  CANIDAR0 = (UNSIGNED8) (ID_Match >> 3);
	break;
	
	case 1:	
      CANIDMR1 = (UNSIGNED8) (ID_Mask >> 3);
	  CANIDAR1 = (UNSIGNED8) (ID_Match >> 3);
	break;
	  	
	case 2:
      CANIDMR2 = (UNSIGNED8) (ID_Mask >> 3);
	  CANIDAR2 = (UNSIGNED8) (ID_Match >> 3);
	break;
	  	
	case 3:
      CANIDMR3 = (UNSIGNED8) (ID_Mask >> 3);
	  CANIDAR3 = (UNSIGNED8) (ID_Match >> 3);
	break;	
		
   	case 4:
      CANIDMR4 = (UNSIGNED8) (ID_Mask >> 3);	  
	  CANIDAR4 = (UNSIGNED8) (ID_Match >> 3);
	break;
	
	case 5:
      CANIDMR5 = (UNSIGNED8) (ID_Mask >> 3);
	  CANIDAR5 = (UNSIGNED8) (ID_Match >> 3);
	break;
	  	
	case 6:
      CANIDMR6 = (UNSIGNED8) (ID_Mask >> 3);
	  CANIDAR6 = (UNSIGNED8) (ID_Match >> 3);
	break;
	  	
	case 7:
      CANIDMR7 = (UNSIGNED8) (ID_Mask >> 3);
	  CANIDAR7 = (UNSIGNED8) (ID_Match >> 3);
	break;		
  }
}
*/

/**************************************************************************
DOES:    Gets the next received CAN message and places it in
         a receive buffer                                         
RETURNS: 0 if no message received, 1 if message received and      
         copied to the buffer                                     
**************************************************************************/
UNSIGNED8 MCOHW_PullMessage
  (
  CAN_MSG *pReceiveBuf  // pointer to a single message sized buffer
                                // to hold the received message
  )
{
  // variable declarations
  UNSIGNED32 Identifier;
  UNSIGNED8  Length;
  UNSIGNED8  i;

  // Check the CAN status register for received message
  if ( CANRFLG & 0x01)
  {
    // Message received!

    Identifier   = (UNSIGNED32)CANRXIDR0;
    Identifier <<= 8;
    Identifier  |= (UNSIGNED32) CANRXIDR1;
    Identifier >>= 5;
	
	Length = (CANRXDLR & 0x0f);
	for ( i = 0; i < Length; i++ )
		*(UNSIGNED8 *)(pReceiveBuf->BUF+i) = *(&CANRXDSR0 + i);   	/* Get received data */
	CANRFLG = 0x01;	  				   						  				/* Clear RXF */

    pReceiveBuf->ID = Identifier;
    pReceiveBuf->LEN = Length;
	
	// release receive buffer
	CANRFLG |= 0x01;
	    // return 1, message received
    return (1);
  }
  else 
  {
    // return 0, no message received
    return (0);
  }
  
}

/**************************************************************************
DOES:    Transmits a CAN message                                  
RETURNS: 0 if the message could not be transmitted, 1 if the      
         message was transmitted                                  
**************************************************************************/
UNSIGNED8 MCOHW_PushMessage
  (
  CAN_MSG *pTransmitBuf  // pointer to buffer containing CAN
                                 // message to transmit
  )
{
 
  unsigned char txbuffer;
  unsigned char priority = 0;
  // CAN message identifier
  UNSIGNED32 Identifier;
  // length of data frame
  UNSIGNED8  Length;
  // local loop counter
  UNSIGNED8  i;

	
    // Prepare length code and identifier.
  	Length     = pTransmitBuf->LEN;
  	Identifier = (pTransmitBuf->ID << 5);

	
    // Wait until write access to CAN controller buffer is allowed 
    // David: rev 4.29 added Timer1 to while loops
	
  Timer1 = .5 * RTI_One_Sec;
  while ( !CANTFLG && Timer1);					 	  		   	 	 /* Is Transmit Buffer full?? */
	if ( !CANTFLG ) 					 	  		   	 	 /* Is Transmit Buffer full?? */
	{
	   return 0;
	}
	   
	CANTBSEL = CANTFLG;		  							 /* Select lowest empty buffer */
	txbuffer = CANTBSEL;								 /* Backup selected buffer */
	
	*((unsigned int *) ((unsigned int)(&CANTXIDR0))) = (unsigned int)Identifier;
	//darrell was --> *((unsigned long *) ((unsigned long)(&CANTXIDR0))) = (unsigned int)Identifier;
	
	for ( i = 0; i < Length; i++ )
	{
	 	*(&CANTXDSR0 + i) = *(UNSIGNED8 *)(pTransmitBuf->BUF+i);  /* Load data to Tx buffer 
														  		   * Data Segment Registers
														  		   */
	}
	
	CANTXDLR = Length;									 /* Set Data Length Code */ 
	CANTXTBPR = priority;								 /* Set priority */
	
	CANTFLG = txbuffer;									 /* Start transmission */
	

  //david added Timer1 to while loop in rev 4.29
	Timer1 = 0.5 * RTI_One_Sec;
	while ( (CANTFLG & txbuffer) != txbuffer  && Timer1 );		  /* Wait for Transmission */
	if ( !Timer1 )
	    Gen_Flags |= Gen_Flags_No2Wire;   
		  			   			 			 			  /* completion  */

	return 1;												  														 


}

/**************************************************************************
DOES:    Gets the value of the current 1 millisecond system timer 
RETURNS: The current timer tick                                   
**************************************************************************/
UNSIGNED16 MCOHW_GetTime
  (
  void
  )
{
  UNSIGNED16 tmp;

  // disable interrupts
  INTR_OFF();

  // make copy of current timer tick
  tmp = gTimCnt;

  // enable interrupts
  INTR_ON();

  return tmp;
}

/**************************************************************************
DOES:    Checks if a moment in time has passed (a timestamp has expired)
RETURNS: 0 if timestamp has not yet expired, 1 if the             
         timestamp has expired                                    
**************************************************************************/
UNSIGNED8 MCOHW_IsTimeExpired
  (
  UNSIGNED16 timestamp  // timestamp to check for expiration
  )
{
  UNSIGNED16 time_now;

  // disable interrupts
  INTR_OFF();
  // get current time
  time_now = gTimCnt;
  // enable interrupts
  INTR_ON();
  // ensure minimum runtime
  timestamp++;
  if (time_now > timestamp)
  {
    if ((time_now - timestamp) < 0x8000)
      return 1;
    else
      return 0;
  }
  else
  {
    if ((timestamp - time_now) > 0x8000)
      return 1;
    else
      return 0;
  }
}

/**************************************************************************
DOES:    Timer interrupt service routine                          
         Increments the global millisecond counter tick           
         This function needs to be called once every millisecond  
RETURNS: nothing
NOTE:    See TC3_Int_Handler                                                     
**************************************************************************/
void MCOHW_TimerISR
  (
  void
  ) 
{
  gTimCnt++;
}

/**************************************************************************
DOES:    Initializes the CAN interface.                           
CAUTION: Does not initialize filters - nothing will be received   
         unless screeners are set using set_screener_std          
RETURNS: 0 for failed init, 1 for successful init                 
**************************************************************************/
UNSIGNED8 MCOHW_Init
  (
  UNSIGNED16 BaudRate  // desired baudrate in kbps
  )
{

UNSIGNED8 i;
UNSIGNED8 baudrateok = 0;

 	CANCTL0 = 0x01;
  //David: added Timer1 in Rev 4.29
  Timer1 = .5 * RTI_One_Sec;
	while ( !(CANCTL1 & 0x01) && Timer1)
	{
	}
	
	CANCTL1 = 0x80;//CANCTL1 = 0xa0;
	CANBTR0 = 0xc1;
	CANBTR1 = 0x3a;

  // This version only supports 125kbit at 12MHz
  if (BaudRate == 125)
  {
     // BTR0 and BTR1 determine the baudrate and sample point position
     // set address to BTR0 register
	CANBTR0 = 0x43;
	CANBTR1 = 0x49;
 
     // the baudrate is supported
    baudrateok = 1;
  }

  // no filters configured
  gCANFilter = 0;

  // Clear all acceptance filters and masks, receive nothing
  for(i=1;i<=16;i++) *(&CANIDAR0 + i) = 0;

  // Set acceptance filter mode to accept standard frames with single
  // acceptance filter.
  //darrell change to 8 bit acceptance                                                    
  ;CANIDAC = 0x20;

  // release CAN controller
    CANCTL0 = 0x00;										 /* Exit Initialization Mode Request */
	//david update for rev 4.29, added timer1
  Timer1 = .5 * RTI_One_Sec;
  while ( (CANCTL1 & 0x01) != 0 && Timer1)						 /* Wait for Normal Mode */
	{
	}
  
  	// Initialize 1ms Timer interrupt 
 	TC3 = TCNT + TC_1ms;
 	TIE |= TIE_C3I;
 	

  return baudrateok;

  

}

#include "stdarg.h"
UNSIGNED8 setFilters[8] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80};
/**************************************************************************
DOES:    Initializes the next available filter                    
RETURNS: 0 for failed init, 1 for successful init
 
This function replaces the original function that just
set up to 8 filters, requiring a match of all bits.
This function can be called as previously with just
the CANID argument and it will function as it previously
was intended.
 
This function keeps track of all filters that have been set.
If the address (last 7 bits in CANID as defined by 'bin'
after applying the mask to the CANID) in a given CANID corresponds
to a set filter (kept in global variable list setFilters; declared
in mcohw.c), no filter is set. Otherwise, the filter is set,
the filter is added to setFilters, gCANFilter is incremented
and 1 (success) is returned
 
The call to this function in "MCO_Init" that sets the filter
for the NMT master does not need modified, should look like
the following: ("if (!MCOHW_SetCANFilter(0))")
 
The call to this function in "MCO_Init" that sets the filter
to accept SDO for this node, should look like the following
(such as; "if (!MCOHW_SetCANFilter(0x600+Node_ID))"
                 
The call to this function in "MCO_Init" that sets the filter
to accept RPDOs for this node, should look like the following
(such as; "if (!MCOHW_SetCANFilter(gRPDOConfig[PDO_NR].CANID))"
                 
**************************************************************************/
UNSIGNED8 MCOHW_SetCANFilter(UNSIGNED16 CANID)
{
    UNSIGNED8 i = 0;
    UNSIGNED8 bin = CANID & 0x7F;
 
    // Handle default where one filter covers all CANIDs matching NODE_ID
    UNSIGNED8 len = sizeof(setFilters) / sizeof(setFilters[0]);
    for (i = 0; i < len; i++) {
        if (setFilters[i] == bin) {
            // If a filter is already set for this CANID, print a message and return
            return 1;  // Successfully set the filter
        }
    }
 
    // check for limit
    if (gCANFilter >= NR_OF_RPDOS)  
    {
        return 0;  // Fail if all filters are used
    }
   
    CANCTL0 = 0x01;
    while ( !(CANCTL1 & 0x01) )
    {
    }
 
    // configure the filter
    set_screener_std(gCANFilter, 0xFF80, CANID  //darrell, use only the Node_ID
    //The following arguments are not used, this has been modified for 16 bit filters            
    //,0xFF,0xFF,0xFF,0xFF);
    );
    //darrell
    //added, exit initialization mode
    CANCTL0 = 0x00;                    /* Exit Initialization Mode Request */
    while ( (CANCTL1 & 0x01) != 0 )            /* Wait for Normal Mode */
    {
    }
   
    // Set the filter for a specific CANID
    setFilters[gCANFilter] = bin;
 
    // Increment current num of filters set
    gCANFilter++;
 
    return 1;  // Successfully set the filter
}
