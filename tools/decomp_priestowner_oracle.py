#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제의 소유자 재정의를 세 실제 PE에서 제한 x86으로 대조한다. 대체 함수/게임/OS 실행 없음.

사제 몸체, 공통 소유자 지정, 타입 조회와 무상태 알림을 실제 명령으로 실행한다.
알림/공통 지정 진입에서 단어와 현재 소유자를 관찰하며 호출을 대체하지 않는다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path

from decomp_owner_oracle import (OwnerOracle, ROOT, SPECS, TYPES, TARGET, STOP, STACK,
    MAX_INSTRUCTIONS, digest, UcError)
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

# 판본별 사제 재정의/알림/가상 표와 로딩·로컬 플레이어 전역이다.
PRIEST = {
    'originals': dict(entry=0x491790, notify=0x4214a0, vtable=0x50f210,
        globals=dict(loading=0x5c89b8, fort_player=0x540cac, battle_player=0x540c70)),
    'originalCD': dict(entry=0x40bda0, notify=0x448c10, vtable=0x5003e0,
        globals=dict(loading=0x5178d4, fort_player=0x50f6d4, battle_player=0x50f6c8)),
}
PRIEST['original1037'] = PRIEST['originalCD']
# 실제 두 판본 사제 타입과 출력 파일이다.
PRIEST_TYPE = 158
FIXTURE = ROOT / 'cpppj/tests/fixtures/priestowner-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-priestowner-evidence.json'


