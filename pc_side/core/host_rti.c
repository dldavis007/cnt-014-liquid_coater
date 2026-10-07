/* host_rti.c — RTI simulation thread.
 *
 * Drives the unit's production RTI handler so firmware timers advance. The
 * handler is skipped while interrupts are masked (g_intr_masked), giving the
 * firmware's INTR_OFF()/INTR_ON() critical sections real meaning here.
 *
 * Load-bearing, not just a convenience: firmware paces CAN frames with
 * `Timer1 = <n>; while (Timer1);`, and only the RTI handler decrements Timer1.
 * Without these ticks the first such wait spins forever.
 */

#define WIN32_LEAN_AND_MEAN
#include <errno.h>          /* before windows.h: TDM-GCC's mm_malloc.h needs EINVAL */
#include <windows.h>

#include "pc_core.h"
#include "pc_log.h"

static volatile int          rti_running   = 0;
static HANDLE                rti_handle    = NULL;
static volatile unsigned int rti_tick_count    = 0;
static volatile unsigned int rti_skipped_count = 0;

/* Never fire more than this many ticks from one wakeup. Without a cap, a long
 * stall (a breakpoint, a suspended VM) would come back and dump tens of
 * thousands of ticks at once, expiring every firmware timer simultaneously. */
#define RTI_MAX_CATCHUP 100

static DWORD WINAPI rti_thread_fn(LPVOID arg)
{
    LARGE_INTEGER freq, now, prev;
    double owed = 0.0;          /* fractional ticks the clock says we still owe */

    (void)arg;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);

    while (rti_running) {
        Sleep(1);
        if (!rti_running) break;

        /* The number of ticks owed comes from the CLOCK, not from how many
         * times we woke up. A 1 ms Sleep does not take 1 ms on Windows — with
         * the default ~15.6 ms timer granularity it lands much longer, so a
         * one-tick-per-wakeup thread falls far short of the rate the
         * firmware's timers assume, and everything timed runs long by exactly
         * that shortfall. Deriving the count from QueryPerformanceCounter makes
         * the simulated RTI track wall-clock however coarse the wakeups are. */
        QueryPerformanceCounter(&now);
        owed += (double)(now.QuadPart - prev.QuadPart)
              / (double)freq.QuadPart * pc_side_unit->rti_hz;
        prev = now;

        if (owed > (double)RTI_MAX_CATCHUP) owed = (double)RTI_MAX_CATCHUP;

        while (owed >= 1.0) {
            if (g_intr_masked) {
                /* Leave the debt on the books: the ticks fire as soon as the
                 * firmware re-enables interrupts, so a critical section delays
                 * time rather than destroying it. */
                rti_skipped_count++;
                break;
            }
            pc_side_unit->rti_isr();
            rti_tick_count++;
            owed -= 1.0;
        }
    }
    return 0;
}

unsigned int pc_side_rti_ticks(void)   { return rti_tick_count; }
unsigned int pc_side_rti_skipped(void) { return rti_skipped_count; }

void pc_side_rti_start(void)
{
    rti_tick_count = 0;
    rti_running    = 1;
    rti_handle     = CreateThread(NULL, 0, rti_thread_fn, NULL, 0, NULL);
    LOG_PRINTF(("[host] RTI sim thread started\n"));
}

void pc_side_rti_stop(void)
{
    rti_running = 0;
    if (rti_handle) {
        WaitForSingleObject(rti_handle, 2000);
        CloseHandle(rti_handle);
        rti_handle = NULL;
    }
}
