#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""캠페인1-1의 실제 ESC/Players 송금에서 SP와 AI 생산 지갑 이전을 관측한다."""
import argparse
import shutil
from pathlib import Path
from cpp_banner_smoke import run, SIZES
from cpp_bridge_ritual_runtime import ROOT, GAME, save, sha

# 이미 정상 입장한 일반 자산 사본이며 각 실행에서 별도 하위 사본을 만든다.
SOURCE = ROOT/'extracted/cpp-building-menus-532/final/data-1024x768'
ALL_ITEM = 'menu-command:02000095,005cad98'  # 원래 미션 메뉴의 정적 asksend 토큰이다.


def plan(output):
    """정상 클릭·문자 사건만 작성하고 게임 상태나 지갑을 직접 바꾸지 않는다."""
    steps = [(5, 'report', 'brief'), (10, 'click', 'Play Mission'), (20, 'report', 'initial'),
             (25, 'key', 'shift+f9'), (30, 'report', 'paused')]

    def menu(frame, command, label):
        """실제 ESC 가로막대에서 상대와 같은 Send Money 자식을 순서대로 연다."""
        steps.extend([(frame, 'esc', ''), (frame+5, 'click', 'Players'),
                      (frame+10, 'click', 'The Jailer'), (frame+15, 'click', 'Send Money'),
                      (frame+20, 'report', label+'Menu'), (frame+25, 'click', command)])

    # 고정 세 금액은 서로 다른 명령을 실제 메뉴에서 선택한다.
    for frame, command, label in [(35, '02000120', '300'), (80, '02000108', '700'), (125, '020000ec', '1000')]:
        menu(frame, 'menu-command:'+command+',2', label)
        steps.append((frame+35, 'report', 'after'+label))
    menu(170, 'menu-command:02000100,2', 'X')
    steps.extend([(205, 'report', 'XDialog'), (205, 'snapshot', str(output/'x-dialog.bmp')),
                  (210, 'key', '2'), (215, 'key', '2'), (220, 'key', '5'), (225, 'report', 'XTyped'),
                  (230, 'click', 'OK'), (240, 'report', 'afterX')])
    menu(250, 'menu-command:02000100,2', 'cancel')
    steps.extend([(285, 'click', 'Cancel'), (295, 'report', 'afterCancel')])
    menu(305, 'menu-command:02000100,2', 'tooMuch')
    # 원래 숫자 입력 제한 안의 금액이며 현재 예산을 넘긴다.
    for index in range(8):
        steps.append((335+index, 'key', '9'))
    steps.extend([(350, 'click', 'OK'), (360, 'report', 'afterTooMuch')])
    menu(365, 'menu-command:02000100,2', 'zero')
    steps.extend([(400, 'click', 'OK'), (410, 'report', 'afterZero')])
    menu(415, 'menu-command:020000ec,2', 'short')
    steps.append((450, 'report', 'afterShort'))
    menu(460, ALL_ITEM, 'allNo')
    steps.extend([(495, 'report', 'allNoAsk'), (495, 'snapshot', str(output/'all-confirm.bmp')),
                  (500, 'click', 'No'), (510, 'report', 'afterNo')])
    menu(520, ALL_ITEM, 'allYes')
    steps.extend([(555, 'click', 'Yes'), (565, 'report', 'afterAll')])
    menu(575, 'menu-command:02000120,2', 'empty')
    steps.extend([(610, 'report', 'afterEmpty'), (615, 'key', 'shift+f9'), (625, 'report', 'resumed'),
                  (630, 'esc', ''), (635, 'click', 'Game'), (640, 'click', 'Leave Mission'),
                  (650, 'click', 'Main Menu'), (660, 'report', 'main')])
    return steps


def one(output, size):
    """전체 실행은 현재 같은 바이너리와 개인 데이터 사본으로 순차 진행한다."""
    width, height, fullscreen = SIZES[size]
    output.mkdir(parents=True, exist_ok=False)
    data = output/'data'
    shutil.copytree(SOURCE, data)
    observed, result = run(output/'run', data, width, height, fullscreen, plan(output), 670, 'thewarbegins')

    def money(label):
        """실제 분산 SP와 실제 AI+2d8 지갑을 별도 열에서 읽는다."""
        row = observed[label]
        return float(row['raw_sp'][1]), int(row['raw_ai_money'][2])

    initial = money('paused')
    assert initial[0] == 3000 and initial[1] == 0, initial
    expected = {'after300': (2700, 300), 'after700': (2000, 1000), 'after1000': (1000, 2000),
                'afterX': (775, 2225), 'afterCancel': (775, 2225), 'afterTooMuch': (775, 2225),
                'afterZero': (775, 2225), 'afterShort': (775, 2225), 'afterNo': (775, 2225),
                'afterAll': (0, 3000), 'afterEmpty': (0, 3000)}
    # 모든 이전은 원래 수동 정지를 유지해 AI 소비/수입과 송금을 섞지 않는다.
    for label, values in expected.items():
        assert money(label) == values, (label, money(label), values)
        assert observed[label]['paused'] == ['1'] and observed[label]['time'][0] == observed['paused']['time'][0]
        assert observed[label]['raw_session'] == ['1', '1'] and observed[label]['raw_world_modes'] == ['0', '1']
    assert observed['resumed']['paused'] == ['0'] and observed['main']['phase'] == ['0']
    assert observed['main']['state_hud'][0] == '0'
    summary = dict(size=[width, height], fullscreen=fullscreen, run=result, initial=initial,
                   transfers={label: money(label) for label in expected}, realAiWallet=True,
                   cancelledAndInsufficientPreserved=True, noDirectMutation=True)
    save(output/'summary.json', summary)
    return summary


def main():
    """지정한 화면비를 검사하고 실패를 덮지 않은 채 현재 실행파일 SHA를 고정한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--case', choices=SIZES, action='append')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    results = []
    # 한 실제 게임이 끝난 뒤에만 다음 개인 사본을 연다.
    for size in args.case or SIZES:
        results.append(one(output/size, size))
    save(output/'summary.json', dict(results=results, binarySha256=sha(GAME), originalGameRuns=0,
                                    newMultiplayerTests=0, naturalClears=0))
    print('Send Money 실제 금액/X/All/취소/자금 부족 확인 완료')


if __name__ == '__main__':
    main()
