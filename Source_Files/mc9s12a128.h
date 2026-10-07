#ifndef _MC9S12A128_H
#define _MC9S12A128_H

/*****************************************************************************
*     Filename  : mc9s12a128.h
*     Processor : MC9S12A128
*	  DocumentId: 9S12DT128DGV2/D 	
*     Version   : V02.09
*     Date		: 13-Jan-2003
*     Abstract  :
*         This implements an IO devices mapping.
*
*     (c) Copyright Chipwerks Pvt Ltd 2003-2005
*     mail      : info@chipwerks.com
******************************************************************************
					
					  MC9S12DT128  USER CONFIGURABLE MEMORY MAP
								Covers MC9S12A128

After Reset the Memory Map is :

0x0000 - 0x03FF : Register Space
0x0000 - 0x1FFF : 8K RAM 
0x0000 - 0x07FF : 2K EEPROM (not visible)

Useful map, which is not the map out of reset : 

0x0000 - 0x03FF : 1K Register Space, Mappable to any 2K Boundary
0x0800 - 0x0FFF : 2K Bytes of EEPROM, Mappable to any 2K Boundary
0x2000 - 0x3FFF : 8K bytes RAM, Mappable to any 8K boundary
0x4000 - 0x7FFF : 0.5K, 1K, 2K or 4K Protected Sector, 16K Fixed Flash EEPROM
0x8000 - 0xBFFF : 16K page window, eight * 16K Flash EEPROM Pages
0xC000 - 0xFFFF : 16K Fixed Flash EEPROM, 2K, 4K, 8K or 16K Protected Boot Sector

0xFF00 - 0xFFFF : Will have Vectors in both Normal and Special Single Chip.
or
0xFF00 - 0xFFFF : BDM (If active)

******************************************************************************

				MC9S12A128 DEVICE REGISTER MAP OVER VIEW 

Address							Module							(Bytes)Size						
------------------------------------------------------------------------------
0x0000 - 0x0017	CORE (Ports A, B, E, Modes, Inits, Test)					24
0x0018 - 0x0019	Reserved													 2
0x001A - 0x001B	Device ID register (PARTID)									 2
0x001C - 0x001F	CORE (MEMSIZ, IRQ, HPRIO)									 4
0x0020 - 0x0027	Reserved													 8
0x0028 - 0x002F	CORE (Background Debug Module)								 8
0x0030 - 0x0033	CORE (PPAGE, Port K)										 4
0x0034 - 0x003F	Clock and Reset Generator (PLL, RTI, COP)					12
0x0040 - 0x007F	Enhanced Capture Timer 16-bit 8 channels					64
0x0080 - 0x009F	Analog to Digital Converter 10-bit 8 channels (ATD0)		32
0x00A0 - 0x00C7	Pulse Width Modulator 8-bit 8 channels (PWM)				40
0x00C8 - 0x00CF	Serial Communications Interface (SCI0)						 8
0x00D0 - 0x00D7	Serial Communications Interface (SCI1)						 8
0x00D8 - 0x00DF	Serial Peripheral Interface (SPI0)							 8
0x00E0 - 0x00E7	Inter IC Bus												 8
0x00F0 - 0x00F7	Serial Peripheral Interface (SPI1)							 8
0x00F8 - 0x00FF	Reserved													 8
0x0100 - 0x010F	Flash Control Register										16
0x0110 - 0x011B	EEPROM Control Register										12
0x011C - 0x011F	Reserved													 4
0x0120 - 0x013F	Analog to Digital Converter 10-bit 8 channels (ATD1)		32
0x01C0 - 0x01FF	Reserved													64
0x0200 - 0x023F	Reserved													64
0x0240 - 0x027F	Port Integration Module (PIM)								64
0x02C0 - 0x02FF	Reserved													64
0x0360 - 0x03FF Reserved												   160
0x0000 - 0x07FF EEPROM array											  2048
0x0000 - 0x1FFF RAM array												  8192
0x4000 - 0x7FFF Fixed Flash EEPROM array
				incl. 0.5K, 1K, 2K or 4K Protected Sector at start		 16384
0x8000 - 0xBFFF Flash EEPROM Page Window								 16384
0xC000 - 0xFFFF Fixed Flash EEPROM array
				incl. 0.5K, 1K, 2K or 4K Protected Sector at end
				and 256 bytes of Vector Space at $FF80 - $FFFF			 16384

*****************************************************************************/

