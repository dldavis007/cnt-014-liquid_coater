/* test_cameras.c
 *
 * CameraMain1() / CameraMain2(), which run on every doevents() pass and handle
 * the two-wire camera protocol.
 *
 * Inbound (RPDO6, 0x521 -> camera_cmds[0..4]), one bit per command:
 *   0x01  generate a new random address for this camera
 *   0x02  report this camera's address  (TX 0x2a1)
 *   0x04  store a new address, bytes [3],[4], gated on [1],[2] == old address
 *   0x08  activate this camera's menu,  gated on [1],[2] == this address
 *
 * Inbound (RPDO5, 0x421 -> camera_addr[0..1]): select-by-address. A match
 * selects that camera (VSEL_PORT CAM_ON, VideoSw picks 1 vs 2) and displays its
 * tag; an address belonging to neither camera deselects.
 *
 * Camera 1 sets VideoSw_Port |= VideoSw; camera 2 clears it. Both are plain
 * SFRs, which under PC_SIDE are just bytes in sfr_regs[], so tests read them
 * back directly.
 *
 * Carried over from the refactored 4.33 suite; every test passes unchanged
 * against the production 4.33.
 */

#include "unity.h"
#include "test_support.h"

#include <stdio.h>
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
extern struct menu_var disp_add1, disp_add2;
extern char          Gen_Flags;
extern char          LAError;
extern float         HeadSpeed;
extern float         LAPos;
extern int           Update_Menu_Timer;
extern unsigned int  LAMoveTimer;
extern unsigned long LAMovingTimer;

extern unsigned cam_add1, cam_add2;
extern unsigned int activeCamAddress;
extern char     cam_addx1[2], cam_addx2[2];
extern char     CurrentLight;
extern char     Gen_Flags;
extern char     State;
extern char     ghostState;

/* Addresses deliberately BELOW 100: the 0x02 report handler does
 * `Timer1 = cam_add1/100; while(Timer1);` to stagger replies, and Timer1 only
 * counts down in the RTI ISR. Keeping the address under 100 makes that zero,
 * so the suite never needs the RTI thread running to avoid a hang. */
#define CAM1_ADDR 0x0042
#define CAM2_ADDR 0x0055

static void set_cmd(unsigned char b0, unsigned char b1, unsigned char b2,
                    unsigned char b3, unsigned char b4)
{
    camera_cmds[0] = b0; camera_cmds[1] = b1;
    camera_cmds[2] = b2; camera_cmds[3] = b3;
    camera_cmds[4] = b4;
}

static void select_address(unsigned addr)
{
    camera_addr[0] = addr & 0x00FF;
    camera_addr[1] = (addr & 0xFF00) >> 8;
}

/* Did the firmware transmit this frame? */
static int sent(UNSIGNED16 id, unsigned char b0, unsigned char b1)
{
    unsigned i;
    for (i = 0; i < can_tx_count && i < CAN_TX_LOG_N; i++)
        if (can_tx_log[i].ID == id && can_tx_log[i].LEN >= 2
            && can_tx_log[i].BUF[0] == b0 && can_tx_log[i].BUF[1] == b1)
            return 1;
    return 0;
}

void setUp(void)
{
    coat_test_begin();
    cam_add1 = CAM1_ADDR;
    cam_add2 = CAM2_ADDR;
    select_address(0);
    set_cmd(0, 0, 0, 0, 0);
    CurrentLight = 0;
    activeCamAddress = 0;
    can_tx_reset();
}

void tearDown(void) { }


/* ==========================================================================
 * Select by address (RPDO5 / 0x421)
 * ========================================================================== */

static void test_matching_address_selects_camera_1(void)
{
    select_address(CAM1_ADDR);

    doevents();

    TEST_ASSERT_TRUE_MESSAGE(VSEL_PORT & CAM_ON, "camera power should be on");
    TEST_ASSERT_TRUE_MESSAGE(VideoSw_Port & VideoSw,
        "VideoSw set selects camera 1");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, CurrentLight, "camera 1 light");
    TEST_ASSERT_EQUAL_STRING("Proc:COATING CAM", last_display_message());
}

static void test_matching_address_selects_camera_2(void)
{
    select_address(CAM2_ADDR);

    doevents();

    TEST_ASSERT_TRUE_MESSAGE(VSEL_PORT & CAM_ON, "camera power should be on");
    TEST_ASSERT_FALSE_MESSAGE(VideoSw_Port & VideoSw,
        "VideoSw cleared selects camera 2");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, CurrentLight, "camera 2 light");
    TEST_ASSERT_EQUAL_STRING("Proc:INSPECT CAM", last_display_message());
}

