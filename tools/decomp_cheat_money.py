#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문 일반 명령의 Editor/mission 관문과 signed 정수→float SP 인수를 관측한다."""
import argparse
import csv
import hashlib
import json
import socket
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, STACK
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

BEGIN, END = 0x4d7b26, 0x4d8bda  # 동일54287c 명령 비교 뒤의 분기와 전체 일반 처리의 공통 tail이다.
EDITOR, MISSION = 0x5c85a4, 0x594fa4  # 원래 short circuit 순서로 읽는 두 DWORD다.


class ObservationStop(Exception):
    """선택한 현재 load/가산 진입 뒤 원문의 부분효과를 보존한다."""


class Oracle(MemoryOracle):
    """실제 기계어의 관문/변환만 실행하며 큰 가산 자식은 명시 경계다."""
    def __init__(self):
        """현재 전체 정밀 소스의 실제 함수 범위를 확인한다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            row = next(row for row in csv.DictReader(stream, delimiter='\t') if int(row['entry'], 16) == 0x4d62b0)
        ranges = [tuple(int(value, 16) for value in part.split('-')) for part in row['ranges'].split(';')]  # 원문이 나뉜 모든 구간을 보존한다.
        assert any(low <= BEGIN < high + 1 for low, high in ranges)
        for address in (EDITOR, MISSION): self.mu.hook_add(UC_HOOK_MEM_READ, self.read_global, begin=address, end=address + 3)  # 실제 선택된 load만 관측한다.

    def event(self, text):
        """효과보다 먼저 원래 진입을 기록한다."""
        self.trace.append(text)
        if self.stop and len(self.trace) == self.stop: raise ObservationStop()

    def read_global(self, mu, access, address, size, value, data):
        """같은 현재 DWORD를 읽으며 getter 자체를 기대 계산기로 대신하지 않는다."""
        assert size == 4
        self.event(('E,' if address == EDITOR else 'M,') + str(self.words(address, 1)[0]))

    def instruction(self, mu, address, size, data):
        """원문의 x87 fild/fstp와 두 분기를 CPU로 실행한다."""
        if address == 0x40eec0:
            value = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 1)[0]
            self.event('A,' + str(value)); self.added = value; self.ret()
        elif not 0x4d7b26 <= address < 0x4d7b5c: raise RuntimeError(f'검토하지 않은 SP 명령 {address:08x}')

    def run(self, editor, mission, amount, stop):
        """독립 caller 입력을 설치하고 전체 trace/실제 float비트를 읽는다."""
        self.put(EDITOR, editor); self.put(MISSION, mission); self.put(STACK + 0x1c, amount)
        self.mu.reg_write(UC_X86_REG_ESP, STACK); self.mu.reg_write(UC_X86_REG_FPCW, 0x37f); self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.trace = []; self.added = None; self.stop = stop; completed = True  # 앞 사례의 가산 효과를 섞지 않는다.
        try: self.mu.emu_start(BEGIN, END, count=1000)
        except ObservationStop: completed = False
        return [str(editor), str(mission), str(amount), str(stop), str(int(completed)), ';'.join(self.trace), '-' if self.added is None else str(self.added)]


def sha(path):
    """최초 관측/보호 소스의 원시 바이트를 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """독립 두 emulation을 비교하고 최초 TSV는 기존 파일을 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--output', type=Path, required=True); args = parser.parse_args()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    amounts = [-2147483648, -2147483647, -1, 0, 1, 9999, 10000, 16777217, 2147483647]  # signed 경계와 float 반올림의 독립 입력이다.
    inputs = [(editor, mission, amount, stop) for editor in (0, 1) for mission in (0, 1) for amount in amounts for stop in range(4)]  # 관문/각 효과 중단144개다.
    first = Oracle(); rows = [first.run(*values) for values in inputs]; replay = Oracle(); repeated = [replay.run(*values) for values in inputs]
    for name, values in (('first.json', rows), ('replay.json', repeated)): (output / name).write_text(json.dumps(values, indent=2) + '\n', encoding='utf-8', newline='\n')  # 독립 두 관측을 보존한다.
    assert rows == repeated
    fixture = ROOT / 'cpppj/tests/fixtures/cheat-money-x86.tsv'
    with fixture.open('x', encoding='utf-8', newline='\n') as stream:
        stream.write('# editor/mission/signed금액/중단/완료/순서/가산float비트\n'); stream.writelines('\t'.join(row) + '\n' for row in rows)
    sources = ['originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'extracted/refined/originals/Netstorm.c', 'tools/decomp_priestmenu.py', 'tools/decomp_cheat_money.py', 'cpppj/tests/fixtures/cheat-money-x86.tsv']
    report = {'host': socket.gethostname(), 'date': '2026-10-30', 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23', 'cases': len(rows), 'observations': len(rows) * 2, 'differences': [], 'entry': '004d7b26', 'tail': '004d8bda', 'commandGlobal': '0054287c', 'commandValue': hex(first.words(0x54287c, 1)[0]), 'fpuControlWord': '037f', 'gameStarted': False,
              'scope': '동일 명령/상위 Guide·Gump·release 분배 뒤 SP 분기만 실제 CPU. 가산 자식은 명시 경계. 일반 모드도0 가산 호출. 포획/희생/공격/승리/MP 실행 없음.', 'sourceSha256': {name: sha(ROOT / name) for name in sources}, 'evidenceSha256': {path.name: sha(path) for path in output.iterdir() if path.is_file()}}
    (ROOT / 'cpppj/cheat-money-native-20261030.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'cases': len(rows), 'observations': len(rows) * 2, 'differences': 0}))


if __name__ == '__main__':
    main()
