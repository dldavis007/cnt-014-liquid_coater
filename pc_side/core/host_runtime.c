/* host_runtime.c — host runtime: SFR backing store, --hw-* options, startup and
 * shutdown, the stall detector and the reset relaunch.
 */

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#define NOMINMAX
#include <errno.h>          /* before windows.h: TDM-GCC's mm_malloc.h needs EINVAL */
#include <windows.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_core.h"
#include "pc_log.h"

/* ==========================================================================
 * SFR backing array + host runtime globals
 * ========================================================================== */
unsigned char sfr_regs[0x400];      /* _REG_BASE under PC_SIDE */
volatile int  g_intr_masked = 1;    /* masked at reset (see pc_side.h) */

const struct pc_side_unit *pc_side_unit;

static unsigned short        recv_port, send_port;   /* for a reset relaunch */
static volatile int           g_running    = 1;
static volatile unsigned long g_loop_count = 0;     /* completed loop passes */

/* ==========================================================================
 * Hardware-fidelity options (--hw-* on the command line).
 * On by default, at the target's values, so the SIL loses and delays frames
 * where the target does. --no-hw turns them all off: lossless RX ring, every
 * frame accepted, instant EEPROM writes and transmits.
 * ========================================================================== */
int pc_side_hw_rx_fifo     = 5;   /* RX frames held before overrun (MSCAN: 5); 0 = off */
int pc_side_hw_filters     = 1;   /* apply mcohw.c's MSCAN acceptance filters */
int pc_side_hw_ee_erase_ms = 20;  /* EEPROM sector erase time; 0 = instant */
int pc_side_hw_can_tx      = 1;   /* transmits take bus time, as mcohw.c waits for them */

/* One --hw-* option into the settings above. 1 if it was one. */
static int hw_option(const char *arg)
{
    if (!strcmp(arg, "--no-hw")) {
        pc_side_hw_rx_fifo = pc_side_hw_filters = pc_side_hw_ee_erase_ms = pc_side_hw_can_tx = 0;
        return 1;
    }
    if (!strncmp(arg, "--hw-rx-fifo=", 13))     { pc_side_hw_rx_fifo     = atoi(arg + 13); return 1; }
    if (!strncmp(arg, "--hw-can-filters=", 17)) { pc_side_hw_filters     = atoi(arg + 17); return 1; }
    if (!strncmp(arg, "--hw-ee-erase-ms=", 17)) { pc_side_hw_ee_erase_ms = atoi(arg + 17); return 1; }
    if (!strncmp(arg, "--hw-can-tx=", 12))      { pc_side_hw_can_tx      = atoi(arg + 12); return 1; }
    return 0;
}

/* The active options as command-line text, for a reset relaunch. */
static void hw_options_text(char *buf, size_t n)
{
    snprintf(buf, n, " --hw-rx-fifo=%d --hw-can-filters=%d --hw-ee-erase-ms=%d --hw-can-tx=%d",
             pc_side_hw_rx_fifo, pc_side_hw_filters, pc_side_hw_ee_erase_ms, pc_side_hw_can_tx);
}

static void hw_log(void)
{
    char rx[24] = "unlimited", ee[24] = "instant";
    if (pc_side_hw_rx_fifo)     snprintf(rx, sizeof rx, "%d frames", pc_side_hw_rx_fifo);
    if (pc_side_hw_ee_erase_ms) snprintf(ee, sizeof ee, "%d ms/sector", pc_side_hw_ee_erase_ms);
    LOG_PRINTF(("[host] hardware fidelity: RX FIFO %s, CAN filters %s, EEPROM %s, CAN TX %s\n",
                rx, pc_side_hw_filters ? "on" : "off", ee,
                pc_side_hw_can_tx ? "timed (125 kbit/s)" : "instant"));
}

/* ==========================================================================
 * Startup / shutdown
 * ========================================================================== */
static void on_sigint(int sig) { (void)sig; g_running = 0; }

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

/* Usage:  <exe> [recv_port] [send_port] [--hw-...]   (prompts if ports omitted)
 *           --no-hw               all of the below off
 *           --hw-rx-fifo=N        hold N received frames, drop the rest (5; 0 = off)
 *           --hw-can-filters=0|1  apply the MSCAN acceptance filters (1)
 *           --hw-ee-erase-ms=N    EEPROM writes block N ms per 4-byte sector (20; 0 = off)
 *           --hw-can-tx=0|1       each transmit blocks for its 125 kbit/s bus time (1) */
int pc_side_begin(int argc, char **argv, const struct pc_side_unit *unit)
{
    int ports[2], nports = 0, i;

    pc_side_unit = unit;
    signal(SIGINT, on_sigint);
    setvbuf(stdout, NULL, _IONBF, 0);   /* unbuffered: live logs */
    disable_console_quickedit();

    printf("%s Rev %s - PC-side host  (Ctrl+C to quit)\n", unit->name, unit->revision);

    /* --hw-* options anywhere; the first two other arguments are the ports. */
    for (i = 1; i < argc; i++) {
        if (hw_option(argv[i]))
            continue;
        if (!strncmp(argv[i], "--", 2))
            printf("unknown option %s (ignored)\n", argv[i]);
        else if (nports < 2)
            ports[nports++] = atoi(argv[i]);
    }

    if (nports == 2) {
        recv_port = (unsigned short)ports[0];
        send_port = (unsigned short)ports[1];
    } else {
        recv_port = ask_port("recv (this host listens on)", unit->recv_port);
        send_port = ask_port("send (hub/peer listens on)",  unit->send_port);
    }

    /* Start async logging now the interactive prompts are done, so all
     * subsequent logging is non-blocking. */
    pc_log_init();
    hw_log();

    if (pc_side_can_init(recv_port, send_port) != 0) {
        LOG_PRINTF(("[fatal] CAN/UDP init failed\n"));
        pc_log_shutdown();
        return 1;
    }
    return 0;
}

