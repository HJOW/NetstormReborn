#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 원본 편집기의 고정 관측과 같은 Fort를 연 cpppj 보고의 Graph 합계·전체 surface 배치를 비교한다."""
import argparse
import collections
import csv
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]  # 현재 제품 보고와 원본 고정 관측이 있는 저장소다.
FIXTURES = ROOT / 'cpppj/tests/fixtures'  # 실행 중 얻은 관측을 수정하거나 제품에서 기대값을 만들지 않는다.
NAME = 'editor-graph-native-720'  # 원본에서 실제 생성·저장·재적재한 한 맵의 식별자다.
FLAGS_SHA256 = '277529af0c146bbde2aef7d715581f0f1d7f9398de3269b2fe66c50f33612460'  # 원본 재적재의 전체188개 TYPE 플래그 관측이다.


def bits(value):
    """원문 float32 좌표와 현재 보고의 십진 좌표를 같은 비트 폭으로 비교한다."""
    return struct.unpack('<I', struct.pack('<f', float(value)))[0]


def membership(native, objects, numbers, records, expected):
    """Graph 번호를 바꿔도 같은 표면끼리 묶였는지와 원본 장부 WORD를 함께 비교한다."""
    keys = ('type', 'state', 'owner', 'xBits', 'yBits')  # 다른 Pool의 SID와 다시 고르는 그림 frame은 소속 식별에 쓰지 않는다.
    def groups(rows, graph):
        """같은 번호의 실제 표면 다중집합을 만들고 noGraph254를 따로 보존한다."""
        result = collections.defaultdict(collections.Counter)
        for row in rows:  # 같은 위치의 중복 표면도 개수로 남기며 집합으로 줄이지 않는다.
            result[graph(row)][tuple(row[key] for key in keys)] += 1
        return result
    if len(numbers) != len(objects) or set(numbers) != {row['sid'] for row in objects}:
        raise ValueError('현재 표면의 개별 Graph 관측 누락/추가')
    native_groups = groups(native, lambda row: row['graph'])
    current_groups = groups(objects, lambda row: numbers[row['sid']])
    def components(grouped, ledger):
        """각 연결 성분의 표면과 signed 장부를 한 항목으로 정규화하며 번호만 제외한다."""
        result = collections.Counter()
        for number, members in grouped.items():  # noGraph는 정상 연결 성분과 합치지 않는다.
            if number == 254:
                continue
            if not 0 < number < 254 or number not in ledger or not ledger[number][1]:
                raise ValueError('현재/원본 표면의 Graph 번호 또는 활성 장부 오류')
            result[(tuple(sorted(members.items())), tuple(ledger[number][:2]))] += 1
        active = {number for number, row in ledger.items() if 0 < number < 254 and row[1]}
        if active != set(grouped) - {254}:
            raise ValueError('표면 없는 활성 Graph 또는 관측되지 않은 활성 장부')
        return result
    native_records = {int(number): row for number, row in expected['activeGraphRecords'].items()}
    left = components(native_groups, native_records)
    right = components(current_groups, records)
    no_graph_matches = native_groups.get(254, {}) == current_groups.get(254, {})
    return dict(observed=True, componentMembershipAndRecordsMatch=left == right,
                noGraphMembershipMatches=no_graph_matches,
                cppNoGraphSurfaces=sum(current_groups.get(254, {}).values()),
                cppGraphHistogram={str(number): sum(rows.values()) for number, rows in sorted(current_groups.items())},
                passed=left == right and no_graph_matches)


