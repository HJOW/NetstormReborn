# -*- coding: utf-8 -*-
"""재시드 하위 몸체의 실제 CRT/time 호출 주소를 읽는다."""
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]  # 보호 PE를 읽기만 하는 같은 저장소다.
sys.path.insert(0,str(root/'extracted/oracle-python'))
import pefile
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
pe=pefile.PE(str(root/'originals/Netstorm.exe'))
# 정확 알려진 재시드 함수의 call 명령만 관찰한다.
for instruction in Cs(CS_ARCH_X86,CS_MODE_32).disasm(pe.get_data(0x455970-pe.OPTIONAL_HEADER.ImageBase,0x60),0x455970):
    if instruction.mnemonic=='call':print(hex(instruction.address),instruction.op_str)
# CRT 두 함수의 실제 TLS 요청과 상태 필드 위치도 읽는다.
for instruction in Cs(CS_ARCH_X86,CS_MODE_32).disasm(pe.get_data(0x4e649e-pe.OPTIONAL_HEADER.ImageBase,0x31),0x4e649e):
    print(hex(instruction.address),instruction.mnemonic,instruction.op_str)
