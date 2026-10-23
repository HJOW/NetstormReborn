#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Fury War의 실제 네 원소 선택을 보호 PE/.type/스크립트의 독립 기술 기대와 대조한다."""
import argparse
from collections import Counter
import hashlib
from pathlib import Path
import re
import struct

import pefile

from cpp_assets_smoke import original_order
from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save, sha
from cpp_fort_smoke import script_value
from cpp_menu_smoke import source
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from taff import TaffArchive
from typefile import get_prop, parse_type

MISSION = 'bc8fury76'  # 읽기 전용 훈련 원문에서 확인하는 미션 이름이다.
THEMES = (('Wind', 'W', 0), ('Rain', 'R', 1), ('Thunder', 'T', 2), ('Sun', 'S', 3))  # 버튼/명령 인자를 원문과 재검사한다.
FIRST_ASSET = 70  # 원본 0049c3b0의 자산 TYPE 시작 번호이며 내장0..69는 기본 theme5라 이번 필터에서 제외된다.
GROUP_TABLE = 0x00542468  # 보호 10.78 PE의 열 개 group 이름 포인터 배열이다.
FRAMES = 134  # 브리핑·@1·일곱 프레임 재개 뒤 두 시점을 관측하는 실제 루프 길이다.
FIELDS = frozenset(('phase', 'page', 'paused', 'time', 'mission', 'mission_start', 'mission_gate',
                    'raw_session', 'raw_technology', 'raw_brains', 'raw_production_parent'))  # 전체 월드를 반복 파싱하지 않는 관측 필드다.


def digest(payload):
    """보호 원문 한 파일의 실제 바이트를 독립 기대 근거로 식별한다."""
    return hashlib.sha256(payload).hexdigest()


def theme_number(value):
    """원본 00472e60의 초기화와 첫 BYTE 조회를 적용하며 소문자 정규화로 의미를 바꾸지 않는다."""
    if value is None:
        return 5
    first = str(value).encode('cp1252')[0] if str(value) else 0  # .type 문자열 첫 BYTE만 원본 표를 조회한다.
    if first >= 122:
        raise ValueError('원본 theme BYTE 표 범위 초과')
    answer = 5 if first < 90 else 0  # 첫90 DWORD는5이며 나머지 지정 전 BSS는0이다.
    for index, letter in enumerate(b'wrtsWRTS.?'):  # PATCH의 다섯 원소/열 지정은 대문자와 소문자에서 다른 값을 낼 수 있다.
        if first == letter:
            answer = index % 5
    return answer


