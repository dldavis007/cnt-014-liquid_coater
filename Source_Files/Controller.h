#ifndef Controller_H
#define Controller_H

#ifndef INTR_ON
#define INTR_ON()	asm("cli")
#define INTR_OFF()	asm("sei")
#endif


//#define CommPull 0x20
//#define CommOut 0x08



#endif