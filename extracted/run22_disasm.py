# -*- coding: utf-8 -*-
"""CD 충돌 상자의 실제 비교 명령을 확인한다."""
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]  # 같은 격리 분석 의존성을 읽는다.
sys.path.insert(0,str(root/'extracted/oracle-python'))
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
pe=pefile.PE(str(root/'originalCD/NETSTORM.EXE'))
md=Cs(CS_ARCH_X86,CS_MODE_32)  # 기계어를 실행하지 않는 읽기 전용 역어셈블러다.
# Bounds의 실제 알려진 몸체 범위만 읽는다.
for instruction in md.disasm(pe.get_data(0x444b50-pe.OPTIONAL_HEADER.ImageBase,0x230),0x444b50):
    print(f'{instruction.address:08x}: {instruction.mnemonic} {instruction.op_str}')
