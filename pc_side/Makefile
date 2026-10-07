# =============================================================================
# Makefile — 12/48 Coater (Rev4.33) PC-SIDE host app (GCC, -DPC_SIDE)
#
# Builds a runnable Windows executable that runs the REAL firmware main loop
# (doevents()) on the PC with live logging and a UDP CAN bus. ImageCraft is not
# involved — this is the host compiler (TDM-GCC/MinGW-w64, same as tests/).
# The shared build lives in core/core.mk; this file holds only the unit's part.
#
# Rev4.33 is the pre-refactor layout: everything lives in Source_Files/ and there
# is NO dependency on the shared ../../../../Library tree — this unit carries its
# own mco.c, user.c and Interrupts.c.
#
# Deliberately NOT compiled (core/ supplies their entry points):
#   Controller.c  holds main() and the hardware bring-up; main.c mirrors its
#                 init order instead
#   EEProm.c      spins on command-complete (core emulates it on eeprom.bin)
#   Flash.c       same, for the flash writer
#   mcohw.c       spins on CAN transmit-buffer status bits that never set
#   Command.c     serial command console, reached only from the SCI ISRs
#
# Usage
# -----
#   mingw32-make        build pc_side_host.exe
#   mingw32-make run    build and run it (Ctrl+C to quit)
#   mingw32-make clean
#
# Bring the bus up first, in another terminal:
#   python can_hub_gui.py   (C:\Working_Projects\can_emulators)
# Defaults: this host binds :20010 and sends to the hub on :20100.
# =============================================================================

SRC      := ../Source_Files
FIRMWARE := Subroutines Subroutines1 mco user Interrupts

# -Ddiag pins HeadSpeed to 8000 in RTI_Int_Handler (Interrupts.c). The head
# tachometer is a timer input-capture on the target, so on the host HeadSpeed
# stays 0 and CkHeadRotation always times out into HeadErrorState — the coating
# sequence could never get past step 2.
UNIT_DEF := -Ddiag

ifeq ($(wildcard core/core.mk),)
$(error pc_side/core is empty - run: git submodule update --init)
endif
include core/core.mk
