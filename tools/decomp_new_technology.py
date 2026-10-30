#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본492a30의 현재 부모/중복/저장·설정·그림·Tell 순서와 중단 부분효과를 동결한다."""
import argparse
import csv
import hashlib
import json
import socket
import struct
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TEXT, STACK, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP

RECORD = 0x12345678  # TYPE 조회가 돌려주는 명시 외부 토큰이며 실제 호스트 포인터로 사용하지 않는다.


class ObservationStop(Exception):
    """지정한 외부 효과 진입에서 원문 부분 순서를 보존하는 관측 중단이다."""


class Oracle(MemoryOracle):
    """원문 부모만 실제 명령으로 실행하고 원래 child 진입과 응답을 명시한다."""
    def __init__(self):
        """현재 PC의 전체 정밀 함수 범위와 보호 PE만 읽는다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []  # 선택한 원문 부모의 정확한 허용 구간이다.
        for part in facts[0x492a30]['ranges'].split(';'):  # 떨어진 원문 구간도 합치지 않는다.
            low, high = (int(value, 16) for value in part.split('-')); self.allowed.append((low, high + 1))

    def event(self, text):
        """callback 진입을 먼저 기록하고 지정 위치에서는 효과를 적용하기 전에 중단한다."""
        self.trace.append(text)
        if self.stop and len(self.trace) == self.stop: raise ObservationStop()

    def instruction(self, mu, address, size, data):
        """원문의 순서/분기/타입 인수는 바꾸지 않고 큰 child/CRT·UI 응답만 공급한다."""
        esp = mu.reg_read(UC_X86_REG_ESP); args = self.words(esp + 4, 6); parent = (mu.reg_read(UC_X86_REG_ECX) - POOL) // 50
        if address == 0x48fc60:
            value = (10, 20, 30)[min(self.parents, 2)]; self.parents += 1; self.event('G,' + str(value)); self.ret(value)
        elif address == 0x4acac0:
            self.event('Q,' + ','.join(map(str, (parent, *args[:3])))); self.ret(self.found, 12)
        elif address == 0x4afa70:
            self.event('I,' + ','.join(map(str, (parent, *args[:5])))); self.ret(77, 20)
        elif address == 0x4bfac0: self.event('S'); self.ret()
        elif address == 0x49a840: self.event('T,' + str(args[0])); self.ret(RECORD)
        elif address == 0x49af80: self.event('N,' + str(mu.reg_read(UC_X86_REG_ECX))); self.ret(TEXT + 0x3000)
        elif address == 0x4414d0:
            key, value = self.string(args[0]), self.string(args[1]); self.event('C,' + key.hex() + ',' + value.hex()); self.config[key] = value; self.ret()
        elif address == 0x4a89c0:
            pattern = self.string(args[1]); value = struct.unpack('<i', struct.pack('<I', args[2]))[0]
            self.event('F,' + pattern.hex() + ',' + str(value)); rendered = (pattern.decode('ascii') % value).encode('ascii'); mu.mem_write(args[0], rendered + b'\0'); self.ret(args[0])
        elif address == 0x4cf960: self.event('L,' + self.string(args[0]).hex()); self.ret()
        elif not any(low <= address < high for low, high in self.allowed): raise RuntimeError(f'검토하지 않은 기술 획득 명령 {address:08x}')

    def run(self, kind, found, caption, stop):
        """입력 문자열/반환값과 지정 중단만 공급하고 원문 최종 trace·설정을 읽는다."""
        self.found = found; self.stop = stop; self.parents = 0; self.trace = []; self.config = {}  # 호출별 명시 장치 상태다.
        self.mu.mem_write(TEXT + 0x3000, caption + b'\0'); self.put(0x5c8464, POOL)
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP): self.mu.reg_write(register, 0)  # caller 일반 레지스터를 고정한다.
        self.mu.reg_write(UC_X86_REG_ESP, STACK); self.mu.mem_write(STACK, struct.pack('<II', STOP, kind)); completed = True
        try: self.mu.emu_start(0x492a30, STOP, count=2000)
        except ObservationStop: completed = False
        return [str(kind), str(found), caption.hex() or '-', str(stop), str(int(completed)), ';'.join(self.trace),
                self.config.get(b'global.newTechName', b'').hex() if b'global.newTechName' in self.config else '-',
                self.config.get(b'global.newTechPic', b'').hex() if b'global.newTechPic' in self.config else '-']


def cases():
    """입력/ABI 반환 폭과 각 효과 중단을 고정하며 기대 분기/설정을 계산하지 않는다."""
    for kind in (83, 0, 0xffffffff, 0x80000000):  # signed 그림 서식과 모든 DWORD 인수 보존을 관측한다.
        for found in (0, 85, 0x10000, 0xffff):  # 원본 caller의 WORD 반환 판정도 직접 관측한다.
            for caption in (b'Sun Cannon', b'Name\0ignored'): yield kind, found, caption, 0  # C 문자열 경계를 분리한다.
    yield 83, 0, b'', 0
    for stop in range(1, 13): yield 83, 0, b'Sun Cannon', stop  # 중복이 없는 정상 경로의 각 외부 효과에서 중단한다.


def sha(path):
    """파일을 수정하지 않고 원래 바이트의 SHA-256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """최초 관측 뒤 새 emulation에서 전체 재관측하고 fixture를 최초 한 번만 쓴다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--output', type=Path, required=True); args = parser.parse_args()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False); inputs = list(cases())
    first = Oracle(); rows = [first.run(*values) for values in inputs]; replay = Oracle(); repeated = [replay.run(*values) for values in inputs]
    (output / 'first.json').write_text(json.dumps(rows, indent=2) + '\n', encoding='utf-8'); (output / 'replay.json').write_text(json.dumps(repeated, indent=2) + '\n', encoding='utf-8'); assert rows == repeated
    fixture = ROOT / 'cpppj/tests/fixtures/new-technology-x86.tsv'
    with fixture.open('x', encoding='utf-8', newline='\n') as stream:
        stream.write('# type/find-return/입력이름/중단/완료/전체순서/이름설정/그림설정\n'); stream.writelines('\t'.join(row) + '\n' for row in rows)
    sources = ['originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'extracted/refined/originals/Netstorm.c', 'tools/decomp_priestmenu.py', 'tools/decomp_new_technology.py', 'cpppj/tests/fixtures/new-technology-x86.tsv']
    report = {'host': socket.gethostname(), 'date': '2026-10-30', 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23',
              'cases': len(rows), 'observations': len(rows) * 2, 'differences': [], 'entry': '00492a30', 'cd1072StaticCounterpart': '0040e300', 'gameStarted': False,
              'boundaries': ['현재 기술부모/contained 반환·생성/snapshot·TYPE/현재 이름/Config·printf/Tell은 명시 외부 경계', '원문 부모의 getter 재조회/조건·호출 인수·signed 서식/중단 prefix는 실제 CPU', '포획/희생/공격/승리·MP 실행 없음'],
              'sourceSha256': {name: sha(ROOT / name) for name in sources}, 'evidenceSha256': {path.name: sha(path) for path in sorted(output.iterdir()) if path.is_file()}}
    (ROOT / 'cpppj/new-technology-native-20261030.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n'); print(json.dumps({'cases': len(rows), 'observations': len(rows) * 2, 'differences': 0}))


if __name__ == '__main__': main()
