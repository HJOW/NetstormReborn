#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""영역 무효화와 일반 다리/섬 Pop을 세 실행 파일의 실제 x86으로 대조한다."""
import argparse
import collections
import csv
import hashlib
import json
import struct
from pathlib import Path
from decomp_rawgraph_oracle import RawGraphOracle, TABLE, FLOOD
from decomp_graph_oracle import records_for
from decomp_pop_oracle import HEADS
from decomp_sid_oracle import POOL
from decomp_surface_oracle import packed
from decomp_oracle import ROOT, pefile

# 실제 실행 파일 세 개이며 CD와 10.37은 동일한 그래프 코드/자료 배치를 가진다.
BINARIES = {'originals': 'originals/Netstorm.exe', 'originalCD': 'originalCD/NETSTORM.EXE',
            'original1037': 'original1037/netstorm.exe'}
# 부모 구현은 SHA가 고정된 기존 검증 도구를 그대로 사용한다.
DEPENDENCIES = ('decomp_rawgraph_oracle.py', 'decomp_oracle.py', 'decomp_sid_oracle.py',
                'decomp_creation_oracle.py', 'decomp_pop_oracle.py', 'decomp_display_oracle.py',
                'decomp_postpop_oracle.py', 'decomp_graph_oracle.py', 'decomp_surface_oracle.py',
                'decomp_bridge_oracle.py')


