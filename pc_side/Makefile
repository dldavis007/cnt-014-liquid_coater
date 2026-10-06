# =============================================================================
# Makefile — 12/48 Coater (Rev4.33) PC-SIDE host app (GCC, -DPC_SIDE)
#
# Builds a runnable Windows executable that runs the REAL firmware main loop
# (doevents()) on the PC with live logging and a UDP CAN bus. ImageCraft is not
# involved — this is the host compiler (TDM-GCC/MinGW-w64, same as tests/).
#
# Rev4.33 vs Rev4.34
# ------------------
# Rev4.33 is the pre-refactor layout: everything lives in Source_Files/ and there
# is NO dependency on the shared ../../../../Library tree — this unit carries its
# own mco.c, user.c and Interrupts.c. So unlike Rev4.34's Makefile there is no
# LIBDRV/LIBINC and no -idirafter: one include path covers it, and the
# ImageCraft C library headers that forced -idirafter over there are not on it.
#
# Deliberately NOT compiled:
#   Controller.c  holds main() and the hardware bring-up; pc_side/main.c is the
#                 host's entry point instead (and mirrors its init order).
#   EEProm.c      spins on command-complete (pc_side_host.c emulates it on a file)
#   Flash.c       same, for the flash writer
#   mcohw.c       spins on CAN transmit-buffer status bits that never set
#   Command.c     serial command console, reached only from the SCI ISRs
# pc_side_host.c supplies those entry points plus the CAN layer.
#
# Usage
# -----
#   mingw32-make        build pc_side_host.exe
#   mingw32-make run    build and run it (Ctrl+C to quit)
#   mingw32-make clean
#
# Bring the bus up first, in another terminal:
#   python ../../../../CAN_RECEIVER_CODE/can_udp_hub.py
# then any emulators you want on it (emulators/coater_la_purge_emulator.py,
# emulators/button_box_emulator.py, emulators/can_hub_gui.py).
# Defaults: this host binds :20010 and sends to the hub on :20100.
# =============================================================================

CC    := gcc
SHELL := cmd.exe

# The first rule below is the $(BUILD) directory, which would otherwise become
# the default goal and a bare `mingw32-make` would only create a folder.
.DEFAULT_GOAL := all

SRC := ../Source_Files

# PC_SIDE selects the host paths in the real headers (SFR array, no ISR pragmas,
# no absolute-address vector table). PC_EEPROM points EE_begin at a host EEPROM
# image, persisted to eeprom.bin (pc_side_host.c).
# -Ddiag pins HeadSpeed to 8000 in RTI_Int_Handler (Interrupts.c). The head
# tachometer is a timer input-capture on the target, so on the host HeadSpeed
# stays 0 and CkHeadRotation always times out into HeadErrorState — the coating
# sequence could never get past step 2.
PC_DEF := -DPC_SIDE -DPC_EEPROM -Ddiag

# Force-include pc_side.h so INTR_ON/OFF are mapped before hc12def.h/Controller.h
# /mcohw.h can define them to cli/sei.
FINC := -include pc_side.h

INCS := -I. -I$(SRC)

# Production TUs are read-only here: silence their warnings. -funsigned-char
# matches the ICC12 default char signedness — without it any `if (x == 0xFF)` on
# a raw byte silently never matches. -MMD -MP emit per-object header deps so
# editing a header rebuilds what includes it.
# -malign-data=abi / no -fdata-sections: Rev4.33's menu code reaches adjacent
# globals by pointer arithmetic off the end of an array (the same stroke-table
# trick Rev4.34 documents), which only holds while GCC lays the arrays out
# contiguously in declaration order, the way ICC12 does.
# -fpermissive allows legacy C89/C90 constructs (e.g., tempstr[0]=NULL) on GCC 14+.
PROD_CFLAGS := -w -g -std=c11 -m32 -fpermissive -funsigned-char -malign-data=abi -ffunction-sections \
               -MMD -MP -Wno-unknown-pragmas -Wno-builtin-declaration-mismatch $(PC_DEF)
			   
