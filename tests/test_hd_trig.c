/* test_hd_trig.c
 *
 * TrigRequest() and the HD trig-mode handshake it drives (Subroutines1.c).
 *
 * SD mode (HDSDSetting == 1): a trig request with a camera on starts the
 * coating sequence immediately.
 *
 * HD mode (HDSDSetting == 2): the unit cannot know whether the active camera is
 * trig'd to IT or to another coater, so a trig request instead transmits a
 * 0x521 query - BUF[0]=0x10, BUF[1..2]=activeCamAddress, BUF[3]=NODE_ID - and
 * arms a 5 s window. The camera answers on 0x521 (RPDO6 -> camera_cmds[0..4]),
 * setting bit 0x20 with the owning node in camera_cmds[3]. A matching answer
 * starts the sequence; silence, or an answer naming another node, times out.
 *
 * The window is driven through set_mco_time(), which opts this suite into the
 * stubs' real wraparound compare - by default MCOHW_IsTimeExpired() is pinned
 * to 1 for the other suites. Nothing here uses wall-clock time.
 */

#include "unity.h"
#include "test_support.h"

#include <stdio.h>
#include <string.h>
#include "mc9s12a128.h"
#include "Subroutines.h"
#include "Subroutines1.h"
#include "Interrupts.h"

/* Rev4.33 declares its globals per-TU rather than in a header, so each suite
 * externs what it touches the same way the firmware TUs do. */
extern char State;
extern char ghostState;
extern struct menu_var HDSDSetting;
extern unsigned int activeCamAddress;
extern bool trig_query_sent;
extern UNSIGNED16 trig_query_timer;

#define CAM_ADDR    0x0042      /* under 100: see test_cameras.c on Timer1 */
#define OTHER_NODE  0x55        /* any node id that is not ours */
#define T0          0x1000      /* arbitrary arm time, far from wraparound */

/* Arm the handshake from a known start: HD mode, idle, camera addressed,
 * clock at T0. Leaves trig_query_sent set and one 0x521 in the TX log. */
static void arm_query(void)
{
    set_mco_time(T0);
    HDSDSetting.value = HDSD_HD;
    State      = FinishState;
    ghostState = 0;
    activeCamAddress = CAM_ADDR;
    can_tx_reset();
    menu_data[0] |= 0x02;
    TrigRequest();
}

/* The camera's answer: 0x20 with the owning node in byte 3. */
static void answer_from(unsigned char node)
{
    camera_cmds[0] |= 0x20;
    camera_cmds[3]  = node;
}

/* Index of the 0x521 query in the TX log, or -1. */
static int query_index(void)
{
    unsigned i;
    for (i = 0; i < can_tx_count && i < CAN_TX_LOG_N; i++)
        if (can_tx_log[i].ID == 0x521 && can_tx_log[i].LEN >= 4
            && can_tx_log[i].BUF[0] == 0x10)
            return (int)i;
    return -1;
}

void setUp(void)
{
    coat_test_begin();
    clear_mco_time();
    trig_query_sent  = false;
    trig_query_timer = 0;
    activeCamAddress = 0;
    ghostState       = 0;
    State            = FinishState;
    menu_data[0]     = 0;
    camera_cmds[0] = camera_cmds[1] = camera_cmds[2] = 0;
    camera_cmds[3] = camera_cmds[4] = 0;
    can_tx_reset();
}

void tearDown(void) { clear_mco_time(); }


/* ==========================================================================
 * HDSDMode - the float guard
 * ========================================================================== */

static void test_hdsd_mode_reads_whole_numbers(void)
{
    HDSDSetting.value = 1.0f;
    TEST_ASSERT_EQUAL_INT(1, HDSDMode());
    HDSDSetting.value = 2.0f;
    TEST_ASSERT_EQUAL_INT(2, HDSDMode());
}

/* The menu stores .value as a float with inc 1, so it should always be
 * integral. This pins the rounding so a future fractional increment (or a
 * float that lands at 1.9999) cannot silently fail every == compare and
 * disable the HD path. */
static void test_hdsd_mode_rounds_a_drifted_float(void)
{
    HDSDSetting.value = 1.9999f;
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, HDSDMode(),
        "a value just under 2 must still read as HD, not fall through to 1");
}


