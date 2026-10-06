/* hardware_stubs.c
 *
 * Host-side definitions needed to link the production Rev 4.33 sources under
 * host GCC. Provides ONLY what the compiled translation units reference but do
 * not define themselves (found by trial link):
 *
 *   - sfr_regs[]     backing store for _REG_BASE under PC_SIDE (mc9s12a128.h)
 *   - g_intr_masked  the flag INTR_ON/OFF map to (pc_side.h)
 *   - EEPROM/Flash stubs (those driver TUs are NOT compiled: they busy-wait on
 *     hardware status bits that never change on a PC)
 *   - the CAN hardware layer (MCOHW_*); mcohw.c is likewise not compiled
 *
 * The real menu engine, MicroCANopen stack, node config and RTI ISR
 * (Subroutines.c, Subroutines1.c, mco.c, user.c, Interrupts.c) ARE compiled, so tests exercise genuine PDO mapping and menu
 * behaviour.
 *
 * IMPORTANT — two different blocking mechanisms are neutralised here:
 *
 *   1. MCOHW_GetTime() returns 0 and MCOHW_IsTimeExpired() returns 1 (always
 *      "expired"), so anything pacing off the MCO time base falls through.
 *
 *   2. Rev4.33's Display()/PositionDisplay() pace with
 *      `Timer1 = <n>; while (Timer1);`, and Timer1 moves only under
 *      RTI_Int_Handler(). The pacing thread at the bottom of this file
 *      services Timer1 and nothing else, so those spins end without
 *      advancing StateTime — see test_support.h for why that separation
 *      matters. This is the one real difference from the Rev4.34 harness.
 *
 * RTI simulation thread (bottom of file)
 * --------------------------------------
 * A Windows thread drives the real RTI_Int_Handler() so the HCS12 Real-Time
 * Interrupt is simulated on the host. Two modes:
 *   - deterministic: rti_thread_start(); advance_ticks(N) fires EXACTLY N ISR
 *     invocations, for exact timer assertions.
 *   - real-time:     rti_thread_start_realtime(ms) auto-fires every `ms`.
 * A suite that never calls rti_thread_start*() is unaffected.
 */

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#define NOMINMAX
#include <errno.h>      /* before windows.h: TDM-GCC's mm_malloc.h needs EINVAL */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <io.h>

#undef TRUE
#undef FALSE

#include "nodecfg.h"
#include "mco.h"
#include "mcohw.h"
#include "Subroutines.h"
#include "Subroutines1.h"
#include "Interrupts.h"
#include "procimg.h"
#include "test_support.h"

void RTI_Int_Handler(void);   /* production ISR in ../Interrupts.c */

/* ==========================================================================
 * SFR backing array + host runtime globals
 * ========================================================================== */
unsigned char sfr_regs[0x400];   /* replaces the SFRs memory-mapped near 0 */

/* The production sources may carry PC_SIDE tracing (pc_side/pc_log.h). Route it
 * to nothing by default - a suite printing firmware trace lines would bury the
 * Unity output. Set trace_to_stdout = 1 in a test to see it. */
int trace_to_stdout = 0;

