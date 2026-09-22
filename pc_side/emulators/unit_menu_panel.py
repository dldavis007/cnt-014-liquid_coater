#!/usr/bin/env python3
"""
unit_menu_panel.py

The button box's own TOP-LEVEL menu: the list of units found by the bus scan,
which you select from to open an individual unit's menu.

This mirrors what the real box does in Scan() — discovered units are written
into the box's own menu, split by the last byte of each unit's 0x2100 name:

    0x32  -> the front menu   (Menuc[0], via ExternalMenu)
    0xFF  -> the AUX UNITS sub-menu (Menuc[5], via ExternalSubMenu)

Selecting an entry there opens that unit's menu; here, choosing a row re-points
the MENU/TRIGGER byte at that unit's RPDO1 (0x200 + node) and emits it, so the
host can pulse MENU and the WIM display panel then shows that unit's screens.

Unit menus are MUTUALLY EXCLUSIVE. On the real box there is one display and one
button word, so once a unit's menu is up the box's own list is off screen and
unreachable — you have to walk to EXIT and close that menu before you can pick
another unit. set_active_unit() reproduces that: it locks the list while a menu
is open. The caller drives it from the unit's own menu-status byte on CAN 0x200
(1 = opened, 0 = closed), so the lock follows the FIRMWARE, not our guess.

Transport-agnostic like the other panels: it holds no CAN knowledge beyond the
menu id already worked out by node_scanner.
"""

from PySide6.QtCore import Qt, Signal
from PySide6.QtGui import QFont
from PySide6.QtWidgets import (
    QGroupBox, QVBoxLayout, QHBoxLayout, QLabel, QPushButton, QListWidget,
    QListWidgetItem,
)

AUX_HEADER = "── AUX UNITS ──"

# Row prefixes: the open menu is marked, everything else is indented to match.
ACTIVE_MARK = "▶ "     # ▶
IDLE_MARK   = "  "


class UnitMenuPanel(QGroupBox):
    """Lists discovered units; emits the chosen one."""

    unit_chosen = Signal(dict)     # the unit dict from node_scanner
    rescan      = Signal()

    def __init__(self, title="Button Box Menu  (units on the bus)", parent=None):
        super().__init__(title, parent)
        self._units = []
        self._active = None            # unit whose menu is open, or None

        root = QVBoxLayout(self)

        mono = QFont("Consolas")
        mono.setStyleHint(QFont.Monospace)
        mono.setPointSize(11)

        self.list = QListWidget()
        self.list.setFont(mono)
        self.list.setStyleSheet(
            "background:#101418; color:#7CFC7C; border:1px solid #2a3138;")
        self.list.setMinimumHeight(120)
        self.list.itemDoubleClicked.connect(self._emit_current)
        self.list.currentItemChanged.connect(self._sync_button)
        root.addWidget(self.list)

        row = QHBoxLayout()
        self.open_btn = QPushButton("Open unit menu")
        self.open_btn.setToolTip(
            "Point MENU / TRIGGER at this unit and open its menu "
            "(its RPDO1 at 0x200 + node id).")
        self.open_btn.clicked.connect(self._emit_current)
        self.open_btn.setEnabled(False)
        row.addWidget(self.open_btn)

        self.scan_btn = QPushButton("Scan bus")
        self.scan_btn.setToolTip(
            "SDO-scan the bus at index 0x2100, as the real button box does at "
            "power-up. Node ids come from which ID each reply arrives on.")
        self.scan_btn.clicked.connect(self.rescan.emit)
        row.addWidget(self.scan_btn)

        self.count_lbl = QLabel("not scanned")
        row.addWidget(self.count_lbl, 1)
        root.addLayout(row)

        # Says why the list is locked when a unit's menu is up.
        self.lock_lbl = QLabel("")
        self.lock_lbl.setWordWrap(True)
        root.addWidget(self.lock_lbl)

        self.set_units([])

    # ── population ────────────────────────────────────────────────────────
    def set_units(self, units):
        """Rebuild the list: front-menu units first, then an AUX UNITS section."""
        self._units = list(units)
        self.list.clear()

        if not units:
            item = QListWidgetItem("  (no units - press Scan bus)")
            item.setFlags(Qt.NoItemFlags)
            self.list.addItem(item)
            self.count_lbl.setText("not scanned")
            self.open_btn.setEnabled(False)
            return

        aux_started = False
        for u in units:                      # already sorted: front, then aux
            if u["aux"] and not aux_started:
                aux_started = True
                hdr = QListWidgetItem(AUX_HEADER)
                hdr.setFlags(Qt.NoItemFlags)   # a divider, not selectable
                self.list.addItem(hdr)
            item = QListWidgetItem(f"{IDLE_MARK}{u['name']}")
            item.setData(Qt.UserRole, u)
            item.setToolTip(
                f"node 0x{u['node']:02X}   menu 0x{u['menu_id']:03X}   "
                f"name marker 0x{u['marker']:02X} "
                f"({'AUX UNITS' if u['aux'] else 'front menu'})")
            self.list.addItem(item)

        n_aux = sum(1 for u in units if u["aux"])
        self.count_lbl.setText(
            f"{len(units)} unit(s)" + (f", {n_aux} aux" if n_aux else ""))

        # Select the first real (non-divider) row.
        for i in range(self.list.count()):
            if self.list.item(i).data(Qt.UserRole) is not None:
                self.list.setCurrentRow(i)
                break

        # A rescan rebuilds every row, so restore the open-menu marking.
        self._refresh_active()

    def units(self):
        return list(self._units)

    # ── which unit currently owns the menu ────────────────────────────────
    def set_active_unit(self, unit):
        """Mark `unit` as having its menu open, or None for 'menu closed'.

        Driven by the 0x200 menu-status byte the unit itself sends, so the UI
        can never disagree with the firmware about what is open."""
        self._active = unit
        self._refresh_active()

    def active_unit(self):
        return self._active

    def is_locked(self):
        return self._active is not None

    def _refresh_active(self):
        node = self._active["node"] if self._active else None
        for i in range(self.list.count()):
            item = self.list.item(i)
            u = item.data(Qt.UserRole)
            if u is None:
                continue                       # divider / placeholder
            open_now = (u["node"] == node)
            item.setText(f"{ACTIVE_MARK if open_now else IDLE_MARK}{u['name']}")
            # Grey out everything you cannot reach while a menu is open.
            item.setFlags(Qt.ItemIsEnabled | Qt.ItemIsSelectable
                          if (node is None or open_now) else Qt.NoItemFlags)

        if self._active is None:
            self.lock_lbl.setText("")
        else:
            self.lock_lbl.setText(
                f"Menu open on {self._active['name']} — only one unit menu can "
                f"be open at a time. Walk to EXIT and press Select to close it "
                f"before choosing another.")
        self._sync_button()

    # ── selection ─────────────────────────────────────────────────────────
    def _current_unit(self):
        item = self.list.currentItem()
        return item.data(Qt.UserRole) if item is not None else None

    def _sync_button(self, *_):
        self.open_btn.setEnabled(
            self._current_unit() is not None and not self.is_locked())

    def _emit_current(self, *_):
        # Mutually exclusive: refuse while another menu is still open.
        if self.is_locked():
            return
        u = self._current_unit()
        if u is not None:
            self.unit_chosen.emit(u)
