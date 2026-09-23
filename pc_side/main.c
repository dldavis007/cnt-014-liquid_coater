/* main.c — PC-side host entry point for the 12/48 Coater (true production Rev 4.33) firmware.
 *
 * Runs the REAL firmware main loop (doevents()) on a PC with live logging and
 * a UDP CAN bus, so the logic can be driven and observed without the HCS12 or
 * NOICE. Compiled by GCC with -DPC_SIDE; ImageCraft never sees this file.
 *
 * Init mirrors Controller.c's main() minus the hardware bring-up
 * (InitPLL, PWMInit, AtoDInit): those busy-wait on status bits that never
 * change on a PC. EEInit() is stubbed in pc_side_host.c, and Load_Variables /
 * Load_Serial_Num / Load_Camera_Add are skipped under SKIP_EEPROM_LOAD because
 * the EEPROM is reached through absolute addresses Windows cannot map.
 *
 * Note Rev4.33's doevents() has NO internal loop — Controller.c's main() calls
 * it from a while(1), so the loop lives here, exactly as on the target.
 *
 * Usage:  pc_side_host.exe [recv_port] [send_port]   (prompts if omitted)
 *         Defaults 20010 / 20100 = bind :20010, send to the shared
 *         can_udp_hub.py bus on :20100 alongside the other emulated nodes.
 *         Start the bus first:  python can_hub_gui.py  (C:\Working_Projects\can_emulators)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* Trim the Win32 headers: winuser.h's A/W macros (LoadMenu->LoadMenuA, ...)
 * collide with firmware symbol names. We only need Sleep()/threads here. */
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#define NOMINMAX
#include <errno.h>
#include <windows.h>

#undef TRUE
#undef FALSE

#include "nodecfg.h"
#include "mco.h"
#include "mcohw.h"
#include "mc9s12a128.h"       /* PORTA, behind VSEL_PORT */
#include "Controller.h"
#include "Subroutines.h"
#include "Subroutines1.h"
#include "Interrupts.h"       /* RTI_One_Sec */
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

void EEInit(void);          /* stubbed in pc_side_host.c (EEProm.c not compiled) */

/* pc_side_host.c entry points. */
int  pc_side_can_init(unsigned short recv_port, unsigned short send_port);
void pc_side_can_shutdown(void);
void rti_thread_start_realtime(unsigned int period_ms);
void rti_thread_stop(void);
void pc_side_reset(void);
unsigned int pc_side_rti_ticks(void);
unsigned int pc_side_rti_skipped(void);

static const char *coat_state_name(int s);        /* defined below */

static volatile int           g_running    = 1;
static volatile unsigned long g_loop_count = 0;   /* completed doevents() passes */

static void on_sigint(int sig) { (void)sig; g_running = 0; }

/* Host diagnostic heartbeat, reported through the CONSOLE TITLE BAR
 * (SetConsoleTitle is a kernel32 call, NOT stdout) so it keeps updating even
 * when the terminal has blocked our stdout. That separates the two freeze modes
 * we otherwise cannot tell apart:
 *   - title 'loop=' climbs while the log is frozen -> firmware is ALIVE, only
 *     the terminal is stuck
 *   - title 'loop=' also stops -> doevents() is genuinely HUNG (the state shown
 *     in the title says where).
 * Also reports the RTI rate: rate < 1.0 means every timed operation runs long
 * by 1/rate, e.g. a 20 s purge timeout at 0.65 really takes ~31 s. On Rev4.33 a
 * collapsing rate is doubly serious — Display()'s `while (Timer1)` spins only
 * end because this thread ticks. */
static DWORD WINAPI loop_watchdog_fn(void *arg)
{
    unsigned long last = 0;
    int    stalled_secs = 0;
    char   title[220];
    unsigned int last_ticks = 0, last_skips = 0;
    DWORD  last_ms = GetTickCount();
    (void)arg;

    while (g_running) {
        unsigned int ticks, skips;
        DWORD  now;
        double rate, real_hz;

        Sleep(1000);
        stalled_secs = (g_loop_count == last) ? stalled_secs + 1 : 0;

        ticks   = pc_side_rti_ticks();
        skips   = pc_side_rti_skipped();
        now     = GetTickCount();
        real_hz = (now > last_ms)
                ? (double)(ticks - last_ticks) * 1000.0 / (double)(now - last_ms)
                : 0.0;
        rate    = real_hz / (double)RTI_One_Sec;

        snprintf(title, sizeof title,
                 "CNT-014 coater host Rev4.33 | loop=%lu | State=%d %s | RTI %.0fHz (%.2fx) skip=%u%s",
                 g_loop_count, (int)State, coat_state_name((int)State),
                 real_hz, rate, skips - last_skips,
                 stalled_secs ? " | *** STALLED ***" : "");
        SetConsoleTitleA(title);

        /* Say it once, loudly, when the loop stops returning - the trace lines
         * from doevents() just before it are what identify where. */
        if (stalled_secs == 1)
            LOG_PRINTF(("[STALL] doevents() has not returned for 1s "
                        "(State=%d %s)\n",
                        (int)State, coat_state_name((int)State)));

        /* One stall is a deliberate reset, not a hang: ResetProc() arms the
         * fastest COP rate and spins waiting for a watchdog the host does not
         * have (RestoreDefaults ends there). CR==1 is unique to that call -
         * the host never arms the COP otherwise - so a genuine hang still just
         * logs [STALL] above and stays there to be debugged. */
        if (stalled_secs >= 1 && (COPCTL & 0x07) == 1) {
            LOG_PRINTF(("[host] ResetProc spin detected -> resetting unit\n"));
            pc_side_reset();                  /* relaunches us; does not return */
        }
        LOG_PRINTF(("[rti ] %.0f tick/s (need %.0f, %.2fx real-time), %u skipped while masked\n",
                    real_hz, (double)RTI_One_Sec, rate, skips - last_skips));

        last       = g_loop_count;
        last_ticks = ticks;
        last_skips = skips;
        last_ms    = now;
    }
    return 0;
}

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

