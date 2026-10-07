#ifndef PC_CORE_H
#define PC_CORE_H

/* pc_core.h — the shared PC-side host core (HCS12 + MicroCANopen units).
 *
 * A unit's main.c fills in a pc_side_unit, then:
 *
 *     pc_side_begin(argc, argv, &unit);   args, logging, UDP CAN bus
 *     ...unit init, mirroring its Controller.c...
 *     pc_side_rti_start();                before anything waits on a timer
 *     pc_side_watchdog_start();           stall detector + reset relaunch
 *     while (pc_side_running()) { doevents(); pc_side_loop_done(); }
 *     pc_side_end();
 */

#include <stddef.h>

/* What the core needs from the unit. */
struct pc_side_unit {
    const char    *name;                    /* banner and title bar */
    const char    *revision;
    unsigned short recv_port, send_port;    /* UDP defaults offered at the prompt */

    void  (*rti_isr)(void);                 /* the production RTI handler */
    double  rti_hz;                         /* RTI_One_Sec */

    /* Title-bar / [STALL] state text, e.g. "State=16 FinishState". May be NULL. */
    void  (*status)(char *buf, size_t n);

    /* Registers the core touches, from the unit's mc9s12a128.h. */
    volatile unsigned char *cop_ctl;        /* &COPCTL: ResetProc arms it to reset */
    volatile unsigned char *can_rflg;       /* &CANRFLG: RX overrun flag */

    /* --hw-can-tx: mcohw.c's transmit wait. Arms *tx_timer = tx_timeout, spins
     * while it counts, calls tx_timed_out if it hit 0. tx_timer NULL = bus time only. */
    volatile unsigned int *tx_timer;
    unsigned int           tx_timeout;
    void                 (*tx_timed_out)(void);
};

int  pc_side_begin(int argc, char **argv, const struct pc_side_unit *unit);  /* 0 = ok */
void pc_side_rti_start(void);
void pc_side_watchdog_start(void);
int  pc_side_running(void);
void pc_side_loop_done(void);
void pc_side_end(void);

/* Shared between the core's own files. */
extern const struct pc_side_unit *pc_side_unit;
extern int pc_side_hw_rx_fifo, pc_side_hw_filters, pc_side_hw_ee_erase_ms, pc_side_hw_can_tx;

int  pc_side_can_init(unsigned short recv_port, unsigned short send_port);
void pc_side_can_shutdown(void);
void pc_side_rti_stop(void);
unsigned int pc_side_rti_ticks(void);
unsigned int pc_side_rti_skipped(void);

#endif  /* PC_CORE_H */