def inputs(edition):
    """모드·로딩·원래 단어·요청값·두 로컬 번호의 입력 격자다. 원본 assert 입력만 제외한다."""
    result = []
    words = (0, 1, 0x81, 0x102, 0xab05, 0xff08, 0xff88, 0x7f, 0xffff)
    players = (0, 1, 3, 8, 9, 127, 128, 0x80000000, 0xffffffff)
    local_pairs = ((1, 3), (3, 1), (0, 0), (8, 8))
    # 기대 결과는 계산하지 않는다. 상태/extra/이전 소유자에 다른 입력 잡음을 섞는다.
    for index, (fort, battle, loading, word, player, local) in enumerate(itertools.product(
            (0, 1), (0, 1), (0, 1, 3), words, players, local_pairs)):
        updating = not battle or loading != 0
        if edition == 'originals' and updating and player == 0: continue
        if edition != 'originals' and ((updating and player > 8) or (not updating and (word & 0x7f) > 8)): continue
        result.append(dict(fort=fort, battle=battle, loading=loading, word=word, player=player,
            fort_player=local[0], battle_player=local[1], state=(0, 2, 4, 6, 8)[index % 5],
            extra=(0, 1, 8, 9, 255)[(index // 5) % 5], owner=index % 9))
    return result


class PriestOracle(OwnerOracle):
    """공통 실행기에 사제 내보내기와 쓰기 필드만 추가한다."""
    def __init__(self, edition):
        """공통 몸체를 재사용하고 사제/알림의 실제 명령 범위를 허용한다."""
        super().__init__(edition)
        self.base_entry = self.spec['entry']
        self.spec = dict(self.spec, entry=PRIEST[edition]['entry'])
        paths = [ROOT / f'extracted/priestowner/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.exports.extend(paths)
        with paths[1].open(encoding='utf-8') as fp:
            # 디컴파일러가 계산한 불연속 몸체 범위만 추가한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))
        self.events = []

    def on_instruction(self, mu, address, size, data):
        """호출 당시 필드를 관찰하고 원본 명령 실행을 그대로 계속한다."""
        if address in (self.base_entry, PRIEST[self.edition]['notify']):
            esp = mu.reg_read(UC_X86_REG_ESP)
            _, argument = struct.unpack('<II', mu.mem_read(esp, 8))
            slot = self.slot(TARGET)
            word = struct.unpack('<H', mu.mem_read(slot + 12, 2))[0]
            owner = mu.mem_read(slot + self.o['owner'], 1)[0]
            if address == self.base_entry:
                if mu.reg_read(UC_X86_REG_ECX) != slot: raise RuntimeError('공통 지정 this 불일치')
                self.events.append(f'B:{TARGET}:{argument}:{word}:{owner}')
            else:
                if argument != TARGET: raise RuntimeError('알림 SID 불일치')
                self.events.append(f'N:{argument}:{word}:{owner}')
        super().on_instruction(mu, address, size, data)

    def run_priest(self, case):
        """입력을 넣고 원본 재정의 전체의 정상 반환·사건·슬롯 전체를 관찰한다."""
        # 공통/사제 모드 전역을 분리하여 입력한다. challenge 모드는 이 격자에서 제외한다.
        for name in ('fort', 'battle'):
            self.mu.mem_write(self.g[name], struct.pack('<I', case[name]))
        self.mu.mem_write(self.g['challenge'], bytes(4))
        # 원본이 비교하는 세 전역은 DWORD다.
        for name, address in PRIEST[self.edition]['globals'].items():
            self.mu.mem_write(address, struct.pack('<I', case[name]))
        base = TYPES + PRIEST_TYPE * self.type_stride
        self.mu.mem_write(base, bytes(self.type_stride))
        self.mu.mem_write(base + 0xec, struct.pack('<I', 0x00210000))
        slot = self.slot(TARGET)
        raw = bytearray(self.stride)
        struct.pack_into('<I', raw, 0, PRIEST[self.edition]['vtable'])
        raw[10], raw[11], raw[self.o['owner']], raw[self.o['extra']] = PRIEST_TYPE, case['state'], case['owner'], case['extra']
        struct.pack_into('<H', raw, 12, case['word'])
        struct.pack_into('<ff', raw, 14, 20.75, 21.9)
        self.mu.mem_write(slot, bytes(raw))
        self.write_ranges = [(slot + 12, slot + 14), (slot + self.o['owner'], slot + self.o['owner'] + 1)]
        # 원본 thiscall 레지스터/반환 주소를 준비한다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, slot)
        self.mu.mem_write(STACK, struct.pack('<II', STOP, case['player']))
        self.events = []
        try: self.mu.emu_start(self.spec['entry'], STOP, count=MAX_INSTRUCTIONS)
        except UcError as error: raise RuntimeError(f'사제 소유자 에뮬레이션 오류: {error}') from error
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 8:
            raise RuntimeError('사제 소유자 반환/스택 복구 불일치')
        return [';'.join(self.events) or '-', bytes(self.mu.mem_read(slot, self.stride)).hex()]


def generate(smoke=False):
    """세 PE의 독립 관찰과 모든 입력/실행 도구/몸체 SHA를 저장한다."""
    rows, reports = [], {}
    paths = {Path(__file__), ROOT / 'tools/decomp_owner_oracle.py', ROOT / 'tools/ghidra/owner-functions.json',
             ROOT / 'tools/ghidra/priestowner-functions.json', FIXTURE}
    # 각 판본은 자기 원본에서만 기대값을 얻는다.
    for edition in SPECS:
        oracle = PriestOracle(edition)
        cases = inputs(edition)
        if smoke: cases = cases[::97]
        # 입력의 기록 순서를 그대로 유지하여 C++에서 재생한다.
        for case in cases:
            rows.append(['Owner', edition, *[case[key] for key in ('fort', 'battle', 'loading', 'fort_player',
                'battle_player', 'word', 'owner', 'player', 'state', 'extra')], *oracle.run_priest(case)])
        reports[edition] = dict(binary=oracle.spec['binary'], sha256=oracle.sha256, cases=len(cases),
            entry=f'{oracle.spec["entry"]:08x}', base_entry=f'{oracle.base_entry:08x}',
            notify_entry=f'{PRIEST[edition]["notify"]:08x}', native_calls=dict(oracle.native_calls),
            assertions=oracle.assertions, instructions=oracle.instructions)
        paths.update(oracle.exports)
        paths.add(ROOT / oracle.spec['binary'])
        print(f'{edition}: {len(cases)}개 소유자 대조 통과', flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 PE 사제 소유자 지정 전체. 대체 함수 없음. B=공통 지정 진입, N=무상태 알림 진입.\n'
        '# Owner 판본 fort battle loading fortPlayer battlePlayer 원래단어 현재소유자 요청 state extra 사건 슬롯전체\n'
        + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    report = dict(schema=1, primary_target='10.78', total=len(rows), cases=dict(collections.Counter(row[0] for row in rows)),
        editions=reports, stubbed=[], os_calls=0,
        files={p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(paths)},
        limitations=['actual priest type 158 with synthetic raw state and type genus',
            'challenge mode, patch loading/nonbattle owner 0 and CD out-of-range base owners excluded (original assertions)',
            'no postPop, capture gameplay, GUI unit migration or game loop'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f'사제 소유자 전체 {len(rows)}개 저장')


def verify():
    """SHA/행 수와 원본 재정의·공통 지정·알림 진입, 대체/OS 호출 0을 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    # 모든 파일의 입력 무결성을 확인한다.
    for name, expected in report['files'].items():
        if digest(ROOT / name) != expected: raise RuntimeError(f'SHA 불일치: {name}')
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows) != report['total'] or dict(collections.Counter(row[0] for row in rows)) != report['cases'] or report['stubbed'] or report['os_calls']:
        raise RuntimeError('행 수/대체 경계 오류')
    # 사건 기록과 실제 명령 진입 횟수가 일치해야 한다.
    for edition, item in report['editions'].items():
        selected = [row for row in rows if row[1] == edition]
        base = sum(row[-2].count('B:') for row in selected)
        notify = sum(row[-2].count('N:') for row in selected)
        if item['assertions'] or len(selected) != item['cases'] or item['native_calls'][item['entry']] != len(selected):
            raise RuntimeError(f'사제 실행 증거 오류: {edition}')
        if item['native_calls'][item['base_entry']] != base or item['native_calls'][item['notify_entry']] != notify:
            raise RuntimeError(f'가상 효과 관찰 횟수 오류: {edition}')
    print(f'priestowner 검증 통과: {len(rows)}개')


def main():
    """전체 생성, 저장된 기록 감사, 무저장 소규모 검사 중 하나를 선택한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args()
    if args.verify: verify()
    else: generate(args.smoke)


if __name__ == '__main__':
    main()
