from pathlib import Path
import win32com.client

ROOT = Path(r"F:\WLWKFSX\QimoProject")
SRC = ROOT / "期末综合项目_智能门禁访客管理系统_完成版.doc"
OUT = ROOT / "_working_report_base.docx"

word = win32com.client.Dispatch("Word.Application")
word.Visible = False

doc = None
for item in word.Documents:
    try:
        if Path(item.FullName).resolve() == SRC.resolve():
            doc = item
            break
    except Exception:
        pass

if doc is None:
    doc = word.Documents.Open(str(SRC), ReadOnly=True)

doc.SaveAs2(str(OUT), FileFormat=16)
print(OUT)
