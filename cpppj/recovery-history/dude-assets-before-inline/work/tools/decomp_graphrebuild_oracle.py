#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 기준 전체 Graph 재구성/소진 할당을 실제 PE 함수만 격리 실행한다."""
import argparse
import collections
import csv
import json
import struct
from pathlib import Path
from decomp_graphremove_oracle import DeleteOracle, BINARIES, DEPENDENCIES, digest
from decomp_graph_oracle import records_for
from decomp_rawgraph_oracle import TABLE
from decomp_surface_oracle import packed
from decomp_oracle import ROOT
from decomp_sid_oracle import SidOracle
from unicorn.x86_const import UC_X86_REG_EAX

# 기존 독립 도구/fixture는 수정하지 않고 새 결과와 SHA를 기록한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/graphrebuild-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-graphrebuild-evidence.json'
PARENTS = ('decomp_graphremove_oracle.py', 'decomp_bridge_oracle.py') + DEPENDENCIES
REBUILD = {'originals': 0x463110, 'originalCD': 0x45bd60}
# 실제 전체 풀 순회 비용을 포함한 16가지 입력을 각 세 경로/판본/정밀도에서 실행한다.
CASE_COUNT = 16


class RebuildOracle(DeleteOracle):
    """기존 실제 풀 준비/출력 관찰에 검토한 전역 재구성 몸체만 추가한다."""
    def __init__(self, binary):
        """이번 읽기 전용 Ghidra 범위를 추가하고 함수 도달만 집계한다."""
        super().__init__(binary)
        self.rebuild_entry = REBUILD[self.edition]
        self.rebuild_calls = collections.Counter()
        path = ROOT / f'extracted/graphrebuild/{binary}/functions.tsv'
        with path.open(encoding='utf-8') as fp:
            # 불연속 몸체 사이의 미검토 코드나 OS 호출을 허용하지 않는다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.facts.append(row)
                # 내보낸 각 실제 몸체만 허용 범위에 추가한다.
                for part in row['ranges'].split(';'):
                    lo, hi = (int(v, 16) for v in part.split('-'))
                    self.allowed.append((lo, hi + 1))
        # 전체 풀 iterator도 모든 명령을 검증하되 고정 범위 조회만 캐시한다. 명령/루프를 건너뛰지 않는다.
        self.allowed_ends = {address: hi for lo, hi in self.allowed for address in range(lo, hi)}
        self.tracked_entries = {self.rebuild_entry: 'Rebuild', **{entry: name for name, entry in self.graph['entries'].items()}}

    def on_instruction(self, mu, address, size, data):
        """재구성/내부 할당·flood 도달을 공개 입력 횟수와 구분한다."""
        if not hasattr(self, 'allowed_ends') or address in (self.spec['assert'], self.spec['copy']):
            return SidOracle.on_instruction(self, mu, address, size, data)
        self.instructions += 1
        if address+size > self.allowed_ends.get(address, 0):
            raise RuntimeError(f'미검토 Graph 명령: {address:08x}')
        name = self.tracked_entries.get(address)
        if name:
            self.rebuild_calls[name] += 1

    def rebuild_step(self, name, reset):
        """32768슬롯 전체 순회와 정상 스택 반환 뒤 모든 출력만 관찰한다."""
        entry = self.rebuild_entry if name == 'Rebuild' else self.graph['entries']['Allocate']
        self.invoke(entry, [reset] if name == 'Rebuild' else [])
        value = '-' if name == 'Rebuild' else (self.mu.reg_read(UC_X86_REG_EAX) - TABLE) // 6
        return [*self.graph_output(value), bytes(self.mu.mem_read(TABLE, 255*6)).hex()]


