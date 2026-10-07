/* main.c — PC-side host entry point for the 12/48 Coater (true production Rev 4.33) firmware.
 *
 * Runs the REAL firmware main loop (doevents()) on a PC with live logging and
 * a UDP CAN bus, so the logic can be driven and observed without the HCS12 or
 * NOICE. Compiled by GCC with -DPC_SIDE; ImageCraft never sees this file.
 *
 * This file is the unit-specific part: init order, test seeds and logging. The
 * shared host (CAN bus, RTI thread, EEPROM image, stall detector, reset,
 * --hw-* options) is pc_side/core - see pc_core.h.
 *
 * Init mirrors Controller.c's main() minus the hardware bring-up
 * (InitPLL, PWMInit, AtoDInit): those busy-wait on status bits that never
 * change on a PC. The EEPROM is a host image persisted to eeprom.bin, so the
 * EEPROM loads run unchanged.
 *
 * Note Rev4.33's doevents() has NO internal loop — Controller.c's main() calls
 * it from a while(1), so the loop lives here, exactly as on the target.
 *
 * Usage:  pc_side_host.exe [recv_port] [send_port] [--hw-...]   (prompts if ports omitted)
 *         Defaults 20010 / 20100 = bind :20010, send to the shared
 *         can_udp_hub.py bus on :20100 alongside the other emulated nodes.
 *         --hw-* options: see pc_side_begin() in core/host_runtime.c.
 *         Start the bus first:  python can_hub_gui.py  (C:\Working_Projects\can_emulators)
 */

#include <stdio.h>

#include "nodecfg.h"
#include "mco.h"
#include "mcohw.h"
#include "mc9s12a128.h"       /* COPCTL, CANRFLG; PORTA, behind VSEL_PORT */
#include "Controller.h"
#include "Subroutines.h"
#include "Subroutines1.h"
#include "Interrupts.h"       /* RTI_One_Sec, Gen_Flags_No2Wire */
#include "pc_core.h"
#include "pc_log.h"

/* Defined in the firmware TUs but not declared in any header.
 * State is a `char` (Subroutines.c), so every coating-state value must stay
 * under 256. */
extern UNSIGNED8 gProcImg[];
extern char      State;
extern char      ghostState;
extern unsigned  cam_add1;
extern unsigned  cam_add2;
extern struct menu_var NullVar;
extern unsigned int Timer1;
extern char      Gen_Flags;

void RTI_Int_Handler(void);   /* production ISR, Interrupts.c */
void EEInit(void);            /* core/host_eeprom.c (EEProm.c not compiled) */

/* Names for the coating state machine (the `State` values in Subroutines.h), so
 * the log reads as a sequence instead of bare numbers. */
static const char *coat_state_name(int s)
{
    switch (s) {
    case TrigState:                  return "TrigState";
    case CkHeadRotation:             return "CkHeadRotation";
    case PurgeRetract:               return "PurgeRetract";
    case PurgeRetractWait:           return "PurgeRetractWait";
    case InitLAMove:                 return "InitLAMove";
    case StartCoatState:             return "StartCoatState";
    case FirstCoatState:             return "FirstCoatState";
    case StartSecondCoatState:       return "StartSecondCoatState";
    case SecondCoatState:            return "SecondCoatState";
    case StartCleanOutState:         return "StartCleanOutState";
    case CleanOutState:              return "CleanOutState";
    case HomeState:                  return "HomeState";
    case StopState:                  return "StopState";
    case CoatingComplete:            return "CoatingComplete";
    case StopWaitState:              return "StopWaitState";
    case FinishState:                return "FinishState";
    case ErrorState:                 return "ErrorState";
    case HeadErrorState:             return "HeadErrorState";
    case HdErrHomeState:             return "HdErrHomeState";
    case HdErrStopState:             return "HdErrStopState";
    default:                         return "(unnamed)";
    }
}

/* throwGhost()'s parallel purge sequence. Only the steps worth naming; the
 * numbered waits in between print as "step N". */
static const char *ghost_state_name(int s)
{
    switch (s) {
    case 0:   return "idle";
    case 1:   return "force retract";
    case 3:   return "await retracted";
    case 5:   return "extend cup";
    case 6:   return "await extend start";
    case 7:   return "await extended";
    case 8:   return "head ON";
    case 10:  return "pump ON / purging";
    case 12:  return "pump OFF";
    case 14:  return "head OFF";
    case 16:  return "retract cup";
    case 17:  return "await retract start";
    case 18:  return "await retracted";
    case 19:  return "complete";
    case 99:  return "ERROR";
    case 100: return "ERROR (latched)";
    default:  return "step";
    }
}

/* Log each coating-sequence transition as `from -> to`, with names. */
static void log_state_transitions(void)
{
    static int prev_state = -1;
    static int prev_ghost = -1;
    int s = (int)State;
    int g = (int)ghostState;

    if (s != prev_state) {
        if (prev_state < 0)
            LOG_PRINTF(("[seq ] State = %d %s\n", s, coat_state_name(s)));
        else
            LOG_PRINTF(("[seq ] State %d %s -> %d %s\n",
                        prev_state, coat_state_name(prev_state),
                        s, coat_state_name(s)));
        prev_state = s;
    }

    if (g != prev_ghost) {
        if (g || prev_ghost > 0)     /* skip the initial 0 -> 0 noise */
            LOG_PRINTF(("[ghost] %d %s -> %d %s\n",
                        prev_ghost < 0 ? 0 : prev_ghost,
                        ghost_state_name(prev_ghost < 0 ? 0 : prev_ghost),
                        g, ghost_state_name(g)));
        prev_ghost = g;
    }
}