class RegionOracle(RawGraphOracle):
    """일반 탐색/영역 helper만 추가하고 결과 계산이나 대체 함수는 사용하지 않는다."""
    def __init__(self, binary):
        """10.37은 실제 PE를 복사한다. 메타데이터 공유는 유일한 차이의 범위 검사로 제한한다."""
        layout = 'originalCD' if binary == 'original1037' else binary
        super().__init__(layout)
        self.binary = binary
        path = ROOT / BINARIES[binary]
        self.sha256 = hashlib.sha256(path.read_bytes()).hexdigest()
        if binary == 'original1037':
            old = (ROOT / BINARIES['originalCD']).read_bytes()
            new = path.read_bytes()
            assert len(old) == len(new) and [(i, a, b) for i, (a, b) in enumerate(zip(old, new)) if a != b] == [(0x3314c, 0x75, 0xeb)]
            pe = pefile.PE(str(path))
            self.mu.mem_write(pe.OPTIONAL_HEADER.ImageBase, pe.get_memory_mapped_image())
            # CD와 공유한 허용 함수 중 패치 분기를 포함하는 몸체가 있으면 실행 전에 거부한다.
            assert not any(lo <= 0x433d4c < hi for lo, hi in self.allowed)
        self.region_entry = 0x462d40 if layout == 'originals' else 0x45c210
        self.region_calls = 0
        # 사각형 최소/최대 helper도 각각 내보낸 실제 몸체를 읽는다.
        for suffix in ('', '/bounds'):
            with (ROOT / f'extracted/regiongraph/{binary}{suffix}/functions.tsv').open(encoding='utf-8') as fp:
                # 각 새 Ghidra 몸체의 불연속 범위를 그대로 허용한다. InsertCD 함수는 실행하지 않는다.
                for row in csv.DictReader(fp, delimiter='\t'):
                    if int(row['entry'], 16) == 0x433bf0:
                        continue
                    self.facts.append(row)
                    # 함수 사이의 미검토 명령은 허용하지 않는다.
                    for part in row['ranges'].split(';'):
                        lo, hi = (int(v, 16) for v in part.split('-'))
                        self.allowed.append((lo, hi + 1))

    def on_instruction(self, mu, address, size, data):
        """후처리 내부의 실제 영역 호출도 상위 호출 수와 구분해 센다."""
        if address == getattr(self, 'region_entry', None):
            self.region_calls += 1
        super().on_instruction(mu, address, size, data)

    def prepare_region(self, nodes, spots, records, seed):
        """네 단계 해시의 실제 머리와 next 체인을 입력한다. 무효화 결과는 계산하지 않는다."""
        stack = struct.pack('<4096I', *((seed + i) & 0xffffffff for i in range(4096)))
        self.prepare_graph(nodes, spots, bytes(records), stack)
        self.mu.mem_write(HEADS, bytes(86272 * 2))
        # 같은 버킷의 앞선 SID를 next에 보존하고 마지막 입력을 머리로 만든다.
        for i, node in enumerate(nodes):
            if node[9] & 4:
                continue
            level = node[12]
            scale = (1, 2, 4, 16)[level]
            index = (node[1] // scale) * (256 // scale) + node[0] // scale
            head = self.bases[level] + index * 2
            self.mu.mem_write(POOL + self.ids[i] * self.stride + 4, bytes(self.mu.mem_read(head, 2)))
            self.mu.mem_write(head, struct.pack('<H', self.ids[i]))

    def region_step(self, name, args):
        """cdecl 영역 helper도 정상 반환/x87/쓰기 제한을 적용한다."""
        if name == 'Region':
            self.invoke(self.region_entry, [self.ids[0]], 0, 0)
            return self.graph_output('-')
        return self.graph_step(name, args)


def digest(path):
    """파일을 수정하지 않고 SHA-256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def generate():
    """합성 SID/타입/지도에서 영역 단독·Pop→영역→Add의 실제 명령 결과를 기록한다."""
    rows = ['# 실제 x86, 대체 함수 없음. 10.37/CD는 같은 코드 배치이며 서로 다른 PE를 실행한다.']
    counts = collections.Counter()
    reports = {}
    # 세 PE와 x87 53/64비트를 독립적으로 실행한다.
    for binary in BINARIES:
        oracle = RegionOracle(binary)
        # 정밀도별 실제 Reset/Create부터 다시 시작한다.
        for control in (0x027f, 0x037f):
            oracle.start_graph(control)
            rows.append(f'Begin\t{binary}\t{control}\t' + ','.join(map(str, oracle.ids)))
            # 일반 원천/표면 원천/일반 다리/일반 섬과 네 단계·체인·교차 발자국을 섞는다.
            for case in range(64):
                mode = case % 4
                coords = [(20, 20), (19, 20), (21, 20), (20, 19), (20, 21), (17, 18), (30, 30)]
                nodes = []
                # 원천/이웃 슬롯마다 독립적인 발자국·타입·상태·해시 단계를 입력한다.
                for i, (x, y) in enumerate(coords):
                    f1 = 0 if (i == 0 and mode == 0) or (i == 5 and case % 3 == 0) else 0x800
                    f2 = (8, 8, 4, 2)[mode] if i == 0 else 4
                    width = (5 if mode == 0 else 3 if mode in (1, 3) else 1) if i == 0 else (3 if i in (2, 4) else 1)
                    height = width if i == 0 else (3 if i == 4 else 1)
                    state = 4 if i == 0 and mode else (2 if i == 4 and case % 7 == 0 else 0)
                    graph = 254 if i == 0 or case % 5 == 0 else i % 3
                    nodes.append([90 if i == 0 and mode else x, 90 if i == 0 and mode else y,
                                  width, height, f1, f2, 65, 80, (i + case) % 2, state,
                                  8 if i == 3 and case % 6 == 0 else 0, graph,
                                  1 if i == 0 else (i + case // 4) % 4])
                spots = [(x, y, 8) for i, (x, y) in enumerate(coords) if i != 0 and (i + case) % 3 != 0]
                members = [(oracle.ids[i], n[11], n[9]) for i, n in enumerate(nodes) if n[4] & 0x800]
                records = bytearray(records_for(members, case * 11))
                if mode == 0 and case % 16 == 0:
                    # Remove의 비활성/0/음수 WORD 경로도 표를 건드리지 않고 번호만 무효화한다.
                    struct.pack_into('<2h', records, 0, 0, 1)
                    struct.pack_into('<2h', records, 6, -3, 1)
                    struct.pack_into('<2h', records, 12, 5, 0)
                seed = case * 17
                oracle.prepare_region(nodes, spots, records, seed)
                rows.append('\t'.join(['Setup', packed(nodes), packed(spots), records.hex(), str(seed)]))
                steps = ([('Region', []), ('PostPop', [1]), ('Region', []), ('PostPop', [0x2000001])]
                         if mode == 0 else [('Pop', [0x201, 20, 20]), ('PostPop', [1]), ('Region', []), ('Add', [])])
                # 실제 기계어 호출 직후의 관찰값만 기대값으로 저장한다.
                for name, args in steps:
                    rows.append('\t'.join(map(str, [name, ','.join(map(str, args)) or '-', *oracle.region_step(name, args)])))
                    counts[name] += 1
        exports = ROOT / f'extracted/regiongraph/{binary}'
        reports[binary] = dict(binary_sha256=oracle.sha256, layout=oracle.edition, function_ranges=oracle.facts,
                               assert_calls=oracle.assertions, region_calls=oracle.region_calls,
                               graph_calls=dict(oracle.graph_calls), lifecycle_calls=dict(oracle.actual_calls),
                               preparation_calls=dict(Reset=2, Create=14), cost_encode_segments=oracle.encodings,
                               exports={n: digest(exports / n) for n in ('creation.c', 'functions.tsv', 'bounds/creation.c', 'bounds/functions.tsv')})
        print(f'{binary}: 영역/일반 다리/섬 실제 x86 완료', flush=True)
    fixture = ROOT / 'cpppj/tests/fixtures/regiongraph-x86.tsv'
    fixture.write_text('\n'.join(rows) + '\n', encoding='utf-8', newline='\n')
    report = dict(schema=1, public_calls=dict(counts), total_calls=sum(counts.values()), sequences=6,
                  editions=reports, stubbed=[], tool_sha256=digest(Path(__file__)), fixture_sha256=digest(fixture),
                  dependencies={n: digest(ROOT / 'tools' / n) for n in DEPENDENCIES},
                  limitations=['세 PE지만 그래프 알고리즘은 두 배치; CD와 10.37은 InsertCD 분기 하나 외에 동일',
                               '합성 타입/FrameCode/SHP, client SID 32768, 정수 좌표, 비전투 dirty 큐 null, AI/배치 선택 없음',
                               '7개 raw 슬롯과 모든 dirty 항목 직접 비교; 전체 풀/해시/spot/표/스택/통계는 Adler-32',
                               '일반 다리/섬 Unpop, 건물 부착, 삭제 분할/전역 복구와 raw GameWorld는 미복원',
                               '준비 Reset/Create/비용 인코딩 접두 구간과 내부 호출은 공개 호출 수에 더하지 않음'])
    (ROOT / 'cpppj/recovery-regiongraph-evidence.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f'영역 그래프 x86 fixture: {report["total_calls"]}회 {dict(counts)}')


def verify():
    """게임/기계어 실행 없이 PE/도구/내보내기/fixture SHA와 호출 수를 확인한다."""
    r = json.loads((ROOT / 'cpppj/recovery-regiongraph-evidence.json').read_text(encoding='utf-8'))
    assert r['tool_sha256'] == digest(Path(__file__)) and r['stubbed'] == []
    # 실제 원본과 각 새 Ghidra 내보내기가 기록과 일치해야 한다.
    for binary, data in r['editions'].items():
        assert data['binary_sha256'] == digest(ROOT / BINARIES[binary]) and data['assert_calls'] == 0
        # C 결과와 몸체 사실 파일을 별도로 확인한다.
        for name, sha in data['exports'].items():
            assert sha == digest(ROOT / f'extracted/regiongraph/{binary}' / name)
    # 부모 도구의 이전 복원 입력/동작도 같은 파일로 유지한다.
    for name, sha in r['dependencies'].items():
        assert sha == digest(ROOT / 'tools' / name)
    fixture = ROOT / 'cpppj/tests/fixtures/regiongraph-x86.tsv'
    assert r['fixture_sha256'] == digest(fixture)
    counts = collections.Counter(line.split('\t')[0] for line in fixture.read_text(encoding='utf-8').splitlines())
    assert all(counts[name] == count for name, count in r['public_calls'].items())
    assert sum(r['public_calls'].values()) == r['total_calls'] and counts['Begin'] == r['sequences']
    print(f'영역 그래프 근거 SHA/호출 수 확인: {r["total_calls"]}회')


if __name__ == '__main__':
    # 검증 모드는 새 실행 없이 기존 결과의 원본/생성 입력/기대값을 확인한다.
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    verify() if parser.parse_args().verify else generate()
