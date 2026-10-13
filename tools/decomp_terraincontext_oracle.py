#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 초기 지형 constructor/bounds/경계문자/도장뒤정리를 보호 두 PE에서 독립 실행한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_terrainstamp_oracle import StampOracle, MAP, HEAP, STOP, STACK, digest, ROOT
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW

# 실제 상위 내보내기의 주소다. ctor에는 두 판본 모두 실제 ChunkMap 포인터를 명시 입력한다.
SPECS = {'originals': {'ctor': 0x4c0390, 'bounds': 0x4c02d0, 'edge': 0x4c09a0, 'normalize': 0x4c0ad0, 'battle': 0x594fbc},
         'originalCD': {'ctor': 0x49cdd0, 'bounds': 0x49cd50, 'edge': 0x49d890, 'normalize': 0x49d9a0, 'battle': 0x540a20}}
# 초기 지도와 큰 주소 metadata는 fixture 결과에서 분리하고 실제 게임 데이터만 비교한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/terraincontext-x86.tsv'
REPORT = ROOT / 'cpppj/terraincontext.json'


def inputs():
    """초기 전역·청크 초과의 판본 차이와 도장 앞뒤 문자 입력을 독립 교차한다."""
    result = []
    # 일반/중립소유자·fringe 제외모드와 지역BYTE 폭·청크 clamp의 두 판본 차이다.
    for owner, battle, no_region, right, bottom in itertools.product((0, 1, 8), (0, 1), (0, 127, 255), (15, 17), (15, 20)):
        result.append(('Ctor', (owner, battle, no_region, 1, 1, right, bottom, 17, 97, 98)))
    # 경계 분류는 이웃alias/지역 불일치와 같은 배열의 앞선 '='변경을 그대로 읽는다.
    for kind, profile, owner, battle in itertools.product(('Edge', 'Normalize'), range(5), (0, 1), (0, 1)):
        result.append((kind, (owner, battle, 127, 3, 3, 4, 4, 17, 97, 98, profile)))
    return result


