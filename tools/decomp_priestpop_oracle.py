#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 사제 vtable의 공통 Pop→firstPop/Activate→사제 postPop 전체 반환을 세 PE에서 대조한다.

공간/단계/활성화/사제 조건과 접두는 원본 명령이다. Carrier/Regular/낙하/보호막은 명시 경계다.
게임/OS/창은 실행하지 않는다. --verify는 저장된 원본/도구/내보내기 SHA와 입력/반환 근거를 감사한다.
"""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_priestpostpoptail_oracle import (TailOracle, POST, PRIEST_TYPE, ROOT, SPECS, TYPES,
    LISTS, TARGET, STACK, STOP, SPOTS, CODES, CONTROLS, digest)
from decomp_owner_oracle import CAPACITY
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 원본 네 해시 배열과 합성 SHP는 기존 보호막/목록/코드 영역과 겹치지 않는다.
HEADS, SHAPE = 0x14000000, 0x15000000
# 판본별 공통 Pop/Activate/firstPop 및 공간/표시/권한 전역이다.
SPACE = {
    'originals': dict(pop=0x4b02d0, activate=0x4ac560, first=0x4ad470, hash=0x542508,
        board=0x531928, dirty=0x5949d0, display=0x59a8b0, depth=0x5c8488,
        multiplayer=0x540bc0, capacity=0x5c847c, foot=0x1d4),
    'originalCD': dict(pop=0x4ad490, activate=0x4abfb0, first=0x4ae0f0, hash=0x5670c0,
        board=0x52e9a8, dirty=0x51c54c, display=0x52039c, depth=0x539600,
        multiplayer=0x540a28, capacity=0x5395f4, foot=0x1b4),
}
SPACE['original1037'] = SPACE['originalCD']
# 최초/재등록, 추상/매몰/강제 이동 불가와 원본 flags를 교차한다.
KEYS = ('extra', 'flags', 'hp', 'ground', 'side', 'found', 'allocated')
FIXTURE = ROOT / 'cpppj/tests/fixtures/priestpop-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-priestpop-evidence.json'


def inputs():
    """외부 효과의 판단을 만들지 않고 순서가 고정된 raw/지면/flags 입력을 공급한다."""
    return list(itertools.product((0, 1, 8, 9, 128, 129, 136, 160), (0, 8, 0x50, 0x800, 0x2000),
        (20, 150), (0, 6), (65, 74), (0, 1), (0, 1)))


class PriestPopOracle(TailOracle):
    """기존 사제 후처리 실행기에 실제 공간 Pop/Activate/firstPop 진입을 추가한다."""
    def __init__(self, edition):
        """실제 vtable을 유지하고 현재 호스트의 읽기 전용 공통 몸체와 공간 입력을 연결한다."""
        super().__init__(edition)
        self.space = SPACE[edition]
        self.mu.mem_map(HEADS, 0x40000); self.mu.mem_map(SHAPE, 0x1000)
        self.bases, cursor = [], HEADS
        # 원본의 네 단계 머리 배열 크기와 순서를 유지한다.
        for scale in (1, 2, 4, 16):
            self.bases.append(cursor); cursor += (256 // scale) ** 2 * 2
        self.exports.extend(ROOT / f'extracted/priestpop/{edition}/{n}' for n in ('creation.c', 'functions.tsv'))
        with self.exports[-1].open(encoding='utf-8') as fp:
            # 내보낸 공통 함수의 실제 불연속 범위만 허용한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                # 각 몸체 구간 사이의 임의 코드는 허용하지 않는다.
                for part in row['ranges'].split(';'):
                    low, high = (int(v, 16) for v in part.split('-'))
                    self.allowed.append((low, high + 1))
        vt = self.p['vtable']
        self.vtable = {f'{off:02x}': f'{self.u32(vt+off):08x}' for off in (0x20, 0x30, 0x48, 0x4c, 0x88, 0x8c, 0x90)}
        if self.u32(vt+0x20) != self.p['entry'] or self.u32(vt+0x30) != self.space['first'] or self.u32(vt+0x90) != self.space['pop']:
            raise RuntimeError('실제 사제 vtable의 postPop/firstPop/Pop 대응 오류')

    def on_instruction(self, mu, address, size, data):
        """Carrier 경계에 실제 flags를 기록하고 공통 base의 깊이 감소만 명시 입력으로 공급한다."""
        if address == self.p['carrier']:
            if mu.reg_read(UC_X86_REG_ECX) != self.slot(TARGET): raise RuntimeError('Carrier this 오류')
            flags = self.u32(mu.reg_read(UC_X86_REG_ESP)+4)
            self.stub_calls[f'{address:08x}'] += 1
            self.events.append(f'C:{TARGET}:{flags}')
            depth = self.space['depth']
            self.mu.mem_write(depth, struct.pack('<I', (self.u32(depth)-1) & 0xffffffff))
            self.ret(4); return
        TailOracle.on_instruction(self, mu, address, size, data)

    def run_pop(self, values, control):
        """실제 Pop 진입부터 ABI/깊이/SEH/x87 정상 반환과 슬롯/해시/목록을 관찰한다."""
        c = self.case = dict(zip(KEYS, values)); c['change'] = 0
        p, s, raw = self.p, self.space, self.slot(TARGET)
        data = bytearray(self.stride); struct.pack_into('<I', data, 0, p['vtable'])
        data[10], data[11], data[self.o['owner']], data[self.o['extra']] = PRIEST_TYPE, 4, 1, c['extra']
        struct.pack_into('<H', data, 12, 1)
        struct.pack_into('<I' if self.stride == 50 else '<H', data, 26, c['hp'])
        self.mu.mem_write(raw, bytes(data))
        base = TYPES + PRIEST_TYPE * self.type_stride
        self.mu.mem_write(base, bytes(self.type_stride)); self.mu.mem_write(base, struct.pack('<i', 200))
        self.mu.mem_write(base+0xe8, struct.pack('<II', 0x69012, 0x210000))
        self.mu.mem_write(base+0xdc, struct.pack('<I', SHAPE)); self.mu.mem_write(base+0x114, struct.pack('<I', 1))
        self.mu.mem_write(base+0x124, struct.pack('<I', CODES)); self.mu.mem_write(base+s['foot'], struct.pack('<II', 1, 1))
        self.mu.mem_write(CODES, bytes([c['side'], 80, 1, 0]))
        self.mu.mem_write(SHAPE+8, struct.pack('<I', 0x100)); self.mu.mem_write(SHAPE+0x100-36, struct.pack('<ff', 1, 1))
        self.mu.mem_write(HEADS, bytes(0x40000)); self.mu.mem_write(SPOTS, bytes(65536))
        self.mu.mem_write(SPOTS+21*256+20, bytes([c['ground']]))
        self.mu.mem_write(SPOTS+22*256+21, bytes([c['ground']]))
        self.mu.mem_write(s['hash'], struct.pack('<7I', 0, 1, 256, *self.bases))
        # 싱글플레이/표시 억제/빈 dirty 큐의 실제 공통 경로를 사용한다.
        for address, value in ((s['board'], 256), (s['capacity'], CAPACITY), (s['dirty'], 0),
                (s['display'], 1), (s['depth'], 0), (s['multiplayer'], 0), (p['priest_type'], PRIEST_TYPE), (p['quarter'], 0)):
            self.mu.mem_write(address, struct.pack('<I', value))
        self.mu.mem_write(0x5e4794 if self.stride == 50 else 0x546778, bytes(4))
        self.mu.mem_write(p['list'], struct.pack('<III', LISTS, 4, 0)); self.mu.mem_write(LISTS, struct.pack('<4I', 60, 50, 70, 80))
        self.mu.mem_write(p['owners'], struct.pack('<9I', *[0xabc000+i for i in range(9)]))
        self.write_ranges = [(0, 4), (raw, raw+self.stride), (HEADS, HEADS+172544),
            (LISTS, LISTS+16), (p['list']+8, p['list']+12), (p['owners'], p['owners']+36), (s['depth'], s['depth']+4)]
        self.mu.mem_write(0, struct.pack('<I', 0x12345678)); self.events = []
        saved = ((UC_X86_REG_EBX, 0x11223344), (UC_X86_REG_ESI, 0x22334455), (UC_X86_REG_EDI, 0x33445566), (UC_X86_REG_EBP, 0x44556677))
        # 원본 callee가 비영 보존 레지스터를 복구하는지 확인한다.
        for reg, value in saved: self.mu.reg_write(reg, value)
        # 휘발 레지스터의 입력도 고정한다.
        for reg in (UC_X86_REG_EAX, UC_X86_REG_EDX): self.mu.reg_write(reg, 0)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2); self.mu.reg_write(UC_X86_REG_FPCW, control); self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK); self.mu.reg_write(UC_X86_REG_ECX, raw)
        self.mu.mem_write(STACK, struct.pack('<IIII', STOP, 0x41a60000, 0x41af3333, c['flags']))
        self.mu.emu_start(s['pop'], STOP, count=20000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK+16:
            raise RuntimeError('Pop 정상 반환/스택 오류')
        if any(self.mu.reg_read(reg) != value for reg, value in saved) or self.u32(0) != 0x12345678 or self.u32(s['depth']) != 0:
            raise RuntimeError('Pop 보존 레지스터/SEH/공통 깊이 오류')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError('Pop x87 상태 오류')
        self.returns += 1
        head = int.from_bytes(self.mu.mem_read(self.bases[1]+(21//2*128+20//2)*2, 2), 'little')
        return [';'.join(self.events) or '-', bytes(self.mu.mem_read(raw, self.stride)).hex(), head,
            str(self.u32(p['list']+8))+':'+','.join(map(str, struct.unpack('<4I', self.mu.mem_read(LISTS, 16)))),
            ','.join(map(str, struct.unpack('<9I', self.mu.mem_read(p['owners'], 36))))]


def generate(smoke=False):
    """세 실제 PE의 두 정밀도에서 관찰이 같은 입력만 별도 fixture로 저장한다."""
    cases, rows, editions = inputs(), [], {}
    if smoke: cases = cases[::79]
    paths = {Path(__file__), FIXTURE, ROOT/'tools/ghidra/priestpop-functions.json'}
    # 실제 실행에 의존한 부모 도구를 SHA 근거에 포함한다.
    for name in ('owner', 'priestpostpop', 'prieststate', 'priestpostpoptail'):
        paths.add(ROOT/f'tools/decomp_{name}_oracle.py')
    # 판본별 PE/가상 표/불연속 내보내기 몸체는 독립적으로 읽는다.
    for edition in SPECS:
        oracle = PriestPopOracle(edition)
        # 목표 슬롯/정규화 flags/낙하 판정을 Python에서 계산하지 않는다.
        for case in cases:
            first, second = (oracle.run_pop(case, control) for control in CONTROLS)
            if first != second: raise RuntimeError('두 x87 정밀도 관찰 불일치')
            rows.append([edition, *case, *first])
        editions[edition] = dict(cases=len(cases), returns=oracle.returns, native_calls=dict(oracle.native_calls),
            vtable=oracle.vtable, stub_calls=dict(oracle.stub_calls), assertions=oracle.assertions, instructions=oracle.instructions)
        paths.update(oracle.exports); paths.add(ROOT/oracle.spec['binary'])
        print(f'{edition}: 실제 사제 Pop/Activate {len(cases)}개 통과', flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 Pop/Activate/사제 postPop 전체. Carrier/Regular/낙하/보호막은 명시 경계.\n'
        '# edition extra flags hp ground side found allocated events raw hashHead list owners\n'
        +'\n'.join('\t'.join(map(str, row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-10',total=len(rows),
        editions=editions,controls=list(CONTROLS),os_calls=0,
        stubbed=['Carrier 후처리와 base 깊이 감소', 'Regular 검색/확보/생성', '낙하/보호막 생성/삭제'],
        limitations=['합성 타입/프레임/SHP/지면', '비멀티플레이/표시 억제', 'GUI/소리/보호막 공간 수명 제외'],
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """파일 SHA·원본 vtable/실제 진입·정상 반환·정확한 입력 순서를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    if report['schema']!=1 or report['controls']!=list(CONTROLS) or set(report['editions'])!=set(SPECS):
        raise RuntimeError('스키마/정밀도/판본 오류')
    # 저장된 원본 PE·실행기·내보내기·fixture를 모두 확인한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected: raise RuntimeError(f'SHA 불일치: {name}')
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    expected=inputs()
    if report['os_calls'] or len(rows)!=len(expected)*3 or report['total']!=len(rows): raise RuntimeError('행 수/OS 오류')
    if any(len(row)!=13 for row in rows): raise RuntimeError('관찰 열 개수 오류')
    # 각 판본은 동일한 전체 입력을 누락/중복 없이 정상 반환해야 한다.
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if [tuple(map(int,row[1:8])) for row in selected]!=expected or item['cases']!=len(expected) or item['returns']!=len(expected)*2 or item['assertions']:
            raise RuntimeError(f'입력/반환/assert 오류: {edition}')
        if item['native_calls'].get(f"{SPACE[edition]['first']:08x}")!=len(expected):
            raise RuntimeError(f'실제 최초 등록 진입 오류: {edition}')
        # 공통 Pop/Activate와 사제 wrapper 모두 정상 반환마다 실제 진입해야 한다.
        for address in (SPACE[edition]['pop'],SPACE[edition]['activate'],POST[edition]['entry']):
            if item['native_calls'].get(f'{address:08x}')!=item['returns']: raise RuntimeError(f'실제 진입 오류: {edition}')
    print(f'priestpop 검증 통과: {len(rows)}개')


def main():
    """원본 관찰 생성/저장된 감사/일부 입력의 비저장 점검을 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true'); parser.add_argument('--smoke',action='store_true')
    args=parser.parse_args()
    if args.verify: verify()
    else: generate(args.smoke)


if __name__=='__main__': main()
