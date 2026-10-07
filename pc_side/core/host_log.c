/* host_log.c — async, drop-on-full logging behind pc_log.h.
 *
 * The firmware main thread (via MCO_ProcessStack -> MCOHW_PushMessage -> can_log)
 * and the CAN RX thread both log on hot paths. A synchronous printf blocks the
 * caller whenever the terminal stops draining (VS Code terminal flow control, a
 * full pipe), which would stall the host. So pc_log_printf() only formats into a
 * bounded ring and returns — it never touches stdout and never blocks; a full
 * ring drops the line and counts it. One writer thread drains to stdout, so only
 * that thread can block, and it clears latched stdout errors so output recovers.
 */

#define WIN32_LEAN_AND_MEAN
#include <errno.h>          /* before windows.h: TDM-GCC's mm_malloc.h needs EINVAL */
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "pc_log.h"

#define LOGQ_N    1024
#define LOGQ_LINE 200
static char              logq[LOGQ_N][LOGQ_LINE];
static volatile unsigned logq_head = 0, logq_tail = 0;
static volatile unsigned logq_dropped = 0;
static CRITICAL_SECTION  logq_lock;
static HANDLE            log_thread  = NULL;
static volatile int      log_running = 0;

static DWORD WINAPI log_thread_fn(LPVOID arg)
{
    (void)arg;
    for (;;) {
        char     line[LOGQ_LINE];
        unsigned dropped = 0;
        int      have = 0;
        EnterCriticalSection(&logq_lock);
        if (logq_tail != logq_head) {
            strcpy(line, logq[logq_tail]);
            logq_tail = (logq_tail + 1) % LOGQ_N;
            have = 1;
            if (logq_dropped) { dropped = logq_dropped; logq_dropped = 0; }
        }
        LeaveCriticalSection(&logq_lock);          /* fputs OUTSIDE the lock */
        if (have) {
            fputs(line, stdout);
            if (dropped)
                fprintf(stdout, "[log] dropped %u line(s) - output too slow\n", dropped);
            if (ferror(stdout)) clearerr(stdout);
        } else {
            if (!log_running) break;               /* stop only once drained */
            Sleep(2);
        }
    }
    fflush(stdout);
    return 0;
}

void pc_log_init(void)
{
    if (log_thread) return;
    InitializeCriticalSection(&logq_lock);
    log_running = 1;
    log_thread  = CreateThread(NULL, 0, log_thread_fn, NULL, 0, NULL);
}

void pc_log_shutdown(void)
{
    if (!log_thread) return;
    log_running = 0;                     /* writer drains the tail, then exits */
    WaitForSingleObject(log_thread, 2000);
    CloseHandle(log_thread);
    log_thread = NULL;
    DeleteCriticalSection(&logq_lock);
}

void pc_log_printf(const char *fmt, ...)
{
    char     line[LOGQ_LINE];
    va_list  ap;
    unsigned nxt;
    va_start(ap, fmt);
    vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    line[sizeof line - 1] = '\0';
    if (!log_thread) { fputs(line, stdout); return; }   /* pre-init: direct */
    EnterCriticalSection(&logq_lock);
    nxt = (logq_head + 1) % LOGQ_N;
    if (nxt == logq_tail) {
        logq_dropped++;                                /* full: drop, never block */
    } else {
        strcpy(logq[logq_head], line);
        logq_head = nxt;
    }
    LeaveCriticalSection(&logq_lock);
}