/* Title bar / [STALL] text. */
static void unit_status(char *buf, size_t n)
{
    snprintf(buf, n, "State=%d %s", (int)State, coat_state_name((int)State));
}

/* mcohw.c's transmit timeout. */
static void unit_tx_timed_out(void)
{
    Gen_Flags |= Gen_Flags_No2Wire;
}

static const struct pc_side_unit unit = {
    .name         = "12/48 Coater",
    .revision     = Revision,
    .recv_port    = 20010,
    .send_port    = 20100,
    .rti_isr      = RTI_Int_Handler,
    .rti_hz       = RTI_One_Sec,
    .status       = unit_status,
    .cop_ctl      = &COPCTL,
    .can_rflg     = &CANRFLG,
    .tx_timer     = &Timer1,               /* mcohw.c: Timer1 = 0.5 * RTI_One_Sec */
    .tx_timeout   = 0.5 * RTI_One_Sec,
    .tx_timed_out = unit_tx_timed_out,
};

int main(int argc, char **argv)
{
    if (pc_side_begin(argc, argv, &unit) != 0)
        return 1;

    /* Same order as Controller.c's main(), minus the hardware bring-up.
     * InitInterrupts() registers the timer/RTI configuration; the RTI thread is
     * started AFTER it, and fires nothing until INTR_ON() below, because
     * g_intr_masked starts set. */
    /* NullVar.str_enum is a NULL pointer (NullVar is an uninitialised global, so
     * it lives in BSS). Menus that declare a variable column but point VarPntr
     * at this sentinel - the CAMERAS menu's "CAMERA 1"/"CAMERA 2" rows do
     * exactly that - reach getstrval(), which calls strlen(var->str_enum).
     *
     * On the HCS12 that is harmless: address 0 is the SFR block, so it is
     * readable, and with len_str == 0 the eventual strncpy copies nothing and
     * the row renders blank. On Windows page 0 is unmapped, so the same call
     * is a hard SIGSEGV and the host dies the moment you open that menu.
     *
     * Pointing str_enum at an empty string reproduces the benign target
     * outcome exactly (strlen() == 0 -> the enum branch is skipped -> blank
     * row) without altering a single byte of firmware. The underlying menu
     * table inconsistency is left alone: it is pre-existing, and whether those
     * rows should show the camera addresses (as the STATUS MENU does with
     * disp_add1/disp_add2) or drop their Pos to 0 is a product decision. */
    NullVar.str_enum = "";

    LOG_PRINTF(("[host] bringing up the unit...\n"));
    InitPorts();
    InitInterrupts();
    EEInit();                    /* loads eeprom.bin */

    /* Real-time RTI simulation. On Rev4.33 this must be running before any
     * Display() call: Display() paces its CAN frames with `while (Timer1)` and
     * only RTI_Int_Handler() decrements Timer1. */
    pc_side_rti_start();

    INTR_ON();          /* enable simulated interrupts (see pc_side.h) */

    pc_side_watchdog_start();

    /* Controller.c's order. After the stall detector, so a ResetProc from a
     * corrupt image is caught and relaunched.
     * TODO: build-time check that this call list matches Controller.c main(). */
    Load_Camera_Add();
    Load_Serial_Num();
    Load_Variables();

    InitCANOpen();
    LOG_PRINTF(("[host] unit + CANopen up, interrupts enabled\n"));

    // InternalExternalCameraSetting.value = 2.0f;  /* default to HD active camera, for testing at least */
    // LOG_PRINTF(("[host] InternalExternalCameraSetting.value = %.1f (default External active camera for testing, default internal in production)\n", InternalExternalCameraSetting.value));

    /* Park the coater idle so the coating sequence is a no-op until commanded
     * (the trigger arrives over CAN, as on the target). */
    State = FinishState;

    /* Erased EEPROM gives both cameras 0xFFFF; seed distinct test addresses
     * until real ones are saved. */
    if (cam_add1 == 0xFFFF) cam_add1 = 0x1928;
    if (cam_add2 == 0xFFFF) cam_add2 = 0x2526;

    /* Assert the camera-present input. doevents() only accepts the start
     * trigger when (VSEL_PORT & CAM_ON) is set; that is a GPIO on the target,
     * so nothing on the UDP bus can ever set it and the trigger would be
     * silently swallowed. */
    // VSEL_PORT |= CAM_ON;

    LOG_PRINTF(("[host] running main loop\n"));
    while (pc_side_running()) {
        /* Controller.c kicks the COP here; the host has no watchdog. */
        doevents();      /* one firmware pass, incl. MCO_ProcessStack (PDO I/O) */

        log_state_transitions();
        LOG_IF_CHANGED("[in  ] purge moving (IN_digi_31) = %ld", gProcImg[IN_digi_31]);
        LOG_IF_CHANGED("[in  ] actuator moving (OUT_digi_7) = %ld", gProcImg[OUT_digi_7]);

        pc_side_loop_done();
    }

    pc_side_end();
    return 0;
}
