#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 스톤 타워의 실제 주변 총 수집/통지를 보호 PE로 관찰한다. 게임/OS 실행 없음."""
import argparse
import csv
import itertools
import json
import socket
import struct
from decomp_guntarget_oracle import GunTargetOracle, ROOT, POOL, HEAP, OUTPUT, PLAYERS, SHAPES, TYPES, bits, sha, FPU_CONTROLS
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

KEYS = ('kind', 'group', 'state', 'extra', 'allied', 'friendly', 'modeShift', 'missing', 'target', 'flyer',
        'capacity', 'reverse', 'x', 'y', 'width', 'height', 'extension', 'enabled')  # 기대값을 계산하지 않는 독립 입력 열이다.
LIST, DESCRIPTOR, PROCESSES = HEAP + 0x18000, HEAP + 0x18100, HEAP + 0x19000  # 격리된20DWORD 목록·세DWORD 헤더·기존 총 객체다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/gun-blocker-x86.tsv'  # 기존 Gun fixture와 분리한 실제 수집 결과다.
REPORT = ROOT / 'cpppj/gun-blocker-native.json'  # 원본 기대값을 먼저 고정한 출처·한계 기록이다.
ENTRIES = (0x469590, 0x46ae80, 0x469440, 0x4819b0, 0x41d1b0, 0x462800, 0x462710, 0x462650, 0x4aea10, 0x4ac950, 0x4ac550, 0x45ae60)  # 추가로 실제 실행할 몸체다.
GUNS = (15050, 15051, 15052, 15053, 15054)  # 같은 공간 해시와 기존 Form을 공유하는 다섯 입력 총이다.


def cases():
    """현재 표적·group·동맹·상태·방향·목록 한도·물리 발자국을 독립 입력으로 교차한다."""
    base = dict(zip(KEYS, (0, 2, 0, 0, 0, 0, 0, 0, 15060, 0, 20, 0, bits(50), bits(50), 3, 3, 0, 1)))
    rows = []  # 같은 입력은 최종 단계에서 한 번만 고정한다.
    # 기본 필터와 배제 조건은 각 group에서 모두 방문한다.
    for group, state, extra, allied, friendly in itertools.product(range(3), range(4), (0, 1, 16), range(2), range(2)):
        c = base.copy();c.update(group=group, state=state, extra=extra, allied=allied, friendly=friendly);rows.append(c)
    # 현재 표적과 실제 Gun 부재·정방향/역방향·목록 포화·입력 체인 순서를 교차한다.
    for group, target, shift, missing, capacity, reverse in itertools.product(range(3), (0, 15060, 15061), (0, 2), (0, 1, 31), (0, 1, 3, 20), range(2)):
        c = base.copy();c.update(group=group, target=target, modeShift=shift, missing=missing, capacity=capacity, reverse=reverse);rows.append(c)
    # 소수 좌표·모서리·다른 발자국·확장 높이와 지상1000점/공중 점수를 방문한다.
    for point, width, height, extension, flyer in itertools.product(((50, 50), (50.25, 50.75), (1.25, 1.25), (254.5, 254.5)), (1, 3), (1, 3), (0, 3), range(2)):
        c = base.copy();c.update(x=bits(point[0]), y=bits(point[1]), width=width, height=height, extension=extension, flyer=flyer);rows.append(c)
    # 실제 통지 래퍼는 boss0의 무호출·기존 Form 부재와 현재 수집 목록을 검사한다.
    for group, enabled, missing, target in itertools.product(range(3), range(2), (0, 1, 31), (0, 15060, 15061)):
        c = base.copy();c.update(kind=1, group=group, enabled=enabled, missing=missing, target=target);rows.append(c)
    return list({tuple(row[key] for key in KEYS): row for row in rows}.values())


