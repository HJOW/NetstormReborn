#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 미션 로딩 창의 초기 프레임·Cancel·후속 진입을 현재 클론 입력으로 관측한다."""
import argparse
from pathlib import Path
from PIL import Image, ImageChops

from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from cpp_demo_start_smoke import DEMOS


def main():
    """명시한 미션만 실행하고 프레임별 원시 보고서와 픽셀을 새 경로에 보존한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--mission', default='tutorial1')
    parser.add_argument('--cancel', action='store_true')
    parser.add_argument('--demo', action='store_true', help='실제 Demo 메뉴의 같은 이름 항목을 누른다')
    parser.add_argument('--reference', type=Path, help='같은 해상도의 원본 로딩 캡처와 실제 창 영역을 RGB로 대조한다')
    args = parser.parse_args()
    directory = args.output.resolve()  # 기존 관측을 덮어쓰지 않는 이번 실행 경로다.
    directory.mkdir(parents=True, exist_ok=True)
    if any(directory.iterdir()):
        raise RuntimeError('빈 관측 경로 필요')
    steps = []  # 시계·게임 상태를 쓰지 않는 실제 입력/출력 계획이다.
    loading_frame = 1  # CLI MissionBegin은 첫 프레임 입력에서 처리된다.
    if args.demo:
        labels = {name.casefold(): label for name, label in DEMOS}  # 원본 DEMOVILLE에 있는 세 라벨만 허용한다.
        label = labels[args.mission.casefold()]
        steps.extend(((3, 'click', 'Demo'), (5, 'click', label)))
        loading_frame = 6  # StartDemo 정수 큐가 다음 루프에서 실제 생성자를 호출한다.
    if args.cancel:
        steps.append((loading_frame+1, 'click', 'Cancel'))
    for frame in range(1, 17):  # 첫 State 전환부터 실제 로딩/브리핑 도달까지 같은 프레임을 읽는다.
        steps.append((frame, 'report', str(frame)))
        if frame <= 4 or loading_frame <= frame <= loading_frame+2:
            steps.append((frame, 'snapshot', str(directory / f'frame-{frame}.bmp')))
    steps.sort(key=lambda step: step[0])  # 같은 프레임에서는 입력 다음 관측의 계획 순서를 유지한다.
    script = directory / 'steps.tsv'  # 실행 파일이 읽는 UTF-8 입력 계획이다.
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps),
                      encoding='utf-8', newline='\n')
    command = [str(GAME), '--run', str(ROOT / 'originals'),
               '--window', '--no-audio', '--frames', '17', '--set',
               'SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0',
               '--ui-script', str(script), '--ui-report', str(directory / 'states.tsv')]
    if not args.demo:
        command.extend(('--mission', args.mission))
    before = snapshots(ROOT / 'originals')  # 허용 설정을 포함한 전체 원본 파일의 실행 전 해시다.
    try:
        with preserve_game_settings(ROOT / 'originals', 'd'):
            result = execute(command, directory, GAME, 60)
    finally:
        unchanged = before == snapshots(ROOT / 'originals')  # 예외/실패에서도 보호 원본 복원을 확인한다.
        save(directory / 'protection.json', dict(originalsUnchanged=unchanged))
    save(directory / 'result.json', result)
    observed = states(directory / 'states.tsv') if (directory / 'states.tsv').exists() else {}
    summary = dict(result=result, originalsUnchanged=unchanged,
                   phases={label: row['phase'][0] for label, row in observed.items()})  # 성공을 합성하지 않는 원시 관측 요약이다.
    loading = observed.get(str(loading_frame), {})  # 실제 로딩 프레임의 같은 등록표 관측이다.
    summary['loadingObserved'] = loading.get('phase') == ['2'] and 'raw_mission_loading' in loading
    final = observed.get('16', {})  # 후속 장면에서도 살아 있는 로딩 부모가 없는지 확인한다.
    summary['loadingReleased'] = 'raw_mission_loading' not in final
    summary['cancelReturnedToMenu'] = not args.cancel or final.get('phase') == ['0'] and 'mission' not in final
    summary['missionEntered'] = args.cancel or final.get('phase') in (['3'], ['4']) and bool(final.get('raw_objects'))
    if args.reference and summary['loadingObserved']:
        original = Image.open(args.reference).convert('RGB')  # 화면 밖 영역을 포함하지 않는 원본 client 캡처다.
        clone = Image.open(directory / f'frame-{loading_frame}.bmp').convert('RGB')  # 같은 프레임의 현재 렌더러 출력이다.
        if original.size != clone.size:
            raise RuntimeError('픽셀 대조 해상도 불일치')
        rect = tuple(map(int, loading['raw_mission_loading'][4:8]))  # 그리기와 같은 실제 부모 경계다.
        difference = ImageChops.difference(original.crop(rect), clone.crop(rect))
        raw = difference.tobytes()  # 세 채널 중 하나라도 다른 픽셀만 세며 허용 오차를 두지 않는다.
        different = sum(raw[index:index+3] != b'\0\0\0' for index in range(0, len(raw), 3))
        summary['panelComparison'] = dict(reference=str(args.reference.resolve()), rect=rect, differentPixels=different)
        difference.save(directory / 'panel-difference.png')
    save(directory / 'summary.json', summary)
    print(summary, flush=True)
    if result['returnCode'] or result['timedOut'] or not unchanged or not all(summary[key] for key in ('loadingObserved', 'loadingReleased', 'cancelReturnedToMenu', 'missionEntered')) or summary.get('panelComparison', {}).get('differentPixels', 0):
        raise RuntimeError('실제 미션 로딩 실행 실패: stderr.log/summary.json 참조')


if __name__ == '__main__':
    main()
