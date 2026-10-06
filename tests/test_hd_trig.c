/* test_hd_trig.c
 *
 * TrigRequest() and HD camera pairing (Subroutines1.c).
 *
 * INTERNAL (InternalExternalCameraSetting == 1): a trig request with a camera
 * on (VSEL_PORT & CAM_ON) starts the coating sequence.
 *
 * EXTERNAL (== 2): an HD camera pairs itself to a coater by sending 0x321
 * (RPDO8 -> pairing_msg[0..7]): [0] = node id, [1..2] = camera address, low
 * byte first. poll_paired_camera_address() latches it when [0] == NODE_ID. A
 * trig request then starts the sequence only if the paired camera is the
 * active one (activeCamAddress).
 */

#include "unity.h"
#include "test_support.h"

#include <stdio.h>
#include <string.h>
#include "mc9s12a128.h"
#include "Subroutines.h"
#include "Subroutines1.h"
#include "Interrupts.h"

extern char State;
extern char ghostState;
extern unsigned int activeCamAddress;
extern UNSIGNED16 paired_camera_address;

#define CAM_ADDR    0x1942
#define OTHER_ADDR  0x2855
#define OTHER_NODE  0x55        /* any node id that is not ours */

/* Writes a 0x321 pairing message into the process image. */
static void pairing_from(unsigned char node, unsigned addr)
{
    memset(pairing_msg, 0, 8);
    pairing_msg[0] = node;
    pairing_msg[1] = addr & 0xFF;
    pairing_msg[2] = (addr >> 8) & 0xFF;
}

static void set_external(void) { InternalExternalCameraSetting.value = EXTERNAL_CAMERA; }
static void set_internal(void) { InternalExternalCameraSetting.value = INTERNAL_CAMERA; }

static void request_trig(void) { menu_data[0] |= 0x02; }

void setUp(void)
{
    coat_test_begin();
    set_internal();
    activeCamAddress      = 0;
    paired_camera_address = 0;
    ghostState            = 0;
    State                 = FinishState;
    menu_data[0]          = 0;
    memset(pairing_msg, 0, 8);
    VSEL_PORT &= ~CAM_ON;
    can_tx_reset();
}

void tearDown(void) { }


/* ==========================================================================
 * internalExternal - the float guard
 * ========================================================================== */

static void test_internal_external_reads_whole_numbers(void)
{
    InternalExternalCameraSetting.value = 1.0f;
    TEST_ASSERT_EQUAL_INT(INTERNAL_CAMERA, internalExternal());
    InternalExternalCameraSetting.value = 2.0f;
    TEST_ASSERT_EQUAL_INT(EXTERNAL_CAMERA, internalExternal());
}

static void test_internal_external_rounds_a_drifted_float(void)
{
    InternalExternalCameraSetting.value = 1.9999f;
    TEST_ASSERT_EQUAL_INT_MESSAGE(EXTERNAL_CAMERA, internalExternal(),
        "a value just under 2 must still read as EXTERNAL");
}


/* ==========================================================================
 * Common to both modes
 * ========================================================================== */

static void test_no_request_does_nothing(void)
{
    VSEL_PORT |= CAM_ON;

    TrigRequest();

    TEST_ASSERT_EQUAL_INT(FinishState, State);
}

static void test_trig_while_coating_is_rejected_and_consumed(void)
{
    VSEL_PORT |= CAM_ON;
    State = TrigState;
    request_trig();

    TEST_ASSERT_EQUAL_INT(TRIG_NOTIDLE, TrigRequest());
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, menu_data[0] & 0x02,
        "the request is consumed even when it cannot be honoured");
}

static void test_trig_while_ghosting_is_rejected(void)
{
    VSEL_PORT |= CAM_ON;
    ghostState = 1;
    request_trig();

    TEST_ASSERT_EQUAL_INT(TRIG_NOTIDLE, TrigRequest());
    TEST_ASSERT_EQUAL_INT(FinishState, State);
}

/* The setting ranges 1..2; anything else must not fall through to INTERNAL. */
static void test_trig_ignored_in_an_unknown_mode(void)
{
    InternalExternalCameraSetting.value = 3.0f;
    VSEL_PORT |= CAM_ON;
    request_trig();

    TEST_ASSERT_EQUAL_INT(TRIG_IDLE, TrigRequest());
    TEST_ASSERT_EQUAL_INT(FinishState, State);
}


