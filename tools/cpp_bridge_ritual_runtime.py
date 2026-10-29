#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""명시 테스트 다리 순서와 실제 클론 입력/집중 검사를 새 경로에 기록한다. 원본 분석 도구가 아니다."""
import argparse
import hashlib
import json
import re
import subprocess
import time
from pathlib import Path
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings

ROOT = Path(__file__).resolve().parents[1]  # 원본은 읽기 전용이고 새 관측은 별도 출력 경로에 둔다.
GAME = ROOT / 'cpppj/build/bin/Release/Netstorm.exe'  # 현재 복원한 클론만 실행한다.
TESTS = ROOT / 'cpppj/build/bin/Release/netstorm_tests.exe'  # 현재 빌드의 집중 검사다.


def sha(path):
    """해당 실행 당시의 전체 파일 바이트를 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    """관측 메타데이터를 UTF-8/LF로 기록한다."""
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def execute(command, directory, binary, timeout):
    """실제 프로세스/PID·종료·원시 로그를 기록하며 타임아웃도 실패로 보존한다."""
    start = time.monotonic();identity = sha(binary)
    with (directory/'stdout.log').open('wb') as stdout, (directory/'stderr.log').open('wb') as stderr:
        # 보조 콘솔만 숨긴다. STARTF_USESHOWWINDOW/SW_HIDE는 게임의 첫 ShowWindow까지 숨겨 실제 커서 Tick을 비활성화하므로 사용하지 않는다.
        process = subprocess.Popen(command, cwd=ROOT, stdout=stdout, stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
        save(directory/'running.json', dict(pid=process.pid, command=command, binarySha256=identity))
        print(json.dumps(dict(pid=process.pid, output=str(directory)), ensure_ascii=False), flush=True)
        try:
            code = process.wait(timeout=timeout);timed_out = False
        except subprocess.TimeoutExpired:
            process.terminate();code = process.wait(timeout=20);timed_out = True
    return dict(command=command, binarySha256=identity, returnCode=code, timedOut=timed_out, seconds=time.monotonic()-start, stdoutSha256=sha(directory/'stdout.log'), stderrSha256=sha(directory/'stderr.log'))


def main():
    """사용자가 선택한 구동 또는 검사만 실행하며 기존 결과/원본 기대값은 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--filter');parser.add_argument('--plan', type=Path);parser.add_argument('--patterns');parser.add_argument('--mission', default='tutorial5')
    parser.add_argument('--frames', type=int, default=1000);parser.add_argument('--fps', type=int, default=40);parser.add_argument('--sleep-ms', type=int, default=1)
    args = parser.parse_args();directory = args.output.resolve();directory.mkdir(parents=True, exist_ok=False)
    if args.filter:
        if args.plan or args.patterns:
            parser.error('--filter와 게임 입력은 함께 지정할 수 없습니다')
        result = execute([str(TESTS), '--filter', args.filter], directory, TESTS, 240)
        stdout = (directory/'stdout.log').read_text(encoding='utf-8', errors='replace');match = re.search(r'(\d+) test\(s\), (\d+) failed check\(s\)', stdout)
        result.update(tests=int(match[1]) if match else None, failed=int(match[2]) if match else None, testOnlyBridgeManipulation=False)
    elif args.plan:
        steps = []
        # 캡처 경로만 새 폴더로 옮기고 실제 입력 원문은 그대로 보존한다.
        for line in args.plan.read_text(encoding='utf-8').splitlines():
            if not line or line.startswith('#'):
                continue
            frame, operation, argument = line.split('\t', 2)
            if operation == 'snapshot':
                argument = str(directory/(Path(argument).stem+'.bmp'))
            steps.append(f'{frame}\t{operation}\t{argument}\n')
        script = directory/'steps.tsv';script.write_text(''.join(steps), encoding='utf-8', newline='\n')
        command = [str(GAME), '--run', str(ROOT/'originals'), '--mission', args.mission, '--window', '--no-audio', '--frames', str(args.frames), '--set', f'SCREENW=1024;SCREENH=768;maxFPS={args.fps};sleepPerLoop={args.sleep_ms};autoDemo=0;tellTips=0', '--ui-script', str(script), '--ui-report', str(directory/'states.tsv')]
        if args.patterns is not None:
            command.extend(['--test-bridge-patterns', args.patterns])
        before = snapshots(ROOT/'originals');save(directory/'originals-before.json', before)
        with preserve_game_settings(ROOT/'originals', 'd'):
            result = execute(command, directory, GAME, max(90, args.frames/max(1, args.fps)*4+30))
        result.update(originalsUnchanged=snapshots(ROOT/'originals')==before, testOnlyBridgeManipulation=args.patterns is not None, bridgePatterns=args.patterns, planSha256=sha(args.plan))
    else:
        parser.error('--filter 또는 --plan이 필요합니다')
    save(directory/'run.json', result);print(json.dumps(result, ensure_ascii=False), flush=True)
    if result['returnCode'] or result['timedOut'] or result.get('originalsUnchanged') is False:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
