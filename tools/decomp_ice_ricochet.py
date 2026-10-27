#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78 반사점·난수·표적 선택·후속 Missile 인자를 실제 명령으로 관측한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_owner_oracle import OwnerOracle, ROOT, TYPES, POOL, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

OBJECT, OUTPUT, ALLOC = 0x11318000, 0x1131b000, 0x1131c000  # 서로 분리한 원문128BYTE/좌표 반환/할당 경계다.
KEYS = ('kind', 'seed', 'ox', 'oy', 'x', 'y', 'range', 'target', 'speed', 'nodes', 'mutation')  # 기대값을 계산하지 않는 독립 입력 열이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/ice-ricochet-x86.tsv'  # 새 원문만 추가하며 이전 자료는 덮어쓰지 않는다.
REPORT = ROOT/'cpppj/ice-ricochet-native-20261027.json'  # 두 정밀도/원문 범위/경계/해시 기록이다.
ENTRIES = (0x482080, 0x482190, 0x41bfb0, 0x41ce20, 0x41cdb0, 0x4558c0, 0x4558f0, 0x455930, 0x468e70, 0x4e49c0, 0x4e5e8a, 0x4e5e94, 0x4ee254, 0x4eded0, 0x4edf9a, 0x4ee1b7, 0x4ac550, 0x49a840, 0x41d0d0, 0x40e800, 0x422230, 0x41d7a0, 0x4ac200, 0x4adc60, 0x41cfc0)  # 선택/기하/난수/HP를 대체하지 않는다.


