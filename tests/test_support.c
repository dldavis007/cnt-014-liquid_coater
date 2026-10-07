/* test_support.c — the 12/48 Coater's test fixtures (production Rev 4.33).
 *
 * The hardware layer underneath (SFR array, CAN capture/inject, RTI and pacing
 * threads, EEPROM image) is shared: pc_side/core/test/test_hw.c.
 *
 * Rev4.33's Display()/PositionDisplay() pace with `Timer1 = <n>; while (Timer1);`,
 * so Timer1 is registered as the pacing timer below — see test_support.h for
 * why that separation from the RTI matters.
 */

#include <string.h>

#include "nodecfg.h"
#include "mco.h"
#include "mc9s12a128.h"       /* sfr_regs */
#include "EEProm.h"
#include "Subroutines.h"
#include "Subroutines1.h"
#include "Interrupts.h"
#include "procimg.h"
#include "test_support.h"

void RTI_Int_Handler(void);   /* production ISR in Interrupts.c */
extern unsigned int Timer1;

const struct test_unit test_unit = { RTI_Int_Handler, &Timer1 };

/* ==========================================================================
 * Firmware bring-up for tests.
 *
 * Mirrors Controller.c's main(), minus the hardware bring-up
 * (InitPLL, PWMInit, AtoDInit) which spins on status bits that never change on
 * a PC, and minus the EEPROM loads, so the unit comes up on its compiled-in
 * defaults.
 *
 * Idempotent: call from every setUp(); only the first call does the work.
 * ========================================================================== */
extern UNSIGNED8 gProcImg[];
extern unsigned  cam_add1;
extern unsigned  cam_add2;
extern struct menu_var NullVar;

static void bind_signal_blocks(void);   /* defined with the accessors below */

void host_firmware_init(void)
{
    static int done = 0;
    if (done) return;
    done = 1;

    bind_signal_blocks();

    /* NullVar.str_enum is NULL (NullVar is an uninitialised global). Any menu
     * that declares a variable column but points VarPntr at this sentinel - the
     * CAMERAS menu does - reaches getstrval() -> strlen(NULL). That is benign on
     * the HCS12, where address 0 is readable SFR space, and a hard SIGSEGV on
     * Windows. Pointing it at "" reproduces the target's blank-row outcome with
     * no firmware change. See pc_side/main.c for the full explanation. */
    NullVar.str_enum = "";

    InitPorts();
    InitInterrupts();
    EEInit();

    /* Must be running before the first Display(): see test_support.h. */
    pacing_thread_start();

    InitCANOpen();
}

/* ==========================================================================
 * Revision-agnostic accessors (see test_support.h). Rev4.34 reaches these bytes
 * through its rpdo/tpdo pointers; Rev4.33 indexes gProcImg directly. Suites go
 * through here, so this block IS the port.
 * ========================================================================== */
void set_actuator_moving(int moving) { gProcImg[OUT_digi_7]  = moving ? 1 : 0; }
void set_purge_moving(int moving)    { gProcImg[IN_digi_31]  = moving ? 1 : 0; }

int  la_commanded_pos(void)          { return gProcImg[IN_digi_12]; }
void set_la_commanded_pos(int tenths){ gProcImg[IN_digi_12] = (UNSIGNED8)tenths; }

/* Signal blocks, bound in host_firmware_init() (gProcImg is a plain array, so
 * these cannot be static initialisers under C's constant-expression rules). */
UNSIGNED8 *menu_data   = NULL;
UNSIGNED8 *camera_addr = NULL;
UNSIGNED8 *camera_cmds = NULL;
UNSIGNED8 *pairing_msg = NULL;

static void bind_signal_blocks(void)
{
    menu_data   = &gProcImg[OUT_digi_0];
    camera_addr = &gProcImg[OUT_digi_8];
    camera_cmds = &gProcImg[OUT_digi_10];
    pairing_msg = &gProcImg[OUT_digi_16];
}

/* Menu-variable setters (see test_support.h): the firmware's own idiom, as
 * Subroutines1.c uses it - strncpy + getvalue, or value + getstrval. */
int update_menu_var_by_str(struct menu_var *var, const char *new_str)
{
    strncpy(var->str_value, new_str, var->len_str);
    getvalue(var, 0);
    return 1;
}

int update_menu_var_by_value(struct menu_var *var, float new_value)
{
    var->value = new_value;
    getstrval(var);
    return 1;
}

/* The sequence keeps progress in file-scope globals that outlive one test. */
void reset_coat_sequence_state(void)
{
    extern int  StrokeNum;
    extern char MoveCmdXmtd;
    extern int  OldPumpSpeed;
    extern char LAError;
    extern unsigned int  LAMoveTimer;
    extern unsigned long LAMovingTimer;
    extern char ghostState;

    StrokeNum     = 0;
    MoveCmdXmtd   = 0;
    OldPumpSpeed  = 0;
    LAError       = 0;
    LAMoveTimer   = 0;
    /* Non-zero: several states route to ErrorState on !LAMovingTimer, which
     * would otherwise fire in every test that never armed a move. */
    LAMovingTimer = 0xFFFF;
    ghostState    = 0;
}

