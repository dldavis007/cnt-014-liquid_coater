/* test_purge_retract.c
 *
 * Covers the purge-retract path of the coating sequence, ported from
 * Rev 4.30a's test_doevents_state.c by way of the Rev4.34 suite.
 *
 * What changed in 4.30a and is being verified here:
 *   - a 20 s PurgeRetractWait timeout now routes to PurgeRetractWaitErrorState
 *     (110) instead of the generic ErrorState (99), so the operator sees
 *     "Warn:PURGE TIMEOUT" rather than "Warn:TIMEOUT ERROR"
 *   - the new state stops head and pump and returns to FinishState — the same
 *     post-conditions as ErrorState, which the last group asserts directly so
 *     the two are shown to be behaviourally equivalent.
 *
 * Rev4.33 signal mapping: the purge-unit "moving" byte is gProcImg[IN_digi_31]
 * and the actuator-moving byte is gProcImg[OUT_digi_7]. The suite never names
 * either — it goes through set_purge_moving() / set_actuator_moving() in
 * test_support.h, which is what made this a port of the fixture rather than a
 * rewrite of the tests. (Rev4.34 reaches the same two bytes as
 * *rpdo7_purge_moving and *rpdo4_actuator_moving.)
 *
 * Rev4.33 also differs in that PurgeRetractWaitErrorState stops the head and
 * pump through update_menu_var_by_str(&X, "OFF") rather than writing .value
 * directly, matching its sibling ErrorState/HeadErrorState in Subroutines1.c.
 * Both land on .value == 1 (the enum is "OFF, ON", 1-based), so the assertions
 * below are unchanged from the 4.34 suite.
 *
 * Isolation: doevents() does a full pass (menu, serialization, CANopen) before
 * reaching switch(State). setUp() quiets every input path that could move the
 * state machine on its own, so each test observes only the case under test.
 * Note the Timer1 pacing service (test_support.h) runs throughout: it is what
 * lets the Display() calls inside these states return, and it deliberately does
 * NOT advance StateTime, so the StateTime assertions below stay exact.
 */

#include "unity.h"
#include "test_support.h"

#include <string.h>
#include "mc9s12a128.h"
#include "Subroutines.h"
#include "Subroutines1.h"
#include "Interrupts.h"     /* RTI_One_Sec (built from Subroutines.h's OscClk) */

extern unsigned char sfr_regs[0x400];

/* Rev4.33 declares these per-TU rather than in a header. */
extern char          State;
extern char          ghostState;
extern char          Gen_Flags;
extern char          LAError;
extern unsigned int  TC0_RCVD_Data;
extern unsigned long StateTime;
extern int           Update_Menu_Timer;
extern float         LAPos;
extern struct menu_var MaxLADist, HeadOnOff, PumpOnOff;

/* StateTime past the 20 s PurgeRetractWait limit. */
static unsigned long purge_timeout(void)
{
    return (unsigned long)(RTI_One_Sec * 20) + 1;
}

/* Turn an on/off menu var ON, setting BOTH fields.
 * Setting .value alone is not a valid machine state and would not exercise the
 * error paths: update_menu_var_by_str() (used by ErrorState/HeadErrorState and
 * by PurgeRetractWaitErrorState) early-returns when str_value already matches
 * its target, so it would never touch .value. The enum is "OFF, ON", hence the
 * leading space on " ON". */
static void set_on(struct menu_var *v)
{
    strncpy(v->str_value, " ON", v->len_str);
    v->value = 2;
}

void setUp(void)
{
    host_firmware_init();
    can_tx_reset();
    can_rx_reset();
    memset(sfr_regs, 0, sizeof sfr_regs);

    /* ATD0STAT0 bit 7 (SCF, sequence complete): satisfies any ATDGetLevel()
     * poll immediately if one is reached. */
    sfr_regs[0x86] = 0x80;

    TC0_RCVD_Data = 0;              /* no telemetry: camera/cursor paths idle */
    ghostState    = 0;              /* throwGhost() returns on case 0         */

    set_actuator_moving(0);         /* LA stopped: PurgeRetract/InitLAMove free to run */
    set_purge_moving(0);

    State     = FinishState;
    StateTime = 0;

    LAPos = MaxLADist.value;        /* so InitLAMove goes forward, not HomeState */

    set_on(&HeadOnOff);             /* head and pump ON, so error states have */
    set_on(&PumpOnOff);             /* something to stop                      */

    Gen_Flags = 0;
    LAError   = 0;
    Update_Menu_Timer = 0;
}

void tearDown(void) { }

/* ============================================================
 * PurgeRetractWait (4) transitions
 * ============================================================ */

/* Purge reports stopped before the timeout: advance to InitLAMove. */
static void test_wait_success_advances_to_init_la_move(void)
{
    State     = PurgeRetractWait;
    StateTime = (unsigned long)RTI_One_Sec;   /* well within 20 s */
    set_purge_moving(0);                      /* retracted        */

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(InitLAMove, State,
        "purge retracted in time: should advance to InitLAMove");
}

static void test_wait_success_resets_state_time(void)
{
    State     = PurgeRetractWait;
    StateTime = 500;
    set_purge_moving(0);

    doevents();

    TEST_ASSERT_EQUAL_INT(InitLAMove, State);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, StateTime,
        "StateTime should be cleared on successful retract");
}

/* Still moving and still inside the window: hold. */
static void test_wait_holds_while_purge_still_moving(void)
{
    State     = PurgeRetractWait;
    StateTime = (unsigned long)RTI_One_Sec;
    set_purge_moving(1);

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(PurgeRetractWait, State,
        "should stay in PurgeRetractWait while purge moves and timeout is not reached");
}