#ifdef PC_SIDE
/* Host build (GCC): SFRs are not memory-mapped at 0 on a PC. Point _REG_BASE at
 * a plain byte array (defined in pc_side/core/host_runtime.c / hardware_stubs.c) so every
 * register macro below compiles and runs unchanged. */
extern unsigned char sfr_regs[0x400];
#define _REG_BASE        sfr_regs
#else
#define _REG_BASE        0
#endif
#define _ADDR(off)      (unsigned char volatile *)(_REG_BASE + off)
#define _P(off)         *(unsigned char volatile *)(_REG_BASE + off)
#define _LP(off)        *(unsigned short volatile *)(_REG_BASE + off)


#define  PORTA   _P(0x00)
#define  PORTAB   _LP(0x00)
#define  PORTB   _P(0x01)
#define  DDRA   _P(0x02)
#define  DDRAB   _LP(0x02)
#define  DDRB   _P(0x03)
//#define  Reserved		_P(0x04-0x07)
#define  PORTE   _P(0x08)
#define  DDRE   _P(0x09)
#define  PEAR   _P(0x0A)
#define  MODE   _P(0x0B)
#define  PUCR   _P(0x0C)
#define  RDRIV   _P(0x0D)
#define  EBICTL   _P(0x0E)
//#define  Reserved		_P(0x0F)


#define  INITRM   _P(0x10)
#define  INITRG   _P(0x11)
#define  INITEE   _P(0x12)
#define  MISC   _P(0x13)
//#define  Reserved		_P(0x14)
#define  MTST0   _P(0x14)


#define  ITCR   _P(0x15)
#define  ITEST   _P(0x16)


#define  MTST1   _P(0x17)


//#define  Reserved		_P(0x18)
//#define  Reserved		_P(0x19)


#define  PARTIDH   _P(0x1A)
#define  PARTIDL   _P(0x1B)


#define  MEMSIZ0   _P(0x1C)
#define  MEMSIZ1   _P(0x1D)


#define  INTCR   _P(0x1E)


#define  HPRIO   _P(0x1F)


//#define  Reserved		_P(0x20-0x27)


#define  BKPCT0   _P(0x28)
#define  BKPCT1   _P(0x29)
#define  BKP0X   _P(0x2A)
#define  BKP0H   _P(0x2B)
#define  BKP0L   _P(0x2C)
#define  BKP1X   _P(0x2D)
#define  BKP1H   _P(0x2E)
#define  BKP1L   _P(0x2F)


#define  PPAGE   _P(0x30)
//#define  Reserved		_P(0x31)


#define  PORTK   _P(0x32)
#define  DDRK   _P(0x33)


#define  SYNR   _P(0x34)
#define  REFDV   _P(0x35)
//#define  CTFLG Test Only	_P(0x36)
#define  CRGFLG   _P(0x37)
#define  CRGINT   _P(0x38)
#define  CLKSEL   _P(0x39)
#define  PLLCTL   _P(0x3A)
#define  RTICTL   _P(0x3B)
#define  COPCTL   _P(0x3C)
//#define  FORBYPE Test Only	_P(0x3D)
//#define  CTCTL Test Only	_P(0x3E)
#define  ARMCOP   _P(0x3F)


