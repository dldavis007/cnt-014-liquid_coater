"""Headless button-box probe for the Rev4.33 pc_side host.

Reproduces "one Up press and the menu cursor keeps scrolling" without the GUI,
so the firmware's flag latching can be observed in isolation.

Stimulus (mirrors emulators/button_box_panel.py exactly):
  0x180, 3 bytes, little-endian word in bytes 0-1, sent every 100 ms (heartbeat)
      0x0400 KeepAlive (idle)   0x0002 Up   0x0001 Down   0x0004 Select
  0x26E, 1 byte : 0x01 = ACTIVATE MENU (momentary)

Timeline: idle -> open menu -> ONE Up pulse of PULSE_MS -> idle forever.
If the cursor keeps moving after the pulse ends, the firmware latched.
"""
import socket, struct, sys, time

HOST_RECV = 20910          # the pc_side host binds this
MY_PORT   = 20900          # we stand in for the hub

BB_ID, MENU_ID = 0x180, 0x26E
KEEPALIVE, BB_UP = 0x0400, 0x0002
PULSE_MS = int(sys.argv[1]) if len(sys.argv) > 1 else 120

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(("127.0.0.1", MY_PORT))
s.setblocking(False)
peer = ("127.0.0.1", HOST_RECV)

def tx(can_id, data):
    s.sendto(struct.pack("<HB", can_id, len(data)) + bytes(data), peer)

def word(w):
    tx(BB_ID, [w & 0xFF, (w >> 8) & 0xFF, 0x00])

def drain():
    """Collect 0x310 display chunks and reassemble the menu text."""
    out = []
    try:
        while True:
            pkt, _ = s.recvfrom(64)
            if len(pkt) < 3:
                continue
            cid, ln = struct.unpack("<HB", pkt[:3])
            if cid == 0x310:
                out.append(pkt[3:3 + ln])
    except (BlockingIOError, ConnectionResetError):
        # ConnectionResetError = a previous sendto drew an ICMP "port
        # unreachable" (host not up yet, or a stale one just died).
        # Windows surfaces that on the NEXT recv; it is not our error.
        pass
    return out

def cursor_line(frames):
    """Pull the '>' cursor item out of the reassembled display stream."""
    txt = ""
    for f in frames:
        if len(f) >= 2 and f[1] == 0x02:
            txt = f[2:].decode("latin1")
        elif len(f) >= 2 and f[1] == 0x03:
            pass
        else:
            txt += f[1:].decode("latin1")
    if ">" in txt:
        seg = txt.split(">", 1)[1]
        return ">" + seg[:18].strip()
    return None

def phase(label, secs, w):
    end = time.time() + secs
    seen = []
    while time.time() < end:
        word(w)
        time.sleep(0.1)
        c = cursor_line(drain())
        if c and (not seen or seen[-1] != c):
            seen.append(c)
    print(f"  {label:<28} word=0x{w:04X}  cursor moves: {len(seen)}")
    for c in seen:
        print(f"      {c}")
    return seen

print(f"probe -> :{HOST_RECV}, listening on :{MY_PORT}, Up pulse = {PULSE_MS} ms\n")

print("[1] idle, no menu")
phase("idle", 1.5, KEEPALIVE)

print("\n[2] ACTIVATE MENU (0x26E bit 0)")
tx(MENU_ID, [0x01]); time.sleep(0.2); tx(MENU_ID, [0x00])
phase("menu open, idle word", 2.0, KEEPALIVE)

print(f"\n[3] ONE Up pulse ({PULSE_MS} ms)")
end = time.time() + PULSE_MS / 1000.0
while time.time() < end:
    word(KEEPALIVE | BB_UP)
    time.sleep(0.02)
drain()

print("\n[4] pulse OVER - idle word only. Cursor must NOT move again.")
moved = phase("after release", 4.0, KEEPALIVE)

print()
if len(moved) > 1:
    print(f"*** LATCHED: cursor moved {len(moved)} times after the button was released.")
else:
    print("OK: cursor settled after release.")