/* The handler consumes the request so it fires once, not every pass. */
static void test_select_clears_the_requested_address(void)
{
    select_address(CAM1_ADDR);

    doevents();

    TEST_ASSERT_EQUAL_UINT8(0, camera_addr[0]);
    TEST_ASSERT_EQUAL_UINT8(0, camera_addr[1]);
}

/* An address belonging to neither camera powers the video off.
 *
 * The deselect test is `low byte differs && high byte differs`, so the foreign
 * address must differ from the other camera in BOTH bytes - hence the distinct
 * high bytes here rather than the suite defaults. */
static void test_foreign_address_deselects_both_cameras(void)
{
    cam_add1 = 0x0142;
    cam_add2 = 0x0255;
    VSEL_PORT |= CAM_ON;
    select_address(0x0377);

    doevents();

    TEST_ASSERT_FALSE_MESSAGE(VSEL_PORT & CAM_ON,
        "an address for another node should turn this unit's camera off");
}

/* PINNED ODDITY: each deselect test is `low differs AND high differs`, and
 * CameraMain1 compares against cam_add2 while CameraMain2 compares against
 * cam_add1. So a foreign address that shares its HIGH byte with BOTH cameras
 * satisfies neither condition and the camera is left powered on. Both cameras
 * sitting in the same 256-address page - the default here, and the likely
 * real-world case - makes every foreign address in that page fail to deselect.
 * Shared with 4.33. */
static void test_foreign_address_in_the_same_high_byte_page_does_not_deselect(void)
{
    cam_add1 = 0x0042;          /* both cameras in page 0x00 */
    cam_add2 = 0x0055;
    VSEL_PORT |= CAM_ON;
    select_address(0x0077);     /* same page: high byte matches both */

    doevents();

    TEST_ASSERT_TRUE_MESSAGE(VSEL_PORT & CAM_ON,
        "high byte matches both cameras, so neither deselect condition fires");
}


/* ==========================================================================
 * 0x01 - generate a random address
 * ========================================================================== */

static void test_cmd_random_address_changes_camera_1(void)
{
    unsigned before = cam_add1;

    set_cmd(0x01, 0, 0, 0, 0);
    doevents();

    TEST_ASSERT_NOT_EQUAL_MESSAGE(before, cam_add1,
        "0x01 should replace camera 1's address");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(cam_add1 & 0xFF, (unsigned char)cam_addx1[0],
        "the byte pair saved to EEPROM should track the new address");
    TEST_ASSERT_EQUAL_UINT8((cam_add1 >> 8) & 0xFF, (unsigned char)cam_addx1[1]);
}

static void test_cmd_random_address_changes_camera_2(void)
{
    unsigned before = cam_add2;

    set_cmd(0x01, 0, 0, 0, 0);
    doevents();

    TEST_ASSERT_NOT_EQUAL_MESSAGE(before, cam_add2,
        "0x01 should replace camera 2's address too");
}


/* ==========================================================================
 * 0x02 - report address
 * ========================================================================== */

static void test_cmd_report_address_transmits_both_cameras(void)
{
    set_cmd(0x02, 0, 0, 0, 0);

    doevents();

    TEST_ASSERT_TRUE_MESSAGE(
        sent(0x2a1, CAM1_ADDR & 0xFF, (CAM1_ADDR >> 8) & 0xFF),
        "camera 1 should report its address on 0x2a1");
    TEST_ASSERT_TRUE_MESSAGE(
        sent(0x2a1, CAM2_ADDR & 0xFF, (CAM2_ADDR >> 8) & 0xFF),
        "camera 2 should report its address on 0x2a1");
}


/* ==========================================================================
 * 0x04 - store a new address (gated on the old one matching)
 * ========================================================================== */

static void test_cmd_store_address_updates_camera_1_when_old_matches(void)
{
    set_cmd(0x04, CAM1_ADDR & 0xFF, (CAM1_ADDR >> 8) & 0xFF, 0x34, 0x12);

    doevents();

    TEST_ASSERT_EQUAL_HEX16_MESSAGE(0x1234, cam_add1,
        "0x04 with the matching old address should store the new one");
}

static void test_cmd_store_address_ignored_when_old_does_not_match(void)
{
    set_cmd(0x04, 0x99, 0x99, 0x34, 0x12);

    doevents();

    TEST_ASSERT_EQUAL_HEX16_MESSAGE(CAM1_ADDR, cam_add1,
        "a store aimed at a different camera must not change this one");
}