def generate():
    """무효 수집/전체 재구성/251개 소진을 각 실제 PE의 두 x87 정밀도로 실행한다."""
    rows = ['# 전체 32768 raw 풀·재구성/소진 할당; 실제 x86만 실행, 대체 함수/assert/OS 없음.']
    counts, reports = collections.Counter(), {}
    # 두 구버전은 같은 코드 배치이지만 각 실제 PE를 독립 실행한다.
    for binary in BINARIES:
        oracle = RebuildOracle(binary)
        # x87의 53/64비트 정밀도에서도 같은 정수 입력을 사용한다.
        for control in (0x027f, 0x037f):
            oracle.start_graph(control)
            rows.append(f'Begin\t{binary}\t{control}\t' + ','.join(map(str, oracle.ids)))
            # 연결된 선/떨어진 무리·기준점 내부·상태·실제 프레임·다단계 hash를 바꾼다.
            for case in range(CASE_COUNT):
                coords = ([(20+i, 20) for i in range(7)] if case % 2 == 0 else
                          [(20,20),(21,20),(30,30),(31,30),(40,40),(41,40),(50,50)])
                nodes = []
                # 노드는 합성 입력이며 결과 그래프 번호/연결은 Python에서 계산하지 않는다.
                for i, (x, y) in enumerate(coords):
                    state = (1, 2, 4, 8)[(case//4) % 4] if i == 6 else 0
                    level = 1 if i == 6 else 0
                    f1 = 0 if i == 5 and case % 4 == 3 else 0x800
                    graph = 254 if (i+case) % 3 == 0 else (i+case) % 3
                    nodes.append([x, y, 3 if i == 0 and case % 4 == 2 else 1, 1,
                                  f1, 4 if i % 2 else 2, ord('L') if i == 1 and case % 4 == 1 else ord('A'),
                                  ord('P'), case % 2 if i == 2 else 0, state, 8 if i == 4 else 0, graph, level])
                spots = [(nodes[3][0], nodes[3][1], 8)] if case % 4 in (1, 2) else []
                members = [(oracle.ids[i], n[11], n[9]) for i, n in enumerate(nodes) if n[4]&0x800]
                initial = records_for(members, 0xab00+case*13)
                # 각 경로마다 같은 슬롯 입력을 다시 넣어 앞선 결과를 이어받지 않는다.
                for name, reset in (('Rebuild', 0), ('Rebuild', 1), ('Allocate', 1)):
                    records = bytearray(initial)
                    if name == 'Allocate':
                        # 쓰레기 크기/사용 흔적이 남은 251개를 모두 활성으로 하여 실제 소진 분기를 탄다.
                        for i in range(251):
                            struct.pack_into('<2h', records, i*6, 32767-i, 1)
                    seed = case*17
                    oracle.prepare_delete(nodes, spots, bytes(records), seed, 0, 1)
                    rows.append('\t'.join(['Setup', packed(nodes), packed(spots), records.hex(), str(seed)]))
                    rows.append('\t'.join(map(str, [name, str(reset), *oracle.rebuild_step(name, reset)])))
                    counts[name] += 1
                print(f'{binary} x87 {control:04x}: 입력 {case+1}/{CASE_COUNT}', flush=True)
        exports = ROOT / f'extracted/graphrebuild/{binary}'
        base = ROOT / f'extracted/graphremove/{binary}'
        base_names = ['creation.c', 'functions.tsv', 'geometry/creation.c', 'geometry/functions.tsv']
        if oracle.edition != 'originals':
            base_names += ['lookup/creation.c', 'lookup/functions.tsv']
        reports[binary] = dict(binary_sha256=oracle.sha256, layout=oracle.edition,
                              actual_calls=dict(oracle.rebuild_calls), assert_calls=oracle.assertions,
                              preparation_calls=dict(Reset=2, Create=14), capacity=oracle.capacity,
                              exports={name: digest(exports/name) for name in ('creation.c','functions.tsv')},
                              base_exports={name: digest(base/name) for name in base_names})
    FIXTURE.write_text('\n'.join(rows)+'\n', encoding='utf-8', newline='\n')
    report = dict(schema=1, primary_target='10.78', total_calls=sum(counts.values()), public_calls=dict(counts),
                  sequences=6, cases_per_sequence=CASE_COUNT, editions=reports, stubbed=[],
                  tool_sha256=digest(Path(__file__)), fixture_sha256=digest(FIXTURE),
                  dependencies={name: digest(ROOT/'tools'/name) for name in PARENTS},
                  limitations=['합성 타입/FrameCode/SHP·32768 전체 풀·정수 좌표·기존 확보 스택·로그 0·비전투 null 큐',
                               '7개 raw 슬롯과 255개 레코드 전체 byte 직접 비교; 풀/해시/spot/스택/통계 Adler-32',
                               'Graph 전역 재구성/소진 할당만 실행; SID 소진 파생 삭제·raw GameWorld·GUI는 미검증',
                               '250개 이상 독립 무리에서 원본 재귀 소진/assert 경로는 실행하지 않음'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')


def verify():
    """게임/에뮬레이션 실행 없이 모든 입력/부모/내보내기/fixture SHA·호출 수를 확인한다."""
    r = json.loads(REPORT.read_text(encoding='utf-8'))
    assert r['tool_sha256'] == digest(Path(__file__)) and r['fixture_sha256'] == digest(FIXTURE)
    assert r['primary_target'] == '10.78' and r['stubbed'] == []
    # 실행한 실제 PE·새/기존 내보내기의 전체 바이트를 확인한다.
    for binary, data in r['editions'].items():
        assert data['binary_sha256'] == digest(ROOT/BINARIES[binary]) and data['assert_calls'] == 0
        assert data['capacity'] == 32768 and data['actual_calls']['Rebuild'] == CASE_COUNT*6
        # 두 종류 내보내기는 별도 경로와 SHA로 보존한다.
        for key, directory in (('exports', 'graphrebuild'), ('base_exports', 'graphremove')):
            # 기록한 각 파일이 검토 당시 바이트인지 확인한다.
            for name, value in data[key].items():
                assert value == digest(ROOT/f'extracted/{directory}/{binary}'/name)
    # 결과에 영향을 주는 부모 도구도 바이트로 고정한다.
    for name, value in r['dependencies'].items():
        assert value == digest(ROOT/'tools'/name)
    counts = collections.Counter(line.split('\t')[0] for line in FIXTURE.read_text(encoding='utf-8').splitlines())
    assert counts['Begin'] == r['sequences'] == 6
    assert all(counts[name] == value for name, value in r['public_calls'].items())
    assert sum(r['public_calls'].values()) == r['total_calls'] == CASE_COUNT*18
    print(f'전역 Graph 재구성 SHA/호출 수 확인: {r["total_calls"]}회')


if __name__ == '__main__':
    # 기본은 검토한 함수 에뮬레이션이며 --verify는 파일만 읽는다.
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    verify() if parser.parse_args().verify else generate()
