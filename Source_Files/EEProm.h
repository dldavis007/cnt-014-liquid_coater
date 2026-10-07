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
#ifdef PC_EEPROM
/* PC host: EEPROM image in RAM, persisted to a file (pc_side/core) */
#define EE_begin ((int)pc_eeprom)
#else
#define EE_begin 0x0800
#endif

#endif