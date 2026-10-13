#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 일반 지형의 통로·청크별 성장·빈틈 보정을 보호 두 PE의 같은 context에서 실행한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_terraincontext_oracle import ContextOracle, ROOT, MAP, HEAP, STOP, STACK, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW

# 원본 ChunkMap의12바이트 항목은 지도/stack/native metadata와 겹치지 않는 heap 일부에 둔다.
CHUNKS = HEAP + 0x4000
# 실제 일반 생성 root와 CD의 단일 청크 wrapper를 현재 정밀 내보내기에서 실행한다.
SPECS = {'originals': {'root': 0x4c10a0, 'helpers': ()},
         'originalCD': {'root': 0x49e160, 'helpers': (0x4a59a0,)}}
# 두 x87 정밀도는 동일 고유 입력의 반복 대조이며 새 입력에 두 번 더하지 않는다.
CONTROLS = (0x027f, 0x037f)
FIXTURE = ROOT / 'cpppj/tests/fixtures/terraingrowth-x86.tsv'
REPORT = ROOT / 'cpppj/terraingrowth.json'


def inputs():
    """빈/단일/연결/다른 지역 청크와 선택 지역·seed를 기대 결과 계산 없이 교차한다."""
    return list(itertools.product(range(6), (0, 7, 0xffffffff), (127, 0, 1)))


def scene(profile, seed):
    """입력 청크 목록만 만든다. 원본 성장/지면 결과를 계산하지 않는다."""
    positions = ((), ((3, 3),), ((3, 3), (4, 3), (3, 4), (4, 4)),
                 ((3, 3), (4, 3), (3, 4), (4, 4)),
                 ((3, 3), (4, 3), (5, 3), (3, 4), (4, 4), (5, 4)), ((3, 3), (4, 4)))[profile]
    result = []
    # 원본 입력의 열별 지역과 방향/seed만 고정한다.
    for index, (x, y) in enumerate(positions):
        island = (x - 3) & 1 if profile in (3, 5) else 0
        result.append((x, y, island, ord('A') + (profile * 3 + index) % 16,
                       (seed + (x - 3) * 13 + (y - 3) * 19) & 255))
    return result


def rle(data):
    """관찰한 전체65,536BYTE를 손실 없이 연속값/길이로 기록한다."""
    return ','.join(f'{value:02x}:{len(list(group))}' for value, group in itertools.groupby(data))


