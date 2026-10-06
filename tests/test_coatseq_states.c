/* test_coatseq_states.c
 *
 * Switch-dispatch coverage for the coating sequence: every state, every branch.
 * Error ROUTES and error-state bodies live in test_coatseq_errors.c; this suite
 * is about where each state sends you and what it sets on the way.
 *
 * Regression intent
 * -----------------
 * Carried over from the refactored 4.33 suite. Against the production 4.33
 * the only change needed was head/pump ON (see test_trigstate_turns_head_on):
 * production turns them on correctly; the refactored revisions regressed it.
 *
 * Several states deliberately execute BOTH of their ifs in one pass, which
 * produces outcomes that look like bugs and are easy to "fix" by accident.
 * Those are pinned here as they ARE, each with a comment saying so - that is
 * the point of a regression suite.
 *
 * Tests drive the machine through the shims in test_support.h, never through
 * 4.34's rpdo pointer names, so the suite can be pointed at 4.33.
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
extern int           OldPumpSpeed;

extern int   Update_Menu_Timer;
extern char  ghostState;
extern float LAPos;
extern char  Cycle_Complete;

/* The head tachometer reads "up to speed"; CkHeadRotation wants > 5000. */
#define HEAD_AT_SPEED   8000.0f
#define HEAD_TOO_SLOW      0.0f

static unsigned long secs(double s) { return (unsigned long)(RTI_One_Sec * s); }

void setUp(void)    { coat_test_begin(); }
void tearDown(void) { }


/* ==========================================================================
 * TrigState (1) - entry. Arms the cycle, turns the head on, tells the purge
 * unit to retract, then waits for the actuator to be stopped.
 * ========================================================================== */

static void test_trigstate_arms_cycle_and_turns_head_on(void)
{
    State     = TrigState;
    LAError   = 1;              /* stale from a previous cycle */
    HeadSpeed = HEAD_AT_SPEED;
    Cycle_Complete = 1;

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, LAError, "TrigState should clear LAError");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, HeadSpeed,
        "TrigState should zero HeadSpeed so CkHeadRotation measures afresh");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, Cycle_Complete,
        "TrigState should clear Cycle_Complete");
}

/* Production 4.33 turns the head on with strncpy(str_value," ON") + getvalue,
 * which matches the " ON" enum token, so .value and the ramp test's
 * `!strcmp(HeadOnOff.str_value," ON")` both see ON.
 *
 * The refactored 4.33 / 4.34 replaced this with update_menu_var_by_str(&X,"ON")
 * - no leading space, so nothing matches and the head never spins up. Their
 * suites pin that as test_KNOWN_BUG_trigstate_head_on_is_a_no_op: it is a
 * regression introduced AFTER this production release. */
static void test_trigstate_turns_head_on(void)
{
    State = TrigState;
    update_menu_var_by_str(&HeadOnOff, "OFF");

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, (int)HeadOnOff.value, "head should be ON");
    TEST_ASSERT_EQUAL_STRING(" ON", HeadOnOff.str_value);
}

/* The same call with the correctly-spaced token does work - this is the fix. */
static void test_head_on_works_with_the_spaced_enum_token(void)
{
    update_menu_var_by_str(&HeadOnOff, "OFF");
    update_menu_var_by_str(&HeadOnOff, " ON");

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, (int)HeadOnOff.value,
        "\" ON\" matches the enum token, so .value tracks");
    TEST_ASSERT_EQUAL_STRING(" ON", HeadOnOff.str_value);
}

static void test_trigstate_commands_purge_retract(void)
{
    unsigned i;
    int found = 0;

    State = TrigState;
    doevents();

    for (i = 0; i < can_tx_count && i < CAN_TX_LOG_N; i++)
        if (can_tx_log[i].ID == 0x361 && can_tx_log[i].LEN == 1
            && can_tx_log[i].BUF[0] == 2)
            found = 1;

    TEST_ASSERT_TRUE_MESSAGE(found,
        "TrigState should send 0x361 [2] to retract the purge unit");
}

static void test_trigstate_displays_coating(void)
{
    State = TrigState;
    doevents();
    TEST_ASSERT_EQUAL_STRING("Proc:Coating", last_display_message());
}

static void test_trigstate_advances_when_actuator_stopped(void)
{
    State = TrigState;
    set_actuator_moving(0);
    StateTime = 123;

    doevents();

    TEST_ASSERT_EQUAL_INT(CkHeadRotation, State);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, StateTime,
        "advancing out of TrigState should restart the state clock");
}

static void test_trigstate_holds_while_actuator_moving(void)
{
    State = TrigState;
    set_actuator_moving(1);
    StateTime = secs(1);

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(TrigState, State,
        "should wait in TrigState while the actuator is still moving");
}


