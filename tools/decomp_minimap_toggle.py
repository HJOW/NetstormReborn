#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78의480810을 격리 CPU에서 실행해 현재 표시·생성·Kill·부착·갱신 순서를 기록한다."""
import argparse
import itertools
import json
from pathlib import Path
from decomp_cursor_lifecycle import Oracle as BaseOracle, POOL, VTABLE, STACK, STOP, ROOT, digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_ECX

# 실제 미니맵 전역과 격리된 가상 Attach 경계다.
CURRENT, REFRESH, PARENT, ATTACH = 0x5949d0, 0x5949d4, 0x563e38, STOP + 0x400


class Probe(BaseOracle):
    """전환 본체만 실행하고 메모리 할당·생성자·Kill·가상 부착은 명시 효과로 관측한다."""
    def __init__(self):
        """기존 보호 PE·격리 SEH/스택을 재사용하고 이 함수의 정확한 코드 범위만 허용한다."""
        super().__init__()
        self.allowed = []
        ranges(self, (0x480810,))
        self.events = []
        self.mutate = False
        self.instructions = 0

    def instruction(self, mu, address, size, data):
        """각 원문 호출의 실제 this/스택 인자를 읽으며 전환 분기는 대신 계산하지 않는다."""
        esp, this = mu.reg_read(UC_X86_REG_ESP), mu.reg_read(UC_X86_REG_ECX)
        if address == 0x466480:
            self.event('kill', this)
            if self.mutate:
                self.put(CURRENT, POOL + 0x200)
            self.ret();return
        if address == 0x4e4391:
            self.event('new', self.get(esp + 4));self.ret(POOL + 0x100);return
        if address == 0x4801b0:
            self.event('construct', this)
            self.put(this, VTABLE);self.put(VTABLE + 0x20, ATTACH)
            if self.mutate:
                self.put(PARENT, POOL + 0x300)
            self.ret(this);return
        if address == ATTACH:
            self.event('attach', this, *(self.get(esp + 4 + i * 4) for i in range(4)))
            self.ret(0, 16);return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'미니맵 전환 코드 범위 밖 {address:08x}')
        self.instructions += 1

    def write(self, mu, access, address, size, value, data):
        """격리 SEH·스택·노드와 세 실제 전역 이외에는 쓰기를 허용하지 않는다."""
        allowed = ((0, 4), (STACK - 0x7f000, STACK + 0x1000), (POOL, POOL + 0x10000),
                   (VTABLE, VTABLE + 0x1000), (CURRENT, REFRESH + 4), (PARENT, PARENT + 4))
        if not any(low <= address and address + size <= high for low, high in allowed):
            raise RuntimeError(f'미니맵 전환 쓰기 범위 밖 {address:08x}')


def main():
    """기존 fixture를 덮어쓰지 않고 두 x87 정밀도의 독립 현재/갱신 입력 결과만 저장한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    probe, rows = Probe(), []
    for present, refresh, mutate in itertools.product((False, True), (0, 7, 0xffffffff), (False, True)):
        # 콜백이 바꾼 현재 포인터/부모도 원문이 나중에 읽는지 두 정밀도에서 관측한다.
        for control in (0x27f, 0x37f):
            probe.events = [];probe.instructions = 0;probe.mutate = mutate
            probe.put(0, 0xdeadca11);probe.put(CURRENT, POOL if present else 0)
            probe.put(REFRESH, refresh);probe.put(PARENT, POOL + 0x400)
            probe.call(0x480810, 0, b'', 0, control)
            assert probe.get(0) == 0xdeadca11
            rows.append(dict(present=present,refreshBefore=refresh,mutate=mutate,control=control,
                currentAfter=probe.get(CURRENT),refreshAfter=probe.get(REFRESH),events=probe.events,
                instructions=probe.instructions))
    result = dict(uniqueInputs=12,executions=len(rows),originalLiveGames=0,observations=rows,
                  boundaries=['466480 Kill', '4e4391 new', '4801b0 constructor', 'virtual+20 Attach'],
                  sha256={name:digest(ROOT / name) for name in ('originals/Netstorm.exe','tools/decomp_minimap_toggle.py')})
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(result, stream, ensure_ascii=False, indent=2);stream.write('\n')
    print(f'미니맵 전환 원문 격리 관측 {len(rows)}개 완료')


if __name__ == '__main__':
    main()