/* One fixture for every coating-sequence suite.
 *
 * doevents() runs a FULL pass before it reaches switch(State): menu, packet
 * serialization, camera handling, heaters. This quiets every input that could
 * move the state machine on its own, so a test observes only the case it set
 * up, and resets the menu variables the sequence branches on (they are globals
 * and a previous test's writes would otherwise leak in). */
void coat_test_begin(void)
{
    extern char  ghostState;
    extern float LAPos;
    extern char  Gen_Flags;
    extern float HeadSpeed;
    extern unsigned int  TC0_RCVD_Data;
    extern unsigned long StateTime;
    extern int   Update_Menu_Timer;
    extern char  State;

    /* Rev4.33 declares its menu variables per-TU rather than in a header
     * (they are defined in Subroutines.c), so the fixture externs them here
     * the same way Subroutines1.c does. */
    extern struct menu_var FirstStrokes, SecondStrokes, CleanOutStrokes;
    extern struct menu_var RetractTime, PumpSpd, MaxLADist;
    extern struct menu_var HeadOnOff, PumpOnOff;

    host_firmware_init();

    memset(sfr_regs, 0, sizeof sfr_regs);
    sfr_regs[0x86] = 0x80;          /* ATD0STAT0 SCF: any ATDGetLevel poll falls through */

    TC0_RCVD_Data = 0;              /* no telemetry: camera/cursor paths idle */
    ghostState    = 0;              /* throwGhost() returns on case 0         */
    Gen_Flags     = 0;
    HeadSpeed     = 0;
    Update_Menu_Timer = 0;

    set_actuator_moving(0);
    set_purge_moving(0);
    set_la_commanded_pos(0);
    menu_data[0] = 0;

    /* Camera addresses come from EEPROM on the target, which the host skips -
     * leaving both 0, which makes CameraMain1/2's address-match test trivially
     * true EVERY pass and emit a "Proc:<camtag>" display. That would pollute
     * every display assertion in the coating suites. Seed distinct nonzero
     * values so a match needs a real reply. */
    cam_add1 = 0x1111;
    cam_add2 = 0x2222;
    camera_addr[0] = camera_addr[1] = 0;
    camera_cmds[0] = 0;

    State     = FinishState;
    StateTime = 0;
    reset_coat_sequence_state();

    /* Menu defaults the sequence branches on (Subroutines.c compiled-in values). */
    update_menu_var_by_value(&FirstStrokes,    5.0f);
    update_menu_var_by_value(&SecondStrokes,  10.0f);
    update_menu_var_by_value(&CleanOutStrokes, 5.0f);
    update_menu_var_by_value(&RetractTime,     0.0f);
    update_menu_var_by_value(&PumpSpd,       100.0f);
    update_menu_var_by_value(&MaxLADist,      12.0f);
    update_menu_var_by_str(&HeadOnOff, "OFF");
    update_menu_var_by_str(&PumpOnOff, "OFF");
    LAPos = MaxLADist.value;        /* "head extended": InitLAMove goes forward */

    /* Last: the menu writes above push CAN frames we do not want to assert on. */
    can_tx_reset();
    can_rx_reset();
}

/* Reassemble Display() text out of the captured 0x310 frames. The menu engine
 * sends [node, STX, 6 chars], then [node, 7 chars]..., then [node, ETX, 0]. */
static char display_buf[128];

const char *last_display_message(void)
{
    unsigned i, j;
    int      collecting = 0;
    unsigned n = 0;
    display_buf[0] = '\0';

    for (i = 0; i < can_tx_count && i < CAN_TX_LOG_N; i++) {
        CAN_MSG *m = &can_tx_log[i];
        if (m->ID != WIM_ID) continue;
        if (m->LEN >= 2 && m->BUF[1] == 0x02) {          /* STX: new message */
            collecting = 1; n = 0;
            for (j = 2; j < m->LEN; j++)
                if (n < sizeof display_buf - 1) display_buf[n++] = (char)m->BUF[j];
        } else if (m->LEN >= 2 && m->BUF[1] == 0x03) {   /* ETX: end */
            collecting = 0;
            display_buf[n] = '\0';
        } else if (collecting) {
            for (j = 1; j < m->LEN; j++)
                if (n < sizeof display_buf - 1) display_buf[n++] = (char)m->BUF[j];
            display_buf[n] = '\0';
        }
    }
    /* Trim trailing pad spaces the chunker adds. */
    while (n > 0 && display_buf[n - 1] == ' ') display_buf[--n] = '\0';
    return display_buf;
}
