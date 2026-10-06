#include "Subroutines.h"

#ifndef Comand_H
#define Comand_H

void command ( char CmdBuf[] );
char Checksum ( char *str );
void parse(char *pbuf, char *str);
void ucase (char *str);
void ltrim (char *c_ptr);
int hextoi(char *string);

//#define CmdBufLen 64

//#define CommXmitngF  0b00000001
//#define CommRcvdF    0b00000010
//#define CommIntEnabF 0b00000100

//#define SINRcvd 0x02

#endif