# Our host TUs: keep real warnings on.
HOST_CFLAGS := -g -std=c11 -m32 -funsigned-char -malign-data=abi -ffunction-sections \
               -MMD -MP -Wall -Wno-unknown-pragmas $(PC_DEF)

LDFLAGS := -m32 -Wl,--gc-sections
LIBS    := -lws2_32 -lm          # winsock (UDP CAN) + math

BUILD := build

UNIT_OBJS := $(BUILD)/Subroutines.o $(BUILD)/Subroutines1.o $(BUILD)/mco.o \
             $(BUILD)/user.o $(BUILD)/Interrupts.o
HOST_OBJS := $(BUILD)/pc_side_host.o $(BUILD)/main.o
OBJS      := $(UNIT_OBJS) $(HOST_OBJS)
EXE       := pc_side_host.exe

# Objects also depend on this Makefile: the -D flags (PC_SIDE, PC_EEPROM,
# diag) live here, so editing them must force a rebuild. Without this, changing a
# define silently leaves every object compiled against the old flags.
MKFILE := $(firstword $(MAKEFILE_LIST))

# Unit sources that must exist under $(SRC). A missing one otherwise surfaces as
# make demanding a path nothing generates, which reads like a broken rule rather
# than a wrong SRC.
UNIT_SRCS    := $(patsubst $(BUILD)/%.o,$(SRC)/%.c,$(UNIT_OBJS))
MISSING_SRCS := $(filter-out $(wildcard $(SRC)/*.c),$(UNIT_SRCS))

$(BUILD):												# the build/ folder
	-mkdir $(BUILD)

$(BUILD)/%.o: $(SRC)/%.c $(MKFILE) | $(BUILD) 			# production .c  → .o
	$(CC) $(PROD_CFLAGS) $(INCS) $(FINC) -c -o $@ $<

$(BUILD)/%.o: %.c $(MKFILE) | $(BUILD)					# pc_side/*.c  → .o
	$(CC) $(HOST_CFLAGS) $(INCS) $(FINC) -c -o $@ $<

$(EXE): $(OBJS)											# .o files  → exe
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

.PHONY: all run clean check-sources stop-if-running retry-relink
all: check-sources $(EXE)

# Fail with the actual reason before make can report it as a missing rule.
check-sources:
ifneq ($(MISSING_SRCS),)
	@echo.
	@echo *** Cannot find these sources under SRC = $(SRC)
	@echo ***   $(MISSING_SRCS)
	@echo ***
	@echo *** Check the SRC line in this Makefile against the real folder name.
	@echo *** If you renamed or moved that folder, also run: mingw32-make clean
	@echo *** - build\*.d caches the old paths and make will keep demanding them.
	@echo.
	@exit 1
endif

# Build FIRST, and only stop a running host if the relink actually needs the
# file (a live .exe locks it). The stop path is reached only after a failed
# build, and stop-if-running decides whether a lock was really the cause.
run: check-sources
	@$(MAKE) --no-print-directory $(EXE) || $(MAKE) --no-print-directory retry-relink
	.\$(EXE)

# Reached only when the build failed. If no host is running the .exe was never
# locked, so the failure was a real compiler/linker error: say so and stop,
# leaving that error as the last thing on screen.
stop-if-running:
	@tasklist /FI "IMAGENAME eq $(EXE)" 2>nul | findstr /I /C:"$(EXE)" >nul || ( \
	  echo. && \
	  echo *** BUILD FAILED - and no $(EXE) was running, so this was NOT a file lock. && \
	  echo *** The compiler or linker error above is the real one. && \
	  echo. && \
	  exit 1 )
	@echo *** relink blocked: a $(EXE) is already running and holds the file open.
	@echo *** Stopping it now - that terminal will exit - then relinking here.
	@taskkill /F /IM $(EXE) >nul

retry-relink: stop-if-running
	@$(MAKE) --no-print-directory $(EXE)

clean:
	-rmdir /s /q $(BUILD) 2>nul
	-del /f /q $(EXE) 2>nul

-include $(wildcard $(BUILD)/*.d)
