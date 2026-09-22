#!/usr/bin/env python3
"""
button_box_emulator.py

Standalone operator Button Box emulator for the FBE coater (CNT-23) over the UDP
CAN mirror. Wraps the reusable ButtonBoxPanel with a CANReceiverUdp transport so
you can fire the COATING TRIGGER (and the menu / direction / speed buttons)
independently of the LA emulator - handy now that the LA emulator lives on the
CNT-31 ports and no longer drives the coater.

Ports (shared bus): binds recv_port (default 20001, our unique port) and sends to
send_port (default 20100, the can_udp_hub.py bus). For direct 2-node mode instead,
set send to the coater host's recv port. Set ports in the top bar before
connecting, or override at launch:
    python button_box_emulator.py [send_port] [recv_port]

Wire frames (see button_box_panel.py / SourceFiles/user.c):
  0x26E  1 byte -> gProcImg[OUT_digi_0] : 0x02 = COATING TRIGGER (momentary),
                                          0x01 = ACTIVATE MENU (momentary)
  0x180  3 byte -> button-box word (direction / speed / select), heartbeat @100ms
  0x310  RX     <- WIM display stream (menu screen + "Proc:" status strings),
                   decoded and drawn by MenuDisplayPanel

Together those give a full operator console: press MENU, walk the screen with
Up/Down/Select, and watch the menu variables (actuator extend/retract speeds, LA
extend/retract times, MaxLADist, coating length, ...) change on the display —
against the firmware running on the PC, with no hardware attached.
"""
import sys
import datetime

from PySide6.QtCore import QTimer
from PySide6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton,
    QSpinBox, QPlainTextEdit,
)

from can_udp import CANReceiverUdp
from button_box_panel import ButtonBoxPanel, BUTTON_BOX_ID
from menu_display_panel import MenuDisplayPanel, WIM_ID
from node_scanner import NodeScanner
from unit_menu_panel import UnitMenuPanel

DEFAULT_SEND_PORT = 20100   # the can_udp_hub.py shared bus (we send here)
DEFAULT_RECV_PORT = 20001   # our unique bind port on the bus

# There is no hard-coded unit list: the box DISCOVERS what is on the bus by SDO
# scan (node_scanner.py), exactly as the real one does at power-up, and puts the
# names it gets back into its own top-level menu (unit_menu_panel.py). Selecting
# one opens that unit's menu, which is its RPDO1 at $NODEID+0x200.
#
# The 0x180 button-box word and the 0x310 display are SHARED by every node — one
# physical panel, one display — so switching units only changes which node the
# MENU / TRIGGER byte is addressed to.
MENU_TRIG_UNSET = None

# The COATING TRIGGER belongs to the FBE coater and must keep going there
# whichever unit's menu is open — it is not a per-unit control. The coater is
# identified from the SCAN rather than hard-coded: it is a front-menu unit
# (name marker 0x32, not 0xFF/AUX) whose name contains this. Falls back to the
# first front-menu unit, and to COAT_TRIGGER_FALLBACK_ID if a scan never ran.
COATER_NAME_HINT        = "COAT"
COAT_TRIGGER_FALLBACK_ID = 0x26E        # CNT-23 NODE_ID 0x6E + 0x200

# Unit menus are MUTUALLY EXCLUSIVE — one display, one button word, so exactly
# one unit menu can be up at a time. Each unit announces its own menu state on
# this flat id (Send_Menu_Status(): CNT-23 Subroutines.c, CNT-31
# MenuFunctions.c), 1 byte, 1 = menu opened / 0 = menu closed. Closure comes out
# of DeSelect() at StackPointer == 0, i.e. selecting EXIT on the top page.
#
# The frame carries NO node id — it is flat 0x200, matching the real box's
# RPDO1 (MCO_InitRPDO(1, 0x200, 1, OUT_digi_0) in user.c). The box knows who it
# came from because it is the one that asked: whichever unit it just pulsed
# ACTIVATE MENU at. We track that the same way, via _pending_unit.
MENU_STATUS_ID  = 0x200
MENU_STATUS_LEN = 1

# The real box scans at power-up; here the transport has to exist first,
# so the scan fires shortly after Connect.
AUTO_SCAN_DELAY_MS = 800


