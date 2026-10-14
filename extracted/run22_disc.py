# -*- coding: utf-8 -*-
"""썬 디스크 실제 가상 Handle/Pre/Post 주소를 보호 PE에서 읽는다."""
import re
from pathlib import Path
import pefile
root=Path(__file__).resolve().parents[1]  # 같은 저장소의 읽기 전용 자료다.
text=(root/'cpppj/src/o/TypeConstructors.inc').read_text(encoding='utf-8')
recipes=(root/'cpppj/src/o/DerivedConstructors.inc').read_text(encoding='utf-8')
# 두 실제 판본의 타입83 가상표를 직접 조회한다.
for edition,table_name,recipe_name,name in (('originals','kPatchConstructors','kPatchRecipes','Netstorm.exe'),('originalCD','kCdConstructors','kCdRecipes','NETSTORM.EXE')):
    values=re.findall(r'0x([0-9a-fA-F]+)U',text.split(table_name,1)[1].split('};',1)[0])
    ctor=int(values[83],16)
    part=recipes.split(recipe_name,1)[1]
    match=re.search(r'\{0x%08xU,0x([0-9a-fA-F]+)U'%ctor,part)
    table=int(match.group(1),16)
    pe=pefile.PE(str(root/edition/name))
    print(edition,hex(ctor),hex(table),[(hex(offset),hex(int.from_bytes(pe.get_data(table+offset-pe.OPTIONAL_HEADER.ImageBase,4),'little'))) for offset in (0x5c,0x64,0x68)])