void pc_log_printf(const char *fmt, ...)
{
    va_list ap;
    if (!trace_to_stdout) return;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

volatile int g_intr_masked = 0;  /* tests do not mask; see pc_side.h */

/* ==========================================================================
 * Driver stubs — EEPROM and Flash.
 * These TUs are excluded from the build: their real bodies spin on hardware
 * status bits (EEPROM/flash command-complete) that never set on a PC.
 * ========================================================================== */
void EEInit(void) { }
void EEWrite(int ArraySize, char WriteData[], int *WriteAddr)
{ (void)ArraySize; (void)WriteData; (void)WriteAddr; }

void FlashInit(void) { }
void FlashWrite(int ArraySize, char WriteData[], int *WriteAddr)
{ (void)ArraySize; (void)WriteData; (void)WriteAddr; }

/* ==========================================================================
 * CAN hardware layer (mcohw.c is not compiled).
 * Non-zero from Init: 0 makes MCO_Init() treat init as failed and call
 * MCOUSER_FatalError(), which re-enters InitCANOpen() -> infinite recursion.
 * ========================================================================== */
UNSIGNED8  MCOHW_Init(UNSIGNED16 BaudRate)          { (void)BaudRate; return 1; }
UNSIGNED8  MCOHW_SetCANFilter(UNSIGNED16 CANID)     { (void)CANID;    return 1; }
void       MCOHW_TimerISR(void)                      { }

/* Controllable MCO time base. OFF by default, so MCOHW_IsTimeExpired() stays
 * pinned to 1 and every suite that paces off it keeps falling through (see the
 * header note). set_mco_time() opts a suite in and switches to the real
 * wraparound compare from mcohw.c, for testing an actual timeout window. */
static int        mco_time_ctl = 0;
static UNSIGNED16 mco_time     = 0;

void set_mco_time(UNSIGNED16 ms) { mco_time_ctl = 1; mco_time = ms; }
void clear_mco_time(void)        { mco_time_ctl = 0; mco_time = 0; }

UNSIGNED16 MCOHW_GetTime(void) { return mco_time; }

UNSIGNED8 MCOHW_IsTimeExpired(UNSIGNED16 timestamp)
{
    if (!mco_time_ctl) return 1;
    timestamp++;                                /* min runtime, matches mcohw.c */
    if (mco_time > timestamp)
        return (UNSIGNED8)((mco_time - timestamp) < 0x8000);
    return (UNSIGNED8)((timestamp - mco_time) > 0x8000);
}

/* The acceptance-filter table mcohw.c owns; user.c clears it on reset. */
UNSIGNED8 setFilters[8] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80};

/* Captured TX frames. A test can inspect what the firmware transmitted without
 * a bus: can_tx_reset() then assert over can_tx_log[] / can_tx_count. */
CAN_MSG  can_tx_log[CAN_TX_LOG_N];
unsigned can_tx_count = 0;

void can_tx_reset(void) { can_tx_count = 0; }

UNSIGNED8 MCOHW_PushMessage(CAN_MSG *pTransmitBuf)
{
    if (can_tx_count < CAN_TX_LOG_N)
        can_tx_log[can_tx_count] = *pTransmitBuf;
    can_tx_count++;
    return 1;
}

/* Injected RX frames: can_rx_inject() queues one, the firmware pulls it on its
 * next MCO_ProcessStack pass exactly as it would from the CAN peripheral. */
#define CAN_RX_RING_N 32
static CAN_MSG  rx_ring[CAN_RX_RING_N];
static unsigned rx_head = 0, rx_tail = 0;

void can_rx_inject(UNSIGNED16 id, const UNSIGNED8 *data, UNSIGNED8 len)
{
    unsigned nxt = (rx_head + 1) % CAN_RX_RING_N;
    unsigned i;
    if (nxt == rx_tail) return;               /* full: drop */
    rx_ring[rx_head].ID  = id;
    rx_ring[rx_head].LEN = len > 8 ? 8 : len;
    for (i = 0; i < rx_ring[rx_head].LEN; i++) rx_ring[rx_head].BUF[i] = data[i];
    rx_head = nxt;
}

void can_rx_reset(void) { rx_head = rx_tail = 0; }

UNSIGNED8 MCOHW_PullMessage(CAN_MSG *pReceiveBuf)
{
    if (rx_tail == rx_head) return 0;
    *pReceiveBuf = rx_ring[rx_tail];
    rx_tail = (rx_tail + 1) % CAN_RX_RING_N;
    return 1;
}

/* Unity writes with putchar(); route it to the terminal. Listed first in the
 * link so it wins over any firmware putchar. */
int putchar(int c)
{
    char ch = (char)c;
    _write(1, &ch, 1);
    return c;
}

