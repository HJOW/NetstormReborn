#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""LeaveBattle 양수 이유의 비멀티 경로와 실제 공통 하위를 격리 CPU로 관측한다."""
import argparse
import csv
import json
import socket
from decomp_local_mission_transport import Oracle as Base, ROOT, TEXT, STACK, digest
from decomp_native_main_menu_state import BLOCKS
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP

BEGIN, END = 0x4d803e, 0x4d8bda  # 명령/뗌/signed 확인 뒤의 양수 본문과 일반 입력의 공통 꼬리다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/battle-leave-reason-x86.tsv'  # 최초 관측을 덮어쓰지 않는 비멀티22행이다.
REPORT = ROOT / 'cpppj/battle-leave-reason-native-20261106.json'  # 같은 PC의 원문·도구·fixture 정체성과 한계를 기록한다.
FIELDS = (0x542544, 0x55a420, 0x5c85e0, 0x5c85ac, 0x5c8878, 0x5c886c, 0x5c874c, 0x5c88fc)  # 실제 쓰기와 외부 장치 진입에서 관측하는 DWORD다.


class Oracle(Base):
    """원문 양수 부모/next3/선행 삭제/common abort를 실행하고 파일 I/O는 경계로 남긴다."""
    def __init__(self):
        """실제 정밀 함수 범위만 허용하며 원격 페널티 함수는 등록하지 않는다."""
        super().__init__()
        self.allowed = [(BEGIN, 0x4d80da)]  # 앞선 명령 일치/확인 요청과 뒤의 다른 입력은 실행하지 않는다.
        self.active = False  # 입력 배치 중 쓰기는 원문 효과로 집계하지 않는다.
        self.instructions = 0  # 실제 허용 기계어의 실행 수다.
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        for entry in (0x4b3540, 0x4b5880, 0x4b6350):  # 선택된 세 하위도 대체하지 않고 실제 PE를 실행한다.
            for part in facts[entry]['ranges'].split(';'):  # 불연속 범위는 정밀 함수표 그대로 사용한다.
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def guard(self, mu, access, address, size, value, data):
        """스택과 원문8DWORD 이외의 쓰기를 즉시 거부한다."""
        if not self.active:
            return
        if STACK - 0x7f000 <= address and address + size <= STACK + 0x1000:
            return
        if not any(at <= address and address + size <= at + 4 for at in FIELDS):
            raise RuntimeError(f'LeaveBattle 비멀티 쓰기 범위 밖 {address:08x}/{size}')

    def instruction(self, mu, address, size, data):
        """현재 직접 쓰기와 실제 파일 경로/삭제 인수·중단 시점을 관측한다."""
        if address in (0x4b5470, 0x4e58e4):
            if address == 0x4b5470:
                self.paths += 1
                event = f'path{self.paths}'
            else:
                event = f'delete{self.paths}'
                if self.words(mu.reg_read(UC_X86_REG_ESP) + 4)[0] != TEXT:
                    raise RuntimeError('LeaveBattle 삭제 인수 불일치')
            self.events.append(event + ',' + ','.join(str(self.words(at)[0]) for at in FIELDS))
            if self.stop == event:
                self.cause = event
                mu.emu_stop()
            else:
                self.ret(TEXT if address == 0x4b5470 else 0)
            return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'LeaveBattle 미검토/원격 실행 주소 {address:08x}')
        self.instructions += 1

    def sample(self, mode, reason, stop):
        """미션/Editor 두 실제 모드만 넣고 상태 전체와 부분 효과를 반환한다."""
        self.active = False
        self.events, self.paths, self.cause, self.stop = [], 0, 'normal', stop  # 각 입력의 외부 경계 기록은 독립적이다.
        for base in BLOCKS:  # 보존되는 네 블록과 상태 인수에도 같은 입력 흔적을 남긴다.
            self.mu.mem_write(base, bytes([0xa5]) * 144)
        for at, value in ((0x595344, mode == 'mission'), (0x594fbc, mode == 'mission'), (0x594fa4, 1), (0x5c85a4, mode == 'editor'), (0x594fc8, 0), (0x542544, 17), (0x55a420, 133), (0x5c85e0, 19), (0x5c85ac, 2)):
            self.put(at, value)  # known/battle/post는 원격 분기가 선택되지 않는 실제 비멀티 조합이다.
        self.put(STACK + 0x1c, reason)
        self.mu.reg_write(UC_X86_REG_EAX, reason)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.active = True
        self.mu.emu_start(BEGIN, END, count=10000)
        self.active = False
        if self.cause == 'normal' and (self.mu.reg_read(UC_X86_REG_EIP) != END or self.mu.reg_read(UC_X86_REG_ESP) != STACK):
            raise RuntimeError('LeaveBattle 공통 꼬리/스택 복귀 불일치')
        raw = ':'.join(bytes(self.mu.mem_read(base, 144)).hex() for base in BLOCKS)
        return '\t'.join((mode, str(reason), stop, self.cause, *(str(self.words(at)[0]) for at in FIELDS[:4]), raw, ';'.join(self.events) or '-'))


