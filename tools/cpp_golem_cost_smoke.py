#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""캠페인1-1의 실제 골렘400SP 지급과 원본 적SP0·399/400 경계 입력을 확인한다."""
import argparse
import re
import shutil
from pathlib import Path

from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save, sha
from cpp_menu_smoke import source
from cpp_production_panel_smoke import repeated
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from taff import TaffArchive

# 실제 캠페인 시작 자금과 사본 시나리오의 경계값을 구별한다.
AMOUNTS = (3000, 400, 399)
# 같은 원본4:3과 확장16:9 창에서 입력한다.
SIZES = ((1280, 720), (1024, 768))
# 원래 본섬의 사제/신전 근처에서 실제 배치 판정이 허용한 칸만 선택한다.
POINT = 'placement-near:94,112'


def one(output, data, size, money):
    """첫 생산 카드가 잠기지 않은 상태에서 실제 취득·배치 또는 자금 거부를 검증한다."""
    output.mkdir(parents=True, exist_ok=False)
    steps = [(10, 'click', 'Play Mission'), (50, 'report', 'initial'),
             (60, 'click', 'production:133'), (63, 'report', 'taken'),
             (63, 'snapshot', str(output / 'taken.bmp'))]
    if money >= 400:
        steps.extend([(70, 'up', POINT), (75, 'report', 'cursor'), (80, 'click', POINT),
                      (105, 'report', 'placed'), (190, 'right', 'world:sunWalker'),
                      (200, 'click', 'About'), (210, 'report', 'help'),
                      (210, 'snapshot', str(output / 'help.bmp')), (220, 'outside', ''),
                      (280, 'report', 'settled'), (280, 'snapshot', str(output / 'settled.bmp'))])
    else:
        steps.extend([(90, 'report', 'denied'), (120, 'report', 'settled'),
                      (120, 'snapshot', str(output / 'settled.bmp'))])
    script, report = output / 'steps.tsv', output / 'states.tsv'
    script.write_text(''.join(f'{f}\t{op}\t{arg}\n' for f, op, arg in steps), encoding='utf-8', newline='\n')
    command = [str(GAME), '--run', str(data), '--mission', 'thewarbegins', '--window', '--no-audio',
               '--frames', '300' if money >= 400 else '185', '--set',
               f'SCREENW={size[0]};SCREENH={size[1]};maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=1',
               '--ui-script', str(script), '--ui-report', str(report)]
    original, before, run = snapshots(ROOT / 'originals'), snapshots(data), {}
    try:
        with preserve_game_settings(data, 'd', output / 'settings-backup'):
            run = execute(command, output, GAME, 90)
    finally:
        run.update(originalsUnchanged=original == snapshots(ROOT / 'originals'), dataUnchanged=before == snapshots(data))
        save(output / 'run.json', run)
    assert run['returnCode'] == 0 and not run['timedOut'] and run['originalsUnchanged'] and run['dataUnchanged'], run
    observed = states(report)
    initial = observed['initial']
    cards = [row for row in repeated(report, 'raw_production_content')['initial'] if row[1] == '133' and int(row[2]) & 8]
    assert len(cards) == 1 and not int(cards[0][2]) & 0x100 and int(cards[0][3]) > 0, cards
    assert float(initial['raw_sp'][1]) == money
    for label, current in observed.items():  # 도중의 AI 생산과 차감도 모든 보고 지점에서 확인한다.
        assert current['phase'] == ['4'] and current['raw_world_modes'] == ['0', '1'] and current['paused'] == ['0']
        assert current['size'] == [str(size[0]), str(size[1])] and current['mouse_cursor'][1] == '1'
        assert float(current['raw_sp'][2]) == 0
        assert not any(row[:2] == ['133', '2'] for row in current['raw_objects'].values()), (label, current['raw_sp'])
    assert not any(row[:2] == ['133', '1'] for row in initial['raw_objects'].values())
    if money >= 400:
        assert float(observed['taken']['raw_sp'][1]) == money and float(observed['cursor']['raw_sp'][1]) == money
        made = [(sid, row) for sid, row in observed['placed']['raw_objects'].items() if row[:2] == ['133', '1']]
        assert len(made) == 1, made
        sid, row = made[0]
        for label in ('placed', 'help', 'settled'):  # 배치 때 한 번만400을 지불하고 같은 실제 골렘이 유지된다.
            current = observed[label]
            assert float(current['raw_sp'][1]) == money - 400
            assert current['raw_objects'][sid][:2] == ['133', '1'] and current['raw_objects'][sid][3:5] == row[3:5]
        assert any('Golem' in row[-1] for row in repeated(report, 'raw_help_body')['help'])
        assert not repeated(report, 'raw_help')['settled']
        result = dict(size=list(size), initialSp=money, finalSp=money-400, paid=400,
                      golemSid=sid, position=row[3:5], enemySp=0, enemyGolems=0, run=run)
    else:
        assert int(observed['taken']['raw_production_budget_flash'][0]) > 0
        assert int(observed['taken']['raw_cursor'][3]) == 0
        assert float(observed['settled']['raw_sp'][1]) == money
        assert not any(row[:2] == ['133', '1'] for row in observed['settled']['raw_objects'].values())
        result = dict(size=list(size), initialSp=money, finalSp=money, paid=0,
                      insufficientMoneyRejected=True, budgetFlashed=True, enemySp=0, enemyGolems=0, run=run)
    save(output / 'summary.json', result)
    return result


def main():
    """3000은 원문,400/399는 명시한 사본 시나리오 설정으로 따로 보존한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    data, output = args.data_root.resolve(), args.output.resolve()
    assert all(data != (ROOT / name).resolve() and (ROOT / name).resolve() not in data.parents for name in ('originals', 'originalCD'))
    output.mkdir(parents=True, exist_ok=False)
    text = source(data, TaffArchive(data / 'netstorm.tarc'), 'd/thewarbegins.english').decode('cp1252')
    assert re.search(r'(?im)^myStartMoney\s*=\s*3000\s*$', text)
    assert re.search(r'(?im)^aiStartMoney\s*=\s*0\s*$', text)
    custom = output / 'custom-data'
    shutil.copytree(data, custom)
    binary, results, scripts = sha(GAME), [], {}
    for money in AMOUNTS:  # 경계 시나리오는 원문에서 시작 자금 한 줄만 바꾸며 적의0SP는 보존한다.
        selected = data
        if money != 3000:
            selected = custom
            changed, count = re.subn(r'(?im)^myStartMoney\s*=\s*3000\s*$', f'myStartMoney = {money}', text)
            assert count == 1
            path = custom / 'd/thewarbegins.english'
            path.write_text(changed, encoding='utf-8', newline='\n')
            shutil.copy2(path, output / f'thewarbegins-{money}.english')
            scripts[str(money)] = sha(path)
        for size in SIZES:  # 같은 최종 제품과 같은 각 시나리오의 새 프로세스를 사용한다.
            result = one(output / f'{money}-{size[0]}x{size[1]}', selected, size, money)
            assert result['run']['binarySha256'] == binary == sha(GAME)
            results.append(result)
    save(output / 'summary.json', dict(results=results, binarySha256=binary, privateScenarioSha256=scripts,
                                     attackFlyersNotTested=True, rawObjectMutation=False))
    print('골렘400SP/399부족·적0SP 무료생산 제외의 두 화면비6실행 통과')


if __name__ == '__main__':
    main()