#define  TIOS   _P(0x40)
#define  CFORC   _P(0x41)
#define  OC7M   _P(0x42)
#define  OC7D   _P(0x43)
#define  TCNT   _LP(0x44)
#define  TCNTHi   _P(0x44)
#define  TCNTLo   _P(0x45)
#define  TSCR1   _P(0x46)
#define  TTOV   _P(0x47)
#define  TCTL1   _P(0x48)
#define  TCTL2   _P(0x49)
#define  TCTL3   _P(0x4A)
#define  TCTL4   _P(0x4B)
#define  TIE   _P(0x4C)
#define  TSCR2   _P(0x4D)
#define  TFLG1   _P(0x4E)
#define  TFLG2   _P(0x4F)
#define  TC0   _LP(0x50)
#define  TC0Hi   _P(0x50)
#define  TC0Lo   _P(0x51)
#define  TC1   _LP(0x52)
#define  TC1Hi   _P(0x52)
#define  TC1Lo   _P(0x53)
#define  TC2   _LP(0x54)
#define  TC2Hi   _P(0x54)
#define  TC2Lo   _P(0x55)
#define  TC3   _LP(0x56)
#define  TC3Hi   _P(0x56)
#define  TC3Lo   _P(0x57)
#define  TC4   _LP(0x58)
#define  TC4Hi   _P(0x58)
#define  TC4Lo   _P(0x59)
#define  TC5   _LP(0x5A)
#define  TC5Hi   _P(0x5A)
#define  TC5Lo   _P(0x5B)
#define  TC6   _LP(0x5C)
#define  TC6Hi   _P(0x5C)
#define  TC6Lo   _P(0x5D)
#define  TC7   _LP(0x5E)
#define  TC7Hi   _P(0x5E)
#define  TC7Lo   _P(0x5F)
#define  PACTL   _P(0x60)
#define  PAFLG   _P(0x61)
#define  PACN3   _P(0x62)
#define  PACN32   _LP(0x62)
#define  PACN2   _P(0x63)
#define  PACN1   _P(0x64)
#define  PACN10   _LP(0x64)
#define  PACN0   _P(0x65)
#define  MCCTL   _P(0x66)
#define  MCFLG   _P(0x67)
#define  ICPAR   _P(0x68)
#define  DLYCT   _P(0x69)
#define  ICOVW   _P(0x6A)
#define  ICSYS   _P(0x6B)
//#define  Reserved		_P(0x6C)
//#define  TIMTST Test Only	_P(0x6D)
//#define  Reserved		_P(0x6E)
//#define  Reserved		_P(0x6F)
#define  PBCTL   _P(0x70)
#define  PBFLG   _P(0x71)
#define  PA32H   _LP(0x72)
#define  PA3H   _P(0x72)
#define  PA2H   _P(0x73)
#define  PA10H   _LP(0x74)
#define  PA1H   _P(0x74)
#define  PA0H   _P(0x75)
#define  MCCNT   _LP(0x76)
#define  MCCNThi   _P(0x76)
#define  MCCNTlo   _P(0x77)
#define  TC0H   _LP(0x78)
#define  TC0Hhi   _P(0x78)
#define  TC0Hlo   _P(0x79)
#define  TC1H   _LP(0x7A)
#define  TC1Hhi   _P(0x7A)
#define  TC1Hlo   _P(0x7B)
#define  TC2H   _LP(0x7C)
#define  TC2Hhi   _P(0x7C)
#define  TC2Hlo   _P(0x7D)
#define  TC3H   _LP(0x7E)
#define  TC3Hhi   _P(0x7E)
#define  TC3Hlo   _P(0x7F)
#define  ATD0CTL0	_P(0x80)
#define  ATD0CTL1	_P(0x81)
#define  ATD0CTL01	 _LP(0x80)
#define  ATD0CTL2   _P(0x82)
#define  ATD0CTL23   _LP(0x82)
#define  ATD0CTL3   _P(0x83)
#define  ATD0CTL4   _P(0x84)
#define  ATD0CTL45   _LP(0x84)
#define  ATD0CTL5   _P(0x85)
#define  ATD0STAT0   _P(0x86)
//#define  Reserved		_P(0x87)
#define	ATD0TEST0	_P(0x88)
#define	ATD0TEST1	_P(0x89)
//#define  Reserved		_P(0x8A)
#define  ATD0STAT1   _P(0x8B)
//#define  Reserved		_P(0x8C)
#define  ATD0DIEN   _P(0x8D)
//#define  Reserved		_P(0x8E)
#define  PORTAD0   _P(0x8F)
#define  ATD0DR0   _LP(0x90)
#define  ATD0DR0H   _P(0x90)
#define  ATD0DR0L   _P(0x91)
#define  ATD0DR1   _LP(0x92)
#define  ATD0DR1H   _P(0x92)
#define  ATD0DR1L   _P(0x93)
#define  ATD0DR2   _LP(0x94)
#define  ATD0DR2H   _P(0x94)
#define  ATD0DR2L   _P(0x95)
#define  ATD0DR3   _LP(0x96)
#define  ATD0DR3H   _P(0x96)
#define  ATD0DR3L   _P(0x97)
#define  ATD0DR4   _LP(0x98)
#define  ATD0DR4H   _P(0x98)
#define  ATD0DR4L   _P(0x99)
#define  ATD0DR5   _LP(0x9A)
#define  ATD0DR5H   _P(0x9A)
#define  ATD0DR5L   _P(0x9B)
#define  ATD0DR6   _LP(0x9C)
#define  ATD0DR6H   _P(0x9C)
#define  ATD0DR6L   _P(0x9D)
#define  ATD0DR7   _LP(0x9E)
#define  ATD0DR7H   _P(0x9E)
#define  ATD0DR7L   _P(0x9F)