/* ==========================================================================
 * 0x08 - activate the camera's menu
 * ========================================================================== */

static void test_cmd_activate_menu_selects_camera_1_and_announces(void)
{
    set_cmd(0x08, CAM1_ADDR & 0xFF, (CAM1_ADDR >> 8) & 0xFF, 0, 0);
    Gen_Flags = 0;                       /* menu not already active */

    doevents();

    TEST_ASSERT_TRUE_MESSAGE(VSEL_PORT & CAM_ON, "camera power on");
    TEST_ASSERT_TRUE_MESSAGE(VideoSw_Port & VideoSw, "camera 1 selected");
    TEST_ASSERT_TRUE_MESSAGE(
        sent(0x421, CAM1_ADDR & 0xFF, (CAM1_ADDR >> 8) & 0xFF),
        "activating should broadcast 0x421 so other cameras deselect");
}

static void test_cmd_activate_menu_consumes_the_command(void)
{
    set_cmd(0x08, CAM1_ADDR & 0xFF, (CAM1_ADDR >> 8) & 0xFF, 0, 0);

    doevents();

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, camera_cmds[0],
        "the command bytes are cleared so the menu opens once");
    TEST_ASSERT_EQUAL_UINT8(0, camera_cmds[1]);
    TEST_ASSERT_EQUAL_UINT8(0, camera_cmds[2]);
}

static void test_cmd_activate_menu_ignored_for_a_different_address(void)
{
    set_cmd(0x08, 0x99, 0x99, 0, 0);

    doevents();

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x08, camera_cmds[0],
        "a command aimed at another camera is left untouched");
}

static void test_cmd_activate_menu_selects_camera_2(void)
{
    set_cmd(0x08, CAM2_ADDR & 0xFF, (CAM2_ADDR >> 8) & 0xFF, 0, 0);

    doevents();

    TEST_ASSERT_TRUE_MESSAGE(VSEL_PORT & CAM_ON, "camera power on");
    TEST_ASSERT_FALSE_MESSAGE(VideoSw_Port & VideoSw, "camera 2 selected");
    TEST_ASSERT_TRUE_MESSAGE(
        sent(0x421, CAM2_ADDR & 0xFF, (CAM2_ADDR >> 8) & 0xFF),
        "camera 2 activation should broadcast its address");
}


/* ==========================================================================
 * Address display mirror
 * ========================================================================== */

static void test_address_change_updates_the_hex_display_variable(void)
{
    cam_add1 = 0x0ABC;
    doevents();
    TEST_ASSERT_EQUAL_STRING_MESSAGE("0ABC", disp_add1.str_value,
        "disp_add1 mirrors cam_add1 as 4 hex digits");

    cam_add2 = 0x0DEF;
    doevents();
    TEST_ASSERT_EQUAL_STRING("0DEF", disp_add2.str_value);
}


/* ==========================================================================
 * Active camera latch (update_active_cam_address)
 *
 * Both CameraMain1 and CameraMain2 ZERO camera_addr[] the moment they match,
 * so the latch has to run BEFORE them in doevents() - it did not, and the
 * coating camera's address was consumed before it could ever be recorded while
 * the inspection camera's survived (CameraMain1 misses it, so nothing clears).
 * These tests pin the ordering and the guards that keep the latch stable.
 * ========================================================================== */

static void test_latch_records_the_coating_camera(void)
{
    select_address(CAM1_ADDR);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, update_active_cam_address(),
        "a new selection should report a change");
    TEST_ASSERT_EQUAL_HEX16_MESSAGE(CAM1_ADDR, activeCamAddress,
        "the coating camera must be latched like any other");
}

static void test_latch_records_the_inspection_camera(void)
{
    select_address(CAM2_ADDR);

    TEST_ASSERT_EQUAL_INT(1, update_active_cam_address());
    TEST_ASSERT_EQUAL_HEX16(CAM2_ADDR, activeCamAddress);
}

/* REGRESSION: the whole reason the latch moved ahead of CameraMain1/2.
 * Through the real pass, CameraMain1 consumes camera_addr[] at the match, so a
 * latch running after it would read 0,0 and never record the coating camera. */
static void test_latch_survives_the_full_doevents_pass(void)
{
    select_address(CAM1_ADDR);

    doevents();

    TEST_ASSERT_EQUAL_HEX16_MESSAGE(CAM1_ADDR, activeCamAddress,
        "the latch must run before CameraMain1 clears the address bytes");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, camera_addr[0],
        "and CameraMain1 still consumes the selection");
}

