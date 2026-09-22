#!/usr/bin/env python3
"""
coater_la_purge_emulator.py

Linear Actuator + Purge Unit HIL emulator for the 12/48 Coater (CNT-014).

Emulates the two mechanisms the coating sequence waits on. Both report a plain
BOOLEAN "moving" byte - this coater tests them with `if (!*rpdo4_actuator_moving)`
and `if (!*rpdo7_purge_moving)`, so 0 = stopped and ANY nonzero = moving. That is
deliberately NOT the CNT-31 status bitfield (0x10 = STOPPED), which would read as
"moving" here and hang the sequence.

  Linear actuator
      RX 0x36A  TPDO3, 2 bytes [position * 10 (inches), speed %]   (LA_TYPE == 2, default)
      RX 0x46A  TPDO6, same payload                                (LA_TYPE != 2)
      TX 0x1EA  RPDO4, 1 byte: 1 = moving, 0 = stopped

  Purge unit (ghost cup)
      RX 0x361  1 byte: 0 = stop/clear, 1 = extend, 2 = retract
      TX 0x1E1  RPDO7, 1 byte: 1 = moving, 0 = stopped

Transmit policy: a mechanism that is MOVING repeats its 1 every REFRESH_MS, so
the coater sees a steady "still moving". A mechanism at rest is SILENT - the
stop edge is sent (and repeated a few times, since UDP can drop and that frame
is the one that matters), after which the channel goes quiet. The coater's RPDO
holds the last byte received, so repeating 0 forever would add nothing.

Timing is modelled on what the firmware budgets, so a move always completes
comfortably inside its windows:

  actuator   must report moving within LAMoveTime (1 s) of a command, and must
             stop before LAMovingTime * inches * 100/speed (+3 s headroom).
             0.889 s per inch at 100% speed matches the firmware's own estimate.
  purge      must report moving within 2 s of extend/retract (throwGhost case
             6/17), and stop within 20 s (case 7/18).

Everything succeeds by default. A command that targets the CURRENT position still
performs a short minimum move, so the firmware's "did it start moving?" checks
are always satisfied.

One fault can be injected, from the GUI: "purge never finishes retracting". It
drives State 4 (PurgeRetractWait) into State 110 (PurgeRetractWaitErrorState) by
starting the retract normally and then holding "moving" past the 20 s timeout.

Ports (shared bus): binds :20004, sends to can_udp_hub.py on :20100.
    python coater_la_purge_emulator.py [recv_port] [send_port]
"""

import sys
import os
import datetime

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from PySide6.QtWidgets import (
    QApplication,
    QMainWindow,
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QGroupBox,
    QFormLayout,
    QLabel,
    QPushButton,
    QTextEdit,
    QSpinBox,
    QDoubleSpinBox,
    QProgressBar,
    QCheckBox,
)
from PySide6.QtCore import Qt, QTimer
from PySide6.QtGui import QFont

from can_udp import CANReceiverUdp


###########################################################################
# CAN IDs
###########################################################################

LA_CMD_PRIMARY   = 0x36A   # RX: TPDO3, LA_TYPE == 2 (the default)
LA_CMD_SECONDARY = 0x46A   # RX: TPDO6, LA_TYPE != 2
LA_MOVING_ID     = 0x1EA   # TX: RPDO4, boolean moving

PURGE_CMD_ID     = 0x361   # RX: purge extend/retract/stop
PURGE_MOVING_ID  = 0x1E1   # TX: RPDO7, boolean moving

PURGE_STOP    = 0
PURGE_EXTEND  = 1
PURGE_RETRACT = 2

DEFAULT_RECV_PORT = 20004
DEFAULT_SEND_PORT = 20100

TICK_MS      = 20     # simulation resolution
REFRESH_MS   = 200    # steady re-send of a moving flag, WHILE MOVING only

# At rest the channel goes quiet: the firmware's RPDO holds the last byte we
# sent, so repeating 0 forever adds nothing but bus noise. That makes the
# stop edge the one frame that must not be lost, so it is repeated a few times
# and then the line falls silent.
STOP_REPEATS   = 3
STOP_REPEAT_MS = 40

