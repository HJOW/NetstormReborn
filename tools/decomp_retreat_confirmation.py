#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 LeaveBattle의 signed 확인 관문과 실제 Tell 인수를 게임 실행 없이 관측한다."""
import argparse
import csv
import json
import socket
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, STACK, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP

BEGIN, CONTINUE, END = 0x4d8035, 0x4d803e, 0x4d8bda  # 명령 일치/뗌 거부 뒤의 확인 관문, 양수 본문, 공통 꼬리다.


class ObservationStop(Exception):
    """선택한 Tell 진입에서 부분효과 관측을 끝낸다."""


class Oracle(MemoryOracle):
    """확인 요청 부분만 CPU로 실행하며 양수 이탈/멀티플레이 본문은 실행하지 않는다."""
    def __init__(self):
        """같은 PC의 정밀 함수 범위에 이번 관문이 포함되는지 확인한다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            row = next(row for row in csv.DictReader(stream, delimiter='\t') if int(row['entry'], 16) == 0x4d62b0)
        ranges = [tuple(int(value, 16) for value in part.split('-')) for part in row['ranges'].split(';')]
        assert all(any(low <= address <= high for low, high in ranges) for address in (BEGIN, CONTINUE, 0x4d80da))

    def instruction(self, mu, address, size, data):
        """원문이 전달한 실제 NUL 문자열만 읽고 큰 Tell 파서는 경계로 남긴다."""
        if address == CONTINUE:
            self.continued = True
            mu.emu_stop()
            return
        if address == 0x4cf960:
            pointer = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 1)[0]
            self.section = bytes(mu.mem_read(pointer, 64)).split(b'\0')[0].decode('ascii')
            if self.stop:
                raise ObservationStop()
            self.ret()
            return
        if not (BEGIN <= address < CONTINUE or 0x4d80da <= address < 0x4d80ec):
            raise RuntimeError(f'확인 관문 밖 실행 {address:08x}')

    def run(self, reason, stop):
        """명시 signed 인수만 설치하고 원문 분기/호출 및 스택 복귀를 기록한다."""
        self.section = ''
        self.stop = stop
        self.continued = False
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_EAX, reason & 0xffffffff)
        completed = True
        try:
            self.mu.emu_start(BEGIN, END, count=50)
        except ObservationStop:
            completed = False
        if completed:
            assert self.mu.reg_read(UC_X86_REG_ESP) == STACK
            assert self.mu.reg_read(UC_X86_REG_EIP) == (CONTINUE if self.continued else END)
        return [str(reason), str(stop), str(int(completed)), str(int(self.continued)), self.section or '-']


def main():
    """최초 원문 출력과 독립 재관측을 보존하며 기존 fixture를 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    inputs = [(reason, stop) for reason in (-2147483648, -2147483647, -1, 0, 1, 2, 3, 9, 2147483647) for stop in (0, 1)]  # 음수/0/양수와 Tell 직전 중단의 독립 입력이다.
    first, replay = Oracle(), Oracle()
    rows = [first.run(*values) for values in inputs]
    repeated = [replay.run(*values) for values in inputs]
    for name, values in (('first.json', rows), ('replay.json', repeated)):  # 독립 두 번의 원시 관측을 남긴다.
        (output / name).write_text(json.dumps(values, indent=2) + '\n', encoding='utf-8', newline='\n')
    assert rows == repeated
    fixture = ROOT / 'cpppj/tests/fixtures/retreat-confirmation-x86.tsv'
    with fixture.open('x', encoding='utf-8', newline='\n') as stream:
        stream.write('# signed이유/중단지정/정상복귀/양수본문진행/실제Tell이름\n')
        stream.writelines('\t'.join(row) + '\n' for row in rows)
    sources = ['originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'extracted/refined/originals/Netstorm.c', 'tools/decomp_priestmenu.py', 'tools/decomp_retreat_confirmation.py', 'cpppj/tests/fixtures/retreat-confirmation-x86.tsv']
    report = {'host': socket.gethostname(), 'date': '2026-10-30', 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23', 'cases': len(rows), 'observations': len(rows) * 2, 'differences': [], 'entry': hex(BEGIN), 'positiveBoundary': hex(CONTINUE), 'tail': hex(END), 'gameStarted': False,
              'scope': '명령 일치/상위 뗌 거부 뒤 signed 확인 관문과 실제 Tell 인수만 관측. 양수 이탈/MP 본문·Tell 파서는 실행하지 않음.', 'sourceSha256': {name: digest(ROOT / name) for name in sources}, 'evidenceSha256': {path.name: digest(path) for path in output.iterdir() if path.is_file()}}
    (ROOT / 'cpppj/retreat-confirmation-native-20261030.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'cases': len(rows), 'observations': len(rows) * 2, 'differences': 0}))


if __name__ == '__main__':
    main()