#define  PWME   _P(0xA0)
#define  PWMPOL   _P(0xA1)
#define  PWMCLK   _P(0xA2)
#define  PWMPRCLK   _P(0xA3)
#define  PWMCAE   _P(0xA4)
#define  PWMCTL   _P(0xA5)
//#define	PWMTST TEST ONLY	_P(0xA6)
//#define  PWMPRSC TEST ONLY	_P(0xA7)
#define  PWMSCLA   _P(0xA8)
#define  PWMSCLB   _P(0xA9)
//#define  PWMSCNTA TEST ONLY   _P(0xAA)
//#define  PWMSCNTB TEST ONLY   _P(0xAB)
#define  PWMCNT0   _P(0xAC)
#define  PWMCNT01   _LP(0xAC)
#define  PWMCNT1   _P(0xAD)
#define  PWMCNT2   _P(0xAE)
#define  PWMCNT23   _LP(0xAE)
#define  PWMCNT3   _P(0xAF)
#define  PWMCNT4   _P(0xB0)
#define  PWMCNT45   _LP(0xB0)
#define  PWMCNT5   _P(0xB1)
#define  PWMCNT6   _P(0xB2)
#define  PWMCNT67   _LP(0xB2)
#define  PWMCNT7   _P(0xB3)
#define  PWMPER0   _P(0xB4)
#define  PWMPER01   _LP(0xB4)
#define  PWMPER1   _P(0xB5)
#define  PWMPER2   _P(0xB6)
#define  PWMPER23   _LP(0xB6)
#define  PWMPER3   _P(0xB7)
#define  PWMPER4   _P(0xB8)
#define  PWMPER45   _LP(0xB8)
#define  PWMPER5   _P(0xB9)
#define  PWMPER6   _P(0xBA)
#define  PWMPER67   _LP(0xBA)
#define  PWMPER7   _P(0xBB)
#define  PWMDTY0   _P(0xBC)
#define  PWMDTY01   _LP(0xBC)
#define  PWMDTY1   _P(0xBD)
#define  PWMDTY2   _P(0xBE)
#define  PWMDTY23   _LP(0xBE)
#define  PWMDTY3   _P(0xBF)
#define  PWMDTY4   _P(0xC0)
#define  PWMDTY45   _LP(0xC0)
#define  PWMDTY5   _P(0xC1)
#define  PWMDTY6   _P(0xC2)
#define  PWMDTY67   _LP(0xC2)
#define  PWMDTY7   _P(0xC3)
#define  PWMSDN   _P(0xC4)
//#define  Reserved		_P(0xC5-0XC7)


#define  SCI0BD   _LP(0xC8)
#define  SCI0BDH   _P(0xC8)
#define  SCI0BDL   _P(0xC9)
#define  SCI0CR1   _P(0xCA)
#define  SCI0CR2   _P(0xCB)
#define  SCI0SR1   _P(0xCC)
#define  SCI0SR2   _P(0xCD)
#define  SCI0DRH   _P(0xCE)
#define  SCI0DRL   _P(0xCF)