def bits(value):
    """입력 float를 물리 비트로 옮기며 결과를 계산하지 않는다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def sample(**changes):
    """서로 독립인 현재128BYTE/표시 자산/후보 입력을 구성한다."""
    row = dict(kind=0, seed=0, ox=bits(50), oy=bits(50), x=bits(60), y=bits(50), range=bits(5), target=60, speed=bits(20), nodes='-', mutation=0)
    row.update(changes)
    return row


def node(sid=15000, flags1=16, flags2=0, extra=0, x=60, y=50, state=0):
    """후보 한 개의 raw 번호·TYPE 플래그·좌표만 직렬화한다."""
    return ','.join(map(str, (sid, flags1, flags2, extra, bits(x), bits(y), state)))


def inputs():
    """방향/난수/동률·서버번호/비행·추상·HP·현재 필드 재조회 경계를 방문한다."""
    rows = []
    # 0 시드 대체, 부호 전환, 지도 네 끝과 정규화 양쪽을 실제 점 계산으로 방문한다.
    for seed, point, origin, distance in itertools.product((0, 1, 19, 20, 49, 50, 98, 99, 0x12345678, 0xffffffff), ((60, 50), (1, 1), (255, 255), (80.25, 64.75)), ((50, 50), (60, 40), (60, 60), (70, 50)), (0, 5, 255)):
        rows.append(sample(seed=seed, x=bits(point[0]), y=bits(point[1]), ox=bits(origin[0]), oy=bits(origin[1]), range=bits(distance)))
    # 같은 좌표에서는 전달한 출력 bytes와 난수가 그대로 남는다.
    for seed, point in itertools.product((0, 1, 0xffffffff), ((50, 50), (1, 255))):
        rows.append(sample(seed=seed, x=bits(point[0]), y=bits(point[1]), ox=bits(point[0]), oy=bits(point[1])))
    # 상위16비트가 seed인 낮은 입력으로 chance의19/20과98 경계를 포함한다.
    for seed in range(110):
        rows.append(sample(kind=1, seed=seed))
    # 사거리0이면 출력 점이 현재 위치라서 후보 거리·동률을 정확히 분리한다.
    for sid, f1, f2, extra, state, target in itertools.product((1, 4, 5, 14999, 15000, 23999), (0, 16), (0, 0x20000, 0x100000, 0x400000), (0, 1), (0, 1, 2), (60, 15000)):
        rows.append(sample(kind=1, seed=20, range=bits(0), nodes=node(sid, f1, f2, extra, state=state), target=target))
    for nodes in (node(15000)+';'+node(15001), node(15001)+';'+node(15000), node(15000, x=61)+';'+node(15001, x=60.5), node(15000, x=10060), node(15000, x=10059.999), node(15000, x=float('nan'))):
        rows.append(sample(kind=1, seed=20, range=bits(0), nodes=nodes))
    # Next 외부 순회 뒤에는 현재 표적/속도/부모SID와 처음 캡처한 표시 슬롯의 현재값을 읽는다.
    for mutation in range(1, 5):
        rows.append(sample(kind=1, seed=20, range=bits(0), nodes=node(15000)+';'+node(15001, x=61), mutation=mutation))
    return list({tuple(row[key] for key in KEYS): row for row in rows}.values())


class Oracle(OwnerOracle):
    """원본 제어와 수치 몸체를 실행하고 일반 Finder 공급/새 Missile 생성만 경계로 둔다."""
    def __init__(self, control):
        """현재 PC 정밀 범위·확장 서버SID 풀·SEH와 읽기 전용 PE를 준비한다."""
        super().__init__('originals')
        self.control = control
        self.mu.mem_map(0, 0x1000)
        self.mu.mem_map(POOL+0x2000, 0x130000)  # 기존128슬롯 뒤에 실제 서버번호 입력만 확장한다.
        self.allowed = []
        self.entries = set(ENTRIES)
        facts = {int(row['entry'], 16): row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'), delimiter='\t')}
        # 전체 정밀 표가 소유하지 않은 CRT 분기는 기존 전용 정밀 범위를 사용한다.
        for row in csv.DictReader((ROOT/'extracted/gunarcher-helper/originals/functions.tsv').open(encoding='utf-8'), delimiter='\t'):
            facts.setdefault(int(row['entry'], 16), row)
        # 마지막RET 명령의 모든 바이트를 포함한 실제 Ghidra 범위만 허용한다.
        for entry in ENTRIES:
            for segment in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in segment.split('-'))
                self.allowed.append((low, high+1))
        self.boundaries = Counter()
        self.returns = 0

    def read(self, address, width=4):
        """에뮬레이터의 비정렬 값을 little endian으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address, width), 'little')

    def put(self, address, value, width=4):
        """명시 입력이나 외부 경계의 출력만 해당 폭으로 쓴다."""
        self.mu.mem_write(address, int(value).to_bytes(width, 'little'))

    def ret(self, value=0, purge=0):
        """선언한 외부 경계만 원본 호출 규약으로 반환한다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_EIP, self.read(esp))
        self.mu.reg_write(UC_X86_REG_ESP, esp+4+purge)

    def on_instruction(self, mu, address, size, data):
        """Finder 순서와 실제 생성자 인자만 관측하며 점/난수/필터/거리는 실행한다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        this = mu.reg_read(UC_X86_REG_ECX)
        if address in (0x4b16d0, 0x4b1810, 0x4e4391, 0x481a50):
            self.boundaries[f'{address:08x}'] += 1
            if address == 0x4b16d0:
                area = [self.read(esp+4+i*4) for i in range(4)]
                self.events.append('B:'+','.join(map(str, area))+':'+str(self.read(this+56))+':'+str(self.read(this+60)))
                self.index = 0
                self.finder = this
                self.put(this+52, self.nodes[0][0] if self.nodes else 0)
                self.ret(purge=16)
            elif address == 0x4b1810:
                self.events.append('N')
                self.index += 1
                if self.index == 1:
                    mutation = self.case['mutation']
                    if mutation == 1:self.put(OBJECT+24, 15001)
                    if mutation == 2:self.put(OBJECT+48, bits(37.25))
                    if mutation == 3:self.put(OBJECT+8, 91)
                    if mutation == 4:
                        self.put(self.slot(90)+10, 132, 1)
                        self.put(self.slot(90)+14, bits(81.25))
                        self.put(self.slot(90)+18, bits(82.5))
                self.put(this+52, self.nodes[self.index][0] if self.index < len(self.nodes) else 0)
                self.ret()
            elif address == 0x4e4391:
                assert self.read(esp+4) == 128
                self.ret(ALLOC)
            else:
                self.events.append('J:'+','.join(str(self.read(esp+4+i*4)) for i in range(12)))
                self.ret(ALLOC, 48)
            return
        super().on_instruction(mu, address, size, data)

    def run_body(self, case):
        """현재 입력의 좌표/전체 필드/관찰 순서/최종 공유 난수·ABI를 보존한다."""
        self.case = case
        self.events = []
        self.nodes = [] if case['nodes'] == '-' else [[int(value) for value in item.split(',')] for item in case['nodes'].split(';')]
        mu = self.mu
        mu.mem_write(0, bytes(4))
        mu.mem_write(OUTPUT, struct.pack('<ff', 123.25, 124.5))
        fields = bytearray((i*13+7)&255 for i in range(128))
        struct.pack_into('<II', fields, 4, 81, 90)
        struct.pack_into('<I', fields, 24, case['target'])
        struct.pack_into('<I', fields, 48, case['speed'])
        struct.pack_into('<II', fields, 56, case['ox'], case['oy'])
        mu.mem_write(OBJECT, bytes(fields))
        mu.mem_write(TYPES, bytes(188*500))
        self.put(TYPES+130*500+0x44, case['range'])
        self.put(0x532710, case['seed'])
        self.put(0x5e4794, 0)
        self.put(0x541348, 188)  # 실제 타입 개수 검사의 현행188 입력이다.
        self.put(0x545370, bits(-10000))
        self.put(0x545374, bits(-10000))  # 프로그램 전역 BoardCoord 생성자가 설치하는 실제 invalid 입력이다.
        # 현재 표시/변경 후 부모와 각 후보의 TYPE를 독립 설치한다.
        for sid, typeid in ((90, 130), (91, 130)):
            mu.mem_write(self.slot(sid), bytes(50))
            self.put(self.slot(sid)+6, 81, 2)
            self.put(self.slot(sid)+10, typeid, 1)
            self.put(self.slot(sid)+14, case['x'])
            self.put(self.slot(sid)+18, case['y'])
        for index, (sid, f1, f2, extra, x, y, state) in enumerate(self.nodes):
            typeid = 70+index
            mu.mem_write(self.slot(sid), bytes(50))
            self.put(self.slot(sid)+10, typeid, 1)
            self.put(self.slot(sid)+11, state, 1)
            self.put(self.slot(sid)+40, extra, 1)
            self.put(self.slot(sid)+14, x)
            self.put(self.slot(sid)+18, y)
            self.put(TYPES+typeid*500+0xe8, f1)
            self.put(TYPES+typeid*500+0xec, f2)
        self.write_ranges = [(0, 4), (OBJECT, OBJECT+128), (OUTPUT, OUTPUT+8), (0x532710, 0x532714), (self.slot(90), self.slot(90)+50)]
        saved = {register: 0x12340000+index for index, register in enumerate((UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP))}
        # 비휘발 레지스터와 x87를 호출마다 독립 초기화하고 반환 뒤 대조한다.
        for register, value in saved.items():mu.reg_write(register, value)
        mu.reg_write(UC_X86_REG_EAX, 0)
        mu.reg_write(UC_X86_REG_EDX, 0)
        mu.reg_write(UC_X86_REG_ECX, OBJECT)
        mu.reg_write(UC_X86_REG_ESP, STACK)
        mu.reg_write(UC_X86_REG_EFLAGS, 2)
        mu.reg_write(UC_X86_REG_FPCW, self.control)
        mu.reg_write(UC_X86_REG_FPSW, 0)
        mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        args = [OUTPUT] if case['kind'] == 0 else []
        mu.mem_write(STACK, struct.pack('<'+'I'*(len(args)+2), STOP, *args, 0xc0dec0de))
        mu.emu_start(0x482080 if case['kind'] == 0 else 0x482190, STOP, count=100000)
        assert mu.reg_read(UC_X86_REG_EIP) == STOP and mu.reg_read(UC_X86_REG_ESP) == STACK+4+len(args)*4
        assert all(mu.reg_read(register) == value for register, value in saved.items())
        assert mu.reg_read(UC_X86_REG_FPCW) == self.control and mu.reg_read(UC_X86_REG_FPSW)&0x3800 == 0 and mu.reg_read(UC_X86_REG_FPTAG) == 0xffff
        assert self.read(0) == 0 and self.read(STACK+4+len(args)*4) == 0xc0dec0de
        if case['kind'] == 0:assert mu.reg_read(UC_X86_REG_EAX) == OUTPUT
        self.returns += 1
        return [';'.join(self.events), bytes(mu.mem_read(OUTPUT, 8)).hex(), bytes(mu.mem_read(OBJECT, 128)).hex(), str(self.read(0x532710))]


