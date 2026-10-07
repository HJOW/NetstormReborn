#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""소유자 지정(Squid vtable +0x74)을 세 실제 PE의 제한 x86으로 대조한다.

실제 명령으로 실행하는 것: 소유자 지정 몸체(범위 검사·모드 검사·genus 검사·소유자 바이트 쓰기),
타입 genus 조회, 소유자별 작업장 목록의 추가/제거(배열 압축 포함).
대체하는 것은 없다. 원본 assert 보고에 닿는 입력(범위 밖 소유자)은 만들지 않는다.
원본 게임/OS/업데이터는 실행하지 않는다.

python -X utf8 tools/decomp_owner_oracle.py            # 기대값·근거 기록 생성
python -X utf8 tools/decomp_owner_oracle.py --verify   # 저장된 기록·입력 파일의 SHA/행 수 확인
"""
import argparse
import collections
import csv
import hashlib
import importlib.metadata
import json
import random
import struct
import sys
import zlib
from pathlib import Path

# 저장소 루트와 격리된 분석용 Python 의존성 경로.
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

# 에뮬레이터 내부 주소. 호스트 메모리와 무관하며 서로 겹치지 않는다.
TYPES = 0x11100000      # 타입 구조체 배열(번호 0부터)
PLAYERS = 0x11300000    # Player 구조체 배열(번호 0부터). 맨 앞 세 DWORD가 작업장 목록(포인터·용량·개수)이다.
LISTS = 0x11310000      # 소유자별 작업장 목록 저장소(소유자마다 0x100바이트)
POOL = 0x12000000       # Squid 배열
STOP = 0x20000000       # 정상 반환을 확인하는 가상 주소
STACK_BASE, STACK = 0x30000000, 0x3003f000
# 풀 슬롯 수, 타입 수, 소유자 수(0 = 없음, 1~8 = 실제 플레이어), 목록 용량.
CAPACITY, MAX_TYPES, OWNERS, LIST_CAPACITY = 128, 96, 9, 6
# 입력 번호: 소유자를 바꾸는 객체와 목록에 섞어 넣는 다른 번호들.
TARGET, OTHER_A, OTHER_B = 50, 60, 70
# 합성 객체의 타입 번호.
OBJECT_TYPE = 82
# 입력만 만드는 고정 시드. 기대 결과는 원본 기계어가 정한다.
SEED = 0x4adf00
# 판본마다 만드는 표본 수와 한 호출에 허용하는 명령 수.
SAMPLES, MAX_INSTRUCTIONS = 1000, 20000

# 판본별 함수·전역·구조체 배치.
SPECS = {
 'originals': dict(binary='originals/Netstorm.exe', stride=50, type_stride=500, player_stride=0xb4, entry=0x4adf00, assert_report=0x4e0620,
    globals=dict(types=0x59ab20, pool=0x5c8464, players=0x59534c, fort=0x594fb8, battle=0x594fbc, challenge=0x594fc8),
    offsets=dict(owner=0x22, extra=0x28)),
 'originalCD': dict(binary='originalCD/NETSTORM.EXE', stride=36, type_stride=468, player_stride=0xac, entry=0x4aefa0, assert_report=0x490040,
    globals=dict(types=0x51c960, pool=0x5395dc, players=0x50f834, fort=0x540a1c, battle=0x540a20, challenge=0x540a24),
    offsets=dict(owner=0x20, extra=0x23)),
}
# 추가 10.37 실행 파일은 CD판과 같은 코드 배치다.
SPECS['original1037'] = dict(SPECS['originalCD'], binary='original1037/netstorm.exe')
# 새 결과 파일은 이전 기록/fixture와 분리한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/owner-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-owner-evidence.json'


def digest(path):
    """파일의 SHA-256."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class OwnerOracle:
    """PE를 가상 메모리에 올리고 내보낸 함수 몸체와 허용한 쓰기만 실행한다."""
    def __init__(self, edition):
        """판본의 코드 허용 범위, 전역, 메모리 배치를 준비한다. 호스트 메모리와 원본 파일은 수정하지 않는다."""
        self.edition = edition
        self.spec = SPECS[edition]
        binary = ROOT / self.spec['binary']
        self.sha256 = digest(binary)
        pe = pefile.PE(str(binary))
        self.base = pe.OPTIONAL_HEADER.ImageBase
        image = pe.get_memory_mapped_image()
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        self.mu.mem_map(self.base, (len(image) + 4095) & ~4095)
        self.mu.mem_write(self.base, image)
        self.stride, self.type_stride, self.player_stride = self.spec['stride'], self.spec['type_stride'], self.spec['player_stride']
        self.o, self.g = self.spec['offsets'], self.spec['globals']
        for address, size in ((TYPES, 0x20000), (PLAYERS, 0x20000), (POOL, (CAPACITY * self.stride + 4095) & ~4095),
                              (STOP, 0x1000), (STACK_BASE, 0x40000)):
            self.mu.mem_map(address, size)
        # 내보낸 함수의 불연속 몸체만 실행을 허용한다.
        self.exports = [ROOT / f'extracted/owner/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed, self.entries = [], set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 함수마다 Ghidra가 계산한 몸체 범위를 그대로 허용한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))
        for name, value in (('types', TYPES), ('pool', POOL), ('players', PLAYERS)):
            self.mu.mem_write(self.g[name], struct.pack('<I', value))
        self.native_calls = collections.Counter()
        self.assertions = 0
        self.instructions = 0
        self.write_ranges = []
        self.mu.hook_add(UC_HOOK_CODE, self.on_instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.on_write)

    def slot(self, sid):
        """슬롯 주소."""
        return POOL + sid * self.stride

    def on_write(self, _mu, _access, address, size, _value, _data):
        """스택·소유자 바이트·작업장 목록(저장소와 개수) 밖의 쓰기를 거부한다."""
        if STACK_BASE <= address and address + size <= STACK_BASE + 0x40000:
            return
        if any(low <= address and address + size <= high for low, high in self.write_ranges):
            return
        raise RuntimeError(f'허용하지 않은 쓰기: {self.edition} {address:08x}+{size}')

    def on_instruction(self, _mu, address, size, _data):
        """내보낸 몸체 안의 명령만 허용한다. assert 보고에 닿으면 입력 계약 위반이다."""
        self.instructions += 1
        if address == self.spec['assert_report']:
            self.assertions += 1
            raise RuntimeError(f'원본 assert 도달: {self.edition}')
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'허용하지 않은 실행 주소: {self.edition} {address:08x}')
        if address in self.entries: self.native_calls[f'{address:08x}'] += 1

    def run(self, case):
        """입력 상태를 만들고 실제 소유자 지정을 한 번 실행한 뒤 (소유자 바이트, 목록들, 슬롯 Adler-32)를 돌려준다."""
        o, g = self.o, self.g
        for name in ('fort', 'battle', 'challenge'):
            self.mu.mem_write(g[name], struct.pack('<I', case[name]))
        self.mu.mem_write(TYPES + OBJECT_TYPE * self.type_stride, bytes(self.type_stride))
        self.mu.mem_write(TYPES + OBJECT_TYPE * self.type_stride + 0xec, struct.pack('<I', case['genus']))
        slot = self.slot(TARGET)
        raw = bytearray(self.stride)
        raw[10], raw[11], raw[o['owner']], raw[o['extra']] = OBJECT_TYPE, case['state'], case['owner'], case['extra']
        self.mu.mem_write(slot, bytes(raw))
        # 소유자마다 목록 저장소 전체(용량까지)와 개수를 쓴다. 개수 밖의 값은 남아 있는 옛 메모리 역할이다.
        self.write_ranges = [(slot + o['owner'], slot + o['owner'] + 1)]
        for owner in range(OWNERS):
            entries, count = case['lists'][owner]
            storage = LISTS + owner * 0x100
            self.mu.mem_write(storage, struct.pack(f'<{LIST_CAPACITY}I', *entries))
            head = PLAYERS + owner * self.player_stride
            self.mu.mem_write(head, struct.pack('<III', storage, case['capacity'][owner], count))
            self.write_ranges += [(storage, storage + LIST_CAPACITY * 4), (head + 8, head + 12)]
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, slot)
        self.mu.mem_write(STACK, struct.pack('<II', STOP, case['player']))
        try:
            self.mu.emu_start(self.spec['entry'], STOP, count=MAX_INSTRUCTIONS)
        except UcError as error:
            raise RuntimeError(f'에뮬레이션 오류 {error}') from error
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 8:
            raise RuntimeError('반환/스택 복구 불일치')
        lists = []
        for owner in range(OWNERS):
            storage, capacity, count = struct.unpack('<III', self.mu.mem_read(PLAYERS + owner * self.player_stride, 12))
            entries = struct.unpack(f'<{LIST_CAPACITY}I', self.mu.mem_read(LISTS + owner * 0x100, LIST_CAPACITY * 4))
            lists.append(f'{count}:' + ','.join(map(str, entries)))
        return (self.mu.mem_read(slot + o['owner'], 1)[0], '|'.join(lists), zlib.adler32(bytes(self.mu.mem_read(slot, self.stride))))


