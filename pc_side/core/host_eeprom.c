/* host_eeprom.c — EEPROM and Flash driver stubs. EEProm.c / Flash.c are not
 * compiled: their real bodies spin on command-complete status bits that never
 * set on a PC.
 */

#define WIN32_LEAN_AND_MEAN
#include <errno.h>          /* before windows.h: TDM-GCC's mm_malloc.h needs EINVAL */
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "pc_core.h"
#include "pc_log.h"

/* EEPROM image (EE_begin points here under PC_EEPROM), kept in EE_FILE across
 * runs and resets. No file = erased (0xFF), so Load_Variables restores defaults. */
#define EE_FILE "eeprom.bin"
unsigned char pc_eeprom[EE_size];

static void ee_save(void)
{
    // opens EE_FILE for write
    FILE *f = fopen(EE_FILE, "wb");
    // writes pc_eeprom to EE_FILE; logs failure if it cannot write the full size
    if (!f || fwrite(pc_eeprom, 1, EE_size, f) != EE_size)
        LOG_PRINTF(("[host] EEPROM: failed to write %s\n", EE_FILE));
    // closes file stream if opened
    if (f) fclose(f);
}

void EEInit(void)
{
    // open EE_FILE for read
    FILE *f = fopen(EE_FILE, "rb");
    // fill pc_eeprom with 0xFF (erased state)
    memset(pc_eeprom, 0xFF, EE_size);
    // if file opened, read EE_size bytes into pc_eeprom; log success or failure
    if (f) {
        fread(pc_eeprom, 1, EE_size, f);
        // close file stream if opened
        fclose(f);
        LOG_PRINTF(("[host] EEPROM: loaded %s\n", EE_FILE));
    } else {
        LOG_PRINTF(("[host] EEPROM: no %s, starting erased\n", EE_FILE));
    }
}

/* The real EEWrite erases each 4-byte sector it touches, then programs it,
 * spinning on every command; the erases dominate. */
void EEWrite(int ArraySize, char WriteData[], int *WriteAddr)
{
    // if optional parameter pc_side_hw_ee_erase_ms is set, sleep for the calculated time based on the number of sectors to erase to simulate EEPROM blocking delay
    if (pc_side_hw_ee_erase_ms) {
        unsigned lead    = (unsigned)((size_t)WriteAddr & 3);
        unsigned sectors = (lead + (unsigned)ArraySize + 3) / 4;
        Sleep(sectors * (unsigned)pc_side_hw_ee_erase_ms);
    }
    // copy ArraySize bytes from WriteData to the address pointed by WriteAddr in pc_eeprom
    memcpy((char *)WriteAddr, WriteData, ArraySize);
    // save the updated EEPROM image to EE_FILE
    ee_save();
}

void FlashInit(void) { }
void FlashWrite(int ArraySize, char WriteData[], int *WriteAddr)
{ (void)ArraySize; (void)WriteData; (void)WriteAddr; }
