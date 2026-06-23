from pathlib import Path
import shutil

import fitz
import win32com.client

ROOT = Path(r"F:\WLWKFSX\QimoProject")
DOCX = ROOT / "期末综合项目_智能门禁访客管理系统_完成版_扩充版.docx"
PDF = ROOT / "render_expanded" / "expanded_report.pdf"
OUT_DIR = ROOT / "render_expanded"
OUT_DIR.mkdir(exist_ok=True)

word = win32com.client.Dispatch("Word.Application")
word.Visible = False
word.DisplayAlerts = 0
doc = word.Documents.Open(str(DOCX), ReadOnly=True)
doc.ExportAsFixedFormat(str(PDF), 17)
doc.Close(False)

pdf = fitz.open(str(PDF))
for page_index, page in enumerate(pdf, start=1):
    pix = page.get_pixmap(matrix=fitz.Matrix(1.8, 1.8), alpha=False)
    pix.save(str(OUT_DIR / f"page-{page_index:02d}.png"))
print(PDF)
print(len(pdf))
