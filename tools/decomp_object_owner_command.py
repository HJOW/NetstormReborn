#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문 SetOwner 기본블록과4d5290을 실행하며 가상74/Core 선택만 명시 경계로 분리한다."""
import argparse
import hashlib
import itertools
import json
import socket
import struct
from decomp_oracle import Oracle, ROOT, SCRATCH, STACK, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP

# 격리된 실제50BYTE 슬롯과 가상74 포인터 표다. 원본 프로세스/장치/창은 실행하지 않는다.
POOL, SID, TABLE = 0x12000000, 50, SCRATCH + 0x1000
# 보호 PE의 실제 SetOwner 토큰이다.
COMMAND = 0x020000cc


class OwnerOracle(Oracle):
    """4d8176 비교부터4d81cb 분기까지와 실제4d5290의 원문을 실행한다."""
    def __init__(self):
        """원래 함수의 허용 범위를 빌리되 선택된 기본블록 밖에서는 즉시 멈춘다."""
        super().__init__('originals', [dict(name='Owner', originals='004d62b0')], [0x4d5290])
        self.mu.mem_map(POOL, 0x1000)
        self.trace = []  # 선택된 외부 효과의 인자/순서다.
        self.mutation = 0  # 가상74가 변경하는 현재 전역의 명시 입력이다.

    def finish(self):
        """cdecl Core 선택 또는 ret4 가상74의 실제 복귀 폭을 구별한다."""
        stack = self.mu.reg_read(UC_X86_REG_ESP)
        address = self.mu.reg_read(UC_X86_REG_EIP)
        target = struct.unpack('<I', self.mu.mem_read(stack, 4))[0]
        self.mu.reg_write(UC_X86_REG_ESP, stack + (8 if address == 0x4adf00 else 4))
        self.mu.reg_write(UC_X86_REG_EIP, target)

    def on_instruction(self, mu, address, size, data):
        """계산은 원문으로 실행하고 선택한 가상효과와 종료 경계만 가로챈다."""
        if address in (0x4d8bda, 0x4d81d0):
            mu.emu_stop();return
        stack = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4adf00:
            owner = struct.unpack('<I', mu.mem_read(stack + 4, 4))[0]
            sid = (mu.reg_read(UC_X86_REG_ECX) - POOL) // 50
            self.trace.append(f'O,{sid},{owner}')
            mu.mem_write(POOL + sid * 50 + 34, bytes([owner & 255]))
            if self.mutation & 1:
                old = struct.unpack('<I', mu.mem_read(0x5c85a4, 4))[0]
                mu.mem_write(0x5c85a4, struct.pack('<I', 1 - old))
            if self.mutation & 2:
                mu.mem_write(0x5caea0, struct.pack('<I', 51))
                mu.mem_write(POOL + sid * 50 + 34, b'\x06')
            self.finish();return
        if address == 0x48f3b0:
            owner, template = struct.unpack('<II', mu.mem_read(stack + 4, 8))
            self.trace.append(f'C,{owner},{template}');self.finish();return
        return super().on_instruction(mu, address, size, data)

    def observe(self, editor, mission, selected, owner, mutation, revision):
        """현재 상태/레지스터를 명시 입력으로 설치하며 출력은 x86 뒤에서만 읽는다."""
        self.trace, self.mutation = [], mutation
        self.mu.mem_write(POOL, bytes(0x1000))
        self.mu.mem_write(POOL + SID * 50, struct.pack('<I', TABLE) + bytes(46))
        self.mu.mem_write(POOL + SID * 50 + 34, b'\x02')
        self.mu.mem_write(TABLE + 0x74, struct.pack('<I', 0x4adf00))
        for address, value in ((0x5c8464, POOL), (0x5c85a4, editor), (0x594fa4, mission),
                               (0x5caea0, selected), (0x542a24, COMMAND), (0x59a8b0, revision)):
            # BSS/정적 전역의 현재 DWORD만 설치한다.
            self.mu.mem_write(address, struct.pack('<I', value))
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_EAX, owner)
        self.mu.reg_write(UC_X86_REG_EBP, COMMAND)
        self.mu.reg_write(UC_X86_REG_ESI, 0x76543210)
        self.mu.emu_start(0x4d8176, STOP, count=3000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == 0x4d8bda
        values = [struct.unpack('<I', self.mu.mem_read(a, 4))[0] for a in (0x59a8b0, 0x5c85a4, 0x5caea0)]
        values.append(self.mu.mem_read(POOL + SID * 50 + 34, 1)[0])
        return ';'.join(self.trace) or '-', ','.join(map(str, values))


def main():
    """새 fixture 또는 기존 fixture의 바이트 재현 일치만 기록한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    oracle = OwnerOracle()
    lines = ['# editor mission selected owner mutation revision trace state(revision,editor,selected,ownerByte)']
    for case in itertools.product((0, 1), (0, 1), (0, SID), (0, 1, 8, 0x12345678), range(4), (0, 0xffffffff)):
        # 가상74 뒤 Editor/selected/owner 변경과 revision 감김을 독립 입력으로 교차한다.
        trace, state = oracle.observe(*case)
        lines.append('\t'.join([*(str(x) for x in case), trace, state]))
    content = ('\n'.join(lines) + '\n').encode('utf-8')
    fixture = ROOT / 'cpppj/tests/fixtures/object-owner-command-x86.tsv'
    if args.verify:
        assert fixture.read_bytes() == content
    else:
        assert not fixture.exists();fixture.write_bytes(content)
    record = dict(host=socket.gethostname(),date='2026-10-28',lastCompletedDecompile='HJOW-Athlon / 2026-10-23',decompileChanged=False,
                  uniqueInputs=len(lines) - 1, verify=args.verify, scope='4d8176..4d81cb 원문 기본블록과4d5290. 가상74/Core 선택은 명시 외부 경계. 일반 입력 전체와 게임 실행 증거가 아니다.',
                  fixtureSha256=hashlib.sha256(content).hexdigest(), originalSha256=hashlib.sha256((ROOT/'originals/Netstorm.exe').read_bytes()).hexdigest())
    (ROOT/'cpppj/object-owner-command-native-20261028.json').write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n',encoding='utf-8')
    print(json.dumps(record, ensure_ascii=False))


if __name__ == '__main__':
    main()