/* ==========================================================================
 * SD mode
 * ========================================================================== */

static void test_sd_trig_starts_the_sequence_without_a_query(void)
{
    HDSDSetting.value = 1.0f;
    VSEL_PORT |= CAM_ON;
    set_actuator_moving(1);
    menu_data[0] |= 0x02;

    TEST_ASSERT_EQUAL_INT(TRIG_STARTED, TrigRequest());

    TEST_ASSERT_EQUAL_INT_MESSAGE(TrigState, State, "SD trig starts the sequence");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, can_tx_count, "SD must not query the camera");
}

static void test_sd_trig_ignored_with_no_camera_on(void)
{
    HDSDSetting.value = 1.0f;
    VSEL_PORT &= ~CAM_ON;
    menu_data[0] |= 0x02;

    TEST_ASSERT_EQUAL_INT(TRIG_NOCAM, TrigRequest());

    TEST_ASSERT_EQUAL_INT_MESSAGE(FinishState, State,
        "no camera on: the trigger must not start the sequence");
}

/* HDSDSetting ranges 1..5; only 1 and 2 have a trig path. A mode with neither
 * must consume the request and do nothing - NOT fall through to the SD start. */
static void test_trig_ignored_in_a_mode_with_no_trig_path(void)
{
    HDSDSetting.value = 3.0f;
    VSEL_PORT |= CAM_ON;
    menu_data[0] |= 0x02;

    TEST_ASSERT_EQUAL_INT(TRIG_IDLE, TrigRequest());

    TEST_ASSERT_EQUAL_INT_MESSAGE(FinishState, State,
        "mode 3 has no trig path; the sequence must not start");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, can_tx_count, "and nothing is queried");
}

/* A request while the sequence is already running is consumed, not queued. */
static void test_trig_while_busy_is_rejected(void)
{
    HDSDSetting.value = 1.0f;
    VSEL_PORT |= CAM_ON;
    State = TrigState;
    menu_data[0] |= 0x02;

    TEST_ASSERT_EQUAL_INT(TRIG_NOTIDLE, TrigRequest());

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, menu_data[0] & 0x02,
        "the request is consumed even when it cannot be honoured");
}


/* ==========================================================================
 * HD mode - the query
 * ========================================================================== */

static void test_hd_trig_transmits_the_query_and_waits(void)
{
    int q;

    arm_query();

    q = query_index();
    TEST_ASSERT_TRUE_MESSAGE(q >= 0, "HD trig should transmit a 0x521 query");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(CAM_ADDR & 0xFF, can_tx_log[q].BUF[1],
        "query carries the active camera address, low byte first");
    TEST_ASSERT_EQUAL_HEX8((CAM_ADDR >> 8) & 0xFF, can_tx_log[q].BUF[2]);
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(NODE_ID, can_tx_log[q].BUF[3],
        "query names this node so the camera can answer for it");
    TEST_ASSERT_TRUE_MESSAGE(trig_query_sent, "the window should be armed");
    TEST_ASSERT_EQUAL_INT_MESSAGE(FinishState, State,
        "HD must NOT start the sequence until the camera answers");
}

static void test_hd_trig_consumes_the_request_bit(void)
{
    arm_query();

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, menu_data[0] & 0x02,
        "the request is consumed so the query fires once, not every pass");
}

/* No camera has been addressed yet, so there is nobody to ask. Querying
 * address 0 would burn the full 5 s window waiting for an impossible answer. */
static void test_hd_trig_does_not_query_without_an_active_camera(void)
{
    set_mco_time(T0);
    HDSDSetting.value = HDSD_HD;
    activeCamAddress = 0;
    menu_data[0] |= 0x02;

    TEST_ASSERT_EQUAL_INT(TRIG_NOCAM, TrigRequest());

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, can_tx_count, "nothing to query");
    TEST_ASSERT_FALSE_MESSAGE(trig_query_sent, "no window should be armed");
    TEST_ASSERT_EQUAL_INT(FinishState, State);
}

