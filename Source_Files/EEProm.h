#ifndef EEProm_H
#define EEProm_H

#pragma paged_function EEInit EEWrite

void EEInit ( void );
void EEWrite ( int ArraySize, char WriteData[], int *WriteAddr );

#define ESTAT_CBEIF 0x80
#define ESTAT_CCIF 0x80

#define INITEE_Init 0x09


#define WordPrg 0x20
#define SecErase 0x40
#define EE_begin 0x0800

#endif