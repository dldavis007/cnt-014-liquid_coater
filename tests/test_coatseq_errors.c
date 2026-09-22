/* test_coatseq_errors.c
 *
 * Every route INTO an error state, and what each error state then does.
 * State-to-state dispatch for the normal path lives in test_coatseq_states.c.
 *
 * The three error states:
 *   ErrorState                 (99)  "Warn:TIMEOUT ERROR" -> FinishState
 *   HeadErrorState            (100)  "Warn:HEAD ERROR"    -> HdErrHomeState (101)
 *   PurgeRetractWaitErrorState(110)  "Warn:PURGE TIMEOUT" -> FinishState
 *
 * All three share the same post-conditions: stop the head, stop the pump.
 * That equivalence is the point of 110 existing - it is 99 with a message that
 * names the purge unit. Asserted head-to-head at the bottom of this file.
 *
 * Regression intent: every route asserted here behaves identically in 4.33
 * EXCEPT the PurgeRetractWait timeout, which went to ErrorState in 4.33 and to
 * PurgeRetractWaitErrorState in 4.34 (the 4.30a change). That single delta is
 * asserted in test_purge_retract.c and recorded in tests/REGRESSION.md.
 *
 * Note on turning things OFF: update_menu_var_by_str(&X,"OFF") works, because
 * "OFF" matches the enum token exactly. The "ON" direction is broken (see
 * REGRESSION.md); these suites use " ON" when they need something really on.
 */

#include "unity.h"
#include "test_support.h"

#include <string.h>
#include "mc9s12a128.h"
#include "Subroutines.h"
#include "Subroutines1.h"
#include "Interrupts.h"     /* RTI_One_Sec (built from Subroutines.h's OscClk) */

/* Rev4.33 declares these per-TU rather than in a header (they are defined in
 * Subroutines.c / Subroutines1.c), so each suite externs what it touches the
 * same way the firmware TUs do. */
extern char          State;
extern unsigned long StateTime;
extern unsigned int  TC0_RCVD_Data;
extern struct menu_var FirstStrokes, SecondStrokes, CleanOutStrokes;
extern struct menu_var RetractTime, PumpSpd, MaxLADist;
extern struct menu_var HeadOnOff, PumpOnOff;
extern char          ghostState;
extern char          Gen_Flags;
extern char          LAError;
extern float         HeadSpeed;
extern float         LAPos;
extern int           Update_Menu_Timer;
extern unsigned int  LAMoveTimer;
extern unsigned long LAMovingTimer;

extern float LAPos;

static unsigned long secs(double s) { return (unsigned long)(RTI_One_Sec * s); }

/* Put head and pump genuinely ON so an error state has something to stop. */
static void machine_running(void)
{
    update_menu_var_by_str(&HeadOnOff, " ON");
    update_menu_var_by_str(&PumpOnOff, " ON");
    can_tx_reset();
}

void setUp(void)    { coat_test_begin(); }
void tearDown(void) { }


/* ==========================================================================
 * Routes into ErrorState (99)
 * ========================================================================== */

/* TrigState: the actuator is still moving after 5 s. */
static void test_trigstate_actuator_stuck_moving_goes_to_error(void)
{
    State     = TrigState;
    set_actuator_moving(1);
    StateTime = secs(5) + 1;

    doevents();

    TEST_ASSERT_EQUAL_INT(ErrorState, State);
}

/* InitLAMove: nothing is configured to run. */
static void test_initlamove_with_no_strokes_configured_goes_to_error(void)
{
    State = InitLAMove;
    update_menu_var_by_value(&FirstStrokes,    0.0f);
    update_menu_var_by_value(&SecondStrokes,   0.0f);
    update_menu_var_by_value(&CleanOutStrokes, 0.0f);

    doevents();

    TEST_ASSERT_EQUAL_INT(ErrorState, State);
}

/* StartCoatState: the travel budget ran out (LAMovingTimer hit zero). */
static void test_startcoat_move_took_too_long_goes_to_error(void)
{
    State = StartCoatState;
    set_actuator_moving(1);        /* hold: only the LAMovingTimer test can fire */
    LAMovingTimer = 0;

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(ErrorState, State,
        "StartCoatState routes to ErrorState when LAMovingTimer expires");
}

/* StopState: same travel-budget check on the way home. */
static void test_stopstate_move_took_too_long_goes_to_error(void)
{
    State = StopState;
    set_actuator_moving(1);
    LAMovingTimer = 0;

    doevents();

    TEST_ASSERT_EQUAL_INT(ErrorState, State);
}

