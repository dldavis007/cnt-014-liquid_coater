#!/usr/bin/env python3
"""
button_box_panel.py

Reusable operator Button Box panel (a QGroupBox) that any emulator can embed.
It is transport-agnostic: the host passes a `send_fn(can_id, data)` callable
(wired to whatever CAN transport + logging the host already has - PCAN or the
UDP mirror), and this panel just calls it. Embed it in la_cnt31_emulator.py,
speed_control_emulator.py, etc.

Frames it emits (FBE coater map; see SourceFiles/user.c + menu_system.c):

  0x180  RPDO2 - 3-byte button-box word, little-endian in bytes 0-1
                 (TC0_RCVD_Data = OUT_digi_2<<8 | OUT_digi_1). This word is the
                 shared physical panel; different nodes react to different bits:
                   0x8000 REV   0x4000 FWD   0x2000 ExSlow  0x1000 Slow
                   0x0800 Fast  0x0400 KeepAlive  0x0004 Select
                   0x0002 Up    0x0001 Down
  <menu_trig_id>  RPDO1 (default 0x26E = 0x200+NODE_ID for the coater), 1 byte
                 -> gProcImg[OUT_digi_0]:  bit0 (0x01) ACTIVATE MENU,
                                           bit1 (0x02) COATING TRIGGER
                 Pass menu_trig_id=None to hide the menu/trigger buttons.
                 ACTIVATE MENU goes to menu_trig_id (which follows the unit you
                 are browsing); the COATING TRIGGER goes to coat_trigger_id,
                 which is FIXED on its own machine.

Direction/speed are latched (checkable) and folded into the word; Up/Down/
Select and the menu/trigger bits are momentary (pulsed on, cleared after
PULSE_MS). An optional keepalive repeat re-sends the current word periodically.
"""

from PySide6.QtWidgets import (
    QGroupBox, QVBoxLayout, QHBoxLayout, QLabel, QPushButton, QCheckBox, QFrame,
)
from PySide6.QtCore import QTimer

# ── 0x180 button-box word bits ──────────────────────────────────────────────
BUTTON_BOX_ID = 0x180
BB_REV       = 0x8000
BB_FWD       = 0x4000
BB_EXSLOW    = 0x2000
BB_SLOW      = 0x1000
BB_FAST      = 0x0800
BB_KEEPALIVE = 0x0400
BB_SELECT    = 0x0004
BB_UP        = 0x0002
BB_DOWN      = 0x0001

# ── menu/trigger byte bits (default id 0x26E) ───────────────────────────────
MENU_ACTIVATE = 0x01
COAT_TRIGGER  = 0x02

PULSE_MS     = 250   # momentary press hold time before auto-release
HEARTBEAT_MS = 100   # the box sends its CURRENT word this often (default cadence)


