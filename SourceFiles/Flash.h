#ifndef Flash_H
#define Flash_H

void FlashInit ( void );
void FlashWrite ( int ArraySize, char WriteData[], int *WriteAddr );

#define FSTAT_CBEIF 0x80
#define FSTAT_CCIF 0x80

#endif