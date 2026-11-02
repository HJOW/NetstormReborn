#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""00436450 전체 부모의 캡처/다섯 진단/Present/snapshot 순서를 실제 원문에서 분리 관측한다."""
from pathlib import Path
import argparse
import json
import socket
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]  # 같은 보호 PE와 정밀 디컴파일을 읽기만 한다.
WORK = None  # 새 --output 디렉터리를 지정하기 전에는 파일을 작성하지 않는다.
sys.path.insert(0, str(ROOT / 'tools'))
from decomp_mission_abort import Oracle as BaseOracle, POOL
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

VIEW = POOL + 0x90000  # 현재 Renderer의 실제 앞64BYTE를 공급하는 독립 주소다.


class Oracle(BaseOracle):
    """부모의 분기/현재 active 조회/실제16DWORD 복사는 실행하고 하위 표시 장치만 기록한다."""
    def __init__(self):
        """실제 부모만 허용하며 하위 DrawWater 전체는 별도 관측에 남긴다."""
        super().__init__()
        self.mu.mem_map(0, 0x1000)
        self.allowed = []
        ranges(self, (0x436450,))

    def guard(self, mu, access, address, size, value, data):
        """SEH/시각 stamp/snapshot의 실제 쓰기만 기존 범위에 더한다."""
        if address == 0 and size == 4 or 0x54de80 <= address and address + size <= 0x54de88 or 0x59a900 <= address and address + size <= 0x59a940:
            return
        super().guard(mu, access, address, size, value, data)

    def instruction(self, mu, address, size, data):
        """하위 경계의 실제 호출 순서와 마지막 현재 view 재조회를 기록한다."""
        names = {0x4977e0: 'MarkDirty', 0x4994b0: 'Draw', 0x4db230: 'Capture', 0x434410: 'HashDebug', 0x434880: 'FireDebug', 0x434a00: 'SpotDebug', 0x434dd0: 'DrawWater', 0x434bf0: 'BoardDebug', 0x499fe0: 'Present'}
        if address in names:
            name = names[address]
            self.events.append(name)
            if name == 'Draw' and self.mutation:
                self.put(0x5df828, 1)
            if name == 'DrawWater' and self.mutation:
                self.put(VIEW + 20, 777)
            self.ret()
            return
        if address == 0x460de0:
            self.ret(1)
            return
        if address == 0x460d70:
            destination = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 1)[0]
            mu.mem_write(destination, struct.pack('<d', 1.25))
            self.ret(destination)
            return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'미명시 프레임 부모 {address:08x}')

    def sample(self, case):
        """다른 화면/캡처 active와 하위 변이를 독립 입력으로 공급한다."""
        visible, flags, capture, self.mutation = case
        self.events = []
        self.put(0, 0)
        self.put(0x54dc50, visible)
        self.put(0x5c78d4, flags)
        self.put(0x5df828, capture)
        self.put(0x59a8bc, VIEW)
        self.mu.mem_write(VIEW, struct.pack('<16I', *range(100, 116)))
        self.mu.mem_write(0x59a900, bytes([0x44]) * 64)
        self.call(0x436450, [])
        return dict(input=case, effects=self.events, snapshot=list(self.words(0x59a900, 16)), capture=self.words(0x5df828, 1)[0])


def main():
    """순서는 실제 전체 부모에서 읽고 제품/기존 fixture는 수정하지 않는다."""
    global WORK
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",type=Path,required=True,help="기존 파일이 없는 새 원시 디렉터리")
    WORK=parser.parse_args().output.resolve()
    if WORK.exists(): raise FileExistsError(WORK)
    (WORK/"modified/cpppj/tests/fixtures").mkdir(parents=True)
    target = WORK / 'native-frame-order.json'
    assert not target.exists()
    oracle = Oracle()
    cases = [(0, 1, 0, 0), (1, 2, 0, 0), (1, 2, 1, 0), (1, 2, 0, 1), (1, 2, 0xffffffff, 0)]
    rows = []
    for case in cases:  #같은 입력의 실제 부모/현재16DWORD 결과가 반복에서 일치해야 한다.
        first = oracle.sample(case)
        assert first == oracle.sample(case)
        rows.append(first)
    record = dict(unit=768, host=socket.gethostname(), inputs=len(cases), observations=len(cases) * 2, actual='00436450 branch/current active/ordered calls/16DWORD snapshot copy', boundary='wall clock/pause and all child draw/present bodies are explicit boundaries, DrawWater whole body has separate exact Native coverage; not actual whole-frame/OS rendering', gameExecutions=0, multiplayerTests=0, builds=0, rows=rows)
    target.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', 'utf-8')
    print(json.dumps(dict(inputs=len(cases), observations=len(cases) * 2)))


if __name__ == '__main__':
    main()
