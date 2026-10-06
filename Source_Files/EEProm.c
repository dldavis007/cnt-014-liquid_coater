#include "EEProm.h"
#include "mc9s12a128.h"
#include "Subroutines.h"

void EEInit ( void )
{
 	 ECLKDIV = (char)(OscClk * (5 + Tbus));
	 INITEE = INITEE_Init;
}



//  char TmpStr2[] = "Hello World!\r\n";
//  int *intptr;

//	intptr = (int*)0x461;
//	EEWrite ( sizeof (TmpStr2), TmpStr2, intptr );	
	

void EEWrite ( int ArraySize, char WriteData[], int *WriteAddr )
{
 	 //Erases and Writes an array of up to 128 char to EEPROM
	 
	 int i, j, TmpData;
	 char *tempAddr;
	 char tempArray[128];
	 
     
	 
	 //Add up to 3 more data bytes to beginning of array
	 //Addr must be divisible by 4 for erase operation
	 
	 tempAddr = (char*)WriteAddr;
     tempAddr = (char*)((int)tempAddr & ~0x03);
	 
	 j = 0;
	 if ( (int)WriteAddr != (int)tempAddr )
	 {
	  	for ( j=0;(int)tempAddr != (int)WriteAddr;tempAddr++, j++)
		{
			tempArray[j] = *tempAddr;
		}	
	 } 

	 //transfer the rest of the data to tempArray

     for (i=0 ;i < ArraySize;i++, j++)
     {
        tempArray[j] = WriteData[i];
     }

	 //pad end of array to make divisible by 4

     if ( (j & 0x03) )
     {
	 	tempAddr = (char*)WriteAddr;
     	tempAddr = (char*)((int)tempAddr & ~0x03);
	  	for ( ;j & 0x3;j++ )
		{
         	tempArray[j] = *(tempAddr + j);
		}
     }

	 
	 //Erase EEprom
	 
	 tempAddr = (char*)WriteAddr;
	 WriteAddr = (int*)((int)WriteAddr & ~0x03);

     for (i=0; i<j; i += 4, WriteAddr +=2 )
     {
		 *WriteAddr = TmpData;
		 ECMD = SecErase;
		 ESTAT = ESTAT_CBEIF;
		 while (!(ESTAT & ESTAT_CBEIF) );
	     ARMCOP = 0x55;
		 ARMCOP = 0xAA;
	 }
	 

	 //Write to EEPROM
	 
	 WriteAddr = (int*)tempAddr;	 
	 WriteAddr = (int*)((int)WriteAddr & ~0x03);

     for (i=0;i<j;i+=2, WriteAddr++)
     {
	  	 TmpData = tempArray[i]*256 + tempArray[i+1];
		 *WriteAddr = TmpData;
		 ECMD = WordPrg;
		 ESTAT = ESTAT_CBEIF;
		 while (!(ESTAT & ESTAT_CBEIF) );
	     ARMCOP = 0x55;
		 ARMCOP = 0xAA;
     }


}