def compare(report, label, require_membership=False):
    """현재 보고의 한 상태에서 전체 surface의 타입·상태·소유자·좌표와 Graph 합계만 판정한다."""
    meta = json.loads((FIXTURES / (NAME + '.json')).read_text('utf-8'))
    for name, expected in meta['fixtureSha256'].items():  # 최초 원본 관측과 실제 저장 맵의 바이트 변화를 거부한다.
        if hashlib.sha256((FIXTURES / name).read_bytes()).hexdigest() != expected:
            raise ValueError('원본 고정 관측 SHA 불일치: ' + name)
    with (FIXTURES / (NAME + '-surfaces.tsv')).open(encoding='utf-8', newline='') as stream:
        native = [{key: int(value) for key, value in row.items()} for row in csv.DictReader(stream, delimiter='\t')]
    flags_path = FIXTURES / (NAME + '-type-flags.tsv')
    if hashlib.sha256(flags_path.read_bytes()).hexdigest() != FLAGS_SHA256:
        raise ValueError('원본 전체 TYPE 플래그 관측 SHA 불일치')
    with flags_path.open(encoding='utf-8', newline='') as stream:
        surface_types = {int(row['type']) for row in csv.DictReader(stream, delimiter='\t') if int(row['flags1']) & 0x800}  # 관측 맵에 없던 타입의 잘못된 추가 surface도 비교에 포함한다.
    objects, fields, graph = [], {}, None  # 현재 제품 보고의 실제 행이며 가상 월드를 만들지 않는다.
    numbers, records = {}, {}  # 새 읽기 전용 보고의 실제 Graph BYTE/앞 두 WORD이며 합계로 대체하지 않는다.
    selected = False
    found = 0
    for line in report.read_text('utf-8').splitlines():  # 같은 객체의 다른 프레임 보고를 합산하지 않는다.
        row = line.split('\t')
        if row[0] == 'frame':
            selected = row[2] == label
            found += selected
        elif selected and row[0] == 'raw_object' and int(row[2]) in surface_types:
            objects.append(dict(sid=int(row[1]), type=int(row[2]), owner=int(row[3]), frame=int(row[4]), xBits=bits(row[5]), yBits=bits(row[6])))
        elif selected and row[0] == 'raw_asset_fields':
            fields[int(row[1])] = int(row[2])
        elif selected and row[0] == 'raw_world':
            graph = dict(connectedSurfaces=int(row[4]), graphs=int(row[5]))
        elif selected and row[0] in ('raw_surface_graph', 'raw_graph_record'):
            target = numbers if row[0] == 'raw_surface_graph' else records
            number = int(row[1])
            if number in target:
                raise ValueError('개별 Graph 관측 중복: ' + row[0])
            target[number] = int(row[2]) if target is numbers else tuple(map(int, row[2:4]))
    if found != 1 or graph is None:
        raise ValueError('현재 보고의 명시 상태/월드가 없거나 중복됨')
    for row in objects:  # 같은 실제 SID의 상태를 붙이며 SID 값 자체는 서로 다른 Pool의 동등성 기준으로 삼지 않는다.
        row['state'] = fields[row['sid']]
    keys = ('type', 'state', 'owner', 'xBits', 'yBits')  # frame 차이는 Graph 판정과 구별해 별도로 보존한다.
    native_placement = collections.Counter(tuple(row[key] for key in keys) for row in native)
    current_placement = collections.Counter(tuple(row[key] for key in keys) for row in objects)
    native_frames = collections.Counter(tuple(row[key] for key in keys) + (row['frame'],) for row in native)
    current_frames = collections.Counter(tuple(row[key] for key in keys) + (row['frame'],) for row in objects)
    expected_graph = {key: meta['reloaded'][key] for key in ('connectedSurfaces', 'graphs')}
    missing = sum((native_placement-current_placement).values())
    extra = sum((current_placement-native_placement).values())
    result = dict(scope='같은 원본 Fort의 한 번 재적재: 전체 surface 배치와 Graph 합계. 개별 Graph 번호/할당 SID·frame·원시 Pool 바이트 동등성은 판정하지 않음.',
                  fixture=NAME, label=label, nativeSurfaces=len(native), cppSurfaces=len(objects), missingSurfaces=missing,
                  extraSurfaces=extra, surfacePlacementMatches=not (missing or extra), nativeGraph=expected_graph, cppGraph=graph,
                  graphMatches=graph == expected_graph, matchingFrames=sum((native_frames & current_frames).values()),
                  unmatchedNativeFrames=sum((native_frames-current_frames).values()),
                  nativeNoGraphSurfaces=sum(row['graph'] == 254 for row in native),
                  reportSha256=hashlib.sha256(report.read_bytes()).hexdigest())
    result['membership'] = membership(native, objects, numbers, records, meta['reloaded']) if numbers or records else dict(observed=False, passed=False)
    result['passed'] = result['surfacePlacementMatches'] and result['graphMatches'] and (result['membership']['passed'] or not require_membership)
    if result['membership']['observed']:
        result['scope'] = '같은 원본 Fort 재적재의 전체 surface 배치·개별 Graph 성분/장부·noGraph 소속. 번호 자체·frame·원시 Pool 바이트 동등성은 판정하지 않음.'
    return result


def main():
    """게임을 실행하지 않고 기존 보고와 최초 원본 관측만 대조한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--label', default='reloaded')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--require-membership', action='store_true', help='개별 Graph 관측이 없는 과거 보고를 성공으로 판정하지 않음')
    args = parser.parse_args()
    result = compare(args.report, args.label, args.require_membership)
    text = json.dumps(result, ensure_ascii=False, indent=2) + '\n'
    if args.output:
        output = args.output.resolve()
        if not output.is_relative_to((ROOT / 'extracted').resolve()):
            raise ValueError('분석 출력은 extracted 내부만 허용')
        output.write_text(text, encoding='utf-8', newline='\n')
    print(text, end='')
    if not result['passed']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
