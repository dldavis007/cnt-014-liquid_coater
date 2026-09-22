#!/usr/bin/env python3
"""
can_hub_gui.py

GUI version of can_udp_hub.py: a software CAN-bus hub for the UDP mirror transport
PLUS a live monitor. It does everything the headless hub does - binds one bus port,
learns each node from the source of its datagrams, and re-broadcasts every frame to
all OTHER nodes (never back to the sender), with NO silence pruning - and adds:

  * a live PER-ID summary table (ID, count, last data, source, last-seen) so you can
    instantly see whether a given ID (e.g. 0x469) is actually on the bus,
  * a scrolling frame log,
  * an include-filter: enter a list of CAN IDs to show ONLY those in the log.

Run it INSTEAD of can_udp_hub.py; point every node's send at this hub's port.
Datagram format (unchanged): [ID lo, ID hi, LEN, LEN data bytes].
"""
import sys
import socket
import time
import datetime

from PySide6.QtCore import QThread, Signal, Qt, QTimer
from PySide6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QLabel,
    QPushButton, QSpinBox, QLineEdit, QCheckBox, QPlainTextEdit, QTableWidget,
    QTableWidgetItem, QHeaderView, QSplitter,
)

HOST = "127.0.0.1"
DEFAULT_BUS_PORT = 20100


def parse_ids(text):
    """Parse a filter string like '0x469, 1ea 180' into a set of ints (hex)."""
    out = set()
    for tok in text.replace(",", " ").split():
        try:
            out.add(int(tok, 16))
        except ValueError:
            pass
    return out