/* THE 4.30a CHANGE: timeout routes to the dedicated purge error state. */
static void test_wait_timeout_routes_to_purge_error_state(void)
{
    State     = PurgeRetractWait;
    StateTime = purge_timeout();
    set_purge_moving(1);            /* still moving, so the success path cannot fire */

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(PurgeRetractWaitErrorState, State,
        "20 s timeout should route to PurgeRetractWaitErrorState (110), not ErrorState (99)");
}

/* ============================================================
 * PurgeRetractWaitErrorState (110) post-conditions
 * ============================================================ */

static void test_purge_error_state_stops_head_and_pump_and_finishes(void)
{
    State     = PurgeRetractWaitErrorState;
    StateTime = 1234;
    set_on(&HeadOnOff);             /* head was ON */
    set_on(&PumpOnOff);             /* pump was ON */

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(FinishState, State,
        "PurgeRetractWaitErrorState should return to FinishState");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, (int)HeadOnOff.value,
        "PurgeRetractWaitErrorState should stop the head (value 1 == OFF)");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, (int)PumpOnOff.value,
        "PurgeRetractWaitErrorState should stop the pump (value 1 == OFF)");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, StateTime,
        "PurgeRetractWaitErrorState should clear StateTime");
}

/* The operator-visible half of the change: a distinct message, so a purge that
 * never retracted is not reported as a generic timeout. */
static void test_purge_error_state_displays_purge_timeout(void)
{
    State     = PurgeRetractWaitErrorState;
    StateTime = 1234;
    can_tx_reset();

    doevents();

    TEST_ASSERT_EQUAL_STRING_MESSAGE("Warn:PURGE TIMEOUT", last_display_message(),
        "PurgeRetractWaitErrorState should display its own message, "
        "not ErrorState's \"Warn:TIMEOUT ERROR\"");
}

/* End-to-end: timeout, then the error state runs on the following pass. */
static void test_timeout_then_error_state_completes_in_two_passes(void)
{
    State     = PurgeRetractWait;
    StateTime = purge_timeout();
    set_purge_moving(1);

    doevents();
    TEST_ASSERT_EQUAL_INT_MESSAGE(PurgeRetractWaitErrorState, State,
        "first pass: timeout routes to PurgeRetractWaitErrorState");

    doevents();
    TEST_ASSERT_EQUAL_INT_MESSAGE(FinishState, State,
        "second pass: error state returns to FinishState");
    TEST_ASSERT_EQUAL_INT(1, (int)HeadOnOff.value);
    TEST_ASSERT_EQUAL_INT(1, (int)PumpOnOff.value);
}

/* ============================================================
 * ErrorState (99) — same post-conditions, different message.
 * Passing this alongside the group above is what shows the new state is
 * behaviourally equivalent to the generic one it replaced.
 * ============================================================ */

static void test_generic_error_state_has_same_post_conditions(void)
{
    State     = ErrorState;
    StateTime = 1234;
    set_on(&HeadOnOff);
    set_on(&PumpOnOff);

    doevents();

    TEST_ASSERT_EQUAL_INT(FinishState, State);
    TEST_ASSERT_EQUAL_INT(1, (int)HeadOnOff.value);
    TEST_ASSERT_EQUAL_INT(1, (int)PumpOnOff.value);
    TEST_ASSERT_EQUAL_UINT32(0, StateTime);
}

/* ============================================================
 * PurgeRetract (3) — the state that feeds PurgeRetractWait.
 * After its 2 s settle it either skips straight to InitLAMove (already
 * retracted) or falls through to PurgeRetractWait (still retracting).
 * ============================================================ */

static void test_purge_retract_skips_wait_when_already_retracted(void)
{
    State     = PurgeRetract;
    StateTime = (unsigned long)(RTI_One_Sec * 2) + 1;
    set_purge_moving(0);

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(InitLAMove, State,
        "already retracted after the 2 s settle: should skip PurgeRetractWait");
}

static void test_purge_retract_enters_wait_when_still_moving(void)
{
    State     = PurgeRetract;
    StateTime = (unsigned long)(RTI_One_Sec * 2) + 1;
    set_purge_moving(1);

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(PurgeRetractWait, State,
        "still retracting after the 2 s settle: should enter PurgeRetractWait");
}

static void test_purge_retract_holds_during_settle_window(void)
{
    State     = PurgeRetract;
    StateTime = (unsigned long)(RTI_One_Sec);   /* inside the 2 s window */
    set_purge_moving(0);

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(PurgeRetract, State,
        "should hold in PurgeRetract until the 2 s settle has elapsed");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_wait_success_advances_to_init_la_move);
    RUN_TEST(test_wait_success_resets_state_time);
    RUN_TEST(test_wait_holds_while_purge_still_moving);
    RUN_TEST(test_wait_timeout_routes_to_purge_error_state);
    RUN_TEST(test_purge_error_state_stops_head_and_pump_and_finishes);
    RUN_TEST(test_purge_error_state_displays_purge_timeout);
    RUN_TEST(test_timeout_then_error_state_completes_in_two_passes);
    RUN_TEST(test_generic_error_state_has_same_post_conditions);
    RUN_TEST(test_purge_retract_skips_wait_when_already_retracted);
    RUN_TEST(test_purge_retract_enters_wait_when_still_moving);
    RUN_TEST(test_purge_retract_holds_during_settle_window);
    return UNITY_END();
}
