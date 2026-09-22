# =============================================================================
# Makefile — 12/48 Coater (Rev4.33) host-side unit tests (Unity)
#
# Prerequisites
# -------------
#   TDM-GCC / MinGW-w64 gcc on PATH (verified with tdm64 gcc 10.3.0)
#   mingw32-make (ships with TDM-GCC)
#
# Usage
# -----
#   mingw32-make all          build and run every suite
#   mingw32-make test_purge_retract   build and run one suite
#   mingw32-make summary      run all via run_all.py, aggregate report
#   mingw32-make check        gated: first failing suite fails the make
#   mingw32-make clean
#
# How the host build works
# ------------------------
# Production sources compile DIRECTLY — nothing is copied or modified, and
# there are no shadow headers. With -DPC_SIDE the real headers select their
# host paths: _REG_BASE points at sfr_regs[] (mc9s12a128.h), and the ICC12
# `#pragma interrupt_handler` block and the absolute-address vector table drop
# out (Interrupts.h / vectors.h). The one force-include is pc_side.h, which
# maps INTR_ON/OFF (cli/sei) onto a flag before hc12def.h/Controller.h/mcohw.h
# can define them.
#
# Rev4.33 vs Rev4.34
# ------------------
# Rev4.33 is the pre-refactor layout: everything lives in SourceFiles/ and there
# is NO dependency on the shared ../../../../Library tree — this unit carries its
# own mco.c, user.c, Interrupts.c, Packets.c and MenuSerialize.c. So unlike
# Rev4.34's tests/Makefile there is no LIBDRV/LIBINC split and no -idirafter:
# one include path covers it, and the ImageCraft C library headers that forced
# -idirafter over there are not on it.
#
# Deliberately NOT compiled:
#   Controller.c  holds main() and the hardware bring-up; hardware_stubs.c's
#                 host_firmware_init() mirrors its init order instead
#   EEProm.c      absolute-address EEPROM access + spins on command-complete
#   Flash.c       same, for the flash writer
#   mcohw.c       spins on CAN transmit-buffer status bits that never set
#   Command.c     serial command console, reached only from the SCI ISRs
# stubs/hardware_stubs.c supplies their entry points plus the CAN layer.
# =============================================================================

CC      := gcc

# The $(BUILD) directory rule comes first below and would otherwise become the
# default goal, so a bare `mingw32-make` would only create a folder.
.DEFAULT_GOAL := all

# Pin the recipe shell to cmd.exe: mingw32-make otherwise picks sh.exe if it
# happens to be on PATH, and the two disagree on launching a local program
# (cmd wants .\prog.exe, sh wants ./prog.exe). The clean recipe needs cmd's del.
SHELL   := cmd.exe

SRC     := ../SourceFiles

# Warnings off on production TUs: that code is read-only here and its warnings
# are noise we cannot act on. -funsigned-char matches the ICC12 default char
# signedness — without it any `if (x == 0xFF)` on a raw byte silently never
# matches. -MMD -MP emit per-object header deps so editing a HEADER rebuilds.
# -malign-data=abi / no -fdata-sections: the menu code reaches adjacent globals
# by pointer arithmetic off the end of an array, which only holds while GCC lays
# the arrays out contiguously in declaration order, the way ICC12 does.
# -fdata-sections would give each array its own section and let the linker
# reorder them; GCC's default alignment would pad between them.
PROD_CFLAGS := -w -g -std=c11 -m32 -funsigned-char -malign-data=abi -ffunction-sections \
               -MMD -MP -Wno-unknown-pragmas -Wno-builtin-declaration-mismatch \
               -DPC_SIDE -DSKIP_EEPROM_LOAD -Ddiag -DUNITY_INCLUDE_DOUBLE

# Test code is ours — keep real warnings on.
TEST_CFLAGS := -Wall -Wno-unknown-pragmas -g -std=c11 -m32 -funsigned-char \
               -malign-data=abi -ffunction-sections -MMD -MP \
               -DPC_SIDE -DSKIP_EEPROM_LOAD -Ddiag -DUNITY_INCLUDE_DOUBLE

