#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 기준 삭제 준비에서 동일 위치의 다른 SID 조회를 실제 x86으로 대조한다."""
import argparse
import collections
import json
from pathlib import Path
from decomp_graphremove_oracle import DeleteOracle, BINARIES, DEPENDENCIES, digest
from decomp_graph_oracle import records_for
from decomp_surface_oracle import packed
from decomp_oracle import ROOT

# 기존 SHA 기록/fixture를 고치지 않고 새 기대값과 기록을 만든다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/graphlookup-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-graphlookup-evidence.json'
PARENTS = ('decomp_graphremove_oracle.py',) + DEPENDENCIES


def generate():
    """실제 Reset/Create와 삭제 helper를 실행하고 입력/전체 raw 출력을 기록한다."""
    rows = ['# 같은 위치 다른 SID의 graph byte와 dead 원천 타입/프레임; 소진 전·대체 함수/OS 없음.']
    editions = {}
    cases = collections.Counter()
    # 구버전 두 PE는 같은 코드 배치이지만 각각의 실제 바이트를 실행한다.
    for binary in BINARIES:
        oracle = DeleteOracle(binary)
        # x87 53/64비트 제어값에서 같은 시퀀스를 실행한다.
        for control in (0x027f, 0x037f):
            oracle.start_graph(control)
            rows.append(f'Begin\t{binary}\t{control}\t' + ','.join(map(str, oracle.ids)))
            # 머리의 정상/무효/미사용 번호·원천 번호·프레임·genus·단계를 독립적으로 바꾼다.
            for case in range(64):
                mode = case % 8
                source_graph = (0, 254, 2, 1)[case // 16]
                lookup_graph = 254 if mode == 1 else 1
                edge = 2 if case >= 32 else 20
                coords = [(edge, edge), (edge-1, edge), (edge+1, edge),
                          (edge, edge-1), (edge, edge+1), (edge-2 if edge>2 else edge+2, edge), (edge, edge)]
                nodes = []
                # 마지막 SID가 원천과 같은 위치의 0단계 머리가 되도록 명시적 입력만 만든다.
                for i, (x, y) in enumerate(coords):
                    state = 2 if i == 0 or (i == 6 and mode != 6) else 0
                    graph = source_graph if i == 0 else lookup_graph if i == 6 else 1 if i < 5 else 2
                    genus = (8 if mode == 4 else 4) if i == 0 else 2
                    level = 1 if i == 0 and mode == 5 else 0
                    side = ord('L') if i == 0 and mode == 3 else ord('A')
                    nodes.append([x, y, 1, 1, 0x800, genus, side, ord('P'), (case // 8) % 2 if i == 0 else 0,
                                  state, 0, graph, level])
                members = [(oracle.ids[i], n[11], n[9]) for i, n in enumerate(nodes)]
                records = bytearray(records_for(members, case*11))
                if mode == 2:
                    records[6:10] = bytes(4)
                spots = [(edge, edge, 8)] if mode == 7 else []
                rebuild, removed, seed = int((case // 8) % 2 == 0), 9 if mode == 4 else 1, case*17
                oracle.prepare_delete(nodes, spots, bytes(records), seed, rebuild, removed)
                rows.append('\t'.join(['Setup', packed(nodes), packed(spots), records.hex(), str(seed)]))
                rows.append('\t'.join(map(str, ['Detach', f'{rebuild},{removed}',
                                                *oracle.delete_step('Detach', [rebuild, removed])])))
                cases[f'mode_{mode}'] += 1
        exports = ROOT / f'extracted/graphremove/{binary}'
        names = ['creation.c', 'functions.tsv', 'geometry/creation.c', 'geometry/functions.tsv']
        if oracle.edition != 'originals':
            names += ['lookup/creation.c', 'lookup/functions.tsv']
        editions[binary] = dict(binary_sha256=oracle.sha256, layout=oracle.edition,
                               actual_calls=dict(oracle.observed), assert_calls=oracle.assertions,
                               preparation_calls=dict(Reset=2, Create=14),
                               exports={name: digest(exports/name) for name in names})
        print(f'{binary}: 같은 위치 조회 실제 x86 128회 완료', flush=True)
    FIXTURE.write_text('\n'.join(rows)+'\n', encoding='utf-8', newline='\n')
    result = dict(schema=1, primary_target='10.78', total_calls=384, sequences=6,
                  public_calls=dict(Detach=384), cases=dict(cases), editions=editions, stubbed=[],
                  tool_sha256=digest(Path(__file__)), fixture_sha256=digest(FIXTURE),
                  dependencies={name: digest(ROOT/'tools'/name) for name in PARENTS},
                  limitations=['합성 타입/FrameCode/SHP·client 32768·정수·dead 원천·소진 전·동결 시계·비전투 null 큐',
                               '0단계 머리의 정상 표면 SID 조회; fencemark/windArcher 특수 조회와 상위 삭제/raw GameWorld는 후속',
                               '7개 슬롯/dirty 직접 비교; 풀/해시/spot/표/스택/통계는 Adler-32',
                               'CD/10.37은 같은 코드 배치; 최신 10.82 규칙을 cpppj 목표로 채택하지 않음'])
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')


def verify():
    """게임/기계어 실행 없이 입력/부모 도구/내보내기/fixture SHA와 호출 수를 확인한다."""
    r = json.loads(REPORT.read_text(encoding='utf-8'))
    assert r['tool_sha256'] == digest(Path(__file__)) and r['fixture_sha256'] == digest(FIXTURE)
    assert r['stubbed'] == [] and r['primary_target'] == '10.78'
    # 실제 PE와 준비에 쓰인 모든 부모 코드의 바이트를 확인한다.
    for name, value in r['dependencies'].items():
        assert value == digest(ROOT/'tools'/name)
    # 새 결과가 참조한 검토 몸체는 기존 내보내기 SHA를 그대로 사용한다.
    for edition, data in r['editions'].items():
        assert data['binary_sha256'] == digest(ROOT/BINARIES[edition]) and data['assert_calls'] == 0
        assert data['actual_calls']['Detach'] == 128
        # 불연속 몸체의 근거 C와 범위 표를 둘 다 확인한다.
        for name, value in data['exports'].items():
            assert value == digest(ROOT/f'extracted/graphremove/{edition}'/name)
    counts = collections.Counter(line.split('\t')[0] for line in FIXTURE.read_text(encoding='utf-8').splitlines())
    assert counts['Detach'] == r['total_calls'] == 384 and counts['Begin'] == r['sequences'] == 6
    print('같은 위치 조회 x86 SHA/호출 수 확인: 384회')


if __name__ == '__main__':
    # 기본은 격리된 함수만 실행하며 --verify는 바이트/호출 수만 읽는다.
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    verify() if parser.parse_args().verify else generate()
