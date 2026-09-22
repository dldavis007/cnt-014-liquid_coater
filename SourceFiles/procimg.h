/**************************************************************************
MODULE:    PROCIMG
CONTAINS:  Process Image Configuration
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

#ifndef _PROCIMG_H
#define _PROCIMG_H


/**************************************************************************
DEFINES: Definition of the process image
Modify these for your application
**************************************************************************/

// Define the size of the process image
#define PROCIMG_SIZE 63

// Define process variables: offsets into the process image 
// Menu Control
#define IN_digi_0 0
// Pump Motor #
#define IN_digi_1 1
// Heater #
#define IN_digi_7 2
// Setpoint
#define IN_digi_8 3
// PGain
#define IN_digi_9 4
// IGain
#define IN_digi_10 5
// ILimit
#define IN_digi_11 6
// Actuator Position
#define IN_digi_12 7
// Actuator Speed
#define IN_digi_13 8
// Actuator Current
#define IN_digi_14 9
// Heater Base SetPoint
#define IN_digi_15 10
// Heater ISO SetPoint
#define IN_digi_16 11
// Heater Hose SetPoint
#define IN_digi_17 12
// Heater xxx
#define IN_digi_18 13
// Heater PGain Base
#define IN_digi_19 14
// Heater IGain Base
#define IN_digi_20 15
// Heater ILimit Base
#define IN_digi_21 16
// Heater xxx
#define IN_digi_22 17
// Heater PGain ISO
#define IN_digi_23 18
// Heater IGain ISO
#define IN_digi_24 19
// Heater ILimit ISO
#define IN_digi_25 20
// Heater xxx
#define IN_digi_26 21
// Heater PGain Hose 
#define IN_digi_27 22
// Heater IGain Hose
#define IN_digi_28 23
// Heater ILimit Hose
#define IN_digi_29 24
// Heater xxx
#define IN_digi_30 25
// Heater xxx
#define IN_digi_31 26
//Cleaner act
#define IN_digi_32 27
//Act Unit
#define IN_digi_33 28

// Analog Input 1 
#define IN_ana_1 29
// Analog Input 2 
#define IN_ana_2 31

//Initiate Menu/Trigger
//#define OUT_digi_0 33
// Button Box Data
#define OUT_digi_1 34
// Button Box Data
#define OUT_digi_2 35
// Button Box Data
#define OUT_digi_3 36
// not used
#define OUT_digi_4 37
// not used
#define OUT_digi_5 38
// not used
#define OUT_digi_6 39
// Actuator Moving
#define OUT_digi_7 40
// Camera address
#define OUT_digi_8 41
//
#define OUT_digi_9 42
// Camera Command
#define OUT_digi_10 43
//
#define OUT_digi_11 44
//
#define OUT_digi_12 45
//
#define OUT_digi_13 46
//
#define OUT_digi_14 47
//
#define OUT_digi_15 48


// Temperature Base
#define OUT_ana_0 49
// Temperature ISO
#define OUT_ana_1 51
// Temperature Hose
#define OUT_ana_2 53


#define OUT_digi_0 55

#endif