/* ==========================================================================
 * INTERNAL
 * ========================================================================== */

static void test_internal_trig_with_camera_on_starts_the_sequence(void)
{
    VSEL_PORT |= CAM_ON;
    request_trig();

    TEST_ASSERT_EQUAL_INT(TRIG_STARTED, TrigRequest());
    TEST_ASSERT_EQUAL_INT(TrigState, State);
    TEST_ASSERT_EQUAL_UINT8(0, menu_data[0] & 0x02);
}

static void test_internal_trig_with_camera_off_is_ignored_and_warns(void)
{
    request_trig();

    TEST_ASSERT_EQUAL_INT(TRIG_IDLE, TrigRequest());
    TEST_ASSERT_EQUAL_INT(FinishState, State);
    TEST_ASSERT_EQUAL_STRING("Warn:WRONG INT CAMERA", last_display_message());
}

/* Pairing is an EXTERNAL concept; a mismatched pairing must not block INTERNAL. */
static void test_internal_trig_ignores_pairing(void)
{
    VSEL_PORT |= CAM_ON;
    paired_camera_address = OTHER_ADDR;
    activeCamAddress      = CAM_ADDR;
    request_trig();

    TEST_ASSERT_EQUAL_INT(TRIG_STARTED, TrigRequest());
    TEST_ASSERT_EQUAL_INT(TrigState, State);
}


/* ==========================================================================
 * EXTERNAL
 * ========================================================================== */

static void test_external_trig_starts_when_paired_camera_is_active(void)
{
    set_external();
    paired_camera_address = CAM_ADDR;
    activeCamAddress      = CAM_ADDR;
    request_trig();

    TEST_ASSERT_EQUAL_INT(TRIG_STARTED, TrigRequest());
    TEST_ASSERT_EQUAL_INT(TrigState, State);
    TEST_ASSERT_EQUAL_UINT8(0, menu_data[0] & 0x02);
}

/* An HD camera is not powered from this unit, so CAM_ON is irrelevant. */
static void test_external_trig_does_not_need_local_camera_power(void)
{
    set_external();
    VSEL_PORT &= ~CAM_ON;
    paired_camera_address = CAM_ADDR;
    activeCamAddress      = CAM_ADDR;
    request_trig();

    TEST_ASSERT_EQUAL_INT(TRIG_STARTED, TrigRequest());
}

static void test_external_trig_ignored_when_paired_camera_is_not_active(void)
{
    set_external();
    paired_camera_address = CAM_ADDR;
    activeCamAddress      = OTHER_ADDR;
    request_trig();

    TEST_ASSERT_EQUAL_INT(TRIG_IDLE, TrigRequest());
    TEST_ASSERT_EQUAL_INT(FinishState, State);
    TEST_ASSERT_EQUAL_STRING("Warn:WRONG EXT CAM", last_display_message());
    TEST_ASSERT_EQUAL_UINT8(0, menu_data[0] & 0x02);
}

static void test_external_trig_ignored_when_not_paired(void)
{
    set_external();
    activeCamAddress = CAM_ADDR;
    request_trig();

    TEST_ASSERT_EQUAL_INT(TRIG_IDLE, TrigRequest());
    TEST_ASSERT_EQUAL_INT(FinishState, State);
    TEST_ASSERT_EQUAL_STRING("Warn:NO EXT CAM PAIRED", last_display_message());
}

/* REGRESSION: both start at 0 on boot, and 0 == 0 used to trigger unpaired. */
static void test_external_trig_ignored_at_boot_with_nothing_paired_or_active(void)
{
    set_external();
    request_trig();

    TEST_ASSERT_EQUAL_INT(TRIG_IDLE, TrigRequest());
    TEST_ASSERT_EQUAL_INT_MESSAGE(FinishState, State,
        "unpaired 0 must not match an unset active address of 0");
}


/* ==========================================================================
 * Pairing - poll_paired_camera_address
 * ========================================================================== */

static void test_pairing_for_this_node_latches_the_address(void)
{
    pairing_from(NODE_ID, CAM_ADDR);

    TEST_ASSERT_EQUAL_INT(1, poll_paired_camera_address());
    TEST_ASSERT_EQUAL_HEX16(CAM_ADDR, paired_camera_address);
}