# Firmware budgets (see module docstring); defaults sit well inside them.
DEFAULT_REACTION_S    = 0.15   # command -> "moving" goes true
DEFAULT_LA_SEC_PER_IN = 0.889  # at 100% speed; = LAMovingTime in seconds
# Deliberately longer than PurgeRetract's 2 s settle window, so a normal run
# goes PurgeRetract -> PurgeRetractWait -> InitLAMove and exercises the wait
# state. Drop it below 2 s to take the firmware's "already retracted" shortcut
# straight to InitLAMove instead.
DEFAULT_PURGE_TRAVEL  = 2.50   # full extend or retract
MIN_TRAVEL_S          = 0.30   # zero-distance command still "moves" this long

# Stuck-retract fault. To land in PurgeRetractWaitErrorState the cup must still
# be MOVING when PurgeRetract checks at 2 s (otherwise the firmware shortcuts to
# InitLAMove and never enters the wait state), and stay moving past
# PurgeRetractWait's 20 s timeout - so the hold has to clear ~22 s.
STUCK_HOLD_S    = 25.0
STUCK_HOLD_MIN  = 22.0


###########################################################################
# Simulation cores (no Qt, no CAN - just state advanced by tick())
###########################################################################

class MechanismSim:
    """Reaction delay, then travel, then stop. Reports a boolean `moving`."""

    def __init__(self):
        self.moving = False
        self._reaction_left = 0.0
        self._travel_left = 0.0
        self._armed = False        # command accepted, reaction not yet elapsed

    def _start(self, reaction_s, travel_s):
        self._reaction_left = max(reaction_s, 0.0)
        self._travel_left = max(travel_s, MIN_TRAVEL_S)
        self._armed = True
        self.moving = False

    def tick(self, dt):
        """Advance the model. Returns the fraction of travel completed this
        tick (0.0..1.0 of the whole move) so subclasses can move a position."""
        if self._armed:
            self._reaction_left -= dt
            if self._reaction_left <= 0.0:
                self._armed = False
                self.moving = True
            return 0.0

        if not self.moving:
            return 0.0

        step = min(dt, self._travel_left)
        frac = step / self._travel_left if self._travel_left > 0 else 1.0
        self._travel_left -= step
        if self._travel_left <= 0.0:
            self._travel_left = 0.0
            self.moving = False
            return 1.0
        return frac

    def stop(self):
        self.moving = False
        self._armed = False
        self._reaction_left = 0.0
        self._travel_left = 0.0


class ActuatorSim(MechanismSim):
    """Linear actuator. Position in tenths of an inch, as the wire carries it."""

    def __init__(self):
        super().__init__()
        self.position_tenths = 0.0
        self.target_tenths = 0.0
        self.speed_pct = 100
        self.sec_per_inch = DEFAULT_LA_SEC_PER_IN
        self.reaction_s = DEFAULT_REACTION_S

    def command(self, target_tenths, speed_pct):
        self.target_tenths = float(target_tenths)
        self.speed_pct = speed_pct if speed_pct > 0 else 100
        inches = abs(self.target_tenths - self.position_tenths) / 10.0
        travel = inches * self.sec_per_inch * (100.0 / self.speed_pct)
        self._start(self.reaction_s, travel)

    def tick(self, dt):
        start = self.position_tenths
        frac = super().tick(dt)
        if frac > 0.0:
            remaining = self.target_tenths - self.position_tenths
            self.position_tenths += remaining * frac if frac < 1.0 else remaining
        if not self.moving and not self._armed:
            self.position_tenths = self.target_tenths
        return self.position_tenths != start