def inputs():
    """원본 판단을 포함하지 않는 시드 고정 표본을 만든다. 범위 밖 소유자(assert 경로)는 넣지 않는다."""
    rng = random.Random(SEED)
    cases = []
    # genus: 작업장 목록 대상(vortex 0x200·factory 0x4000)과 그 밖의 비트 조합.
    genera = [0, 0x200, 0x4000, 0x4200, 0x10004, 0xffffbdff, 0xffffffff, 0x4000 | 0x10000]
    for _ in range(SAMPLES):
        challenge = rng.choice([0, 0, 0, 1])
        lists, capacity = [], []
        # 소유자마다 목록 내용: 대상 번호의 유무·중복·가득 찬 목록을 섞는다.
        for owner in range(OWNERS):
            cap = rng.choice([LIST_CAPACITY, LIST_CAPACITY, 2, 1, 0])
            count = rng.randrange(0, cap + 1)
            entries = [rng.choice([TARGET, TARGET, OTHER_A, OTHER_B, 0, 7]) for _ in range(LIST_CAPACITY)]
            lists.append((entries, count))
            capacity.append(cap)
        cases.append(dict(fort=rng.choice([0, 1]), battle=rng.choice([0, 1]), challenge=challenge, genus=rng.choice(genera),
                          state=rng.choice([0, 0, 4, 2, 6]), extra=rng.choice([0, 0, 8, 1, 9]), owner=rng.randrange(0, OWNERS),
                          # 범위 검사: 평소에는 0~8, 다른 모드에서는 1~39가 유효하다. 두 모드에서 모두 목록이 있는 1~8만 쓴다.
                          player=rng.randrange(1, OWNERS) if challenge else rng.randrange(0, OWNERS), lists=lists, capacity=capacity))
    return cases


