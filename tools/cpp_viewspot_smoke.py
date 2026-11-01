#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 RunForIt의 [Init] 대본 ViewSpot이 입장 카메라를 지정 좌표로 옮기는지 클론에서 관측한다(단위676)."""
import argparse
import json
import subprocess
from pathlib import Path
from cpp_smoke_files import preserve_game_settings

ROOT = Path(__file__).resolve().parents[1]
WIDTH, HEIGHT = 1024, 768  # 원본 최대 해상도의 창 검사 크기다.
SPOT = (139, 88)  # extracted/tarc/d/runforit.english [Init]의 <$ViewSpot,(139x88)> 입력이다.
LABELS = ((3, 'first'), (8, 'early'), (20, 'briefing'), (30, 'later'))  # Guide 단계 진행 전후의 관측 프레임이다.


def camera_rows(path):
    """검사 보고에서 프레임별 raw_camera_frame 행과 단계 표시를 읽는다."""
    rows, label = {}, None
    # 보고는 항목별 탭 행이며 report 머리가 뒤 행의 묶음을 정한다.
    for line in path.read_text(encoding='utf-8', errors='replace').splitlines():
        fields = line.split('\t')
        if fields[0] == 'frame':
            label = fields[2]
        if fields[0] == 'raw_camera_frame' and label:
            rows[label] = [int(value) for value in fields[1:]]
    return rows


def main():
    """지정 실행 파일로 RunForIt에 들어가 카메라 목표를 출력하고 필요하면 기대 좌표를 판정한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT/'cpppj/build/bin/Release/Netstorm.exe')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--expect', action='store_true', help='목표가 ViewSpot 좌표인지 판정한다.')
    arguments = parser.parse_args()
    arguments.output.mkdir(parents=True, exist_ok=False)
    data = ROOT/'originals'  # 보호 원본 자료를 읽기만 하며 허용된 두 설정은 preserve가 원래 바이트로 되돌린다.
    plan, report = arguments.output/'steps.tsv', arguments.output/'states.tsv'
    plan.write_text(''.join(f'{frame}\treport\t{label}\n' for frame, label in LABELS), encoding='utf-8', newline='\n')
    command = [str(arguments.exe), '--run', str(data), '--mission', 'RunForIt', '--no-audio', '--window', '--frames', '34',
               '--set', f'SCREENW={WIDTH};SCREENH={HEIGHT};maxFPS=40;sleepPerLoop=1;edgeScroll=0;scrollWhileMouseOut=0;autoDemo=0;tellTips=0',
               '--ui-script', str(plan), '--ui-report', str(report)]
    with preserve_game_settings(data, 'd', arguments.output/'settings-backup'):
        result = subprocess.run(command, capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=120)
    (arguments.output/'stdout.log').write_text(result.stdout, encoding='utf-8')
    (arguments.output/'stderr.log').write_text(result.stderr, encoding='utf-8')
    rows = camera_rows(report)
    summary = {'exit': result.returncode, 'camera': rows, 'unknownViewSpot': 'ViewSpot' in result.stderr}
    print(json.dumps(summary, ensure_ascii=False))
    (arguments.output/'summary.json').write_text(json.dumps(summary, ensure_ascii=False, indent=1), encoding='utf-8', newline='\n')
    assert result.returncode == 0, result.stderr[-800:]
    if arguments.expect:
        expected = [SPOT[0]*16-WIDTH//2, SPOT[1]*11-HEIGHT//2]  # 원문497f20의 16/11 곱과 화면 절반이다.
        assert rows['later'][:2] == expected and rows['later'][2:4] == expected, (rows['later'], expected)
        assert not summary['unknownViewSpot']


if __name__ == '__main__':
    main()
