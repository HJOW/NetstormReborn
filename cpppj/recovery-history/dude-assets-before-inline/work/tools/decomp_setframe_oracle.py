#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Squid 프레임 지정(004acee0 ↔ CD 004acbc0)을 세 실제 PE의 제한 x86으로 대조한다.

실제 명령으로 실행하는 것: 프레임 지정 몸체, 현재 프레임의 해시 단계 계산(004ace40 ↔ CD 004acb00)과 프레임 추가 헤더 조회(00419850),
가상 표 +0x4c의 넘김 함수(현재 위치에 flags로 다시 Pop, 0041c0d0 ↔ CD 00401c60), +0x8c의 넘김 함수(+0x88로 점프, 004ad670 ↔ CD 004ae3e0).
명시적으로 대체하는 것(호출 사실과 인자만 기록): 가상 표시 갱신(+0x88·+0x8c), 가상 Unpop(+0x48), 가상 Pop(+0x90).
원본 게임/OS/업데이터는 실행하지 않는다.

python -X utf8 tools/decomp_setframe_oracle.py            # 기대값·근거 기록 생성
python -X utf8 tools/decomp_setframe_oracle.py --verify   # 저장된 기록·입력 파일의 SHA/행 수 확인
"""
import argparse
import collections
import csv
import importlib.metadata
import itertools
import json
import struct
from pathlib import Path

from decomp_bridgeevent_oracle import (ROOT, SPECS, CONTROLS, BRIDGE, BRIDGE_TYPE, VTABLE, TYPES, CODES, STOP, float_bits, digest)
from decomp_neighbor_oracle import NeighborOracle
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 합성 SHP 자료의 에뮬레이터 주소(프레임 코드 영역의 빈 뒷부분)와 대체 가상 함수 진입점.
SHP = CODES + 0x30000
UNPOP, UPDATE88, UPDATE8C = STOP + 0x130, STOP + 0x140, STOP + 0x150
# 판본별 진입점·넘김 함수·필드 위치(해시 단계 바이트, 프레임 수 두 배 플래그는 패치판 00419850만 본다).
EXTRA = {
    'originals': dict(entries=dict(SetFrame=0x4acee0, Thunk8c=0x4ad670), repop=0x41c0d0, level=0x21),
    'originalCD': dict(entries=dict(SetFrame=0x4acbc0, Thunk8c=0x4ae3e0), repop=0x401c60, level=0x1f),
}
EXTRA['original1037'] = EXTRA['originalCD']
# 프레임 여덟 개의 추가 헤더 +0·+4(칸 단위 크기). 큰 쪽이 2 이하면 1단계, 4 이하면 2단계, 넘으면 3단계다.
SIZES = ((1.0, 1.0), (2.0, 2.0), (2.5, 1.0), (3.0, 4.0), (4.0, 4.5), (0.5, 9.0), (2.0, 1.5), (4.0, 4.0))
# 타입 종류: (flags1, flags2, 프레임 수 필드). priest 타입은 flags1의 0x40000 때문에 패치판에서 프레임 수가 두 배로 취급된다.
KINDS = ((0x00000802, 0x00000004, 8), (0x28000803, 0x00000002, 8), (0x28000003, 0x01000000, 8), (0x00069012, 0x00210000, 4))
# 객체 위치(소수 좌표). 다시 Pop할 때 그대로 넘어가는지 본다.
POSITION = (20.75, 21.9)
# 새 결과 파일은 이전 기록/fixture와 분리한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/setframe-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-setframe-evidence.json'


class FrameOracle(NeighborOracle):
    """프레임 지정 몸체와 단계 계산만 실제로 실행하고 가상 표시 갱신·Unpop·Pop은 호출 기록으로 대체한다."""
    def __init__(self, edition):
        """새 내보내기의 몸체를 허용하고 합성 가상 표에 대체/넘김 함수 주소를 쓴다."""
        super().__init__(edition)
        extra = EXTRA[edition]
        self.spec = dict(self.spec, entries=dict(self.spec['entries'], **extra['entries']))
        self.level_offset = extra['level']
        self.stubs.update({UNPOP: 'unpop', UPDATE88: 'update88', UPDATE8C: 'update8c'})
        # +0x48 Unpop, +0x88·+0x8c 표시 갱신은 대체, +0x4c는 실제 넘김 함수(안에서 +0x90 Pop 대체를 부른다).
        for offset, target in ((0x48, UNPOP), (0x4c, extra['repop']), (0x88, UPDATE88), (0x8c, UPDATE8C)):
            self.mu.mem_write(VTABLE + offset, struct.pack('<I', target))
        paths = [ROOT / f'extracted/setframe/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.exports.extend(paths)
        with paths[1].open(encoding='utf-8') as fp:
            # 내보낸 불연속 몸체만 허용한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))

    def on_instruction(self, mu, address, size, data):
        """세 대체 가상 함수는 사건을 기록하고 반환한다. 그 밖에는 부모의 허용 범위 검사를 따른다."""
        name = self.stubs.get(address)
        if name not in ('unpop', 'update88', 'update8c', 'pop'):
            super().on_instruction(mu, address, size, data)
            return
        self.instructions += 1
        self.stub_calls[name] += 1
        esp = mu.reg_read(UC_X86_REG_ESP)
        ret, a0, a1, a2 = struct.unpack('<IIII', mu.mem_read(esp, 16))
        sid = self.sid_of(mu.reg_read(UC_X86_REG_ECX))
        # 호출 시점의 프레임 필드를 사건에 함께 적는다. 프레임을 쓰는 시점과 가상 호출의 순서가 드러난다.
        frame = int.from_bytes(mu.mem_read(self.slot(sid) + self.o['frame'], self.o['frame_size']), 'little')
        purge = 0
        if name == 'unpop':
            self.events.append(f'U:{sid}:{a0}:{frame}')
            purge = 4
        elif name == 'pop':
            self.events.append(f'P:{sid}:{a0}:{a1}:{a2}:{frame}')
            purge = 12
        else:
            self.events.append(f'{"D" if name == "update88" else "E"}:{sid}:{frame}')
        mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge)
        mu.reg_write(UC_X86_REG_EIP, ret)

    def prepare_object(self, kind, level, frame):
        """타입 구조체·합성 SHP·객체 슬롯을 입력한다. 단계 판단은 계산하지 않는다."""
        flags1, flags2, count = kind
        base = TYPES + BRIDGE_TYPE * self.type_stride
        self.mu.mem_write(base, bytes(self.type_stride))
        self.mu.mem_write(base + 0xe8, struct.pack('<II', flags1, flags2))
        self.mu.mem_write(base + 0x114, struct.pack('<I', count))
        self.mu.mem_write(base + 0xdc, struct.pack('<I', SHP))
        blob = bytearray(0x1000)
        # SHP: +8부터 8바이트 간격으로 프레임 자료의 오프셋이 있고, 그 36바이트 앞이 추가 헤더(+0·+4가 칸 단위 크기)다.
        for index, (width, height) in enumerate(SIZES):
            offset = 0x200 + index * 0x40
            struct.pack_into('<I', blob, 8 + index * 8, offset)
            struct.pack_into('<ff', blob, offset - 0x24, width, height)
        self.mu.mem_write(SHP, bytes(blob))
        slot = self.slot(BRIDGE)
        self.mu.mem_write(slot, bytes(self.stride))
        self.mu.mem_write(slot, struct.pack('<I', VTABLE))
        self.mu.mem_write(slot + 10, bytes([BRIDGE_TYPE, 0]))
        self.mu.mem_write(slot + 14, struct.pack('<ff', *POSITION))
        self.mu.mem_write(slot + self.o['frame'], frame.to_bytes(self.o['frame_size'], 'little'))
        self.mu.mem_write(slot + self.level_offset, bytes([level]))
        # 원본이 쓸 수 있는 것은 프레임 필드와 단계 바이트뿐이다.
        self.write_ranges = [(slot + self.o['frame'], slot + self.o['frame'] + self.o['frame_size']),
                             (slot + self.level_offset, slot + self.level_offset + 1)]
        self.pending = None

    def set_frame(self, kind, level, frame, new, flags, control):
        """프레임 지정 한 번의 사건 순서와 슬롯 전체를 돌려준다."""
        self.prepare_object(kind, level, frame)
        self.call('SetFrame', [new, flags], self.slot(BRIDGE), 8, control, returns_float=False)
        return [';'.join(self.events) or '-', bytes(self.mu.mem_read(self.slot(BRIDGE), self.stride)).hex()]

    def thunk(self, kind, control):
        """+0x8c 넘김 함수가 +0x88의 대체로 넘어가는지 본다."""
        self.prepare_object(kind, 1, 0)
        self.call('Thunk8c', [], self.slot(BRIDGE), 0, control, returns_float=False)
        return ';'.join(self.events) or '-'


def inputs():
    """타입 종류·저장된 단계·현재 프레임·새 프레임·flags의 격자. 새 프레임에는 현재와 같은 값·범위 밖·바이트를 넘는 값을 넣는다."""
    result = []
    for kind, level, frame, choice, flags in itertools.product(range(len(KINDS)), (0, 1, 2, 3, 0xff), range(8),
                                                               (0, 3, 4, 7, 'same', 0x101), (0, 0x2000)):
        result.append((kind, level, frame, frame if choice == 'same' else choice, flags))
    return result


def both(run):
    """두 x87 정밀도에서 실행해 관찰이 같을 때만 돌려준다."""
    first, second = (run(control) for control in CONTROLS)
    if first != second: raise RuntimeError(f'x87 정밀도에 따라 결과가 다름: {first} / {second}')
    return first


def generate(smoke=False):
    """세 실제 PE와 두 x87 정밀도에서 새 fixture·SHA 근거를 만든다."""
    cases = inputs()
    if smoke: cases = cases[::97]
    rows = []
    reports = {}
    paths = {Path(__file__), ROOT / 'tools/decomp_bridgeevent_oracle.py', ROOT / 'tools/decomp_neighbor_oracle.py', FIXTURE,
             ROOT / 'tools/ghidra/setframe-functions.json'}
    for edition in SPECS:
        oracle = FrameOracle(edition)
        for kind, level, frame, new, flags in cases:
            rows.append(['Set', edition, kind, level, frame, new, flags,
                         *both(lambda c: oracle.set_frame(KINDS[kind], level, frame, new, flags, c))])
        for kind in range(len(KINDS)):
            rows.append(['Thunk', edition, kind, both(lambda c: oracle.thunk(KINDS[kind], c))])
        print(f'{edition}: {dict(collections.Counter(row[0] for row in rows if row[1] == edition))}', flush=True)
        reports[edition] = dict(binary=oracle.spec['binary'], sha256=oracle.sha256,
            entries={k: f'{v:08x}' for k, v in oracle.spec['entries'].items()},
            replaced_entry_points={f'{k:08x}': v for k, v in oracle.stubs.items()},
            native_calls=dict(oracle.native_calls), stub_calls=dict(oracle.stub_calls),
            assertions=oracle.assertions, instructions=oracle.instructions)
        paths.add(ROOT / oracle.spec['binary'])
        paths.update(oracle.exports)
    if smoke:
        for row in rows[:40]: print('\t'.join(map(str, row)))
        return
    header = ['# 실제 PE 제한 x86 Squid 프레임 지정 기대값; 가상 표시 갱신(D=+0x88, E=+0x8c)·Unpop(U)·Pop(P)은 호출 기록 대체. 사건의 마지막 칸은 호출 시점의 프레임 필드다',
              '# 두 x87 정밀도의 관찰이 같은 입력만 저장한다. 객체 위치는 (20.75, 21.9), 타입 종류와 프레임 크기 표는 도구의 KINDS·SIZES다.',
              '# Sizes 프레임별 폭,높이 | Kind 번호 flags1 flags2 프레임수 | Set 판본 종류 저장단계 현재프레임 새프레임 flags 사건 슬롯 | Thunk 판본 종류 사건']
    tables = [['Sizes', ';'.join(f'{float_bits(w)},{float_bits(h)}' for w, h in SIZES)]]
    tables += [['Kind', index, *kind] for index, kind in enumerate(KINDS)]
    FIXTURE.write_text('\n'.join(header) + '\n' + '\n'.join('\t'.join(map(str, row)) for row in tables + rows) + '\n',
                       encoding='utf-8', newline='\n')
    counts = collections.Counter(row[0] for row in rows)
    report = dict(schema=1, primary_target='10.78', method='Unicorn x86 32-bit; OS/API/file/window execution forbidden',
        unicorn_version=importlib.metadata.version('unicorn'), x87_control_words=[hex(v) for v in CONTROLS],
        cases=dict(counts), total=sum(counts.values()), editions=reports, os_calls=0,
        files={p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(paths)},
        limitations=['virtual display update (+0x88/+0x8c), Unpop (+0x48) and Pop (+0x90) are recorded substitutes',
                     'synthetic type record and SHP frame headers; the current frame is always inside the frame table',
                     'flags other than 0 and 0x2000 reach the original assert and are not part of the inputs',
                     'no GameWorld, real display update, real Unpop/Pop'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(dict(cases=dict(counts), total=sum(counts.values())), ensure_ascii=False))


def verify():
    """저장된 입력/도구/fixture/몸체/PE의 SHA, 행 수, assert 0, 실제 몸체 실행을 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    for name, expected in report['files'].items():
        if digest(ROOT / name) != expected: raise RuntimeError(f'SHA 불일치: {name}')
    rows = [line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    counts = collections.Counter(line.split('\t', 1)[0] for line in rows)
    counts.pop('Sizes', None)
    counts.pop('Kind', None)
    if dict(counts) != report['cases'] or sum(counts.values()) != report['total']: raise RuntimeError('fixture 행 수 불일치')
    for edition, item in report['editions'].items():
        if item['assertions'] or not item['native_calls'].get('SetFrame') or not item['native_calls'].get('Thunk8c'):
            raise RuntimeError(f'원본 실행 증거 오류: {edition}')
    print(f'setframe 검증 통과: {sum(counts.values())}개')


def main():
    """새 기대값 생성, 저장 결과 감사, 파일을 쓰지 않는 도구 점검 가운데 하나를 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--smoke', action='store_true', help='파일을 쓰지 않고 일부 입력만 실행해 도구를 점검한다')
    args = parser.parse_args()
    if args.verify: verify()
    else: generate(args.smoke)


if __name__ == '__main__':
    main()
