#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 세 PE의 side/direction과 PathProcess 도착 프레임 접두를 제한 x86으로 대조한다.

패치는 Repop 직후 0048bdd0에서, CD/10.37은 종료 함수 0047e220 진입에서 멈춘다.
후속 경로/프로세스 정리는 실행하지 않으며 가상 Unpop/Pop은 호출 기록 대체다. 게임/OS/창 실행 없음.
"""
import argparse
import collections
import csv
import json
import struct
from pathlib import Path

from decomp_setframe_oracle import FrameOracle, both
from decomp_bridgeevent_oracle import (ROOT, SPECS, CONTROLS, BRIDGE, BRIDGE_TYPE, CODES,
    STACK, STOP, VTABLE, float_bits, digest)
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 실제 진입점과 접두 실행을 마치는 경계다. 경계 뒤 기계어는 실행하지 않는다.
ENTRIES = {
    'originals': dict(Side=0x4ad680, Direction=0x4ad710, StopPrefix=0x48bd90),
    'originalCD': dict(Side=0x4ae3f0, Direction=0x4ae4b0, StopPrefix=0x480580),
}
ENTRIES['original1037'] = ENTRIES['originalCD']
BOUNDARIES = dict(originals=0x48bdd0, originalCD=0x47e220, original1037=0x47e220)
# 부모 SID를 담는 합성 프로세스와 출력 경로다.
PROCESS = CODES + 0x20000
FIXTURE = ROOT / 'cpppj/tests/fixtures/pathanimation-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-pathanimation-evidence.json'


def profiles():
    """물리 순서와 코드 번호가 다른 표, 비연속 표, BYTE 넘침, signed side 전체를 입력한다."""
    regular = b''.join(bytes([side, ord('P'), number, 0])
                       for side in range(ord('A'), ord('P') + 1) for number in (0, 4, 1, 7))
    shuffled = b''.join(bytes([ord(side), ord('Q') if i % 2 else ord('P'), i % 8, 0x20])
                        for i, side in enumerate('PONMLKJIHGFEDCBAABCDEFGHIJKLMNOP'))
    overflow = bytes([ord('A'), ord('P'), 0, 0]) * 255 + bytes([ord('P'), ord('P'), 9, 0])
    signed = b''.join(bytes([side, ord('P'), 1, 0]) for side in range(256))
    return (regular, shuffled, overflow, signed)


class PathOracle(FrameOracle):
    """기존 안전 실행기에서 방향 조회와 도착 프레임 접두만 추가 허용한다."""
    def __init__(self, edition):
        """읽기 전용 내보내기 범위와 부모 SID를 넣고 종료 경계 관찰을 준비한다."""
        super().__init__(edition)
        self.spec = dict(self.spec, entries=dict(self.spec['entries'], **ENTRIES[edition]))
        paths = [ROOT / f'extracted/pathanimation/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.exports.extend(paths)
        with paths[1].open(encoding='utf-8') as fp:
            # Ghidra의 불연속 몸체만 실행 허용 범위에 추가한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))
        self.mu.mem_write(PROCESS + 8, struct.pack('<I', BRIDGE))
        self.boundary_stops = 0

    def on_instruction(self, mu, address, size, data):
        """도착 프레임 접두 뒤를 실행하지 않고 검사 호출의 종료 주소로 넘긴다."""
        if address == BOUNDARIES[self.edition]:
            self.boundary_stops += 1
            # 원본 반환 규약의 증명이 아니라 접두 전용 실행기의 종료 어댑터다.
            mu.reg_write(UC_X86_REG_ESP, STACK + 4)
            mu.reg_write(UC_X86_REG_EIP, STOP)
            return
        super().on_instruction(mu, address, size, data)

    def setup_codes(self, codes, table=True):
        """실제 원본이 글자 표를 만든다. signed 조회 표는 표 생성자를 부르지 않는다."""
        if table:
            self.table(codes, CONTROLS[0])
        else:
            types = struct.unpack('<I', self.mu.mem_read(self.g['types'], 4))[0]
            record = types + BRIDGE_TYPE * self.type_stride
            self.mu.mem_write(CODES, codes)
            self.mu.mem_write(record + 0x114, struct.pack('<I', len(codes) // 4))
            self.mu.mem_write(record + 0x124, struct.pack('<I', CODES))

    def prepare_slot(self, frame, extra=0, owner=0):
        """raw 입력을 만든다. 허용 쓰기는 프레임뿐이며 extra/소유자/위치 보존을 확인한다."""
        slot = self.slot(BRIDGE)
        self.mu.mem_write(slot, bytes(self.stride))
        self.mu.mem_write(slot, struct.pack('<I', VTABLE))
        self.mu.mem_write(slot + 10, bytes([BRIDGE_TYPE, 0]))
        self.mu.mem_write(slot + 14, struct.pack('<II', float_bits(20.75), float_bits(21.9)))
        self.mu.mem_write(slot + self.o['owner'], bytes([owner]))
        self.mu.mem_write(slot + self.o['frame'], frame.to_bytes(self.o['frame_size'], 'little'))
        self.mu.mem_write(slot + self.o['flag'], bytes([extra]))
        self.write_ranges = [(slot + self.o['frame'], slot + self.o['frame'] + self.o['frame_size'])]

    def read(self, frame, control):
        """두 조회 함수의 signed EAX를 직접 관찰한다."""
        self.prepare_slot(frame)
        result = []
        # 같은 입력 슬롯에서 독립 호출 두 번을 실행한다.
        for name in ('Side', 'Direction'):
            self.call(name, [], self.slot(BRIDGE), 0, control, returns_float=False)
            result.append(struct.unpack('<i', struct.pack('<I', self.mu.reg_read(UC_X86_REG_EAX)))[0])
        return result

    def stop(self, frame, extra, owner, control):
        """접두의 가상 사건 순서/인자와 슬롯 전체를 관찰한다."""
        self.prepare_slot(frame, extra, owner)
        self.call('StopPrefix', [], PROCESS, 0, control, returns_float=False)
        return [';'.join(self.events), bytes(self.mu.mem_read(self.slot(BRIDGE), self.stride)).hex()]


def generate(smoke=False):
    """PE 명령으로 기대값을 만들고 입력/몸체/도구/fixture의 SHA를 함께 기록한다."""
    codes = profiles()
    rows, reports = [], {}
    paths = {Path(__file__), ROOT / 'tools/decomp_setframe_oracle.py', ROOT / 'tools/decomp_neighbor_oracle.py',
             ROOT / 'tools/decomp_bridgeevent_oracle.py', ROOT / 'tools/ghidra/pathanimation-functions.json', FIXTURE}
    # 판본마다 자신의 PE에서 기대값을 얻는다.
    for edition in SPECS:
        oracle = PathOracle(edition)
        # 네 합성 표를 각각 준비한다.
        for profile, data in enumerate(codes):
            oracle.setup_codes(data, profile != 3)
            frames = list(range(len(data) // 4))
            if smoke: frames = sorted(set((0, min(1, len(frames)-1), len(frames)-1)))
            # 모든 물리 프레임의 getter와 접두를 두 x87 정밀도에서 대조한다.
            for frame in frames:
                rows.append(['Read', edition, profile, frame, *both(lambda c: oracle.read(frame, c))])
                if profile == 3: continue
                # BYTE 프레임 옆 extra를 바꿔도 쓰기는 그 바이트를 보존해야 한다.
                for extra in (0, 1, 16, 255):
                    owner = (frame + extra) % 9
                    rows.append(['Stop', edition, profile, frame, extra, owner,
                                 *both(lambda c: oracle.stop(frame, extra, owner, c))])
        reports[edition] = dict(binary=oracle.spec['binary'], sha256=oracle.sha256,
            entries={name: f'{value:08x}' for name, value in ENTRIES[edition].items()},
            prefix_boundary=f'{BOUNDARIES[edition]:08x}', boundary_stops=oracle.boundary_stops,
            native_calls=dict(oracle.native_calls), stub_calls=dict(oracle.stub_calls), assertions=oracle.assertions)
        paths.update(oracle.exports)
        paths.add(ROOT / oracle.spec['binary'])
        print(f'{edition}: 방향 조회/도착 접두 통과', flush=True)
    if smoke: return
    tables = [['Codes', profile, data.hex()] for profile, data in enumerate(codes)]
    FIXTURE.write_text('# 원본 x86 side/direction 및 도착 프레임 접두. Unpop/Pop 기록 대체, 경로 종료 실행 안 함.\n'
        + '\n'.join('\t'.join(map(str, row)) for row in tables + rows) + '\n', encoding='utf-8', newline='\n')
    counts = collections.Counter(row[0] for row in rows)
    report = dict(schema=1, primary_target='10.78', total=len(rows), cases=dict(counts), os_calls=0,
        x87_control_words=list(CONTROLS), editions=reports,
        files={p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(paths)},
        limitations=['Unpop and Pop are recorded substitutes; actual +0x4c Repop thunk is executed',
            'only the arrival animation prefix; patch inline path cleanup and CD Finish are excluded',
            'synthetic codes; overflow target is observed with Pop substituted, not validated as a real SHP frame',
            'no PathProcess frame stepping, path search, unit lifecycle or game loop'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(dict(cases=dict(counts), total=len(rows))))


def verify():
    """SHA/행 수, 실제 진입/접두 경계 도달 및 예상 밖 assert/OS 호출 0을 확인한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    # 저장한 입력을 한 파일씩 감사한다.
    for name, expected in report['files'].items():
        if digest(ROOT / name) != expected: raise RuntimeError(f'SHA 불일치: {name}')
    lines = FIXTURE.read_text(encoding='utf-8').splitlines()
    counts = collections.Counter(line.split('\t')[0] for line in lines if line and not line.startswith('#'))
    counts.pop('Codes')
    if dict(counts) != report['cases'] or sum(counts.values()) != report['total'] or report['os_calls']:
        raise RuntimeError('fixture/OS 호출 집계 오류')
    # 두 정밀도마다 접두 경계에 도달했는지 판본별로 확인한다.
    for edition, item in report['editions'].items():
        stops = sum(line.startswith(f'Stop\t{edition}\t') for line in lines)
        reads = sum(line.startswith(f'Read\t{edition}\t') for line in lines)
        if item['assertions'] or item['boundary_stops'] != stops * 2 or item['native_calls']['StopPrefix'] != stops * 2:
            raise RuntimeError(f'접두 실행 증거 오류: {edition}')
        if item['native_calls']['Side'] != reads * 2 or item['native_calls']['Direction'] != (reads + stops) * 2:
            raise RuntimeError(f'방향 조회 실행 증거 오류: {edition}')
    print(f'pathanimation 검증 통과: {report["total"]}개')


def main():
    """전체 생성/저장 기록 감사/작은 무저장 실행 중 하나를 선택한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args()
    if args.verify: verify()
    else: generate(args.smoke)


if __name__ == '__main__':
    main()
