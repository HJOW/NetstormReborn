#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Add/Detach/Pop/postPop에서 실제 Graph 소진 복구가 이어지는 순서를 격리 대조한다."""
import argparse
import collections
import csv
import json
import struct
from pathlib import Path
from decomp_graphrebuild_oracle import RebuildOracle, PARENTS
from decomp_graphremove_oracle import BINARIES, digest
from decomp_rawgraph_oracle import TABLE
from decomp_surface_oracle import packed
from decomp_oracle import ROOT
from unicorn.x86_const import UC_X86_REG_ESP

# 기존 독립 도구와 fixture를 보존하는 새 출력이다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/graphrecovery-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-graphrecovery-evidence.json'
DEPENDENCIES = ('decomp_graphrebuild_oracle.py',) + PARENTS
# 각 입력은 네 상위 경로의 전역 복구를 실제 전체 풀에서 실행한다.
CASE_COUNT = 8


class RecoveryOracle(RebuildOracle):
    """이번 상위 여섯 함수의 내보내기를 추가하고 내부 전역 복구 도달을 관찰한다."""
    def __init__(self, binary):
        """기존 엄격한 실제 x86 기반에 새 읽기 전용 몸체만 추가한다."""
        super().__init__(binary)
        with (ROOT/f'extracted/graphrecovery/{binary}/functions.tsv').open(encoding='utf-8') as fp:
            # 함수 사이 미검토 코드나 OS API를 허용하지 않는다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.facts.append(row)
                # 각 불연속 몸체의 시작/끝만 사용한다.
                for part in row['ranges'].split(';'):
                    lo, hi = (int(v, 16) for v in part.split('-'))
                    self.allowed.append((lo, hi+1))
        # 원본 명령을 건너뛰지 않고 고정된 검토 범위 조회만 캐시한다.
        self.allowed_ends = {address: hi for lo, hi in self.allowed for address in range(lo, hi)}
        self.tracked_entries.update({self.delete[0]: 'Detach', self.space['pop']: 'Pop', self.space['post']: 'PostPop'})

    def on_instruction(self, mu, address, size, data):
        """assert를 실행하지 않고 손상된 준비 입력의 조건 문자열을 진단한다."""
        if address == self.spec['assert']:
            self.assertions += 1
            args = struct.unpack('<4I', mu.mem_read(mu.reg_read(UC_X86_REG_ESP),16))
            message = bytes(mu.mem_read(args[1],256)).split(b'\0')[0].decode('ascii',errors='replace')
            raise RuntimeError(f'원본 assert 즉시 중단: {self.binary} {message} / {args[3]:x}')
        return super().on_instruction(mu,address,size,data)

    def recovery_step(self, name, args):
        """실제 상위 함수를 호출하고 슬롯/표/스택/공간/통계/dirty 출력만 읽는다."""
        output = self.delete_step(name, args) if name == 'Detach' else self.graph_step(name, args)
        return [*output, bytes(self.mu.mem_read(TABLE, 255*6)).hex()]


