# =============================================================================
# Makefile — 12/48 Coater (true production Rev 4.33) host-side unit tests (Unity)
#
# The shared build lives in ../pc_side/core/test/test.mk; this file holds only
# the unit's part. Fixtures: test_support.c. Every test_*.c is a suite.
#
# Deliberately NOT compiled (core/test/test_hw.c supplies their entry points):
#   Controller.c  holds main(); test_support.c's host_firmware_init() mirrors it
#   EEProm.c      spins on command-complete (test_hw.c: EEPROM image in RAM)
#   Flash.c       same, for the flash writer
#   mcohw.c       spins on CAN transmit-buffer status bits that never set
#   Command.c     serial command console, reached only from the SCI ISRs
#
# Usage:  mingw32-make all | check | summary | test_<suite> | clean
# =============================================================================

SRC      := ../Source_Files
FIRMWARE := Subroutines Subroutines1 mco user Interrupts
UNIT_DEF := -Ddiag

ifeq ($(wildcard ../pc_side/core/test/test.mk),)
$(error pc_side/core is empty - run: git submodule update --init)
endif
include ../pc_side/core/test/test.mk