class Oracle(GunTargetOracle):
    """실제 네 해시/일반 finder/좌표/점수/목록 분기를 실행하며 기존 Kernel 조회만 경계로 둔다."""
    def __init__(self):
        """같은 PC의 정밀 범위만 더하며 새 기대값 계산기는 넣지 않는다."""
        super().__init__('originals')
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        # 기존 실제 하위와 새 최상위의 불연속 명령 범위를 정확히 합친다.
        for entry in ENTRIES:
            self.gun_entries.add(entry)
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed[low-self.base:high+1-self.base] = b'\x01' * (high+1-low)
        self.spec = dict(self.spec, entries=dict(self.spec['entries'], Collect=0x469590, Notice=0x46ae80))

    def on_write(self, mu, access, address, size, value, data):
        """원본 결과 목록/헤더 외에는 기존 읽기 전용 계약을 유지한다."""
        if LIST <= address and address + size <= DESCRIPTOR + 12:
            return
        super().on_write(mu, access, address, size, value, data)

    def on_instruction(self, mu, address, size, data):
        """기존 Form/Kernel·현재 공격 모드와 외부 통지만 명시 인자로 관찰한다."""
        if address not in (0x4b1a90, 0x4ae2d0, 0x4d4d00, 0x46a960, 0x4e4391, 0x4e4354):
            return super().on_instruction(mu, address, size, data)
        sp = mu.reg_read(UC_X86_REG_ESP);this = mu.reg_read(UC_X86_REG_ECX)
        args = [self.read(sp+4+i*4) for i in range(4)]  # 실제 스택 인자이며 기대값을 만들지 않는다.
        value = 0;purge = 0  # cdecl이 기본이며 thiscall 한 인자만 별도 정리한다.
        if address == 0x4b1a90:
            assert args[1:] == [34, 0, 0]
            if args[0] in GUNS:
                index = GUNS.index(args[0]);value = 100+index if not self.case['missing'] & (1 << index) else 0
        elif address == 0x4ae2d0:
            form = (this-POOL)//self.stride;value = PROCESSES+(form-100)*0x40+0x28
        elif address == 0x4d4d00:
            sid = (this-POOL)//self.stride
            if sid in GUNS:
                index = GUNS.index(sid);value = ((4, 0, 2, 6, 0)[index] + self.case['modeShift']) % 8
        elif address == 0x46a960:
            index = (this-PROCESSES)//0x40
            self.events.append(f'N:{GUNS[index]}:{args[0]}');purge = 4
        elif address == 0x4e4391:
            assert args[0] == 0x50;value = LIST
        else:
            assert args[0] == LIST
        self.stub_calls[f'{address:08x}'] += 1
        mu.reg_write(UC_X86_REG_EAX, value);mu.reg_write(UC_X86_REG_ESP, sp+4+purge);mu.reg_write(UC_X86_REG_EIP, self.read(sp))

    def run(self, case, control):
        """입력 공간과 현재 타입/동맹/Gun만 공급하고 실제 반환 목록·통지 순서를 기록한다."""
        self.case = case
        self.mu.mem_write(POOL, bytes(24000*self.stride));self.mu.mem_write(TYPES, bytes(256*self.type_stride))
        coordinates = ((49, 20), (49, 80), (20, 49), (80, 49), (49, 45))  # 입력 총 다섯 개의 실제 원점이다.
        groups = [case['group']]*5 if case['group'] < 2 else [1, 1, 1, 1, 0]  # group은 원본 TYPE getter로 읽힌다.
        objects = [(sid, 100+i, case['state'], case['extra'], bits(x), bits(y), 1, 1, 0, 1)
                   for i, (sid, (x, y)) in enumerate(zip(GUNS, coordinates))]
        if case['reverse']:
            objects.reverse()
        objects += [(15060, 90, 0, 0, bits(49), bits(150), 2, 2, case['extension'], 1),
                    (15061, 91, 0, 0, bits(48), bits(48), 2, 2, case['extension'], 1),
                    (15070, 78, 0, 0, case['x'], case['y'], case['width'], case['height'], 0, 1)]
        self.setup(objects)
        # 각 총의 현재 타입·소유자·SHP 포구·기존 표적 필드를 명시 입력으로 채운다.
        for i, sid in enumerate(GUNS):
            type_address = TYPES+(100+i)*self.type_stride
            self.put(type_address+0x9c, groups[i]);self.put(self.slot(sid)+34, 2, 1)
            shape = SHAPES+i*0x400;self.put(type_address+0xdc, shape);self.put(type_address+0x114, 1)
            self.put(shape+8, 0x80+0x24);self.mu.mem_write(shape+0xa0, bytes(4))
            self.mu.mem_write(PROCESSES+i*0x40, bytes(0x40));self.put(PROCESSES+i*0x40+0x18, case['target'], 2)
        self.put(TYPES+90*self.type_stride+0xec, 0x100000 if case['flyer'] else 0)
        self.put(TYPES+91*self.type_stride+0xec, 0x100000 if case['flyer'] else 0)
        self.put(self.slot(15070)+34, 1, 1)
        self.put(0x541190, 78);self.put(0x5410f8, 34);self.put(0x540bc4, case['enabled']);self.put(0x55a490, case['friendly'])
        self.mu.mem_write(0x595200, bytes(81*4));self.put(0x595200+(1*9+2)*4, case['allied'])
        self.put(0x59ab18, bits(70));self.put(0x59ab1c, bits(20))
        self.put(0x59a92c, bits(16));self.put(0x59a930, bits(11))  # ?? BoardCoord ??? ?? ????.
        self.mu.mem_write(LIST, bytes(80));self.mu.mem_write(DESCRIPTOR, struct.pack('<3I', LIST, case['capacity'], 0))
        self.mu.mem_write(OUTPUT, bytes(16))
        if case['kind']:
            self.call('Notice', [15070], control=control)
        else:
            self.call('Collect', [1, case['x'], case['y'], DESCRIPTOR], control=control)
        count = self.read(DESCRIPTOR+8) if not case['kind'] else 0  # 래퍼의 지역 헤더는 스택에 있으므로 통지를 별도로 읽는다.
        assert count <= case['capacity'] <= 20
        self.returns += 1
        return [','.join(str(self.read(LIST+i*4)) for i in range(count)) or '-', ';'.join(self.events) or '-']


