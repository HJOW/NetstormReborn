#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 선택 로그와 개발자 F7의 실제 파일·뗌·정지·메뉴 수명을 격리 사본에서 검사한다."""
import argparse
import json
import re
import shutil
import subprocess
import time
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT, GAME, save, sha
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from cpp_renderer_smoke import snapshots


def fingerprint(path):
    """출력 바이트와 실제 수정 시각을 함께 읽어 뗌의 재쓰기 여부를 구별한다."""
    if not path.exists():
        return None
    return dict(size=path.stat().st_size, modifiedNs=path.stat().st_mtime_ns, sha256=sha(path))


def one(directory, source, executable, width, height, level, fullscreen, editor):
    """같은 제품의 일반/편집 입력을 실행하고 살아 있는 보고 단계에서 파일을 관측한다."""
    directory.mkdir(parents=True, exist_ok=False)
    data = directory / 'data'
    map_alias = None
    if editor:
        (data / 'd').mkdir(parents=True)
        alias_files = {}
        for extension in ('fort', 'english'):  # 원문은 정렬 전에50개를 받으므로 같은 TEST01 사본을 실제 디렉터리의 첫 항목으로 생성한다.
            original, alias = source / 'd' / f'TEST01.{extension}', data / 'd' / f'AAA560.{extension}'
            shutil.copyfile(original, alias)
            assert sha(original) == sha(alias)
            alias_files[extension] = sha(alias)
        map_alias = dict(source='TEST01', alias='AAA560', fileSha256=alias_files,
                         reason='원문 첫50개 목록 상한을 유지하고 같은 TEST01 바이트를 기본 Edit 목록에서 선택하기 위한 검사 사본 이름')
        shutil.copytree(source, data, dirs_exist_ok=True)
    else:
        shutil.copytree(source, data)
    if (data / 'squids.txt').exists():
        raise RuntimeError('입력 사본의 기존 squids.txt를 검사 출력으로 덮어쓸 수 없음')
    if editor:
        steps = [(3, 'click-if', 'OK'), (5, 'click-if', 'OK'), (7, 'report', 'initial'), (10, 'click', 'Edit'), (18, 'click', 'AAA560'),
                 (55, 'report', 'editor'), (60, 'key', 'ctrl+shift+f7'), (70, 'report', 'dump'),
                 (100, 'keyup', 'ctrl+shift+f7'), (110, 'report', 'released'),
                 (140, 'key', 'alt+ctrl+shift+f7'), (150, 'report', 'alt')]
        frames = 180
    else:
        steps = [(5, 'report', 'brief'), (10, 'click', 'Play Mission'), (20, 'report', 'playing'),
                 (25, 'click', 'world:rainVortex'), (30, 'report', 'selected'), (35, 'outside', ''),
                 (40, 'key', 'shift+f9'), (45, 'report', 'paused'),
                 (50, 'key', 'ctrl+shift+f7'), (60, 'report', 'dump'),
                 (90, 'keyup', 'ctrl+shift+f7'), (100, 'report', 'released'),
                 (130, 'key', 'alt+ctrl+shift+f7'), (140, 'report', 'alt'),
                 (160, 'key', 'shift+f9'), (170, 'report', 'resumed'), (180, 'esc', ''),
                 (185, 'click', 'Game'), (190, 'click', 'Leave Mission'), (200, 'click', 'Main Menu'),
                 (210, 'report', 'main'), (220, 'key', 'ctrl+shift+f7'), (230, 'report', 'mainDump')]
        frames = 250
    steps = [(frame + (280 if frame >= (140 if editor else 130) else 0), operation, argument)
             for frame, operation, argument in steps]  # H: 파일시각의2초 해상도에서도 새 누름 재쓰기를 구별하도록 간격을5초 이상 둔다.
    frames += 280
    script, report = directory / 'steps.tsv', directory / 'states.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8')
    command = [str(executable), '--run', str(data), '--no-audio', '--frames', str(frames), '--set',
               f'SCREENW={width};SCREENH={height};maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=16;tracelevel={level};startInFullScreen={int(fullscreen)}',
               '--ui-script', str(script), '--ui-report', str(report)]
    if not fullscreen:
        command.append('--window')
    if not editor:
        command += ['--mission', 'thewarbegins']
    baseline = snapshots(data)
    files, start = {}, time.monotonic()
    with preserve_game_settings(data, 'd', directory / 'settings-backup'), (directory / 'stdout.log').open('wb') as stdout, (directory / 'stderr.log').open('wb') as stderr:
        process = subprocess.Popen(command, cwd=directory, stdout=stdout, stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
        save(directory / 'running.json', dict(pid=process.pid, command=command, binarySha256=sha(executable)))
        print(json.dumps(dict(pid=process.pid, directory=str(directory)), ensure_ascii=False), flush=True)
        try:
            while process.poll() is None:  # 실행 중 마지막 실제 보고 단계에서 두 파일의 현재 시각/바이트만 읽는다.
                if time.monotonic() - start > 180:
                    raise TimeoutError('진단 UI 검사 180초 초과')
                if report.exists():
                    labels = re.findall(rb'^frame\t\d+\t([^\r\n]+)', report.read_bytes(), re.M)
                    if labels:
                        label = labels[-1].decode('utf-8')
                        if label in ('dump', 'released', 'alt', 'mainDump') and label not in files:
                            files[label] = dict(squids=fingerprint(data / 'squids.txt'), processes=fingerprint(directory / 'procSumm.txt'))
                            for kind, path in [('squids', data / 'squids.txt'), ('processes', directory / 'procSumm.txt')]:  # 다음 덤프가 같은 파일을 덮기 전에 실제 바이트를 별도 보존한다.
                                if path.exists():
                                    (directory / f'{label}-{kind}.txt').write_bytes(path.read_bytes())
                time.sleep(0.02)
        finally:
            if process.poll() is None:
                process.terminate()
            code = process.wait(timeout=20)
    result = dict(returnCode=code, seconds=time.monotonic()-start, command=command, binarySha256=sha(executable), files=files, testOnlyMapAlias=map_alias)
    save(directory / 'run.json', result)
    assert code == 0, (code, (directory / 'stderr.log').read_bytes()[-1200:])
    observed = states(report)
    for label in ('dump', 'released', 'alt'):  # 실제 Client의 같은 모드/깊이/들여쓰기가 호출 뒤 복구됐는지 읽는다.
        assert observed[label]['raw_trace'][:4] == [str(level), '0', '0', '0'], observed[label]['raw_trace']
        assert files[label]['squids'] is not None and files[label]['processes'] is not None
    assert files['dump'] == files['released'], files
    assert files['alt']['squids']['modifiedNs'] > files['released']['squids']['modifiedNs']
    assert files['alt']['processes']['modifiedNs'] > files['released']['processes']['modifiedNs']
    output = (directory / 'stdout.log').read_bytes()
    if not editor:
        sid = int(observed['selected']['selected'][0])
        assert sid and observed['paused']['paused'] == observed['dump']['paused'] == ['1']
        assert observed['resumed']['paused'] == ['0']
        selected = f'~1~y{sid:04}~n Base type='.encode('ascii')
        assert (selected in output) == bool(level >= 29)
        assert observed['mainDump']['raw_trace'][:4] == [str(level), '0', '0', '0']
        assert files['mainDump']['processes']['size'] == 0
    else:
        assert observed['editor']['editor_runtime'][0] == '1'
    for label, pair in files.items():  # 후처리된 파일에는 원문 마크업이 남지 않고 실제 프로세스 행이 존재해야 한다.
        if label != 'mainDump':
            assert pair['squids']['size'] > 0 and pair['processes']['size'] > 0
            squids = (directory / f'{label}-squids.txt').read_bytes()
            processes = (directory / f'{label}-processes.txt').read_bytes().splitlines()
            assert b'~' not in squids and b'Base type=' in squids
            assert len(processes) == int(observed[label]['shared_kernel'][1])
            assert all(row.startswith(b'pType=') and b' sid=' in row and b', attachToType=' in row for row in processes)
    final = snapshots(data)
    assert all(final.get(name) == digest for name, digest in baseline.items())
    assert set(final)-set(baseline) == {'squids.txt'}
    result.update(states=len(observed), level=level, editor=editor, fullscreen=fullscreen, dataAssetsUnchanged=True)
    save(directory / 'verification.json', result)
    return result


def main():
    """창4:3·한글 경로 전체화면16:9·편집 모드를 순서대로 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--exe', type=Path, default=GAME)
    parser.add_argument('--case', choices=('campaign-4x3', '한글-16x9', 'editor'), help='이미 통과한 같은 제품 사례를 반복하지 않고 선택 사례만 실행한다')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    before = snapshots(ROOT / 'originals')
    results = []
    try:
        for name, width, height, level, fullscreen, editor in [('campaign-4x3', 1024, 768, 29, False, False), ('한글-16x9', 1280, 720, 0, True, False), ('editor', 1024, 768, 29, False, True)]:  # 같은 입력과 실제 화면/필터/모드를 비교한다.
            if args.case and name != args.case:
                continue
            results.append(one(output / name, args.root.resolve(), args.exe.resolve(), width, height, level, fullscreen, editor))
    finally:
        assert snapshots(ROOT / 'originals') == before
    save(output / 'results.json', results)
    print(json.dumps([dict(returnCode=r['returnCode'], states=r['states'], level=r['level'], editor=r['editor'], fullscreen=r['fullscreen']) for r in results], ensure_ascii=False))


if __name__ == '__main__':
    main()
