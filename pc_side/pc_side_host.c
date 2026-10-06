/* pc_side_host.c
 *
 * PC-only support layer for the 12/48 Coater Rev4.33 host build (GCC, -DPC_SIDE).
 * ImageCraft never compiles this file. It provides the host implementations of
 * everything the production translation units reference but cannot have on a PC:
 *
 *   - sfr_regs[]     backing store for _REG_BASE (mc9s12a128.h under PC_SIDE)
 *   - g_intr_masked  the flag INTR_ON/OFF map to (pc_side.h)
 *   - stubs for the EEPROM/Flash writers, whose real bodies spin on hardware
 *     status bits that never change here
 *   - the CAN transport: MCOHW_PushMessage / MCOHW_PullMessage over UDP, so the
 *     firmware exchanges real CAN frames with the Python emulators in
 *     C:\Working_Projects\can_emulators (same wire format as lib/can_udp.py)
 *   - the RTI simulation thread: drives the real RTI_Int_Handler() so firmware
 *     timers (StateTime, Timer1/2, MenuTimer, ...) count down
 *
 * The REAL menu engine, MicroCANopen stack and node config (Subroutines.c,
 * Subroutines1.c, mco.c, user.c, Interrupts.c) are
 * compiled in, so MCO_ProcessStack genuinely maps RPDOs into gProcImg[] and
 * emits TPDOs. This file only supplies the CAN hardware layer beneath them.
 *
 * Rev4.33 vs Rev4.34
 * ------------------
 * Rev4.33 paces its CAN display frames with Timer1 busy-waits
 * (`Timer1 = RTI_One_Sec * .05; while (Timer1);` in Display(), and the same
 * shape elsewhere). Those are NOT #ifdef'd out for the host build the way
 * Rev4.34's MCOHW_GetTime pacing is — the RTI thread below decrements Timer1
 * exactly as the target's real RTI interrupt does, so the spins simply end.
 * That keeps the production pacing code on the host path instead of compiling
 * a different program than the one that ships.
 */

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#define NOMINMAX
#include <errno.h>          /* before windows.h: TDM-GCC's mm_malloc.h needs EINVAL */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* windows.h defines TRUE/FALSE; the firmware headers redefine them. Drop the
 * Win32 ones so the firmware's definitions win without a warning. */
#undef TRUE
#undef FALSE

#include "nodecfg.h"
#include "mco.h"
#include "mcohw.h"
#include "mc9s12a128.h"     /* CANRFLG, backed by sfr_regs[] */
#include "Subroutines.h"    /* OscClk, which RTI_One_Sec is built from */
#include "Interrupts.h"     /* RTI_One_Sec: ticks/sec the firmware assumes */
#include "pc_log.h"

void RTI_Int_Handler(void);   /* production ISR, ../Interrupts.c */

/* ==========================================================================
 * SFR backing array + host runtime globals
 * ========================================================================== */
unsigned char sfr_regs[0x400];
volatile int  g_intr_masked = 1;    /* masked at reset (see pc_side.h) */

/* ==========================================================================
 * Hardware-fidelity options (--hw-* on the command line, parsed by main.c).
 * On by default, at the target's values, so the SIL loses and delays frames
 * where the target does. --no-hw turns them all off: lossless RX ring, every
 * frame accepted, instant EEPROM writes and transmits.
 * ========================================================================== */
int pc_side_hw_rx_fifo     = 5;   /* RX frames held before overrun (MSCAN: 5); 0 = off */
int pc_side_hw_filters     = 1;   /* apply mcohw.c's MSCAN acceptance filters */
int pc_side_hw_ee_erase_ms = 20;  /* EEPROM sector erase time; 0 = instant */
int pc_side_hw_can_tx      = 1;   /* transmits take bus time, as mcohw.c waits for them */