/* Prompt for a UDP port; blank input (Enter) keeps the default. */
static unsigned short ask_port(const char *label, unsigned short def)
{
    char line[32];
    printf("%s port [%u]: ", label, (unsigned)def);
    if (fgets(line, sizeof line, stdin)) {
        int v = atoi(line);            /* blank/non-numeric -> 0 -> keep default */
        if (v > 0 && v < 65536) return (unsigned short)v;
    }
    return def;
}

/* Windows consoles ship with QuickEdit ON: clicking in (or selecting text in)
 * the window PAUSES our stdout — the next printf in ANY thread blocks until a
 * key is pressed. With three threads logging here, one stray click silently
 * freezes the host with no error while the rest of the bus keeps running.
 * (ENABLE_EXTENDED_FLAGS must be set for the change to apply.) */
#ifndef ENABLE_QUICK_EDIT_MODE
#define ENABLE_QUICK_EDIT_MODE 0x0040
#endif
#ifndef ENABLE_EXTENDED_FLAGS
#define ENABLE_EXTENDED_FLAGS  0x0080
#endif
static void disable_console_quickedit(void)
{
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    DWORD  mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode)) {
        mode &= ~ENABLE_QUICK_EDIT_MODE;
        mode |=  ENABLE_EXTENDED_FLAGS;
        SetConsoleMode(h, mode);
    }
}

int main(int argc, char **argv)
{
    unsigned short recv_port, send_port;

    signal(SIGINT, on_sigint);
    setvbuf(stdout, NULL, _IONBF, 0);   /* unbuffered: live logs */
    disable_console_quickedit();

    printf("12/48 Coater Rev4.33 - PC-side host  (Ctrl+C to quit)\n");

    if (argc > 2) {
        recv_port = (unsigned short)atoi(argv[1]);
        send_port = (unsigned short)atoi(argv[2]);
    } else {
        recv_port = ask_port("recv (this host listens on)", 20010);
        send_port = ask_port("send (hub/peer listens on)",  20100);
    }

    /* Start async logging now the interactive prompts are done, so all
     * subsequent logging is non-blocking. */
    pc_log_init();

    if (pc_side_can_init(recv_port, send_port) != 0) {
        LOG_PRINTF(("[fatal] CAN/UDP init failed\n"));
        return 1;
    }

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
    EEInit();                    /* stubbed in pc_side_host.c */

    /* Real-time RTI simulation. On Rev4.33 this must be running before any
     * Display() call: Display() paces its CAN frames with `while (Timer1)` and
     * only RTI_Int_Handler() decrements Timer1. */
    rti_thread_start_realtime(1 /* ms per wakeup */);
    LOG_PRINTF(("[host] RTI sim thread started\n"));

    INTR_ON();          /* enable simulated interrupts (see pc_side.h) */

    CreateThread(NULL, 0, loop_watchdog_fn, NULL, 0, NULL);   /* stall detector */

    InitCANOpen();
    LOG_PRINTF(("[host] unit + CANopen up, interrupts enabled\n"));

    /* Park the coater idle so the coating sequence is a no-op until commanded
     * (the trigger arrives over CAN, as on the target). */
    State = FinishState;

    /* Camera addresses normally come from EEPROM (Load_Camera_Add), which the
     * host skips — leaving both 0, which makes CameraMain1/2's address-match
     * test trivially true EVERY pass and floods the bus with display frames.
     * Seed distinct nonzero values so a match needs a real reply. */
    cam_add1 = 0x1111;
    cam_add2 = 0x2222;

    /* Assert the camera-present input. doevents() only accepts the start
     * trigger when (VSEL_PORT & CAM_ON) is set; that is a GPIO on the target,
     * so nothing on the UDP bus can ever set it and the trigger would be
     * silently swallowed. */
    // VSEL_PORT |= CAM_ON;

    LOG_PRINTF(("[host] running main loop\n"));
    while (g_running) {
        /* Controller.c kicks the COP here; the host has no watchdog. */
        doevents();      /* one firmware pass, incl. MCO_ProcessStack (PDO I/O) */
        g_loop_count++;  /* proof-of-life for loop_watchdog_fn */

        log_state_transitions();
        LOG_IF_CHANGED("[in  ] purge moving (IN_digi_31) = %ld", gProcImg[IN_digi_31]);
        LOG_IF_CHANGED("[in  ] actuator moving (OUT_digi_7) = %ld", gProcImg[OUT_digi_7]);
        Sleep(5);
    }

    LOG_PRINTF(("\n[host] shutting down\n"));
    rti_thread_stop();
    pc_side_can_shutdown();
    pc_log_shutdown();      /* drain the log tail, then stop the writer thread */
    return 0;
}
