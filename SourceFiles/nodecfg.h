/**************************************************************************
MODULE:    NODECFG
CONTAINS:  MicroCANopen Node Configuation
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

#ifndef _NODECFG_H
#define _NODECFG_H


/**************************************************************************
DEFINES: MEMORY TYPES USED
**************************************************************************/

// CONST Object Dictionary Data
//darrell #define MEM_CONST const code
#define MEM_CONST const

// Process data and frequently used variables
//darrell #define MEM_NEAR data

// Seldomly used variables
//darrell #define MEM_FAR xdata


/**************************************************************************
DEFINES: DATA TYPES USED
**************************************************************************/

#define UNSIGNED8 unsigned char
#define UNSIGNED16 unsigned int
#define UNSIGNED32 unsigned long


/**************************************************************************
DEFINES: CONST ENTRIES IN OBJECT DICTIONARY
Modify these for your application
**************************************************************************/

#define OD_DEVICE_TYPE   0x000F0191L 
#define OD_VENDOR_ID     0x43525453L
#define OD_PRODUCT_CODE  0x00010002L
#define OD_REVISION      0x00010000L

// The following are optional and can also be left "undefined"
#define OD_SERIAL        0xFFFFFFFFL


/**************************************************************************
DEFINES: ENABLING/DISABLING CODE FUNCTIONALITY
**************************************************************************/

// Maximum number of transmit PDOs (0 to 8)
#define NR_OF_TPDOS 8
//#define NR_OF_TPDOS 0

// Maximum number of receive PDOs (0 to 8)
#define NR_OF_RPDOS 8
//#define NR_OF_RPDOS 1

// If defined, 1 or more TPDOs use the event timer
#define USE_EVENT_TIME

// If defined, 1 or more TPDOs are change-of-state and use the inhibit timer
#define USE_INHIBIT_TIME

// If defined, the PDO parameters are added to the Object Dictionary
// Entries must be added to the SDOResponseTable in user_xxxx.c
// Suppported in commercial version of MicroCANopen available from
// www.CANopenStore.com

//#define PDO_IN_OD

// If defined, the Process Data is accesible via SDO requests
// Entries must be added to the ODProcTable in user_xxxx.c
// Suppported in commercial version of MicroCANopen available from
// www.CANopenStore.com

//#define PROCIMG_IN_OD

// If defined, OD entry [1017,00] is supported with SDO read/write accesses
// This is also an example on how to implement dynamic/variable OD entries
#define DYNAMIC_HEARTBEAT

// If defined, node starts up automatically (does not wait for NMT master)
#define AUTOSTART

// If defined, all parameters passed to functions are checked for consistency. 
// On failures, the user function MCOUSER_FatalError is called.
#define CHECK_PARAMETERS


#endif
