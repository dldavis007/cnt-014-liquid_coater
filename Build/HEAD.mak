CC = icc12w
LIB = ilibw
CFLAGS =  -IC:\iccv712\include\ -e -D__ICC_VERSION=708 -D__BUILD=1610  -l -g -Wa-g -Wf-cpdon 
ASFLAGS = $(CFLAGS) 
LFLAGS =  -LC:\iccv712\lib\ -g -nb:1610 -ucrt12initrm.o -btext:0x4000.0x7FFF:0xC000.0xFFFF -bdata:0x1000 -bextcode:0xE0000.0xFFFFF -dinit_sp:0x4000 -fmots19 -dinitrm:0x21 -bidata:0xd000
FILES = Controller.o EEProm.o mco.o user.o mcohw.o Interrupts.o Subroutines.o Subroutines1.o MenuSerialize.o Packets.o 

HEAD:	$(FILES)
	$(CC) -o HEAD $(LFLAGS) @HEAD.lk   -lfp12p -lfp12 -lc12p
Controller.o: C:\iccv712\include\stdio.h C:\iccv712\include\stdarg.h C:\iccv712\include\_const.h .\..\SourceFiles\Controller.h .\..\SourceFiles\mc9s12a128.h .\..\SourceFiles\EEProm.h .\..\SourceFiles\Subroutines.h .\..\SourceFiles\Subroutines1.h .\..\SourceFiles\mco.h .\..\SourceFiles\nodecfg.h .\..\SourceFiles\procimg.h
Controller.o:	..\SourceFiles\Controller.c
	$(CC) -c $(CFLAGS) ..\SourceFiles\Controller.c
EEProm.o: .\..\SourceFiles\EEProm.h .\..\SourceFiles\mc9s12a128.h .\..\SourceFiles\Subroutines.h
EEProm.o:	..\SourceFiles\EEProm.c
	$(CC) -c $(CFLAGS) ..\SourceFiles\EEProm.c
mco.o: C:\iccv712\include\string.h C:\iccv712\include\_const.h .\..\SourceFiles\Controller.h .\..\SourceFiles\Interrupts.h .\..\SourceFiles\Subroutines.h .\..\SourceFiles\mco.h .\..\SourceFiles\nodecfg.h .\..\SourceFiles\procimg.h .\..\SourceFiles\mcohw.h .\..\SourceFiles\mc9s12a128.h .\..\SourceFiles\Packets.h C:\iccv712\include\stddef.h C:\iccv712\include\stdio.h C:\iccv712\include\stdarg.h .\..\SourceFiles\MenuSerialize.h C:\iccv712\include\ctype.h
mco.o:	..\SourceFiles\mco.c
	$(CC) -c $(CFLAGS) ..\SourceFiles\mco.c
user.o: C:\iccv712\include\string.h C:\iccv712\include\_const.h .\..\SourceFiles\Controller.h .\..\SourceFiles\mco.h .\..\SourceFiles\nodecfg.h .\..\SourceFiles\procimg.h .\..\SourceFiles\mcohw.h .\..\SourceFiles\mc9s12a128.h .\..\SourceFiles\subroutines.h
user.o:	..\SourceFiles\user.c
	$(CC) -c $(CFLAGS) ..\SourceFiles\user.c
mcohw.o: .\..\SourceFiles\Controller.h .\..\SourceFiles\mc9s12a128.h .\..\SourceFiles\mcohw.h .\..\SourceFiles\mco.h .\..\SourceFiles\nodecfg.h .\..\SourceFiles\procimg.h .\..\SourceFiles\Interrupts.h .\..\SourceFiles\Subroutines.h .\..\..\..\..\..\iccv712\include\stdarg.h
mcohw.o:	..\SourceFiles\mcohw.c
	$(CC) -c $(CFLAGS) ..\SourceFiles\mcohw.c