/* One --hw-* option into the settings above. 1 if it was one. */
int pc_side_hw_option(const char *arg)
{
    if (!strcmp(arg, "--no-hw")) {
        pc_side_hw_rx_fifo = pc_side_hw_filters = pc_side_hw_ee_erase_ms = pc_side_hw_can_tx = 0;
        return 1;
    }
    if (!strncmp(arg, "--hw-rx-fifo=", 13))     { pc_side_hw_rx_fifo     = atoi(arg + 13); return 1; }
    if (!strncmp(arg, "--hw-can-filters=", 17)) { pc_side_hw_filters     = atoi(arg + 17); return 1; }
    if (!strncmp(arg, "--hw-ee-erase-ms=", 17)) { pc_side_hw_ee_erase_ms = atoi(arg + 17); return 1; }
    if (!strncmp(arg, "--hw-can-tx=", 12))      { pc_side_hw_can_tx      = atoi(arg + 12); return 1; }
    return 0;
}

/* The active options as command-line text, for a reset relaunch. */
static void hw_options_text(char *buf, size_t n)
{
    snprintf(buf, n, " --hw-rx-fifo=%d --hw-can-filters=%d --hw-ee-erase-ms=%d --hw-can-tx=%d",
             pc_side_hw_rx_fifo, pc_side_hw_filters, pc_side_hw_ee_erase_ms, pc_side_hw_can_tx);
}

void pc_side_hw_log(void)
{
    char rx[24] = "unlimited", ee[24] = "instant";
    if (pc_side_hw_rx_fifo)     snprintf(rx, sizeof rx, "%d frames", pc_side_hw_rx_fifo);
    if (pc_side_hw_ee_erase_ms) snprintf(ee, sizeof ee, "%d ms/sector", pc_side_hw_ee_erase_ms);
    LOG_PRINTF(("[host] hardware fidelity: RX FIFO %s, CAN filters %s, EEPROM %s, CAN TX %s\n",
                rx, pc_side_hw_filters ? "on" : "off", ee,
                pc_side_hw_can_tx ? "timed (125 kbit/s)" : "instant"));
}

/* ==========================================================================
 * Driver stubs. EEProm.c / Flash.c / mcohw.c are excluded from the build:
 * their real bodies spin on hardware status bits (EEPROM command-complete,
 * CAN transmit-buffer-empty) that never set on a PC.
 * ========================================================================== */
void EEInit(void) { }

/* The real EEWrite erases each 4-byte sector it touches, then programs it,
 * spinning on every command; the erases dominate. */
void EEWrite(int ArraySize, char WriteData[], int *WriteAddr)
{
    (void)WriteData;
    if (pc_side_hw_ee_erase_ms) {
        unsigned lead    = (unsigned)((size_t)WriteAddr & 3);
        unsigned sectors = (lead + (unsigned)ArraySize + 3) / 4;
        Sleep(sectors * (unsigned)pc_side_hw_ee_erase_ms);
    }
}

void FlashInit(void) { }
void FlashWrite(int ArraySize, char WriteData[], int *WriteAddr)
{ (void)ArraySize; (void)WriteData; (void)WriteAddr; }

/* ==========================================================================
 * CAN hardware-layer calls that mco.c makes (mcohw.c is not compiled).
 * Non-zero from Init: 0 makes MCO_Init() treat init as failed and call
 * MCOUSER_FatalError(), which re-enters InitCANOpen() -> infinite recursion.
 * ========================================================================== */
/* The acceptance-filter table mcohw.c owns. user.c clears it on reset, so the
 * symbol must exist even when nothing here filters — the UDP bus delivers
 * every frame and mco.c does its own CAN-ID matching. 0x80 is the "empty slot"
 * marker mcohw.c initialises it with. */
UNSIGNED8 setFilters[8] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80};

/* --hw-can-filters: the MSCAN filters mcohw.c programs. CANIDAC = 0x20 gives
 * eight 8-bit filters on IDR0 (ID10..ID3); set_screener_std writes mask
 * 0xFF80 >> 3 = 0xF0, so only ID bits 6..3 are compared. Unused filters stay
 * code 0 / mask 0, which accepts IDs 0x000-0x007. */
