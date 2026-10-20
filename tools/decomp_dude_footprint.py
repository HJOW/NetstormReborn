#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""발자국0의 실제 일반 탐색과 Dude24 경로 실패/겹침/재시도를 독립 관측한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from decomp_pathstart_oracle import StartOracle, ATTACH, DISPOSE, TYPES
from decomp_rawfinder_oracle import FinderOracle, ROOT, HEAP, VTABLE, bits, sha
from decomp_bridgedecay_oracle import STACK, UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP
from unicorn.x86_const import UC_X86_REG_ESP

FIND = ROOT/'cpppj/tests/fixtures/dude-footprint-find-x86.tsv'  # 두 판본의 새 후보/전체 커서 원문이다.
FAIL = ROOT/'cpppj/tests/fixtures/dude-footprint-fail-x86.tsv'  # 기존 PathStart 원문을 덮어쓰지 않는 실패 원문이다.
REPORT = ROOT/'cpppj/dude-footprint-native.json'  # 보호 원본과 실제 경계·반환 수다.
CONTROLS = (0x27f, 0x37f)  # 원본의53/64비트 제어어를 각각 실행한다.
DIMENSIONS = ((0, 0), (0, 1), (1, 0), (1, 1), (3, 2))  # Dude와 이전 양수 발자국 회귀 입력이다.
AREAS = ((20, 21, 20, 21), (21, 22, 21, 22), (19, 20, 22, 23))  # 접촉/밖/전체 후보의 정수 범위다.
POSITIONS = ((8.75, 8.75), (9.5, 9.5), (10.5, 10.5), (11.25, 10.25))  # 부모9.2/9.3의0/양수 발자국 양쪽을 구분한다.
SOURCES = ((9.2, 9.3), (0, 9.3), (9.2, 0), (255.25, 9.3))  #40e800이 원래 점 전체를 유효/무효로 판정하는 경계를 포함한다.


class GuardedCalls:
    """기존 원본 호출기에 GP 보존·스택 canary와 실제 정상 반환 수 검사를 더한다."""
    def call(self, name, args=(), this=0, purge=0, control=CONTROLS[0]):
        """입력 스택 끝의 canary를 두고 원본 retN과 모든 비휘발 레지스터를 확인한다."""
        marker = STACK+4+4*len(args)
        self.mu.mem_write(marker, b'\xc0\xde\xc0\xde')
        result = super().call(name, args, this, purge, control)
        assert bytes(self.mu.mem_read(marker, 4)) == b'\xc0\xde\xc0\xde'
        # 기존 호출기는 모든 GP를0으로 초기화하므로 보존 레지스터도0으로 복구되어야 한다.
        for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            assert self.mu.reg_read(register) == 0
        self.normal_returns += 1
        return result


class ZeroFinder(GuardedCalls, FinderOracle):
    """후보 계산·버킷·cursor·CRT는 실제 원본 일반 탐색을 그대로 실행한다."""
    def __init__(self, edition):
        """기존 보호 이미지/함수 범위만 재사용하며 새 원문용 카운터를 분리한다."""
        super().__init__(edition)
        self.normal_returns = 0  # Begin/Next 각각의 실제 정상 반환 수다.


