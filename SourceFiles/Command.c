#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Controller.h"
#include "Command.h"
#include "mc9s12a128.h"
#include "EEProm.h"
#include "Interrupts.h"
#include "Subroutines.h"

extern char SIN0Buf[];
extern char SOUT1Buf[];
extern int SOUT1Bufptr;
extern char Gen_Flags;
unsigned int jmpAddress;
extern unsigned int Timer1;

extern UNSIGNED8 gProcImg[];

extern char SW1[];
extern char VariableFlag;

void command ( char CmdBuf[] )
{
    char ParseBuf[64] = "\0", TmpStr[64];
	int CmdBufptr;
	
	if ( CmdBuf == SIN0Buf )
	    printf ( "\r\n" );
			 
    ucase( CmdBuf );
	
	while( strlen( CmdBuf ) && CmdBuf[0] != '\r' )
    {

        parse (ParseBuf, CmdBuf);
        
        if( !( strncmp( ParseBuf, "HELP", 4 ) ) || !( strncmp( ParseBuf, "?", 1 ) ) )
        {
			printf ( "/--------------------------- HELP MENU -------------------------------\\\r\n" );
            printf ( "|RESET                                                                |\r\n" );
            printf ( "|LOAD                                                                 |\r\n" );			
            printf ( "|GO                                                                   |\r\n" );			
			printf ( "|REVISION                                                             |\r\n" );			
			printf ( "\\---------------------------------------------------------------------/\r\n" );
            ParseBuf[0]=NULL;
        }
        else if( !( strncmp( ParseBuf, "RESET", 5 ) ) || !( strncmp( ParseBuf, "#RESET", 6 ) ) )
        {
			 COPCTL = 0x01;				//enable COP 
			 while (1);					//wait for reset
		}
        else if( !( strncmp( ParseBuf, "LOAD", 4 ) ) )
        {
		 	 char tmpstr[8];
			 int ByteCnt, DataByte, i,j;
			 char *Address;
			 
			 while (1)
			 {
    			 Gen_Flags &= ~Gen_Flags_SIN0Rcvd;
    			 while ( !(Gen_Flags & Gen_Flags_SIN0Rcvd) );
    			 
                 if ( CmdBuf[0] == 'S' & CmdBuf[1] == '1')
                 {
                    tmpstr[0]= CmdBuf[2];
                    tmpstr[1]= CmdBuf[3];
                    tmpstr[2]= 0;
                    ByteCnt = hextoi ( tmpstr );
                    for ( i=0;i<4;i++ )
                    {
                        tmpstr[i]= CmdBuf[i+4];
                    }
                    tmpstr[4] = 0;
                    Address = (char*)hextoi ( tmpstr );
                    for ( j=8,i=3;i<ByteCnt;i++,j+=2 )
                    {
                        tmpstr[0]= CmdBuf[j];
                        tmpstr[1]= CmdBuf[j+1];
                        tmpstr[2]= 0;
                        *Address++ = (char)hextoi ( tmpstr );
                    }
                 }
    			 else if ( CmdBuf[0] == 'S' & CmdBuf[1] == '9' )
    			   	break;
			 }
			 ParseBuf[0]=NULL;
		}
        else if( !( strncmp( ParseBuf, "GO", 2 ) ) )
        {
			 parse( ParseBuf, CmdBuf );
			 
			 jmpAddress = (unsigned int)hextoi ( ParseBuf );
			 
			 asm ("pshx");
			 asm ("ldx _jmpAddress");
			 asm ("jsr 0,x");
			 asm ("pulx");
			 ParseBuf[0]=NULL;
		}
        else if( !( strncmp( ParseBuf, "REVISION", 8 ) ) )
        {
			printf (  "Revision %s\r\n", Revision );
            ParseBuf[0]=NULL;
		}
        if (strlen(ParseBuf))
        {
			//printf ( "Command not recognized\r\n" );
			CmdBuf[0] = '\0';
			ParseBuf[0]=NULL;
        }
    }		   
    //printf ( "\r\n>" );
    
}




char Checksum ( char *str )
{
 	char tmpstr[64], ParseBuf[5];
	char cksum = 0; 
    strcpy (tmpstr, str);
	while ( tmpstr[0] )
	{
	    parse( ParseBuf, tmpstr );
	    cksum += (unsigned int)hextoi ( ParseBuf );
	}
	return cksum;
	
}

void parse(char *pbuf, char *str)
{
	 char *ptr;
     ltrim (str);
     ptr = (char *)memchr (str, '\n', strlen(str));
	 if (ptr)
	 	*ptr = NULL;
     ptr = (char *)memchr (str, '\r', strlen(str));
	 if (ptr)
	 	*ptr = NULL;
     ptr = (char *)memchr (str, ' ', strlen(str));
     if (ptr)
     {
        *ptr = NULL;

        strcpy (pbuf, str);
        ptr++;
        strcpy (str, ptr);
        return;
      }
      else
      {
        strcpy (pbuf,str);
        *str = NULL;
        return;
      }
}

void ucase (char *str)
{
     while (*str)
     {
           if (*str >= 'a' && *str <= 'z')
              *str -= 0x20;
           str++;
     }
     return;
}

void ltrim (char *c_ptr)
{
     while ((c_ptr[0] == ' ') && (c_ptr[0] != NULL))
        strcpy (c_ptr, c_ptr+1);
}

int hextoi(char *string)
{
  int number = 0;
  int index;
 
  for (index = 0; string[index] != '\0'; index++)
    {
      char tmp = string[index];
      int digit;
 
      if (tmp >= '0' && tmp <= '9')
        digit = tmp - '0';
      else if (tmp >= 'A' && tmp <= 'F')
        digit = tmp - 'A' + 10;
      else if (tmp >= 'a' && tmp <= 'f')
        digit = tmp - 'a' + 10;
      else
        break;
 
      number = number * 16 + digit;
    }
  return(number);
}