static void test_pairing_assembles_the_address_low_byte_first(void)
{
    memset(pairing_msg, 0, 8);
    pairing_msg[0] = NODE_ID;
    pairing_msg[1] = 0x42;      /* LSB */
    pairing_msg[2] = 0x19;      /* MSB */

    poll_paired_camera_address();

    TEST_ASSERT_EQUAL_HEX16(0x1942, paired_camera_address);
}

/* Once consumed, the stale bytes must not be read as a new message. */
static void test_pairing_consumes_the_message(void)
{
    unsigned i;
    pairing_from(NODE_ID, CAM_ADDR);
    pairing_msg[3] = 0x01;      /* camera type / tag bytes */
    pairing_msg[7] = 0x7A;

    poll_paired_camera_address();

    for (i = 0; i < 8; i++)
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, pairing_msg[i], "all 8 bytes cleared");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, poll_paired_camera_address(),
        "a cleared message is not a new pairing");
    TEST_ASSERT_EQUAL_HEX16(CAM_ADDR, paired_camera_address);
}

/* 0x321 is a shared bus ID: another coater's pairing must not hijack ours. */
static void test_pairing_for_another_node_is_ignored(void)
{
    paired_camera_address = CAM_ADDR;
    pairing_from(OTHER_NODE, OTHER_ADDR);

    TEST_ASSERT_EQUAL_INT(0, poll_paired_camera_address());
    TEST_ASSERT_EQUAL_HEX16(CAM_ADDR, paired_camera_address);
}

static void test_pairing_with_a_zero_address_is_ignored(void)
{
    paired_camera_address = CAM_ADDR;
    pairing_from(NODE_ID, 0);

    TEST_ASSERT_EQUAL_INT(0, poll_paired_camera_address());
    TEST_ASSERT_EQUAL_HEX16_MESSAGE(CAM_ADDR, paired_camera_address,
        "address 0 would leave the unit unpaired");
}

static void test_pairing_with_the_same_address_is_not_a_change(void)
{
    paired_camera_address = CAM_ADDR;
    pairing_from(NODE_ID, CAM_ADDR);

    TEST_ASSERT_EQUAL_INT(0, poll_paired_camera_address());
    TEST_ASSERT_EQUAL_HEX16(CAM_ADDR, paired_camera_address);
}

static void test_pairing_to_a_new_camera_replaces_the_old_one(void)
{
    paired_camera_address = CAM_ADDR;
    pairing_from(NODE_ID, OTHER_ADDR);

    TEST_ASSERT_EQUAL_INT(1, poll_paired_camera_address());
    TEST_ASSERT_EQUAL_HEX16(OTHER_ADDR, paired_camera_address);
}

/* Our camera announcing it now belongs to another node is a stale pairing. */
static void test_pairing_released_when_our_camera_pairs_to_another_node(void)
{
    paired_camera_address = CAM_ADDR;
    pairing_from(OTHER_NODE, CAM_ADDR);

    TEST_ASSERT_EQUAL_INT(1, poll_paired_camera_address());
    TEST_ASSERT_EQUAL_HEX16(0, paired_camera_address);
}

static void test_external_trig_after_release_warns_not_paired(void)
{
    set_external();
    paired_camera_address = CAM_ADDR;
    activeCamAddress      = CAM_ADDR;
    pairing_from(OTHER_NODE, CAM_ADDR);
    poll_paired_camera_address();
    request_trig();

    TEST_ASSERT_EQUAL_INT(TRIG_IDLE, TrigRequest());
    TEST_ASSERT_EQUAL_INT(FinishState, State);
    TEST_ASSERT_EQUAL_STRING("Warn:NO EXT CAM PAIRED", last_display_message());
}

static void test_pairing_for_another_node_is_consumed(void)
{
    pairing_from(OTHER_NODE, OTHER_ADDR);

    poll_paired_camera_address();

    TEST_ASSERT_EQUAL_UINT8(0, pairing_msg[0]);
}


/* ==========================================================================
 * End to end: 0x321 on the bus through RPDO8 and doevents()
 * ========================================================================== */

/* Runs passes until the injected frame has been mapped and polled. The first
 * MCO_ProcessStack pass after bring-up sends boot-up and reads nothing. */
static void run_passes(int n)
{
    while (n--)
        doevents();
}

