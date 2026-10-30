#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""자료루트 내부 두 원문 작업맵의 실제 Edit→Test Battle 진입을 독립 사본에서 확인한다."""
import argparse
import shutil
from pathlib import Path

from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save, sha
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states

# 기존 실제4:3과 확장16:9 창을 순차 실행한다.
SIZES = ((1024, 768), (1280, 720))


def one(source, output, width, height):
    """입력 상태를 주입하지 않고 메인 메뉴부터 기존 사용자 맵을 열어 시험 전투를 시작한다."""
    output.mkdir(parents=True, exist_ok=False)
    data = output / 'data'
    shutil.copytree(source, data)
    maps=data/'entry-test-maps';maps.mkdir()  # 현재 편집 이관 정책이 허용하는 source 루트 내부 namespace다.
    copied={}
    for name in ('TEST01','TEST02'):  # 원문 Fort 바이트만 복사하고 실제 목록/Template/Graph 적재는 그대로 실행한다.
        original=data/'d'/(name+'.fort');target=maps/(name+'.fort');shutil.copyfile(original,target);assert sha(original)==sha(target);copied[name]=sha(target)
    before = snapshots(data)
    steps = [(3, 'click-if', 'OK'), (5, 'click-if', 'OK'), (7, 'click-if', 'Main Menu'), (10, 'click', 'Edit'),
             (18, 'click', 'TEST02'), (55, 'report', 'editor'),
             (55, 'snapshot', str(output / 'editor.bmp')), (80, 'esc', ''),
             (85, 'click', 'Game'), (90, 'click', 'Test Battle'),
             (130, 'report', 'waiting'), (140, 'click-if', 'Go!'),
             (155, 'click-if', 'Go!'), (170, 'report', 'playing'),
             (300, 'report', 'final'), (300, 'snapshot', str(output / 'final.bmp'))]
    script, report = output / 'steps.tsv', output / 'states.tsv'
    script.write_text(''.join(f'{frame}\t{op}\t{arg}\n' for frame, op, arg in steps), encoding='utf-8', newline='\n')
    command = [str(GAME), '--run', str(data), '--window', '--no-audio', '--frames', '315',
               '--set', f'SCREENW={width};SCREENH={height};maxFPS=40;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=1;mayEditMissions=0;fortSpec=entry-test-maps/{{local.1}}.fort;SOLOSpec=entry-test-maps/{{local.1}}.fort',  # 원문 FileSpec으로 두 실제 작업맵만 열거한다. 외부 절대경로·원본 목록 상한·Template/게임 상태는 변경하지 않는다.
               '--ui-script', str(script), '--ui-report', str(report)]
    with preserve_game_settings(data, 'd', output / 'settings-backup'):
        run = execute(command, output, GAME, 120)
    save(output / 'run.json', run)
    assert run['returnCode'] == 0 and not run['timedOut'], run
    observed = states(report)
    editor, playing, final = (observed[label] for label in ('editor', 'playing', 'final'))
    assert editor['raw_world_modes'] == ['1', '1'] and editor['raw_session'] == ['1', '1']
    for state in (playing, final):  # 실제 시험 전투의 공통 세션·현재 미션·마우스 활성을 확인한다.
        assert state['phase'] == ['4'] and state['paused'] == ['0']
        assert state['raw_world_modes'] == ['0', '1'] and state['raw_session'] == ['1', '1']
        assert state['mission'][0].casefold() == 'test02' and state['mouse_cursor'][1] == '1'
    delta = float(final['raw_world'][0]) - float(playing['raw_world'][0])
    assert delta > 1, delta
    after = snapshots(data)
    changes = [name for name in before if before[name] != after.get(name)]
    added = [name for name in after if name not in before]
    result = dict(size=[width, height], editorModes=editor['raw_world_modes'], battleModes=final['raw_world_modes'],
                  gameTimeDelta=delta, editorGraph=editor['raw_world'][3:5], battleGraph=final['raw_world'][3:5],
                  copiedFortSha256=copied, sourceNamespace='entry-test-maps/{local.1}.fort',
                  changedCopyFiles=changes, addedCopyFiles=added, run=run)
    save(output / 'summary.json', result)
    return result


def main():
    """원본 경로는 입력 사본으로만 읽으며 실제 쓰기는 새 출력 폴더 아래 사본에 한정한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    source, output = args.data_root.resolve(), args.output.resolve()
    assert source != output and source not in output.parents
    output.mkdir(parents=True, exist_ok=False)
    original = snapshots(ROOT / 'originals')
    source_before = snapshots(source)
    results = []
    for width, height in SIZES:  # 같은 옵션과 포커스를 공유하는 두 창을 동시에 띄우지 않는다.
        results.append(one(source, output / f'{width}x{height}', width, height))
    assert snapshots(source) == source_before and snapshots(ROOT / 'originals') == original
    save(output / 'summary.json', dict(results=results, sourceUnchanged=True, originalsUnchanged=True,
                                      gameSha256=sha(GAME), naturalCompletionVerified=False))
    print('실제 Edit→Test Battle 두 화면비 진입·비정지 시간 진행 확인')


if __name__ == '__main__':
    main()