class ButtonBoxEmulator(QWidget):

    def __init__(self, send_port, recv_port):
        super().__init__()
        self.setWindowTitle("FBE Coater - Button Box Emulator")
        self._udp = None
        self._last_word = None          # last-logged 0x180 word (skip repeats)

        root = QVBoxLayout(self)

        # The TX log is CREATED first so anything that logs while the rest of
        # the UI is being built has somewhere to go — _on_target_changed() below
        # does exactly that. It is added to the layout last, so it still appears
        # at the bottom.
        self.log = QPlainTextEdit()
        self.log.setReadOnly(True)
        self.log.setMaximumBlockCount(500)

        # ── connection bar (ports editable until connected) ──────────────
        bar = QHBoxLayout()
        bar.addWidget(QLabel("Send port:"))
        self.send_spin = QSpinBox()
        self.send_spin.setRange(1, 65535)
        self.send_spin.setValue(send_port)
        bar.addWidget(self.send_spin)
        bar.addWidget(QLabel("Recv port:"))
        self.recv_spin = QSpinBox()
        self.recv_spin.setRange(1, 65535)
        self.recv_spin.setValue(recv_port)
        bar.addWidget(self.recv_spin)
        self.conn_btn = QPushButton("Connect")
        self.conn_btn.clicked.connect(self._toggle_conn)
        bar.addWidget(self.conn_btn)
        self.status_lbl = QLabel("disconnected")
        bar.addWidget(self.status_lbl, 1)
        root.addLayout(bar)

        # ── the reusable button box (trigger / menu / dir / speed) ───────
        # ── the box's OWN top-level menu: the units found by the scan ────
        self.units_panel = UnitMenuPanel()
        self.units_panel.unit_chosen.connect(self._open_unit)
        self.units_panel.rescan.connect(self._start_scan)
        root.addWidget(self.units_panel)

        # No target until the scan names one; the buttons stay on screen and
        # simply do nothing until a unit is selected above.
        self.panel = ButtonBoxPanel(
            self._send, menu_trig_id=MENU_TRIG_UNSET, show_menu_buttons=True,
            coat_trigger_id=COAT_TRIGGER_FALLBACK_ID,
            title="Button Box  (menu/trigger -> selected unit, 0x180 word shared)")
        root.addWidget(self.panel)

        # ── the WIM display the button box is driving (0x310 in) ─────────
        self.display = MenuDisplayPanel()
        root.addWidget(self.display, 1)

        # Bus scanner: discovers which units are actually present.
        self._scanner = NodeScanner(self._send, self)
        self._scanner.progress.connect(lambda m: self._log(f"[scan] {m}"))
        self._scanner.finished.connect(self._on_scan_done)

        # _pending_unit: asked to open, not yet confirmed on 0x200.
        # _active_unit : confirmed open by the unit itself. Only one, ever.
        self._pending_unit = None
        self._active_unit = None
        self._display_title = self.display.title()

        # ── TX log (constructed at the top; placed here) ──────────────────
        root.addWidget(self.log, 1)

    # ── transport connect / disconnect ───────────────────────────────────
    def _toggle_conn(self):
        if self._udp is None:
            try:
                self._udp = CANReceiverUdp(recv_port=self.recv_spin.value(),
                                           send_port=self.send_spin.value())
            except OSError as e:
                self._log(f"[error] {e}")
                self._udp = None
                return
            self._udp.can_status_signal.connect(self.status_lbl.setText)
            self._udp.can_error_signal.connect(lambda s: self._log(f"[error] {s}"))
            self._udp.message_received_signal.connect(self._on_rx)
            self._udp.start()
            self.send_spin.setEnabled(False)
            self.recv_spin.setEnabled(False)
            self.conn_btn.setText("Disconnect")
            self._log(f"[conn] send:{self.send_spin.value()} "
                      f"recv:{self.recv_spin.value()}")
            # Scan as soon as the transport is up — the real box scans at
            # power-up. Delayed slightly so the RX thread and any node that is
            # still booting are ready to answer.
            QTimer.singleShot(AUTO_SCAN_DELAY_MS, self._start_scan)
        else:
            self._udp.stop()
            self._udp = None
            self.send_spin.setEnabled(True)
            self.recv_spin.setEnabled(True)
            self.conn_btn.setText("Connect")
            self.status_lbl.setText("disconnected")
            # Off the bus we can no longer hear 0x200, so any menu-open state we
            # are holding is stale — drop it rather than stay locked forever.
            self._pending_unit = None
            self._active_unit = None
            self.units_panel.set_active_unit(None)
            self.display.setTitle(self._display_title)
            self._log("[conn] disconnected")

    # ── send_fn handed to the panel: TX + log ────────────────────────────
    def _send(self, can_id, data):
        """The panel calls this for every frame. No-ops until connected; the
        repetitive 0x180 heartbeat is logged only when the word changes."""
        if self._udp is None:
            return
        self._udp.send_can_message(can_id, data)
        if can_id == BUTTON_BOX_ID:
            word = data[0] | (data[1] << 8)
            if word == self._last_word:
                return
            self._last_word = word
        self._log(f"TX 0x{can_id:03X} [{' '.join(f'{b:02X}' for b in data)}]")

    # ── bus scan ─────────────────────────────────────────────────────────
    def _start_scan(self):
        if self._udp is None:
            self._log("[scan] not connected - connect first")
            return
        self.units_panel.scan_btn.setEnabled(False)
        self._scanner.start()

    def _on_scan_done(self, units):
        """Put the discovered names into the box's own menu."""
        self.units_panel.scan_btn.setEnabled(True)
        self.units_panel.set_units(units)
        if not units:
            self._log("[scan] no units answered")
        self._pin_coating_trigger(units)

    def _pin_coating_trigger(self, units):
        """Aim the COATING TRIGGER at the coater and leave it there.

        Deduced from the scan, not hard-coded: the coater is a FRONT-menu unit
        (marker 0x32 — AUX units are never trigger targets, matching
        Trigger_Device() in the real box, which only indexes its front menu)."""
        front = [u for u in units if not u["aux"]]
        pick = next((u for u in front
                     if COATER_NAME_HINT in u["name"].upper()), None)
        if pick is None and front:
            pick = front[0]

        if pick is None:
            self.panel.set_coat_trigger_id(COAT_TRIGGER_FALLBACK_ID)
            self._log(f"[trig] no front-menu unit found; coating trigger left "
                      f"at 0x{COAT_TRIGGER_FALLBACK_ID:03X}")
        else:
            self.panel.set_coat_trigger_id(pick["menu_id"])
            self._log(f"[trig] coating trigger pinned to {pick['name']} "
                      f"(node 0x{pick['node']:02X}, 0x{pick['menu_id']:03X})")

    # ── open a unit's menu ───────────────────────────────────────────────
    def _open_unit(self, unit):
        """Point MENU / TRIGGER at this unit and open its menu.

        Only that byte moves: the 0x180 word and the 0x310 display are shared,
        so Up/Down/Select automatically reach whichever unit has its menu open
        and the display panel shows it with no extra wiring."""
        if self._active_unit is not None:
            # One menu at a time. The other unit has to close its own menu —
            # we cannot close it for it, exactly as on the real box.
            self._log(f"[menu] {self._active_unit['name']} still has its menu "
                      f"open - close it (EXIT) before opening "
                      f"{unit['name']}")
            return
        self._pending_unit = unit
        # Menu target only. The coating trigger stays pinned to its own machine
        # (see _pin_coating_trigger) — browsing a unit must never re-aim it.
        self.panel.set_menu_trig_id(unit["menu_id"])
        self.display.setTitle(
            f"WIM Display  (0x310)   -  {unit['name']}  "
            f"(node 0x{unit['node']:02X})")
        self._log(f"[menu] opening {unit['name']} "
                  f"(node 0x{unit['node']:02X}, menu 0x{unit['menu_id']:03X})")
        # Pulse ACTIVATE MENU at the newly selected unit. Nothing is considered
        # open until that unit says so on 0x200.
        self.panel.activate_menu()

    def _on_menu_status(self, stat):
        """A unit reported its menu state on 0x200 (1 = opened, 0 = closed).

        Units repeat the 'opened' byte, so only transitions are acted on."""
        if stat:
            if self._active_unit is not None:
                return                              # already open, repeat byte
            unit = self._pending_unit
            if unit is None:
                # A menu opened that we did not ask for (someone at the real
                # panel, or a unit left open). Lock anyway — the display and
                # button word are now that unit's — but we cannot name it.
                self._log("[menu] a unit menu opened (0x200=01) with no "
                          "pending request - list locked until it closes")
                self.units_panel.lock_lbl.setText(
                    "A unit menu is open. Close it (EXIT) to choose another.")
                self.units_panel.open_btn.setEnabled(False)
                return
            self._active_unit = unit
            self.units_panel.set_active_unit(unit)
            self._log(f"[menu] {unit['name']} confirmed menu ACTIVE (0x200=01)")
        else:
            was = self._active_unit
            self._active_unit = None
            self._pending_unit = None
            self.units_panel.set_active_unit(None)
            self.display.setTitle(self._display_title)
            if was is not None:
                self._log(f"[menu] {was['name']} closed its menu (0x200=00) - "
                          f"pick another unit")
            else:
                self._log("[menu] menu closed (0x200=00)")

    # ── RX: hand display frames to the panel ─────────────────────────────
    def _on_rx(self, msg):
        """CANReceiverUdp emits {'id': int, 'data': [int, ...]} per frame.
        0x310 is the display; 0x200 is a unit's menu open/closed byte.
        The display stream is not logged here — it is many small frames per
        redraw and would bury the TX log; the panel keeps its own decoded log."""
        can_id, data = msg.get("id"), msg.get("data", [])
        if can_id == WIM_ID:
            self.display.feed(can_id, data)
        elif can_id == MENU_STATUS_ID and len(data) == MENU_STATUS_LEN:
            self._on_menu_status(data[0])
        else:
            # SDO replies during a scan; harmless at any other time.
            self._scanner.feed(can_id, data)

    def _log(self, line):
        """Never raise. Logging is called from construction, from Qt signals and
        from the RX path, so a missing widget must not take the app down with
        it — fall back to stdout instead."""
        ts = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
        widget = getattr(self, "log", None)
        if widget is None:
            # Qt handles any text; a cp1252 console does not, and unit names
            # come off the bus as arbitrary bytes. Never let logging raise.
            try:
                print(f"[{ts}] {line}")
            except UnicodeEncodeError:
                print(f"[{ts}] {line.encode('ascii', 'replace').decode('ascii')}")
            return
        widget.appendPlainText(f"[{ts}] {line}")

    def closeEvent(self, ev):
        if self._udp is not None:
            self._udp.stop()
        super().closeEvent(ev)


def main():
    send_port = int(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_SEND_PORT
    recv_port = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_RECV_PORT
    app = QApplication(sys.argv)
    w = ButtonBoxEmulator(send_port, recv_port)
    w.resize(560, 520)
    w.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
