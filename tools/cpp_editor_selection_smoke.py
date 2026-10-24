#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 메인 Edit·HTML 맵 목록·취소/ReadFort 예약을 검사하며 Editor 설치 완료와 구별한다."""
import argparse
import hashlib
import json
import re
import shutil
from pathlib import Path
from PIL import Image
from cpp_campaign_entry_smoke import ROOT, EXE
from cpp_bridge_ritual_runtime import execute, save
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states, verify_capture_frame, dialog_button_labels


def sha(path):
    """원본 복사 입력/실행 계획의 전체 바이트 식별자를 읽는다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def repeated_rows(path, label, tag):
    """같은 보고 지점의 반복 메뉴 행을 덮어쓰지 않고 원래 순서로 읽는다."""
    result, current = [], False
    for line in Path(path).read_text('utf-8').splitlines():  # 실제 보고 태그 사이의 원시 행만 수집한다.
        fields = line.split('\t')
        if fields[0] == 'frame':
            current = fields[2] == label
        elif current and fields[0] == tag:
            result.append(fields[1:])
    return result


def run(args):
    """정상 클릭과 현재 설정만 사용하고 원본 복구/실행 실패도 원시 그대로 보관한다."""
    directory = args.output.resolve()
    directory.mkdir(parents=True, exist_ok=False)
    rows = [(2, 'report', 'main'), (3, 'click', 'Edit'), (3, 'report', 'opened'),
            (12, 'report', 'editable'), (15, 'click', args.select or 'Cancel'), (15, 'report', 'requested'), (22, 'report', 'returned')]
    for frame, operation, label in list(rows):  # 각 판정 상태와 동일 프레임의 실제 화면을 보존한다.
        if operation == 'report':
            rows.append((frame, 'snapshot', str(directory / (label + '.bmp'))))
    rows.sort(key=lambda row: row[0])  # 같은 프레임의 입력/보고 순서는 유지한다.
    script = directory / 'steps.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in rows), encoding='utf-8', newline='\n')
    settings = 'SCREENW=1024;SCREENH=768;maxFPS=60;sleepPerLoop=10;autoDemo=0;tellTips=0;currentLanguage=english'
    if args.custom_only:
        settings += ';mayEditMissions=0'  # 원문이 제공한 공식 미션 제외 설정이며 종류2 맵의50개 상한은 유지한다.
    if args.last_edit:
        assert re.fullmatch('[A-Za-z0-9_-]+', args.last_edit)
        settings += ';lastEditFort=' + args.last_edit  # 실제 최근 이름의 별도pending/체크 표시를 검사한다.
    copied = {}
    if args.working_maps:
        maps = directory / 'maps'
        maps.mkdir()
        for name in ('TEST01', 'TEST02'):  # 보호 원문을 수정하지 않고 두 실제 Fort의 바이트만 별도 namespace로 복사한다.
            source, target = ROOT / 'originals/d' / (name + '.fort'), maps / (name + '.fort')
            shutil.copyfile(source, target)
            assert sha(source) == sha(target)
            copied[name] = dict(source=str(source), path=str(target), sha256=sha(target))
        pattern = maps.as_posix() + '/{local.1}.fort'
        settings += ';fortSpec=' + pattern + ';SOLOSpec=' + pattern  # 기존 파일 열거/PathSpec/Template 분류를 그대로 사용한다.
    command = [str(EXE), '--run', str(ROOT / 'originals'), '--window', '--no-audio', '--frames', '25', '--set', settings,
               '--ui-script', str(script), '--ui-report', str(directory / 'states.tsv')]
    profile = dict(command=command, gameplayStateInjected=False, copiedMaps=copied, customOnly=args.custom_only, lastEdit=args.last_edit,
                   selected=args.select, planSha256=sha(script), selectorOnly=True, editorInstallVerified=False, goalComplete=False)
    save(directory / 'prepared.json', profile)
    before = snapshots(ROOT / 'originals')
    save(directory / 'originals-before.json', before)
    evidence = dict(profile=profile, completed=False, passed=False)
    save(directory / 'execution.json', evidence)
    try:
        with preserve_game_settings(ROOT / 'originals', 'd', directory / 'settings-backup'):
            evidence.update(execute(command, directory, EXE, args.timeout))
    except Exception as error:
        evidence['error'] = repr(error)
        raise
    finally:
        after = snapshots(ROOT / 'originals')
        save(directory / 'originals-after.json', after)
        evidence['originalsUnchanged'] = before == after
        save(directory / 'execution.json', evidence)
    try:
        assert evidence['returnCode'] == 0 and not evidence['timedOut'] and evidence['originalsUnchanged']
        observed = states(directory / 'states.tsv')
        for label, row in observed.items():  # 실제 상태와 이미지의 입력 프레임 연결도 검사한다.
            verify_capture_frame(directory, row, label)
            Image.open(directory / (label + '.bmp')).save(directory / (label + '.png'))
        initial, opened, editable, requested = (observed[name] for name in ('main', 'opened', 'editable', 'requested'))
        assert opened['page'] == editable['page'] == ['LoadEditable'] and opened['frame'] == 3
        assert opened['fort_selection'][0] == editable['fort_selection'][0] == '2'
        assert opened['fort_selection'][1:4] == initial['fort_selection'][1:4]  # Ask는mode만 바꾸고 State를 예약하지 않는다.
        assert {'Create New Map', 'Cancel'} <= dialog_button_labels(editable)
        items = repeated_rows(directory / 'states.tsv', 'editable', 'raw_menu_item')
        assert items and all(row[3] == str(0x020000ca) and row[5] == '1' for row in items)
        assert all(not (int(row[2]) & 0x200) for row in items)
        if args.working_maps:
            assert {row[-1] for row in items} == {'TEST01', 'TEST02'} and editable['fort_catalog'][:1] == ['2']
        if args.last_edit:
            assert editable['fort_selection'][-1] == args.last_edit
            assert [row[-1] for row in items if int(row[2]) & 4] == [args.last_edit]
        if args.select:
            assert requested['fort_selection'][3:] == ['1', args.select]  # 명령이 발화한 같은 프레임의 실제next/별도pending이다.
        else:
            assert requested['fort_selection'] == editable['fort_selection']  # Cancel은미완료설치를임의완료하거나예약을지우지않는다.
        assert requested['html_window_state'] == ['0', '0', '0']
        value = dict(binarySha256=evidence['binarySha256'], originalsUnchanged=True, sameFrameVerified=True,
                     observed={label: {key: row.get(key) for key in ('frame', 'phase', 'page', 'fort_selection', 'fort_catalog', 'html_window_state')} for label, row in observed.items()},
                     menuItems=items, selectorOnly=True, editorInstallVerified=False, goalComplete=False)
    except Exception as error:
        evidence['inspectionError'] = repr(error)
        save(directory / 'execution.json', evidence)
        raise
    evidence.update(completed=True, passed=True)
    save(directory / 'execution.json', evidence)
    save(directory / 'results.json', value)
    return dict(passed=True, mapRows=len(items), selected=args.select, originalFilesUnchanged=True, editorInstallVerified=False)


def main():
    """새 출력 경로에만 실제 관측을 만들며 기본은 현재 원문 목록과 Cancel을 검사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--select', help='실제로 보이는 맵 이름을 클릭한다; 생략하면Cancel이다')
    parser.add_argument('--custom-only', action='store_true')
    parser.add_argument('--last-edit')
    parser.add_argument('--working-maps', action='store_true', help='두 보호TEST Fort를 별도 작업폴더에 그대로 복사하고 원문FileSpec으로 조회한다')
    parser.add_argument('--timeout', type=float, default=90)
    args = parser.parse_args()
    assert args.timeout > 0
    print(json.dumps(run(args), ensure_ascii=True))


if __name__ == '__main__':
    main()