def original_expectations():
    """클론 보고서·소스·덤프를 읽지 않고 보호 PE와 느슨한 파일 우선 원문에서 기대를 만든다."""
    root = ROOT / 'originals'  # 기존 원본은 읽기만 하고 어떤 검사 파일도 이 경로에 만들지 않는다.
    archive = TaffArchive(root / 'netstorm.tarc')  # 독립 Python VFS가 사용하는 원본 아카이브다.
    pe = pefile.PE(str(root / 'Netstorm.exe'))  # group 문자열 표도 복원 C++ 상수를 재사용하지 않는다.
    image, base = pe.get_memory_mapped_image(), pe.OPTIONAL_HEADER.ImageBase  # 보호 PE의 실제 가상 주소 기준이다.
    groups = []  # 배열 순서가 원본 group 숫자와 동일하다.
    for index in range(10):  # 원본 배열의 열 포인터만 읽으며 NO_GROUP은 별도10이다.
        pointer, = struct.unpack_from('<I', image, GROUP_TABLE - base + index * 4)
        groups.append(image[pointer - base:pointer - base + 80].split(b'\0', 1)[0].decode('ascii'))
    pe.close()
    order = original_order('originals')  # 보호 00540dd0에서116개 이름을 직접 읽는 기존 독립 PE 판독기다.
    rows = []  # 각 원본 TYPE의 실제 필터 입력 및 원문 SHA를 보존한다.
    for index, name in enumerate(order):  # 모든 자산을 원본 번호 순서로 판독한다.
        payload = source(root, archive, f'd/{name}.type')
        definition = parse_type(payload.decode('cp1252'))
        group_name = get_prop(definition, 'group')
        matches = [number for number, group in enumerate(groups) if str(group_name).casefold() == group.casefold()]  # 원본 stricmp와 같은 이름 비교다.
        rows.append(dict(type=FIRST_ASSET + index, name=name, theme=theme_number(get_prop(definition, 'theme')),
                         group=matches[0] if matches else 10, groupName=group_name, sourceSha256=digest(payload)))
    payload = source(root, archive, f'd/{MISSION}.english')  # 선택 본문이 바뀌면 기대도 새 원문을 사용한다.
    script = '\n'.join(payload.decode('cp1252').splitlines())  # CRCRLF도 원문 행 구분자로 처리한다.
    sections = {}  # 실제 섹션 이름과 본문이며 런타임 page 출력에서 역산하지 않는다.
    for match in re.finditer(r'(?m)^\[([^\]]+)\]([^\[]*)', script):  # 이번 명령 전용 W/R/T/S와 A./@1의 원문을 분리한다.
        sections[match[1].casefold()] = match[2]
    initial_names = [name for name in (script_value(script, 'mytech') or '').split(';') if name]  # Fury 원문의 명시 시작 지식 목록이다.
    by_name = {row['name'].casefold(): row for row in rows}  # 원본 파일명 대소문자만 정규화한 독립 TYPE 검색이다.
    initial = [by_name[name.casefold()]['name'] for name in initial_names]
    if len(initial) != len(set(initial)):
        raise ValueError('Fury 시작 기술 중복으로 독립 기대 재검토 필요')
    if not re.search(r'(?im)^\$Button=Lets do this!,DoNothing,0\s*$', sections.get('a.', '')):
        raise ValueError('Fury 시작 버튼 원문 변경')
    branches = {}  # 각 테마의 정확한 지급/미지급·초기 중복·최종 기대를 저장한다.
    for label, section, number in THEMES:  # 세 부분 모두 보호 미션에서 직접 확인한 뒤 실행 계획에 사용한다.
        if not re.search(r'(?im)^\$Button=\s*' + re.escape(label) + ',tell,' + section + r'\s*$', sections.get('@1', '')):
            raise ValueError('Fury 선택 버튼 원문 변경: ' + label)
        if sections.get(section.casefold(), '').strip() != f'<$CheatAllTechnology,{number}>':
            raise ValueError('Fury 선택 명령 전용 본문 변경: ' + section)
        included = [row for row in rows if row['group'] != 10 and row['theme'] != 5 and row['theme'] == number]
        excluded = [row for row in rows if row['theme'] == number and row['group'] == 10]
        expected = [row['name'] for row in rows if row['name'] in initial or row in included]  # 기존 기술을 지우지 않고 TYPE 순서로 합친다.
        branches[label] = dict(theme=number, section=section, included=included, excludedSameTheme=excluded,
                               existingMatches=[row['name'] for row in included if row['name'] in initial],
                               newlyGranted=[row['name'] for row in included if row['name'] not in initial], expected=expected)
    return dict(basis='보호 10.78 PE의 TYPE/group 이름 표 + VFS 원문 .type + bc8fury76.english; 클론 출력 미사용',
                originalExeSha256=sha(root / 'Netstorm.exe'), archiveSha256=sha(root / 'netstorm.tarc'),
                scriptSha256=digest(payload), groups=groups, allAssetTypes=rows, initialTechnology=initial,
                command=dict(name='CheatAllTechnology', id='02000028', body='00492ae0',
                             filter='group != 10 && theme != 5 && theme == argument',
                             creation='FindContained(currentParent,4,type,0)가0이면 부모 재조회 뒤 Create(parent,type,0,4,0x10,0)',
                             repeated='현재 부모에 같은 mask4 기술이 있으면 생성하지 않는다'), branches=branches)


def make_plan(directory, label):
    """각 버튼을 한 번만 눌러 전후를 관측하며 when의 전체 Report 반복 직렬화를 사용하지 않는다."""
    steps = [(12, 'require', r'page\tA\.\n'), (12, 'snapshot', str(directory / 'briefing.bmp')),
             (13, 'click', 'Lets do this!'), (23, 'report', 'afterBriefing'),
             (88, 'require', r'page\t@1\n'), (88, 'snapshot', str(directory / 'choice.bmp')),
             (89, 'click', label), (90, 'report', 'afterChoice'),
             (108, 'report', 'resumed'), (108, 'snapshot', str(directory / 'resumed.bmp')),
             (133, 'report', 'final'), (133, 'snapshot', str(directory / 'final.bmp'))]  # 고정 루프30ms로 자연 @1 시한과7프레임 재개를 지난다. phase는현재Tell부모의 생성경로와구별한다.
    plan = directory / 'steps.tsv'  # 실패 후에도 분기당 같은 단회 입력을 재현할 수 있는 원문이다.
    plan.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8', newline='\n')
    return plan


