#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 RenderProcess의 실제 Form 삭제 통지 영역을 격리 x86에서 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from decomp_fierytail_frame import FieryOracle, ROOT, OBJECT
from decomp_particle_gradient import digest

FIXTURE = ROOT / 'cpppj/tests/fixtures/render-removal-x86.tsv'  # 초기 정수화/카메라/중간 float 저장의 실제 기대값이다.
REPORT = ROOT / 'cpppj/render-removal.json'  # 실제 호출·반환·근거 SHA를 보존한다.
ENTRY = 0x487960  # TYPE11을 포함하는 원본 RenderProcess Form 삭제 가상 함수다.


def inputs():
    """음수/소수/큰 float 좌표와 현재 카메라·삭제 flags의 명시 입력만 만든다."""
    rectangles = ((12.25, 13.75, 30.5, 35.5), (-12.9, -13.1, 30.9, 35.1), (-.99, -.01, .01, .99),
                  (0, 0, 0, 0), (16777215, -16777215, 16777218, 16777220), (-16777218, -16777220, -16777215, 16777215))
    cameras = ((0, 0), (.25, -.5), (100.75, -128.25), (16777216, 16777216))
    return [struct.pack('<6fI', *rectangle, *camera, flags) for rectangle, camera, flags in itertools.product(rectangles, cameras, (0, 0xffffffff))]


class RemovalOracle(FieryOracle):
    """정수 변환까지 실제 몸체를 실행하고 dirty 큐 소비만 기존 관찰 경계를 사용한다."""
    def __init__(self):
        """현재 PC의 정밀 함수 범위만 더하며 불필요한 주변 코드는 허용하지 않는다."""
        super().__init__()
        self.capacity = 0  # 기존 쓰기 guard의 입자 허용 범위는 비워 둔다.
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            row = next(row for row in csv.DictReader(stream, delimiter='\t') if int(row['entry'], 16) == ENTRY)
        # 함수의 실제 불연속 범위만 실행 가능하다.
        for part in row['ranges'].split(';'):
            low, high = (int(value, 16) for value in part.split('-'))
            self.allowed.append((low, high + 1))

    def sample(self, payload, precision):
        """실제 삭제 통지의 flags 인자와 등록 영역을 넣고 단일 dirty 요청을 읽는다."""
        raw = b'\xa5' * 12 + payload[:16] + b'\xa5' * (0x68 - 28)
        self.mu.mem_write(OBJECT, raw)
        self.mu.mem_write(0x59a938, payload[16:24])
        self.dirty, self.lines = [], []
        self.call(ENTRY, OBJECT, payload[24:], 4, precision)
        assert bytes(self.mu.mem_read(OBJECT, 0x68)) == raw
        assert len(self.dirty) == 1 and not self.lines
        return self.dirty[0].hex()


def main():
    """48입력·두 정밀도의 실제 결과를 보존하거나 SHA/고유 입력을 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        facts = json.loads(REPORT.read_text(encoding='utf-8'))
        # 출력 재계산 없이 현재 근거의 전체 바이트를 확인한다.
        for name, expected in facts['sha256'].items():
            assert digest(ROOT / name) == expected, name
        rows = [row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
        assert len(rows) == facts['uniqueInputs'] * 2 and all(len(row) == 4 for row in rows)
        assert len({(row[1], row[2]) for row in rows}) == len(rows)
        print('RenderProcess 삭제 영역 SHA/고유 입력 감사:', facts['uniqueInputs'])
        return
    oracle, rows, differences = RemovalOracle(), [], []
    cases = inputs()
    assert len(set(cases)) == len(cases)
    # 기대 영역을 Python으로 계산하지 않고 원본 dirty 인자로 보존한다.
    for index, payload in enumerate(cases):
        outputs = []
        # 같은 입력의 다른 정밀도를 새 고유 입력으로 세지 않는다.
        for precision in (0x027f, 0x037f):
            output = oracle.sample(payload, precision)
            rows.append((f'case{index}', precision, payload.hex(), output))
            outputs.append(output)
        if outputs[0] != outputs[1]:
            differences.append(index)
    FIXTURE.write_text('# name cw rectCameraFlags dirtyRequest\n' + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_particle_gradient.py', 'tools/decomp_fierytail_frame.py', 'tools/decomp_render_removal.py', FIXTURE.relative_to(ROOT).as_posix()]
    facts = dict(host=socket.gethostname(), lastDecompileHost='HJOW-Athlon', entry='00487960', uniqueInputs=len(cases), normalReturns=len(rows), precisionDifferentInputs=differences, helperCalls=dict(oracle.calls), sha256={name: digest(ROOT / name) for name in paths}, limitations=['실제00487960과__ftol을 실행하고499310 dirty 큐 소비만 요청 경계로 관찰했다.', 'Host/Kernel·실제 게임·장치/픽셀 실행 관찰은 아니다. 두 정밀도의 동일 입력을 고유 입력으로 중복 가산하지 않는다.'])
    REPORT.write_text(json.dumps(facts, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('RenderProcess 삭제 영역:', len(cases), '고유 입력,', len(rows), '정상 반환')


if __name__ == '__main__':
    main()
