#!/usr/bin/env python3
"""
node_scanner.py

Discovers the units on the bus the way the REAL button box does, so the
emulator's menu list is whatever is actually out there rather than a hard-coded
guess. Mirrors Scan() in the button box firmware
(TEST-006-Test-Button-Box/Rev_3.04.../Subroutines.c).

────────────────────────────────────────────────────────────────────────────
Protocol

1. Probe: send an expedited SDO upload request to every node's SDO server,
   index 0x2100 subindex 0:

       ID   0x600 + node_id            (the box scans node 0x10 .. 0x7F)
       data 40 00 21 00 00 00 00 00     (0x40 = upload request, index LE)

2. Any live unit answers on 0x580 + node_id, so the NODE ID IS DEDUCED FROM
   THE RESPONSE — nothing needs to know in advance who is on the bus.

       data 43 00 21 ss d0 d1 d2 d3     (0x43|((4-len)<<2), index LE, sub, data)

3. Name: read subindexes 1..4 of 0x2100 from each responder. Each carries 4
   bytes, so the four together are a 16-byte name.

4. Placement is decided by the LAST byte of that name:

       0xFF  -> the "AUX UNITS" sub-menu
       else  -> the button box's front menu (the firmware forces it to 0x32)

   Known units:
       FBE COATER      + 0x32  (CNT-23, node 0x6E)  -> front menu
       INSP. B0DY      + 0xFF  (CNT-31, node 0x69)  -> AUX UNITS

5. Selecting a unit opens ITS menu, which is RPDO1 at 0x200 + node_id.

The parsing is a plain class with no Qt so it can be unit-tested; NodeScanner
adds the timer-driven sequencing on top.
"""

from PySide6.QtCore import QObject, QTimer, Signal

SDO_REQ_BASE  = 0x600      # request  -> 0x600 + node
SDO_RSP_BASE  = 0x580      # response <- 0x580 + node
RPDO1_BASE    = 0x200      # a unit's menu/trigger byte -> 0x200 + node
NAME_INDEX    = 0x2100
NAME_SUBS     = (1, 2, 3, 4)   # 4 bytes each = a 16-byte name

AUX_MARKER    = 0xFF       # last name byte -> "AUX UNITS" sub-menu
MAIN_MARKER   = 0x32       # last name byte -> front menu

SCAN_FIRST_NODE = 0x10
SCAN_LAST_NODE  = 0x7F


def parse_sdo_response(can_id, data):
    """(node, index, sub, 4 data bytes) for an SDO upload response, else None."""
    if not (SDO_RSP_BASE < can_id <= SDO_RSP_BASE + 0x7F) or len(data) < 4:
        return None
    if (data[0] & 0xE0) != 0x40:          # 0x4x = upload response / expedited
        return None
    node = can_id - SDO_RSP_BASE
    index = data[1] | (data[2] << 8)
    sub = data[3]
    payload = bytes(data[4:8]).ljust(4, b"\x00")
    return node, index, sub, payload


class DiscoveredUnits:
    """Accumulates scan responses into a unit list. No Qt — unit-testable."""

    def __init__(self):
        self.nodes = {}      # node -> {sub: 4 bytes}

    def feed(self, can_id, data):
        """Returns the node id if this frame was a 0x2100 response, else None."""
        parsed = parse_sdo_response(can_id, data)
        if parsed is None:
            return None
        node, index, sub, payload = parsed
        if index != NAME_INDEX:
            return None
        self.nodes.setdefault(node, {})[sub] = payload
        return node

    @staticmethod
    def _name_bytes(subs):
        out = bytearray()
        for s in NAME_SUBS:
            out += subs.get(s, b"    ")
        return bytes(out)

    def units(self):
        """[{node, name, marker, aux, menu_id}] sorted, front-menu units first."""
        out = []
        for node, subs in self.nodes.items():
            raw = self._name_bytes(subs)
            marker = raw[15] if len(raw) >= 16 else 0
            # Drop the marker byte, then trim padding, for the display name.
            name = raw[:15].decode("latin-1").rstrip(" \x00").strip()
            out.append({
                "node":    node,
                "name":    name or f"node 0x{node:02X}",
                "marker":  marker,
                "aux":     marker == AUX_MARKER,
                "menu_id": RPDO1_BASE + node,
                "raw":     raw,
            })
        # Front-menu units first, then AUX, each by node id — the order the real
        # box builds its menu in.
        out.sort(key=lambda u: (u["aux"], u["node"]))
        return out


class NodeScanner(QObject):
    """Timer-driven SDO scan. Non-blocking so the GUI stays responsive."""

    unit_found = Signal(dict)     # one discovered unit
    finished   = Signal(list)     # full unit list
    progress   = Signal(str)

    def __init__(self, send_fn, parent=None,
                 probe_ms=4, settle_ms=400, name_ms=12):
        super().__init__(parent)
        self._send = send_fn
        self._probe_ms = probe_ms
        self._settle_ms = settle_ms
        self._name_ms = name_ms
        self._found = DiscoveredUnits()
        self._timer = QTimer(self)
        self._timer.timeout.connect(self._step)
        self._queue = []
        self._phase = "idle"

    # ── driven by the host's RX ───────────────────────────────────────────
    def feed(self, can_id, data):
        self._found.feed(can_id, data)

    # ── run ───────────────────────────────────────────────────────────────
    def start(self):
        self._found = DiscoveredUnits()
        self._queue = list(range(SCAN_FIRST_NODE, SCAN_LAST_NODE + 1))
        self._phase = "probe"
        self.progress.emit(f"probing nodes 0x{SCAN_FIRST_NODE:02X}-0x{SCAN_LAST_NODE:02X} ...")
        self._timer.start(self._probe_ms)

    def _sdo_read(self, node, sub):
        self._send(SDO_REQ_BASE + node,
                   [0x40, NAME_INDEX & 0xFF, (NAME_INDEX >> 8) & 0xFF, sub,
                    0x00, 0x00, 0x00, 0x00])

    def _step(self):
        if self._phase == "probe":
            if self._queue:
                self._sdo_read(self._queue.pop(0), 0)
                return
            # Everyone has been asked; let the last replies land.
            self._phase = "settle"
            self._timer.start(self._settle_ms)
            self.progress.emit("waiting for replies ...")
            return

        if self._phase == "settle":
            nodes = sorted(self._found.nodes)
            self.progress.emit(
                f"{len(nodes)} unit(s) replied; reading names ..." if nodes
                else "no units replied")
            # Ask each responder for its name, subindexes 1..4.
            self._queue = [(n, s) for n in nodes for s in NAME_SUBS]
            self._phase = "names"
            self._timer.start(self._name_ms)
            return

        if self._phase == "names":
            if self._queue:
                node, sub = self._queue.pop(0)
                self._sdo_read(node, sub)
                return
            self._phase = "done"
            self._timer.start(self._settle_ms)
            return

        # done
        self._timer.stop()
        self._phase = "idle"
        units = self._found.units()
        for u in units:
            self.unit_found.emit(u)
        self.progress.emit(
            f"found {len(units)} unit(s): " +
            ", ".join(f"{u['name']}{' [AUX]' if u['aux'] else ''}" for u in units)
            if units else "scan complete - no units found")
        self.finished.emit(units)
