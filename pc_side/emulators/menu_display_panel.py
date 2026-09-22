#!/usr/bin/env python3
"""
menu_display_panel.py

Reusable WIM display panel (a QGroupBox) that renders what the FBE coater
(CNT-23) sends to the display on 0x310 — the menu screen and the "Proc:" status
strings. Transport-agnostic, exactly like button_box_panel.py: the host feeds it
frames with `feed(can_id, data)` and this panel decodes and draws them.

Pair it with ButtonBoxPanel to get a working operator console: the button box
sends Up/Down/Select on 0x180 and MENU on 0x26E, and this panel shows the menu
those keys are walking — so menu variables (actuator speeds, LA extend/retract
times, MaxLADist, coating length ...) can be read and edited with the firmware
running on the PC, with no hardware display.

────────────────────────────────────────────────────────────────────────────
Wire protocol (SourceFiles/menu_system.c)

Everything is on WIM_ID = 0x310 and is framed STX .. ETX:

  STX frame      BUF[1] == 0x02, payload = BUF[2 : LEN]
  continuation   payload = BUF[1 : LEN]
  ETX            BUF[1] == 0x03  (an 0x03 anywhere in a payload also ends it)

BUF[0] is NODE_ID (0x6E) on Display() frames and 0x00 on menu-draw frames; it is
not part of the payload either way.

Completed messages are one of:

  "Home:"   cursor home        (PositionDisplay)
  "Clr:"    clear screen       (ClearTitler)
  "Menu:"   start of a menu screen redraw (DisplayTitler / ProcessDisplayState)
  anything else   a status line from Display("Proc:...")

After a "Menu:" header the firmware streams the screen as 5-character chunks,
4 chunks per 20-character line, for lines 0..MAX_DISPLAY_LINES (9 lines), then
sends ETX. Those chunks arrive as ordinary continuation frames, so they are
accumulated and re-split here.

Note the two chunk sizes: Display() continuations carry 7 chars (LEN 8) while
menu-draw chunks carry 5 (LEN 7, with BUF[6] a NUL pad). Taking BUF[1:LEN] and
stripping trailing NULs handles both.
"""

import datetime

from PySide6.QtWidgets import (
    QGroupBox, QVBoxLayout, QHBoxLayout, QLabel, QPushButton, QPlainTextEdit,
)
from PySide6.QtGui import QFont

WIM_ID = 0x310

STX = 0x02
ETX = 0x03

# Both boards send their menu to WIM_ID. Status strings carry the sender in
# BUF[0]; menu redraws do not (both use 0), which is harmless because only one
# node can have its menu open at a time.
NODE_NAMES = {
    0x6E: "CNT-23 coater",
    0x69: "CNT-31 body",
}

MENU_LINE_LEN   = 20   # MAX_LINE_LENGTH
MENU_CHUNK      = 5    # chars per menu-draw chunk
MENU_MAX_LINES  = 9    # lines 0..MAX_DISPLAY_LINES inclusive


