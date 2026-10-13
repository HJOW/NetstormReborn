#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다리 추첨·CanonDecoder·열린 끝·수명 갱신 접두 구간을 두 원본 x86에서 검증한다.

원본 게임·OS API를 실행하지 않는다. 코드와 메모리 쓰기를 모두 허용 목록으로 제한한다.
수명 함수는 첫 외부 효과 호출 직전에 의도적으로 멈춘다. 전체 붕괴 검증이 아니다.
"""
import collections
import hashlib
import json
import random
import struct
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH, STACK, STOP, MAX_INSTRUCTIONS
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)
from cpp_assets_smoke import read_data, expected_code
from taff import TaffArchive
from typefile import parse_type

# 같은 의미인 검증 대상. Life는 접두 구간만, 나머지는 정상 return/ret N을 검사한다.
PAIRS = [
    {'name': 'Draw', 'originals': '004257c0', 'originalCD': '0041fc70', 'purge': 0},
    {'name': 'Random', 'originals': '004558c0', 'originalCD': '0048cbd0', 'purge': 0},
    {'name': 'Ctor', 'originals': '00425c20', 'originalCD': '0041fcf0', 'purge': 24},
    {'name': 'Advance', 'originals': '00425860', 'originalCD': '0041feb0', 'purge': 0},
    {'name': 'Cell', 'originals': '00425700', 'originalCD': '0041fbb0', 'purge': 20},
    {'name': 'FindFrame', 'originals': '0049a940', 'originalCD': '004442f0', 'purge': 12},
    {'name': 'Open', 'originals': '00421770', 'originalCD': '00449e30', 'purge': 8},
    {'name': 'Life', 'originals': '00421c30', 'originalCD': '0044a1c0', 'purge': 4},
]
# 인라인 여부가 달라 별도 CD 함수 쌍이 없는 패치판 열린 끝 검사.
DIRECTION = {'name': 'Direction', 'originals': '004217f0'}
# 실제 보조 함수만 실행한다. 부동소수 정수 변환도 원본 CRT 기계어를 사용한다.
HELPERS = {'originals': [0x49a840, 0x4ad680, 0x41d080, 0x40eaf0, 0x40e9d0, 0x4e49c0],
           'originalCD': [0x4ae3f0, 0x43f8a0, 0x4f161c]}
# 타입 표·타입 번호·표면 지도·추첨 캐시·난수·전투 상태의 판본별 주소.
GLOBALS = {'originals': (0x59ab20, 0x5411a0, 0x541204, 0x542514, 0x5453d4, 0x532710, 0x540bc0),
           'originalCD': (0x51c960, 0x51ca8c, 0x51caf0, 0x5670cc, 0x516618, 0x5308ec, 0x540a28)}
# 호출할 외부 효과 바로 앞: 삭제 가상 호출, 수명 비트 저장 후 유효성 검사 호출.
LIFE_STOPS = {'originals': {0x421c7b: 'remove', 0x421cb6: 'update'},
              'originalCD': {0x44a210: 'remove', 0x44a241: 'update'}}
# 가상 객체·이웃 목록·타입 구조체·프레임 배열·표면 지도의 서로 겹치지 않는 주소.
OBJECT, LIST, MEMBERS, TYPE, FRAMES, MAP = (SCRATCH, SCRATCH+0x1000, SCRATCH+0x1200,
    SCRATCH+0x4000, SCRATCH+0x8000, SCRATCH+0x20000)
# 실제 타입 ID의 최소 범위와 영역 시험용 ID. 등록 전역만 에뮬레이터 안에서 설정한다.
BRIDGE_ID, TERRITORY_ID = 70, 71
# 재현 가능한 입력 생성 시드.
SEED = 0x421770
# 모양 표의 두 판본 주소와 크기.
TABLES = {'originals': 0x52f998, 'originalCD': 0x514a80}
RECORD_SIZE, PATTERN_COUNT = 72, 26


def float_bits(value):
    """단정도 좌표의 실제 비트를 TSV에 보존한다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def codes(edition, name):
    """C++와 독립인 기존 파서로 실제 타입의 프레임 코드 표를 읽는다."""
    root = ROOT / edition
    archive = TaffArchive(str(root/'netstorm.tarc'))
    definition = parse_type(read_data(root, archive, f'd/{name}.type').decode('cp1252'))
    return b''.join(expected_code(label, flags) for label, flags, _refs in definition.clusters)