/* FirstCoatState / SecondCoatState / CleanOutState all bail on LAError. */
static void test_firstcoat_laerror_goes_to_error(void)
{
    State     = FirstCoatState;
    LAError   = 1;
    HeadSpeed = 8000.0f;        /* head healthy, so only LAError can route */

    doevents();

    TEST_ASSERT_EQUAL_INT(ErrorState, State);
}

static void test_secondcoat_laerror_goes_to_error(void)
{
    State     = SecondCoatState;
    LAError   = 1;
    HeadSpeed = 8000.0f;        /* head healthy, so only LAError can route */

    doevents();

    TEST_ASSERT_EQUAL_INT(ErrorState, State);
}

static void test_cleanout_laerror_goes_to_error(void)
{
    State   = CleanOutState;
    LAError = 1;

    doevents();

    TEST_ASSERT_EQUAL_INT(ErrorState, State);
}


/* ==========================================================================
 * Routes into HeadErrorState (100)
 * ========================================================================== */

/* CkHeadRotation: head never reached speed inside 5 s. */
static void test_ckheadrotation_timeout_goes_to_head_error(void)
{
    State     = CkHeadRotation;
    StateTime = secs(5) + 1;
    HeadSpeed = 0.0f;              /* still not spinning */

    doevents();

    TEST_ASSERT_EQUAL_INT(HeadErrorState, State);
}

/* FirstCoatState / SecondCoatState: the head slowed down mid-coat. */
static void test_firstcoat_head_slowdown_goes_to_head_error(void)
{
    State     = FirstCoatState;
    LAError   = 0;
    HeadSpeed = 4999.0f;

    doevents();

    TEST_ASSERT_EQUAL_INT(HeadErrorState, State);
}

static void test_secondcoat_head_slowdown_goes_to_head_error(void)
{
    State     = SecondCoatState;
    LAError   = 0;
    HeadSpeed = 4999.0f;

    doevents();

    TEST_ASSERT_EQUAL_INT(HeadErrorState, State);
}

/* PINNED ORDERING: in FirstCoatState both checks run, LAError first then head
 * speed. With BOTH true the head check wins, because it is assigned second. */
static void test_firstcoat_head_check_wins_when_both_faults_present(void)
{
    State     = FirstCoatState;
    LAError   = 1;
    HeadSpeed = 4999.0f;

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(HeadErrorState, State,
        "both faults: the later assignment (head) wins over LAError");
}


/* ==========================================================================
 * Error-state bodies
 * ========================================================================== */

static void test_errorstate_stops_everything_and_finishes(void)
{
    State     = ErrorState;
    StateTime = 1234;
    machine_running();

    doevents();

    TEST_ASSERT_EQUAL_STRING("Warn:TIMEOUT ERROR", last_display_message());
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, (int)HeadOnOff.value, "head off");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, (int)PumpOnOff.value, "pump off");
    TEST_ASSERT_EQUAL_INT(FinishState, State);
    TEST_ASSERT_EQUAL_UINT32(0, StateTime);
}

static void test_headerrorstate_stops_everything_and_homes(void)
{
    State     = HeadErrorState;
    StateTime = 1234;
    machine_running();

    doevents();

    TEST_ASSERT_EQUAL_STRING("Warn:HEAD ERROR", last_display_message());
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, (int)HeadOnOff.value, "head off");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, (int)PumpOnOff.value, "pump off");
    TEST_ASSERT_EQUAL_INT_MESSAGE(HdErrHomeState, State,
        "HeadErrorState continues into the head-error recovery, not FinishState");
    TEST_ASSERT_EQUAL_UINT32(0, StateTime);
}

static void test_purgeerrorstate_stops_everything_and_finishes(void)
{
    State     = PurgeRetractWaitErrorState;
    StateTime = 1234;
    machine_running();

    doevents();

    TEST_ASSERT_EQUAL_STRING("Warn:PURGE TIMEOUT", last_display_message());
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, (int)HeadOnOff.value, "head off");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, (int)PumpOnOff.value, "pump off");
    TEST_ASSERT_EQUAL_INT(FinishState, State);
    TEST_ASSERT_EQUAL_UINT32(0, StateTime);
}

/* The reason 110 was added: same effects as 99, different message. If a future
 * change makes them diverge in anything but the text, this fails. */
static void test_purge_error_and_generic_error_have_identical_effects(void)
{
    int  head_a, pump_a, state_a;
    int  head_b, pump_b, state_b;

    State = ErrorState;
    machine_running();
    doevents();
    head_a = (int)HeadOnOff.value; pump_a = (int)PumpOnOff.value; state_a = State;

    coat_test_begin();
    State = PurgeRetractWaitErrorState;
    machine_running();
    doevents();
    head_b = (int)HeadOnOff.value; pump_b = (int)PumpOnOff.value; state_b = State;

    TEST_ASSERT_EQUAL_INT_MESSAGE(head_a,  head_b,  "head result must match");
    TEST_ASSERT_EQUAL_INT_MESSAGE(pump_a,  pump_b,  "pump result must match");
    TEST_ASSERT_EQUAL_INT_MESSAGE(state_a, state_b, "next state must match");
}


