/* test_menu_cursor.c
 *
 * Press / hold / release semantics for the menu navigation buttons.
 *
 * Why this suite exists
 * ---------------------
 * The three navigation flags (CursorUpFlag, CursorDownFlag, SelectFlag in
 * Subroutines1.c) are TRI-STATE, and the middle state is easy to break:
 *
 *     0   idle, no button held
 *     1   fresh press seen, not yet acted on
 *    -1   already acted on - KEEP auto-repeating while the button is held
 *
 * `-1` is deliberately truthy, because `if ( CursorUpFlag ) { CursorUp(); ... }`
 * is what produces hold-to-repeat at the MenuTime rate. Release is detected
 * separately, by comparing against the sentinel:
 *
 *     else if ( CursorUpFlag == -1 && !( TC0_RCVD_Data & TeleData_CamTog2 ) )
 *         CursorUpFlag = 0;
 *
 * That comparison is the fragile part. The flags were originally plain `char`,
 * which works on ICC12 (it truncates char compares to 8 bits, so 0xFF matches
 * -1) but was DEAD under host GCC with -funsigned-char, where the flag promotes
 * to int 255 and `255 == -1` is false. The flag then never returned to 0, stayed
 * truthy, and one button press scrolled the entire menu forever at ~3 Hz.
 * See pc_side/README.md, "ICC12 vs GCC: the char comparison trap".
 *
 * What these tests do and do not cover
 * ------------------------------------
 * They cover the BEHAVIOUR: a press moves the cursor, a hold repeats, a release
 * stops it. That contract is real on both compilers, so it is worth pinning
 * regardless of which one exposed the break. Dropping `signed` from the flag
 * declarations makes `test_release_stops_the_cursor` fail immediately.
 *
 * They CANNOT verify target codegen - no host test can. Whether the declaration
 * changes ICC12 output is a build-time object-file A/B, not a unit test.
 *
 * Determinism
 * -----------
 * The cursor block only runs when MenuTimer has expired. MenuTimer is driven by
 * the RTI ISR, so instead of running the RTI sim (which would also advance
 * StateTime and make this racy) each helper zeroes MenuTimer directly to open
 * exactly one repeat window. menu_window() == "one MenuTime elapsed".
 */

#include "unity.h"
#include "test_support.h"

#include <string.h>
#include "mc9s12a128.h"
#include "Subroutines.h"
#include "Subroutines1.h"
#include "Interrupts.h"

extern unsigned char sfr_regs[0x400];
extern UNSIGNED8     gProcImg[];

extern char          State;
extern char          Gen_Flags;
extern char          Variable_flag;
extern char          StackPointer;
extern unsigned int  TC0_RCVD_Data;
extern unsigned int  MenuTimer;
extern unsigned long StateTime;
extern signed char   CursorUpFlag;
extern signed char   CursorDownFlag;
extern signed char   SelectFlag;
extern struct MenuStack MenuStackc[];

/* 0x180 button-box word bits, as emulators/button_box_panel.py sends them and
 * as Interrupts.h names them (TeleData_*). */
#define BB_KEEPALIVE 0x0400
#define BB_SELECT    0x0004      /* TeleData_PLCTrig  */
#define BB_UP        0x0002      /* TeleData_CamTog2  */
#define BB_DOWN      0x0001      /* TeleData_CamTog1  */

/* The button word arrives on RPDO2 (0x180) at gProcImg[OUT_digi_1..3];
 * doevents() folds bytes 1,2 into TC0_RCVD_Data. Write it the way the bus would. */
static void set_word(unsigned w)
{
    gProcImg[OUT_digi_1] = (UNSIGNED8)(w & 0xFF);
    gProcImg[OUT_digi_2] = (UNSIGNED8)((w >> 8) & 0xFF);
}

static int cursor_pos(void) { return (int)MenuStackc[StackPointer].CursorPos; }

/* One doevents() pass with NO repeat window (MenuTimer left alone). Used to let
 * the word propagate into TC0_RCVD_Data. */
static void pass(void) { doevents(); }

/* One doevents() pass WITH a repeat window open, i.e. "MenuTime has elapsed". */
static void menu_window(void) { MenuTimer = 0; doevents(); }

