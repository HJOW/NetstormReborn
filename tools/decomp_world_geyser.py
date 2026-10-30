#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세계 간헐천 부모00486440의 원본 분기·인수·현재값 재조회·난수를 격리 관측한다."""
import argparse
import csv
import json
import socket
import struct
import decomp_priestmenu as base
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

ROOT = base.ROOT  # 보호 PE와 현재 PC의 정밀 산출물만 읽는다.
KEYS = ('battle', 'present', 'hidden', 'size', 'density', 'placement', 'decoration', 'predictable', 'time', 'alter', 'consume')  # 모두 외부 경계 전에 공급하는 원시 입력이다.
TYPE_ADDRESSES = (0x5411e8, 0x541338, 0x5411ec, 0x54133c)  # treeTwo/RUIN/treeThree/Monument의 실제 전역 주소다.


def double_bits(value):
    """시각 입력을 십진 변환 오차 없이 IEEE64 DWORD 둘로 보존한다."""
    return struct.unpack('<Q', struct.pack('<d', value))[0]


def case(**changes):
    """부모가 읽는 현재 전역/하위 응답의 기본 입력만 만든다."""
    result = dict(zip(KEYS, (1, 15, 0, 2, 1, 2, 0, 1, double_bits(12.25), 0, 0)))
    result.update(changes);return result


def cases():
    """예산/간격/밀도/모드와 장식/시드 및 하위가 바꾼 현재값을 독립 입력한다."""
    result = []
    for present in (0, 1, 3, 15, 255):  # 실제 Player+2c의0~8명 예산을 선택한다.
        for size in range(4):  # 실제 네 간격의 보정 계수를 구별한다.
            for density in range(4):  # -1/0/1/2의 실제 밀도 getter다.
                for placement in (0, 1, 2, 3):  # 미선택과 두 부모 분기 및 다른 BYTE를 포함한다.
                    result.append(case(present=present, size=size, density=density, placement=placement))
    for decoration in (1, 2, 3):  # 여섯 타입 가중치/Monument 상한10/현재 반복상한이다.
        for predictable in (0, 1):  # 세계 시드의 고정/시각 선택은 장식 시드와 독립이다.
            result.append(case(decoration=decoration, predictable=predictable, consume=1))
    for alter in range(1, 11):  # 하위 뒤 옵션·Player·타입·demo 재조회를 구별한다.
        for placement in (1, 2):
            result.append(case(placement=placement, decoration=2 if alter in (2, 6, 7) else 0, alter=alter, consume=1))
    for time in (-0.125, 0.0, 1000000.125):  # 시각의 음수/0/큰 값과 독립 두 local 시드를 보존한다.
        result.append(case(time=double_bits(time), decoration=1, predictable=0, consume=1))
    for hidden in (0x55, 0xff):  # 숨김과 전투 밖0 예산도 실제 getter를 실행한다.
        for battle in (0, 1): result.append(case(battle=battle, present=255, hidden=hidden, decoration=1))
    result.append(case(battle=0, alter=1))  # Local 뒤 현재 battle 재조회를 따로 구별한다.
    return list({tuple(item[key] for key in KEYS): item for item in result}.values())


