#ifndef VECTORS_H
#define VECTORS_H
//#define DUMMY_ENTRY     (void (*)(void))0xFFFF
//
// A Reset vector for this program.
//
#ifndef PC_SIDE   /* target only: ISR vector table at an absolute address (ICC12) */
extern void _start(void);
//extern void ic0_interrupt(void);
//extern void CANInit ( void );
//#define _start 0x4000
//#pragma abs_address:0xffd6  ff80
#pragma abs_address:0xFF80
void (*interrupt_vectors[])(void) =
{
		
        DUMMY_ENTRY, 	/*Reserved $FF80*/
		DUMMY_ENTRY, 	/*Reserved $FF82*/
		DUMMY_ENTRY, 	/*Reserved $FF84*/
		DUMMY_ENTRY, 	/*Reserved $FF86*/
		DUMMY_ENTRY, 	/*Reserved $FF88*/
		DUMMY_ENTRY, 	/*Reserved $FF8A*/
		DUMMY_ENTRY, 	/*PWM Emergency Shutdown*/
		DUMMY_ENTRY, 	/*Port P Interrupt*/
		DUMMY_ENTRY, 	/*MSCAN 4 Transmit*/
		DUMMY_ENTRY, 	/*MSCAN 4 Receive*/
		DUMMY_ENTRY, 	/*MSCAN 4 Error*/
		DUMMY_ENTRY, 	/*MSCAN 4 Wake-up*/
		DUMMY_ENTRY, 	/*MSCAN 3 Transmit*/
		DUMMY_ENTRY, 	/*MSCAN 3 Receive*/
		DUMMY_ENTRY, 	/*MSCAN 3 Error*/
		DUMMY_ENTRY, 	/*MSCAN 3 Wake-up*/
		DUMMY_ENTRY, 	/*MSCAN 2 Transmit*/
		DUMMY_ENTRY, 	/*MSCAN 2 Receive*/
		DUMMY_ENTRY, 	/*MSCAN 2 Error*/
		DUMMY_ENTRY, 	/*MSCAN 2 Wake-up*/
		DUMMY_ENTRY, 	/*MSCAN 1 Transmit*/
		DUMMY_ENTRY, 	/*MSCAN 1 Receive*/
		DUMMY_ENTRY, 	/*MSCAN 1 Error*/
		DUMMY_ENTRY, 	/*MSCAN 1 Wake-up*/
		DUMMY_ENTRY, 	/*MSCAN 0 Transmit*/
		CANRxISR, 		/*MSCAN 0 Receive*/
		DUMMY_ENTRY, 	/*MSCAN 0 Error*/
		DUMMY_ENTRY, 	/*MSCAN 0 Wake-up*/
		DUMMY_ENTRY, 	/*Flash*/
		DUMMY_ENTRY, 	/*EEPROM*/
		DUMMY_ENTRY, 	/*SPI2*/
		DUMMY_ENTRY, 	/*SPI1*/
		DUMMY_ENTRY, 	/*IIC Bus*/
		DUMMY_ENTRY, 	/*DLC*/
		DUMMY_ENTRY, 	/*SCME*/
		DUMMY_ENTRY, 	/*CRG Lock*/
		DUMMY_ENTRY, 	/*Pulse Accumulator B Overflow*/
		DUMMY_ENTRY, 	/*Modulus Down Counter Underflow*/
		DUMMY_ENTRY, 	/*Port H Interrupt*/
		DUMMY_ENTRY, 	/*Port J Interrupt*/
		DUMMY_ENTRY, 	/* ATD1 */
		DUMMY_ENTRY, 	/* ATD0 */
		DUMMY_ENTRY, 	/* SCI1 */
        DUMMY_ENTRY, 	/* SCI0 */
        DUMMY_ENTRY,    /* SPI */
        DUMMY_ENTRY,    /* PAIE */
        DUMMY_ENTRY,    /* PAO */
        DUMMY_ENTRY,    /* TOF */
        TC7_Int_Handler,    /* TOC5 */      /* HC12 TC7 */
        DUMMY_ENTRY,    /* TOC4 */      /* TC6 */
        TC5_Int_Handler,    /* TOC3 */      /* TC5 */
        TC4_Int_Handler,    /* TOC2 */      /* TC4 */
        TC3_Int_Handler,    /* TOC1 */      /* TC3 */
        DUMMY_ENTRY,    /* TIC3 */      /* TC2 */
        DUMMY_ENTRY,    /* TIC2 */      /* TC1 */
        TC0_Int_Handler,    /* TIC1 */      /* TC0 */
        RTI_Int_Handler,    /* RTI */
        IRQ_Int_Handler,    /* IRQ */
        DUMMY_ENTRY,    /* XIRQ */
        DUMMY_ENTRY,    /* SWI */
        DUMMY_ENTRY,    /* ILLOP */
        (void *)0x4000,    		/* COP */
        DUMMY_ENTRY,    /* CLM */
        (void *)0x4000,    		/* RESET */
};

#pragma end_abs_address
#endif   /* !PC_SIDE */
#endif