/* Open the real menu through the real path: RPDO1 bit 0 is ACTIVATE MENU.
 * doevents() then loads the menu, inserts the cursor and sets Menu_Active. */
static void open_menu(void)
{
    gProcImg[OUT_digi_0] |= 0x01;
    pass();
    TEST_ASSERT_TRUE_MESSAGE(Gen_Flags & Gen_Flags_Menu_Active,
        "fixture: ACTIVATE MENU should have opened the menu");
}

void setUp(void)
{
    host_firmware_init();
    memset(sfr_regs, 0, sizeof sfr_regs);
    sfr_regs[0x86] = 0x80;          /* ATD0STAT0 SCF */

    State     = FinishState;        /* coating sequence idle */
    StateTime = 0;

    Gen_Flags     = 0;
    Variable_flag = 0;              /* cursor mode, not variable-edit mode */
    TC0_RCVD_Data = 0;
    CursorUpFlag = CursorDownFlag = SelectFlag = 0;

    set_word(BB_KEEPALIVE);         /* idle: keepalive only, no buttons */
    gProcImg[OUT_digi_0] = 0;

    can_tx_reset();
    can_rx_reset();

    open_menu();
    set_word(BB_KEEPALIVE);
    pass();                         /* settle: TC0 back to idle */
    CursorUpFlag = CursorDownFlag = SelectFlag = 0;
}

void tearDown(void) { }

/* ============================================================
 * The sentinel itself
 * ============================================================ */

/* Executable documentation of the invariant the release test depends on:
 * a signed char assigned -1 must compare equal to -1, whatever the plain-char
 * signedness of the build. */
static void test_char_sentinel_survives_the_round_trip(void)
{
    signed char flag = -1;
    TEST_ASSERT_TRUE_MESSAGE(flag == -1,
        "a signed char holding -1 must match -1 - this is what release detection uses");
}

/* ============================================================
 * Press
 * ============================================================ */

/* Assertions here are deliberately "the cursor MOVED", not "it moved to N".
 * CursorPos wraps at the ends of the menu (Up from position 1 lands on the last
 * item), so an absolute expectation would encode the current menu's length and
 * would break whenever a menu item is added. Movement is the contract; the
 * destination is menu geometry. */
static void test_up_press_moves_the_cursor(void)
{
    int before = cursor_pos();

    set_word(BB_KEEPALIVE | BB_UP);
    pass();              /* word -> TC0_RCVD_Data            */
    menu_window();       /* flag latches 1, CursorUp() runs  */

    TEST_ASSERT_TRUE_MESSAGE(cursor_pos() != before,
        "an Up press should move the cursor");
}

static void test_down_press_moves_the_cursor(void)
{
    int before = cursor_pos();

    set_word(BB_KEEPALIVE | BB_DOWN);
    pass();
    menu_window();

    TEST_ASSERT_EQUAL_INT_MESSAGE(before + 1, cursor_pos(),
        "a Down press should move the cursor down one position "
        "(Down from the top cannot wrap, so this one is exact)");
}

/* ============================================================
 * Hold - the auto-repeat feature must KEEP working. A "fix" that stopped the
 * flag being truthy at the sentinel would break this instead.
 * ============================================================ */

static void test_holding_up_repeats(void)
{
    int p0, p1, p2, p3;

    set_word(BB_KEEPALIVE | BB_UP);      /* held for the whole test */
    pass();
    p0 = cursor_pos();
    menu_window(); p1 = cursor_pos();     /* move 1 */
    menu_window(); p2 = cursor_pos();     /* move 2 */
    menu_window(); p3 = cursor_pos();     /* move 3 */

    TEST_ASSERT_TRUE_MESSAGE(p1 != p0 && p2 != p1 && p3 != p2,
        "holding Up should auto-repeat once per MenuTime window - each window "
        "must move the cursor again");
}

/* ============================================================
 * Release - THE REGRESSION. Reverting `== (char)-1` to `== -1` fails here.
 * ============================================================ */

