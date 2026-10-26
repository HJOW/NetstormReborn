#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 문자→생산 큐→Gump 취득과 커서 회전·배치·보충을 클론 화면에서 관측한다."""
import argparse
import json
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from cpp_production_panel_smoke import repeated

KEYS = (('q', 0), ('W', 1), ('a', 2), ('S', 3), ('z', 4), ('X', 5), ('y', 4), ('E', None))  # 실제 명령 큐 취득을 요청하는 슬롯과 전체 예약이다.


def main():
    """원본 파일이나 게임 상태를 주입하지 않고 현재 빌드와 입력 계획/화면/전체 보고를 보존한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--data-root', type=Path, default=ROOT/'originals', help='실행 자료 복사본 경로. 생략하면 기존 originals를 사용한다.')
    args = parser.parse_args();out = args.output.resolve();out.mkdir(parents=True, exist_ok=False)
    data_root = args.data_root.resolve()  # 별도 복사본에서도 같은 원본 자원과 입력열로 검사한다.
    steps = [(5, 'key', 'q'), (8, 'report', 'briefingKey'), (10, 'click', 'Play Mission'),
             (15, 'key', 'X'), (18, 'report', 'missingSlot'),
             (1100, 'report', 'mature'), (1100, 'snapshot', str(out/'mature.bmp')),
             (1105, 'esc', ''), (1107, 'report', 'menuOpen'), (1108, 'key', 'q'), (1110, 'report', 'menuKey'),
             (1115, 'esc', ''), (1119, 'report', 'closedMenu')]  # 실제막대는자동Pause하지않으므로q취득후ESC로커서/팝업을취소한다.
    for index, (key, slot) in enumerate(KEYS):  # 현재 공급 슬롯에 있거나 없는 모든 키를 같은 활성 커서에서 차례대로 누른다.
        frame = 1130 + index*10;steps.extend([(frame, 'key', key), (frame+4, 'report', 'key'+key)])
    steps.extend([(1215, 'keyup', 'W'), (1218, 'report', 'releasedKey'), (1220, 'key', 'q'), (1224, 'report', 'retaken'),
                  (1230, 'key', 'right'), (1231, 'keyup', 'right'), (1234, 'report', 'rotated'), (1234, 'snapshot', str(out/'rotated.bmp')),
                  (1240, 'key', 'left'), (1241, 'keyup', 'left'), (1244, 'report', 'restored'),
                  (1250, 'up', 'cell:109,97'), (1260, 'report', 'prepared'), (1270, 'click', 'cell:109,97'),
                  (1280, 'report', 'placed'), (1280, 'snapshot', str(out/'placed.bmp')), (1880, 'report', 'refilled'), (1880, 'snapshot', str(out/'refilled.bmp'))])  # 양방향 회전 후 원래 방향의 실제 합법 위치에 배치한다.
    steps.sort(key=lambda item:item[0]);script = out/'steps.tsv';report = out/'states.tsv'
    script.write_text(''.join(f'{frame}\t{op}\t{argument}\n' for frame, op, argument in steps), encoding='utf-8', newline='\n')
    command = [str(GAME), '--run', str(data_root), '--mission', 'thewarbegins', '--window', '--no-audio', '--frames', '1883',
               '--set', 'SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=0', '--ui-script', str(script), '--ui-report', str(report)]
    before = snapshots(data_root);save(out/'originals-before.json', before)
    with preserve_game_settings(data_root, 'd'):
        result = execute(command, out, GAME, 110)
    result.update(originalsUnchanged=snapshots(data_root)==before, dataRoot=str(data_root), testOnlyBridgeManipulation=False)
    save(out/'run.json', result)
    if result['returnCode'] or result['timedOut'] or not result['originalsUnchanged']:
        raise RuntimeError(result)
    observed = states(report);contents = repeated(report, 'raw_production_content');bridges = repeated(report, 'raw_production_bridge')
    assert observed['briefingKey']['phase']==['3'] and observed['briefingKey']['raw_cursor'][0]!='0', observed['briefingKey']
    early_slots = {int(row[4]) for row in contents['missingSlot'] if int(row[2])&0x40}  # 아직 공급하지 않은 슬롯5를 X로 요청해도 취득하지 않는다.
    assert 5 not in early_slots and observed['missingSlot']['raw_cursor'][0]!='0', early_slots
    mature = observed['mature'];slots = {int(row[4]):int(row[0]) for row in contents['mature'] if int(row[2])&0x40}  # 실제로 공급된 동일 기술의 SID만 기대 입력으로 사용한다.
    assert mature['paused']==['0'] and mature['popup']==[''] and mature['mission_start'][0]=='1' and slots, mature
    assert observed['menuOpen']['paused']==['0'] and observed['menuOpen']['popup']==[''] and observed['menuOpen']['game_menu_bar'][1]=='1', observed['menuOpen']
    cursor = observed['menuKey']['raw_cursor']
    assert observed['menuKey']['paused']==['0'] and cursor[0]=='0' and int(cursor[2])==82 and int(cursor[3])==slots[0], cursor
    assert observed['closedMenu']['paused']==['0'] and observed['closedMenu']['game_menu_bar'][1]=='0' and observed['closedMenu']['raw_cursor'][0]!='0', observed['closedMenu']
    previous = None;selection = []
    for key, slot in KEYS:  # 없는 슬롯은 이전 커서가 유지되고 E는 현재 존재하는 마지막 슬롯까지 순서대로 취득한다.
        target = max(slots) if slot is None else slot
        if target in slots:previous = slots[target]
        actual = observed['key'+key]['raw_cursor'];assert previous is not None and actual[0]=='0' and int(actual[2])==82 and int(actual[3])==previous, (key, slots, actual)
        selection.append(dict(key=key, requestedSlot=slot, actualSource=previous, slotPresent=target in slots))
    assert observed['releasedKey']['raw_cursor'][3]==observed['keyE']['raw_cursor'][3]
    assert int(observed['retaken']['raw_cursor'][3])==slots[0]
    direction = int(observed['retaken']['raw_piece'][1]);assert int(observed['rotated']['raw_piece'][1])==(direction+2)%8
    assert int(observed['restored']['raw_piece'][1])==direction
    assert observed['prepared']['raw_cursor'][8:]==['1','1','1'], observed['prepared']['raw_cursor']
    before_bridges = {sid:row for sid,row in mature['raw_objects'].items() if sid>=15000 and row[:2]==['82','1']}
    placed = {sid:row for sid,row in observed['placed']['raw_objects'].items() if sid>=15000 and row[:2]==['82','1']}
    assert set(placed)-set(before_bridges), placed
    local_owner = int(mature['raw_session'][0])  # 적 AI의 자연 수입을 금지하지 않고 로컬 다리 비용만 확인한다.
    assert observed['placed']['raw_sp'][local_owner]==mature['raw_sp'][local_owner] and observed['refilled']['raw_sp'][local_owner]==mature['raw_sp'][local_owner]
    refilled = bridges['refilled'];assert len(refilled)==len(bridges['mature']) and all(row[2]=='2' for row in refilled), refilled
    summary = dict(passed=True, earlyMissingSlot=5, earlySlots=sorted(early_slots), slots=slots, selections=selection, rotated=observed['rotated']['raw_piece'], prepared=observed['prepared']['raw_cursor'],
                   newServerBridges=sorted(set(placed)-set(before_bridges)), refilled=refilled, originalsUnchanged=result['originalsUnchanged'],
                   limitations=['WM_CHAR 이후의 실제 큐/프레임 경로이며 OS 키 메시지 발생 자체는 주입한다.', '회전과 반대 회전 후 원래 방향으로 배치했다.', '현재 실제 공급 슬롯과 없는 슬롯을 검증하며 전체 미션 완주 증거는 별도다.'])
    save(out/'summary.json', summary);print(json.dumps(summary, ensure_ascii=False))


if __name__ == '__main__':  # 명시 CLI 요청에서만 실제 클론 창을 실행한다.
    main()
