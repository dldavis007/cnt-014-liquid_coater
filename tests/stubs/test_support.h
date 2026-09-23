#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

/* Host-side test controls implemented in stubs/hardware_stubs.c. */

#include "nodecfg.h"
#include "mco.h"

/* Captured CAN transmissions (what the firmware pushed). */
#define CAN_TX_LOG_N 64
extern CAN_MSG  can_tx_log[CAN_TX_LOG_N];
extern unsigned can_tx_count;
void can_tx_reset(void);

/* Injected CAN receptions; the firmware pulls these on its next stack pass. */
void can_rx_inject(UNSIGNED16 id, const UNSIGNED8 *data, UNSIGNED8 len);
void can_rx_reset(void);

/* Runs the real firmware bring-up once (ports, interrupts, CANopen), and starts
 * the Timer1 pacing service described below. Idempotent — call it from setUp(). */
void host_firmware_init(void);

/* RTI simulation. Deterministic mode: advance_ticks(N) fires exactly N ISRs. */
void rti_thread_start(void);
void rti_thread_start_realtime(unsigned int period_ms);
void rti_thread_stop(void);
void advance_ticks(unsigned int n);
unsigned int rti_ticks(void);

/* ---------------------------------------------------------------------------
 * Timer1 pacing service — Rev4.33 only.
 *
 * Rev4.33 paces its outgoing CAN display frames with the production idiom
 *
 *     Timer1 = RTI_One_Sec * .05;  while ( Timer1 );
 *
 * in Display() and PositionDisplay(). Timer1 is
 * decremented ONLY by RTI_Int_Handler(), so on the host those spins never end
 * and the first Display() in the coating sequence hangs the suite. (Rev4.34
 * paces with MCOHW_GetTime instead, which the stubs below pin to "always
 * expired", so it never had this problem.)
 *
 * We do NOT solve it by #ifdef'ing the waits out of the firmware — the pacing
 * code should stay on the host path. Instead a background thread services
 * Timer1 and NOTHING else, so those spins complete while StateTime and every
 * other firmware timer stay under deterministic advance_ticks() control. Using
 * the full RTI ISR here would advance StateTime by ~100 ticks per Display()
 * call and make every timeout assertion racy.
 *
 * Started automatically by host_firmware_init(); the controls are exposed for
 * a test that wants to observe the spin itself.
 * ------------------------------------------------------------------------- */
void pacing_thread_start(void);
void pacing_thread_stop(void);

/* ---------------------------------------------------------------------------
 * Revision-agnostic accessors for the signals the coating sequence reads.
 *
 * Suites use ONLY these, never a revision's own signal names. Rev4.34 reaches
 * these bytes as *rpdo4_actuator_moving / *rpdo7_purge_moving /
 * tpdo3_actuator_1[0]; Rev4.33 reaches them as gProcImg[OUT_digi_7] /
 * gProcImg[IN_digi_31] / gProcImg[IN_digi_12]. Keeping the tests behind this
 * seam is what made porting the suites here a reimplementation of this block
 * rather than a rewrite of every test.
 * ------------------------------------------------------------------------- */
void set_actuator_moving(int moving);
void set_purge_moving(int moving);
int  la_commanded_pos(void);          /* tenths of an inch, as sent on 0x36A */
void set_la_commanded_pos(int tenths);

/* Menu/trigger and camera signal BLOCKS, as pointers so a suite can index them
 * the way the firmware does. The mapping is 1:1 between revisions — Rev4.34's
 * config.c aliases exactly these three gProcImg regions:
 *
 *   menu_data    -> gProcImg[OUT_digi_0]   (1 byte in production 4.33; 8 in the refactored 4.33 / 4.34 rpdo1_menu_data)
 *   camera_addr  -> gProcImg[OUT_digi_8]   (2 bytes)  == 4.34 rpdo5_camera_addr
 *   camera_cmds  -> gProcImg[OUT_digi_10]  (5 bytes)  == 4.34 rpdo6_camera_cmds
 */
extern UNSIGNED8 *menu_data;
extern UNSIGNED8 *camera_addr;
extern UNSIGNED8 *camera_cmds;

/* The shared fixture for every coating-sequence suite: runs the bring-up once,
 * quiets every input that could move the state machine on its own, resets the
 * menu variables the sequence branches on, and clears the CAN logs. Call it
 * first in setUp(); then set up only what the test is about. */
void coat_test_begin(void);

/* Reset the coating sequence's cross-test state: the sequence keeps progress in
 * file-scope globals (StrokeNum, MoveCmdXmtd, odd_even_counter, ...) that
 * outlive a single test, and host_firmware_init() only runs once. Call from
 * setUp() so each test starts from the same place. */
void reset_coat_sequence_state(void);

/* Reassemble the Display() text the firmware pushed on WIM_ID (0x310) out of
 * can_tx_log. Returns the most recent complete message, or "" if none.
 * Display frames are STX/data/ETX chunks, which is awkward to assert on raw. */
const char *last_display_message(void);

/* Menu-variable setters. The refactored revisions have these in firmware; the
 * production 4.33 does not, so the harness supplies them using this firmware's
 * own idiom (strncpy + getvalue, value + getstrval). */
struct menu_var;
int update_menu_var_by_str(struct menu_var *var, const char *new_str);
int update_menu_var_by_value(struct menu_var *var, float new_value);

#endif /* TEST_SUPPORT_H */