Interrupts.o: .\..\SourceFiles\Interrupts.h .\..\SourceFiles\mc9s12a128.h .\..\SourceFiles\Controller.h .\..\SourceFiles\mco.h .\..\SourceFiles\nodecfg.h .\..\SourceFiles\procimg.h .\..\SourceFiles\mcohw.h .\..\SourceFiles\Subroutines.h C:\iccv712\include\stdlib.h C:\iccv712\include\_const.h C:\iccv712\include\limits.h C:\iccv712\include\math.h .\..\SourceFiles\vectors.h
Interrupts.o:	..\SourceFiles\Interrupts.c
	$(CC) -c $(CFLAGS) ..\SourceFiles\Interrupts.c
Subroutines.o: C:\iccv712\include\stdio.h C:\iccv712\include\stdarg.h C:\iccv712\include\_const.h C:\iccv712\include\string.h C:\iccv712\include\stdlib.h C:\iccv712\include\limits.h .\..\SourceFiles\Subroutines.h .\..\SourceFiles\Subroutines1.h .\..\SourceFiles\mc9s12a128.h .\..\SourceFiles\Interrupts.h .\..\SourceFiles\mcohw.h .\..\SourceFiles\mco.h .\..\SourceFiles\nodecfg.h .\..\SourceFiles\procimg.h .\..\SourceFiles\Controller.h .\..\SourceFiles\EEProm.h .\..\SourceFiles\Packets.h C:\iccv712\include\stddef.h .\..\SourceFiles\MenuSerialize.h C:\iccv712\include\ctype.h
Subroutines.o:	..\SourceFiles\Subroutines.c
	$(CC) -c $(CFLAGS) ..\SourceFiles\Subroutines.c
Subroutines1.o: C:\iccv712\include\stdio.h C:\iccv712\include\stdarg.h C:\iccv712\include\_const.h C:\iccv712\include\string.h C:\iccv712\include\stdlib.h C:\iccv712\include\limits.h C:\iccv712\include\math.h .\..\SourceFiles\Subroutines.h .\..\SourceFiles\Subroutines1.h .\..\SourceFiles\mc9s12a128.h .\..\SourceFiles\Interrupts.h .\..\SourceFiles\mcohw.h .\..\SourceFiles\mco.h .\..\SourceFiles\nodecfg.h .\..\SourceFiles\procimg.h .\..\SourceFiles\Controller.h .\..\SourceFiles\EEProm.h .\..\SourceFiles\Packets.h C:\iccv712\include\stddef.h .\..\SourceFiles\MenuSerialize.h C:\iccv712\include\ctype.h
Subroutines1.o:	..\SourceFiles\Subroutines1.c
	$(CC) -c $(CFLAGS) ..\SourceFiles\Subroutines1.c
MenuSerialize.o: .\..\SourceFiles\MenuSerialize.h C:\iccv712\include\stdio.h C:\iccv712\include\stdarg.h C:\iccv712\include\_const.h C:\iccv712\include\string.h C:\iccv712\include\ctype.h C:\iccv712\include\stddef.h .\..\SourceFiles\Subroutines.h .\..\SourceFiles\Packets.h .\..\SourceFiles\mc9s12a128.h .\..\SourceFiles\Interrupts.h .\..\SourceFiles\mcohw.h .\..\SourceFiles\mco.h .\..\SourceFiles\nodecfg.h .\..\SourceFiles\procimg.h .\..\SourceFiles\Controller.h
MenuSerialize.o:	..\SourceFiles\MenuSerialize.c
	$(CC) -c $(CFLAGS) ..\SourceFiles\MenuSerialize.c
Packets.o: .\..\SourceFiles\Packets.h C:\iccv712\include\stddef.h C:\iccv712\include\stdio.h C:\iccv712\include\stdarg.h C:\iccv712\include\_const.h .\..\SourceFiles\mc9s12a128.h .\..\SourceFiles\Interrupts.h .\..\SourceFiles\mcohw.h .\..\SourceFiles\mco.h .\..\SourceFiles\nodecfg.h .\..\SourceFiles\procimg.h .\..\SourceFiles\Controller.h .\..\SourceFiles\Subroutines.h .\..\SourceFiles\MenuSerialize.h C:\iccv712\include\string.h C:\iccv712\include\ctype.h
Packets.o:	..\SourceFiles\Packets.c
	$(CC) -c $(CFLAGS) ..\SourceFiles\Packets.c
