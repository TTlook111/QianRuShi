from pathlib import Path
import shutil

from PIL import Image, ImageDraw, ImageFont
from docx import Document
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_CELL_VERTICAL_ALIGNMENT
from docx.shared import Inches, Pt, RGBColor
from docx.oxml import OxmlElement
from docx.oxml.ns import qn


ROOT = Path(r"F:\WLWKFSX\QimoProject")
TEMPLATE = ROOT / "期末综合项目模板_wordsave.docx"
OUT = ROOT / "期末综合项目_智能门禁访客管理系统_完成版.docx"
IMG_DIR = ROOT / "report_assets"
IMG_DIR.mkdir(exist_ok=True)

FONT = r"C:\Windows\Fonts\msyh.ttc"
FONT_BOLD = r"C:\Windows\Fonts\msyhbd.ttc"


def font(size, bold=False):
    return ImageFont.truetype(FONT_BOLD if bold else FONT, size)


def rounded(draw, box, fill, outline="#2c3e50", radius=18, width=2):
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def multiline_center(draw, box, text, fnt, fill="#102030", spacing=6):
    x1, y1, x2, y2 = box
    lines = text.split("\n")
    heights = [draw.textbbox((0, 0), line, font=fnt)[3] for line in lines]
    total = sum(heights) + spacing * (len(lines) - 1)
    y = y1 + (y2 - y1 - total) / 2
    for line, h in zip(lines, heights):
        bbox = draw.textbbox((0, 0), line, font=fnt)
        x = x1 + (x2 - x1 - (bbox[2] - bbox[0])) / 2
        draw.text((x, y), line, font=fnt, fill=fill)
        y += h + spacing


def arrow(draw, start, end, fill="#34495e", width=4):
    draw.line([start, end], fill=fill, width=width)
    x1, y1 = start
    x2, y2 = end
    if abs(x2 - x1) >= abs(y2 - y1):
        s = 12 if x2 >= x1 else -12
        pts = [(x2, y2), (x2 - s, y2 - 8), (x2 - s, y2 + 8)]
    else:
        s = 12 if y2 >= y1 else -12
        pts = [(x2, y2), (x2 - 8, y2 - s), (x2 + 8, y2 - s)]
    draw.polygon(pts, fill=fill)


def make_architecture():
    path = IMG_DIR / "architecture_3layer.png"
    img = Image.new("RGB", (1600, 900), "#f7f9fb")
    d = ImageDraw.Draw(img)
    title = "智能门禁访客管理系统三层架构"
    d.text((60, 35), title, font=font(42, True), fill="#12324a")
    d.text((62, 92), "感知层采集/执行，网络层组网传输，应用层展示、控制与存储", font=font(22), fill="#536475")

    bands = [
        (130, 190, 1470, 360, "#e8f4ff", "硬件层 / 感知层"),
        (130, 400, 1470, 570, "#eef9f1", "传输层 / 网络层"),
        (130, 610, 1470, 780, "#fff7e8", "应用层"),
    ]
    for x1, y1, x2, y2, color, label in bands:
        rounded(d, (x1, y1, x2, y2), color, "#aab7c4", 20, 2)
        d.text((x1 + 25, y1 + 20), label, font=font(26, True), fill="#1f3a4d")

    nodes = [
        ((260, 245, 510, 330), "sensor-a\n温湿度/光照", "#ffffff"),
        ((560, 245, 810, 330), "sensor-c\nPIR/门磁/门铃", "#ffffff"),
        ((860, 245, 1110, 330), "sensor-b\n门锁/蜂鸣/RGB", "#ffffff"),
        ((1160, 245, 1370, 330), "Coordinator\n协调器", "#ffffff"),
        ((250, 450, 490, 535), "ZigBee\nZ-Stack 组网", "#ffffff"),
        ((560, 450, 850, 535), "ZXBee帧协议\nAA...CHK...55", "#ffffff"),
        ((930, 450, 1230, 535), "智云平台\nWebSocket/TCP", "#ffffff"),
        ((250, 660, 530, 745), "Python解析\n字段归一化", "#ffffff"),
        ((610, 660, 890, 745), "PyQt5界面\n实时看板/控制", "#ffffff"),
        ((970, 660, 1250, 745), "MySQL数据库\n事件与环境数据", "#ffffff"),
    ]
    for box, text, color in nodes:
        rounded(d, box, color, "#5f7f99", 16, 2)
        multiline_center(d, box, text, font(24, True), "#1f3345")

    arrow(d, (510, 288), (560, 288))
    arrow(d, (810, 288), (860, 288))
    arrow(d, (1110, 288), (1160, 288))
    arrow(d, (1265, 330), (1090, 450))
    arrow(d, (490, 492), (560, 492))
    arrow(d, (850, 492), (930, 492))
    arrow(d, (1080, 535), (760, 660))
    arrow(d, (530, 702), (610, 702))
    arrow(d, (890, 702), (970, 702))
    d.text((1310, 492), "上行：状态/告警\n下行：开门/告警/复位", font=font(20), fill="#2e5d3e")
    img.save(path)
    return path