/* A pass with nothing selected must leave the latch alone - the address has to
 * outlive the single pass in which CameraMain1/2 consume it.
 *
 * NOTE: with both camera addresses configured, the foreign-address filter
 * already rejects the 0,0 case, so this passes with or without the explicit
 * zero guard. test_latch_holds_when_a_camera_address_is_unconfigured covers
 * what the zero guard itself is load-bearing for. */
static void test_latch_is_not_wiped_by_a_pass_with_no_selection(void)
{
    select_address(CAM1_ADDR);
    doevents();
    TEST_ASSERT_EQUAL_HEX16(CAM1_ADDR, activeCamAddress);

    doevents();                         /* nothing selected this pass */
    TEST_ASSERT_EQUAL_HEX16_MESSAGE(CAM1_ADDR, activeCamAddress,
        "an idle pass must not clear the active camera");

    doevents();
    TEST_ASSERT_EQUAL_HEX16_MESSAGE(CAM1_ADDR, activeCamAddress,
        "and it stays put for as long as nothing else is selected");
}

/* REGRESSION for the zero guard specifically.
 *
 * A camera whose address was never configured reads back as 0 (EEPROM unset,
 * or the host's SKIP_EEPROM_LOAD path before main.c seeds it). An idle pass
 * presents camera_addr[] as 0,0 - which now MATCHES that unconfigured address,
 * so the foreign-address filter lets it through and the latch is wiped on the
 * pass after every selection. Only the zero guard stops it. */
static void test_latch_holds_when_a_camera_address_is_unconfigured(void)
{
    cam_add1 = 0;                       /* never addressed */

    select_address(CAM2_ADDR);
    update_active_cam_address();
    TEST_ASSERT_EQUAL_HEX16(CAM2_ADDR, activeCamAddress);

    select_address(0);                  /* idle pass: camera_addr[] is 0,0 */

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, update_active_cam_address(),
        "an idle pass is not a selection, even of an unconfigured camera");
    TEST_ASSERT_EQUAL_HEX16_MESSAGE(CAM2_ADDR, activeCamAddress,
        "the zero guard keeps 0,0 from matching an unset camera address");
}

/* 0x421 is a shared bus ID. A select aimed at another unit's camera reaches
 * this latch untouched (neither CameraMain clears a non-matching address), so
 * without the address filter it would hijack activeCamAddress - and with it the
 * HD trig query that gets sent to that address. */
static void test_latch_ignores_an_address_belonging_to_another_unit(void)
{
    select_address(CAM1_ADDR);
    update_active_cam_address();

    select_address(0x0377);             /* neither cam_add1 nor cam_add2 */

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, update_active_cam_address(),
        "a foreign address is not a change");
    TEST_ASSERT_EQUAL_HEX16_MESSAGE(CAM1_ADDR, activeCamAddress,
        "and must not displace the camera we already hold");
}

static void test_latch_reports_no_change_when_the_same_camera_is_reselected(void)
{
    select_address(CAM1_ADDR);
    TEST_ASSERT_EQUAL_INT(1, update_active_cam_address());

    select_address(CAM1_ADDR);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, update_active_cam_address(),
        "reselecting the camera we already hold is not a change");
    TEST_ASSERT_EQUAL_HEX16(CAM1_ADDR, activeCamAddress);
}

static void test_latch_switches_between_the_two_cameras(void)
{
    select_address(CAM1_ADDR);
    doevents();
    TEST_ASSERT_EQUAL_HEX16(CAM1_ADDR, activeCamAddress);

    select_address(CAM2_ADDR);
    doevents();
    TEST_ASSERT_EQUAL_HEX16_MESSAGE(CAM2_ADDR, activeCamAddress,
        "selecting the other camera moves the latch");
}

/* The address is assembled low byte first, matching how CameraMain1/2 write it
 * (camera_addr[0] = addr, camera_addr[1] = addr >> 8). A swapped read here sent
 * every HD trig query to a byte-reversed address. */
static void test_latch_assembles_the_address_low_byte_first(void)
{
    cam_add1 = 0x1928;
    camera_addr[0] = 0x28;              /* LSB */
    camera_addr[1] = 0x19;              /* MSB */

    update_active_cam_address();

    TEST_ASSERT_EQUAL_HEX16_MESSAGE(0x1928, activeCamAddress,
        "camera_addr[0] is the LOW byte");
}