INCS    := -I$(SRC) -Istubs -Iunity -I../pc_side
FINC    := -include ../pc_side/pc_side.h
LDFLAGS := -m32 -Wl,--gc-sections -Wl,--allow-multiple-definition

BUILD   := build

UNIT_OBJS := $(BUILD)/Subroutines.o $(BUILD)/Subroutines1.o $(BUILD)/mco.o \
             $(BUILD)/user.o $(BUILD)/Interrupts.o $(BUILD)/MenuSerialize.o \
             $(BUILD)/Packets.o
STUB_OBJ  := $(BUILD)/hardware_stubs.o
UNITY_OBJ := $(BUILD)/unity.o
SHARED_OBJS := $(STUB_OBJ) $(UNIT_OBJS) $(UNITY_OBJ)

# Every test_*.c here is a suite.
TESTS := $(patsubst %.c,%,$(wildcard test_*.c))

# Order-only prerequisite (after the |): objects depend on the directory
# EXISTING, not its timestamp — which changes on every write into it.
# Objects also depend on this Makefile: the -D flags (PC_SIDE, SKIP_EEPROM_LOAD,
# diag) live here, so editing them must force a rebuild. Without this, changing a
# define silently leaves every object compiled against the old flags.
MKFILE := $(firstword $(MAKEFILE_LIST))

$(BUILD):
	-mkdir $(BUILD)

$(BUILD)/%.o: $(SRC)/%.c $(MKFILE) | $(BUILD)
	$(CC) $(PROD_CFLAGS) $(INCS) $(FINC) -c -o $@ $<

$(BUILD)/hardware_stubs.o: stubs/hardware_stubs.c $(MKFILE) | $(BUILD)
	$(CC) $(PROD_CFLAGS) $(INCS) $(FINC) -c -o $@ $<

$(BUILD)/unity.o: unity/unity.c | $(BUILD)
	$(CC) $(PROD_CFLAGS) $(INCS) -c -o $@ $<

# Without .PRECIOUS, make treats a pattern-rule .o as intermediate and DELETES
# it after linking, so every suite recompiled every production TU from scratch
# each run. Covers the shared objects too — they are all pattern-rule built.
.PRECIOUS: $(BUILD)/%.o

$(BUILD)/test_%.o: test_%.c $(MKFILE) | $(BUILD)
	$(CC) $(TEST_CFLAGS) $(INCS) $(FINC) -c -o $@ $<

# hardware_stubs.o first so its putchar()/stubs win the link.
test_%.exe: $(BUILD)/test_%.o $(SHARED_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(STUB_OBJ) $(UNIT_OBJS) $(UNITY_OBJ) $< -lm

# Per-suite run target. These must be EXPLICIT rules, not a `test_%:` pattern:
# GNU Make skips pattern rules for .PHONY targets, so a pattern would silently
# do nothing. $(eval) stamps one rule per discovered suite.
define RUN_RULE
$(1): $(1).exe
	-.\$(1).exe
endef
$(foreach t,$(TESTS),$(eval $(call RUN_RULE,$(t))))

.PHONY: all check clean summary $(TESTS)

# Each run recipe is `-.\x.exe`, so a failing suite is reported but does not
# stop the others.
all: $(TESTS)

summary: $(addsuffix .exe,$(TESTS))
	python run_all.py

# Gated variant: the FIRST failing suite makes this exit non-zero.
check: $(addsuffix .exe,$(TESTS))
	$(foreach t,$(TESTS),.\$(t).exe && ) echo All test suites passed

clean:
	-rmdir /s /q $(BUILD) 2>nul
	-del /f /q *.exe 2>nul

# Header dependencies from -MMD -MP. Without these, editing a header never
# rebuilds the objects that include it and a suite can pass against a stale
# build. Keep last.
-include $(wildcard $(BUILD)/*.d)
