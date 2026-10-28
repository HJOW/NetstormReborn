#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""격리 자료의 실제 번개·브리핑 정지·메뉴 이탈·재진입을 현재 ESC 메뉴로 검사한다."""
import argparse
import json
from pathlib import Path
from cpp_world_smoke import states
import cpp_game_menu_smoke as menu


def run(directory, data, width, height, fullscreen):
    """표시 모드별 원래 기한을 충분히 지난 뒤 같은 실제 상태와 출력 화면을 확인한다."""
    steps = [(5, 'report', 'initial')]
    for index in range(4):  # 브리핑 정지 중 네 곡을 모두 지나며 게임 시각과 독립적인 종료를 관측한다.
        frame = 10 + index * 50
        steps.extend([(frame, 'music-next', ''), (frame, 'report', f'cycle{index}'),
                      (frame, 'snapshot', str(directory / f'cycle{index}.bmp')),
                      (frame + 40, 'report', f'ended{index}')])
    steps.extend([(210, 'click', 'Play Mission'), (215, 'report', 'playing'),
                  (220, 'esc', ''), (225, 'click', 'Game'), (230, 'click', 'Leave Mission'),
                  (235, 'report', 'leaving')])
    for _ in range(4):  # 이탈 모달에서 같은 프레임에 한 번개를 새로 만들고 즉시 월드를 반납한다.
        steps.append((240, 'music-next', ''))
    steps.extend([(240, 'report', 'activeBeforeExit'), (240, 'click', 'Main Menu'), (245, 'report', 'menu')])
    for index in range(4):  # 부모가 없는 메뉴에서는 천둥 곡도 새 프로세스를 등록하지 않는다.
        frame = 250 + index * 20
        steps.extend([(frame, 'music-next', ''), (frame, 'report', f'menu{index}')])
    steps.extend([(330, 'click', 'Campaign'), (340, 'click', 'Struggle For Freedom'),
                  (350, 'click', '1 The War Begins!'), (360, 'report', 'reload')])
    report, result = menu.protected_run(directory, width, height, steps, 365, data_root=data, fullscreen=fullscreen)
    observed = states(report)
    assert observed['initial']['paused'] == ['1']
    previous = observed['initial']
    thunders = 0
    for index in range(4):  # 현재 곡과 등록/종료, 정지된 게임 시각과 진행한 벽시계를 따로 대조한다.
        current, ended = observed[f'cycle{index}'], observed[f'ended{index}']
        weather = int(current['weather'][0])
        assert weather == (int(previous['weather'][0]) + 1) % 4
        assert current['thunder_processes'] == [str(int(weather == 2))]
        assert ended['thunder_processes'] == ['0']
        assert current['paused'] == ended['paused'] == ['1']
        assert current['time'][0] == ended['time'][0]
        assert float(ended['time'][1]) > float(current['time'][1])
        thunders += int(weather == 2)
        previous = ended
    assert thunders == 1
    assert observed['menu']['thunder_processes'] == ['0']
    for index in range(4):  # 메뉴에 남은 PID가 후속 Kernel 슬롯을 잘못 삭제하지 않는지도 관측한다.
        assert observed[f'menu{index}']['thunder_processes'] == ['0']
    assert observed['reload']['thunder_processes'] == ['0']
    assert observed['reload']['paused'] == ['1']
    assert observed['playing']['shared_kernel'][0] == '1'
    result.update(states=len(observed), fullscreen=fullscreen, viewport=[width, height],
                  thunderCycles=thunders, scope='짧은 번개/메뉴 수명 검사이며 자연 완주가 아니다')
    (directory / 'verification.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    return result


def main():
    """명시한 사본과 새 출력 폴더에서 창/전체화면을 순서대로 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--exe', type=Path, default=menu.GAME)
    args = parser.parse_args()
    menu.GAME = args.exe.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    results = []
    for name, width, height, fullscreen in [('window', 1024, 768, False), ('fullscreen', 1280, 720, True)]:  # Windows10의 두 실제 장치를 같은 제품으로 비교한다.
        results.append(run(output / name, args.root.resolve(), width, height, fullscreen))
    (output / 'results.json').write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps([dict(returnCode=r['returnCode'], states=r['states'], fullscreen=r['fullscreen'],
                           originalsUnchanged=r['originalsUnchanged'], dataUnchanged=r['dataUnchanged']) for r in results], ensure_ascii=False))


if __name__ == '__main__':
    main()
