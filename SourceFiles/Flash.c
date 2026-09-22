#include "Flash.h"
#include "mc9s12a128.h"
#include "Subroutines.h"

//Caution Flash.c should be the first file in the files section under the project browser


void FlashInit ( void )
{
 	 FCLKDIV = (char)(OscClk * (5 + Tbus));
}

char FlashCmd[] = {0x7b,0x01,0x06,0xc6,0x80,0x7b,0x01,0x05,
	 			   0x1f,0x01,0x05,0x40,0xfb,0x3d};
//FlashCmd   7B0106         STAB    0106
//       	 C680           LDAB    #80
//       	 7B0105         STAB    0105
//       	 1F010540FB     BRCLR   0105 #40 1009
//       	 3D             RTS     




//  char TmpStr2[] = "Hello World!\r\n";
//  int *intptr;

//	intptr = (int*)0xc1f0;
//	FlashWrite ( sizeof (TmpStr2), TmpStr2, intptr );	


void FlashWrite ( int ArraySize, char WriteData[], int *WriteAddr )
{
 	 //Erases and Writes an array of up to 512 char to Flash
	 
	 int i, j, TmpData;
	 char *tempAddr;
	 char tempArray[1024];
	 
     
	 
	 //Add up to 511 more data bytes to beginning of array
	 //Addr must be divisible by 512 for erase operation
	 
	 tempAddr = (char*)WriteAddr;
     tempAddr = (char*)((int)tempAddr & ~0x1ff);
	 
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

	 //pad end of array to make divisible by 512

     if ( (j & 0x1ff) )
     {
	 	tempAddr = (char*)WriteAddr;
     	tempAddr = (char*)((int)tempAddr & ~0x1ff);
	  	for ( ;j & 0x1ff;j++ )
		{
         	tempArray[j] = *(tempAddr + j);
		}
     }

	 
	 //Erase Flash
	 
	 tempAddr = (char*)WriteAddr;
	 WriteAddr = (int*)((int)WriteAddr & ~0x1ff);

     for (i=0; i<j; i += 512, WriteAddr +=256 )
     {
		 *WriteAddr = TmpData;
		 asm ("ldab #0x40");
		 asm ("jsr _FlashCmd");
//		 FCMD = 0x40;
//		 FSTAT = FSTAT_CCIF;
//		 while (!(FSTAT & FSTAT_CCIF) );
	 }
	 

	 //Write to Flash
	 
	 WriteAddr = (int*)tempAddr;	 
	 WriteAddr = (int*)((int)WriteAddr & ~0x1ff);

     for (i=0;i<j;i+=2, WriteAddr++)
     {
	  	 TmpData = tempArray[i]*256 + tempArray[i+1];
		 *WriteAddr = TmpData;
		 asm ("ldab #0x20");
		 asm ("jsr _FlashCmd");
//		 FCMD = 0x20;
//		 FSTAT = FSTAT_CCIF;
//		 while (!(FSTAT & FSTAT_CCIF) );
     }


}
