# -*- coding: utf-8 -*-
"""Entry point for the Lua MCU UART update GUI."""

import sys

from PyQt5.QtWidgets import QApplication

from gui import MainWindow


def main() -> int:
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    return app.exec_()


if __name__ == "__main__":
    raise SystemExit(main())
