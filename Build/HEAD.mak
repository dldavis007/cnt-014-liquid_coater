CC = icc12w
LIB = ilibw
CFLAGS =  -IC:\iccv712\include\ -e -D__ICC_VERSION=708 -D__BUILD=1603  -l -g -Wa-g -Wf-cpdon 
ASFLAGS = $(CFLAGS) 
LFLAGS =  -LC:\iccv712\lib\ -g -nb:1603 -ucrt12initrm.o -btext:0x4000.0x7FFF:0xC000.0xFFFF -bdata:0x2000 -bextcode:0xE0000.0xFFFFF -dinit_sp:0x4000 -fmots19 -dinitrm:0x21 -bidata:0xd000
FILES = Controller.o EEProm.o mco.o user.o mcohw.o Interrupts.o Subroutines.o Subroutines1.o 

HEAD:	$(FILES)
	$(CC) -o HEAD $(LFLAGS) @HEAD.lk   -lfp12p -lfp12 -lc12p
Controller.o: C:\iccv712\include\stdio.h C:\iccv712\include\stdarg.h C:\iccv712\include\_const.h .\..\..\REV4~1.33\Source_Files\Controller.h .\..\..\REV4~1.33\Source_Files\mc9s12a128.h .\..\..\REV4~1.33\Source_Files\EEProm.h .\..\..\REV4~1.33\Source_Files\Subroutines.h .\..\..\REV4~1.33\Source_Files\Subroutines1.h .\..\..\REV4~1.33\Source_Files\mco.h .\..\..\REV4~1.33\Source_Files\nodecfg.h .\..\..\REV4~1.33\Source_Files\procimg.h
Controller.o:	..\..\REV4~1.33\Source_Files\Controller.c
	$(CC) -c $(CFLAGS) ..\..\REV4~1.33\Source_Files\Controller.c
EEProm.o: .\..\..\REV4~1.33\Source_Files\EEProm.h .\..\..\REV4~1.33\Source_Files\mc9s12a128.h .\..\..\REV4~1.33\Source_Files\Subroutines.h
EEProm.o:	..\..\REV4~1.33\Source_Files\EEProm.c
	$(CC) -c $(CFLAGS) ..\..\REV4~1.33\Source_Files\EEProm.c
mco.o: C:\iccv712\include\string.h C:\iccv712\include\_const.h .\..\..\REV4~1.33\Source_Files\Controller.h .\..\..\REV4~1.33\Source_Files\Interrupts.h .\..\..\REV4~1.33\Source_Files\Subroutines.h .\..\..\REV4~1.33\Source_Files\mco.h .\..\..\REV4~1.33\Source_Files\nodecfg.h .\..\..\REV4~1.33\Source_Files\procimg.h .\..\..\REV4~1.33\Source_Files\mcohw.h .\..\..\REV4~1.33\Source_Files\mc9s12a128.h
mco.o:	..\..\REV4~1.33\Source_Files\mco.c
	$(CC) -c $(CFLAGS) ..\..\REV4~1.33\Source_Files\mco.c
user.o: C:\iccv712\include\string.h C:\iccv712\include\_const.h .\..\..\REV4~1.33\Source_Files\Controller.h .\..\..\REV4~1.33\Source_Files\mco.h .\..\..\REV4~1.33\Source_Files\nodecfg.h .\..\..\REV4~1.33\Source_Files\procimg.h .\..\..\REV4~1.33\Source_Files\mcohw.h .\..\..\REV4~1.33\Source_Files\mc9s12a128.h .\..\..\REV4~1.33\Source_Files\subroutines.h
user.o:	..\..\REV4~1.33\Source_Files\user.c
	$(CC) -c $(CFLAGS) ..\..\REV4~1.33\Source_Files\user.c