def observe():
    """정상 이유6종과 두 삭제의 실패prefix를 원본 출력으로 고정한다."""
    oracle = Oracle()
    lines = ['# mode / reason / stop / cause / scriptBreak / source / abortPending / outcome / 5x144 raw / boundary trace']
    for mode in ('mission', 'editor'):  # 멀티플레이와 메타맵 조합은 새 관측에 넣지 않는다.
        for reason in (1, 2, 3, 4, 9, 2147483647):  # 메뉴복귀/직접복귀/Cancel/일반양수/강제삭제/최대signed 입력이다.
            lines.append(oracle.sample(mode, reason, 'none'))
    for reason in (3, 9):  # 선행 삭제와 공통 삭제 모두 앞선 부분 효과를 남긴다.
        for stop in ('path1', 'delete1', 'path2', 'delete2'):
            lines.append(oracle.sample('mission', reason, stop))
    for stop in ('path1', 'delete1'):  # 일반 이유의 공통 중단도 별도로 관측한다.
        lines.append(oracle.sample('mission', 1, stop))
    return '\n'.join(lines) + '\n', oracle.instructions


def main():
    """최초 파일은 배타 생성하고 재관측은 원시 전체와 근거 SHA를 비교한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if not args.verify and (FIXTURE.exists() or REPORT.exists()):
        raise RuntimeError('LeaveBattle 최초 관측 덮어쓰기 금지')
    if args.verify:
        saved = json.loads(REPORT.read_text(encoding='utf-8'))
        for path, expected in saved['sha256'].items():  # 독립 재관측의 원문과 도구도 같은 바이트여야 한다.
            if digest(ROOT / path) != expected:
                raise RuntimeError('LeaveBattle SHA 불일치 ' + path)
    observed, count = observe()
    if args.verify:
        if observed != FIXTURE.read_text(encoding='utf-8'):
            raise RuntimeError('LeaveBattle 양수 이유 재관측 차이')
        print(json.dumps({'verified': True, 'rows': 22, 'differences': 0}))
        return
    with FIXTURE.open('x', encoding='utf-8', newline='\n') as stream:
        stream.write(observed)
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_battle_leave_reason.py', 'tools/decomp_local_mission_transport.py', 'tools/decomp_native_main_menu_state.py', FIXTURE.relative_to(ROOT).as_posix())
    report = {'host': socket.gethostname(), 'date': '2026-11-06', 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-11-03', 'rows': 22, 'instructions': count, 'native': ['004d803e-004d80d9', '004b3540', '004b5880', '004b6350'], 'scope': '양수 비멀티 이유/ScriptBreak/선행 삭제/공통 abort·144BYTE5개/부분 중단. 파일 I/O는 명시 장치. 원격 페널티·State6·게임/창/통신 실행 없음.', 'gameStarted': False, 'multiplayerTestsExecuted': False, 'sha256': {path: digest(ROOT / path) for path in paths}}
    with REPORT.open('x', encoding='utf-8', newline='\n') as stream:
        stream.write(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps({'rows': 22, 'instructions': count, 'gameStarted': False}))


if __name__ == '__main__':
    main()
