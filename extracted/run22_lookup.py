# -*- coding: utf-8 -*-
"""기존 생성자 표와 보호 PE의 썬 아처 포구 가상 함수 주소를 읽는다."""
import re
from pathlib import Path
import pefile
root=Path(__file__).resolve().parents[1]  # 같은 작업공간의 읽기 전용 입력이다.
text=(root/'cpppj/src/o/TypeConstructors.inc').read_text(encoding='utf-8')
recipes=(root/'cpppj/src/o/DerivedConstructors.inc').read_text(encoding='utf-8')
# 두 판본의 실제71 생성자와 가상표+b4를 읽는다.
for edition,table_name,recipe_name,name in (('originals','kPatchConstructors','kPatchRecipes','Netstorm.exe'),('originalCD','kCdConstructors','kCdRecipes','NETSTORM.EXE')):
    values=re.findall(r'0x([0-9a-fA-F]+)U',text.split(table_name,1)[1].split('};',1)[0])
    ctor=int(values[71],16)
    part=recipes.split(recipe_name,1)[1]
    match=re.search(r'\{0x%08xU,0x([0-9a-fA-F]+)U'%ctor,part)
    table=int(match.group(1),16)
    pe=pefile.PE(str(root/edition/name))
    entry=int.from_bytes(pe.get_data(table+0xb4-pe.OPTIONAL_HEADER.ImageBase,4),'little')
    print(edition,hex(ctor),hex(table),hex(entry))