/* ==========================================================================
 * Which camera arms the coating trigger
 *
 * The gate (unit.c, the menu_data[0] & 0x02 block) reads only
 * VSEL_PORT & CAM_ON. CAM_ON is bit 4 of PORTA and means "a camera is on";
 * VideoSw is bit 5 and is what says WHICH. Both CameraMain1 and CameraMain2
 * set CAM_ON, and the gate never looks at VideoSw or CurrentLight - so the
 * coating sequence starts on either camera. These tests pin that down.
 * ========================================================================== */

/* Select a camera, then pulse the trigger bit, and report where State landed.
 * The actuator is held "moving" so the sequence parks in TrigState instead of
 * advancing within the same pass - the gate and the switch both run per pass. */
static char trigger_after_selecting(unsigned addr)
{
    select_address(addr);
    doevents();                     /* CameraMain1/2 act on the selection */

    set_actuator_moving(1);
    State      = FinishState;
    ghostState = 0;
    menu_data[0] |= 0x02;
    doevents();

    return State;
}

static void test_coating_camera_arms_the_trigger(void)
{
    char state = trigger_after_selecting(CAM1_ADDR);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, CurrentLight, "camera 1 should be selected");
    TEST_ASSERT_TRUE_MESSAGE(VideoSw_Port & VideoSw, "VideoSw should route camera 1");
    TEST_ASSERT_EQUAL_INT_MESSAGE(TrigState, state, "camera 1 should arm the trigger");
}

static void test_inspection_camera_also_arms_the_trigger(void)
{
    char state = trigger_after_selecting(CAM2_ADDR);

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, CurrentLight, "camera 2 should be selected");
    TEST_ASSERT_FALSE_MESSAGE(VideoSw_Port & VideoSw, "VideoSw should route camera 2");
    TEST_ASSERT_EQUAL_INT_MESSAGE(TrigState, state,
        "camera 2 arms the trigger too - the gate reads CAM_ON, which both set");
}

static void test_trigger_is_ignored_with_no_camera_on(void)
{
    set_actuator_moving(1);
    VSEL_PORT &= ~CAM_ON;
    State      = FinishState;
    ghostState = 0;
    menu_data[0] |= 0x02;

    doevents();

    TEST_ASSERT_EQUAL_INT_MESSAGE(FinishState, State,
        "no camera on: the trigger must not start the sequence");
}


int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_matching_address_selects_camera_1);
    RUN_TEST(test_matching_address_selects_camera_2);
    RUN_TEST(test_select_clears_the_requested_address);
    RUN_TEST(test_foreign_address_deselects_both_cameras);
    RUN_TEST(test_foreign_address_in_the_same_high_byte_page_does_not_deselect);

    RUN_TEST(test_cmd_random_address_changes_camera_1);
    RUN_TEST(test_cmd_random_address_changes_camera_2);

    RUN_TEST(test_cmd_report_address_transmits_both_cameras);

    RUN_TEST(test_cmd_store_address_updates_camera_1_when_old_matches);
    RUN_TEST(test_cmd_store_address_ignored_when_old_does_not_match);

    RUN_TEST(test_cmd_activate_menu_selects_camera_1_and_announces);
    RUN_TEST(test_cmd_activate_menu_consumes_the_command);
    RUN_TEST(test_cmd_activate_menu_ignored_for_a_different_address);
    RUN_TEST(test_cmd_activate_menu_selects_camera_2);

    RUN_TEST(test_address_change_updates_the_hex_display_variable);

    RUN_TEST(test_latch_records_the_coating_camera);
    RUN_TEST(test_latch_records_the_inspection_camera);
    RUN_TEST(test_latch_survives_the_full_doevents_pass);
    RUN_TEST(test_latch_is_not_wiped_by_a_pass_with_no_selection);
    RUN_TEST(test_latch_holds_when_a_camera_address_is_unconfigured);
    RUN_TEST(test_latch_ignores_an_address_belonging_to_another_unit);
    RUN_TEST(test_latch_reports_no_change_when_the_same_camera_is_reselected);
    RUN_TEST(test_latch_switches_between_the_two_cameras);
    RUN_TEST(test_latch_assembles_the_address_low_byte_first);

    RUN_TEST(test_coating_camera_arms_the_trigger);
    RUN_TEST(test_inspection_camera_also_arms_the_trigger);
    RUN_TEST(test_trigger_is_ignored_with_no_camera_on);

    return UNITY_END();
}
