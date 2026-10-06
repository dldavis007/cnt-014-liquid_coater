/* test_smoke.c — proves the host harness is wired up:
 * the production sources link, the RTI sim drives real firmware timers, and
 * the CAN seams capture/inject frames.
 *
 * Rev4.33 note: the RTI ISR here is monolithic (../Interrupts.c drives
 * StateTime and every other timer directly), so unlike Rev4.34 there is no
 * Register_RTI_Callback step — the sim thread calls RTI_Int_Handler() and the
 * firmware's timers move.
 */

#include "unity.h"
#include "test_support.h"

#include "mc9s12a128.h"
#include "mcohw.h"          /* MCOHW_PushMessage / MCOHW_PullMessage */
#include "Subroutines.h"
#include "Subroutines1.h"

extern unsigned long StateTime;
extern unsigned int  Timer1;

void setUp(void)    { can_tx_reset(); can_rx_reset(); }
void tearDown(void) { }

/* sfr_regs[] backs every SFR macro under PC_SIDE, so a register write is just
 * an array write — no fault, and it reads back. */
static void test_sfr_access_is_backed_by_array(void)
{
    PORTA = 0x5A;
    TEST_ASSERT_EQUAL_HEX8(0x5A, PORTA);
}

/* The real RTI ISR runs on the host and drives StateTime, the tick count every
 * coating-sequence timeout is measured against. */
static void test_rti_ticks_advance_state_time(void)
{
    rti_thread_start();          /* deterministic: N calls == N ISRs */
    StateTime = 0;

    advance_ticks(50);

    TEST_ASSERT_EQUAL_UINT(50, rti_ticks());
    TEST_ASSERT_EQUAL_UINT32(50, StateTime);
    rti_thread_stop();
}

/* The pacing service is what keeps Rev4.33's production `while (Timer1)` waits
 * in Display()/PositionDisplay() from hanging the suite. Prove it clears Timer1
 * without the RTI sim running at all — that separation is the whole point:
 * StateTime must stay under advance_ticks() control. */
static void test_pacing_service_clears_timer1_without_advancing_state_time(void)
{
    unsigned long before;
    int spins = 0;

    pacing_thread_start();       /* idempotent; host_firmware_init also starts it */
    before = StateTime;
    Timer1 = 1000;

    while (Timer1 && ++spins < 100000000) { }   /* the production idiom */

    TEST_ASSERT_EQUAL_UINT(0, Timer1);
    TEST_ASSERT_EQUAL_UINT32(before, StateTime);
}

static void test_can_tx_seam_captures_frames(void)
{
    CAN_MSG m;
    m.ID = 0x361; m.LEN = 1; m.BUF[0] = 2;

    MCOHW_PushMessage(&m);

    TEST_ASSERT_EQUAL_UINT(1, can_tx_count);
    TEST_ASSERT_EQUAL_HEX16(0x361, can_tx_log[0].ID);
    TEST_ASSERT_EQUAL_HEX8(2, can_tx_log[0].BUF[0]);
}

static void test_can_rx_seam_delivers_frames(void)
{
    UNSIGNED8 data[1] = { 1 };
    CAN_MSG   got;

    can_rx_inject(0x1e1, data, 1);

    TEST_ASSERT_EQUAL_UINT8(1, MCOHW_PullMessage(&got));
    TEST_ASSERT_EQUAL_HEX16(0x1e1, got.ID);
    TEST_ASSERT_EQUAL_HEX8(1, got.BUF[0]);
    TEST_ASSERT_EQUAL_UINT8(0, MCOHW_PullMessage(&got));   /* ring now empty */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sfr_access_is_backed_by_array);
    RUN_TEST(test_rti_ticks_advance_state_time);
    RUN_TEST(test_pacing_service_clears_timer1_without_advancing_state_time);
    RUN_TEST(test_can_tx_seam_captures_frames);
    RUN_TEST(test_can_rx_seam_delivers_frames);
    return UNITY_END();
}
