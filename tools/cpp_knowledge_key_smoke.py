#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""단일 짧은 캠페인 실행에서 F6의 정지/수정키와 실제 지식 카드 표시를 확인한다."""
import argparse
import json
from pathlib import Path
import re
from cpp_bridge_ritual_runtime import save
from cpp_game_menu_smoke import protected_run
from cpp_solo_entry_smoke import BUTTONS
from cpp_world_smoke import states


def cards(path):
    """실제 보고의 활성 지식 카드만 세며 새 메뉴나 입력을 만들지 않는다."""
    result = {}
    # 각 보고 시점에서 살아 있는 팝업의 종류/플래그/부모를 그대로 판정한다.
    for block in re.split(r'(?m)^frame\t', path.read_text(encoding='utf-8'))[1:]:
        header, body = block.split('\n', 1)
        _, label = header.split('\t', 1)
        found = []
        # 지식 작성기가 붙인 bit0x1000000과 같은 현재 활성/반납 비트만 읽는다.
        for line in body.splitlines():
            if not line.startswith('raw_menu\t'):
                continue
            fields = line.split('\t')
            if int(fields[3]) and int(fields[4]) & 1 and not int(fields[4]) & 0x10000 and int(fields[5]) & 0x1000000:
                found.append(int(fields[1]))
        result[label] = found
    return result


def main():
    """실제 표시된 시작 안내만 닫고 공격/생산/시나리오 승리 없이 키 경계를 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    steps = [(230, 'report', 'initial'), (235, 'key', 'shift+f9'), (245, 'report', 'paused'),
             (250, 'key', 'f6'), (260, 'report', 'pausedF6'), (265, 'keyup', 'f6'), (270, 'report', 'pausedRelease'),
             (275, 'key', 'shift+f9'), (285, 'report', 'resumed'), (290, 'key', 'ctrl+f6'), (300, 'report', 'ctrlBlocked'),
             (305, 'key', 'shift+f6'), (315, 'report', 'shiftBlocked'), (320, 'keyup', 'f6'), (330, 'report', 'releaseBlocked'),
             (335, 'key', 'f6'), (345, 'report', 'normalOpen'), (350, 'esc', ''), (360, 'report', 'closed'),
             (365, 'key', 'alt+f6'), (375, 'report', 'altOpen'), (380, 'snapshot-presented', str(out/'knowledge.bmp')),
             (390, 'esc', ''), (400, 'report', 'final')]
    # 로딩 중 고정 클릭을 피하고 현재 안내에 실제로 있는 버튼만 클릭한다.
    for frame in range(12, 220, 8):
        # 다른 프레임/가려진 안내의 문자열을 입력 대상으로 재사용하지 않는다.
        for label in BUTTONS:
            condition = r'phase\t[13]\n&&(?:control\t'+re.escape(label)+r'\t|raw_html_button[^\n]*\t'+re.escape(label)+r'\n)'
            steps.append((frame, 'when', condition+';;click;;'+label))
    steps.sort(key=lambda row: row[0])
    report, run = protected_run(out/'run', 1024, 768, steps, 410, mission='thewarbegins')
    all_observed, visible = states(report), cards(report)
    observed = {label: all_observed[label] for _, operation, label in steps if operation == 'report'}
    # 모든 관측은 같은 실제 Mission/World에서만 비교한다.
    for label, scene in observed.items():
        assert scene['phase'] == ['4'], (label, scene['phase'])
    assert observed['paused']['paused'] == observed['pausedF6']['paused'] == observed['pausedRelease']['paused'] == ['1']
    assert observed['paused']['time'][0] == observed['pausedF6']['time'][0] == observed['pausedRelease']['time'][0]
    # 정지/수정키/뗌과 ESC 뒤에는 살아 있는 지식 카드가 없어야 한다.
    for label in ('initial', 'paused', 'pausedF6', 'pausedRelease', 'resumed', 'ctrlBlocked', 'shiftBlocked', 'releaseBlocked', 'closed', 'final'):
        assert not visible[label], (label, visible[label])
    assert observed['resumed']['paused'] == observed['normalOpen']['paused'] == observed['altOpen']['paused'] == ['0']
    assert visible['normalOpen'] and len(visible['normalOpen']) == len(visible['altOpen'])
    summary = dict(unit=741, passed=True, processes=1, exeSha256=run['binarySha256'], returnCode=0,
                   originalsUnchanged=run['originalsUnchanged'], dataUnchanged=run['dataUnchanged'],
                   observations={label:dict(paused=scene['paused'], time=scene['time'][0], knowledgeCards=visible[label]) for label, scene in observed.items()},
                   scope='앱 내 입력/단일 짧은 캠페인 UI이며 물리 키/전체 픽셀·PCM/자연 완주/포획·희생/특정 적 공격/승리/공중3종/MP 검사가 아니다.')
    save(out/'summary.json', summary)
    print(json.dumps({key:value for key,value in summary.items() if key!='observations'}, ensure_ascii=False))


if __name__ == '__main__':
    main()
