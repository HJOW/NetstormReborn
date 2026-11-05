#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""정상 종료한 데모 원시 관측에서 세 세대의 원문15초 안내/안내 부재를 대조한다."""
import argparse
import json
import re
from pathlib import Path

from cpp_bridge_ritual_runtime import ROOT, save
from cpp_demo_repeat_smoke import observe, reference
from cpp_demo_start_smoke import DEMOS
from cpp_menu_smoke import source
from taff import TaffArchive


def audit(directory, mission, result_name='result.json'):
    """종료/본 검사 통과 뒤만 원시를 읽어 동시 재작성 간섭 없이 안내 지속 시간을 확인한다."""
    assert Path(result_name).name == result_name, '같은 실행 폴더의 결과 파일명만 허용'
    record = json.loads((directory / result_name).read_text(encoding='utf-8'))
    assert record['returnCode'] == 0 and record['passed'] and not record['timedOut'], '본 검사 정상 종료 뒤 실행 필요'
    expected = reference(mission)
    text = source(ROOT / 'originals', TaffArchive(ROOT / 'originals/netstorm.tarc'), 'd/' + mission + '.english').decode('cp1252')
    section = re.search(r'\[Demo\](.*?)(?:\[END\]|\Z)', text, re.S)
    timeout = re.search(r'\$Timeout\s*=\s*(\d+)\s*,\s*DoNothing\s*,\s*0', section[1]) if section else None
    assert bool(section) == expected['demoSection']
    if section:
        assert timeout and int(timeout[1]) == 15, '원문 안내15초 계약 변경'
    groups = {}  # 종료된 보고의 Start/기한별 세대 순서를 유지한다.
    for row in observe(directory / 'states.tsv'):  # 중간 기한0의 예약은 실제 Start 세대로 만들지 않는다.
        gate = row.get('mission_start', [])
        if gate[:3] == ['1', '4', '1'] and float(gate[4]):
            groups.setdefault(tuple(gate[3:5]), []).append(row)
    assert len(groups) == 3, '세 세대 필요'
    result = []  # 안내를 처음 본 시각과 비정지 재개를 처음 본 시각 사이의 관측 범위다.
    for identity, rows in groups.items():  # 각 세대의 안내를 서로 다른 반복의 재개와 섞지 않는다.
        guides = [row for row in rows if row.get('page') == ['Demo'] and row.get('paused') == ['1']]
        playing = [row for row in rows if row.get('phase') == ['4'] and row.get('paused') == ['0']]
        if section:
            assert guides and playing
            first = float(guides[0]['time'][1])
            last = float(guides[-1]['time'][1])
            resumed = next(float(row['time'][1]) for row in playing if float(row['time'][1]) > last)
            # 실제 보고 간격 약2초를 고려하되15초 안내를 즉시 닫은 실행은 거부한다.
            assert 12 < resumed-first < 20, (identity, first, last, resumed)
            result.append(dict(start=identity, guideFirstWall=first, guideLastWall=last, resumedWall=resumed, observedDuration=resumed-first))
        else:
            assert not guides, '원문에 없는 Demo 안내'
            result.append(dict(start=identity, guideAbsent=True))
    return dict(passed=True, reference=expected, timeoutSeconds=int(timeout[1]) if timeout else None, generations=result)


def main():
    """완성된 실행 폴더만 입력받고 최초 보조 판정 파일을 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--mission', choices=[item[0] for item in DEMOS], required=True)
    parser.add_argument('--result', default='result.json', help='보존한 최초 결과 또는 별도 재판정 결과 파일명')
    args = parser.parse_args()
    directory = args.directory.resolve()
    assert all(directory != (ROOT / name).resolve() and (ROOT / name).resolve() not in directory.parents for name in ('originals', 'originalCD'))
    output = directory / 'guide-audit.json'
    assert not output.exists(), '최초 안내 판정 보존'
    result = audit(directory, args.mission, args.result)
    save(output, result)
    print(json.dumps(result, ensure_ascii=False))


if __name__ == '__main__':
    main()