def make_wiring():
    path = IMG_DIR / "wiring_diagram.png"
    img = Image.new("RGB", (1600, 950), "#fbfbf8")
    d = ImageDraw.Draw(img)
    d.text((60, 35), "CC2530 门禁节点传感器与执行器接线示意", font=font(40, True), fill="#243b53")
    d.text((62, 90), "按项目实现整理，报告中用于说明主要引脚映射与信号方向", font=font(22), fill="#66788a")

    rounded(d, (640, 240, 960, 690), "#eaf2f8", "#2b5c7c", 22, 3)
    multiline_center(d, (640, 240, 960, 690), "CC2530\nZigBee SoC\n3.3V GPIO", font(30, True), "#12344d")

    left = [
        ((90, 185, 430, 265), "PIR人体红外\nP0.0 输入", "#ffe8e8"),
        ((90, 305, 430, 385), "震动/触摸门铃\nP0.1/P0.0 输入", "#fff1d6"),
        ((90, 425, 430, 505), "霍尔门磁\nP0.2 输入", "#e8f6ef"),
        ((90, 545, 430, 625), "火焰/可燃气体\nP0.3/P0.4 输入", "#f4ecff"),
        ((90, 665, 430, 745), "红外光栅\nP0.5 输入", "#e9f7ff"),
    ]
    right = [
        ((1170, 255, 1510, 335), "继电器/门锁\nP0.6 输出", "#e8f6ef"),
        ((1170, 405, 1510, 485), "蜂鸣器\nP0.7 输出", "#fff1d6"),
        ((1170, 555, 1510, 635), "RGB指示灯\nPWM/GPIO 输出", "#ffe8e8"),
    ]
    for box, text, color in left + right:
        rounded(d, box, color, "#8ca0b3", 16, 2)
        multiline_center(d, box, text, font(23, True), "#203040")

    for box, _, _ in left:
        arrow(d, (box[2], (box[1] + box[3]) // 2), (640, (box[1] + box[3]) // 2), "#3f6d8a")
    for box, _, _ in right:
        arrow(d, (960, (box[1] + box[3]) // 2), (box[0], (box[1] + box[3]) // 2), "#3f6d8a")

    rounded(d, (560, 760, 1040, 840), "#ffffff", "#9aa7b1", 14, 2)
    multiline_center(d, (560, 760, 1040, 840), "VDD 3.3V 与 GND 为所有模块公共供电\n输入信号统一进行去抖/中断处理", font(21), "#394b59")
    img.save(path)
    return path


def set_cell_shading(cell, fill):
    tc_pr = cell._tc.get_or_add_tcPr()
    shd = OxmlElement("w:shd")
    shd.set(qn("w:fill"), fill)
    tc_pr.append(shd)


def style_table(table):
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    tbl_pr = table._tbl.tblPr
    borders = tbl_pr.first_child_found_in("w:tblBorders")
    if borders is None:
        borders = OxmlElement("w:tblBorders")
        tbl_pr.append(borders)
    for edge in ("top", "left", "bottom", "right", "insideH", "insideV"):
        tag = "w:" + edge
        element = borders.find(qn(tag))
        if element is None:
            element = OxmlElement(tag)
            borders.append(element)
        element.set(qn("w:val"), "single")
        element.set(qn("w:sz"), "6")
        element.set(qn("w:space"), "0")
        element.set(qn("w:color"), "A6B3BF")
    for row_idx, row in enumerate(table.rows):
        for cell in row.cells:
            cell.vertical_alignment = WD_CELL_VERTICAL_ALIGNMENT.CENTER
            for p in cell.paragraphs:
                p.paragraph_format.space_after = Pt(0)
                for r in p.runs:
                    r.font.name = "宋体"
                    r._element.rPr.rFonts.set(qn("w:eastAsia"), "宋体")
                    r.font.size = Pt(9)
            if row_idx == 0:
                set_cell_shading(cell, "D9EAF7")
                for p in cell.paragraphs:
                    for r in p.runs:
                        r.bold = True


def set_run_font(run, name="宋体", size=10.5, bold=False):
    run.font.name = name
    run._element.rPr.rFonts.set(qn("w:eastAsia"), name)
    run.font.size = Pt(size)
    run.bold = bold


def add_para(doc, text="", style=None, bold_prefix=None):
    p = doc.add_paragraph(style=style)
    p.paragraph_format.first_line_indent = Pt(21) if style is None else None
    p.paragraph_format.line_spacing = 1.25
    p.paragraph_format.space_after = Pt(6)
    if bold_prefix and text.startswith(bold_prefix):
        r1 = p.add_run(bold_prefix)
        set_run_font(r1, bold=True)
        r2 = p.add_run(text[len(bold_prefix):])
        set_run_font(r2)
    else:
        r = p.add_run(text)
        set_run_font(r)
    return p


def add_heading(doc, text, level):
    p = doc.add_paragraph(text, style=f"Heading {level}")
    for r in p.runs:
        set_run_font(r, "黑体", 14 if level == 1 else 12, True)
    return p


def add_caption(doc, text):
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_after = Pt(8)
    r = p.add_run(text)
    set_run_font(r, "宋体", 9)


def add_picture(doc, path, width=5.8):
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = p.add_run()
    run.add_picture(str(path), width=Inches(width))
    return p


def add_table(doc, headers, rows):
    table = doc.add_table(rows=1, cols=len(headers))
    for i, h in enumerate(headers):
        table.rows[0].cells[i].text = h
    for row_data in rows:
        row = table.add_row()
        for i, val in enumerate(row_data):
            row.cells[i].text = str(val)
    style_table(table)
    doc.add_paragraph()
    return table


def clear_body_from(doc, marker="1  系统概述"):
    start = None
    for i, p in enumerate(doc.paragraphs):
        if p.text.strip() == marker and i > 20:
            start = i
            break
    if start is None:
        raise RuntimeError("Cannot locate body start")
    for p in list(doc.paragraphs[start:]):
        p._element.getparent().remove(p._element)
    doc.add_page_break()


def fill_cover(doc):
    replacements = {
        "综合项目：  智能物联网系统实现": "综合项目：  智能门禁访客管理系统实现",
        "学生姓名： （三号，仿宋，下划线）": "学生姓名：________________",
        "所在学院： （三号，仿宋，下划线）": "所在学院：________________",
        "专    业： （三号，仿宋，下划线）": "专    业：________________",
        "学    号：（阿拉伯数字，三号，仿宋，下划线）": "学    号：________________",
    }
    for p in doc.paragraphs:
        if p.text.strip() in replacements:
            p.text = replacements[p.text.strip()]
            for r in p.runs:
                set_run_font(r, "仿宋", 16)


def build_doc():
    arch = make_architecture()
    wiring = make_wiring()
    clean_ui = ROOT / "screenshots" / "pyqt_main_window_clean.png"
    hw_photo = ROOT / "221a402f28e548763e0e749dae77ff0c.jpg"

    shutil.copyfile(TEMPLATE, OUT)
    doc = Document(OUT)
    fill_cover(doc)
    clear_body_from(doc)

    add_heading(doc, "1  系统概述", 1)
    add_heading(doc, "1.1  项目背景与意义", 2)
    add_para(doc, "随着校园、办公区和家庭门禁场景对安全性、远程化和数据化管理的需求提高，传统门锁和单点报警方式已经难以满足访客识别、异常提醒、远程控制和历史追溯等综合要求。本项目以“智能门禁访客管理系统”为主题，基于 CC2530、Z-Stack、ZXBee 帧协议、智云平台和 Python/PyQt5 上位机，完成从硬件感知到云端转发再到应用展示与数据库存储的完整物联网闭环。")
    add_para(doc, "项目的意义在于将课程中的传感器驱动、ZigBee 组网、云平台接入、上位机界面和数据库 CRUD 综合到同一业务场景中，实现“有人靠近可感知、门铃事件可记录、异常开门可告警、远程控制可执行、历史数据可查询”的系统目标。")
    add_heading(doc, "1.2  应用场景描述", 2)
    add_para(doc, "系统面向宿舍、实验室、办公室等小型门禁场景。门口部署安防检测节点，持续采集 PIR 人体红外、门磁、门铃/触摸、震动、火焰、可燃气体和红外光栅等状态；室内或控制端部署执行节点，完成门锁继电器、蜂鸣器和 RGB 灯联动；PC 端上位机通过智云平台读取实时数据并下发控制命令。")
    add_para(doc, "典型流程为：访客靠近门口时 PIR 触发，若按下门铃则生成访客通知；若停留超过设定时间但未按门铃，则记录为徘徊事件；夜间模式下门磁检测到门被打开，系统立即升级为报警等级，并通过蜂鸣器、红灯和上位机事件日志进行提示。")
    add_heading(doc, "1.3  功能需求概述", 2)
    requirements = [
        ("数据采集功能", "sensor-a 定时采集温度、湿度和光照；sensor-c 采集 PIR、门磁、门铃、火焰、气体、光栅等安防状态。"),
        ("无线传输功能", "各终端节点通过 Z-Stack 组网，经协调器完成 ZigBee 数据收发。"),
        ("云平台接入功能", "协调器与智云平台对接，使用 WebSocket/TCP 方式实现数据中转。"),
        ("远程监控功能", "PyQt5 上位机实时显示门禁状态、环境信息、事件时间线和统计图表。"),
        ("远程控制功能", "支持远程开门、手动关门、门铃触发、声光告警、解除告警、夜间/白天模式切换。"),
        ("数据持久化功能", "数据库保存访客事件和环境数据，支持插入、查询、更新、删除和导出。"),
        ("告警联动功能", "依据停留时长、夜间模式和门磁状态实现注意/报警两级联动。"),
        ("团队协作", "硬件节点、通信协议、上位机界面和数据库按统一字段表对接。"),
    ]
    for i, (k, v) in enumerate(requirements, 1):
        add_para(doc, f"（{i}）{k}：{v}")

    add_heading(doc, "2  系统方案设计", 1)
    add_heading(doc, "2.1  系统总体架构（三层架构）", 2)
    add_para(doc, "本系统采用物联网三层架构。硬件层负责环境数据、门禁安防状态采集和执行器控制；传输层负责 ZigBee 组网、ZXBee 帧封包、协调器转发和智云平台通信；应用层负责 Python 数据解析、PyQt5 可视化、远程控制和数据库持久化。")
    add_picture(doc, arch, 6.2)
    add_caption(doc, "图2-1 系统三层架构图")
    add_heading(doc, "2.2  方案框图", 2)
    add_para(doc, "系统由四类核心节点组成：协调器负责网络管理和串口/云端桥接；sensor-a 负责环境数据采集；sensor-b 负责门锁、蜂鸣器和 RGB 灯等执行器控制；sensor-c 负责门禁安防检测。各节点通过统一字段和统一 ZXBee 帧协议完成上行状态上报与下行命令执行。")
    add_table(doc, ["模块", "主要组成", "作用"], [
        ["感知采集", "sensor-a、sensor-c", "采集温湿光照、人体红外、门磁、门铃和扩展安防状态"],
        ["控制执行", "sensor-b", "执行远程开门、蜂鸣器、RGB 灯和告警复位"],
        ["网络传输", "ZigBee + Coordinator", "建立无线网络，转发终端节点与智云平台之间的数据"],
        ["应用管理", "Python/PyQt5 + MySQL", "实时看板、远程控制、事件时间线、统计分析和数据持久化"],
    ])

    add_heading(doc, "3  系统硬件设计", 1)
    add_heading(doc, "3.1  硬件平台概述", 2)
    add_para(doc, "项目基于 xLab-BaseKits（CC2530）实验箱平台开发。CC2530 集成 8051 内核与 2.4GHz IEEE 802.15.4 射频收发器，适合低功耗 ZigBee 节点。开发环境采用 IAR Embedded Workbench for 8051 和 Z-Stack 协议栈，节点工程位于 zstack-2.4.0-1.4.0x/Projects/zstack/Samples/。")
    add_picture(doc, hw_photo, 5.8)
    add_caption(doc, "图3-1 CC2530 实验箱与 ZigBee 节点实物图")
    add_heading(doc, "3.2  传感器选型与引脚映射", 2)
    add_table(doc, ["节点", "传感器/执行器", "连接引脚/接口", "功能说明"], [
        ["sensor-a", "HTU21D", "I2C", "采集温度和湿度"],
        ["sensor-a", "BH1750", "I2C", "采集光照强度"],
        ["sensor-c", "PIR 人体红外", "P0.0", "检测访客靠近"],
        ["sensor-c", "触摸/震动门铃", "P0.0/P0.1", "检测门铃或敲门事件"],
        ["sensor-c", "霍尔门磁", "P0.2", "判断门开/关状态"],
        ["sensor-c", "火焰/可燃气体/光栅", "P0.3/P0.4/P0.5", "扩展安防检测"],
        ["sensor-b", "继电器/蜂鸣器/RGB", "GPIO/PWM", "执行开门、提示和告警联动"],
    ])
    add_heading(doc, "3.3  传感器电路原理图", 2)
    add_picture(doc, wiring, 6.1)
    add_caption(doc, "图3-2 CC2530 与传感器/执行器接线示意图")
    add_heading(doc, "3.4  CC2530外设驱动设计", 2)
    add_para(doc, "sensor-a 初始化 HTU21D 和 BH1750 后周期读取环境数据，并通过 sensorUpdate() 构造 JSON 负载上报。sensor-c 使用 GPIO 输入与中断结合的方式读取 PIR、门磁、触摸、震动等传感器状态，并在 security_logic_update() 中计算停留时间、夜间状态和告警等级。sensor-b 负责继电器、蜂鸣器和 RGB 灯驱动，支持开门后自动关闭、门铃短响和告警长响。")
    add_para(doc, "驱动设计中对输入类传感器进行去抖处理，对状态变化采用立即上报和周期上报结合的方式，既保证事件响应速度，也避免无效重复数据占用链路。")
    add_heading(doc, "3.5  ZXBee协议封包设计", 2)
    add_para(doc, "通信外层采用课程规定的 ZXBee 帧格式：SOF + DST + SRC + CMD + LEN + PAYLOAD + CHK + EOF。其中 SOF 固定为 0xAA，EOF 固定为 0x55，CHK 为从 SOF 到 PAYLOAD 最后一个字节的累加低 8 位，PAYLOAD 使用项目统一 JSON 字段。")
    add_table(doc, ["字段", "含义", "本项目说明"], [
        ["DST/SRC", "目的/源地址", "协调器 0x0000，sensor-a/b/c 分别为 0x0001/0x0002/0x0003"],
        ["CMD=0x01", "普通上报", "温湿度、光照、门禁普通状态"],
        ["CMD=0x02", "安防事件", "徘徊、夜间入侵、门铃等事件"],
        ["CMD=0x03", "写命令", "远程开门、蜂鸣器、RGB 控制"],
        ["CMD=0x06", "复位命令", "解除告警并恢复安全状态"],
    ])

    add_heading(doc, "4  系统软件设计", 1)
    add_heading(doc, "4.1  CC2530嵌入式软件设计", 2)
    add_table(doc, ["工程", "关键文件", "核心功能"], [
        ["sensor-a", "Source/sensor.c", "环境数据读取、周期上报、字段 temp/humi/lux 封装"],
        ["sensor-b", "Source/sensor.c", "解析 unlock/buzz/rgb/reset，下发后控制继电器、蜂鸣器、RGB"],
        ["sensor-c", "Source/sensor.c、security_logic.c", "门禁状态检测、停留时长判断、夜间告警逻辑"],
        ["common", "zxbee.h/c、zxbee-inf.h/c", "ZXBee 封包/解析、校验和、ZigBee 收发接口"],
    ])
    add_heading(doc, "4.2  ZigBee组网通信设计", 2)
    add_para(doc, "协调器作为 ZigBee 网络中心节点，负责网络建立、终端节点入网和数据中转。sensor-a、sensor-b、sensor-c 加入网络后设置本地地址，并通过 ZXBeeInfSend() 将业务 JSON 数据封装为 ZXBee 帧发送给协调器。协调器再通过串口或智云平台通道将数据转发至上位机。")
    add_para(doc, "上行链路为“传感器节点 -> ZXBee 封包 -> ZigBee -> 协调器 -> 智云平台 -> Python/PyQt”；下行链路为“PyQt 按钮命令 -> 智云平台 -> 协调器 -> ZigBee -> 终端节点执行”。")
    add_heading(doc, "4.3  智云物联网平台配置", 2)
    add_para(doc, "上位机配置文件 config.py 中保存智云平台主机、应用 ID、密钥、WebSocket 地址和节点 MAC 映射。实际运行时，WebSocketClient 先向智云平台发送 authenticate 请求，认证成功后接收 sensor/message 数据包，并按节点 MAC 将旧字段 A0/A1/A2 映射为 temp/humi/lux 或 pir/tch/door 等项目字段。")
    add_heading(doc, "4.4  Python数据采集与解析", 2)
    add_para(doc, "Python 应用层对智云数据进行统一解析：优先解析 JSON 格式，同时兼容 {key=value} 和数组字段格式。解析后的数据更新 SensorState 内存状态，驱动门禁看板、环境曲线、事件时间线和数据库写入。远程控制命令由 PyQt 按钮触发，转换为 {unlock=1}、{buzz=500}、{rgb=[0,255,0]}、{reset=1} 等字段下发。")
    add_heading(doc, "4.5  PyQt5界面设计", 2)
    add_para(doc, "PyQt5 上位机采用深色主题，主界面包括标题栏、门禁状态看板、环境信息面板、远程控制区、系统诊断区、访客事件时间线和统计图表。界面可实时显示访客状态、门状态、门铃状态、安全模式、火焰/气体/光栅状态、温湿度、光照和告警等级。")
    add_picture(doc, clean_ui, 6.2)
    add_caption(doc, "图4-1 PyQt5 智能门禁访客管理系统主界面")
    add_heading(doc, "4.6  MySQL数据库设计", 2)
    add_para(doc, "数据库层支持 SQLite 与 MySQL 双模式，实际项目配置为 MySQL，数据库名为 door_access。主要数据表包括 access_log 和 door_env，分别保存访客事件与环境数据。程序为 create_time、event_type 等字段建立索引，以便按时间范围和事件类型查询。")
    add_table(doc, ["表名", "关键字段", "用途"], [
        ["access_log", "id、student_id、sensor、event_type、value、alert、is_remote_unlock、create_time", "保存门铃、PIR、开门、徘徊、入侵和远程开门等事件"],
        ["door_env", "id、student_id、temp、humi、lux、create_time", "保存门口温度、湿度和光照历史数据"],
    ])
    doc.add_page_break()
    add_heading(doc, "4.7  联动控制逻辑设计", 2)
    add_table(doc, ["触发条件", "系统判断", "联动动作"], [
        ["PIR 检测到有人靠近", "pir=1", "界面显示有人，事件时间线记录访客靠近"],
        ["有人停留超过 10 秒且未按门铃", "stay>10 且 tch=0", "alert=1，记录徘徊事件，提示注意"],
        ["夜间模式下门被打开", "night=1 且 door=1", "alert=2，触发蜂鸣器和红色 RGB 告警"],
        ["远程开门按钮", "unlock=1", "继电器打开，RGB 绿灯，3 秒后自动关门"],
        ["解除告警按钮", "reset=1", "蜂鸣器关闭，RGB 恢复安全状态，alert 归零"],
    ])

    add_heading(doc, "5  系统测试与分析", 1)
    add_heading(doc, "5.1  硬件测试", 2)
    add_table(doc, ["测试项", "测试过程", "测试结果"], [
        ["PIR 人体红外", "人员靠近/离开门口，读取 P0.0 状态并观察上报字段 pir", "无人为 0，有人为 1，状态变化可上报"],
        ["霍尔门磁", "开关门模拟磁铁靠近/远离，读取 door 字段", "关门为 0，开门为 1，符合门禁逻辑"],
        ["触摸/震动门铃", "触摸按键或敲击震动模块，观察 tch 字段和事件日志", "触发后生成门铃事件"],
        ["继电器/蜂鸣/RGB", "下发 unlock、buzz、rgb、reset 命令", "门锁、声音和灯光响应正常"],
    ])
    doc.add_page_break()
    add_heading(doc, "5.2  传输层测试", 2)
    add_table(doc, ["测试内容", "输入/操作", "预期与结果"], [
        ["环境数据上报", '{"temp":27.0,"humi":65.0,"lux":200}', "协调器可接收并转发，PyQt 环境面板显示对应数值"],
        ["门禁状态上报", '{"pir":1,"door":0,"tch":0,"alert":0}', "门禁看板状态更新，事件时间线记录"],
        ["远程开门下发", '{"unlock":1,"rgb":[0,255,0]}', "sensor-b 继电器开门，随后自动关门"],
        ["告警复位下发", '{"reset":1}', "sensor-b/sensor-c 清除告警状态"],
    ])
    add_heading(doc, "5.3  应用层测试", 2)
    add_para(doc, "应用层测试使用模拟数据和实际字段格式进行验证。模拟数据连续注入后，PyQt5 界面能够刷新门禁状态卡片、环境 LCD 数值、温度曲线和事件时间线；控制按钮能够生成符合通信接口说明的命令字典，并调用通信层发送。数据库层能够初始化 access_log、door_env 表，并完成事件和环境数据的插入与查询。")
    add_picture(doc, clean_ui, 6.2)
    add_caption(doc, "图5-1 应用层界面与模拟数据联调截图")
    add_heading(doc, "5.4  整体联调测试", 2)
    add_table(doc, ["步骤", "联调流程", "结果"], [
        ["1", "sensor-a 上传温湿光照数据", "PyQt 环境面板刷新，door_env 写入记录"],
        ["2", "sensor-c 检测访客靠近并上报 pir=1", "门禁看板显示有人，access_log 记录 PIR 事件"],
        ["3", "访客按门铃，上报 tch=1", "事件时间线显示门铃事件，蜂鸣器可短响"],
        ["4", "夜间模式下门磁触发 door=1", "系统生成 alert=2，触发声光告警"],
        ["5", "用户点击解除告警", "下发 reset=1，告警恢复为安全状态"],
    ])
    add_heading(doc, "5.5  测试结果分析", 2)
    add_para(doc, "测试结果表明，系统能够完成从硬件状态变化到上位机显示、数据库记录和远程控制反馈的端到端闭环。ZXBee 帧格式统一后，各模块字段保持一致，降低了 sensor-a、sensor-b、sensor-c 与 Python 应用层之间的对接成本。")
    add_para(doc, "目前系统已经覆盖课程要求的采集类、安防类和控制类设备，并实现云平台接入、可视化界面、数据库 CRUD 与告警联动。后续可进一步增加真实门锁机构、摄像头抓拍、人脸识别或移动端消息推送，以增强实际部署能力。")

    add_heading(doc, "6  总结与展望", 1)
    add_para(doc, "本项目完成了智能门禁访客管理系统的综合设计与实现。硬件层基于 CC2530 和多类传感器完成门禁感知与执行控制；传输层使用 ZigBee 与 ZXBee 协议实现节点间数据交互；应用层通过智云平台、Python/PyQt5 和数据库实现监控、控制、记录和统计分析。系统整体体现了物联网三层架构的完整链路。")
    add_para(doc, "不足之处在于当前测试仍以实验箱和模拟门锁为主，部分云平台数据和数据库运行环境依赖现场网络与本机配置。后续可在真实门禁场景中进行长时间稳定性测试，增加多用户权限管理、告警消息推送和数据备份机制，使系统更接近实际工程部署。")

    doc.save(OUT)


if __name__ == "__main__":
    build_doc()
    print(OUT)