static UNSIGNED8 hw_filter_code[8];
static UNSIGNED8 hw_filter_count = 0;    /* gCANFilter */

UNSIGNED8 MCOHW_Init(UNSIGNED16 BaudRate)
{
    (void)BaudRate;
    hw_filter_count = 0;
    return 1;
}

/* Same bookkeeping as mcohw.c: one filter per distinct CANID & 0x7F. */
UNSIGNED8 MCOHW_SetCANFilter(UNSIGNED16 CANID)
{
    UNSIGNED8 i, bin = CANID & 0x7F;
    if (!pc_side_hw_filters)
        return 1;
    for (i = 0; i < 8; i++)
        if (setFilters[i] == bin)
            return 1;
    if (hw_filter_count >= NR_OF_RPDOS)
        return 0;
    hw_filter_code[hw_filter_count] = (UNSIGNED8)(CANID >> 3);
    setFilters[hw_filter_count]     = bin;
    hw_filter_count++;
    return 1;
}

static int hw_accepts(UNSIGNED16 id)
{
    UNSIGNED8 i, idr0 = (UNSIGNED8)(id >> 3);
    if (!pc_side_hw_filters)
        return 1;
    for (i = 0; i < hw_filter_count; i++)
        if ((idr0 & 0x0F) == (hw_filter_code[i] & 0x0F))
            return 1;
    return hw_filter_count < 8 && idr0 == 0;     /* an unused filter */
}

void      MCOHW_TimerISR(void) { }   /* time base is GetTickCount-driven */

/* Real millisecond time base, mirroring mcohw.c's wraparound compare, so the
 * stack's TPDO event/inhibit timers and the heartbeat pace like the target
 * rather than firing every ProcessStack pass. MCO uses these only for
 * non-blocking checks, never a busy-wait, so real time cannot stall the loop. */
UNSIGNED16 MCOHW_GetTime(void) { return (UNSIGNED16)(GetTickCount() & 0xFFFF); }

UNSIGNED8 MCOHW_IsTimeExpired(UNSIGNED16 timestamp)
{
    UNSIGNED16 time_now = MCOHW_GetTime();
    timestamp++;                                /* min runtime, matches mcohw.c */
    if (time_now > timestamp)
        return (UNSIGNED8)((time_now - timestamp) < 0x8000);
    return (UNSIGNED8)((timestamp - time_now) > 0x8000);
}

/* ==========================================================================
 * CAN transport over UDP
 *
 * Wire format, one datagram per frame (length = 3 + LEN) — must match
 * can_emulators' lib/can_udp.py:
 *   byte 0-1 : CAN ID   (uint16, little-endian)
 *   byte 2   : LEN      (0..8)
 *   byte 3.. : LEN data bytes
 *
 * This host binds recv_port and sends to send_port on 127.0.0.1. Point
 * send_port at can_udp_hub.py's bus port to join the shared bus, or at a
 * peer's recv port for a direct two-node link. A background RX thread parses
 * datagrams into a locked ring; MCOHW_PullMessage pops from it (returning 0
 * when empty, exactly like the polled CAN hardware).
 * ========================================================================== */
#define RX_RING_N 128

#ifndef SIO_UDP_CONNRESET               /* not always in the mingw headers */
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif

static SOCKET             can_sock = INVALID_SOCKET;
static struct sockaddr_in can_peer;
static unsigned short     can_recv_port = 0, can_send_port = 0;  /* for a reset relaunch */
static HANDLE             can_rx_thread = NULL;
static volatile int       can_running   = 0;

static CAN_MSG            rx_ring[RX_RING_N];
static volatile unsigned  rx_head = 0, rx_tail = 0;   /* head=write, tail=read */
static CRITICAL_SECTION   rx_lock;

/* ==========================================================================
 * Async, drop-on-full logging.
 *
 * The firmware main thread (via MCO_ProcessStack -> MCOHW_PushMessage -> can_log)
 * and the CAN RX thread both log on hot paths. A synchronous printf blocks the
 * caller whenever the terminal stops draining (VS Code terminal flow control, a
 * full pipe), which would stall the host. So pc_log_printf() only formats into a
 * bounded ring and returns — it never touches stdout and never blocks; a full
 * ring drops the line and counts it. One writer thread drains to stdout, so only
 * that thread can block, and it clears latched stdout errors so output recovers.
 * ========================================================================== */