def main():
    """제품 변경 전에 기대값을 분리 고정하거나 기존 고정값의 고유 입력/출처를 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true');parser.add_argument('--smoke', action='store_true');args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 실제 원본·내보내기·도구·기대값의 전체 바이트를 각각 검사한다.
        for name, expected in report['sources'].items():
            assert sha(ROOT/name) == expected, name
        rows = [row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
        assert len(rows) == report['uniqueInputs'] == len({row[0] for row in rows})
        print('gun blocker native verified:', len(rows));return
    assert args.smoke or (not FIXTURE.exists() and not REPORT.exists()), '기존 관측은 덮어쓰지 않는다.'
    oracle = Oracle();rows = [];different = []  # 두 정밀도 실제 결과를 별도 실행한다.
    inputs = cases()[:1] if args.smoke else cases()
    # 제품 코드를 읽거나 호출하지 않고 보호 PE가 반환한 목록과 통지만 저장한다.
    for index, case in enumerate(inputs):
        try:
            outputs = [oracle.run(case, control) for control in FPU_CONTROLS]
        except Exception as error:
            raise RuntimeError(f'입력{index} {case}: {error}') from error
        if outputs[0] != outputs[1]:
            different.append(dict(index=index, secondary=outputs[1]))
        rows.append('\t'.join([','.join(str(case[key]) for key in KEYS), *outputs[0]]))
    if args.smoke:
        print(rows[0]);return
    FIXTURE.write_text('# 독립10.78 실제469590/46ae80 수집·통지. 기존 Form/Kernel/공격모드만 명시 경계다.\n'+'\n'.join(rows)+'\n', encoding='utf-8', newline='\n')
    names = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_gun_blocker.py',
             'tools/decomp_guntarget_oracle.py', 'tools/decomp_rawfinder_oracle.py', 'tools/decomp_bridgeeffects_oracle.py',
             'tools/decomp_bridgedecay_oracle.py', FIXTURE.relative_to(ROOT).as_posix())
    report = dict(host=socket.gethostname(), lastDecompileHost='VM-W11-Codex', lastDecompileDate='2026-10-19',
                  uniqueInputs=len(rows), normalReturns=oracle.returns, precisionDifferences=different,
                  nativeCalls=dict(oracle.native_calls), boundaries=dict(oracle.stub_calls), sources={name: sha(ROOT/name) for name in names},
                  limitations=['독립 해시 입력이며 실제 게임/OS 실행·자연 완주가 아니다.', 'Form/Kernel 조회·현재 공격 모드·할당/해제·SetTarget은 명시 외부 경계다.'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print('gun blocker native:', len(rows), 'inputs;', oracle.returns, 'returns;', len(different), 'precision differences')


if __name__ == '__main__':
    main()
