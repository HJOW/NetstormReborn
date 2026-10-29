#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""단독 미션/편집 종료의 공통004b6350 직접 쓰기와 저장 경로/삭제 호출 순서를 관측한다."""
import argparse
import csv
import json
import socket
from decomp_local_mission_transport import Oracle as Base, ROOT, TEXT, STACK, STOP, digest
from decomp_native_main_menu_state import BLOCKS
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP

FIXTURE = ROOT / 'cpppj/tests/fixtures/battle-abort-state-x86.tsv'  # 앞서 고정한16행을 변경하지 않는 추가 공통 부모 관측이다.
REPORT = ROOT / 'cpppj/battle-abort-state-native-20261030.json'  # 원문/도구/8행의 정체성과 관측 범위를 고정한다.


class Oracle(Base):
    """실제 공통 함수만 실행하고 파일 경로/삭제는 명시 장치 경계로 구분한다."""
    def __init__(self):
        """원본 PE를 격리 복사하고 실제 함수표의004b6350 범위만 허용한다."""
        super().__init__(); self.allowed = []; self.active = False; self.instructions = 0  # CPU 실행 중에만 쓰기 범위를 검사한다.
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            fact = next(row for row in csv.DictReader(stream, delimiter='\t') if int(row['entry'], 16) == 0x4b6350)
        for part in fact['ranges'].split(';'):  # 분리된 함수 조각도 실제 함수표 그대로 허용한다.
            low, high = (int(value, 16) for value in part.split('-')); self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def guard(self, mu, access, address, size, value, data):
        """실제 네 DWORD와 상태 부가인자 세 DWORD 외의 직접 쓰기를 거부한다."""
        if not self.active: return
        fields = (0x5c85e0, 0x5c85ac, 0x5c8878, 0x5c88fc, 0x5c874c, 0x5c886c)
        if STACK - 0x7f000 <= address and address + size <= STACK + 0x1000: return
        if not any(at <= address and address + size <= at + 4 for at in fields):
            raise RuntimeError(f'BattleAbort 원문 쓰기 범위 밖 {address:08x}/{size}')

    def instruction(self, mu, address, size, data):
        """파일을 열거나 지우지 않고 실제 경계의 인자와 선행 부분효과만 기록한다."""
        if address == STOP: mu.emu_stop(); return
        if address in (0x4b5470, 0x4e58e4):  # 디컴파일 이름 thunk_FUN_004e58ba의 실제 호출 진입은004e58e4다.
            event = 'path' if address == 0x4b5470 else 'delete'
            if event == 'delete' and self.words(self.mu.reg_read(UC_X86_REG_ESP) + 4)[0] != TEXT: raise RuntimeError('BattleAbort 삭제 인자 불일치')
            self.events.append(event)
            if self.stop == event: self.cause = event; mu.emu_stop(); return
            self.ret(TEXT if event == 'path' else 0); return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'BattleAbort 미검토 실행 주소 {address:08x}')
        self.instructions += 1

    def sample(self, mode, fill, outcome, stop):
        """단독 미션 또는 편집 모드만 배치하며 원격/메타맵 입력은 실행하지 않는다."""
        self.active = False; self.events = []; self.cause = 'normal'; self.stop = stop  # 매 입력의 실제 호출/종료 원인을 새로 기록한다.
        for base in BLOCKS: self.mu.mem_write(base, bytes([fill]) * 144)  # 다른 네 블록과 부가인자의 보존 흔적이다.
        for address, value in ((0x594fa4, 1), (0x5c85a4, mode == 'editor'), (0x594fc8, 0), (0x594fbc, mode != 'editor'), (0x5c85ac, outcome), (0x5c85e0, 0xaabbccdd)):
            self.put(address, value)  # 실제 공통 함수가 읽을 현재 외부 입력만 설치한다.
        self.put(STACK, STOP); self.mu.reg_write(UC_X86_REG_ESP, STACK); self.active = True
        self.mu.emu_start(0x4b6350, STOP, count=10000); self.active = False
        if self.cause == 'normal' and (self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4): raise RuntimeError('BattleAbort 정상 ret 불일치')
        raw = ':'.join(bytes(self.mu.mem_read(base, 144)).hex() for base in BLOCKS)
        return '\t'.join((mode, str(fill), str(outcome), stop, self.cause, str(self.words(0x5c85e0)[0]), str(self.words(0x5c85ac)[0]), raw, ';'.join(self.events)))


def observe():
    """원본이 쓰는 실제720BYTE/두 전역/장치 순서를 수집하며 기대 모델은 계산하지 않는다."""
    oracle = Oracle(); lines = ['# mode / fill / outcome / stop / cause / abortPending / outcomeAfter / 5x144 raw / actual trace']
    for mode in ('mission', 'editor'):  # 실제 단독/편집 공통 경로만 관측한다.
        for fill, outcome in ((0, 0), (0xa5, 5), (0xff, 0x80000000)): lines.append(oracle.sample(mode, fill, outcome, 'none'))  # 상위비트와 보존 물리를 포함한다.
    for stop in ('path', 'delete'): lines.append(oracle.sample('mission', 0xa5, 5, stop))  # 장치 중단 전의 직접 쓰기가 유지되는지 기록한다.
    return '\n'.join(lines) + '\n', oracle.instructions


def main():
    """최초 관측을 보존하고 verify는 같은 명령과 전체 바이트를 다시 비교한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); args = parser.parse_args()
    if not args.verify and (FIXTURE.exists() or REPORT.exists()): raise RuntimeError('BattleAbort 최초 관측 덮어쓰기 금지')
    if args.verify:
        saved = json.loads(REPORT.read_text(encoding='utf-8'))
        for path, expected in saved['sha256'].items():  # 원본/도구/fixture의 정체성도 함께 확인한다.
            if digest(ROOT / path) != expected: raise RuntimeError('BattleAbort SHA 불일치 ' + path)
    text, count = observe()
    if args.verify:
        if text != FIXTURE.read_text(encoding='utf-8'): raise RuntimeError('BattleAbort 재관측 차이')
        print(json.dumps({'verified': True, 'rows': 8, 'differences': 0})); return
    FIXTURE.write_text(text, encoding='utf-8')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_battle_abort_state.py', 'tools/decomp_local_mission_transport.py', 'tools/decomp_native_main_menu_state.py', str(FIXTURE.relative_to(ROOT)).replace('\\', '/'))
    report = {'host': socket.gethostname(), 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23', 'rows': 8, 'instructions': count,
              'scope': '단독/편집004b6350의 실제 직접 쓰기와 path/delete 경계만 관측. 파일 삭제/게임/창/네트워크/승패 실행 없음.', 'sha256': {path: digest(ROOT / path) for path in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8'); print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__': main()