def main():
    """두 정밀도의 실제 관찰만 새 파일로 보존하거나 저장된 해시를 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'))
        for path, expected in report['sha256'].items():assert digest(ROOT/path) == expected, path
        assert sum(bool(line) and not line.startswith('#') for line in FIXTURE.read_text('utf-8').splitlines()) == report['uniqueInputs']
        print('아이스 반사 원문 SHA 확인', report['uniqueInputs'])
        return
    selected = inputs()
    if args.smoke:selected = [selected[0], next(row for row in selected if row['kind'] == 1 and row['seed'] == 20 and row['nodes'] != '-')]
    oracles = [Oracle(0x27f), Oracle(0x37f)]
    rows = []
    differences = []
    # 같은 입력을 두 정밀도에서 실행하며 기준 출력은53비트 원문 반환이다.
    for index, case in enumerate(selected):
        try:output = [oracle.run_body(case) for oracle in oracles]
        except Exception as error:raise RuntimeError(f'반사 입력{index} {case}: {error}') from error
        if output[0] != output[1]:differences.append(dict(index=index, secondary=output[1]))
        rows.append('\t'.join([str(case[key]) for key in KEYS]+output[0]))
    if args.smoke:
        print('반사 smoke', len(rows), rows)
        return
    with FIXTURE.open('x', encoding='utf-8', newline='\n') as stream:stream.write('# 보호10.78 반사점/선택. 일반Finder 후보 공급과 할당 성공/새Missile ctor는 명시 외부 경계다.\n'+'\n'.join(rows)+'\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'extracted/gunarcher-helper/originals/functions.tsv', 'tools/decomp_ice_ricochet.py', 'tools/decomp_owner_oracle.py', FIXTURE.relative_to(ROOT).as_posix())
    report = dict(host=socket.gethostname(), lastFullDecompilation='HJOW-Athlon / 2026-10-23', uniqueInputs=len(rows), normalReturns=len(rows)*2, precisionDifferences=differences, actualEntries=[f'{entry:08x}' for entry in ENTRIES], boundaries=dict(oracles[0].boundaries), nativeCalls=dict(oracles[0].native_calls), limits='원본 반사점/난수/선택/HP/거리/점Finder ctor를 실행한다. 일반Finder 후보 공급과 비NULL 할당/후속Missile ctor는 경계다. 새 탄의 수명/실제 세션 연결/완주는 별도다. 변위0의 미초기화 지역점은 Point 함수의 명시 출력 보존만 검사한다.', sha256={path:digest(ROOT/path) for path in paths})
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print('반사 고유입력', len(rows), '반환', len(rows)*2, '정밀도 차이', len(differences))


if __name__ == '__main__':main()