def generate():
    """세 실제 PE×두 x87 정밀도에서 251개가 사용 중인 네 상위 경로를 실행한다."""
    rows = ['# Add/Detach/Pop/postPop의 실제 전역 소진 복구; 32768 전체 풀·대체 함수/assert/OS 없음.']
    counts, reports = collections.Counter(), {}
    # CD/10.37의 코드 배치는 같지만 실제 각 PE 바이트를 독립 실행한다.
    for binary in BINARIES:
        oracle = RecoveryOracle(binary)
        # 두 x87 정밀도에서 동일 정수 입력과 상위 호출 순서를 사용한다.
        for control in (0x027f, 0x037f):
            # 앞 시퀀스의 특수 9 감소 타입은 생성 금지 타입이다. 실제 Create 준비 전 그 합성 입력을 비운다.
            oracle.mu.mem_write(oracle.delete[2],struct.pack('<I',255))
            oracle.start_graph(control)
            rows.append(f'Begin\t{binary}\t{control}\t' + ','.join(map(str, oracle.ids)))
            # 원천/이웃 프레임·void/contained·genus·분할 정책과 특수 9 감소를 바꾼다.
            for case in range(CASE_COUNT):
                # 모든 단계는 같은 최초 표에서 시작한다. Python은 복구 결과를 계산하지 않는다.
                for name in ('Add', 'PostPop', 'Pop', 'Detach'):
                    coords = [(20,20),(19,20),(21,20),(20,19),(20,21),(18,20),(50,50)]
                    nodes = []
                    # 실제 슬롯과 합성 타입/프레임은 초기 입력으로만 기록한다.
                    for i, (x, y) in enumerate(coords):
                        state = 2 if i == 0 and name == 'Detach' else 4 if i == 0 and name == 'Pop' else 0
                        if i == 6 and case >= 4:
                            state = 8 if case == 7 else 4
                        f1 = 0 if i == 6 and case == 3 else 0x800
                        f2 = (8 if case >= 4 else 4) if i == 0 and name == 'Detach' else 0 if i == 0 else 2
                        graph = 0 if name == 'Detach' else (case % 3 if i == 0 else 254)
                        level = 1 if i == 6 else 0
                        nodes.append([80 if i == 0 and name == 'Pop' else x,
                                      80 if i == 0 and name == 'Pop' else y, 1, 1, f1, f2,
                                      ord('L') if i == 1 and case % 2 else ord('A'), ord('P'),
                                      case % 2 if i == 0 else 0, state, 0, graph, level])
                    spots = [(21,20,8)] if case % 4 == 2 else []
                    # 전체 표의 stale 크기와 reserved를 넣고 모든 정상 레코드를 사용 중으로 만든다.
                    records = b''.join(struct.pack('<3H', 32767-i if i < 251 else 0x7dfd, 1 if i < 254 else 0x7dfd,
                                                    (0xab00+case*13+i*7)&0xffff) for i in range(255))
                    rebuild, removed, seed = int(bool(case&2)), 9 if case >= 4 else 1, case*17
                    oracle.prepare_delete(nodes, spots, records, seed, rebuild, removed)
                    args = [rebuild, removed] if name == 'Detach' else [0x200,20,20] if name == 'Pop' else [0x201] if name == 'PostPop' else []
                    rows.append('\t'.join(['Setup', packed(nodes), packed(spots), records.hex(), str(seed)]))
                    before = oracle.rebuild_calls['Rebuild']
                    output = oracle.recovery_step(name, args)
                    assert oracle.rebuild_calls['Rebuild'] == before+1, (binary, case, name)
                    rows.append('\t'.join(map(str, [name, ','.join(map(str,args)) or '-', *output])))
                    counts[name] += 1
                print(f'{binary} x87 {control:04x}: 상위 복구 입력 {case+1}/{CASE_COUNT}', flush=True)
        exports = {}
        # 실행한 새/부모 내보내기의 정확한 전체 SHA를 기록한다.
        for directory in ('graphrecovery','graphrebuild','graphremove'):
            path = ROOT/f'extracted/{directory}/{binary}'
            names = ['creation.c','functions.tsv']
            if directory == 'graphremove':
                names += ['geometry/creation.c','geometry/functions.tsv']
                if oracle.edition != 'originals':
                    names += ['lookup/creation.c','lookup/functions.tsv']
            exports[directory] = {name: digest(path/name) for name in names}
        reports[binary] = dict(binary_sha256=oracle.sha256, capacity=oracle.capacity,
                              actual_calls=dict(oracle.rebuild_calls), assert_calls=oracle.assertions,
                              preparation_calls=dict(Reset=2, Create=14), exports=exports)
    FIXTURE.write_text('\n'.join(rows)+'\n', encoding='utf-8', newline='\n')
    report = dict(schema=1, primary_target='10.78', total_calls=sum(counts.values()), public_calls=dict(counts),
                  sequences=6, cases_per_sequence=CASE_COUNT, editions=reports, stubbed=[],
                  fixture_sha256=digest(FIXTURE), tool_sha256=digest(Path(__file__)),
                  dependencies={name: digest(ROOT/'tools'/name) for name in DEPENDENCIES},
                  limitations=['정수·합성 자산 타입/FrameCode/SHP·32768 전체 raw 풀·기존 스택·동결 시계·비전투 null 큐',
                               '7개 슬롯/255개 Graph 레코드/dirty 직접 비교; 전체 풀/해시/spot/스택/통계 Adler-32',
                               '상위 공개 192회 중 각 호출 내부 재구성 1회; 준비/내부 도달은 상위 수에 더하지 않음',
                               'form/process·재구성 중 재소진·SID 소진 파생 삭제·raw GameWorld/GUI는 미검증'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')


def verify():
    """원본 실행 없이 원본/부모/새 내보내기/fixture SHA와 공개·내부 호출 수를 읽는다."""
    r = json.loads(REPORT.read_text(encoding='utf-8'))
    assert r['tool_sha256'] == digest(Path(__file__)) and r['fixture_sha256'] == digest(FIXTURE)
    assert r['primary_target'] == '10.78' and r['stubbed'] == []
    # 각 실제 PE와 검토 몸체/관찰 결과의 바이트를 확인한다.
    for binary, data in r['editions'].items():
        assert data['binary_sha256'] == digest(ROOT/BINARIES[binary]) and data['assert_calls'] == 0
        assert data['capacity'] == 32768 and data['actual_calls']['Rebuild'] == CASE_COUNT*8
        # 새/부모 몸체는 별도 디렉터리로 보존한다.
        for directory, files in data['exports'].items():
            # 불연속 범위 표와 디컴파일 C 모두 당시 바이트여야 한다.
            for name, value in files.items():
                assert value == digest(ROOT/f'extracted/{directory}/{binary}'/name)
    # 원본 준비와 관찰에 사용한 모든 부모 도구의 SHA를 확인한다.
    for name, value in r['dependencies'].items():
        assert value == digest(ROOT/'tools'/name)
    counts = collections.Counter(line.split('\t')[0] for line in FIXTURE.read_text(encoding='utf-8').splitlines())
    assert counts['Begin'] == r['sequences'] == 6
    assert all(counts[name] == value for name, value in r['public_calls'].items())
    assert sum(r['public_calls'].values()) == r['total_calls'] == CASE_COUNT*24
    print(f'상위 Graph 소진 복구 SHA/호출 수 확인: {r["total_calls"]}회')


if __name__ == '__main__':
    # --verify는 에뮬레이션 없이 기록만 확인한다.
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    verify() if parser.parse_args().verify else generate()