/* ==========================================================================
 * CkHeadRotation (2) - wait for the head to spin up, 5 s limit.
 * ========================================================================== */

static void test_ckheadrotation_advances_once_up_to_speed(void)
{
    State     = CkHeadRotation;
    StateTime = secs(1);
    HeadSpeed = HEAD_AT_SPEED;

    doevents();

    TEST_ASSERT_EQUAL_INT(PurgeRetract, State);
    TEST_ASSERT_EQUAL_UINT32(0, StateTime);
}

static void test_ckheadrotation_holds_below_speed_threshold(void)
{
    State     = CkHeadRotation;
    StateTime = secs(1);
    HeadSpeed = 5000.0f;        /* threshold is STRICTLY greater than 5000 */

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(CkHeadRotation, State,
        "HeadSpeed == 5000 is not 'up to speed'; the test is > 5000");
}

/* PINNED ODDITY: both ifs run. At the timeout with the head ALSO up to speed,
 * the first if sets HeadErrorState (100) and the second then does State++,
 * landing on 101 (HdErrHomeState) - so HeadErrorState's body (display, head
 * off, pump off) never executes. Shared with 4.33. */
static void test_ckheadrotation_timeout_and_at_speed_skips_error_body(void)
{
    State     = CkHeadRotation;
    StateTime = secs(5) + 1;
    HeadSpeed = HEAD_AT_SPEED;
    update_menu_var_by_str(&HeadOnOff, " ON");   /* spaced token: actually ON */
    can_tx_reset();

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(HdErrHomeState, State,
        "timeout + at-speed in the same pass falls through HeadErrorState to 101");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", last_display_message(),
        "HeadErrorState's body is skipped, so no warning is displayed");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, (int)HeadOnOff.value,
        "and the head is left running, because that body never ran");
}


/* ==========================================================================
 * PurgeRetract (3) / PurgeRetractWait (4)
 * Timeout behaviour of state 4 is covered in test_purge_retract.c.
 * ========================================================================== */

static void test_purgeretract_holds_during_2s_settle(void)
{
    State     = PurgeRetract;
    StateTime = secs(1);
    set_purge_moving(0);

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(PurgeRetract, State,
        "no decision before the 2 s settle has elapsed");
}

static void test_purgeretract_skips_wait_when_already_retracted(void)
{
    State     = PurgeRetract;
    StateTime = secs(2) + 1;
    set_purge_moving(0);

    doevents();

    TEST_ASSERT_EQUAL_INT(InitLAMove, State);
}

static void test_purgeretract_enters_wait_when_still_moving(void)
{
    State     = PurgeRetract;
    StateTime = secs(2) + 1;
    set_purge_moving(1);

    doevents();

    TEST_ASSERT_EQUAL_INT(PurgeRetractWait, State);
}


/* ==========================================================================
 * InitLAMove (5) - the fan-out. Picks which coat stage runs, or bails.
 * ========================================================================== */

static void test_initlamove_holds_while_actuator_moving(void)
{
    State = InitLAMove;
    set_actuator_moving(1);

    doevents();

    TEST_ASSERT_EQUAL_INT(InitLAMove, State);
}

static void test_initlamove_goes_home_when_head_not_extended(void)
{
    State = InitLAMove;
    LAPos = MaxLADist.value - 1.0f;     /* not fully extended */

    doevents();

    TEST_ASSERT_EQUAL_INT(HomeState, State);
    TEST_ASSERT_EQUAL_STRING("Proc:Head Not Extended", last_display_message());
}

static void test_initlamove_picks_first_coat_when_configured(void)
{
    State = InitLAMove;
    update_menu_var_by_value(&FirstStrokes, 5.0f);

    doevents();

    TEST_ASSERT_EQUAL_INT(StartCoatState, State);
}

static void test_initlamove_falls_through_to_second_coat(void)
{
    State = InitLAMove;
    update_menu_var_by_value(&FirstStrokes,  0.0f);
    update_menu_var_by_value(&SecondStrokes, 10.0f);

    doevents();

    TEST_ASSERT_EQUAL_INT(StartSecondCoatState, State);
}

static void test_initlamove_falls_through_to_cleanout(void)
{
    State = InitLAMove;
    update_menu_var_by_value(&FirstStrokes,     0.0f);
    update_menu_var_by_value(&SecondStrokes,    0.0f);
    update_menu_var_by_value(&CleanOutStrokes,  5.0f);

    doevents();

    TEST_ASSERT_EQUAL_INT(StartCleanOutState, State);
}

static void test_initlamove_errors_when_nothing_configured(void)
{
    State = InitLAMove;
    update_menu_var_by_value(&FirstStrokes,    0.0f);
    update_menu_var_by_value(&SecondStrokes,   0.0f);
    update_menu_var_by_value(&CleanOutStrokes, 0.0f);

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(ErrorState, State,
        "nothing to do: InitLAMove should route to ErrorState");
}