int pc_side_running(void) { return g_running; }

/* After each main-loop pass: proof-of-life for the stall detector, then pace. */
void pc_side_loop_done(void)
{
    g_loop_count++;
    Sleep(5);
}

void pc_side_end(void)
{
    LOG_PRINTF(("\n[host] shutting down\n"));
    pc_side_rti_stop();
    pc_side_can_shutdown();
    pc_log_shutdown();      /* drain the log tail, then stop the writer thread */
}

/* ==========================================================================
 * Processor reset
 *
 * ResetProc() resets the unit the hardware way: arm the fastest COP rate, then
 * spin in while(1) until the watchdog fires. There is no COP here, so that spin
 * is forever and RestoreDefaults() - which ends in ResetProc - hangs the host.
 * The stall detector spots the spin and calls this.
 *
 * A reset is a RELAUNCH, not a jump back into main(): globals come back on their
 * initializers, then Load_Variables reads eeprom.bin - keeping them if
 * RestoreDefaults left the 0xFF flag, exactly as the target does.
 *
 * Called from the stall-detector thread, with the firmware thread still
 * spinning in ResetProc - nothing is asked of it. Closing the socket FIRST is
 * required, not tidiness: the child binds the same recv port and would fail
 * while we still hold it.
 * ========================================================================== */
static void pc_side_reset(void)
{
    char exe[MAX_PATH];
    char cmd[MAX_PATH + 128];
    char hw[96];
    STARTUPINFOA        si;
    PROCESS_INFORMATION pi;

    LOG_PRINTF(("[host] *** RESET: relaunching on :%u -> :%u ***\n",
                recv_port, send_port));

    pc_side_rti_stop();
    pc_side_can_shutdown();

    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    memset(&pi, 0, sizeof pi);

    if (GetModuleFileNameA(NULL, exe, sizeof exe)) {
        hw_options_text(hw, sizeof hw);
        snprintf(cmd, sizeof cmd, "\"%s\" %u %u%s",
                 exe, recv_port, send_port, hw);
        /* Ports as arguments so the child skips the interactive prompts.
         * Inherit handles: without it a redirected run (a tee, a log capture)
         * loses the child's output at the reset. The socket is already closed,
         * so there is nothing else worth inheriting. */
        if (CreateProcessA(exe, cmd, NULL, NULL, 1, 0, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        } else {
            LOG_PRINTF(("[host] relaunch failed (%lu)\n", GetLastError()));
        }
    }

    pc_log_shutdown();          /* drain the tail before we go */
    ExitProcess(0);
}

/* ==========================================================================
 * Stall detector
 *
 * Reports through the CONSOLE TITLE BAR (SetConsoleTitle is a kernel32 call,
 * NOT stdout) so it keeps updating even when the terminal has blocked our
 * stdout. That separates the two freeze modes we otherwise cannot tell apart:
 *   - title 'loop=' climbs while the log is frozen -> firmware is ALIVE, only
 *     the terminal is stuck
 *   - title 'loop=' also stops -> the main loop is genuinely HUNG (the state
 *     shown in the title says where).
 * Also reports the RTI rate: rate < 1.0 means every timed operation runs long
 * by 1/rate, e.g. a 20 s purge timeout at 0.65 really takes ~31 s.
 * ========================================================================== */
static DWORD WINAPI loop_watchdog_fn(void *arg)
{
    const struct pc_side_unit *u = pc_side_unit;
    unsigned long last = 0;
    int    stalled_secs = 0;
    char   title[220], status[100];
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
        rate    = real_hz / u->rti_hz;

        status[0] = '\0';
        if (u->status) u->status(status, sizeof status);

        snprintf(title, sizeof title,
                 "%s host Rev %s | loop=%lu | %s | RTI %.0fHz (%.2fx) skip=%u%s",
                 u->name, u->revision, g_loop_count, status,
                 real_hz, rate, skips - last_skips,
                 stalled_secs ? " | *** STALLED ***" : "");
        SetConsoleTitleA(title);

        /* Say it once, loudly, when the loop stops returning - the trace lines
         * just before it are what identify where. */
        if (stalled_secs == 1)
            LOG_PRINTF(("[STALL] main loop has not returned for 1s (%s)\n", status));

        /* One stall is a deliberate reset, not a hang: ResetProc() arms the
         * fastest COP rate and spins waiting for a watchdog the host does not
         * have. CR==1 is unique to that call - the host never arms the COP
         * otherwise - so a genuine hang still just logs [STALL] above and stays
         * there to be debugged. */
        if (stalled_secs >= 1 && (*u->cop_ctl & 0x07) == 1) {
            LOG_PRINTF(("[host] ResetProc spin detected -> resetting unit\n"));
            pc_side_reset();                  /* relaunches us; does not return */
        }
        LOG_PRINTF(("[rti ] %.0f tick/s (need %.0f, %.2fx real-time), %u skipped while masked\n",
                    real_hz, u->rti_hz, rate, skips - last_skips));

        last       = g_loop_count;
        last_ticks = ticks;
        last_skips = skips;
        last_ms    = now;
    }
    return 0;
}

void pc_side_watchdog_start(void)
{
    CreateThread(NULL, 0, loop_watchdog_fn, NULL, 0, NULL);
}