/* ==========================================================================
 * Firmware bring-up for tests.
 *
 * Mirrors Controller.c's main(), minus the hardware bring-up
 * (InitPLL, PWMInit, AtoDInit) which spins on status bits that never change on
 * a PC, and minus the EEPROM loads (SKIP_EEPROM_LOAD) which are reached through
 * absolute addresses Windows cannot map — so the unit comes up on its
 * compiled-in defaults.
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

/* ==========================================================================
 * Timer1 pacing service — Rev4.33 only. See test_support.h for the rationale.
 *
 * Services ONLY Timer1, so the production `while (Timer1)` spins in Display()
 * and PositionDisplay() complete while every other firmware timer
 * stays under deterministic advance_ticks() control.
 *
 * Zeroing Timer1 rather than decrementing it: the spins only test for zero, and
 * a real-rate decrement would make each Display() call take ~100 ms of wall
 * clock, which across the suites is minutes of dead waiting for no extra
 * coverage. A suite that wants to observe the spin can stop this thread and
 * drive advance_ticks() itself.
 * ========================================================================== */
extern unsigned int Timer1;

static volatile int pacing_running = 0;
static HANDLE       pacing_handle  = NULL;

static DWORD WINAPI pacing_thread_fn(LPVOID arg)
{
    (void)arg;
    while (pacing_running) {
        if (Timer1) Timer1 = 0;
        Sleep(0);                  /* yield; the spin is on the main thread */
    }
    return 0;
}

void pacing_thread_start(void)
{
    if (pacing_handle) return;
    pacing_running = 1;
    pacing_handle  = CreateThread(NULL, 0, pacing_thread_fn, NULL, 0, NULL);
}

void pacing_thread_stop(void)
{
    pacing_running = 0;
    if (pacing_handle) {
        WaitForSingleObject(pacing_handle, 2000);
        CloseHandle(pacing_handle);
        pacing_handle = NULL;
    }
}

/* ==========================================================================
 * RTI simulation thread — drives the real RTI_Int_Handler() (Interrupts.c) so
 * firmware timers advance on the host.
 * ========================================================================== */
static volatile int          rti_running   = 0;
static volatile int          rti_realtime  = 0;
static volatile unsigned int rti_period_ms = 1;
static HANDLE                rti_handle    = NULL;
static HANDLE                rti_go_sem    = NULL;
static volatile unsigned int rti_tick_count = 0;

static DWORD WINAPI rti_thread_fn(LPVOID arg)
{
    (void)arg;
    while (rti_running) {
        WaitForSingleObject(rti_go_sem, rti_realtime ? rti_period_ms : INFINITE);
        if (!rti_running) break;
        RTI_Int_Handler();
        rti_tick_count++;
    }
    return 0;
}

static void rti_thread_launch(void)
{
    rti_tick_count = 0;
    rti_running    = 1;
    rti_go_sem     = CreateSemaphore(NULL, 0, 0x7FFFFFFF, NULL);
    rti_handle     = CreateThread(NULL, 0, rti_thread_fn, NULL, 0, NULL);
}

void rti_thread_start(void)          { rti_realtime = 0; rti_thread_launch(); }

void rti_thread_start_realtime(unsigned int period_ms)
{
    rti_realtime  = 1;
    rti_period_ms = period_ms ? period_ms : 1;
    rti_thread_launch();
}

void rti_thread_stop(void)
{
    rti_running = 0;
    if (rti_go_sem) ReleaseSemaphore(rti_go_sem, 1, NULL);
    if (rti_handle) {
        WaitForSingleObject(rti_handle, 2000);
        CloseHandle(rti_handle);
        rti_handle = NULL;
    }
    if (rti_go_sem) { CloseHandle(rti_go_sem); rti_go_sem = NULL; }
}

/* Exactly one ISR invocation per call, so advance_ticks(N) means N. */
void rti_signal_tick(void)
{
    unsigned int target = rti_tick_count + 1;
    ReleaseSemaphore(rti_go_sem, 1, NULL);
    while (rti_tick_count < target) SwitchToThread();
}

void advance_ticks(unsigned int n)
{
    unsigned int i;
    for (i = 0; i < n; i++) rti_signal_tick();
}

unsigned int rti_ticks(void) { return rti_tick_count; }