static void test_initlamove_arms_the_move_timer(void)
{
    State = InitLAMove;
    LAMoveTimer = 0;

    doevents();

    TEST_ASSERT_EQUAL_UINT_MESSAGE((unsigned)LAMoveTime, LAMoveTimer,
        "InitLAMove should arm LAMoveTimer for the move that follows");
}


/* ==========================================================================
 * StartCoatState (6) / StartSecondCoatState (8) / StartCleanOutState (10)
 * ========================================================================== */

static void test_startcoat_enters_first_coat_when_strokes_set(void)
{
    State = StartCoatState;
    update_menu_var_by_value(&FirstStrokes, 5.0f);

    doevents();

    TEST_ASSERT_EQUAL_INT(FirstCoatState, State);
    TEST_ASSERT_EQUAL_UINT32(0, StateTime);
}

static void test_startcoat_skips_to_second_when_no_strokes(void)
{
    State = StartCoatState;
    update_menu_var_by_value(&FirstStrokes, 0.0f);

    doevents();

    TEST_ASSERT_EQUAL_INT(StartSecondCoatState, State);
}

static void test_startcoat_holds_while_actuator_moving(void)
{
    State = StartCoatState;
    set_actuator_moving(1);

    doevents();

    TEST_ASSERT_EQUAL_INT(StartCoatState, State);
}

static void test_startsecondcoat_enters_second_coat_when_strokes_set(void)
{
    State = StartSecondCoatState;
    update_menu_var_by_value(&SecondStrokes, 10.0f);

    doevents();

    TEST_ASSERT_EQUAL_INT(SecondCoatState, State);
}

static void test_startsecondcoat_skips_to_cleanout_when_no_strokes(void)
{
    State = StartSecondCoatState;
    update_menu_var_by_value(&SecondStrokes, 0.0f);

    doevents();

    TEST_ASSERT_EQUAL_INT(StartCleanOutState, State);
}

/* RetractTime != 0 makes the clean-out stage pre-spin the pump at 75%. */
static void test_startcleanout_primes_pump_when_retract_time_set(void)
{
    State = StartCleanOutState;
    update_menu_var_by_value(&RetractTime, 2.0f);
    update_menu_var_by_value(&PumpSpd,   100.0f);
    set_actuator_moving(1);         /* hold in-state so we see the priming only */

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(75, (int)PumpSpd.value,
        "RetractTime set: clean-out should drop the pump to 75%");
    /* Production marks the pump ON (strncpy " ON" + getvalue); the refactored
     * revisions' by_str("ON") regression leaves it OFF - see the head test. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, (int)PumpOnOff.value, "pump should be ON");
    TEST_ASSERT_EQUAL_INT_MESSAGE(100, OldPumpSpeed,
        "the previous pump speed should be saved for restoration");
}

static void test_startcleanout_leaves_pump_alone_without_retract_time(void)
{
    State = StartCleanOutState;
    update_menu_var_by_value(&RetractTime, 0.0f);
    update_menu_var_by_value(&PumpSpd,   100.0f);
    set_actuator_moving(1);

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(100, (int)PumpSpd.value,
        "RetractTime zero: pump speed untouched");
}

static void test_startcleanout_advances_and_stops_pump(void)
{
    State = StartCleanOutState;
    set_actuator_moving(0);

    doevents();

    TEST_ASSERT_EQUAL_INT(CleanOutState, State);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, (int)PumpOnOff.value,
        "clean-out turns the pump back OFF once the stroke is armed");
}


/* ==========================================================================
 * HomeState (12) / StopState (13) / CoatingComplete (14) / StopWaitState (15)
 * ========================================================================== */

static void test_homestate_turns_head_off(void)
{
    State = HomeState;
    update_menu_var_by_str(&HeadOnOff, "ON");
    set_actuator_moving(1);         /* hold in-state */

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, (int)HeadOnOff.value,
        "HomeState should turn the head OFF");
}

/* PINNED ODDITY: both branches land on the same state. "Wasn't at home" does
 * State++ (12 -> 13) and "was already there" assigns StopState (13). The
 * distinction has no effect. Shared with 4.33. */
static void test_homestate_both_branches_reach_stopstate(void)
{
    State = HomeState;
    set_la_commanded_pos(50);       /* not at home */
    doevents();
    TEST_ASSERT_EQUAL_INT_MESSAGE(StopState, State, "not-at-home branch");

    coat_test_begin();
    State = HomeState;
    set_la_commanded_pos(0);        /* already home */
    doevents();
    TEST_ASSERT_EQUAL_INT_MESSAGE(StopState, State, "already-home branch");
}