class ButtonBoxPanel(QGroupBox):

    def __init__(self, send_fn, menu_trig_id=0x26E, title="Button Box", parent=None,
                 show_menu_buttons=None, coat_trigger_id=None):
        """send_fn(can_id:int, data:list[int]) -> None  (host wires TX + logging).

        menu_trig_id=None means "no target yet" — the buttons are hidden by
        default, which is the historical behaviour. Pass show_menu_buttons=True
        to keep them visible anyway: a host that discovers its units by bus scan
        has no target until the scan finishes, but still wants the buttons on
        screen. They simply do nothing until set_menu_trig_id() supplies one.
        """
        super().__init__(title, parent)
        self._send = send_fn
        self._menu_trig_id = menu_trig_id
        # ACTIVATE MENU follows whichever unit you are browsing; the COATING
        # TRIGGER does NOT — it belongs to one machine (the FBE coater) and must
        # keep going there no matter whose menu is open. The real box behaves the
        # same way: Trigger_Device() only ever indexes its FRONT menu, so an AUX
        # unit can never become a trigger target. Defaults to the menu target so
        # existing embedders (la_cnt31_emulator, ...) are unaffected.
        self._coat_trigger_id = (menu_trig_id if coat_trigger_id is None
                                 else coat_trigger_id)
        self._show_menu_buttons = (menu_trig_id is not None
                                   if show_menu_buttons is None
                                   else show_menu_buttons)
        self._momentary = 0          # momentary bits currently pulsed into the word

        # The button box continuously sends its CURRENT word every HEARTBEAT_MS.
        # The word always includes BB_KEEPALIVE, so with no direction/speed
        # selected it is 0x0400 = the neutral "stop" command; FWD/REV/speed just
        # OR their bits into the same heartbeat.
        self._hb_timer = QTimer(self)
        self._hb_timer.setInterval(HEARTBEAT_MS)
        self._hb_timer.timeout.connect(self._send_word)

        lay = QVBoxLayout(self)

        # Direction (latched, mutually exclusive)
        lay.addWidget(QLabel("Direction:"))
        drow = QHBoxLayout()
        self._fwd = QPushButton("FWD"); self._fwd.setCheckable(True)
        self._rev = QPushButton("REV"); self._rev.setCheckable(True)
        self._fwd.clicked.connect(lambda: self._exclusive(self._fwd, [self._rev]))
        self._rev.clicked.connect(lambda: self._exclusive(self._rev, [self._fwd]))
        drow.addWidget(self._fwd); drow.addWidget(self._rev)
        lay.addLayout(drow)

        # Speed (latched, mutually exclusive)
        lay.addWidget(QLabel("Speed:"))
        srow = QHBoxLayout()
        self._exs = QPushButton("ExSlow"); self._exs.setCheckable(True)
        self._slw = QPushButton("Slow");   self._slw.setCheckable(True)
        self._fst = QPushButton("Fast");   self._fst.setCheckable(True)
        self._exs.clicked.connect(lambda: self._exclusive(self._exs, [self._slw, self._fst]))
        self._slw.clicked.connect(lambda: self._exclusive(self._slw, [self._exs, self._fst]))
        self._fst.clicked.connect(lambda: self._exclusive(self._fst, [self._exs, self._slw]))
        for b in (self._exs, self._slw, self._fst):
            srow.addWidget(b)
        lay.addLayout(srow)

        lay.addWidget(self._hline())

        # Menu navigation (momentary)
        lay.addWidget(QLabel("Menu nav (momentary):"))
        nrow = QHBoxLayout()
        b_up = QPushButton("Up");     b_dn = QPushButton("Down"); b_sel = QPushButton("Select")
        b_up.clicked.connect(lambda: self._pulse_word(BB_UP))
        b_dn.clicked.connect(lambda: self._pulse_word(BB_DOWN))
        b_sel.clicked.connect(lambda: self._pulse_word(BB_SELECT))
        for b in (b_up, b_dn, b_sel):
            nrow.addWidget(b)
        lay.addLayout(nrow)

        # Menu activate / coating trigger (momentary, on the node's 0x200+ID)
        if self._show_menu_buttons:
            lay.addWidget(self._hline())
            mrow = QHBoxLayout()
            b_menu = QPushButton("Activate Menu")
            b_trig = QPushButton("Coating Trigger")
            b_trig.setStyleSheet("background:#2e7d32;color:white;font-weight:bold;")
            b_menu.clicked.connect(self.activate_menu)
            b_trig.clicked.connect(self.coating_trigger)
            self._trig_btn = b_trig
            mrow.addWidget(b_menu); mrow.addWidget(b_trig)
            lay.addLayout(mrow)

        lay.addWidget(self._hline())

        # STOP: clear all direction/speed so the heartbeat reverts to the neutral
        # stop word (0x0400 -> "00 04 00").
        self._stop_btn = QPushButton("STOP")
        self._stop_btn.setStyleSheet("background:#c62828;color:white;font-weight:bold;padding:8px;")
        self._stop_btn.clicked.connect(self._stop)
        lay.addWidget(self._stop_btn)

        # Word readout + controls
        self._word_lbl = QLabel(); self._word_lbl.setStyleSheet("font-family:Courier;")
        lay.addWidget(self._word_lbl)
        crow = QHBoxLayout()
        self._heartbeat = QCheckBox("Send every 100 ms")
        self._heartbeat.setChecked(True)
        self._heartbeat.stateChanged.connect(self._on_heartbeat)
        crow.addWidget(self._heartbeat)
        b_send = QPushButton("Send Once")
        b_send.clicked.connect(self._send_word)
        crow.addWidget(b_send)
        lay.addLayout(crow)

        lay.addStretch()
        self._refresh_word_label()
        self._hb_timer.start()   # heartbeat on by default (send_fn no-ops until connected)

    # ── word assembly ───────────────────────────────────────────────────────
    def _build_word(self):
        word = BB_KEEPALIVE
        if self._fwd.isChecked(): word |= BB_FWD
        if self._rev.isChecked(): word |= BB_REV
        if self._exs.isChecked(): word |= BB_EXSLOW
        if self._slw.isChecked(): word |= BB_SLOW
        if self._fst.isChecked(): word |= BB_FAST
        word |= self._momentary
        return word

    def _refresh_word_label(self):
        self._word_lbl.setText(f"0x180 word = 0x{self._build_word():04X}")

    def _send_word(self):
        word = self._build_word()
        self._send(BUTTON_BOX_ID, [word & 0xFF, (word >> 8) & 0xFF, 0x00])
        self._refresh_word_label()

    # ── button handlers ──────────────────────────────────────────────────────
    def _exclusive(self, btn, others):
        if btn.isChecked():
            for o in others:
                o.setChecked(False)
        self._send_word()

    def _pulse_word(self, bit):
        self._momentary |= bit
        self._send_word()
        QTimer.singleShot(PULSE_MS, lambda: self._clear_momentary(bit))

    def _clear_momentary(self, bit):
        self._momentary &= ~bit
        self._send_word()

    def set_menu_trig_id(self, can_id):
        """Re-point the MENU / TRIGGER byte at a different node.

        Each node has its own RPDO1 at $NODEID+0x200 (FBE coater 0x26E,
        inspection body 0x269), so this is what selects WHOSE menu the Activate
        Menu button opens. The 0x180 button-box word is deliberately unaffected:
        it is one shared physical panel, and Up/Down/Select go to whichever node
        currently has its menu open.
        """
        self._menu_trig_id = can_id

    def menu_trig_id(self):
        return self._menu_trig_id

    def set_coat_trigger_id(self, can_id):
        """Point the COATING TRIGGER at its machine, independently of the menu.

        Browsing another unit's menu must not re-aim the trigger — starting a
        coating cycle on the wrong node (or on one that reads that bit as
        something else entirely) is not a mistake worth allowing."""
        self._coat_trigger_id = can_id
        btn = getattr(self, "_trig_btn", None)
        if btn is not None:
            btn.setEnabled(can_id is not None)
            btn.setToolTip("" if can_id is None else
                           f"Coating trigger -> 0x{can_id:03X} (fixed)")

    def coat_trigger_id(self):
        return self._coat_trigger_id

    def activate_menu(self):
        """Pulse ACTIVATE MENU at the currently selected unit."""
        self._pulse_to(self._menu_trig_id, MENU_ACTIVATE)

    def coating_trigger(self):
        """Pulse the COATING TRIGGER at ITS machine — never the menu target."""
        self._pulse_to(self._coat_trigger_id, COAT_TRIGGER)

    def _pulse_to(self, target, bit):
        if target is None:
            return
        self._send(target, [bit])
        QTimer.singleShot(PULSE_MS, lambda: self._send(target, [0x00]))

    def _on_heartbeat(self, on):
        if on:
            self._hb_timer.start()
        else:
            self._hb_timer.stop()

    def _stop(self):
        # Clear direction/speed/momentary -> the word reverts to 0x0400 (stop).
        for b in (self._fwd, self._rev, self._exs, self._slw, self._fst):
            b.setChecked(False)
        self._momentary = 0
        self._send_word()

    @staticmethod
    def _hline():
        f = QFrame(); f.setFrameShape(QFrame.HLine); f.setFrameShadow(QFrame.Sunken)
        return f