#define LOGQ_N    1024
#define LOGQ_LINE 200
static char              logq[LOGQ_N][LOGQ_LINE];
static volatile unsigned logq_head = 0, logq_tail = 0;
static volatile unsigned logq_dropped = 0;
static CRITICAL_SECTION  logq_lock;
static HANDLE            log_thread  = NULL;
static volatile int      log_running = 0;

static DWORD WINAPI log_thread_fn(LPVOID arg)
{
    (void)arg;
    for (;;) {
        char     line[LOGQ_LINE];
        unsigned dropped = 0;
        int      have = 0;
        EnterCriticalSection(&logq_lock);
        if (logq_tail != logq_head) {
            strcpy(line, logq[logq_tail]);
            logq_tail = (logq_tail + 1) % LOGQ_N;
            have = 1;
            if (logq_dropped) { dropped = logq_dropped; logq_dropped = 0; }
        }
        LeaveCriticalSection(&logq_lock);          /* fputs OUTSIDE the lock */
        if (have) {
            fputs(line, stdout);
            if (dropped)
                fprintf(stdout, "[log] dropped %u line(s) - output too slow\n", dropped);
            if (ferror(stdout)) clearerr(stdout);
        } else {
            if (!log_running) break;               /* stop only once drained */
            Sleep(2);
        }
    }
    fflush(stdout);
    return 0;
}

void pc_log_init(void)
{
    if (log_thread) return;
    InitializeCriticalSection(&logq_lock);
    log_running = 1;
    log_thread  = CreateThread(NULL, 0, log_thread_fn, NULL, 0, NULL);
}

void pc_log_shutdown(void)
{
    if (!log_thread) return;
    log_running = 0;                     /* writer drains the tail, then exits */
    WaitForSingleObject(log_thread, 2000);
    CloseHandle(log_thread);
    log_thread = NULL;
    DeleteCriticalSection(&logq_lock);
}

void pc_log_printf(const char *fmt, ...)
{
    char     line[LOGQ_LINE];
    va_list  ap;
    unsigned nxt;
    va_start(ap, fmt);
    vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    line[sizeof line - 1] = '\0';
    if (!log_thread) { fputs(line, stdout); return; }   /* pre-init: direct */
    EnterCriticalSection(&logq_lock);
    nxt = (logq_head + 1) % LOGQ_N;
    if (nxt == logq_tail) {
        logq_dropped++;                                /* full: drop, never block */
    } else {
        strcpy(logq[logq_head], line);
        logq_head = nxt;
    }
    LeaveCriticalSection(&logq_lock);
}

static void can_log(const char *dir, UNSIGNED16 id, const unsigned char *buf, int len)
{
    char hex[3 * 8 + 1];
    int  i, n = 0;
    SYSTEMTIME t;
    for (i = 0; i < len && i < 8; i++)
        n += sprintf(hex + n, "%02X ", buf[i]);
    hex[n] = '\0';
    GetLocalTime(&t);
    LOG_PRINTF(("[%02d:%02d:%02d.%03d] [CAN %s] 0x%03X [%d] %s\n",
                t.wHour, t.wMinute, t.wSecond, t.wMilliseconds,
                dir, id, len, hex));
}