#define  SCI1BD   _LP(0xD0)
#define  SCI1BDH   _P(0xD0)
#define  SCI1BDL   _P(0xD1)
#define  SCI1CR1   _P(0xD2)
#define  SCI1CR2   _P(0xD3)
#define  SCI1SR1   _P(0xD4)
#define  SCI1SR2   _P(0xD5)
#define  SCI1DRH   _P(0xD6)
#define  SCI1DRL   _P(0xD7)


#define  SPI0CR1   _P(0xD8)
#define  SPI0CR2   _P(0xD9)
#define  SPI0BR   _P(0xDA)
#define  SPI0SR   _P(0xDB)
//#define  Reserved		_P(0xDC)
#define  SPI0DR   _P(0xDD)
//#define  Reserved		_P(0xDE)
//#define  Reserved		_P(0xDF)


#define  IBAD   _P(0xE0)
#define  IBFD   _P(0xE1)
#define  IBCR   _P(0xE2)
#define  IBSR   _P(0xE3)
#define  IBDR   _P(0xE4)
//#define  Reserved		_P(0xE5)
//#define  Reserved		_P(0xE6)
//#define  Reserved		_P(0xE7)

// BDLC not enabled 0xE8 - 0xEF

#define  SPI1CR1   _P(0xF0)
#define  SPI1CR2   _P(0xF1)
#define  SPI1BR   _P(0xF2)
#define  SPI1SR   _P(0xF3)
//#define  Reserved		_P(0xF4)
#define  SPI1DR   _P(0xF5)
//#define  Reserved		_P(0xF6)
//#define  Reserved		_P(0xF7)


//#define  Reserved		_P(0xF8-0XFF)


#define  FCLKDIV   _P(0x100)
#define  FSEC   _P(0x101)
//define  FTSTMOD	_P(0X102)
#define  FCNFG   _P(0x103)
#define  FPROT   _P(0x104)
#define  FSTAT   _P(0x105)
#define  FCMD   _P(0x106)
//#define	Reserved for Factory Test	_P(0x107)
#define  FADDRHI   _P(0x108)
#define  FADDRLO   _P(0x109)
#define  FDATAHI   _P(0x10A)
#define  FDATALO   _P(0x10B)
//#define	Reserved	_P(0x10C-0x10F)


#define  ECLKDIV   _P(0x110)
//#define	Reserved	_P(0x111)
//#define	Reserved For Factory Test	_P(0x112)
#define  ECNFG   _P(0x113)
#define  EPROT   _P(0x114)
#define  ESTAT   _P(0x115)
#define  ECMD   _P(0x116)
//#define	Reserved For Factory Test	_P(0x117)
#define  EADDRHI   _P(0x118)
#define  EADDRLO   _P(0x119)
#define  EDATAHI   _P(0x11A)
#define  EDATALO   _P(0x11B)


//#define	Reserved	_P(0x11C-0x11F)


#define  ATD1CTL0   _P(0x120)
#define  ATD1CTL1   _P(0x121)
//#define  ATD1CTL2   _LP(0x120)
#define  ATD1CTL2   _P(0x122)
#define  ATD1CTL23   _LP(0x122)
#define  ATD1CTL3   _P(0x123)
#define  ATD1CTL4   _P(0x124)
#define  ATD1CTL45   _LP(0x124)
#define  ATD1CTL5   _P(0x125)
#define  ATD1STAT0   _P(0x126)
//#define	Reserved	_P(0x127)
#define  ATD1TEST0   _P(0x128)
#define  ATD1TEST1   _P(0x129)
//#define	Reserved	_P(0x12A)
#define  ATD1STAT1   _P(0x12B)
//#define	Reserved	_P(0x12C)
#define  ATD1DIEN   _P(0x12D)
//#define	Reserved	_P(0x12E)
#define  PORTAD1   _P(0x12F)
#define  ATD1DR0   _LP(0x130)
#define  ATD1DR0H   _P(0x130)
#define  ATD1DR0L   _P(0x131)
#define  ATD1DR1   _LP(0x132)
#define  ATD1DR1H   _P(0x132)
#define  ATD1DR1L   _P(0x133)
#define  ATD1DR2   _LP(0x134)
#define  ATD1DR2H   _P(0x134)
#define  ATD1DR2L   _P(0x135)
#define  ATD1DR3   _LP(0x136)
#define  ATD1DR3H   _P(0x136)
#define  ATD1DR3L   _P(0x137)
#define  ATD1DR4   _LP(0x138)
#define  ATD1DR4H   _P(0x138)
#define  ATD1DR4L   _P(0x139)
#define  ATD1DR5   _LP(0x13A)
#define  ATD1DR5H   _P(0x13A)
#define  ATD1DR5L   _P(0x13B)
#define  ATD1DR6   _LP(0x13C)
#define  ATD1DR6H   _P(0x13C)
#define  ATD1DR6L   _P(0x13D)
#define  ATD1DR7   _LP(0x13E)
#define  ATD1DR7H   _P(0x13E)
#define  ATD1DR7L   _P(0x13F)


