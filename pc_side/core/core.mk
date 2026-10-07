# =============================================================================
# core.mk — shared build for a unit's PC-side host (GCC, -DPC_SIDE).
#
# The unit Makefile sets these, then includes this file:
#   SRC        the unit's firmware source folder
#   FIRMWARE   firmware .c basenames to compile from SRC
#   UNIT_DEF   extra -D flags for this unit (optional)
#   UNIT_INC   extra -I paths, e.g. a library holding mc9s12a128.h (optional)
#   EXE        output name (optional, default pc_side_host.exe)
# The unit's own host code is main.c, next to its Makefile.
#
# Targets: all (default), run, clean.
# =============================================================================

CORE_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

CC    := gcc
SHELL := cmd.exe

# The first rule below is the $(BUILD) directory, which would otherwise become
# the default goal and a bare `mingw32-make` would only create a folder.
.DEFAULT_GOAL := all

EXE   ?= pc_side_host.exe
BUILD := build

# PC_SIDE selects the host paths in the real headers (SFR array, no ISR pragmas,
# no absolute-address vector table). PC_EEPROM points EE_begin at the host
# EEPROM image, persisted to eeprom.bin (host_eeprom.c).
PC_DEF := -DPC_SIDE -DPC_EEPROM $(UNIT_DEF)

# Force-include pc_side.h so INTR_ON/OFF are mapped before hc12def.h/Controller.h
# /mcohw.h can define them to cli/sei.
FINC := -include $(CORE_DIR)/pc_side.h

INCS := -I. -I$(CORE_DIR) -I$(SRC) $(UNIT_INC)

# Production TUs are read-only here: silence their warnings. -funsigned-char
# matches the ICC12 default char signedness — without it any `if (x == 0xFF)` on
# a raw byte silently never matches. -MMD -MP emit per-object header deps so
# editing a header rebuilds what includes it.
# -malign-data=abi / no -fdata-sections: menu code reaches adjacent globals by
# pointer arithmetic off the end of an array, which only holds while GCC lays
# the arrays out contiguously in declaration order, the way ICC12 does.
# -fpermissive allows legacy C89/C90 constructs (e.g., tempstr[0]=NULL) on GCC 14+.
PROD_CFLAGS := -w -g -std=c11 -m32 -fpermissive -funsigned-char -malign-data=abi -ffunction-sections \
               -MMD -MP -Wno-unknown-pragmas -Wno-builtin-declaration-mismatch $(PC_DEF)

# Host TUs (core + the unit's main.c): keep real warnings on.
HOST_CFLAGS := -g -std=c11 -m32 -funsigned-char -malign-data=abi -ffunction-sections \
               -MMD -MP -Wall -Wno-unknown-pragmas $(PC_DEF)

LDFLAGS := -m32 -Wl,--gc-sections
LIBS    := -lws2_32 -lm          # winsock (UDP CAN) + math

FIRMWARE_OBJS := $(patsubst %,$(BUILD)/%.o,$(FIRMWARE))
CORE_OBJS     := $(patsubst $(CORE_DIR)/%.c,$(BUILD)/%.o,$(wildcard $(CORE_DIR)/*.c))
OBJS          := $(FIRMWARE_OBJS) $(CORE_OBJS) $(BUILD)/main.o

# Objects also depend on the makefiles: the -D flags live there, so editing them
# must force a rebuild.
MKFILES := $(firstword $(MAKEFILE_LIST)) $(CORE_DIR)/core.mk

# Firmware sources that must exist under $(SRC). A missing one otherwise
# surfaces as make demanding a path nothing generates, which reads like a broken
# rule rather than a wrong SRC.
MISSING_SRCS := $(filter-out $(wildcard $(SRC)/*.c),$(patsubst %,$(SRC)/%.c,$(FIRMWARE)))

$(BUILD):												# the build/ folder
	-mkdir $(BUILD)

$(BUILD)/%.o: $(SRC)/%.c $(MKFILES) | $(BUILD) 			# firmware .c  → .o
	$(CC) $(PROD_CFLAGS) $(INCS) $(FINC) -c -o $@ $<

$(BUILD)/%.o: $(CORE_DIR)/%.c $(MKFILES) | $(BUILD)		# core/*.c  → .o
	$(CC) $(HOST_CFLAGS) $(INCS) $(FINC) -c -o $@ $<

$(BUILD)/%.o: %.c $(MKFILES) | $(BUILD)					# unit main.c  → .o
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
	@echo *** Check the SRC line in the unit Makefile against the real folder name.
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