/* An answer left over from a previous handshake must not satisfy the new one. */
static void test_hd_trig_clears_a_stale_response_when_arming(void)
{
    answer_from(NODE_ID);

    arm_query();

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, camera_cmds[0] & 0x20,
        "arming drops any answer that predates the query");
    TEST_ASSERT_EQUAL_INT_MESSAGE(FinishState, State,
        "a stale answer must not start the sequence");
}


/* ==========================================================================
 * HD mode - the response
 * ========================================================================== */

static void test_response_for_this_node_starts_the_sequence(void)
{
    arm_query();
    set_actuator_moving(1);

    answer_from(NODE_ID);
    TEST_ASSERT_EQUAL_INT(TRIG_STARTED, HDTrigQueryService());

    TEST_ASSERT_EQUAL_INT_MESSAGE(TrigState, State,
        "the camera named us, so the coating sequence starts");
    TEST_ASSERT_FALSE_MESSAGE(trig_query_sent, "the window should be closed");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, camera_cmds[0] & 0x20,
        "the response bit is consumed");
}

/* The query goes to the active camera, but 0x521 is a shared bus ID - the
 * answer has to be checked against NODE_ID or another coater's handshake would
 * start this unit's sequence. */
static void test_response_for_another_node_is_ignored(void)
{
    arm_query();

    answer_from(OTHER_NODE);
    TEST_ASSERT_EQUAL_INT_MESSAGE(TRIG_WAITING, HDTrigQueryService(),
        "not our answer, and the window has not closed - keep waiting");

    TEST_ASSERT_EQUAL_INT_MESSAGE(FinishState, State,
        "an answer naming another coater must not start our sequence");
    TEST_ASSERT_TRUE_MESSAGE(trig_query_sent,
        "our window stays open - that answer was not for us");
}

static void test_response_while_ghosting_does_not_start_the_sequence(void)
{
    arm_query();
    ghostState = 1;

    answer_from(NODE_ID);
    TEST_ASSERT_EQUAL_INT_MESSAGE(TRIG_NOTIDLE, HDTrigQueryService(),
        "the answer arrived, but a ghost band is running");

    TEST_ASSERT_EQUAL_INT_MESSAGE(FinishState, State,
        "a ghost band is running; the trig must not cut in");
    TEST_ASSERT_FALSE_MESSAGE(trig_query_sent, "the handshake still completes");
}


/* ==========================================================================
 * HD mode - the timeout window
 * ========================================================================== */

static void test_window_stays_open_just_before_the_timeout(void)
{
    arm_query();

    set_mco_time(T0 + 4999);
    TEST_ASSERT_EQUAL_INT(TRIG_WAITING, HDTrigQueryService());

    TEST_ASSERT_TRUE_MESSAGE(trig_query_sent, "4999 ms is inside the 5 s window");
}

static void test_window_closes_after_the_timeout(void)
{
    arm_query();

    set_mco_time(T0 + 5002);
    TEST_ASSERT_EQUAL_INT(TRIG_TIMEDOUT, HDTrigQueryService());

    TEST_ASSERT_FALSE_MESSAGE(trig_query_sent, "5 s elapsed: give up");
    TEST_ASSERT_EQUAL_INT_MESSAGE(FinishState, State,
        "a timeout leaves the state machine where it was");
}

/* A late answer, arriving after the window closed, must be inert - the unit
 * already gave up, and the operator's trig request is gone. */
static void test_answer_after_the_timeout_does_not_start_the_sequence(void)
{
    arm_query();
    set_mco_time(T0 + 5002);
    HDTrigQueryService();

    answer_from(NODE_ID);
    TEST_ASSERT_EQUAL_INT_MESSAGE(TRIG_IDLE, HDTrigQueryService(),
        "nothing is armed any more, so there is nothing to resolve");

    TEST_ASSERT_EQUAL_INT_MESSAGE(FinishState, State,
        "the window is closed; a late answer is ignored");
}

/* REGRESSION: trig_query_timer must be 16-bit.
 *
 * It was `unsigned char`, so `MCOHW_GetTime() + 5000` was truncated to 8 bits
 * and compared against a full 16-bit clock - the window became arbitrary,
 * usually "already expired". Arming near the 16-bit wraparound is what makes
 * an 8-bit store unmistakable: 0xF000 + 5000 wraps to 0x0388, which a byte
 * cannot hold at all.
 */