// CAN0 not enabled 0x140 - 0x17F
// CAN1 not enabled 0x180 - 0x1BF
// Reserved 0x1C0 - 0x23F   


#define  PTT   _P(0x240)
#define  PTIT   _P(0x241)
#define  DDRT   _P(0x242)
#define  RDRT   _P(0x243)
#define  PERT   _P(0x244)
#define  PPST   _P(0x245)
//#define	Reserved	_P(0x246)
//#define	Reserved	_P(0x147)
#define  PTS   _P(0x248)
#define  PTIS   _P(0x249)
#define  DDRS   _P(0x24A)
#define  RDRS   _P(0x24B)
#define  PERS   _P(0x24C)
#define  PPSS   _P(0x24D)
#define  WOMS   _P(0x24E)
//#define	Reserved	_P(0x24F)
#define  PTM   _P(0x250)
#define  PTIM   _P(0x251)
#define  DDRM   _P(0x252)
#define  RDRM   _P(0x253)
#define  PERM   _P(0x254)
#define  PPSM   _P(0x255)
#define  WOMM   _P(0x256)
#define  MODRR   _P(0x257)
#define  PTP   _P(0x258)
#define  PTIP   _P(0x259)
#define  DDRP   _P(0x25A)
#define  RDRP   _P(0x25B)
#define  PERP   _P(0x25C)
#define  PPSP   _P(0x25D)
#define  PIEP   _P(0x25E)
#define  PIFP   _P(0x25F)
#define  PTH   _P(0x260)
#define  PTIH   _P(0x261)
#define  DDRH   _P(0x262)
#define  RDRH   _P(0x263)
#define  PERH   _P(0x264)
#define  PPSH   _P(0x265)
#define  PIEH   _P(0x266)
#define  PIFH   _P(0x267)
#define  PTJ   _P(0x268)
#define  PTIJ   _P(0x269)
#define  DDRJ   _P(0x26A)
#define  RDRJ   _P(0x26B)
#define  PERJ   _P(0x26C)
#define  PPSJ   _P(0x26D)
#define  PIEJ   _P(0x26E)
#define  PIFJ   _P(0x26F)
//#define	Reserved	_P(0x270-0X27F)

// CAN4 not enabled 0x280 - 0x2bf


//#define	Reserved	_P(0x2C0-0X2FF)

// byte flight not enabled 0x300 - 0x35F
//#define	Reserved	_P(0x360-0x3FF)


#define  BDMSTS   _P(0xFF01)
#define  BDMCCR   _P(0xFF06)
#define  BDMINR   _P(0xFF07)

#define _CAN_BASE		0x140

