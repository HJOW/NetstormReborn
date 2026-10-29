#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""전투 종료 통지와 State3 예약의 원본 명령을 창·파일·네트워크 실행 없이 고정한다."""
import argparse
import csv
import json
import socket
import struct
from pathlib import Path
from decomp_local_mission_transport import Oracle as MemoryOracle, ROOT, TEXT, PLAYERS, STACK, STOP, digest
from decomp_native_main_menu_state import Oracle as MenuOracle, cases, BLOCKS
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

FIXTURE = ROOT / 'cpppj/tests/fixtures/battle-leave-x86.tsv'  # 제품 작성 전에 고정하는 원시 CPU 출력이다.
REPORT = ROOT / 'cpppj/battle-leave-native-20261030.json'  # 입력 PE/정밀 소스/관측 도구의 정체성과 실행 범위다.


class Oracle(MemoryOracle):
    """생성자·길이 setter·수신기·예약기의 실제 명령만 실행한다."""
    def __init__(self):
        """기존 격리 PE 장치를 빌리고 이번 검토 함수의 실제 범위로 실행을 제한한다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []  # 네 함수의 불연속 기계어 범위다.
        for entry in (0x40aa60, 0x4df230, 0x4a71a0, 0x4b3540):  # 큰 외부 자식의 결과를 대신 계산하지 않는다.
            for part in facts[entry]['ranges'].split(';'):  # 원문 함수표의 실제 조각을 모두 허용한다.
                low, high = (int(value, 16) for value in part.split('-')); self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)
        self.instructions = 0  # 실제 실행 명령 수만 기록한다.

    def guard(self, mu, access, address, size, value, data):
        """명시 입력 저장소와 스택 외에 쓰면 관측을 실패시킨다."""
        ranges = ((STACK - 0x7f000, STACK + 0x1000), (TEXT, TEXT + 16),
                  (PLAYERS, PLAYERS + 1620), (0x5c8878, 0x5c887c))
        if not any(low <= address and address + size <= high for low, high in ranges):
            raise RuntimeError(f'BattleLeave 원문 쓰기 범위 밖 {address:08x}/{size}')

    def instruction(self, mu, address, size, data):
        """원본 종료 표식에서 멈추고 허용 함수 이외의 실행은 거부한다."""
        if address == STOP: mu.emu_stop(); return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'BattleLeave 미검토 실행 주소 {address:08x}')
        self.instructions += 1

    def run(self, entry, argument=None, this=0):
        """기계어의 원래 cdecl/thiscall 스택을 만들고 실제 ret와 정리 폭을 검사한다."""
        self.put(STACK, STOP)
        if argument is not None: self.put(STACK + 4, argument)
        self.mu.reg_write(UC_X86_REG_ESP, STACK); self.mu.reg_write(UC_X86_REG_ECX, this)
        self.mu.emu_start(entry, STOP, count=10000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError('BattleLeave 정상 ret/스택 불일치')


class ParentOracle(MenuOracle):
    """기존 State3의 실제 송신 경계에서 작성된 다섯 BYTE만 추가 관측한다."""
    def __init__(self):
        """기존 원문 부모/큰 장치의 구분을 유지하고 송신 관측만 비운다."""
        super().__init__(); self.sent = []  # 실제 004e2dc0의 target와 작성된 패킷이다.

    def instruction(self, mu, address, size, data):
        """송신의 기대값을 만들지 않고 실제 스택 인자와 버퍼를 읽는다."""
        if address == 0x4e2dc0:
            target, packet = self.words(self.mu.reg_read(UC_X86_REG_ESP) + 4, 2)
            self.sent.append(f'{target}:' + bytes(self.mu.mem_read(packet, 5)).hex())
        return super().instruction(mu, address, size, data)


def observe():
    """명시 입력만 배치하고 결과는 모두 실제 명령 뒤의 원시 물리에서 수집한다."""
    oracle = Oracle(); lines = ['# kind / input / raw output / actual trace']; counts = {}
    for fill in (0, 0xa5, 0xff):  # 생성자가 쓰지 않는 다섯째 BYTE와 꼬리 보존을 관측한다.
        oracle.mu.mem_write(TEXT, bytes([fill]) * 16); oracle.run(0x40aa60, this=TEXT)
        if oracle.mu.reg_read(UC_X86_REG_EAX) != TEXT: raise RuntimeError('BattleLeave 생성자 this 반환 불일치')
        lines.append(f'header\t{fill}\t' + bytes(oracle.mu.mem_read(TEXT, 16)).hex() + '\tthis')
    for flags in (0, 1, 2, 4, 0x80000000, 0xfffffffb, 0xffffffff):  # 단독 로컬 Player1의 현재 DWORD만 변화시킨다.
        raw = bytes((index * 19 + 7) & 255 for index in range(1620)); oracle.mu.mem_write(PLAYERS, raw)
        oracle.put(PLAYERS + 180 + 80, flags); oracle.put(0x59534c, PLAYERS); oracle.run(0x4a71a0, argument=1)
        lines.append(f'receive\t{flags}\t' + bytes(oracle.mu.mem_read(PLAYERS, 1620)).hex() + '\tlocal:1')
    for fill in (0, 0xa5, 0xff):  # 다음 상태의 나머지140BYTE와 다른 네 블록을 보존하는지 기록한다.
        for base in BLOCKS: oracle.mu.mem_write(base, bytes([fill]) * 144)  # 독립 다섯 Native 블록을 같은 입력으로 채운다.
        oracle.run(0x4b3540)
        lines.append(f'next\t{fill}\t' + ':'.join(bytes(oracle.mu.mem_read(base, 144)).hex() for base in BLOCKS) + '\t')
    selected = dict(cases())
    for label in ('battle-fourteen', 'battle-other-phase', 'default'):  # 승패/포획/희생/특정 공격 입력은 실행하지 않는다.
        parent = ParentOracle(); parent.sample(label, selected[label], 0x27f)
        lines.append(f'parent\t{label}\t' + ';'.join(parent.sent) + '\t' + ';'.join(parent.events))
        counts[label] = parent.instructions
    return '\n'.join(lines) + '\n', oracle.instructions, counts


def main():
    """최초 관측은 덮어쓰지 않고 재검증은 동일 CPU 실행과 바이트 해시를 모두 대조한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify', action='store_true'); args = parser.parse_args()
    if not args.verify and (FIXTURE.exists() or REPORT.exists()): raise RuntimeError('BattleLeave 최초 관측 덮어쓰기 금지')
    if args.verify:
        saved = json.loads(REPORT.read_text(encoding='utf-8'))
        for path, expected in saved['sha256'].items():  # 고정한 PE/소스/도구/fixture의 실제 바이트를 먼저 확인한다.
            if digest(ROOT / path) != expected: raise RuntimeError('BattleLeave SHA 불일치 ' + path)
    text, instructions, parents = observe()
    if args.verify:
        if FIXTURE.read_text(encoding='utf-8') != text: raise RuntimeError('BattleLeave 재관측 차이')
        print(json.dumps({'verified': True, 'rows': len(text.splitlines()) - 1, 'differences': 0})); return
    FIXTURE.write_text(text, encoding='utf-8')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv',
             'tools/decomp_battle_leave.py', 'tools/decomp_local_mission_transport.py', 'tools/decomp_native_main_menu_state.py', str(FIXTURE.relative_to(ROOT)).replace('\\', '/'))
    report = {'host': socket.gethostname(), 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23',
              'rows': len(text.splitlines()) - 1, 'instructions': instructions, 'parentInstructions': parents,
              'scope': '원본 CPU 생성자/로컬 Player1 수신/다음3 예약과 기존 State3 송신 관측. 게임·창·네트워크·승패 재현 없음.',
              'sha256': {path: digest(ROOT / path) for path in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8'); print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__': main()
