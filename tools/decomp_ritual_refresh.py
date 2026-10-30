#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문449d98~449dc5의 세 호출과 사용하지 않는 두 번째 owner 읽기만 관측한다."""
import argparse
import csv
import hashlib
import json
import socket
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, STACK
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

OWNER = 0x540cac  # 원문의 현재 Template owner DWORD다.
BASE = 0x5c89f0  # 현재 Template9 배열의 원래32비트 주소다.
BEGIN, END = 0x449d98, 0x449dc5  # 상위 owner/mission 조건 뒤의 로컬 분기 내부만 실행한다.


class ObservationStop(Exception):
    """지정한 외부 효과/현재 owner 읽기 진입에서 원래 부분효과를 보존한다."""


class Oracle(MemoryOracle):
    """기존 PE를 실행하되 큰 Core/저장 자식은 독립 명시 경계로 남긴다."""
    def __init__(self):
        """현재 전체 정밀 범위에 실제 관측 구간이 포함됐는지 확인한다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            row = next(row for row in csv.DictReader(stream, delimiter='\t') if int(row['entry'], 16) == 0x449b30)
        ranges = [tuple(int(value, 16) for value in part.split('-')) for part in row['ranges'].split(';')]
        assert any(low <= BEGIN < END <= high + 1 for low, high in ranges)
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read_owner, begin=OWNER, end=OWNER + 3)

    def event(self, text):
        """효과보다 먼저 원문의 현재 진입을 기록한다."""
        self.trace.append(text)
        if self.stop and len(self.trace) == self.stop: raise ObservationStop()

    def read_owner(self, mu, access, address, size, value, data):
        """외부 자식이 바꾼 현재 DWORD의 실제 두 CPU load만 기록한다."""
        assert address == OWNER and size == 4
        self.event('O,' + str(self.words(OWNER, 1)[0]))

    def instruction(self, mu, address, size, data):
        """실제 세 call과 LEA/load는 CPU에 맡기며 자식 반환만 공급한다."""
        if address == 0x44fc00:
            self.event('C'); self.core = True; self.put(OWNER, self.first); self.ret()
        elif address == 0x4beb30:
            pointer = mu.reg_read(UC_X86_REG_ECX); delta = pointer - BASE
            assert delta % 20 == 0 and 0 <= delta <= 160
            owner = delta // 20; self.event('D,' + str(owner)); self.written = owner; self.put(OWNER, self.second); self.ret()
        elif address == 0x4bfac0: self.event('S'); self.snapshot = True; self.ret()
        elif not BEGIN <= address < END: raise RuntimeError(f'검토하지 않은 로컬 완료 명령 {address:08x}')

    def run(self, first, second, stop):
        """상위 의식/포획 없이 분기 내부의 독립 caller 입력과 반환만 사용한다."""
        self.first, self.second, self.stop = first, second, stop
        self.trace = []; self.core = False; self.written = None; self.snapshot = False  # 앞 사례의 부분효과를 섞지 않는다.
        self.put(OWNER, 7); self.mu.reg_write(UC_X86_REG_ESP, STACK); completed = True
        try: self.mu.emu_start(BEGIN, END, count=1000)
        except ObservationStop: completed = False
        return [str(first), str(second), str(stop), str(int(completed)), ';'.join(self.trace), str(self.words(OWNER, 1)[0]), str(int(self.core)), '-' if self.written is None else str(self.written), str(int(self.snapshot))]


def sha(path):
    """보호 입력/최초 관측의 전체 바이트를 읽기만 한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """두 독립 emulation을 비교하고 최초 fixture는 기존 파일을 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--output', type=Path, required=True); args = parser.parse_args()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    inputs = [(first, second, stop) for first in (0, 1, 8) for second in (0, 2, 8) for stop in range(6)]  # 정상/현재 변경/각 효과 중단의 명시 입력54개다.
    oracle = Oracle(); first = [oracle.run(*values) for values in inputs]
    replay = Oracle(); repeated = [replay.run(*values) for values in inputs]
    for name, rows in (('first.json', first), ('replay.json', repeated)): (output / name).write_text(json.dumps(rows, indent=2) + '\n', encoding='utf-8', newline='\n')  # 독립 관측 둘을 보존한다.
    assert first == repeated
    fixture = ROOT / 'cpppj/tests/fixtures/ritual-refresh-x86.tsv'
    with fixture.open('x', encoding='utf-8', newline='\n') as stream:
        stream.write('# Core뒤owner/CoreData뒤owner/중단/완료/순서/최종owner/Core적용/기록owner/snapshot적용\n')
        stream.writelines('\t'.join(row) + '\n' for row in first)
    sources = ['originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'extracted/refined/originals/Netstorm.c', 'tools/decomp_priestmenu.py', 'tools/decomp_ritual_refresh.py', 'cpppj/tests/fixtures/ritual-refresh-x86.tsv']
    report = {'host': socket.gethostname(), 'date': '2026-10-30', 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23', 'cases': len(first), 'observations': len(first) * 2, 'differences': [], 'entryRange': ['00449d98', '00449dc5'], 'parent': '00449b30', 'gameStarted': False,
              'scope': '상위 missionPost/owner 조건 뒤 분기 내부만 실제 CPU. Core/CoreData/snapshot은 명시 외부 자식. 의식/포획/희생/공격/승리/MP 실행 없음.', 'sourceSha256': {name: sha(ROOT / name) for name in sources}, 'evidenceSha256': {path.name: sha(path) for path in output.iterdir() if path.is_file()}}
    (ROOT / 'cpppj/ritual-refresh-native-20261030.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'cases': len(first), 'observations': len(first) * 2, 'differences': 0}))


if __name__ == '__main__':
    main()