#define  CANCTL0    _P(0x00 + _CAN_BASE)
#define  CANCTL1    _P(0x01 + _CAN_BASE)
#define	 CANBTR0    _P(0x02 + _CAN_BASE)
#define	 CANBTR1    _P(0x03 + _CAN_BASE)
#define	 CANRFLG    _P(0x04 + _CAN_BASE)
#define	 CANRIER    _P(0x05 + _CAN_BASE)
#define	 CANTFLG    _P(0x06 + _CAN_BASE)
#define	 CANTIER    _P(0x07 + _CAN_BASE)
#define	 CANTARQ    _P(0x08 + _CAN_BASE)
#define	 CANTAAK    _P(0x09 + _CAN_BASE)
#define	 CANTBSEL   _P(0x0a + _CAN_BASE)
#define	 CANIDAC    _P(0x0b + _CAN_BASE)

#define	 CANRXERR   _P(0x0e + _CAN_BASE)
#define	 CANTXERR   _P(0x0f + _CAN_BASE)
#define	 CANIDAR0   _P(0x10 + _CAN_BASE)
#define	 CANIDAR1   _P(0x11 + _CAN_BASE)
#define	 CANIDAR2   _P(0x12 + _CAN_BASE)
#define	 CANIDAR3   _P(0x13 + _CAN_BASE)
#define	 CANIDMR0   _P(0x14 + _CAN_BASE)
#define	 CANIDMR1   _P(0x15 + _CAN_BASE)
#define	 CANIDMR2   _P(0x16 + _CAN_BASE)
#define	 CANIDMR3   _P(0x17 + _CAN_BASE)
#define	 CANIDAR4   _P(0x18 + _CAN_BASE)
#define	 CANIDAR5   _P(0x19 + _CAN_BASE)
#define	 CANIDAR6   _P(0x1a + _CAN_BASE)
#define	 CANIDAR7   _P(0x1b + _CAN_BASE)
#define	 CANIDMR4   _P(0x1c + _CAN_BASE)
#define	 CANIDMR5   _P(0x1d + _CAN_BASE)
#define	 CANIDMR6   _P(0x1e + _CAN_BASE)
#define	 CANIDMR7   _P(0x1f + _CAN_BASE)

#define	 CANRXFG    _P(0x20 + _CAN_BASE)
#define  CANRXIDR0  _P(0x20 + _CAN_BASE)
#define  CANRXIDR1  _P(0x21 + _CAN_BASE)
#define  CANRXIDR2  _P(0x22 + _CAN_BASE)
#define  CANRXIDR3  _P(0x23 + _CAN_BASE)
#define  CANRXDSR0  _P(0x24 + _CAN_BASE)
#define  CANRXDSR1  _P(0x25 + _CAN_BASE)
#define  CANRXDSR2  _P(0x26 + _CAN_BASE)
#define  CANRXDSR3  _P(0x27 + _CAN_BASE)
#define  CANRXDSR4  _P(0x28 + _CAN_BASE)
#define  CANRXDSR5  _P(0x29 + _CAN_BASE)
#define  CANRXDSR6  _P(0x2a + _CAN_BASE)
#define  CANRXDSR7  _P(0x2b + _CAN_BASE)
#define  CANRXDLR   _P(0x2c + _CAN_BASE)
#define  CANRXTBPR  _P(0x2d + _CAN_BASE)

#define	 CANTXFG    _P(0x30 + _CAN_BASE)
#define  CANTXIDR0  _P(0x30 + _CAN_BASE)
#define  CANTXIDR1  _P(0x31 + _CAN_BASE)
#define  CANTXIDR2  _P(0x32 + _CAN_BASE)
#define  CANTXIDR3  _P(0x33 + _CAN_BASE)
#define  CANTXDSR0  _P(0x34 + _CAN_BASE)
#define  CANTXDSR1  _P(0x35 + _CAN_BASE)
#define  CANTXDSR2  _P(0x36 + _CAN_BASE)
#define  CANTXDSR3  _P(0x37 + _CAN_BASE)
#define  CANTXDSR4  _P(0x38 + _CAN_BASE)
#define  CANTXDSR5  _P(0x39 + _CAN_BASE)
#define  CANTXDSR6  _P(0x3a + _CAN_BASE)
#define  CANTXDSR7  _P(0x3b + _CAN_BASE)
#define  CANTXDLR   _P(0x3c + _CAN_BASE)
#define  CANTXTBPR  _P(0x3d + _CAN_BASE)

#endif