static void test_release_returns_the_flag_to_idle(void)
{
    set_word(BB_KEEPALIVE | BB_UP);
    pass();
    menu_window();
    /* Works only because the flag is declared signed char: a plain char would
     * hold 255 under -funsigned-char and never match -1. */
    TEST_ASSERT_TRUE_MESSAGE(CursorUpFlag == -1,
        "after acting, the flag should hold the -1 'handled' sentinel");

    set_word(BB_KEEPALIVE);      /* button released */
    pass();                      /* TC0 refreshes to idle */
    pass();                      /* top-of-loop clears the flag */

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, (int)CursorUpFlag,
        "releasing the button must return CursorUpFlag to 0; if this fails the "
        "sentinel comparison is broken (see the file header) and the cursor "
        "will free-run");
}

static void test_release_stops_the_cursor(void)
{
    int settled;

    set_word(BB_KEEPALIVE | BB_UP);
    pass();
    menu_window();               /* one deliberate move */

    set_word(BB_KEEPALIVE);      /* released */
    pass();
    pass();
    settled = cursor_pos();

    /* Ten further repeat windows. With the button up, NONE of them may move
     * the cursor. Before the fix this walked the whole menu. */
    menu_window(); menu_window(); menu_window(); menu_window(); menu_window();
    menu_window(); menu_window(); menu_window(); menu_window(); menu_window();

    TEST_ASSERT_EQUAL_INT_MESSAGE(settled, cursor_pos(),
        "after release the cursor must not move again, however many MenuTime "
        "windows elapse");
}

static void test_release_stops_the_cursor_after_a_hold(void)
{
    int settled;

    set_word(BB_KEEPALIVE | BB_DOWN);
    pass();
    menu_window(); menu_window(); menu_window();   /* held, repeated 3x */

    set_word(BB_KEEPALIVE);
    pass();
    pass();
    settled = cursor_pos();

    menu_window(); menu_window(); menu_window(); menu_window(); menu_window();

    TEST_ASSERT_EQUAL_INT_MESSAGE(settled, cursor_pos(),
        "a hold must also stop cleanly on release, not free-run");
}

/* A second press after a release must still work - i.e. the flag really went
 * back to 0 and can re-arm, rather than being stuck in a state that merely
 * looks idle. */
static void test_second_press_works_after_release(void)
{
    int before, after_first;

    before = cursor_pos();
    set_word(BB_KEEPALIVE | BB_UP);
    pass();
    menu_window();
    after_first = cursor_pos();
    TEST_ASSERT_TRUE_MESSAGE(after_first != before, "first press should move");

    set_word(BB_KEEPALIVE);
    pass();
    pass();

    set_word(BB_KEEPALIVE | BB_UP);
    pass();
    menu_window();

    TEST_ASSERT_TRUE_MESSAGE(cursor_pos() != after_first,
        "a second press after release should move the cursor again - if the "
        "flag never really returned to 0 it could not re-arm");
}

/* ============================================================
 * Select shares the identical tri-state pattern, so it shares the trap.
 * ============================================================ */

static void test_select_release_returns_the_flag_to_idle(void)
{
    set_word(BB_KEEPALIVE | BB_SELECT);
    pass();
    menu_window();

    set_word(BB_KEEPALIVE);
    pass();
    pass();

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, (int)SelectFlag,
        "releasing Select must return SelectFlag to 0");
}

static void test_down_release_returns_the_flag_to_idle(void)
{
    set_word(BB_KEEPALIVE | BB_DOWN);
    pass();
    menu_window();

    set_word(BB_KEEPALIVE);
    pass();
    pass();

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, (int)CursorDownFlag,
        "releasing Down must return CursorDownFlag to 0");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_char_sentinel_survives_the_round_trip);
    RUN_TEST(test_up_press_moves_the_cursor);
    RUN_TEST(test_down_press_moves_the_cursor);
    RUN_TEST(test_holding_up_repeats);
    RUN_TEST(test_release_returns_the_flag_to_idle);
    RUN_TEST(test_release_stops_the_cursor);
    RUN_TEST(test_release_stops_the_cursor_after_a_hold);
    RUN_TEST(test_second_press_works_after_release);
    RUN_TEST(test_select_release_returns_the_flag_to_idle);
    RUN_TEST(test_down_release_returns_the_flag_to_idle);
    return UNITY_END();
}
