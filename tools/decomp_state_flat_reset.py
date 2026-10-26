#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PATCH의 PlayerNoMake255DWORD·spot65536BYTE 정리를 제품 변경 전에 실제 CPU로 고정한다."""
import argparse
import csv
import itertools
import json
import socket
import zlib
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

# 두 실제 함수·대상 주소·바이트 크기이며 CD의 자동 대응 주소는 사용하지 않는다.
SPECS = {'warnings': (0x4328a0, 0x54d4e8, 1020), 'spots': (0x4ab940, POOL + 256, 65536)}
FIXTURE = ROOT / 'cpppj/tests/fixtures/state-flat-reset-x86.tsv'  # 제품 결과로 덮어쓰지 않는 최초 독립 관측이다.
REPORT = ROOT / 'cpppj/state-flat-reset-native-20261026.json'  # 현재 호스트와 실제 PE/함수 SHA를 남긴다.


class Oracle(MemoryOracle):
    """두 작은 실제 몸체만 실행하고 모든 CPU store의 주소·폭·순서를 감사한다."""
    def __init__(self):
        """보호 PE 사본과 가상 Pool에 쓰기 감사를 붙이며 원본 파일에는 쓰지 않는다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.ranges = {entry: (entry, entry + int(facts[entry]['size'])) for entry, _, _ in SPECS.values()}
        self.running = False
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def instruction(self, mu, address, size, data):
        """허용 몸체 이외의 함수·OS·명시 효과 치환을 모두 거부한다."""
        self.instructions += 1
        low, high = self.ranges[self.entry]
        if not low <= address < high or self.instructions > 70000:
            raise RuntimeError('작은 정리의 실행 범위 이탈: ' + hex(address))

    def write(self, mu, access, address, size, value, data):
        """스택 저장을 제외한 실제 순차 DWORD0 쓰기를 한 개씩 대조한다."""
        if not self.running or STACK - 4096 <= address < STACK + 64:
            return
        if address != self.target + self.stores * 4 or size != 4 or value != 0:
            raise RuntimeError('작은 정리의 예상 밖 CPU store: ' + str((address, size, value)))
        self.stores += 1

    def observe(self, kind, seed, control):
        """명시 비영 입력과 인접32BYTE를 둔 뒤 실제 CPU의 전체 결과와 FPU 보존을 읽는다."""
        self.entry, self.target, length = SPECS[kind]
        before = bytes((seed + index * 17) & 255 for index in range(length + 32))
        self.mu.mem_write(self.target - 16, before)
        if kind == 'spots':
            self.put(0x5c7c44, self.target)
        self.mu.mem_write(STACK - 4096, b'\xcd' * (4096 + 64))
        self.put(STACK, STOP)
        saved = ((UC_X86_REG_EBX, 0x456789ab), (UC_X86_REG_ESI, 0x56789abc), (UC_X86_REG_EDI, 0x6789abcd), (UC_X86_REG_EBP, 0x789abcde))
        for register, value in ((UC_X86_REG_EAX, 0x89abcdef), (UC_X86_REG_ECX, 0x12345678), (UC_X86_REG_EDX, 0x3456789a), *saved):  # 호출 규약의 비영 초기값을 명시한다.
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.stores = self.instructions = 0
        self.running = True
        try:
            self.mu.emu_start(self.entry, STOP)
        finally:
            self.running = False
        after = bytes(self.mu.mem_read(self.target - 16, length + 32))
        if self.stores != length // 4 or self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError('작은 정리의 store 수/복귀 차이')
        for register, value in saved:  # 비휘발 레지스터 보존을 전체 호출 뒤 확인한다.
            if self.mu.reg_read(register) != value:
                raise RuntimeError('작은 정리의 비휘발 레지스터 차이')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) != 0 or self.mu.reg_read(UC_X86_REG_FPTAG) != 0xffff:
            raise RuntimeError('작은 정리의 FPU 보존 차이')
        if after[:16] != before[:16] or after[-16:] != before[-16:]:
            raise RuntimeError('작은 정리의 인접 바이트 변경')
        return [kind, seed, control, length, zlib.adler32(before), zlib.adler32(after), (after[:16] + after[-16:]).hex(), self.stores, self.instructions]


def main():
    """최초 관측과 저장된 근거 검증을 구별하며 게임 창을 실행하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for name, wanted in report['sha256'].items():  # 원문·관측 도구·최초 fixture의 바이트를 모두 대조한다.
            if digest(ROOT / name) != wanted:
                raise RuntimeError('작은 정리의 최초 근거 SHA 차이: ' + name)
        print(json.dumps({'verified': True, 'observations': report['observations']}))
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('최초 관측을 덮어쓸 수 없음')
    oracle = Oracle()
    rows = [oracle.observe(kind, seed, control) for kind, seed, control in itertools.product(SPECS, (0, 17, 131, 255), (0x27f, 0x37f))]
    FIXTURE.write_bytes(('# kind / seed / control / bytes / inputAdler / outputAdler / adjacent32Hex / stores / instructions\n' + ''.join('\t'.join(map(str, row)) + '\n' for row in rows)).encode('utf-8'))
    names = ['originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_state_flat_reset.py', FIXTURE.relative_to(ROOT).as_posix()]
    report = {'host': socket.gethostname(), 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23', 'edition': 'PATCH 10.78', 'observations': len(rows), 'effectsStubbed': False, 'originalGameExecuted': False, 'functions': {kind: {'entry': hex(entry), 'bytes': high - low, 'bodyHex': bytes(oracle.mu.mem_read(entry, high - low)).hex()} for kind, (entry, _, _) in SPECS.items() for low, high in [oracle.ranges[entry]]}, 'sha256': {name: digest(ROOT / name) for name in names}, 'cdNote': '자동 대응표의00487900은 CD에서65536BYTE spot 정리다. PATCH의255DWORD PlayerNoMake 정리와 동일 함수로 취급하지 않는다.'}
    REPORT.write_bytes((json.dumps(report, ensure_ascii=False, indent=2) + '\n').encode('utf-8'))
    print(json.dumps({'observations': len(rows), 'host': report['host'], 'passed': True}))


if __name__ == '__main__':  # 명시 실행한 독립 관측에서만 PE 사본의 CPU 명령을 실행한다.
    main()