/* ==========================================================================
 * Head-error recovery chain: 100 -> 101 -> 102 -> FinishState
 * ========================================================================== */

/* PINNED ODDITY: like HomeState, both branches land on the same state.
 * "Wasn't at home" does State++ (101 -> 102); "was already there" assigns
 * HdErrStopState (102). Shared with 4.33. */
static void test_hderrhome_both_branches_reach_hderrstop(void)
{
    State = HdErrHomeState;
    set_la_commanded_pos(50);
    doevents();
    TEST_ASSERT_EQUAL_INT_MESSAGE(HdErrStopState, State, "not-at-home branch");

    coat_test_begin();
    State = HdErrHomeState;
    set_la_commanded_pos(0);
    doevents();
    TEST_ASSERT_EQUAL_INT_MESSAGE(HdErrStopState, State, "already-home branch");
}

static void test_hderrhome_holds_while_actuator_moving(void)
{
    State = HdErrHomeState;
    set_actuator_moving(1);

    doevents();

    TEST_ASSERT_EQUAL_INT(HdErrHomeState, State);
}

/* HdErrStopState commands a full extend, then finishes. Reaching the "already
 * there" branch needs the commanded position to equal MaxLADist BEFORE the
 * MoveLA call overwrites it - which cannot happen, since MoveLA sets it to
 * MaxLADist*10 (tenths) and the comparison is against MaxLADist (inches).
 * So the "wasn't there" branch always wins: 102 -> 103 -> default -> Finish. */
static void test_hderrstop_commands_extend_and_recovers_to_finish(void)
{
    State = HdErrStopState;
    set_actuator_moving(0);

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE((int)(MaxLADist.value * 10), la_commanded_pos(),
        "HdErrStopState should command the actuator back out to MaxLADist");

    doevents();     /* 103 is unmapped -> default -> FinishState */
    TEST_ASSERT_EQUAL_INT(FinishState, State);
}

static void test_hderrstop_holds_while_actuator_moving(void)
{
    State = HdErrStopState;
    set_actuator_moving(1);

    doevents();

    TEST_ASSERT_EQUAL_INT(HdErrStopState, State);
}

/* End to end: a head fault from mid-coat runs the whole recovery to Finish. */
static void test_head_fault_recovers_all_the_way_to_finish(void)
{
    int guard = 0;

    State     = FirstCoatState;
    HeadSpeed = 0.0f;               /* head stalled */
    machine_running();
    set_actuator_moving(0);

    while (State != FinishState && guard++ < 10)
        doevents();

    TEST_ASSERT_TRUE_MESSAGE(guard < 10, "recovery should terminate");
    TEST_ASSERT_EQUAL_INT(FinishState, State);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, (int)HeadOnOff.value,
        "head must be off once the recovery has finished");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, (int)PumpOnOff.value,
        "pump must be off once the recovery has finished");
}


int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_trigstate_actuator_stuck_moving_goes_to_error);
    RUN_TEST(test_initlamove_with_no_strokes_configured_goes_to_error);
    RUN_TEST(test_startcoat_move_took_too_long_goes_to_error);
    RUN_TEST(test_stopstate_move_took_too_long_goes_to_error);
    RUN_TEST(test_firstcoat_laerror_goes_to_error);
    RUN_TEST(test_secondcoat_laerror_goes_to_error);
    RUN_TEST(test_cleanout_laerror_goes_to_error);

    RUN_TEST(test_ckheadrotation_timeout_goes_to_head_error);
    RUN_TEST(test_firstcoat_head_slowdown_goes_to_head_error);
    RUN_TEST(test_secondcoat_head_slowdown_goes_to_head_error);
    RUN_TEST(test_firstcoat_head_check_wins_when_both_faults_present);

    RUN_TEST(test_errorstate_stops_everything_and_finishes);
    RUN_TEST(test_headerrorstate_stops_everything_and_homes);
    RUN_TEST(test_purgeerrorstate_stops_everything_and_finishes);
    RUN_TEST(test_purge_error_and_generic_error_have_identical_effects);

    RUN_TEST(test_hderrhome_both_branches_reach_hderrstop);
    RUN_TEST(test_hderrhome_holds_while_actuator_moving);
    RUN_TEST(test_hderrstop_commands_extend_and_recovers_to_finish);
    RUN_TEST(test_hderrstop_holds_while_actuator_moving);
    RUN_TEST(test_head_fault_recovers_all_the_way_to_finish);

    return UNITY_END();
}