class PurgeSim(MechanismSim):
    """Purge cup (ghost cup): retracted <-> extended."""

    def __init__(self):
        super().__init__()
        self.extended = False
        self._target_extended = False
        self.travel_s = DEFAULT_PURGE_TRAVEL
        self.reaction_s = DEFAULT_REACTION_S

        # Fault injection: when armed, a RETRACT still starts moving (so the
        # firmware enters PurgeRetractWait) but keeps reporting "moving" for
        # stuck_hold_s, long enough to trip the 20 s timeout.
        self.stuck_retract = False
        self.stuck_hold_s = STUCK_HOLD_S
        self.fault_active = False

    def command(self, cmd):
        if cmd == PURGE_STOP:
            self.stop()
            self.fault_active = False
            return
        self._target_extended = (cmd == PURGE_EXTEND)
        if self.stuck_retract and cmd == PURGE_RETRACT:
            self.fault_active = True
            self._start(self.reaction_s, self.stuck_hold_s)
        else:
            self.fault_active = False
            self._start(self.reaction_s, self.travel_s)

    def stop(self):
        super().stop()
        self.fault_active = False

    def tick(self, dt):
        was = self.extended
        frac = super().tick(dt)
        if frac >= 1.0:
            self.extended = self._target_extended
            self.fault_active = False       # hold elapsed; cup settles normally
        return self.extended != was

    @property
    def state_text(self):
        if self.moving:
            if self.fault_active:
                return "RETRACTING - STUCK (fault injected)"
            return "EXTENDING" if self._target_extended else "RETRACTING"
        return "EXTENDED" if self.extended else "RETRACTED"


###########################################################################
# GUI
###########################################################################