class GrowthOracle(ContextOracle):
    """실제 context와 현재 raw 청크 저장소에서 좁은 정밀 성장 몸체만 실행한다."""
    def __init__(self, edition):
        """기존 보호 PE/쓰기 감시를 재사용하고 이 단위의 명시 범위를 추가한다."""
        super().__init__(edition)
        self.growth = SPECS[edition]
        self.growth_returns = 0
        self.growth_entries = collections.Counter()
        self.control = CONTROLS[0]
        self.entries = set()
        paths = [ROOT / f'extracted/terraingrowth/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.exports.extend(paths)
        with paths[1].open(encoding='utf-8') as stream:
            # 내보낸 불연속 범위만 허용하며 이전 helper 완료 개수는 추가하지 않는다.
            for row in csv.DictReader(stream, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                for span in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in span.split('-'))
                    self.allowed_bytes[low-0x400000:high+1-0x400000] = b'\x01' * (high+1-low)
        full = ROOT / f'extracted/refined/{edition}/functions.tsv'
        self.exports.append(full)
        with full.open(encoding='utf-8') as stream:
            selected = [row for row in csv.DictReader(stream, delimiter='\t')
                        if int(row['entry'], 16) in self.growth['helpers']]
        if len(selected) != len(self.growth['helpers']):
            raise RuntimeError('일반 성장의 현재 호스트 PRNG helper 범위 누락')
        self.selected_helpers = selected
        # 새 narrow export에 없는 실제 PRNG core 한 몸체만 현재 전체 정밀 metadata에서 선택한다.
        for row in selected:
            self.entries.add(int(row['entry'], 16))
            for span in row['ranges'].split(';'):
                low, high = (int(value, 16) for value in span.split('-'))
                self.allowed_bytes[low-0x400000:high+1-0x400000] = b'\x01' * (high+1-low)
        # 초기 stamp의 명시 ChunkRegion 경계를 사용하지 않고 여기서는 실제 ChunkMap getter를 실행한다.
        self.s = dict(self.s)
        self.s['chunk'] = 0

    def instruction(self, mu, address, size, data):
        """현재 새 root/helper의 실제 진입을 세고 기존 제한 실행기에 주소를 전달한다."""
        if address in self.entries:
            self.growth_entries[f'{address:08x}'] += 1
        super().instruction(mu, address, size, data)

    def call(self, address, arguments):
        """actual this/입력·두 x87 정밀도·반환 스택과 넉넉한 명령 한계를 확인한다."""
        mu = self.mu
        # 앞선 입력의 범용 레지스터를 새로운 입력에 가져오지 않는다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            mu.reg_write(register, 0)
        mu.reg_write(UC_X86_REG_ECX, MAP)
        mu.reg_write(UC_X86_REG_ESP, STACK)
        mu.reg_write(UC_X86_REG_EFLAGS, 2)
        mu.reg_write(UC_X86_REG_FPCW, self.control)
        mu.reg_write(UC_X86_REG_FPSW, 0)
        mu.mem_write(STACK, struct.pack('<'+'I'*(len(arguments)+1), STOP,
                                      *[value & 0xffffffff for value in arguments]))
        mu.emu_start(address, STOP, count=10000000)
        if mu.reg_read(UC_X86_REG_EIP) != STOP or mu.reg_read(UC_X86_REG_ESP) != STACK+4+len(arguments)*4:
            raise RuntimeError(f'일반 성장 반환/스택 오류: {self.case}')
        if mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError('일반 성장 x87 스택 오류')

    def run_growth(self, case, control):
        """원본 청크/초기 배열만 공급하고 실제 일반 생성의 전체 출력과 난수를 관찰한다."""
        self.case = case
        self.control = control
        self.events = []
        self.assertions = 0
        self.heap = HEAP
        profile, seed, selected = case
        self.mu.mem_write(MAP, b'\xa5'*0x21000)
        self.mu.mem_write(HEAP, bytes(0x10000))
        self.put(self.context['battle'], 0)
        self.put(0x50c038 if self.edition == 'originals' else 0x505eb0, 127)
        self.put(self.s['isle'], 97)
        self.put(self.s['edge'], 98)
        self.put(self.s['rng'], seed)
        self.put(CHUNKS, 1)
        self.put(CHUNKS+4, 16)
        self.put(CHUNKS+8, 16)
        # 실제 청크256개는12바이트 항목 첫BYTE를 noRegion으로 초기화한다.
        for index in range(256):
            self.put(CHUNKS+12+index*12, 127, 1)
        # 지형 결과 없이 명시 입력 청크의 섬/방향/record seed만 공급한다.
        for x, y, island, side, chunk_seed in scene(profile, seed):
            address = CHUNKS+12+(x*16+y)*12
            self.put(address, island, 1)
            self.put(address+1, side, 1)
            self.put(address+5, chunk_seed, 1)
        right = 6 if profile == 4 else 5
        self.call(self.context['ctor'], (1, selected, 3, 3, right, 5, CHUNKS))
        if profile == 5:
            # Fill의 현재x/다른지역 유지와 특수문자 덮어쓰기 입력이다.
            self.cell(56, 56, ord('x'), 3)
            self.cell(57, 56, ord('='), 3)
        self.call(self.growth['root'], ())
        offset = MAP+self.s['offset']
        codes = bytes(self.mu.mem_read(offset, 65536))
        regions = bytes(self.mu.mem_read(offset+0x10000, 65536))
        self.growth_returns += 1
        return (self.read(self.s['rng']), rle(codes), rle(regions), self.assertions)


def generate(smoke):
    """전체 원본 배열을 두 정밀도에서 재대조하고 현재 출처/고유 입력 보고서를 저장한다."""
    cases = inputs()
    if smoke:
        cases = cases[::31]
    rows = []
    editions = {}
    files = {Path(__file__).resolve(), ROOT/'tools/decomp_terraincontext_oracle.py',
             ROOT/'tools/decomp_terrainstamp_oracle.py', ROOT/'tools/ghidra/terraingrowth-functions.json',
             ROOT/'cpppj/src/o/RawTerrainGrowth.h', ROOT/'cpppj/src/o/RawTerrainGrowth.cpp'}
    # 각 판본의 고유 input tuple은 한 번만 fixture에 보관한다.
    for edition in SPECS:
        oracle = GrowthOracle(edition)
        for case in cases:
            before = oracle.run_growth(case, CONTROLS[0])
            repeated = oracle.run_growth(case, CONTROLS[1])
            if repeated != before:
                raise RuntimeError(f'일반 성장 x87 출력 차이: {edition}/{case}')
            if before[-1]:
                raise RuntimeError(f'일반 성장 정상 입력 assertion: {edition}/{case}')
            if before[0] != case[1]:
                raise RuntimeError(f'일반 성장의 예상 밖 게임 전역 난수 소비: {edition}/{case}')
            rows.append((edition, ','.join(map(str, case)), *before))
        editions[edition] = {'normal_cases': len(cases), 'returns': oracle.growth_returns,
                             'entries': dict(oracle.growth_entries), 'external': dict(oracle.external),
                             'selected_current_helper_ranges': oracle.selected_helpers}
        files.update(oracle.exports)
        files.add(ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'))
    FIXTURE.parent.mkdir(parents=True, exist_ok=True)
    FIXTURE.write_text('# 실제 일반 지형·전체배열RLE·게임난수·assert. 반복x87는새고유입력아님.\n'
                       + '\n'.join('\t'.join(map(str, row)) for row in rows)+'\n', encoding='utf-8', newline='\n')
    files.add(FIXTURE)
    data = {'host': 'HJOW-Athlon', 'total': len(rows), 'normal_total': len(rows), 'assertion_total': 0,
            'editions': editions, 'smoke': smoke, 'game_runs': 0,
            'files': {path.relative_to(ROOT).as_posix(): digest(path) for path in sorted(files)},
            'limits': ['일반 초기 지형 생성의 실제 code/region/ChunkMap 조회/PRNG 몸체만 실행한다.',
                       'archipelago wrapper·지면 Factory/Stamp·edgeFarm·GUI/게임 완주 관찰이 아니다.',
                       '같은 입력의 x87 반복·기존 초기 context/stamp의 완료 입력은 합산하지 않는다.']}
    REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps({'normal_total': len(rows), 'editions': editions}, ensure_ascii=False))


def verify():
    """현재 보호 PE/좁은 내보내기·전체 native 출력과 새 구현 출처 바이트를 확인한다."""
    data = json.loads(REPORT.read_text(encoding='utf-8'))
    # 보고서의 원본과 구현 출처는 현재 파일마다 SHA로 비교한다.
    for name, expected in data['files'].items():
        if digest(ROOT/name) != expected:
            raise RuntimeError(f'일반 성장 출처 SHA 차이: {name}')
    rows = [line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows) != data['total'] or data['assertion_total'] != 0:
        raise RuntimeError('일반 성장 고유 입력/정상 구분 오류')
    print(f"일반 성장 원본 전체배열/현재SHA 감사 통과: {data['total']}")


def main():
    """기본 실행은 전수 생성, --smoke는 소수 입력, --verify는 현재 저장 근거 감사다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--smoke', action='store_true')
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    verify() if args.verify else generate(args.smoke)


if __name__ == '__main__':
    main()