def read_reports(path):
    """큰 월드는 행 수만 세며 실제 기술 내용물 반복 행과 각 판정 필드를 잃지 않고 읽는다."""
    result = {}  # 라벨별 실제 관측이며 독립 기대와 별도 파일에 저장한다.
    current = None  # 아직 첫 frame에 도달하지 않았으면 기록 대상이 없다.
    with path.open(encoding='utf-8') as stream:
        for line in stream:  # 전체 파일 재파싱/수만 객체 사전 생성을 피하고 한 번만 순회한다.
            fields = line.rstrip('\r\n').split('\t')
            kind = fields[0]
            if kind == 'frame':
                current = dict(frame=int(fields[1]), controls={}, technologyContents=[], rawObjectCount=0, loadingPresent=False)
                result[fields[2]] = current
            elif current is not None:
                if kind in FIELDS:
                    current[kind] = fields[1:]
                elif kind == 'control':
                    current['controls'][fields[1]] = fields[2:]
                elif kind == 'raw_production_content' and int(fields[3]) & 4:
                    current['technologyContents'].append(dict(sid=int(fields[1]), type=int(fields[2]), flags=int(fields[3]),
                                                              quality=int(fields[4]), slot=int(fields[5])))
                elif kind == 'raw_object':
                    current['rawObjectCount'] += 1
                elif kind == 'raw_mission_loading':
                    current['loadingPresent'] = True
    return result


def run_branch(directory, label, expected, prepare_only):
    """실제 GUI 한 창의 종료·화면·기술 체인을 독립 기대와 대조하고 실패도 그대로 보존한다."""
    directory.mkdir(parents=True, exist_ok=False)
    plan = make_plan(directory, label)  # 게임 상태/시계/완료/기술을 직접 쓰는 입력은 전혀 넣지 않는다.
    command = [str(GAME), '--run', str(ROOT / 'originals'), '--mission', MISSION, '--window', '--no-audio',
               '--frames', str(FRAMES), '--set', 'SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=30',
               '--ui-script', str(plan), '--ui-report', str(directory / 'states.tsv')]
    record = dict(theme=label, selectedTheme=expected['branches'][label]['theme'], command=command,
                  planSha256=sha(plan), selectionClickCount=1, briefingClickCount=1,
                  repeatedSelectionTested=False, testStateInjection=False, preparedOnly=prepare_only, passed=False)
    save(directory / 'result.json', record)
    if prepare_only:
        return record
    result = execute(command, directory, GAME, 120)  # 실제 창/렌더링을 포함한 상한이며 상한 종료는 성공 처리하지 않는다.
    record['result'] = result
    save(directory / 'result.json', record)
    observed = read_reports(directory / 'states.tsv') if (directory / 'states.tsv').exists() else {}
    save(directory / 'observations.json', observed)  # 제품 보고를 원본 기대와 같은 이름으로 덮어쓰지 않는다.
    before, choice = observed.get('require12', {}), observed.get('require88', {})
    resumed, final = observed.get('resumed', {}), observed.get('final', {})
    branch = expected['branches'][label]
    type_by_name = {row['name']: row['type'] for row in expected['allAssetTypes']}  # 보호 PE 이름을 사용하는 독립 번호 표다.
    expected_types = Counter(type_by_name[name] for name in branch['expected'])  # 모든 기술은 실제 내용물 한 개씩이어야 한다.
    before_contents = {row['type']: row for row in before.get('technologyContents', [])}
    final_contents = {row['type']: row for row in final.get('technologyContents', [])}
    checks = dict(normalExit=result['returnCode'] == 0 and not result['timedOut'],
                  briefingObserved=before.get('page') == ['A.'] and before.get('paused') == ['1'] and 'Lets do this!' in before.get('controls', {}),
                  choiceObserved=choice.get('page') == ['@1'] and choice.get('paused') == ['1'] and set(choice.get('controls', {})) == {name for name, _, _ in THEMES} and
                                 all(row[0] == '1' for row in choice.get('controls', {}).values()),
                  initialTechnology=sorted(before.get('raw_technology', [])) == sorted(expected['initialTechnology']),
                  choiceKeepsInitial=sorted(choice.get('raw_technology', [])) == sorted(expected['initialTechnology']),
                  playing=final.get('phase') == ['4'] and final.get('page') == ['mission'] and final.get('paused') == ['0'],
                  guide=final.get('mission_start', [])[:2] == ['1', '4'],
                  gateReleased=len(final.get('mission_gate', [])) >= 5 and final['mission_gate'][2:5] == ['0', '0', '0'],
                  correctMission=final.get('mission', [''])[0].casefold() == MISSION.casefold(),
                  world=final.get('raw_session') == ['1', '1'] and final.get('rawObjectCount', 0) > 0,
                  loadingReleased=bool(final) and not final.get('loadingPresent', True),
                  exactTechnology=final.get('raw_technology') == branch['expected'],
                  exactContentMultiplicity=Counter(row['type'] for row in final.get('technologyContents', [])) == expected_types,
                  existingContentRetained=all(type_by_name[name] in before_contents and type_by_name[name] in final_contents and
                                             before_contents[type_by_name[name]]['sid'] == final_contents[type_by_name[name]]['sid']
                                             for name in expected['initialTechnology']),
                  newlyGrantedFlags=all(type_by_name[name] in final_contents and final_contents[type_by_name[name]]['flags'] == 4 and
                                        final_contents[type_by_name[name]]['quality'] == 0 for name in branch['newlyGranted']),
                  stableTechnology=resumed.get('raw_technology') == final.get('raw_technology') and
                                   {row['type']: row['sid'] for row in resumed.get('technologyContents', [])} ==
                                   {row['type']: row['sid'] for row in final.get('technologyContents', [])},
                  naturalTimeAdvanced=resumed.get('paused') == ['0'] and len(final.get('time', [])) == 2 and
                                      len(resumed.get('time', [])) == 2 and float(final['time'][0]) > float(resumed['time'][0]),
                  choicesGone=not ({name for name, _, _ in THEMES} & set(final.get('controls', {}))))
    record.update(checks=checks, observations=observed,
                  screenshots={path.name: sha(path) for path in directory.glob('*.bmp')},
                  expectedTechnology=branch['expected'], expectedNewTechnology=branch['newlyGranted'],
                  passed=all(checks.values()))
    save(directory / 'result.json', record)
    return record