class MenuDisplayDecoder:
    """Pure decoder for the 0x310 stream — no Qt, so it is unit-testable.

    Call feed(data) with each 0x310 frame's byte list. Returns a list of
    (kind, payload) events, kind being one of:
        'home'   'clear'   'status'   'menu'
    'status' payload is the text; 'menu' payload is a list of screen lines.
    Each event is (kind, payload, node), node being BUF[0] of the opening frame
    (NODE_ID for a status string, 0 for a menu redraw).
    """

    def __init__(self):
        self._buf = ""          # text accumulated for the message in flight
        self._in_msg = False
        self._menu_mode = False  # collecting screen chunks after a "Menu:" header
        self._menu_chunks = ""
        self._node = 0          # BUF[0] of the STX that opened the message

    @staticmethod
    def _payload(data, start):
        """Bytes start..LEN as text, trailing NUL padding removed."""
        raw = bytes(data[start:])
        return raw.decode("latin-1").rstrip("\x00")

    def feed(self, data):
        if len(data) < 2:
            return []

        events = []

        if data[1] == STX:
            # STX always starts a fresh message, and anything half-collected is
            # abandoned. That includes a menu redraw in progress: Display() is
            # called straight from the coating sequence and really does cut into
            # ProcessDisplayState's chunk stream, so without this the status text
            # is appended to the screen buffer and surfaces as a bogus menu line
            # (seen live as a screen reading "Proc:Retracting"). A half-drawn
            # screen is dropped rather than reported — the next redraw is whole.
            self._menu_mode = False
            self._menu_chunks = ""
            self._buf = self._payload(data, 2)
            self._in_msg = True
            # BUF[0] is the sending node on a status string (Display() sets it to
            # NODE_ID) and 0 on a menu redraw, where both boards use 0. Both
            # nodes share WIM_ID 0x310, so this is the only way to tell whose
            # "Proc:" text just arrived — screens are unambiguous in practice
            # because only one menu can be open at a time.
            self._node = data[0]
        elif data[1] == ETX:
            events += self._finish()
            return events
        else:
            if not self._in_msg and not self._menu_mode:
                return []          # mid-stream junk before any STX; ignore
            self._buf += self._payload(data, 1)

        # An ETX embedded in the payload (Home:/Clr: pack it inline) ends it.
        if ETX in self._buf.encode("latin-1"):
            self._buf = self._buf.split("\x03", 1)[0]
            events += self._finish()
            return events

        # "Menu:" is a header, not a message: switch to collecting screen chunks.
        if self._in_msg and self._buf.rstrip() == "Menu:":
            self._buf = ""
            self._in_msg = False
            self._menu_mode = True
            self._menu_chunks = ""
            return events

        if self._menu_mode and not self._in_msg:
            self._menu_chunks += self._buf
            self._buf = ""

        return events

    def _finish(self):
        """Close out whatever is in flight and classify it."""
        events = []

        if self._menu_mode:
            text = self._menu_chunks + self._buf
            lines = [text[i:i + MENU_LINE_LEN]
                     for i in range(0, len(text), MENU_LINE_LEN)]
            lines = [l for l in lines if l.strip()][:MENU_MAX_LINES]
            if lines:
                events.append(("menu", lines, self._node))
            self._menu_mode = False
            self._menu_chunks = ""
        elif self._in_msg:
            text = self._buf.strip()
            if text == "Home:":
                events.append(("home", text, self._node))
            elif text == "Clr:":
                events.append(("clear", text, self._node))
            elif text:
                events.append(("status", text, self._node))

        self._buf = ""
        self._in_msg = False
        return events


class MenuDisplayPanel(QGroupBox):
    """Renders the decoded 0x310 stream: the menu screen plus a status log."""

    def __init__(self, title="WIM Display  (0x310)", parent=None):
        super().__init__(title, parent)
        self._dec = MenuDisplayDecoder()

        root = QVBoxLayout(self)

        mono = QFont("Consolas")
        mono.setStyleHint(QFont.Monospace)
        mono.setPointSize(11)

        # ── the menu screen: fixed 20-column monospace block ──────────────
        self.screen = QLabel("")
        self.screen.setFont(mono)
        self.screen.setStyleSheet(
            "background:#101418; color:#7CFC7C; padding:8px;"
            "border:1px solid #2a3138;")
        self.screen.setMinimumHeight(190)
        self.screen.setTextInteractionFlags(self.screen.textInteractionFlags())
        root.addWidget(self.screen)
        self._set_screen(["(waiting for a menu redraw —",
                          " press MENU on the button box)"])

        # ── latest status line from Display("Proc:...") ──────────────────
        row = QHBoxLayout()
        row.addWidget(QLabel("Status:"))
        self.status = QLabel("—")
        self.status.setFont(mono)
        row.addWidget(self.status, 1)
        clr = QPushButton("Clear log")
        clr.clicked.connect(lambda: self.log.clear())
        row.addWidget(clr)
        root.addLayout(row)

        # ── decoded-message log ──────────────────────────────────────────
        self.log = QPlainTextEdit()
        self.log.setReadOnly(True)
        self.log.setFont(mono)
        self.log.setMaximumBlockCount(400)
        self.log.setMinimumHeight(90)
        root.addWidget(self.log, 1)

    # ── host feeds every received frame here ─────────────────────────────
    def feed(self, can_id, data):
        if can_id != WIM_ID:
            return
        for kind, payload, node in self._dec.feed(list(data)):
            who = NODE_NAMES.get(node, f"node 0x{node:02X}") if node else ""
            tag = f" ({who})" if who else ""
            if kind == "menu":
                self._set_screen(payload)
                self._log(f"[menu] redraw, {len(payload)} lines")
            elif kind == "status":
                self.status.setText(f"{payload}{tag}")
                self._log(f"[text]{tag} {payload}")
            elif kind == "clear":
                self._set_screen([])
                self._log(f"[cmd ]{tag} clear screen")
            elif kind == "home":
                self._log(f"[cmd ]{tag} cursor home")

    def _set_screen(self, lines):
        shown = [l.ljust(MENU_LINE_LEN)[:MENU_LINE_LEN] for l in lines]
        while len(shown) < 9:
            shown.append(" " * MENU_LINE_LEN)
        self.screen.setText("\n".join(shown))

    def _log(self, line):
        ts = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
        self.log.appendPlainText(f"[{ts}] {line}")