def case_columns(case):
    """TSV 입력 칸. 재생 쪽이 같은 순서로 읽는다."""
    lists = '|'.join(f'{case["capacity"][owner]}:{count}:' + ','.join(map(str, entries)) for owner, (entries, count) in enumerate(case['lists']))
    return [case['fort'], case['battle'], case['challenge'], case['genus'], case['state'], case['extra'], case['owner'], case['player'], lists]


def generate():
    """세 실제 PE에서 새 fixture·SHA 근거를 만든다."""
    rows = []
    counts = collections.Counter()
    reports = {}
    cases = inputs()
    for edition in SPECS:
        oracle = OwnerOracle(edition)
        for case in cases:
            owner, lists, adler = oracle.run(case)
            rows.append(['Owner', edition, *case_columns(case), owner, lists, adler])
            counts['Owner'] += 1
        reports[edition] = dict(binary=oracle.spec['binary'], sha256=oracle.sha256, entry=f'{oracle.spec["entry"]:08x}',
            native_calls=dict(oracle.native_calls), assertions=oracle.assertions, instructions=oracle.instructions)
    header = ['# 실제 PE 제한 x86 소유자 지정 기대값. 대체 함수 없음. assert 경로(범위 밖 소유자)는 입력에서 제외.',
              '# Owner 판본 요새모드 전투모드 다른모드 genus state extra 이전소유자 새소유자 목록입력(용량:개수:항목|...) 소유자바이트 목록결과(개수:항목|...) 슬롯']
    FIXTURE.write_text('\n'.join(header) + '\n' + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    paths = [Path(__file__), FIXTURE, ROOT / 'tools/ghidra/owner-functions.json']
    for edition in SPECS:
        paths.extend([ROOT / reports[edition]['binary'], ROOT / f'extracted/owner/{edition}/creation.c',
                      ROOT / f'extracted/owner/{edition}/functions.tsv'])
    report = dict(schema=1, primary_target='10.78', method='Unicorn x86 32-bit; OS/API/file/window execution forbidden',
        unicorn_version=importlib.metadata.version('unicorn'), seed=SEED, cases=dict(counts), total=sum(counts.values()),
        editions=reports, stubbed=[], os_calls=0,
        files={p.relative_to(ROOT).as_posix(): digest(p) for p in paths},
        limitations=['synthetic type genus, object slot and per-owner list storage', 'owner numbers 0..8 only (assert paths and 9..39 of the other mode are not generated)',
                     'no GameWorld; AI/production recalculation after the list change is outside this function'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(dict(cases=dict(counts), total=sum(counts.values())), ensure_ascii=False))


def verify():
    """저장된 입력/도구/fixture/몸체 SHA와 행 수를 확인한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    for name, expected in report['files'].items():
        if digest(ROOT / name) != expected: raise RuntimeError(f'SHA 불일치: {name}')
    rows = [line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    counts = collections.Counter(line.split('\t', 1)[0] for line in rows)
    if dict(counts) != report['cases'] or len(rows) != report['total']: raise RuntimeError('fixture 행 수 불일치')
    if any(item['assertions'] for item in report['editions'].values()): raise RuntimeError('assert 도달')
    print(f'owner 검증 통과: {len(rows)}개')


def main():
    """새 기대값 생성과 저장 결과 감사 중 하나를 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    verify() if args.verify else generate()


if __name__ == '__main__':
    main()
