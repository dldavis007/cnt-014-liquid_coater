CC = icc12w
LIB = ilibw
CFLAGS =  -IC:\iccv712\include\ -I..\pc_side -e -D__ICC_VERSION=708 -D__BUILD=1604  -l -g -Wa-g -Wf-cpdon 
ASFLAGS = $(CFLAGS) 
LFLAGS =  -LC:\iccv712\lib\ -g -nb:1604 -ucrt12initrm.o -btext:0x4000.0x7FFF:0xC000.0xFFFF -bdata:0x2000 -bextcode:0xE0000.0xFFFFF -dinit_sp:0x4000 -fmots19 -dinitrm:0x21 -bidata:0xd000
FILES = Controller.o EEProm.o mco.o user.o mcohw.o Interrupts.o Subroutines.o Subroutines1.o 

HEAD:	$(FILES)
	$(CC) -o HEAD $(LFLAGS) @HEAD.lk   -lfp12p -lfp12 -lc12p
Controller.o: C:\iccv712\include\stdio.h C:\iccv712\include\stdarg.h C:\iccv712\include\_const.h .\..\Source_Files\Controller.h .\..\Source_Files\mc9s12a128.h .\..\Source_Files\EEProm.h .\..\Source_Files\Subroutines.h .\..\Source_Files\Subroutines1.h .\..\Source_Files\mco.h .\..\Source_Files\nodecfg.h .\..\Source_Files\procimg.h
Controller.o:	..\Source_Files\Controller.c
	$(CC) -c $(CFLAGS) ..\Source_Files\Controller.c
EEProm.o: .\..\Source_Files\EEProm.h .\..\Source_Files\mc9s12a128.h .\..\Source_Files\Subroutines.h
EEProm.o:	..\Source_Files\EEProm.c
	$(CC) -c $(CFLAGS) ..\Source_Files\EEProm.c
mco.o: C:\iccv712\include\string.h C:\iccv712\include\_const.h .\..\Source_Files\Controller.h .\..\Source_Files\Interrupts.h .\..\Source_Files\Subroutines.h .\..\Source_Files\mco.h .\..\Source_Files\nodecfg.h .\..\Source_Files\procimg.h .\..\Source_Files\mcohw.h .\..\Source_Files\mc9s12a128.h
mco.o:	..\Source_Files\mco.c
	$(CC) -c $(CFLAGS) ..\Source_Files\mco.c
user.o: C:\iccv712\include\string.h C:\iccv712\include\_const.h .\..\Source_Files\Controller.h .\..\Source_Files\mco.h .\..\Source_Files\nodecfg.h .\..\Source_Files\procimg.h .\..\Source_Files\mcohw.h .\..\Source_Files\mc9s12a128.h .\..\Source_Files\subroutines.h
user.o:	..\Source_Files\user.c
	$(CC) -c $(CFLAGS) ..\Source_Files\user.c
mcohw.o: .\..\Source_Files\Controller.h .\..\Source_Files\mc9s12a128.h .\..\Source_Files\mcohw.h .\..\Source_Files\mco.h .\..\Source_Files\nodecfg.h .\..\Source_Files\procimg.h .\..\Source_Files\Interrupts.h .\..\Source_Files\Subroutines.h .\..\..\..\iccv712\include\stdarg.h
mcohw.o:	..\Source_Files\mcohw.c
	$(CC) -c $(CFLAGS) ..\Source_Files\mcohw.c
Interrupts.o: .\..\Source_Files\Interrupts.h .\..\Source_Files\mc9s12a128.h .\..\Source_Files\Controller.h .\..\Source_Files\mco.h .\..\Source_Files\nodecfg.h .\..\Source_Files\procimg.h .\..\Source_Files\mcohw.h .\..\Source_Files\Subroutines.h C:\iccv712\include\stdlib.h C:\iccv712\include\_const.h C:\iccv712\include\limits.h C:\iccv712\include\math.h .\..\Source_Files\vectors.h
Interrupts.o:	..\Source_Files\Interrupts.c
	$(CC) -c $(CFLAGS) ..\Source_Files\Interrupts.c
Subroutines.o: C:\iccv712\include\stdio.h C:\iccv712\include\stdarg.h C:\iccv712\include\_const.h C:\iccv712\include\string.h C:\iccv712\include\stdlib.h C:\iccv712\include\limits.h .\..\Source_Files\Subroutines.h .\..\Source_Files\Subroutines1.h .\..\Source_Files\mc9s12a128.h .\..\Source_Files\Interrupts.h .\..\Source_Files\mcohw.h .\..\Source_Files\mco.h .\..\Source_Files\nodecfg.h .\..\Source_Files\procimg.h .\..\Source_Files\Controller.h .\..\Source_Files\EEProm.h
Subroutines.o:	..\Source_Files\Subroutines.c
	$(CC) -c $(CFLAGS) ..\Source_Files\Subroutines.c
Subroutines1.o: C:\iccv712\include\stdio.h C:\iccv712\include\stdarg.h C:\iccv712\include\_const.h C:\iccv712\include\string.h C:\iccv712\include\stdlib.h C:\iccv712\include\limits.h C:\iccv712\include\math.h .\..\Source_Files\Subroutines.h .\..\Source_Files\Subroutines1.h .\..\Source_Files\mc9s12a128.h .\..\Source_Files\Interrupts.h .\..\Source_Files\mcohw.h .\..\Source_Files\mco.h .\..\Source_Files\nodecfg.h .\..\Source_Files\procimg.h .\..\Source_Files\Controller.h .\..\Source_Files\EEProm.h .\..\pc_side\pc_log.h
Subroutines1.o:	..\Source_Files\Subroutines1.c
	$(CC) -c $(CFLAGS) ..\Source_Files\Subroutines1.c
