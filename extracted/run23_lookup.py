# -*- coding: utf-8 -*-
"""투사체/충돌 효과와 사제의 실제 가상 삭제/피격 진입점을 읽는다."""
import re
from pathlib import Path
import pefile
root=Path(__file__).resolve().parents[1]  # 보호 PE/생성자 표를 읽기만 한다.
constructors=(root/'cpppj/src/o/TypeConstructors.inc').read_text(encoding='utf-8')
recipes=(root/'cpppj/src/o/DerivedConstructors.inc').read_text(encoding='utf-8')
# 같은 호스트에서 두 실제 판본의 현재 생성자/가상 표를 직접 조회한다.
for edition,table_name,recipe_name,name in (('originals','kPatchConstructors','kPatchRecipes','Netstorm.exe'),('originalCD','kCdConstructors','kCdRecipes','NETSTORM.EXE')):
    values=re.findall(r'0x([0-9a-fA-F]+)U',constructors.split(table_name,1)[1].split('};',1)[0])
    part=recipes.split(recipe_name,1)[1];pe=pefile.PE(str(root/edition/name))
    for type_number in (83,158,160):
        ctor=int(values[type_number],16)
        match=re.search(r'\{0x%08xU,0x([0-9a-fA-F]+)U'%ctor,part);table=int(match.group(1),16)
        print(edition,type_number,hex(table),[(hex(offset),hex(int.from_bytes(pe.get_data(table+offset-pe.OPTIONAL_HEADER.ImageBase,4),'little'))) for offset in (0x10,0x14,0x18,0x78,0x98)])
