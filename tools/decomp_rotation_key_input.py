#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""C/c 회전 방향 키의 실제 일반 입력·현재 DWORD·Config 호출 순서를 먼저 동결한다(717)."""
import hashlib
import itertools
import json
from pathlib import Path
import socket
import sys
import decomp_production_shortcuts as gate
from decomp_game_menu_actions import Oracle as MenuOracle, cases
from unicorn.x86_const import UC_X86_REG_ESP

ROOT = Path(__file__).resolve().parents[1]  # 현재 PC에서 완료한 정밀 자료/보호 PE만 읽는다.
MODE = 0x54db60  # 원문 Interpret Options와 C 분기가 공유하는 rotmode DWORD다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/rotation-key-input-x86.tsv'  # 최초 관측 뒤 덮어쓰지 않는다.
REPORT = ROOT / 'cpppj/rotation-key-native-20261102.json'  # 부분 경계와 실제 실행 범위를 구별한다.
WORK = ROOT / 'extracted/cpp-rotation-key-717'  # 전체 원시 관측을 Git 제외 경로에 둔다.


def digest(path):
    """입력과 최초 기대값의 전체 물리 바이트를 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class Oracle(MenuOracle):
    """기존 일반 입력 관문에 회전 DWORD의 실제 쓰기와 Config 시점 읽기만 추가한다."""
    def guard(self, mu, access, address, size, value, data):
        """허용한 rotmode 네 바이트 이외에는 기존 쓰기 제한을 그대로 따른다."""
        if address == MODE and size == 4:
            self.writes['rotation'] += 1
            return
        return super().guard(mu, access, address, size, value, data)

    def instruction(self, mu, address, size, data):
        """원문이 Config를 호출하는 순간의 같은 현재 전역을 기록하고 실제 분배를 계속한다."""
        if address == 0x441580:
            sp = mu.reg_read(UC_X86_REG_ESP)
            self.config_calls.append((self.cstring(self.number(sp + 4)), self.number(MODE)))
        return super().instruction(mu, address, size, data)

    def rotation(self, case, initial, control):
        """원문 일반 입력 전체 범위를 실행하며 결과를 Python에서 계산하지 않는다."""
        self.number(MODE, initial)
        self.config_calls = []
        output = self.run(case, control)
        output.update(rotationMode=self.number(MODE), configCalls=self.config_calls)
        return output


def inputs():
    """문자/수정키/뗌/정지·초기 전체 DWORD와 Config 재진입/중단 입력만 만든다."""
    seed = cases()[0]
    rows = []  # 각 원문 실행에 넘길 입력이며 기대 분기를 담지 않는다.
    for key, mods, release, pause, initial in itertools.product((ord('C'), ord('c')), range(8), (0, 0x80000000), (0, 1), (0, 1, 0x80000000, 0xffffffff)):  # Alt/Control/Shift를 독립 비트로 공급한다.
        flags = ((mods & 1) * 0x20000000) | ((mods & 2) * 0x08000000) | ((mods & 4) * 0x02000000)
        values = list(map(int, seed['globals'].split(':')))
        values[8], values[9] = pause, 0
        case = seed.copy()
        case.update(name=f'rotation-{len(rows)}', event=f'{key | flags | release}:37:4294967277', globals=':'.join(map(str, values)), guideResult=0, gumpResult=0, read=0, filled=0, mutation='-', stop='-')
        rows.append((case, initial, 0, 0))
    for initial, mutate, stop in itertools.product((0, 0xffffffff), (0, 0x80000001), (0, 1)):  # Config 진입 전에 쓴 값과 callback 뒤 최종 값을 구별한다.
        case = rows[0][0].copy()
        case.update(name=f'callback-{len(rows)}', mutation=f'config1/{MODE}/{mutate}', stop='config1' if stop else '-')
        rows.append((case, initial, mutate, stop))
    return rows


def main():
    """최초 fixture만 만들며 verify는 원문 실행 없이 같은 해시와 행 수를 확인한다."""
    if '--verify' in sys.argv:
        record = json.loads(REPORT.read_text('utf-8'))
        for name, expected in record['sha256'].items():  # 최초 생성한 자료를 제품 변화에 맞춰 다시 쓰지 않는다.
            assert digest(ROOT / name) == expected, name
        assert record['inputs'] == 264
        print('회전 C 키 최초264입력 보존 확인')
        return
    assert not FIXTURE.exists() and not REPORT.exists(), '최초 기대값 덮어쓰기 금지'
    oracle = Oracle()
    rows, observations = [], []  # 원문이 실제 반환한 필드만 저장한다.
    for case, initial, mutate, stop in inputs():  # 전체 일반 입력의 실제 queue/Guide/Gump 관문도 함께 실행한다.
        result = [oracle.rotation(case, initial, control) for control in (0x27f, 0x37f, 0x27f)]
        assert result[0] == result[1] == result[2]
        output = result[0]
        code = int(case['event'].split(':')[0])
        pause = int(case['globals'].split(':')[8])
        calls = output['configCalls']
        assert len(calls) <= 1
        config, at_call = calls[0] if calls else ('-', 0xffffffff)
        rows.append((code, initial, pause, mutate, stop, output['rotationMode'], config, at_call, output['completed'], output['stackdelta']))
        observations.append(dict(input=case, initial=initial, observations=result))
    assert len(rows) == 264
    FIXTURE.write_text('# event initial pause mutate stop final config atConfig completed stackDelta\n' + ''.join('\t'.join(map(str, row)) + '\n' for row in rows), 'utf-8', newline='\n')
    (WORK / 'native-observations.json').write_text(json.dumps(observations, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    paths = ['AGENTS.md', 'originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'extracted/refined/originals/raw.c', 'tools/decomp_rotation_key_input.py', 'tools/decomp_game_menu_actions.py', 'tools/decomp_production_shortcuts.py', FIXTURE.relative_to(ROOT).as_posix()]
    record = dict(unit=717, date='2026-11-02', host=socket.gethostname(), lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-23'), inputs=len(rows), observations=len(rows) * 3, precisionOrRepeatDifferences=0, nativeRange='004d6e61..004da477', rotationBody='004d97a0..004d97ce', rotationGlobal='0054db60', gameExecutions=0, multiplayerTests=0, boundaries=['Guide 가상 입력/Gump/Config/formatter는 기존 명시 장치다. 실제 일반 입력과 정지/뗌/회전 DWORD 쓰기 순서를 실행한다.', 'Config callback의 전역 변경/명시 중단은 입력이다. 기존 큐/출력/스택/x87와 최초 fixture를 보존한다.', '전체 원문 Interpret Options/GUI/실제 회전·OS·통신을 실행한 관측은 아니다.'], sha256={name: digest(ROOT / name) for name in paths})
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    print(json.dumps({key: value for key, value in record.items() if key not in ('sha256', 'boundaries')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