def main():
    """원본 전체 해시와 허용 설정 복구를 남기며 각 테마를 독립된 새 프로세스로 순차 관측한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--theme', choices=tuple(label for label, _, _ in THEMES), action='append', help='생략하면 네 분기 모두 실행한다')
    parser.add_argument('--prepare-only', action='store_true', help='보호 원문 기대와 입력 계획만 만들고 GUI를 실행하지 않는다')
    args = parser.parse_args()
    output = args.output.resolve()  # 기존 실패 기록을 덮어쓰지 않는 새 산출물 경로다.
    output.mkdir(parents=True, exist_ok=False)
    expected = original_expectations()  # 실행 전에 독립 기대를 완성하고 별도 파일에 보존한다.
    save(output / 'original-expectations.json', expected)
    labels = args.theme or [label for label, _, _ in THEMES]  # 지정한 테마도 한 번만 실행하여 의도치 않은 재선택을 막는다.
    if len(labels) != len(set(labels)):
        parser.error('동일 --theme를 중복 지정할 수 없습니다')
    before = snapshots(ROOT / 'originals')  # 허용 설정을 포함해 모든 파일의 실행 전 실제 SHA를 저장한다.
    save(output / 'originals-before.json', before)
    records = []  # 예외가 나도 다른 독립 분기를 지우거나 실패를 숨기지 않는다.
    try:
        with preserve_game_settings(ROOT / 'originals', 'd', output / 'settings-backup'):
            for label in labels:  # 같은 설정을 쓰는 실제 창은 항상 순차 실행한다.
                try:
                    record = run_branch(output / label.lower(), label, expected, args.prepare_only)
                except Exception as error:
                    record = dict(theme=label, passed=False, error=repr(error))
                    save(output / label.lower() / 'failure.json', record)
                records.append(record)
                save(output / 'results.json', dict(runs=records, complete=False, preparedOnly=args.prepare_only))
                print(label, 'PREPARED' if args.prepare_only and 'error' not in record else 'PASS' if record['passed'] else 'FAIL',
                      record.get('checks', record.get('error', 'GUI 미실행')), flush=True)
    finally:
        after = snapshots(ROOT / 'originals')  # 예외/상한 뒤에도 설정 복구 후 전체 파일을 다시 해시한다.
        save(output / 'originals-after.json', after)
        summary = dict(runs=records, complete=len(records) == len(labels), preparedOnly=args.prepare_only,
                       originalsUnchanged=before == after, selectedThemes=labels,
                       originalExpectationsSha256=sha(output / 'original-expectations.json'),
                       scope='각 Fury 초기 브리핑과 @1의 단회 실제 선택/Guide4/비정지 플레이 진입/원본 기술 체인 대조; 재선택·희생·완주·전체 동일성 검사는 별도')
        save(output / 'results.json', summary)
    if before != after or any('error' in record or (not args.prepare_only and not record['passed']) for record in records):
        raise SystemExit(1)


if __name__ == '__main__':
    main()
