#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""양판본 네 Form cdecl 생성자의 지정 물리 SID/단일 DWORD 저장을 격리 PE에서 관찰한다."""
import argparse
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP

ROOT = Path(__file__).resolve().parents[1]  # 보호 원본/새 기대값의 기준 경로다.
POOL, STACK, STOP = 0x12000000, 0x20000100, 0x30000000  # 서로 겹치지 않는 raw 풀/스택/정상 반환 주소다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/form-constructor-x86.tsv'  # 실제 원문56입력과 전후 슬롯/반환 포인터의 상대값이다.
EDITIONS = {
    'originals': ('originals/Netstorm.exe', 50, 0x5c8464, ((1, 0x4ae760), (2, 0x4ae3d0), (3, 0x4ae9f0), (6, 0x4aea40))),
    'originalCD': ('originalCD/NETSTORM.EXE', 36, 0x5395dc, ((1, 0x4af920), (2, 0x4af5c0), (3, 0x4afd60), (6, 0x4afe00)))
}  # 실제 함수 표의 주소이며 C++ 결과에서 얻은 값이 아니다.


class FormConstructorOracle:
    """실제 함수만 실행하고 목표 슬롯 밖 쓰기를 거부한다. 게임/창/할당 함수를 호출하지 않는다."""

    def __init__(self, path, stride, global_pool):
        """원본 코드와 충분한 물리 풀을 별도32비트 주소 공간에 설치한다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        pe = pefile.PE(str(ROOT / path))
        image = pe.get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(image) + 4095) & ~4095)
        self.mu.mem_write(0x400000, image)
        self.mu.mem_map(POOL, 0x400000)
        self.mu.mem_map(STACK & ~4095, 0x1000)
        self.mu.mem_map(STOP, 0x1000)
        self.mu.mem_write(global_pool, struct.pack('<I', POOL))
        self.stride, self.target, self.writes = stride, 0, []  # 현재 원문 폭과 허용할 목표 저장이다.
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def write(self, mu, access, address, size, value, data):
        """원문이 실제로 실행한 한DWORD 저장의 주소/폭/값만 수집한다."""
        assert address == self.target and size == 4
        self.writes.append((address - POOL, size, value & 0xffffffff))

    def observe(self, entry, sid, before):
        """첫 인자 뒤의 원문 caller 잔여 인자도 보존하여 단일 저장/정상 cdecl 반환을 읽는다."""
        self.target = POOL + sid * self.stride
        self.mu.mem_write(self.target, before)
        arguments = struct.pack('<4I', STOP, sid, 0, 0xabcdef01)
        self.mu.mem_write(STACK, arguments)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.writes = []
        self.mu.emu_start(entry, STOP, count=1000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert bytes(self.mu.mem_read(STACK, len(arguments))) == arguments
        assert len(self.writes) == 1 and self.mu.reg_read(UC_X86_REG_EAX) == self.target
        after = bytes(self.mu.mem_read(self.target, self.stride))
        assert after[4:] == before[4:]
        return after.hex(), self.mu.reg_read(UC_X86_REG_EAX) - POOL


def main():
    """일반/예약/0/경계 SID와 free/dead/타입 payload를 교차하여 새 파일 또는 재관측을 만든다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    rows = []  # 원문 입력/관측을 그대로 보존한다.
    # 각 판본의 네 실제 생성 함수를 개별 실행한다.
    for edition, (path, stride, global_pool, constructors) in EDITIONS.items():
        oracle = FormConstructorOracle(path, stride, global_pool)
        for kind, entry in constructors:
            for sid in (0, 4, 5, 50, 23001, 32767, 65534):
                before = bytearray((index * 29 + kind * 7) & 255 for index in range(stride))
                before[10], before[11] = 74, sid % 4
                after, returned = oracle.observe(entry, sid, bytes(before))
                rows.append((edition, kind, sid, before.hex(), after, returned))
    content = '# edition type sid nativeBefore nativeAfter nativeReturnOffset; 실제 PE 원문56행/게임0\n' + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8') == content
    else:
        if FIXTURE.exists():
            raise RuntimeError('기존 fixture 덮어쓰기 거부; --verify로 재관측한다.')
        FIXTURE.write_text(content, encoding='utf-8', newline='\n')
    print(json.dumps({'observations': len(rows), 'reobserved': args.verify, 'gameExecutions': 0}, ensure_ascii=False))


if __name__ == '__main__':
    main()