mcohw.o: .\..\..\REV4~1.33\Source_Files\Controller.h .\..\..\REV4~1.33\Source_Files\mc9s12a128.h .\..\..\REV4~1.33\Source_Files\mcohw.h .\..\..\REV4~1.33\Source_Files\mco.h .\..\..\REV4~1.33\Source_Files\nodecfg.h .\..\..\REV4~1.33\Source_Files\procimg.h .\..\..\REV4~1.33\Source_Files\Interrupts.h .\..\..\REV4~1.33\Source_Files\Subroutines.h .\..\..\..\..\iccv712\include\stdarg.h
mcohw.o:	..\..\REV4~1.33\Source_Files\mcohw.c
	$(CC) -c $(CFLAGS) ..\..\REV4~1.33\Source_Files\mcohw.c
Interrupts.o: .\..\..\REV4~1.33\Source_Files\Interrupts.h .\..\..\REV4~1.33\Source_Files\mc9s12a128.h .\..\..\REV4~1.33\Source_Files\Controller.h .\..\..\REV4~1.33\Source_Files\mco.h .\..\..\REV4~1.33\Source_Files\nodecfg.h .\..\..\REV4~1.33\Source_Files\procimg.h .\..\..\REV4~1.33\Source_Files\mcohw.h .\..\..\REV4~1.33\Source_Files\Subroutines.h C:\iccv712\include\stdlib.h C:\iccv712\include\_const.h C:\iccv712\include\limits.h C:\iccv712\include\math.h .\..\..\REV4~1.33\Source_Files\vectors.h
Interrupts.o:	..\..\REV4~1.33\Source_Files\Interrupts.c
	$(CC) -c $(CFLAGS) ..\..\REV4~1.33\Source_Files\Interrupts.c
Subroutines.o: C:\iccv712\include\stdio.h C:\iccv712\include\stdarg.h C:\iccv712\include\_const.h C:\iccv712\include\string.h C:\iccv712\include\stdlib.h C:\iccv712\include\limits.h .\..\..\REV4~1.33\Source_Files\Subroutines.h .\..\..\REV4~1.33\Source_Files\Subroutines1.h .\..\..\REV4~1.33\Source_Files\mc9s12a128.h .\..\..\REV4~1.33\Source_Files\Interrupts.h .\..\..\REV4~1.33\Source_Files\mcohw.h .\..\..\REV4~1.33\Source_Files\mco.h .\..\..\REV4~1.33\Source_Files\nodecfg.h .\..\..\REV4~1.33\Source_Files\procimg.h .\..\..\REV4~1.33\Source_Files\Controller.h .\..\..\REV4~1.33\Source_Files\EEProm.h
Subroutines.o:	..\..\REV4~1.33\Source_Files\Subroutines.c
	$(CC) -c $(CFLAGS) ..\..\REV4~1.33\Source_Files\Subroutines.c
Subroutines1.o: C:\iccv712\include\stdio.h C:\iccv712\include\stdarg.h C:\iccv712\include\_const.h C:\iccv712\include\string.h C:\iccv712\include\stdlib.h C:\iccv712\include\limits.h C:\iccv712\include\math.h .\..\..\REV4~1.33\Source_Files\Subroutines.h .\..\..\REV4~1.33\Source_Files\Subroutines1.h .\..\..\REV4~1.33\Source_Files\mc9s12a128.h .\..\..\REV4~1.33\Source_Files\Interrupts.h .\..\..\REV4~1.33\Source_Files\mcohw.h .\..\..\REV4~1.33\Source_Files\mco.h .\..\..\REV4~1.33\Source_Files\nodecfg.h .\..\..\REV4~1.33\Source_Files\procimg.h .\..\..\REV4~1.33\Source_Files\Controller.h .\..\..\REV4~1.33\Source_Files\EEProm.h
Subroutines1.o:	..\..\REV4~1.33\Source_Files\Subroutines1.c
	$(CC) -c $(CFLAGS) ..\..\REV4~1.33\Source_Files\Subroutines1.c