class CoaterMechEmulator(QMainWindow):

    def __init__(self, recv_port, send_port):
        super().__init__()
        self.setWindowTitle("12/48 Coater - Linear Actuator + Purge Unit Emulator")
        self.resize(760, 780)

        self.can = None
        self.connected = False
        self.la = ActuatorSim()
        self.purge = PurgeSim()

        # Last flag actually transmitted per channel, so an edge goes out
        # immediately rather than waiting for the heartbeat.
        self._last_la_moving = None
        self._last_purge_moving = None
        # Remaining repeats of a stop edge, and the gap between them.
        self._la_stop_repeats = 0
        self._purge_stop_repeats = 0
        self._la_repeat_countdown = STOP_REPEAT_MS
        self._purge_repeat_countdown = STOP_REPEAT_MS

        central = QWidget()
        self.setCentralWidget(central)
        layout = QVBoxLayout(central)
        layout.addWidget(self._build_connection_group(recv_port, send_port))
        layout.addWidget(self._build_actuator_group())
        layout.addWidget(self._build_purge_group())
        layout.addWidget(self._build_monitor_group())

        self.statusBar().showMessage("Disconnected")

        self.tick_timer = QTimer(self)
        self.tick_timer.timeout.connect(self.on_tick)
        self.tick_timer.start(TICK_MS)

        self.refresh_timer = QTimer(self)
        self.refresh_timer.timeout.connect(self.send_moving_heartbeat)
        self.refresh_timer.start(REFRESH_MS)

    # ------------------------------------------------------------------ UI

    def _build_connection_group(self, recv_port, send_port):
        grp = QGroupBox("Connection")
        row = QHBoxLayout(grp)

        row.addWidget(QLabel("Recv (our port):"))
        self.recv_spin = QSpinBox()
        self.recv_spin.setRange(1, 65535)
        self.recv_spin.setValue(recv_port)
        row.addWidget(self.recv_spin)

        row.addWidget(QLabel("Send (hub):"))
        self.send_spin = QSpinBox()
        self.send_spin.setRange(1, 65535)
        self.send_spin.setValue(send_port)
        row.addWidget(self.send_spin)

        self.connect_btn = QPushButton("Connect")
        self.connect_btn.clicked.connect(self.on_connect)
        row.addWidget(self.connect_btn)

        self.disconnect_btn = QPushButton("Disconnect")
        self.disconnect_btn.clicked.connect(self.on_disconnect)
        self.disconnect_btn.setEnabled(False)
        row.addWidget(self.disconnect_btn)

        self.connection_label = QLabel("Disconnected")
        self.connection_label.setStyleSheet("color:red;font-weight:bold;")
        row.addWidget(self.connection_label)
        row.addStretch()
        return grp

    def _build_actuator_group(self):
        grp = QGroupBox("Linear Actuator   (RX 0x36A / 0x46A   TX 0x1EA)")
        form = QFormLayout(grp)

        self.la_pos_bar = QProgressBar()
        self.la_pos_bar.setRange(0, 255)          # tenths of an inch, one byte
        self.la_pos_bar.setFormat("%v tenths")
        form.addRow("Position:", self.la_pos_bar)

        self.la_pos_label = QLabel("0.0 in")
        form.addRow("", self.la_pos_label)

        self.la_target_label = QLabel("-")
        form.addRow("Commanded:", self.la_target_label)

        self.la_moving_label = QLabel("STOPPED")
        self.la_moving_label.setStyleSheet("color:green;font-weight:bold;")
        form.addRow("Moving (0x1EA):", self.la_moving_label)

        self.la_speed_spin = QDoubleSpinBox()
        self.la_speed_spin.setRange(0.05, 10.0)
        self.la_speed_spin.setSingleStep(0.05)
        self.la_speed_spin.setDecimals(3)
        self.la_speed_spin.setValue(DEFAULT_LA_SEC_PER_IN)
        self.la_speed_spin.setSuffix(" s/in @100%")
        self.la_speed_spin.valueChanged.connect(
            lambda v: setattr(self.la, "sec_per_inch", v))
        form.addRow("Travel rate:", self.la_speed_spin)

        self.la_react_spin = QDoubleSpinBox()
        self.la_react_spin.setRange(0.0, 0.9)     # firmware allows up to 1 s
        self.la_react_spin.setSingleStep(0.05)
        self.la_react_spin.setValue(DEFAULT_REACTION_S)
        self.la_react_spin.setSuffix(" s")
        self.la_react_spin.valueChanged.connect(
            lambda v: setattr(self.la, "reaction_s", v))
        form.addRow("Reaction delay:", self.la_react_spin)
        return grp

    def _build_purge_group(self):
        grp = QGroupBox("Purge Unit / Ghost Cup   (RX 0x361   TX 0x1E1)")
        form = QFormLayout(grp)

        self.purge_state_label = QLabel("RETRACTED")
        self.purge_state_label.setStyleSheet("font-weight:bold;")
        form.addRow("State:", self.purge_state_label)

        self.purge_cmd_label = QLabel("-")
        form.addRow("Last command:", self.purge_cmd_label)

        self.purge_moving_label = QLabel("STOPPED")
        self.purge_moving_label.setStyleSheet("color:green;font-weight:bold;")
        form.addRow("Moving (0x1E1):", self.purge_moving_label)

        self.purge_travel_spin = QDoubleSpinBox()
        self.purge_travel_spin.setRange(0.1, 18.0)   # firmware allows 20 s
        self.purge_travel_spin.setSingleStep(0.25)
        self.purge_travel_spin.setValue(DEFAULT_PURGE_TRAVEL)
        self.purge_travel_spin.setSuffix(" s")
        self.purge_travel_spin.valueChanged.connect(
            lambda v: setattr(self.purge, "travel_s", v))
        form.addRow("Travel time:", self.purge_travel_spin)

        self.purge_react_spin = QDoubleSpinBox()
        self.purge_react_spin.setRange(0.0, 1.8)     # firmware allows 2 s
        self.purge_react_spin.setSingleStep(0.05)
        self.purge_react_spin.setValue(DEFAULT_REACTION_S)
        self.purge_react_spin.setSuffix(" s")
        self.purge_react_spin.valueChanged.connect(
            lambda v: setattr(self.purge, "reaction_s", v))
        form.addRow("Reaction delay:", self.purge_react_spin)

        self.purge_stuck_chk = QCheckBox(
            "FAULT: purge never finishes retracting  →  PurgeRetractWaitErrorState")
        self.purge_stuck_chk.setStyleSheet("color:#b00;font-weight:bold;")
        self.purge_stuck_chk.setToolTip(
            "Simulates a jammed cup. On a RETRACT the purge still starts moving "
            "(so the firmware enters PurgeRetractWait rather than shortcutting to "
            "InitLAMove) but keeps reporting 'moving' past the 20 s timeout, "
            "driving State 4 -> 110 (PurgeRetractWaitErrorState): "
            "\"Warn:PURGE TIMEOUT\", head and pump off, back to FinishState.\n\n"
            "Stays armed until unchecked. Affects EXTEND commands not at all.")
        self.purge_stuck_chk.toggled.connect(self.on_stuck_toggled)
        form.addRow(self.purge_stuck_chk)

        self.purge_stuck_spin = QDoubleSpinBox()
        self.purge_stuck_spin.setRange(STUCK_HOLD_MIN, 90.0)
        self.purge_stuck_spin.setSingleStep(1.0)
        self.purge_stuck_spin.setValue(STUCK_HOLD_S)
        self.purge_stuck_spin.setSuffix(" s")
        self.purge_stuck_spin.setToolTip(
            f"How long the stuck cup keeps reporting 'moving'. Must clear ~{STUCK_HOLD_MIN:.0f} s "
            "(PurgeRetract's 2 s settle + PurgeRetractWait's 20 s timeout) for the "
            "error state to fire; the minimum here enforces that.")
        self.purge_stuck_spin.setEnabled(False)
        self.purge_stuck_spin.valueChanged.connect(
            lambda v: setattr(self.purge, "stuck_hold_s", v))
        form.addRow("   stuck-moving hold:", self.purge_stuck_spin)
        return grp

    def on_stuck_toggled(self, on):
        self.purge.stuck_retract = on
        self.purge_stuck_spin.setEnabled(on)
        self.log_line(
            f"FAULT {'ARMED' if on else 'cleared'}: purge stuck retracting"
            + (f" ({self.purge_stuck_spin.value():.0f} s hold)" if on else ""))

    def _build_monitor_group(self):
        grp = QGroupBox("CAN Monitor")
        col = QVBoxLayout(grp)

        row = QHBoxLayout()
        self.log_status_chk = QCheckBox("Log periodic status frames")
        self.log_status_chk.setChecked(False)
        self.log_status_chk.setToolTip(
            "Off by default: the 0x1EA/0x1E1 refresh would otherwise flood the log."
        )
        row.addWidget(self.log_status_chk)
        clear_btn = QPushButton("Clear")
        clear_btn.clicked.connect(lambda: self.log.clear())
        row.addWidget(clear_btn)
        row.addStretch()
        col.addLayout(row)

        self.log = QTextEdit()
        self.log.setReadOnly(True)
        self.log.setFont(QFont("Courier New", 9))
        col.addWidget(self.log)
        return grp

    # ------------------------------------------------------------ plumbing

    def log_line(self, text):
        ts = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
        self.log.append(f"[{ts}] {text}")

    def log_can(self, direction, can_id, data):
        hexed = " ".join(f"{b:02X}" for b in data)
        self.log_line(f"{direction} 0x{can_id:03X} [{len(data)}] {hexed}")

    def on_connect(self):
        if self.connected:
            return
        self.can = CANReceiverUdp(recv_port=self.recv_spin.value(),
                                  send_port=self.send_spin.value())
        self.can.message_received_signal.connect(self.on_rx)
        self.can.can_status_signal.connect(
            lambda m: self.statusBar().showMessage(m))
        self.can.can_error_signal.connect(
            lambda m: self.statusBar().showMessage(m))
        self.can.start()

        self.connected = True
        self.recv_spin.setEnabled(False)
        self.send_spin.setEnabled(False)
        self.connect_btn.setEnabled(False)
        self.disconnect_btn.setEnabled(True)
        self.connection_label.setText("Connected")
        self.connection_label.setStyleSheet("color:green;font-weight:bold;")
        self.log_line(f"connected: recv :{self.recv_spin.value()} "
                      f"send :{self.send_spin.value()}")

        # Announce "stopped" once on connect, then stay quiet until something
        # moves - a coater that is already running needs a defined value.
        self.announce_stopped()

    def on_disconnect(self):
        if not self.connected:
            return
        self.la.stop()
        self.purge.stop()
        if self.can:
            self.can.stop()
            self.can = None
        self.connected = False
        self.recv_spin.setEnabled(True)
        self.send_spin.setEnabled(True)
        self.connect_btn.setEnabled(True)
        self.disconnect_btn.setEnabled(False)
        self.connection_label.setText("Disconnected")
        self.connection_label.setStyleSheet("color:red;font-weight:bold;")
        self.log_line("disconnected")

    def send_frame(self, can_id, data, periodic=False):
        if not self.connected:
            return
        self.can.send_can_message(can_id, list(data))
        if not periodic or self.log_status_chk.isChecked():
            self.log_can("TX", can_id, list(data))

    def send_moving_heartbeat(self):
        """Steady re-send, but ONLY for a mechanism that is actually moving.
        A stopped mechanism stays silent - its last transmitted 0 persists in
        the coater's process image."""
        if self.la.moving:
            self.send_frame(LA_MOVING_ID, [1], periodic=True)
        if self.purge.moving:
            self.send_frame(PURGE_MOVING_ID, [1], periodic=True)

    def announce_stopped(self):
        """One explicit 0 per channel, so a coater that is already running has a
        defined value even though we are about to go quiet."""
        self.send_frame(LA_MOVING_ID, [0])
        self.send_frame(PURGE_MOVING_ID, [0])
        self._last_la_moving = self.la.moving
        self._last_purge_moving = self.purge.moving
        self._la_stop_repeats = 0
        self._purge_stop_repeats = 0

    # ------------------------------------------------------------- receive

    def on_rx(self, msg):
        can_id = msg["id"]
        data = list(msg["data"])

        if can_id in (LA_CMD_PRIMARY, LA_CMD_SECONDARY):
            self.log_can("RX", can_id, data)
            target = data[0] if len(data) > 0 else 0
            speed = data[1] if len(data) > 1 else 100
            self.la.command(target, speed)
            self.la_target_label.setText(
                f"{target / 10.0:.1f} in @ {speed}%"
                + ("  (0x46A)" if can_id == LA_CMD_SECONDARY else ""))
            self.log_line(f"    LA move -> {target / 10.0:.1f} in at {speed}%")
            return

        if can_id == PURGE_CMD_ID:
            self.log_can("RX", can_id, data)
            cmd = data[0] if len(data) > 0 else 0
            name = {PURGE_STOP: "STOP", PURGE_EXTEND: "EXTEND",
                    PURGE_RETRACT: "RETRACT"}.get(cmd, f"UNKNOWN({cmd})")
            self.purge.command(cmd)
            self.purge_cmd_label.setText(name)
            self.log_line(f"    purge {name}")
            return

    # ---------------------------------------------------------------- tick

    def on_tick(self):
        dt = TICK_MS / 1000.0
        self.la.tick(dt)
        self.purge.tick(dt)

        self._tick_channel(LA_MOVING_ID, self.la.moving, "la")
        self._tick_channel(PURGE_MOVING_ID, self.purge.moving, "purge")

        self._refresh_ui()

    def _tick_channel(self, can_id, moving, tag):
        """Edge -> send now (the firmware's reaction windows are short and must
        not wait for the heartbeat). A stop edge is repeated a few times, then
        the channel goes silent until it moves again."""
        last = getattr(self, f"_last_{tag}_moving")
        repeats = getattr(self, f"_{tag}_stop_repeats")
        countdown = getattr(self, f"_{tag}_repeat_countdown")

        if moving != last:
            self.send_frame(can_id, [1 if moving else 0])
            setattr(self, f"_last_{tag}_moving", moving)
            # Going quiet after this, so make the 0 robust to a dropped datagram.
            setattr(self, f"_{tag}_stop_repeats", 0 if moving else STOP_REPEATS)
            setattr(self, f"_{tag}_repeat_countdown", STOP_REPEAT_MS)
            return

        if repeats > 0:
            countdown -= TICK_MS
            if countdown <= 0:
                self.send_frame(can_id, [0])
                setattr(self, f"_{tag}_stop_repeats", repeats - 1)
                countdown = STOP_REPEAT_MS
            setattr(self, f"_{tag}_repeat_countdown", countdown)

    def _refresh_ui(self):
        pos = int(round(self.la.position_tenths))
        self.la_pos_bar.setValue(max(0, min(255, pos)))
        self.la_pos_label.setText(f"{self.la.position_tenths / 10.0:.2f} in")
        moving = self.la.moving
        self.la_moving_label.setText("MOVING" if moving else "STOPPED")
        self.la_moving_label.setStyleSheet(
            "color:orange;font-weight:bold;" if moving
            else "color:green;font-weight:bold;")

        self.purge_state_label.setText(self.purge.state_text)
        pmoving = self.purge.moving
        self.purge_moving_label.setText("MOVING" if pmoving else "STOPPED")
        self.purge_moving_label.setStyleSheet(
            "color:orange;font-weight:bold;" if pmoving
            else "color:green;font-weight:bold;")

    def closeEvent(self, event):
        self.on_disconnect()
        super().closeEvent(event)


def main():
    recv_port = int(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_RECV_PORT
    send_port = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_SEND_PORT
    app = QApplication(sys.argv)
    win = CoaterMechEmulator(recv_port, send_port)
    win.show()
    win.on_connect()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