static DWORD WINAPI can_rx_fn(LPVOID arg)
{
    unsigned char pkt[16];
    (void)arg;
    while (can_running) {
        int n = recvfrom(can_sock, (char *)pkt, sizeof pkt, 0, NULL, NULL);
        if (n < 3) { Sleep(1); continue; }     /* no data (WSAEWOULDBLOCK)/short */
        {
            CAN_MSG  m;
            unsigned nxt, held;
            int      overrun = 0;
            m.ID  = (UNSIGNED16)(pkt[0] | (pkt[1] << 8));
            m.LEN = pkt[2];
            if (m.LEN > 8) m.LEN = 8;
            if (n < 3 + (int)m.LEN) continue;
            memcpy(m.BUF, pkt + 3, m.LEN);
            if (!hw_accepts(m.ID))
                continue;                      /* the MSCAN filters never see it */

            EnterCriticalSection(&rx_lock);
            held = (rx_head + RX_RING_N - rx_tail) % RX_RING_N;
            nxt  = (rx_head + 1) % RX_RING_N;
            if (pc_side_hw_rx_fifo && held >= (unsigned)pc_side_hw_rx_fifo) {
                overrun = 1;                   /* MSCAN discards the new frame */
            } else if (nxt != rx_tail) {       /* drop if full */
                rx_ring[rx_head] = m;
                rx_head = nxt;
            }
            LeaveCriticalSection(&rx_lock);
            if (overrun) {
                CANRFLG |= 0x02;               /* OVRIF, as the target would set */
                can_log("RX OVERRUN, dropped", m.ID, m.BUF, m.LEN);
            } else {
                can_log("RX", m.ID, m.BUF, m.LEN);
            }
        }
    }
    return 0;
}

/* Bring up the UDP CAN bus. Returns 0 on success. */
int pc_side_can_init(unsigned short recv_port, unsigned short send_port)
{
    WSADATA wsa;
    struct sockaddr_in me;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    can_recv_port = recv_port;      /* a reset relaunches us on these */
    can_send_port = send_port;

    can_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (can_sock == INVALID_SOCKET) return 2;

    memset(&me, 0, sizeof me);
    me.sin_family      = AF_INET;
    me.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    me.sin_port        = htons(recv_port);
    if (bind(can_sock, (struct sockaddr *)&me, sizeof me) == SOCKET_ERROR) return 3;

    /* Windows UDP: a prior sendto to an unbound port raises ICMP "port
     * unreachable", which would make the NEXT recv fail with WSAECONNRESET.
     * Disable that so a peer that is not up yet cannot poison RX. */
    {
        BOOL  off = 0;
        DWORD ret = 0;
        WSAIoctl(can_sock, SIO_UDP_CONNRESET, &off, sizeof off,
                 NULL, 0, &ret, NULL, NULL);
    }

    /* Non-blocking: MCOHW_PushMessage must never block the firmware thread. A
     * blocking sendto stalls when a peer's receive buffer saturates under the
     * display-frame stream. Non-blocking drops instead (UDP is lossy anyway). */
    {
        u_long nb = 1;
        ioctlsocket(can_sock, FIONBIO, &nb);
    }

    memset(&can_peer, 0, sizeof can_peer);
    can_peer.sin_family      = AF_INET;
    can_peer.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    can_peer.sin_port        = htons(send_port);

    InitializeCriticalSection(&rx_lock);
    can_running   = 1;
    can_rx_thread = CreateThread(NULL, 0, can_rx_fn, NULL, 0, NULL);

    LOG_PRINTF(("[can] UDP bus up: recv :%u  send :%u\n", recv_port, send_port));
    return 0;
}

void pc_side_can_shutdown(void)
{
    can_running = 0;
    if (can_rx_thread) {
        WaitForSingleObject(can_rx_thread, 1000);
        CloseHandle(can_rx_thread);
        can_rx_thread = NULL;
    }
    if (can_sock != INVALID_SOCKET) {
        closesocket(can_sock);
        can_sock = INVALID_SOCKET;
    }
    DeleteCriticalSection(&rx_lock);
    WSACleanup();
}

extern unsigned int Timer1;
extern char         Gen_Flags;

/* --hw-can-tx: mcohw.c arms Timer1 for 0.5 s and spins until the frame is on
 * the bus, so every send blocks for its bit time and leaves Timer1 near 0.5 s.
 * 125 kbit/s = 8 us/bit; 47 overhead bits + 8 per byte, no stuffing, idle bus. */