class BridgeOracle(Oracle):
    """호스트와 무관한 PE·가상 메모리에서 검토 함수와 허용 쓰기만 실행한다."""
    def __init__(self, edition):
        """전역·메모리 배치와 코드/쓰기 검사기를 준비한다."""
        pairs = PAIRS + ([DIRECTION] if edition == 'originals' else [])
        super().__init__(edition, pairs, helpers=HELPERS[edition])
        # assert 보고만 대체한다. float 변환·조회·프레임 검색은 원본 기계어다.
        self.stubs = {address: name for address, name in self.stubs.items() if name == 'assert'}
        self.mu.mem_map(MAP, 0x20000)
        self.stride = 500 if edition == 'originals' else 468
        base, bridge, territory, surface, weight, state, _battle = GLOBALS[edition]
        self.mu.mem_write(base, struct.pack('<I', TYPE - BRIDGE_ID*self.stride))
        self.mu.mem_write(bridge, struct.pack('<I', BRIDGE_ID))
        self.mu.mem_write(territory, struct.pack('<I', TERRITORY_ID))
        self.mu.mem_write(surface, struct.pack('<I', MAP))
        self.writes = set()
        self.write_ranges = [(OBJECT, OBJECT+0x100), (0x30000000, 0x30010000), (weight, weight+4), (state, state+4)]
        self.prefix_active, self.prefix_stop = False, None
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.on_write)

    def on_write(self, _mu, _access, address, size, _value, _data):
        """입력 표·다른 전역·코드·예상 밖 객체를 쓰면 즉시 중단한다."""
        if not any(low <= address and address+size <= high for low, high in self.write_ranges):
            raise RuntimeError(f'허용하지 않은 메모리 쓰기: {self.edition} {address:08x}+{size}')
        self.writes.add((address, size))

    def on_instruction(self, mu, address, size, data):
        """수명 접두 검증은 의도한 호출 앞에서만 중단한다. 나머지는 부모의 코드 제한을 따른다."""
        if self.prefix_active and address in LIFE_STOPS[self.edition]:
            self.prefix_stop = LIFE_STOPS[self.edition][address]
            mu.emu_stop()
            return
        super().on_instruction(mu, address, size, data)

    def call(self, name, args, this=0, purge=0):
        """짧은 함수는 명령 수 상한으로 제한하여 호출마다 타이머 스레드를 만드는 비용을 줄인다."""
        # 검토된 함수 범위와 명령 수 상한은 유지하고 일반 레지스터를 매 호출 초기화한다.
        for reg in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(reg, 0)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, this)
        words = [STOP] + [arg & 0xffffffff for arg in args]
        self.mu.mem_write(STACK, struct.pack('<'+'I'*len(words), *words))
        self.mu.emu_start(self.entries[name], STOP, count=MAX_INSTRUCTIONS)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError(f'{name}: 명령 수 상한 내 반환하지 않음')
        if self.mu.reg_read(UC_X86_REG_ESP) != STACK+4+purge:
            raise RuntimeError(f'{name}: ret N/스택 복구 불일치')
        return self.mu.reg_read(UC_X86_REG_EAX)

    def setup_type(self, frame_codes):
        """확인된 32비트 타입 필드 +114/+124와 두 구조체를 가상 입력으로 만든다."""
        self.mu.mem_write(TYPE, bytes(self.stride*2))
        self.mu.mem_write(FRAMES, frame_codes)
        # 다리와 영역 두 타입에 같은 검사 프레임 배열을 연결한다.
        for index in range(2):
            self.mu.mem_write(TYPE+index*self.stride+0x114, struct.pack('<I', len(frame_codes)//4))
            self.mu.mem_write(TYPE+index*self.stride+0x124, struct.pack('<I', FRAMES))

    def draw(self, value):
        """원본 가중치 합 캐시를 지워 실제 초기화 분기도 실행한다."""
        weight = GLOBALS[self.edition][4]
        self.mu.mem_write(weight, bytes(4))
        result = self.call('Draw', [value])
        return result, struct.unpack('<I', self.mu.mem_read(weight, 4))[0]

    def next_random(self, state, limit):
        """반환값뿐 아니라 다음 난수 상태도 읽는다."""
        address = GLOBALS[self.edition][5]
        self.mu.mem_write(address, struct.pack('<I', state))
        result = self.call('Random', [limit])
        return result, struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def decode(self, kind, shape, direction, frame_codes, x, y):
        """실제 생성자와 반복 함수가 내는 모든 프레임·좌표·라벨을 끝까지 읽는다."""
        self.setup_type(frame_codes)
        self.mu.mem_write(OBJECT, bytes(0x100))
        type_id = BRIDGE_ID if kind == 'B' else TERRITORY_ID
        result = self.call('Ctor', [type_id, shape, direction, float_bits(x), float_bits(y), 0], OBJECT, 24)
        assert result == OBJECT
        output = []
        # 정상 모양은 최대 15칸이며 빈 칸/누락 프레임은 원본이 건너뛴다.
        for _ in range(16):
            valid = struct.unpack('<I', self.mu.mem_read(OBJECT+0x14, 4))[0]
            if not valid:
                return output
            frame = struct.unpack('<i', self.mu.mem_read(OBJECT+0x10, 4))[0]
            label = struct.unpack('<i', self.mu.mem_read(OBJECT+0x18, 4))[0]
            bx, by = struct.unpack('<II', self.mu.mem_read(OBJECT+0x20, 8))
            output.append((frame, bx, by, label, frame_codes[frame*4]))
            self.call('Advance', [], OBJECT)
        raise RuntimeError('CanonDecoder가 끝나지 않음')

    def prepare_surface(self, side, x, y, candidate, members):
        """그 방향의 전체 표면 지도·이웃 목록·Squid의 판본별 프레임 필드를 준비한다."""
        self.setup_type(bytes([ord(side), ord('P'), 1, 0]))
        self.mu.mem_write(OBJECT, bytes(0x100))
        self.mu.mem_write(OBJECT+10, bytes([BRIDGE_ID]))
        self.mu.mem_write(OBJECT+0xe, struct.pack('<ff', x, y))
        # 65536은 실제 표면 번호가 아니라 비균일 시험 지도를 고르는 입력 표시다.
        if candidate == 65536:
            # x/y마다 다른 표면 번호를 주어 내부 셀 주소 계산도 대조한다.
            surface = [([0, 7, 8, 9][(x+3*y) % 4]) for y in range(256) for x in range(256)]
            self.mu.mem_write(MAP, struct.pack('<65536H', *surface))
        else:
            self.mu.mem_write(MAP, struct.pack('<H', candidate)*65536)
        self.mu.mem_write(LIST, struct.pack('<III', MEMBERS, len(members), len(members)))
        if members:
            self.mu.mem_write(MEMBERS, struct.pack('<'+'I'*len(members), *members))

    def open_direction(self, side, direction, x, y, candidate, members):
        """보조 함수의 반환값과 패치판 전체 열린 끝 판단을 실행한다."""
        self.prepare_surface(side, x, y, candidate, members)
        opened = self.call('Open', [LIST, direction], OBJECT, 8)
        selected = None
        if self.edition == 'originals':
            selected = self.call('Direction', [LIST], OBJECT, 4)
            if selected >= 0x80000000:
                selected -= 0x100000000
        return opened, selected

    def life_prefix(self, word, reduction, battle, side):
        """수명 비트 저장 또는 실제 삭제 호출 인자까지만 실행하고 외부 효과는 실행하지 않는다."""
        self.prepare_surface(side, 0, 0, 0, [])
        self.mu.mem_write(OBJECT+0xc, struct.pack('<H', word))
        self.mu.mem_write(GLOBALS[self.edition][6], struct.pack('<I', battle))
        # 이전 호출의 일반 레지스터 흔적을 없앤다.
        for reg in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(reg, 0)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, OBJECT)
        self.mu.mem_write(STACK, struct.pack('<II', STOP, reduction & 0xffffffff))
        self.prefix_active, self.prefix_stop = True, None
        try:
            self.mu.emu_start(self.entries['Life'], STOP, count=MAX_INSTRUCTIONS)
        finally:
            self.prefix_active = False
        if self.prefix_stop is None:
            raise RuntimeError('수명 함수의 지정 중단점을 만나지 않음')
        value = struct.unpack('<H', self.mu.mem_read(OBJECT+0xc, 2))[0]
        flags = 0
        if self.prefix_stop == 'remove':
            flags = struct.unpack('<I', self.mu.mem_read(self.mu.reg_read(UC_X86_REG_ESP), 4))[0]
        return value, int(self.prefix_stop == 'remove'), flags


def main():
    """원본 입력 기대값·판본 차이·쓰기 제한과 범위가 기록된 근거를 생성한다."""
    patch, cd = BridgeOracle('originals'), BridgeOracle('originalCD')
    rng = random.Random(SEED)
    lines = ['# 원본 게임 실행 없음. Life는 첫 외부 효과 앞에서 중단한 접두 구간이다.']
    frames = {}
    # 실제 두 타입의 프레임 코드가 두 판본에서 같은지 확인한다.
    for kind, name in [('B', 'bridge'), ('T', 'puzzlePiece')]:
        frames[kind] = codes('originals', name)
        assert frames[kind] == codes('originalCD', name)
        lines.append(f'Frames\t{kind}\t{frames[kind].hex()}')
    counts = collections.Counter()
    differences = 0
    # 게임에서 쓰는 0~9999와 signed 나머지 경계·큰 정수도 모두 실제 기계어로 읽는다.
    for value in list(range(10000)) + [-2147483648, -307, -287, -1, 2147483647]:
        a, b = patch.draw(value), cd.draw(value)
        differences += a[0] != b[0]
        lines.append(f'Draw\t{value}\t{a[0]}\t{a[1]}\t{b[0]}\t{b[1]}')
        counts['draw'] += 1
    print(f'추첨 {counts["draw"]}개 완료; 판본별 결과 차이 {differences}개', flush=True)
    # seed=0, wrap, 여러 상한과 다음 상태를 검증한다.
    for state in [0, 1, 0xffffffff, 0xbad0bad] + [rng.getrandbits(32) for _ in range(128)]:
        for limit in [1, 2, 7, 287, 306, 10000, 65536, 2147483647]:
            expected = patch.next_random(state, limit)
            assert expected == cd.next_random(state, limit)
            lines.append(f'Random\t{state}\t{limit}\t{expected[0]}\t{expected[1]}')
            counts['random'] += 1
    print(f'난수 {counts["random"]}개 완료', flush=True)
    # 빈 칸·회전·라벨·실제 프레임과, 누락 프레임을 건너뛰는 경로까지 검사한다.
    for kind, size in [('B', 26), ('T', 68)]:
        for shape in range(size):
            for direction in [0, 2, 4, 6]:
                for subset in [0, 1]:
                    original = frames[kind]
                    selected = original if not subset else b''.join(original[i:i+4] for i in range(0, len(original), 4) if original[i] not in (ord('F'), ord('L')))
                    x, y = (-2.25, 255.5) if subset else (3.5, 7.25)
                    expected = patch.decode(kind, shape, direction, selected, x, y)
                    assert expected == cd.decode(kind, shape, direction, selected, x, y)
                    output = ';'.join(','.join(map(str, entry)) for entry in expected) or '-'
                    lines.append(f'Decode\t{kind}\t{shape}\t{direction}\t{subset}\t{float_bits(x)}\t{float_bits(y)}\t{output}')
                    counts['decode'] += 1
    print(f'반복자 {counts["decode"]}개 완료', flush=True)
    # 8방향·A~P·지도 밖·소수 좌표·번호 0·이웃 목록의 포함/미포함을 조합한다.
    for side in 'ABCDEFGHIJKLMNOP':
        for direction in range(8):
            for x, y in [(10, 10), (0, 0), (255, 255), (-1.5, 255.8), (0.3, -0.2), (256, 256),
                         (0.4999999701976776, 255.49998474121094), (-0.5000000596046448, -1.4999998807907104),
                         (0.00010001658665714785, 255.00009155273438)]:
                for candidate, members in [(0, []), (7, []), (7, [7]), (7, [0, 1, 8]), (65536, [7])]:
                    a = patch.open_direction(side, direction, x, y, candidate, members)
                    b = cd.open_direction(side, direction, x, y, candidate, members)
                    assert a[0] == b[0], (side, direction, x, y, a, b)
                    listed = ','.join(map(str, members)) or '-'
                    lines.append(f'Open\t{side}\t{direction}\t{float_bits(x)}\t{float_bits(y)}\t{candidate}\t{listed}\t{a[0]}\t{a[1]}')
                    counts['open'] += 1
    print(f'열린 끝 {counts["open"]}개 완료', flush=True)
    # 상태 단어의 수명 외 비트 보존과, 0에서 전투/편집기 분기를 검증한다.
    for previous in range(8):
        for target in range(-2, 8):
            for battle in [0, 1]:
                for side in ['J', 'K', 'B']:
                    word = (rng.randrange(65536) & ~0x78) | (previous << 3)
                    reduction = previous-target
                    expected = patch.life_prefix(word, reduction, battle, side)
                    assert expected == cd.life_prefix(word, reduction, battle, side)
                    lines.append(f'Life\t{word}\t{reduction}\t{battle}\t{side}\t{expected[0]}\t{expected[1]}\t{expected[2]}')
                    counts['life_prefix'] += 1
    payload = '\n'.join(lines)+'\n'
    fixture = ROOT/'cpppj/tests/fixtures/bridge-x86.tsv'
    fixture.write_text(payload, encoding='utf-8', newline='\n')
    report = {'schema': 1, 'functions': PAIRS, 'patch_only': [DIRECTION], 'helpers': HELPERS,
        'binary_sha256': {'originals': patch.sha256, 'originalCD': cd.sha256},
        'fixture_sha256': hashlib.sha256(payload.encode('utf-8')).hexdigest(),
        'generator_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'seed': SEED, 'cases': dict(counts), 'total_cases': sum(counts.values()),
        'different_draws': differences, 'weight_totals': {'originals': 287, 'originalCD': 306},
        'rejected_pairs': [{'originals': '004217f0', 'originalCD': '00449e30',
            'reason': '전체 열린 끝 탐색(ret 4)과 한 방향 보조 검사(ret 8)의 오대응. CD판 전체 판단은 00449250에 인라인.'}],
        'reviewed_pairs': [entry for entry in PAIRS if entry['name'] != 'Life'],
        'life_stop_sites': LIFE_STOPS, 'assert_reports': {'originals': patch.assertions, 'originalCD': cd.assertions},
        'native_write_sites': {'originals': len(patch.writes), 'originalCD': len(cd.writes)},
        'limitations': ['전체 배치·표면 그래프·붕괴 스캔·세계 객체 생성/삭제 검증 아님',
            'Life는 정상 ret까지 실행하지 않으며 금/그림/낙하 효과 이전의 실제 비트 저장·삭제 인자만 검증',
            'OpenDirection 전체는 패치판 기계어만 검증; CD판 별도 함수 쌍을 만들지 않음',
            '정상 A~P·유한 float·정상 수명 입력. 누락 프레임 assert 보고만 대체']}
    (ROOT/'cpppj/recovery-bridge-evidence.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps({'cases':dict(counts), 'different_draws':differences, 'total':sum(counts.values())}))


# 스크립트로 실행했을 때만 원본 기계어 기대값을 만든다.
if __name__ == '__main__':
    main()
