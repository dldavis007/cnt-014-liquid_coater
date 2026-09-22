#ifndef _HC12DEF_H
#define _HC12DEF_H

/* These values are for a 8Mhz clock
 */
typedef enum {
	BAUD38K = 6, BAUD19K = 12, BAUD14K = 16,
	BAUD9600 = 48, BAUD4800 = 48, BAUD2400 = 96, 
	BAUD1200 = 192, BAUD600 = 384, BAUD300 = 1048
	} BaudRate;
void setbaud(BaudRate);

#ifndef INTR_ON
#define INTR_ON()	asm("cli")
#define INTR_OFF()	asm("sei")
#endif

#ifndef bit
#define bit(x)	(1 << (x))
#endif

#ifdef _SCI
/* SCI bits */
#define TE		bit(3)
#define RE		bit(2)
#define TDRE	bit(7)
#define TC		bit(6)
#define RDRF	bit(5)
#define T8		bit(6)
#define R8		bit(7)
#endif

#ifdef _SPI
/* SPI bits */
#define MSTR	bit(4)
#define SPE		bit(6)
#define SPIF	bit(7)
#endif

#ifdef _EEPROM
/* EEPROM */
#define EEPGM	bit(0)
#define EELAT	bit(1)
#endif

#define _FP_CONTEXT_SIZE	5

typedef struct
	{
	char fp1[_FP_CONTEXT_SIZE];
	char fp2[_FP_CONTEXT_SIZE];
	} _FP_CONTEXT;

void _FP_SaveContext(_FP_CONTEXT *buf);
void _FP_RestoreContext(_FP_CONTEXT *buf);

void write_eeprom (unsigned char *addr, unsigned char b);

#pragma nonpaged_function _icall
int _icall(int (*)(void));
#endif