static void test_stopstate_commands_full_extend_and_completes(void)
{
    State = StopState;
    set_actuator_moving(0);

    doevents();

    TEST_ASSERT_EQUAL_INT(CoatingComplete, State);
    TEST_ASSERT_EQUAL_INT_MESSAGE((int)(MaxLADist.value * 10), la_commanded_pos(),
        "StopState should command the actuator back out to MaxLADist");
}

static void test_stopstate_holds_while_actuator_moving(void)
{
    State = StopState;
    set_actuator_moving(1);

    doevents();

    TEST_ASSERT_EQUAL_INT(StopState, State);
}

static void test_coatingcomplete_waits_one_second_then_reports(void)
{
    State     = CoatingComplete;
    StateTime = secs(0.5);
    doevents();
    TEST_ASSERT_EQUAL_INT_MESSAGE(CoatingComplete, State, "still inside the 1 s hold");

    StateTime = secs(1) + 1;
    doevents();
    TEST_ASSERT_EQUAL_INT(StopWaitState, State);
    TEST_ASSERT_EQUAL_STRING("Proc:Coating Complete", last_display_message());
    TEST_ASSERT_EQUAL_UINT32(0, StateTime);
}

/* PINNED ODDITY: StopWaitState's 15 s timeout is unreachable. doevents() does
 * `if (1) Cycle_Complete = 1;` at the top of EVERY pass, so the second if
 * always fires first and the state advances immediately. The 4.30a source
 * carries a TODO about exactly this. Shared with 4.33. */
static void test_stopwait_advances_immediately_because_cycle_complete_is_forced(void)
{
    State     = StopWaitState;
    StateTime = 0;

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(FinishState, State,
        "Cycle_Complete is forced true every pass, so StopWaitState never waits");
}

static void test_finishstate_is_stable(void)
{
    State = FinishState;
    doevents();
    doevents();
    TEST_ASSERT_EQUAL_INT_MESSAGE(FinishState, State, "FinishState should self-loop");
}

/* An unmapped state value falls to `default:` and recovers to FinishState. */
static void test_unknown_state_recovers_to_finish(void)
{
    State = 77;
    doevents();
    TEST_ASSERT_EQUAL_INT(FinishState, State);
}


int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_trigstate_arms_cycle_and_turns_head_on);
    RUN_TEST(test_trigstate_turns_head_on);
    RUN_TEST(test_head_on_works_with_the_spaced_enum_token);
    RUN_TEST(test_trigstate_commands_purge_retract);
    RUN_TEST(test_trigstate_displays_coating);
    RUN_TEST(test_trigstate_advances_when_actuator_stopped);
    RUN_TEST(test_trigstate_holds_while_actuator_moving);

    RUN_TEST(test_ckheadrotation_advances_once_up_to_speed);
    RUN_TEST(test_ckheadrotation_holds_below_speed_threshold);
    RUN_TEST(test_ckheadrotation_timeout_and_at_speed_skips_error_body);

    RUN_TEST(test_purgeretract_holds_during_2s_settle);
    RUN_TEST(test_purgeretract_skips_wait_when_already_retracted);
    RUN_TEST(test_purgeretract_enters_wait_when_still_moving);

    RUN_TEST(test_initlamove_holds_while_actuator_moving);
    RUN_TEST(test_initlamove_goes_home_when_head_not_extended);
    RUN_TEST(test_initlamove_picks_first_coat_when_configured);
    RUN_TEST(test_initlamove_falls_through_to_second_coat);
    RUN_TEST(test_initlamove_falls_through_to_cleanout);
    RUN_TEST(test_initlamove_errors_when_nothing_configured);
    RUN_TEST(test_initlamove_arms_the_move_timer);

    RUN_TEST(test_startcoat_enters_first_coat_when_strokes_set);
    RUN_TEST(test_startcoat_skips_to_second_when_no_strokes);
    RUN_TEST(test_startcoat_holds_while_actuator_moving);
    RUN_TEST(test_startsecondcoat_enters_second_coat_when_strokes_set);
    RUN_TEST(test_startsecondcoat_skips_to_cleanout_when_no_strokes);
    RUN_TEST(test_startcleanout_primes_pump_when_retract_time_set);
    RUN_TEST(test_startcleanout_leaves_pump_alone_without_retract_time);
    RUN_TEST(test_startcleanout_advances_and_stops_pump);

    RUN_TEST(test_homestate_turns_head_off);
    RUN_TEST(test_homestate_both_branches_reach_stopstate);
    RUN_TEST(test_stopstate_commands_full_extend_and_completes);
    RUN_TEST(test_stopstate_holds_while_actuator_moving);
    RUN_TEST(test_coatingcomplete_waits_one_second_then_reports);
    RUN_TEST(test_stopwait_advances_immediately_because_cycle_complete_is_forced);
    RUN_TEST(test_finishstate_is_stable);
    RUN_TEST(test_unknown_state_recovers_to_finish);

    return UNITY_END();
}
