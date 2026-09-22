#ifndef PC_LOG_H
#define PC_LOG_H

/* pc_log.h — logging macros for the PC-side host build.
 *
 * Two macros, both no-ops unless PC_SIDE is defined, so they compile away to
 * nothing in the ImageCraft firmware build (the byte-identical .s19 check
 * confirms this). Include this header wherever you want to add logging.
 *
 *   LOG_PRINTF(( "fmt", args ))   one-shot printf. DOUBLE PARENTHESES — this
 *                                 avoids C99 variadic macros (__VA_ARGS__),
 *                                 which ICC12 v7 may not support, so the no-op
 *                                 production form still parses on any C89
 *                                 compiler.
 *                                 e.g. LOG_PRINTF(("cam_add1=%d\n", cam_add1));
 *
 *   LOG_IF_CHANGED("fmt", val)    (preferred) prints only when `val` differs
 *                                 from the last time THIS call site ran, so a
 *                                 line placed in a polling loop (doevents /
 *                                 the coating state machine) prints once per
 *                                 transition instead of every pass. Each call
 *                                 site keeps its own static "last value".
 *                                 `fmt` must contain exactly one %ld.
 *                                 e.g. LOG_IF_CHANGED("coat state = %ld", state);
 */

#ifdef PC_SIDE

#include <stdio.h>

/* Logging is ASYNC (implemented in pc_side_host.c). The firmware/CAN threads log
 * on their hot paths, so a slow or stalled terminal must never block them: these
 * macros only format + enqueue a line (dropping it if the queue is full) and
 * return immediately. A dedicated writer thread drains the queue to stdout. Call
 * pc_log_init() once at startup (before any logging) and pc_log_shutdown() at
 * exit to flush the tail. See pc_side_host.c for the rationale. */
void pc_log_init(void);
void pc_log_shutdown(void);
void pc_log_printf(const char *fmt, ...);

#define LOG_PRINTF(args)  pc_log_printf args

#define LOG_IF_CHANGED(fmt, val)                     \
    do {                                             \
        static long _lc_last = 0x7FFFFFFFL;          \
        long _lc_v = (long)(val);                    \
        if (_lc_v != _lc_last) {                     \
            pc_log_printf(fmt "\n", _lc_v);          \
            _lc_last = _lc_v;                        \
        }                                            \
    } while (0)

#else   /* firmware build: both compile to nothing */

#define LOG_PRINTF(args)          ((void)0)
#define LOG_IF_CHANGED(fmt, val)  ((void)0)

#endif  /* PC_SIDE */

#endif  /* PC_LOG_H */
