/* host_can.c — the MicroCANopen hardware layer (mcohw.c's entry points) over a
 * UDP CAN bus, so the real mco.c exchanges frames with the Python emulators in
 * C:\Working_Projects\can_emulators (same wire format as lib/can_udp.py).
 *
 * mcohw.c is not compiled: it spins on CAN transmit-buffer status bits that
 * never set on a PC.
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
#include <stdio.h>

/* windows.h defines TRUE/FALSE; the firmware headers redefine them. */
#undef TRUE
#undef FALSE

#include "nodecfg.h"
#include "mco.h"
#include "mcohw.h"
#include "pc_core.h"
#include "pc_log.h"

/* ==========================================================================
 * CAN hardware-layer calls that mco.c makes.
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
static HANDLE             can_rx_thread = NULL;
static volatile int       can_running   = 0;

static CAN_MSG            rx_ring[RX_RING_N];
static volatile unsigned  rx_head = 0, rx_tail = 0;   /* head=write, tail=read */
static CRITICAL_SECTION   rx_lock;

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
                *pc_side_unit->can_rflg |= 0x02;   /* OVRIF, as the target would set */
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

/* --hw-can-tx: mcohw.c arms a timer and spins until the frame is on the bus,
 * so every send blocks for its bit time and leaves that timer near its reload.
 * 125 kbit/s = 8 us/bit; 47 overhead bits + 8 per byte, no stuffing, idle bus. */
static void hw_tx_wait(int len)
{
    const struct pc_side_unit *u = pc_side_unit;
    LARGE_INTEGER freq, start, now;
    double us = (47 + 8 * len) * 8.0;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    if (u->tx_timer)
        *u->tx_timer = u->tx_timeout;
    do {
        QueryPerformanceCounter(&now);
    } while ((double)(now.QuadPart - start.QuadPart) * 1e6 / (double)freq.QuadPart < us
             && (!u->tx_timer || *u->tx_timer));
    if (u->tx_timer && !*u->tx_timer && u->tx_timed_out)
        u->tx_timed_out();
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
