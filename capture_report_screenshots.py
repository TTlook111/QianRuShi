import os
import sys
from datetime import datetime, timedelta
from pathlib import Path

ROOT = Path(r"F:\WLWKFSX\QimoProject")
APP_DIR = ROOT / "zstack-2.4.0-1.4.0x" / "Projects" / "zstack" / "Samples" / "sensor-e"
OUT_DIR = ROOT / "report_assets"
OUT_DIR.mkdir(exist_ok=True)

sys.path.insert(0, str(APP_DIR))
os.chdir(APP_DIR)

from PyQt5.QtCore import QTimer, Qt
from PyQt5.QtWidgets import QApplication, QScrollArea

import config

config.USE_REAL_CLOUD = False

from ui.main_window import MainWindow
from data.models import AccessLog


def main():
    app = QApplication(sys.argv)
    style_path = APP_DIR / "resources" / "style.qss"
    if style_path.exists():
        app.setStyleSheet(style_path.read_text(encoding="utf-8"))

    win = MainWindow()
    win.resize(1600, 1000)
    win.show()

    def populate_and_capture():
        try:
            samples = [
                {"_addr": config.NODE_MAC["sensor-a"], "temp": 29.4, "humi": 46.4, "lux": 457},
                {"_addr": config.NODE_MAC["sensor-c"], "pir": 1, "door": 0, "tch": 0, "alert": 0,
                 "flame": 0, "gas": 0, "grating": 0, "stay": 18, "night": 0},
                {"_addr": config.NODE_MAC["sensor-b"], "unlock": 0, "buzz": 0, "rgb": [0, 255, 0], "alert": 0},
                {"_addr": config.NODE_MAC["sensor-c"], "pir": 1, "door": 1, "tch": 1, "alert": 1,
                 "flame": 0, "gas": 0, "grating": 0, "stay": 28, "night": 1},
                {"_addr": config.NODE_MAC["sensor-b"], "unlock": 1, "buzz": 1, "rgb": [255, 0, 0], "alert": 2},
            ]
            for item in samples:
                win._on_data_received(item)
                app.processEvents()

            base_time = datetime.now()
            demo_events = [
                ("tch", "doorbell", 1, 0, 0),
                ("pir", "pir_detect", 1, 0, 1),
                ("remote", "remote_unlock", 1, 0, 1),
                ("pir", "loiter", 28, 1, 2),
                ("door", "door_open", 1, 2, 2),
                ("alert", "night_alarm", 2, 2, 3),
                ("tch", "doorbell", 1, 0, 4),
                ("pir", "pir_detect", 1, 0, 5),
            ]
            for sensor, event_type, value, alert, minutes_ago in demo_events:
                win._db.insert_access_log(AccessLog(
                    student_id="",
                    sensor=sensor,
                    event_type=event_type,
                    value=value,
                    alert=alert,
                    is_remote_unlock=(event_type == "remote_unlock"),
                    create_time=base_time - timedelta(minutes=minutes_ago),
                ))
            win._refresh_database_counts()
            if hasattr(win, "_stats_panel"):
                win._stats_panel._show_daily_chart()
            app.processEvents()

            top_path = OUT_DIR / "ui_light_dashboard_top.png"
            win.grab().save(str(top_path))

            scroll = win.findChild(QScrollArea)
            if scroll:
                scroll.verticalScrollBar().setValue(scroll.verticalScrollBar().maximum())
                app.processEvents()
            bottom_path = OUT_DIR / "ui_light_dashboard_bottom.png"
            win.grab().save(str(bottom_path))
            print(top_path)
            print(bottom_path)
        except Exception as exc:
            print(f"capture failed: {exc}", file=sys.stderr)
        finally:
            app.quit()

    QTimer.singleShot(1200, populate_and_capture)
    app.exec_()


if __name__ == "__main__":
    main()