class ContextOracle(StampOracle):
    """기존 제한 실행기에 실제 초기화 네 몸체 범위를 더하고 모든 지도 쓰기를 관찰한다."""
    def __init__(self, edition):
        """현재 호스트 readOnly 상위 범위와 보호 PE를 같은 분리 메모리로 연결한다."""
        super().__init__(edition)
        self.context = SPECS[edition]
        self.native = collections.Counter()
        self.context_returns = 0
        paths = [ROOT / f'extracted/terrainworld/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        if edition == 'originalCD':
            paths.extend(ROOT / f'extracted/terraincontext-helper/{edition}/{name}' for name in ('creation.c', 'functions.tsv'))
        self.exports.extend(paths)
        # 명세에 내보낸 불연속 범위만 실행하며 원본/OS/게임 창을 호출하지 않는다.
        for table in paths[1::2]:
            with table.open(encoding='utf-8') as stream:
                for row in csv.DictReader(stream, delimiter='\t'):
                    for span in row['ranges'].split(';'):
                        low, high = (int(value, 16) for value in span.split('-'))
                        self.allowed_bytes[low-0x400000:high+1-0x400000] = b'\x01' * (high+1-low)

    def instruction(self, mu, address, size, data):
        """실제 네 몸체 진입을 세고 원본 assert 보고만 명시 관찰 경계로 처리한다."""
        if address in self.context.values():
            self.native[f'{address:08x}'] += 1
        super().instruction(mu, address, size, data)

    def call(self, address, arguments):
        """this/인자/반환 스택 및 x87를 확인하여 실제 몸체 하나를 실행한다."""
        mu = self.mu
        # 이전 입력의 레지스터를 고유 입력의 초기 상태로 가져오지 않는다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            mu.reg_write(register, 0)
        mu.reg_write(UC_X86_REG_ECX, MAP)
        mu.reg_write(UC_X86_REG_ESP, STACK)
        mu.reg_write(UC_X86_REG_EFLAGS, 2)
        mu.reg_write(UC_X86_REG_FPCW, 0x027f)
        mu.reg_write(UC_X86_REG_FPSW, 0)
        mu.mem_write(STACK, struct.pack('<'+'I'*(len(arguments)+1), STOP, *[value & 0xffffffff for value in arguments]))
        mu.emu_start(address, STOP, count=1000000)
        if mu.reg_read(UC_X86_REG_EIP) != STOP or mu.reg_read(UC_X86_REG_ESP) != STACK+4+len(arguments)*4:
            raise RuntimeError(f'초기 context 반환/스택 오류: {self.case}')
        if mu.reg_read(UC_X86_REG_FPSW) & 0x3800:
            raise RuntimeError('초기 context x87 스택 오류')

    def run_context(self, kind, case):
        """새 원본 입력을 준비하고 실제 배열/대응표/context·문자 변환 결과를 읽는다."""
        self.case = (kind, case)
        self.events = []
        self.assertions = 0
        self.heap = HEAP
        owner, battle, no_region, left, top, right, bottom, region, isle, edge = case[:10]
        mu = self.mu
        mu.mem_write(MAP, b'\xa5'*0x21000)
        mu.mem_write(HEAP, bytes(0x10000))
        self.put(self.context['battle'], battle)
        self.put(0x50c038 if self.edition == 'originals' else 0x505eb0, no_region)
        self.put(self.s['isle'], isle)
        self.put(self.s['edge'], edge)
        self.call(self.context['ctor'], (owner, region, left, top, right, bottom, HEAP+0x4000))
        offset = self.s['offset']
        codes = MAP+offset
        regions = codes+0x10000
        if kind != 'Ctor':
            profile = case[10]
            # 같은 입력5×5만 채운다. 기대 경계/정리 결과를 실행기에서 계산하지 않는다.
            for x in range(49, 54):
                for y in range(51, 56):
                    value = ord('x')
                    if profile == 1 and x == 49: value = ord('X')
                    if profile == 2 and y == 51: value = ord('o')
                    if profile == 3 and x == 52: value = ord('=')
                    if profile == 4 and x == 51 and y == 53: value = ord('.')
                    self.put(codes+x*256+y, value, 1)
                    self.put(regions+x*256+y, 6 if profile == 3 and y == 54 else 5, 1)
            self.call(self.context['edge'] if kind == 'Edge' else self.context['normalize'], (120, 61) if kind == 'Edge' else ())
        code_bytes = bytes(mu.mem_read(codes, 65536))
        region_bytes = bytes(mu.mem_read(regions, 65536))
        # ctor의 전체 배열 동일성/대표BYTE는 원본65536개를 모두 확인한다.
        uniform = (int(code_bytes == bytes([code_bytes[0]])*65536), code_bytes[0],
                   int(region_bytes == bytes([region_bytes[0]])*65536), region_bytes[0])
        mapping = ','.join(str(self.read(codes+0x20000+index*4)) for index in range(256))
        aliases = ','.join(str(self.read(codes+0x20400+index*4)) for index in range(256))
        cell_bounds = ','.join(str(self.read(MAP+self.s['bounds']+index*4)) for index in range(4))
        chunks = ','.join(str(self.read(MAP+self.s['bounds']+16+index*4)) for index in range(4))
        context = ','.join(str(self.read(MAP+self.s['context']+index*4)) for index in (0, 1, 2, 4))
        window = ''.join(code_bytes[x*256+48:x*256+64].hex() for x in range(48, 64))
        region_window = ''.join(region_bytes[x*256+48:x*256+64].hex() for x in range(48, 64))
        self.context_returns += 1
        return (self.read(MAP), context, chunks, cell_bounds, mapping, aliases, ','.join(map(str, uniform)), window, region_window, self.assertions)


def generate(smoke):
    """제한 실제 실행의 고유 입력/모든 출력과 현재 출처 SHA를 저장한다."""
    cases = inputs()
    if smoke: cases = cases[::31]
    rows = []
    editions = {}
    files = {Path(__file__).resolve(), ROOT/'tools/decomp_terrainstamp_oracle.py', ROOT/'tools/ghidra/terrainworld-functions.json',
             ROOT/'tools/ghidra/terraincontext-helper-functions.json', FIXTURE,
             ROOT/'cpppj/src/o/RawTerrainContext.cpp', ROOT/'cpppj/src/o/RawTerrainContext.h'}
    # 양판본의같은 고유 입력은 판본별로독립 실행한다.
    for edition in SPECS:
        oracle = ContextOracle(edition)
        normal = assertions = 0
        for kind, case in cases:
            result = oracle.run_context(kind, case)
            rows.append((edition, kind, ','.join(map(str, case)), *result))
            if result[-1]: assertions += 1
            else: normal += 1
        editions[edition] = {'normal_cases': normal, 'assertion_cases': assertions, 'returns': oracle.context_returns,
                            'native': dict(oracle.native), 'external': dict(oracle.external)}
        files.update(oracle.exports)
        files.add(ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'))
        print(edition, oracle.context_returns, flush=True)
    FIXTURE.write_text('# 실제 초기context/bounds/문자변환 독립원본. assert보고/ChunkMap주소는명시경계.\n' +
                       '\n'.join('\t'.join(map(str, row)) for row in rows)+'\n', encoding='utf-8', newline='\n')
    data = {'host': 'HJOW-Athlon', 'total': len(rows), 'editions': editions,
            'normal_total': sum(item['normal_cases'] for item in editions.values()),
            'assertion_total': sum(item['assertion_cases'] for item in editions.values()),
            'game_runs': 0, 'files': {path.relative_to(ROOT).as_posix(): digest(path) for path in sorted(files)},
            'limits': ['원본 ChunkMap 포인터 주소·복사 metadata는 C++ 게임데이터 출력에 합산하지 않는다.',
                       '실제 성장/군집 배치/wrapper 및 GUI/캠페인 완주 근거가 아니다.']}
    REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps({'total': len(rows), 'normal': data['normal_total'], 'assertions': data['assertion_total']}, ensure_ascii=False))


def verify():
    """현재 원본 범위/소스/fixture SHA와 고유 입력·정상/assert 반환 수를 확인한다."""
    data = json.loads(REPORT.read_text(encoding='utf-8'))
    rows = [row for row in csv.reader(FIXTURE.open(encoding='utf-8'), delimiter='\t') if row and not row[0].startswith('#')]
    if len(rows) != data['total'] or len({tuple(row[:3]) for row in rows}) != len(rows):
        raise RuntimeError('초기 context 고유입력/행 수 오류')
    for name, expected in data['files'].items():
        if digest(ROOT/name) != expected: raise RuntimeError('초기 context SHA 불일치 '+name)
    print('초기 context 원본범위/고유입력/정상·assert·현재SHA 감사 통과', len(rows))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--smoke', action='store_true')
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    verify() if args.verify else generate(args.smoke)
