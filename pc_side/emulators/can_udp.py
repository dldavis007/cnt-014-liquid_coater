#!/usr/bin/env python3
"""
can_udp.py - UDP drop-in transport for the FBE Coater PC-side host.

Mirrors the surface of CANReceiverPcanBasic that speed_control_emulator.py
(and the other HIL emulators) already use, so switching from a real PEAK
PCAN-USB dongle to the software UDP bus is a one-line change:

    # from CANReceiverPcanBasic import CANReceiverPcanBasic
    from can_udp import CANReceiverUdp
    ...
    # self.can = CANReceiverPcanBasic(desired_baud_rate=baud)
    self.can = CANReceiverUdp()          # talks to pc_side_host.exe over UDP

Everything downstream is unchanged: it exposes message_received_signal
(emitting {'id': int, 'data': [int,...]}), can_error_signal, can_status_signal,
a truthy .pcan_channel, and start()/stop()/send_can_message(id, data).

Wire format (must match pc_side/pc_side_host.c), one datagram per frame:
    byte 0-1 : CAN ID  (uint16, little-endian)
    byte 2   : LEN     (0..8)
    byte 3.. : LEN data bytes

Port convention (defaults): the C host binds 20000 and sends to 20001, so the
Python side binds 20001 (recv) and sends to 20000 (send) - the mirror.
"""

import socket

from PySide6.QtCore import QThread, Signal


class CANReceiverUdp(QThread):
    message_received_signal = Signal(dict)   # {'id': int, 'data': list[int]}
    can_error_signal        = Signal(str)
    can_status_signal       = Signal(str)

    def __init__(self, recv_port=20001, send_port=20000, host="127.0.0.1",
                 desired_baud_rate=None):
        super().__init__()
        # desired_baud_rate is accepted for constructor compatibility with
        # CANReceiverPcanBasic and ignored (a UDP bus has no baud rate).
        self.pcan_channel = 1                # truthy: emulator's _check_conn() passes
        self._peer = (host, send_port)
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # NOTE: intentionally NO SO_REUSEADDR. With it, a second/stale emulator
        # can silently bind this same recv port and steal the incoming datagrams
        # (you can still SEND, but you stop RECEIVING). Without it, a duplicate
        # bind fails loudly right here instead of causing a silent RX blackout.
        try:
            self._sock.bind((host, recv_port))
        except OSError as e:
            raise OSError(
                f"UDP recv port {recv_port} is already in use - another emulator "
                f"instance is still holding it. Close it (or kill stray python.exe) "
                f"and retry.  ({e})"
            )
        # Windows only: a prior sendto() to a port with nothing bound raises an
        # ICMP "port unreachable", which makes the NEXT recvfrom() on this socket
        # fail with WSAECONNRESET — permanently killing RX even after the peer
        # comes up. That is a normal state here: the emulator is often started
        # before the firmware hosts. SIO_UDP_CONNRESET switches that behaviour
        # off, the same guard pc_side_host.c applies on the C side.
        if hasattr(socket, "SIO_UDP_CONNRESET"):
            try:
                self._sock.ioctl(socket.SIO_UDP_CONNRESET, False)
            except OSError:
                pass                          # not Windows, or not supported

        self._sock.settimeout(0.2)           # so run() can observe self._running
        self._running = True

    def send_can_message(self, can_id, data):
        """Send one CAN frame. data: iterable of 0..8 byte values."""
        data = bytes(data)[:8]
        pkt = bytes([can_id & 0xFF, (can_id >> 8) & 0xFF, len(data)]) + data
        try:
            self._sock.sendto(pkt, self._peer)
        except OSError as e:
            self.can_error_signal.emit(f"UDP send failed: {e}")

    def run(self):
        self.can_status_signal.emit("UDP bus connected")
        while self._running:
            try:
                pkt, _ = self._sock.recvfrom(64)
            except socket.timeout:
                continue
            except OSError as e:
                # Report but KEEP LISTENING. Breaking here turns a transient
                # per-datagram error (a peer that is not up yet) into a silent,
                # permanent RX blackout that looks like "the bus is dead".
                self.can_error_signal.emit(f"UDP recv failed: {e}")
                continue
            if len(pkt) < 3:
                continue
            can_id = pkt[0] | (pkt[1] << 8)
            n = min(pkt[2], 8)
            self.message_received_signal.emit({'id': can_id, 'data': list(pkt[3:3 + n])})

    def stop(self):
        self._running = False
        self.wait()
        try:
            self._sock.close()
        except OSError:
            pass