class HubThread(QThread):
    """The actual hub: recvfrom + broadcast-to-others (no prune). Emits each frame."""
    frame = Signal(dict)          # {ts, src_port, can_id, dlc, data}
    peers = Signal(int)
    status = Signal(str)

    def __init__(self, bus_port):
        super().__init__()
        self.bus_port = bus_port
        self._running = True

    def run(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            sock.ioctl(socket.SIO_UDP_CONNRESET, False)   # Windows: ignore ICMP resets
        except (AttributeError, OSError):
            pass
        try:
            sock.bind((HOST, self.bus_port))
        except OSError as e:
            self.status.emit(f"bind :{self.bus_port} FAILED - {e}")
            return
        sock.settimeout(0.2)
        self.status.emit(f"hub up on {HOST}:{self.bus_port}")
        known = {}                                        # addr -> True (never pruned)
        while self._running:
            try:
                pkt, src = sock.recvfrom(64)
            except socket.timeout:
                continue
            except (ConnectionResetError, OSError):
                continue
            if src not in known:
                known[src] = True
                self.peers.emit(len(known))
            for addr in list(known):                      # fan out to everyone else
                if addr != src:
                    try:
                        sock.sendto(pkt, addr)
                    except OSError:
                        pass
            if len(pkt) >= 3:
                dlc = pkt[2]
                n = min(dlc, 8)
                self.frame.emit({
                    "ts": time.time(),
                    "src_port": src[1],
                    "can_id": pkt[0] | (pkt[1] << 8),
                    "dlc": dlc,
                    "data": list(pkt[3:3 + n]),
                })
        try:
            sock.close()
        except OSError:
            pass

    def stop(self):
        self._running = False
        self.wait(1000)


class HubWindow(QMainWindow):

    def __init__(self, bus_port):
        super().__init__()
        self.setWindowTitle("CAN Bus Hub + Monitor")
        self.hub = None
        self.paused = False
        self.filter_ids = set()
        self.filter_on = False
        self.summary = {}        # can_id -> [count, last_data_str, src_port, last_ts]
        self._dirty = False      # summary changed since last table refresh

        central = QWidget()
        self.setCentralWidget(central)
        root = QVBoxLayout(central)

        # --- connection bar ------------------------------------------------
        bar = QHBoxLayout()
        bar.addWidget(QLabel("Bus port:"))
        self.port_spin = QSpinBox()
        self.port_spin.setRange(1, 65535)
        self.port_spin.setValue(bus_port)
        bar.addWidget(self.port_spin)
        self.start_btn = QPushButton("Start")
        self.start_btn.clicked.connect(self._toggle)
        bar.addWidget(self.start_btn)
        self.status_lbl = QLabel("stopped")
        bar.addWidget(self.status_lbl, 1)
        self.peers_lbl = QLabel("peers: 0")
        bar.addWidget(self.peers_lbl)
        root.addLayout(bar)

        # --- filter bar ----------------------------------------------------
        fbar = QHBoxLayout()
        self.filter_chk = QCheckBox("Show only IDs:")
        self.filter_chk.stateChanged.connect(self._apply_filter)
        fbar.addWidget(self.filter_chk)
        self.filter_edit = QLineEdit()
        self.filter_edit.setPlaceholderText("e.g.  0x469, 0x1EA, 180   (hex, space/comma separated)")
        self.filter_edit.editingFinished.connect(self._apply_filter)
        fbar.addWidget(self.filter_edit, 1)
        self.pause_btn = QPushButton("Pause log")
        self.pause_btn.setCheckable(True)
        self.pause_btn.toggled.connect(self._set_paused)
        fbar.addWidget(self.pause_btn)
        clear_btn = QPushButton("Clear")
        clear_btn.clicked.connect(self._clear)
        fbar.addWidget(clear_btn)
        root.addLayout(fbar)

        # --- summary table (top) + frame log (bottom) ----------------------
        split = QSplitter(Qt.Vertical)

        self.table = QTableWidget(0, 5)
        self.table.setHorizontalHeaderLabels(["CAN ID", "Count", "Src", "Last Data", "Last Seen"])
        self.table.horizontalHeader().setSectionResizeMode(3, QHeaderView.Stretch)
        self.table.setEditTriggers(QTableWidget.NoEditTriggers)
        self.table.verticalHeader().setVisible(False)
        # double-click a row to filter to just that ID
        self.table.cellDoubleClicked.connect(self._filter_to_row)
        split.addWidget(self.table)

        self.log = QPlainTextEdit()
        self.log.setReadOnly(True)
        self.log.setMaximumBlockCount(5000)
        self.log.setStyleSheet("font-family: Consolas, monospace;")
        split.addWidget(self.log)
        split.setSizes([240, 380])
        root.addWidget(split, 1)

        # refresh the summary table on a timer (cheap; not per-frame)
        self._timer = QTimer(self)
        self._timer.setInterval(250)
        self._timer.timeout.connect(self._refresh_table)
        self._timer.start()

        self.resize(760, 640)

    # -- hub start/stop ----------------------------------------------------
    def _toggle(self):
        if self.hub is None:
            self.hub = HubThread(self.port_spin.value())
            self.hub.frame.connect(self._on_frame)
            self.hub.peers.connect(lambda n: self.peers_lbl.setText(f"peers: {n}"))
            self.hub.status.connect(self.status_lbl.setText)
            self.hub.start()
            self.port_spin.setEnabled(False)
            self.start_btn.setText("Stop")
        else:
            self.hub.stop()
            self.hub = None
            self.port_spin.setEnabled(True)
            self.start_btn.setText("Start")
            self.status_lbl.setText("stopped")
            self.peers_lbl.setText("peers: 0")

    # -- per-frame handling ------------------------------------------------
    def _on_frame(self, f):
        cid = f["can_id"]
        data_str = " ".join(f"{b:02X}" for b in f["data"])
        rec = self.summary.get(cid)
        if rec is None:
            self.summary[cid] = [1, data_str, f["src_port"], f["ts"]]
        else:
            rec[0] += 1
            rec[1] = data_str
            rec[2] = f["src_port"]
            rec[3] = f["ts"]
        self._dirty = True

        if self.paused:
            return
        if self.filter_on and self.filter_ids and cid not in self.filter_ids:
            return
        ts = datetime.datetime.fromtimestamp(f["ts"]).strftime("%H:%M:%S.%f")[:-3]
        self.log.appendPlainText(
            f"[{ts}] :{f['src_port']:<5} 0x{cid:03X} [{f['dlc']}] {data_str}"
        )

    def _refresh_table(self):
        if not self._dirty:
            return
        self._dirty = False
        rows = sorted(self.summary.items())
        if self.filter_on and self.filter_ids:
            rows = [(cid, r) for cid, r in rows if cid in self.filter_ids]
        self.table.setRowCount(len(rows))
        for i, (cid, r) in enumerate(rows):
            count, data_str, src, ts = r
            seen = datetime.datetime.fromtimestamp(ts).strftime("%H:%M:%S.%f")[:-3]
            for col, val in enumerate(
                (f"0x{cid:03X}", str(count), f":{src}", data_str, seen)
            ):
                item = QTableWidgetItem(val)
                if col in (1, 2):
                    item.setTextAlignment(Qt.AlignCenter)
                self.table.setItem(i, col, item)

    # -- filter / controls -------------------------------------------------
    def _apply_filter(self):
        self.filter_ids = parse_ids(self.filter_edit.text())
        self.filter_on = self.filter_chk.isChecked()
        self._dirty = True

    def _filter_to_row(self, row, _col):
        item = self.table.item(row, 0)
        if item:
            self.filter_edit.setText(item.text())
            self.filter_chk.setChecked(True)
            self._apply_filter()

    def _set_paused(self, on):
        self.paused = on
        self.pause_btn.setText("Resume log" if on else "Pause log")

    def _clear(self):
        self.log.clear()
        self.summary.clear()
        self.table.setRowCount(0)
        self._dirty = False

    def closeEvent(self, ev):
        if self.hub is not None:
            self.hub.stop()
        super().closeEvent(ev)


def main():
    bus_port = int(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_BUS_PORT
    app = QApplication(sys.argv)
    w = HubWindow(bus_port)
    w.show()
    w._toggle()          # auto-start the hub on launch
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