static void hw_tx_wait(int len)
{
    LARGE_INTEGER freq, start, now;
    double us = (47 + 8 * len) * 8.0;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    Timer1 = 0.5 * RTI_One_Sec;
    do {
        QueryPerformanceCounter(&now);
    } while ((double)(now.QuadPart - start.QuadPart) * 1e6 / (double)freq.QuadPart < us
             && Timer1);
    if (!Timer1)
        Gen_Flags |= Gen_Flags_No2Wire;     /* as mcohw.c on a transmit timeout */
}

/* TX seam: firmware -> bus. */
UNSIGNED8 MCOHW_PushMessage(CAN_MSG *m)
{
    unsigned char pkt[11];
    int len;
    if (can_sock == INVALID_SOCKET) return 1;       /* CAN not up: drop */
    pkt[0] = (unsigned char)(m->ID & 0xFF);
    pkt[1] = (unsigned char)((m->ID >> 8) & 0xFF);
    pkt[2] = m->LEN;
    len = m->LEN; if (len > 8) len = 8;
    memcpy(pkt + 3, m->BUF, len);
    if (pc_side_hw_can_tx)
        hw_tx_wait(len);                    /* receivers see it once it is sent */
    if (sendto(can_sock, (const char *)pkt, 3 + len, 0,
               (struct sockaddr *)&can_peer, sizeof can_peer) == SOCKET_ERROR) {
        /* Host-side loss (e.g. send buffer full); the target would not drop it */
        char dir[40];
        snprintf(dir, sizeof dir, "TX SIL SEND FAILED (err %d), dropped", WSAGetLastError());
        can_log(dir, m->ID, m->BUF, len);
        return 1;
    }
    can_log("TX", m->ID, m->BUF, len);
    return 1;
}

/* RX seam: bus -> firmware. 1 and *m filled if a frame is waiting, else 0 —
 * exactly the contract of the polled CAN-hardware pull. */
UNSIGNED8 MCOHW_PullMessage(CAN_MSG *m)
{
    int got = 0;
    EnterCriticalSection(&rx_lock);
    if (rx_tail != rx_head) {
        *m = rx_ring[rx_tail];
        rx_tail = (rx_tail + 1) % RX_RING_N;
        got = 1;
    }
    LeaveCriticalSection(&rx_lock);
    return (UNSIGNED8)got;
}

/* ==========================================================================
 * RTI simulation thread
 *
 * Drives the production RTI_Int_Handler() so firmware timers advance. The
 * handler is skipped while interrupts are masked (g_intr_masked), giving the
 * firmware's INTR_OFF()/INTR_ON() critical sections real meaning here.
 *
 * On Rev4.33 this thread is load-bearing, not just a convenience: Display(),
 * and PositionDisplay() pace their CAN frames with
 * `Timer1 = <n>; while (Timer1);`, and Timer1 is decremented ONLY by
 * RTI_Int_Handler(). Without these ticks the first Display() call in the
 * coating sequence spins forever.
 * ========================================================================== */
static volatile int          rti_running   = 0;
static volatile unsigned int rti_period_ms = 1;
static HANDLE                rti_handle    = NULL;
static volatile unsigned int rti_tick_count    = 0;
static volatile unsigned int rti_skipped_count = 0;

/* Never fire more than this many ticks from one wakeup. Without a cap, a long
 * stall (a breakpoint, a suspended VM) would come back and dump tens of
 * thousands of ticks at once, expiring every firmware timer simultaneously. */
#define RTI_MAX_CATCHUP 100