static void test_rpdo8_frame_pairs_the_unit(void)
{
    UNSIGNED8 frame[8] = { NODE_ID, CAM_ADDR & 0xFF, (CAM_ADDR >> 8) & 0xFF,
                           0x01, 'A', 'B', 'C', 'D' };

    can_rx_inject(0x321, frame, 8);
    run_passes(3);

    TEST_ASSERT_EQUAL_HEX16_MESSAGE(CAM_ADDR, paired_camera_address,
        "0x321 must land in pairing_msg and be latched");
}

/* RPDO8 is 8 bytes; mapped anywhere else it would overwrite neighbours such as
 * the menu/trig byte. */
static void test_rpdo8_frame_does_not_touch_the_trig_byte(void)
{
    UNSIGNED8 frame[8] = { OTHER_NODE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

    can_rx_inject(0x321, frame, 8);
    run_passes(3);

    TEST_ASSERT_EQUAL_UINT8(0, menu_data[0]);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, pairing_msg[0],
        "every 321 is consumed, even one for another node");
}

/* Camera re-paired to another coater on the bus: this unit must let go. */
static void test_rpdo8_frame_for_another_node_releases_our_camera(void)
{
    UNSIGNED8 frame[8] = { OTHER_NODE, CAM_ADDR & 0xFF, (CAM_ADDR >> 8) & 0xFF,
                           0, 0, 0, 0, 0 };

    paired_camera_address = CAM_ADDR;
    can_rx_inject(0x321, frame, 8);
    run_passes(3);

    TEST_ASSERT_EQUAL_HEX16(0, paired_camera_address);
}

static void test_external_pair_select_and_trig_through_doevents(void)
{
    UNSIGNED8 frame[8] = { NODE_ID, CAM_ADDR & 0xFF, (CAM_ADDR >> 8) & 0xFF,
                           0, 0, 0, 0, 0 };

    set_external();
    set_actuator_moving(1);     /* park in TrigState instead of advancing */

    can_rx_inject(0x321, frame, 8);
    run_passes(3);

    camera_addr[0] = CAM_ADDR & 0xFF;   /* 0x421 select of the HD camera */
    camera_addr[1] = (CAM_ADDR >> 8) & 0xFF;
    doevents();
    TEST_ASSERT_EQUAL_HEX16(CAM_ADDR, activeCamAddress);

    request_trig();
    doevents();
    TEST_ASSERT_EQUAL_INT(TrigState, State);
}


int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_internal_external_reads_whole_numbers);
    RUN_TEST(test_internal_external_rounds_a_drifted_float);

    RUN_TEST(test_no_request_does_nothing);
    RUN_TEST(test_trig_while_coating_is_rejected_and_consumed);
    RUN_TEST(test_trig_while_ghosting_is_rejected);
    RUN_TEST(test_trig_ignored_in_an_unknown_mode);

    RUN_TEST(test_internal_trig_with_camera_on_starts_the_sequence);
    RUN_TEST(test_internal_trig_with_camera_off_is_ignored_and_warns);
    RUN_TEST(test_internal_trig_ignores_pairing);

    RUN_TEST(test_external_trig_starts_when_paired_camera_is_active);
    RUN_TEST(test_external_trig_does_not_need_local_camera_power);
    RUN_TEST(test_external_trig_ignored_when_paired_camera_is_not_active);
    RUN_TEST(test_external_trig_ignored_when_not_paired);
    RUN_TEST(test_external_trig_ignored_at_boot_with_nothing_paired_or_active);

    RUN_TEST(test_pairing_for_this_node_latches_the_address);
    RUN_TEST(test_pairing_assembles_the_address_low_byte_first);
    RUN_TEST(test_pairing_consumes_the_message);
    RUN_TEST(test_pairing_for_another_node_is_ignored);
    RUN_TEST(test_pairing_with_a_zero_address_is_ignored);
    RUN_TEST(test_pairing_with_the_same_address_is_not_a_change);
    RUN_TEST(test_pairing_to_a_new_camera_replaces_the_old_one);
    RUN_TEST(test_pairing_released_when_our_camera_pairs_to_another_node);
    RUN_TEST(test_external_trig_after_release_warns_not_paired);
    RUN_TEST(test_pairing_for_another_node_is_consumed);

    RUN_TEST(test_rpdo8_frame_pairs_the_unit);
    RUN_TEST(test_rpdo8_frame_does_not_touch_the_trig_byte);
    RUN_TEST(test_rpdo8_frame_for_another_node_releases_our_camera);
    RUN_TEST(test_external_pair_select_and_trig_through_doevents);

    return UNITY_END();
}