class FailedStart(GuardedCalls, StartOracle):
    """실제 Dude24 생성/빈 Init와 전체 실패 finder/겹침/재귀 몸체를 실행한다."""
    def __init__(self):
        """기존 실제 Start 범위에 Dude24 생성자만 더한다. 코드 바이트는 바꾸지 않는다."""
        super().__init__('originals')
        self.normal_returns = 0  # 외부 Start 호출의 실제 정상 반환 수다.
        self.configured = False  # 재귀 시작에서 입력이나 동적 변경을 되돌리지 않는다.
        self.case = None  # 이번 명시 발자국/후보 위치/재시도/효과 변경 입력이다.
        self.areas = []  # 실제 finder Begin이 받은 정수 네 좌표다.
        self.actual_instructions = 0  # 경계 진입도 포함하는 이번 실제 명령 수다.
        # 새 생성자의 정밀 함수 범위만 추가한다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream, delimiter='\t'):
                if int(row['entry'], 16) != 0x48bf30:
                    continue
                self.entries[0x48bf30] = '0048bf30'
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed[low-self.base:high+1-self.base] = b'\x01'*(high+1-low)

    def on_instruction(self, mu, address, size, data):
        """생성자 선택/입력 배치만 바꾸고 원본 실패 계산과 전체 후보 순회를 실행한다."""
        self.actual_instructions += 1
        if address == self.p['start'] and not self.configured:
            width, height, source_x, source_y, candidate_x, candidate_y, candidate_zero, retry, mutation = self.case
            self.configured = True
            self.put(TYPES+70*self.type_stride+self.foot_offset, width)
            self.put(TYPES+70*self.type_stride+self.foot_offset+4, height)
            self.put(TYPES+24*self.type_stride+0x1d0, 0x48bf30)
            self.put(0x50e3d8, DISPOSE)
            self.put(0x50e3d8+0x2c, ATTACH)
            self.put(self.p['retry'], retry)
            # 실패 경로가 사용하는1단계만 재배치하고 표면/나머지 원본 입력은 유지한다.
            self.mu.mem_write(self.bases[1], bytes((256//2)**2*2))
            x, y = (struct.unpack('<f', struct.pack('<I', value))[0] for value in (candidate_x, candidate_y))
            sx, sy = (struct.unpack('<f', struct.pack('<I', value))[0] for value in (source_x, source_y))
            self.put(self.slot(5)+14, source_x)
            self.put(self.slot(5)+18, source_y)
            self.put(self.slot(53)+14, candidate_x)
            self.put(self.slot(53)+18, candidate_y)
            self.put(self.slot(53)+4, 0, 2)
            self.put(TYPES+150*self.type_stride+self.foot_offset, 0 if candidate_zero else 1)
            self.put(TYPES+150*self.type_stride+self.foot_offset+4, 0 if candidate_zero else 1)
            # 부모의 실제 Dude genus도 후보로 존재하지만 실패 겹침 genus와는 다르다.
            self.put(TYPES+70*self.type_stride+0xec, 0x400)
            self.put(self.slot(5)+4, 0, 2)
            for sid, px, py in ((5, sx, sy), (53, x, y)):
                bucket = self.bases[1]+((int(py)//2)*128+int(px)//2)*2
                self.put(self.slot(sid)+4, self.read(bucket, 2), 2)
                self.put(bucket, sid, 2)
        if address == self.finder_spec['Begin']:
            esp = mu.reg_read(UC_X86_REG_ESP)
            self.areas.append(','.join(map(str, struct.unpack('<4i', mu.mem_read(esp+4, 16)))))
        if address == self.p['clear'] and self.case[8]:
            self.put(self.slot(53)+self.o['extra'], 8, 1)
        return super().on_instruction(mu, address, size, data)

    def sample(self, case, control):
        """초기 표면 없는 원본 Init의 Count0부터 전체 실패·정리·재시도 출력을 읽는다."""
        self.case = case
        self.configured = False
        self.areas = []
        result = self.run((70, 6, 1, 0, 0, 0), control)
        assert result[0] == 0 and result[3] == '-'
        return '|'.join((str(result[0]), result[1], str(result[4]), result[5], ';'.join(self.areas), str(self.read(self.slot(53)+self.o['extra'], 1))))


def find_cases():
    """유효한0/양수 크기와 확장 높이·단계·접촉·부류 시작 입력을 교차한다."""
    return list(itertools.product(range(5), range(3), (0, 1, 4, 5), (0, 2), (0, 1)))


def fail_cases():
    """현재 점0 경계와 후보0크기·효과 후 매장·재시도 비트를 실제 float 입력으로 구분한다."""
    rows = []
    # 위치 입력의 최종 float 비트도 저장하여 C++가 상대 위치 수식을 다시 계산하지 않게 한다.
    for (width, height), (sx, sy), (px, py), zero, retry, mutation in itertools.product(DIMENSIONS, SOURCES, POSITIONS, (0, 1), (0, 1), (0, 1)):
        x, y = max(0.25, min(255.75, sx+px-9.2)), max(0.25, min(255.75, sy+py-9.3))
        rows.append((width, height, bits(sx), bits(sy), bits(x), bits(y), zero, retry, mutation))
    return rows


def objects(case):
    """기대 후보를 계산하지 않고 실제 raw TYPE와 세 해시 입력만 만든다."""
    dimension, area, flags, extension, level = case
    width, height = DIMENSIONS[dimension]
    return [(50, 70, 0, 0, bits(20.75), bits(21.25), width, height, extension, level), (60, 71, 0, 0, bits(21.25), bits(22.75), 1, 1, 0, 1), (61, 72, 0, 8, bits(19.75), bits(20.5), width, height, extension, 1)]


def main():
    """최초 원문을 별도 동결하거나 원본/도구/정상 반환·정밀도 일치만 읽기 전용 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'))
        # 최초 원문과 보호PE·도구·정밀 근거를 각각 검사한다.
        for path, expected in report['sha256'].items():
            assert sha(ROOT/path) == expected, path
        find = [line.split('\t') for line in FIND.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        fail = [line.split('\t') for line in FAIL.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert len(find) == 2*len(find_cases()) == report['finderInputs']
        assert len(fail) == len(fail_cases()) == report['failedStartInputs']
        assert all(row[-1] == row[-2] for row in find+fail)
        assert report['failedStartReturns'] == 2*len(fail)
        print('발자국0 원본 감사:', len(find), '탐색 입력/', len(fail), '실패 입력')
        return
    find_rows = []
    find_reports = {}
    paths = {ROOT/'tools/decomp_dude_footprint.py', ROOT/'tools/decomp_pathstart_oracle.py', ROOT/'tools/decomp_rawfinder_oracle.py', ROOT/'tools/decomp_bridgeeffects_oracle.py', ROOT/'tools/decomp_bridgedecay_oracle.py', ROOT/'extracted/refined/originals/functions.tsv'}
    # 두 판본의 finder 전체 cursor를 두 정밀도로 각각 실제 실행한다.
    for edition in ('originals', 'originalCD'):
        oracle = ZeroFinder(edition)
        for case in find_cases():
            description = ';'.join(','.join(map(str, item)) for item in objects(case))
            output = [oracle.find(objects(case), AREAS[case[1]], case[2], 0, control) for control in CONTROLS]
            assert output[0] == output[1]
            find_rows.append((edition, ','.join(map(str, case)), description, ','.join(map(str, AREAS[case[1]])), str(case[2]), *output))
        find_reports[edition] = dict(normalReturns=oracle.normal_returns, instructions=oracle.instructions, calls=dict(oracle.native_calls))
        paths.update(oracle.body_paths)
        paths.add(ROOT/oracle.spec['binary'])
    fail_rows = []
    oracle = FailedStart()
    # 제품 경로를 사용하지 않고 실제 ctor/Init/사각형/finder/재귀 결과를 얻는다.
    for case in fail_cases():
        output = [oracle.sample(case, control) for control in CONTROLS]
        assert output[0] == output[1]
        fail_rows.append((','.join(map(str, case)), *output))
    paths.update(oracle.body_paths)
    FIND.write_text('# edition input objects area flags output53 output64\n'+'\n'.join('\t'.join(row) for row in find_rows)+'\n', encoding='utf-8', newline='\n')
    FAIL.write_text('# width,height,sourceXbits,sourceYbits,candidateXbits,candidateYbits,candidateZero,retry,mutation success|effects|frame|remaining|areas|candidateExtra53 output64\n'+'\n'.join('\t'.join(row) for row in fail_rows)+'\n', encoding='utf-8', newline='\n')
    paths.update((FIND, FAIL))
    report = dict(host=socket.gethostname(), date='2026-10-21', lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-20'), finderInputs=len(find_rows), finder=find_reports, failedStartInputs=len(fail_rows), failedStartReturns=oracle.normal_returns, failedStartInstructions=oracle.actual_instructions, failedStartCalls=dict(oracle.native_calls), controls=list(CONTROLS), osCalls=0, boundaries='일반 finder Begin/Next·현재0크기 TYPE·전체cursor·부류 시작·확장 높이는 두 판본 실제 명령. 실패는10.78 Start/24ctor/Init/49ae80 minmax/41def0/실제 finder/재귀 전체. 시작 표면 부재로Init가자연 반환하며BFS는 도달하지 않는다. new·구 경로 lookup·Dispose·겹침 효과는 명시 경계. GP/retN/canary/x87/허용 코드·쓰기 범위 검사. 게임/OS/창/C++제품 실행 없음.', sha256={path.relative_to(ROOT).as_posix(): sha(path) for path in sorted(paths) if path.exists()})
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('finderInputs', 'failedStartInputs', 'failedStartReturns', 'failedStartInstructions')}))


if __name__ == '__main__':
    main()