class Oracle(base.Oracle):
    """부모/옵션/예산/PRNG/시각 seed/CRT는 실제 명령이며 큰 세계 효과만 경계다."""
    def __init__(self):
        """원본 이미지와 별도 Guide/Player 및 승인 함수 범위를 설치한다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []
        for entry in (0x486440, 0x41c8e0, 0x41ca10, 0x41ca40, 0x463b50, 0x4bcab0, 0x4bca20, 0x4e49c0):  # 실제 계산 몸체만 CPU에서 실행한다.
            for part in facts[entry]['ranges'].split(';'):  # 분리된 함수 조각은 현재 정밀 표를 따른다.
                low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high+1))

    def get(self, address, width=4):
        """현재 원문 폭의 raw 정수만 읽는다."""
        return int.from_bytes(self.mu.mem_read(address, width), 'little')

    def instruction(self, mu, address, size, data):
        """실제 호출 인수/순서와 명시 하위의 상태 변경만 기록한다."""
        esp = mu.reg_read(UC_X86_REG_ESP);args = self.words(esp+4, 9)
        if address == 0x4a68e0:
            assert self.string(args[0]) == b'Make Local Geysers\n';self.events.append('Log');self.ret();return
        if address == 0x486360:
            self.events.append('Local')
            if self.values['alter'] == 1: self.put(0x594fbc, 1);self.put(0x52f7e9, 1, 1)
            if self.values['alter'] == 7:
                for target, value in zip(TYPE_ADDRESSES, (101, 184, 100, 185)): self.put(target, value)  # Local 뒤 타입 snapshot 입력이다.
            self.ret();return
        if address == 0x441470:
            assert self.string(args[0]) == b'predictableGeysers' and args[1] == 1
            self.events.append('Config')
            if self.values['alter'] == 8: self.put(0x52f7f2, 1, 1);self.put(0x52f7f1, 3, 1);self.put(0x52f7e6, 0, 1)
            self.ret(self.values['predictable']);return
        if address == 0x460d70:
            self.events.append('Time');value = struct.unpack('<d', struct.pack('<Q', self.values['time']))[0]+self.time_calls*0.125;self.time_calls += 1
            mu.mem_write(args[0], struct.pack('<d', value));self.ret(args[0]);return
        if address in (0x485bc0, 0x485cc0):
            if address == 0x485bc0: self.events.append('Whole');rectangle = (16.25, 24.5, 240.75, 232.25)
            else: self.events.append(f'Part:{args[1]}:{args[2]}');rectangle = (args[1]*10.25, 24.5, args[1]*10.25+32.5, 72.25)
            mu.mem_write(args[0], struct.pack('<4f', *rectangle));self.ret(args[0]);return
        if address in (0x40ded0, 0x486130):
            seed = self.get(args[0] if address == 0x40ded0 else args[1]);rectangle_at = esp+8 if address == 0x40ded0 else esp+12
            rectangle = mu.mem_read(rectangle_at, 16).hex()
            if address == 0x40ded0:
                self.events.append(f'Decor:{seed}:{rectangle}:'+','.join(map(str,args[5:9])));self.decor_calls += 1
                if self.values['alter'] == 2 and self.decor_calls == 1: self.put(0x52f7e9, 0, 1)
                if self.values['alter'] == 6 and self.decor_calls == 1: self.put(0x54133c, 100)
                random_address = args[0]
            else:
                self.events.append(f'Pop:{args[0]}:{seed}:{rectangle}:'+','.join(map(str,args[6:9])));self.pop_calls += 1
                if self.values['alter'] == 3 and self.pop_calls == 1: self.put(0x52f7f2, 1, 1);self.put(0x52f7f1, 3, 1)
                if self.values['alter'] == 4 and self.pop_calls == 1: self.put(base.PLAYERS+2*0xb4+0x2c, 0);self.put(0x52f81f, 1, 1)
                random_address = args[1]
            if self.values['consume']: self.put(random_address, self.get(random_address)+0x10203)
            self.ret();return
        if address == 0x41c8e0: self.events.append('Budget')
        elif address == 0x41ca10: self.events.append('Spacing')
        elif address == 0x41ca40:
            self.events.append('Density');self.density_calls += 1
            if self.values['alter'] == 10: self.put(0x52f7f1, (1,1,2,0,3)[min(self.density_calls-1,4)], 1)
        elif address == 0x463b50:
            self.events.append('Demo');self.demo_calls += 1
            if self.values['alter'] == 5: self.put(base.TEXT+0x68, 1 if self.demo_calls == 1 else 0)
            elif self.values['alter'] == 9: self.put(base.TEXT+0x68, 1)
        assert any(low<=address and address+size<=high for low, high in self.allowed), f'미등록 명령 {address:08x}'
        self.instructions += 1

    def observe(self, values):
        """기대 부모를 계산하지 않고 원본이 남긴 전체 상태와 효과 순서를 읽는다."""
        self.values, self.events = values, [];self.time_calls = self.decor_calls = self.pop_calls = self.density_calls = self.demo_calls = 0
        self.put(0x594fbc, values['battle']);self.put(0x59534c, base.PLAYERS);self.put(0x5634a0, base.TEXT);self.put(base.TEXT+0x68, 0)
        self.mu.mem_write(base.PLAYERS, bytes(1620));self.mu.mem_write(0x52f7e0, bytes(87))
        for target, value in zip(TYPE_ADDRESSES, (100, 184, 101, 185)): self.put(target, value)  # 실제 PE 초기 타입이며 다른 snapshot 사례는 Local에서 바꾼다.
        for index, key in ((6, 'size'), (9, 'decoration'), (17, 'density'), (18, 'placement')): self.put(0x52f7e0+index, values[key], 1)  # 실제 옵션의 원문 위치다.
        for owner in range(1, 9):  # 실제 예산이 읽는 Player+2c와 숨김BYTE만 입력한다.
            self.put(base.PLAYERS+owner*0xb4+0x2c, 1 if values['present'] & (1<<(owner-1)) else 0)
            self.put(0x52f81e+owner, 128 if values['hidden'] & (1<<(owner-1)) else 0, 1)
        self.call(0x486440)
        return [';'.join(self.events), self.mu.mem_read(0x52f7e0,87).hex(), self.mu.mem_read(base.PLAYERS,1620).hex(), ','.join(str(self.get(address)) for address in TYPE_ADDRESSES), str(self.get(base.TEXT+0x68))]


def main():
    """독립 두 emulator의 최초 관측만 fixture로 고정한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--output', required=True)
    parser.add_argument('--verify', action='store_true')  # 최초 fixture를 변경하지 않고 현재 도구로 두 관측을 다시 대조한다.
    args = parser.parse_args()
    output = ROOT / args.output;output.mkdir(parents=True, exist_ok=False)
    values = cases();first, replay = Oracle(), Oracle()
    observed = [[str(value[key]) for key in KEYS]+first.observe(value) for value in values]
    repeated = [[str(value[key]) for key in KEYS]+replay.observe(value) for value in values]
    for name, result in (('first.json', observed), ('replay.json', repeated)): (output / name).write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8', newline='\n')  # 최초 두 출력을 별도로 보존한다.
    assert observed == repeated
    fixture = ROOT / 'cpppj/tests/fixtures/world-geyser-x86.tsv'
    payload = '# '+'/'.join(KEYS)+'/원문출력5열\n'+''.join('\t'.join(row)+'\n' for row in observed)
    if args.verify: assert fixture.read_bytes() == payload.encode('utf-8')
    else:
        with fixture.open('xb') as stream: stream.write(payload.encode('utf-8'))
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'extracted/refined/originals/Netstorm.c', 'tools/decomp_priestmenu.py', 'tools/decomp_world_geyser.py', 'cpppj/tests/fixtures/world-geyser-x86.tsv']
    report = {'unit':609,'host':socket.gethostname(),'cases':len(values),'observations':2*len(values),'differences':[],'verificationOnly':args.verify,'instructions':[first.instructions,replay.instructions], 'scope':'00486440/0041c8e0/0041ca10/0041ca40/00463b50/004bcab0/004bca20/실제CRT. Local/전체·분할범위/장식·일반배치/Config·벽시각은 명시 경계. 새 게임/GUI/MP 없음.', 'sourceSha256':{name:base.digest(ROOT/name) for name in paths},'evidenceSha256':{path.name:base.digest(path) for path in output.iterdir() if path.is_file()}}
    (ROOT / 'cpppj/world-geyser-native-20261030.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(report,ensure_ascii=False))


if __name__ == '__main__':
    main()
