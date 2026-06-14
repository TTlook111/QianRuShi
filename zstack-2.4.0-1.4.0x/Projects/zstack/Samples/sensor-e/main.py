"""
main.py — 智能门禁访客管理系统 PyQt5 上位机入口
E同学：Python应用层负责人

启动方式：
    python main.py

依赖安装：
    pip install -r requirements.txt
"""

import sys
import os
import logging

# 将当前目录加入 Python 路径
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from PyQt5.QtWidgets import QApplication
from PyQt5.QtCore import Qt

import config


def setup_logging():
    """配置日志"""
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
        datefmt="%H:%M:%S"
    )


def load_stylesheet(app):
    """加载 QSS 样式表"""
    qss_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "resources", "style.qss"
    )
    if os.path.exists(qss_path):
        with open(qss_path, "r", encoding="utf-8") as f:
            app.setStyleSheet(f.read())
        logging.info("已加载样式表: %s", qss_path)
    else:
        logging.warning("样式表文件不存在: %s", qss_path)


def main():
    """程序入口"""
    setup_logging()
    logger = logging.getLogger(__name__)
    logger.info("=" * 50)
    logger.info("智能门禁访客管理系统 — PyQt5 上位机")
    logger.info("E同学：Python应用层负责人")
    logger.info("=" * 50)

    # 高DPI支持
    QApplication.setAttribute(Qt.AA_EnableHighDpiScaling, True)
    QApplication.setAttribute(Qt.AA_UseHighDpiPixmaps, True)

    app = QApplication(sys.argv)
    app.setApplicationName(config.WINDOW_TITLE)

    # 加载深色主题样式
    load_stylesheet(app)

    # 创建主窗口
    from ui.main_window import MainWindow
    window = MainWindow()
    window.show()

    logger.info("主窗口已显示，进入事件循环")
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
