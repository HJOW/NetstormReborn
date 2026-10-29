#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Early5의 실제 입력 재생과 Devastation 관측을 원본 변경 없이 보존한다."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states

ROOT = Path(__file__).resolve().parents[1]  # 원본과 검사 출력을 구분하는 저장소 기준이다.
EXE = ROOT / 'cpppj/build/bin/Release/Netstorm.exe'  # 복원한 클론만 실행한다.
REPORT = ROOT / 'cpppj/early5-devastation-runtime.json'  # 실제 화면 진행과 명시 입력 통합 검사를 구분한 기록이다.


def sha(path):
    """실행 당시 파일 전체 바이트의 SHA-256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(directory):
    """실제 입력 보고에서 자연 안내·적 피해·시전 패킷의 관측 범위만 계산한다."""
    execution = json.loads((directory/'run.json').read_text('utf-8'))
    observed = states(directory/'states.tsv')
    final = next(reversed(observed.values()))
    pages, minimum, maxima = [], {}, dict(cannons=0, golems=0, spells=0)
    first_e, first_f = None, None
    # 보고 이름을 성공으로 해석하지 않고 실제 객체와 단계만 순회한다.
    for label, state in observed.items():
        page = state.get('page', [''])[0]
        if page not in pages:
            pages.append(page)
        gate = state.get('mission_gate', [])
        if gate[:2] == ['5', '69'] and first_e is None:
            first_e = dict(label=label, frame=state['frame'], time=state['raw_world'][0])
        if gate[:2] == ['5', '70'] and first_f is None:
            first_f = dict(label=label, frame=state['frame'], time=state['raw_world'][0])
        counts = dict(cannons=0, golems=0, spells=0)
        # 같은 원본 적 SID와 실제 로컬 객체의 개수만 보관한다.
        for sid, row in state['objects'].items():
            if row[1] == '2' and row[0] in ('thunderCannon', 'sunAviary', 'priest'):
                minimum[str(sid)] = min(minimum.get(str(sid), int(row[12])), int(row[12]))
            if row[1] == '1':
                for key, kind in (('cannons', 'sunCannon'), ('golems', 'sunWalker'), ('spells', 'bombSpecialOne')):
                    counts[key] += row[0] == kind
        # 서로 다른 보고의 최대값을 합쳐 동시 생성으로 주장하지 않는다.
        for key in maxima:
            maxima[key] = max(maxima[key], counts[key])
    return dict(returnCode=execution['returnCode'], binarySha256=execution['binarySha256'],
                originalsUnchanged=execution['originalsUnchanged'], pages=pages, lastPage=final.get('page'),
                lastGate=final.get('mission_gate'), gameTime=final.get('raw_world', [None])[0],
                enemyMinimumHp=minimum, maximumOwn=maxima, firstE=first_e, firstF=first_f,
                castRequests=final.get('raw_packets', []).count('80'),
                templeDestroyed='VortexDestroyed' in pages,
                failure=(directory/'stderr.log').read_bytes().decode('utf-8', errors='replace').split('NetstormCpp: ')[-1]
                if execution['returnCode'] else None)


def verify():
    """현재 소스·원문·실행 결과를 대조하고 화면 시전과 미션 완주의 한계를 유지한다."""
    record = json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 과거 실행 증거를 각각 원래 저장한 해시와 비교한다.
    for group in ('sources', 'evidence'):
        for name, expected in record[group].items():
            assert sha(ROOT/name) == expected, name
    # 화면 입력 재생은 별도 상태 보고를 다시 읽어 확인한다.
    for name, expected in record['observations'].items():
        assert inspect(ROOT/name) == expected, name
        assert expected['originalsUnchanged']
    # 집중/전체 검사는 동일한 최종 바이너리의 실제 종료와 원문을 요구한다.
    for name, count in record['validation']['testRuns'].items():
        path = ROOT/name
        result = json.loads(path.read_text('utf-8'))
        assert result['returnCode'] == result['failed'] == 0 and result['tests'] == count
        assert result['binarySha256'] == record['binaries']['tests']
        assert sha(path.with_suffix('.stdout')) == result['stdoutSha256']
        assert sha(path.with_suffix('.stderr')) == result['stderrSha256']
    assert record['enemyCaptureDropIntegrated'] and not record['goalComplete'] and not record['fullFidelityVerified']
    assert record['runtimeTestsDeferred'] and not record['fullRegressionCompleted']
    cancelled = json.loads((ROOT/record['validation']['cancelledFull']).read_text('utf-8'))
    assert cancelled['returnCode'] != 0 and cancelled['tests'] is None and cancelled['failed'] is None
    interrupted = ROOT/record['validation']['cancelledUi']
    assert json.loads((interrupted/'run.json').read_text('utf-8'))['returnCode'] != 0
    assert not (interrupted/'states.tsv').exists()
    pins = json.loads((ROOT/record['validation']['currentPins']).read_text('utf-8'))
    assert pins['semanticReversalToHead'] and pins['reports'] == 98 and pins['pins'] == 181
    print('Early5 적 Devastation·실제 포획/내려놓기와 화면 관측 출처 감사 통과')


def run(plan, directory, frames):
    """새 폴더에 입력·종료·설정 복구 결과를 남기며 기존 관측을 덮어쓰지 않는다."""
    directory.mkdir(parents=True, exist_ok=False)
    rows = []  # 캡처 목적지 외에는 원래 계획의 입력을 바꾸지 않는다.
    # 게임 결과를 직접 쓰는 검사 동작을 허용하지 않는다.
    for line in plan.read_text('utf-8').splitlines():
        if not line or line.startswith('#'):
            continue
        frame, operation, argument = line.split('\t', 2)
        assert operation in ('click', 'click-if', 'up', 'up-if', 'right', 'esc', 'outside',
                             'key', 'keyup', 'report', 'snapshot', 'when', 'require')
        if operation == 'snapshot':
            argument = str(directory / (argument + '.bmp'))
        rows.append(f'{frame}\t{operation}\t{argument}\n')
    script = directory / 'steps.tsv'  # 실제 실행에 사용한 UTF-8 입력 원문이다.
    script.write_text(''.join(rows), encoding='utf-8', newline='\n')
    command = [str(EXE), '--run', str(ROOT/'originals'), '--mission', 'tutorial5', '--window',
               '--no-audio', '--frames', str(frames), '--set',
               'SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;tellTips=0',
               '--ui-script', str(script), '--ui-report', str(directory/'states.tsv')]
    startup = subprocess.STARTUPINFO()  # 실행 보조 콘솔만 숨기며 클론 창은 정상 경로로 실행한다.
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    before = snapshots(ROOT/'originals')
    (directory/'originals-before.json').write_text(json.dumps(before, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    binary = sha(EXE)  # 이후 빌드의 해시로 과거 실행을 덮어쓰지 않는다.
    with preserve_game_settings(ROOT/'originals', 'd'):
        result = subprocess.run(command, cwd=ROOT, capture_output=True, startupinfo=startup, timeout=frames*.2+60)
        (directory/'stdout.log').write_bytes(result.stdout)
        (directory/'stderr.log').write_bytes(result.stderr)
    execution = dict(command=command, binarySha256=binary, returnCode=result.returncode,
                     originalsUnchanged=snapshots(ROOT/'originals') == before)
    (directory/'run.json').write_text(json.dumps(execution, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(execution, ensure_ascii=False), flush=True)
    assert execution['originalsUnchanged']


def main():
    """지정한 입력만 재생하고 저장 상태의 요약은 읽기 전용으로 표시한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plan', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--frames', type=int)
    parser.add_argument('--inspect', type=Path)
    parser.add_argument('--verify', action='store_true')
    options = parser.parse_args()
    if options.verify:
        verify()
    elif options.inspect:
        print(json.dumps(inspect(options.inspect.resolve()), ensure_ascii=False, indent=2))
    elif options.plan and options.output and options.frames:
        run(options.plan.resolve(), options.output.resolve(), options.frames)
    else:
        parser.error('--plan/--output/--frames 또는 --inspect가 필요합니다')


if __name__ == '__main__':
    main()