static void test_timeout_window_survives_the_16_bit_wraparound(void)
{
    set_mco_time(0xF000);
    HDSDSetting.value = HDSD_HD;
    State      = FinishState;
    ghostState = 0;
    activeCamAddress = CAM_ADDR;
    menu_data[0] |= 0x02;
    TrigRequest();

    TEST_ASSERT_TRUE_MESSAGE(trig_query_sent, "armed at 0xF000");

    set_mco_time((UNSIGNED16)(0xF000 + 4000));      /* wraps past 0xFFFF */
    TEST_ASSERT_EQUAL_INT_MESSAGE(TRIG_WAITING, HDTrigQueryService(),
        "4000 ms after arming is still inside the window, across the wrap");

    set_mco_time((UNSIGNED16)(0xF000 + 5002));
    TEST_ASSERT_EQUAL_INT_MESSAGE(TRIG_TIMEDOUT, HDTrigQueryService(),
        "5 s after arming the window closes, across the wrap");
}


/* ==========================================================================
 * Full path through doevents()
 * ========================================================================== */

static void test_hd_handshake_end_to_end_through_doevents(void)
{
    set_mco_time(T0);
    HDSDSetting.value = HDSD_HD;
    activeCamAddress = CAM_ADDR;
    State      = FinishState;
    ghostState = 0;
    set_actuator_moving(1);
    can_tx_reset();

    menu_data[0] |= 0x02;
    doevents();
    TEST_ASSERT_TRUE_MESSAGE(trig_query_sent, "doevents should arm the query");
    TEST_ASSERT_EQUAL_INT_MESSAGE(FinishState, State, "still waiting");

    answer_from(NODE_ID);
    doevents();
    TEST_ASSERT_EQUAL_INT_MESSAGE(TrigState, State,
        "the answer starts the coating sequence on the next pass");
}

/* The old code chained the response/timeout checks onto `else if` of the
 * request bit, so neither ran on a pass where a fresh request arrived. */
static void test_service_runs_on_a_pass_that_also_carries_a_request(void)
{
    arm_query();

    answer_from(NODE_ID);
    set_actuator_moving(1);
    menu_data[0] |= 0x02;       /* a new request lands in the same pass */
    TEST_ASSERT_EQUAL_INT_MESSAGE(TRIG_STARTED, TrigRequest(),
        "the handshake outcome wins over the request that raced it");

    TEST_ASSERT_EQUAL_INT_MESSAGE(TrigState, State,
        "the answer must still be serviced on a pass carrying a request");
}


int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_hdsd_mode_reads_whole_numbers);
    RUN_TEST(test_hdsd_mode_rounds_a_drifted_float);

    RUN_TEST(test_sd_trig_starts_the_sequence_without_a_query);
    RUN_TEST(test_sd_trig_ignored_with_no_camera_on);
    RUN_TEST(test_trig_ignored_in_a_mode_with_no_trig_path);
    RUN_TEST(test_trig_while_busy_is_rejected);

    RUN_TEST(test_hd_trig_transmits_the_query_and_waits);
    RUN_TEST(test_hd_trig_consumes_the_request_bit);
    RUN_TEST(test_hd_trig_does_not_query_without_an_active_camera);
    RUN_TEST(test_hd_trig_clears_a_stale_response_when_arming);

    RUN_TEST(test_response_for_this_node_starts_the_sequence);
    RUN_TEST(test_response_for_another_node_is_ignored);
    RUN_TEST(test_response_while_ghosting_does_not_start_the_sequence);

    RUN_TEST(test_window_stays_open_just_before_the_timeout);
    RUN_TEST(test_window_closes_after_the_timeout);
    RUN_TEST(test_answer_after_the_timeout_does_not_start_the_sequence);
    RUN_TEST(test_timeout_window_survives_the_16_bit_wraparound);

    RUN_TEST(test_hd_handshake_end_to_end_through_doevents);
    RUN_TEST(test_service_runs_on_a_pass_that_also_carries_a_request);

    return UNITY_END();
}
