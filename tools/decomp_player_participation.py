#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 PATCH Player 로그인/방향별 동맹/brain 필드 수명을 격리 관찰한다. 원본 게임은 실행하지 않는다."""
import csv
import json
import struct
import sys
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, PLAYERS, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# 원본 방향별 동맹/숨김 표시의 실제 주소다.
ALLIES, HIDDEN = 0x595200, 0x52f81e
# 고정된 기본입력 순서:owner,pending,mask,flags,oldBrain,subscriber,known,local,metamap,battle,direct,hiddenMask,newBrain.
BASE = [1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 7002]


class Oracle(MemoryOracle):
    """기존 격리 메모리/CRT를 재사용하고 이번 함수 범위와 외부효과만 허용한다."""
    def __init__(self):
        """7개 읽기전용 export의 실제 불연속 함수 범위를 실행 허용 목록으로 만든다."""
        super().__init__();self.allowed = []
        with (ROOT / 'extracted/player-participation/originals/functions.tsv').open(encoding='utf-8') as fp:
            for row in csv.DictReader(fp, delimiter='\t'):
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))

    def instruction(self, mu, address, size, data):
        """접속/Chat/메시지/실제AI 소유자의 파괴·반납 요청만 관찰한다."""
        esp = mu.reg_read(UC_X86_REG_ESP);ecx = mu.reg_read(UC_X86_REG_ECX);args = self.words(esp + 4, 8)
        if address in [0x4a68e0, 0x4c2630]:
            self.ret()
        elif address == 0x4e27b0:
            self.events.append('M,' + ','.join(map(str, args[:2])));self.ret()
        elif address == 0x4a6920:
            assert args[1] == 0;self.events.append('R,' + str(args[0]));self.ret()
        elif address == 0x4e2940:
            self.events.append('D,' + str(args[0]));self.ret()
        elif address == 0x4e0620:
            name = 'B' if self.string(args[0]) == b'!brain' else 'M'
            self.events.append('A,' + name);self.ret()
        elif address == 0x445080:
            self.events.append('X,' + str(args[0]));self.ret()
        elif address == 0x444ee0:
            assert args[2:5] == (0, 0, 0xffffffff) and args[6:8] == (0xffffffff, 0)
            self.events.append('C,' + str(args[0]) + ',' + self.string(args[1]).hex() + ',' + str(args[5]));self.ret()
        elif address == 0x40a780:
            self.ret(ecx)
        elif address == 0x4e2dc0:
            owner = self.mu.mem_read(args[1] + 4, 1)[0];mask = self.words(args[1] + 5, 1)[0]
            self.events.append('S,' + ','.join(map(str, [args[0], owner, mask])));self.ret()
        elif address == 0x410100:
            self.events.append('K,' + str(ecx));self.ret()
        elif address == 0x4e4354:
            self.events.append('F,' + str(args[0]));self.ret()
        else:
            assert any(low <= address and address + size <= high for low, high in self.allowed), f'미등록 명령 {address:08x}'
            self.instructions += 1

    def prepare(self, values):
        """원본 판단을 계산하지 않고 같은 Player9·81DWORD·현재 전역의 독립 입력을 준비한다."""
        owner, pending, mask, flags, brain, subscriber, known, local, meta, battle, direct, hidden, new_brain = values[:13]
        self.events = [];self.mu.mem_write(PLAYERS, bytes(1620));self.put(0x59534c, PLAYERS)
        for player in range(9):
            self.mu.mem_write(PLAYERS + player * 180 + 12, ('P' + str(player)).encode() + b'\0')
            self.put(PLAYERS + player * 180 + 44, player + 100)
            self.mu.mem_write(PLAYERS + player * 180 + 48, b'Z' * 30 + b'~\x7f')
            self.mu.mem_write(HIDDEN + player, bytes([int(bool(hidden & (1 << player)))]))
            for target in range(9):
                self.put(ALLIES + (player * 9 + target) * 4, 1 if player == target else 7 if (player + target) % 3 == 0 else 0)
        selected = owner if 0 <= owner < 9 else 0
        self.put(PLAYERS + selected * 180 + 80, flags);self.put(PLAYERS + selected * 180 + 84, mask)
        self.put(PLAYERS + selected * 180 + 176, brain);self.put(PLAYERS + selected * 180 + 44, subscriber)
        if len(values) == 14:
            self.mu.mem_write(PLAYERS + selected * 180 + 12, b'N' * values[13] + b'\0')
        for address, value in [(0x595344, known), (0x540c70, local), (0x594fc8, meta), (0x594fbc, battle), (0x54db50, direct)]:
            self.put(address, value)

    def execute(self, kind, values):
        """정상 반환·cdecl/thiscall 스택·보존 레지스터·제어어와 전체 raw 출력/효과를 관찰한다."""
        self.prepare(values);owner, pending, mask = values[:3]
        address, args, ecx, purge = {
            'Login': (0x4a6dc0, [pending, owner], 0, 0),
            'Alliance': (0x490ea0, [owner, mask], 0, 0),
            'Add': (0x48f6a0, [pending], PLAYERS + owner * 180, 4),
            'Attach': (0x4911a0, [values[12]], PLAYERS + owner * 180, 4),
            'Remove': (0x413ae0, [owner], 0, 0),
            'RemoveAll': (0x413b20, [], 0, 0)
        }[kind]
        saved = {UC_X86_REG_EBX: 0x123456, UC_X86_REG_ESI: 0x234567, UC_X86_REG_EDI: 0x345678, UC_X86_REG_EBP: 0x456789}
        for register, value in saved.items():
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_FPCW, 0x027f);self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.reg_write(UC_X86_REG_ECX, ecx)
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * (len(args) + 1), STOP, *(value & 0xffffffff for value in args)))
        self.mu.emu_start(address, STOP, count=100000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4 + purge
        assert all(self.mu.reg_read(register) == value for register, value in saved.items())
        assert self.mu.reg_read(UC_X86_REG_FPCW) == 0x027f
        return [kind, ','.join(map(str, values)), self.mu.reg_read(UC_X86_REG_EAX) if kind == 'Login' else '-', bytes(self.mu.mem_read(PLAYERS, 1620)).hex(), bytes(self.mu.mem_read(ALLIES, 324)).hex(), ';'.join(self.events) or '-']


def inputs():
    """경계 값/방향/이전포인터/진단/모드 입력만 만들며 출력 모델은 작성하지 않는다."""
    for owner in [-1, 0, 1, 8, 9]:
        for pending in [-1, 1, 9]:
            for flags in [0, 1, 2, 3, 0x100, 0x121, 0xa5a50000]:
                for direct in [0, 1]:
                    case = BASE.copy();case[0] = owner;case[1] = pending;case[3] = flags;case[10] = direct;yield 'Login', case
    for owner in [0, 1, 8]:
        for mask in [0, 1, 0x100, 0xffffffff, 0x10000]:
            for known in [0, 1]:
                for meta in [0, 1]:
                    for hidden in [0, 0x155]:
                        case = BASE.copy();case[0] = owner;case[2] = mask;case[6] = known;case[8] = meta;case[11] = hidden;yield 'Alliance', case
    for owner in [0, 1, 8]:
        for target in [0, 1, 8]:
            for battle in [0, 1]:
                for meta in [0, 1]:
                    for mask in [0, 0xffffffff]:
                        case = BASE.copy();case[0] = owner;case[1] = target;case[2] = mask;case[8] = meta;case[9] = battle;yield 'Add', case
    for owner in [0, 1, 8]:
        for old in [0, 7001]:
            for new in [0, 7002]:
                for subscriber in [0, 1, 0xffffffff, 0x80000000]:
                    for meta in [0, 1]:
                        for flags in [0, 0x80000103]:
                            case = BASE.copy();case[0] = owner;case[3] = flags;case[4] = old;case[5] = subscriber;case[8] = meta;case[12] = new;yield 'Attach', case
    for kind in ['Remove', 'RemoveAll']:
        for owner in [0, 1, 8]:
            for old in [0, 7001]:
                case = BASE.copy();case[0] = owner;case[3] = 0x12345678;case[4] = old;yield kind, case
    # +0xc..2b의32BYTE이름필드가정상수용하는30/31글자경계다.
    for length in [30, 31]:
        case = BASE.copy();case[6] = 1;case.append(length);yield 'Alliance', case


def main():
    """현재 소스/실제 export/전체 fixture의 계보와 고유입력 수를 생성하거나 감사한다."""
    report_path = ROOT / 'cpppj/player-participation.json'
    fixture = ROOT / 'cpppj/tests/fixtures/player-participation-x86.tsv'
    if '--verify' in sys.argv:
        report = json.loads(report_path.read_text(encoding='utf-8'))
        for path, expected in report['sha256'].items():
            assert digest(ROOT / path) == expected, f'SHA 불일치: {path}'
        with fixture.open(encoding='utf-8') as fp:
            rows = list(csv.reader(fp, delimiter='\t'))[1:]
        assert len(rows) == report['observations'] and len({tuple(row[:2]) for row in rows}) == len(rows)
        assert {kind: sum(row[0] == kind for row in rows) for kind in report['counts']} == report['counts']
        print(json.dumps({'verified': True, 'observations': len(rows)}));return
    oracle = Oracle();rows = [oracle.execute(kind, values) for kind, values in inputs()]
    with fixture.open('w', encoding='utf-8', newline='') as fp:
        writer = csv.writer(fp, delimiter='\t', lineterminator='\n');writer.writerow(['kind', 'input', 'return', 'native_Player9', 'native_Allies81', 'native_effects']);writer.writerows(rows)
    paths = ['originals/Netstorm.exe', 'extracted/player-participation/originals/creation.c', 'extracted/player-participation/originals/functions.tsv', 'extracted/refined/originals/functions.tsv', 'tools/ghidra/player-participation-functions.json', 'tools/decomp_player_participation.py', 'tools/decomp_priestmenu.py', 'cpppj/src/o/RawPlayerParticipation.cpp', 'cpppj/src/o/RawPlayerParticipation.h', 'cpppj/tests/PlayerParticipationTests.cpp', 'cpppj/tests/fixtures/player-participation-x86.tsv']
    counts = {kind: sum(row[0] == kind for row in rows) for kind in ['Login', 'Alliance', 'Add', 'Attach', 'Remove', 'RemoveAll']}
    report = {'host': 'VM-W11-Codex', 'observations': len(rows), 'counts': counts, 'instructions': oracle.instructions, 'boundaries': ['로그/assert 진단', '연결번호 재배치/거부응답/연결해제', 'Chat 채널초기화/등록', '동맹packet생성/송신', '실제AI destructor/메모리반납'], 'limits': ['실제brain생성자/AI전략/로그인전체/실제Chat전송은별도', 'PATCH전용이며CD대응은정적참고'], 'sha256': {path: digest(ROOT / path) for path in paths}}
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'observations': len(rows), 'counts': counts}))


if __name__ == '__main__':
    main()
