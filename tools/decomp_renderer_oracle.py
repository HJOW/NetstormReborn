#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 두 판본의 Renderer 정렬·변경 표 함수만 격리 x86 실행한다. 게임·OS API는 실행하지 않는다."""
import hashlib
import json
import random
import struct
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH

# 실제 두 판본의 정렬 비교·변경 영역 추가 함수. Win32 호출이 없다.
PAIRS = [
    {'name': 'Order', 'originals': '00497900', 'originalCD': '00458240'},
    {'name': 'Dirty', 'originals': '00497580', 'originalCD': '00457320'},
]
# 판본마다 다른 화면 크기·전체 변경·병합 끄기 전역 주소.
GLOBALS = {'originals': (0x5c78f0, 0x5c78f4, 0x59a8b0, 0x59a8d0),
           'originalCD': (0x516b10, 0x516b14, 0x52039c, 0x5203bc)}
# 반환 객체와 인자의 가상 주소. 호스트 메모리·파일과 관계없다.
OBJECT = SCRATCH
ARGUMENT = SCRATCH + 0x1000
# 입력 생성의 고정 난수 시드.
SEED = 0x4994b0


class RendererOracle(Oracle):
    """두 함수와 패치판의 사각형 합집합/대입 보조 함수만 허용한다."""
    def __init__(self, edition):
        """원본 기계어를 읽기만 하여 격리 메모리에 올린다."""
        super().__init__(edition, PAIRS, helpers=[0x4834c0, 0x4970a0] if edition == 'originals' else [])
        self.stubs = {}

    def order(self, a, b):
        """원본 16바이트 draw entry의 두 키를 주고 실제 반환값을 읽는다."""
        self.mu.mem_write(ARGUMENT, struct.pack('<ffIhBB', a[0], a[1], 0, a[2], 0, 0))
        self.mu.mem_write(ARGUMENT + 16, struct.pack('<ffIhBB', b[0], b[1], 0, b[2], 0, 0))
        result = self.call('Order', [ARGUMENT, ARGUMENT + 16])
        return result if result < 0x80000000 else result - 0x100000000

    def dirty(self, width, height, operations):
        """변경 표를 초기화하고 추가 시퀀스 뒤 원본 표와 전체 변경 플래그를 읽는다."""
        self.mu.mem_write(OBJECT, bytes(0x900))
        w, h, full, merge = GLOBALS[self.edition]
        # 필요한 전역만 매 입력 초기화한다.
        for address, value in [(w, width), (h, height), (full, 0), (merge, 0)]:
            self.mu.mem_write(address, struct.pack('<I', value))
        # 각 사각형의 원본 플래그도 입력 그대로 전달한다.
        for left, top, right, bottom, flags in operations:
            self.mu.mem_write(ARGUMENT, struct.pack('<iiii', left, top, right, bottom))
            self.call('Dirty', [ARGUMENT, flags], this=OBJECT, purge=8)
        count = struct.unpack('<I', self.mu.mem_read(OBJECT + 100, 4))[0]
        entries = []
        # 원본의 무시 항목도 빼지 않고 비교한다.
        for i in range(count):
            rect = struct.unpack('<iiii', self.mu.mem_read(OBJECT + 0x68 + i * 16, 16))
            flag = struct.unpack('<I', self.mu.mem_read(OBJECT + 0x6a8 + i * 4, 4))[0]
            entries.append((*rect, flag))
        return struct.unpack('<I', self.mu.mem_read(full, 4))[0], entries


def packed(entries):
    """TSV 안의 사각형 시퀀스를 정수 다섯 개씩 구분하여 기록한다."""
    return ';'.join(','.join(str(value) for value in entry) for entry in entries) or '-'


def main():
    """두 판본의 실제 반환·출력 표가 같을 때만 C++ 고정 기대값과 근거를 저장한다."""
    patch, cd = RendererOracle('originals'), RendererOracle('originalCD')
    rng = random.Random(SEED)
    lines = ['# 두 판본 Renderer 기계어 기대값. 원본 게임 프로세스·Win32 API 실행 없음.']
    orders = [((0, 0, 0), (0, 0, 0)), ((10, 20, -32768), (10, 20, 32767)),
              ((10, 20, 32767), (10, 20, -32768)), ((1, 2, 0), (2, 2, 0)), ((1, 2, 0), (1, 3, 0))]
    # 동률·음수·깊이 경계와 float 좌표를 입력한다.
    for _ in range(256):
        a = (rng.randrange(-1024, 1024) / 4, rng.randrange(-1024, 1024) / 4, rng.choice([-3, 0, 1, 32767, -32768]))
        b = (rng.randrange(-1024, 1024) / 4, rng.randrange(-1024, 1024) / 4, rng.choice([a[2], -3, 0, 1]))
        orders.append((a, b))
    # Python은 정렬 의미를 재현하지 않고 원본 실행 결과만 기록한다.
    for a, b in orders:
        expected = patch.order(a, b)
        assert expected == cd.order(a, b), (a, b, expected)
        lines.append('Order\t' + '\t'.join(str(v) for v in (*a, *b, expected)))
    sequences = [[], [(0, 0, 5, 5, 0), (1, 1, 3, 3, 4)],
                 [(1, 1, 3, 3, 0), (0, 0, 5, 5, 4)], [(0, 0, 10, 10, 4), (1, 1, 3, 3, 0)],
                 [(i * 30, 0, i * 30 + 1, 1, 0) for i in range(105)]]
    # 화면 밖·비겹침·병합 허용치·무시·불투명 플래그의 다양한 시퀀스.
    for _ in range(128):
        entries = []
        # 최대 24개의 변경 영역을 차례로 추가한다.
        for _entry in range(rng.randrange(1, 25)):
            x, y = rng.randrange(-20, 100), rng.randrange(-20, 100)
            entries.append((x, y, x + rng.randrange(1, 40), y + rng.randrange(1, 40), rng.randrange(8)))
        sequences.append(entries)
    # 표 넘침을 만드는 입력만 넓은 화면에서 실행한다.
    for operations in sequences:
        width, height = (4096, 128) if len(operations) == 105 else (96, 96)
        expected = patch.dirty(width, height, operations)
        assert expected == cd.dirty(width, height, operations), (operations, expected)
        lines.append(f'Dirty\t{width}\t{height}\t{packed(operations)}\t{expected[0]}\t{packed(expected[1])}')
    payload = '\n'.join(lines) + '\n'
    output = ROOT / 'cpppj/tests/fixtures/renderer-x86.tsv'
    output.write_text(payload, encoding='utf-8', newline='\n')
    report = {'functions': PAIRS, 'helpers': {'originals': ['004834c0', '004970a0'], 'originalCD': []},
              'binary_sha256': {'originals': patch.sha256, 'originalCD': cd.sha256}, 'order_cases': len(orders),
              'dirty_sequences': len(sequences), 'total_cases': len(orders) + len(sequences),
              'fixture_sha256': hashlib.sha256(payload.encode('utf-8')).hexdigest(), 'stubbed': [],
              'limitations': ['Squid 목록 수집·구름·Win32 출력·게임 플레이 대조가 아니다', '유한 float와 정상 사각형 입력']}
    (ROOT / 'cpppj/recovery-renderer-evidence.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f'Renderer 두 판본 x86 일치: {len(orders)} 정렬, {len(sequences)} 변경 표 시퀀스')


if __name__ == '__main__':
    main()