static DWORD WINAPI rti_thread_fn(LPVOID arg)
{
    LARGE_INTEGER freq, now, prev;
    double owed = 0.0;          /* fractional ticks the clock says we still owe */

    (void)arg;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);

    while (rti_running) {
        Sleep(rti_period_ms);
        if (!rti_running) break;

        /* The number of ticks owed comes from the CLOCK, not from how many
         * times we woke up. A 1 ms Sleep does not take 1 ms on Windows — with
         * the default ~15.6 ms timer granularity it lands much longer, so a
         * one-tick-per-wakeup thread falls far short of the RTI_One_Sec the
         * firmware's timers assume, and everything timed runs long by exactly
         * that shortfall. Deriving the count from QueryPerformanceCounter makes
         * the simulated RTI track wall-clock however coarse the wakeups are. */
        QueryPerformanceCounter(&now);
        owed += (double)(now.QuadPart - prev.QuadPart)
              / (double)freq.QuadPart * (double)RTI_One_Sec;
        prev = now;

        if (owed > (double)RTI_MAX_CATCHUP) owed = (double)RTI_MAX_CATCHUP;

        while (owed >= 1.0) {
            if (g_intr_masked) {
                /* Leave the debt on the books: the ticks fire as soon as the
                 * firmware re-enables interrupts, so a critical section delays
                 * time rather than destroying it. */
                rti_skipped_count++;
                break;
            }
            RTI_Int_Handler();
            rti_tick_count++;
            owed -= 1.0;
        }
    }
    return 0;
}

unsigned int pc_side_rti_ticks(void)   { return rti_tick_count; }
unsigned int pc_side_rti_skipped(void) { return rti_skipped_count; }

void rti_thread_start_realtime(unsigned int period_ms)
{
    rti_tick_count = 0;
    rti_period_ms  = period_ms ? period_ms : 1;
    rti_running    = 1;
    rti_handle     = CreateThread(NULL, 0, rti_thread_fn, NULL, 0, NULL);
}

void rti_thread_stop(void)
{
    rti_running = 0;
    if (rti_handle) {
        WaitForSingleObject(rti_handle, 2000);
        CloseHandle(rti_handle);
        rti_handle = NULL;
    }
}

/* ==========================================================================
 * Processor reset
 *
 * ResetProc() (Subroutines.c) resets the unit the hardware way: arm the fastest
 * COP rate, then spin in while(1) until the watchdog fires. There is no COP
 * here, so that spin is forever and RestoreDefaults() - which ends in ResetProc
 * - hangs the host. main.c's stall detector spots the spin and calls this.
 *
 * A reset is a RELAUNCH, not a jump back into main(): the restore only works
 * because startup re-initializes every global from its initializers (EEWrite is
 * a stub above and SKIP_EEPROM_LOAD skips the load, so a fresh process comes up
 * on the compiled-in defaults, exactly as the target does after the 0xFF flag).
 *
 * Called from the stall-detector thread, with the firmware thread still
 * spinning in ResetProc - nothing is asked of it. Closing the socket FIRST is
 * required, not tidiness: the child binds the same recv port and would fail
 * while we still hold it.
 * ========================================================================== */
void pc_side_reset(void)
{
    char exe[MAX_PATH];
    char cmd[MAX_PATH + 128];
    char hw[96];
    STARTUPINFOA        si;
    PROCESS_INFORMATION pi;

    LOG_PRINTF(("[host] *** RESET: relaunching on :%u -> :%u ***\n",
                can_recv_port, can_send_port));

    rti_thread_stop();
    pc_side_can_shutdown();

    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    memset(&pi, 0, sizeof pi);

    if (GetModuleFileNameA(NULL, exe, sizeof exe)) {
        hw_options_text(hw, sizeof hw);
        snprintf(cmd, sizeof cmd, "\"%s\" %u %u%s",
                 exe, can_recv_port, can_send_port, hw);
        /* Ports as arguments so the child skips the interactive prompts.
         * Inherit handles: without it a redirected run (a tee, a log capture)
         * loses the child's output at the reset. The socket is already closed,
         * so there is nothing else worth inheriting. A literal 1, not TRUE:
         * this build's mco.h defines TRUE as `1;`, semicolon included. */
        if (CreateProcessA(exe, cmd, NULL, NULL, 1, 0, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        } else {
            LOG_PRINTF(("[host] relaunch failed (%lu)\n", GetLastError()));
        }
    }

    pc_log_shutdown();          /* drain the tail before we go */
    ExitProcess(0);
}
