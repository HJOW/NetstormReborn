#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CreateAt 실제 parser/전체 부모/지면/HP·타입frame 쓰기를 work에만 최초 관측한다."""
from pathlib import Path
import csv
import itertools
import json
import hashlib
import socket
import struct
import sys

ROOT = Path(r'H:\T5Workspace\NetstormReborn')  # 보호 PE/원문은 읽기만 한다.
WORK = Path(__file__).resolve().parent  # 새 기대값/보고서만 작성한다.
sys.path.insert(0, str(ROOT / 'tools'))
from decomp_destroy_at import ParserOracle, TEXT, THREAD, POOL, TABLE, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

TYPES, MAP = 0x12100000, 0x12010000  # 실제 TYPE 필드/첫프레임/개수와 단순 지면 WORD 입력이다.
OBJECT = POOL + 50  # 명시 new가 반환하는 하나의 독립 합성 객체다.
OWNER, POP = STOP + 0x200, STOP + 0x210  # 실제 가상+74/+90의 효과만 명시한다.
NAMES = {b'': 0, b'suncannon': 131, b'provider': 76, b'special2': 129, b'special3': 140, b'special4': 142}  # 실제 타입 조회 대신 공급하는 독립 lookup 경계다.


class NullObject(Exception):
    """원문 NULL 생성 객체 첫 역참조 직전의 prefix를 구별한다. 정상 생성 성공이 아니다."""


def frame_codes(reverse):
    """프레임 표의 입력 코드만 만든다. 첫프레임/개수 기대값은 실제49b060이 작성한다."""
    result = bytearray()
    for side in (range(15, -1, -1) if reverse else range(16)):  #현재TYPE가 바뀐 뒤에도 메타데이터 getter를 다시 읽는 입력이다.
        for number in range(1 + side % 3):
            result.extend((65 + side, 65, number, 0))
    return bytes(result)


class Oracle(ParserOracle):
    """기존 정확 parser에 실제 생성 부모/지면/HP·TYPE 메타데이터 몸체만 추가한다."""
    def __init__(self):
        """파서의 CRT/TLS 경계와 작은 독립 합성 객체만 준비하며 게임/OS는 시작하지 않는다."""
        super().__init__()
        self.mu.mem_map(POOL, 0x40000)
        self.mu.mem_map(TYPES, 0x40000)
        self.entries.update(CreateAt=0x44c660, BuildFrames=0x49b060)
        facts = {int(r['entry'], 16): r for r in csv.DictReader((ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8'), delimiter='\t')}
        for entry in (0x44c660, 0x40da00, 0x4adc60, 0x4adcb0, 0x4adca0, 0x425830, 0x4ac550, 0x49fce0, 0x49b060):  #선택된 실제 하위의 불연속 몸체만 더한다.
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(v, 16) for v in part.split('-'))
                self.allowed.append((low, high + 1))
        self.word(TABLE + 0x74, OWNER)
        self.word(TABLE + 0x90, POP)
        self.calls = {}

    def write(self, mu, access, address, size, value, data):
        """원문 지역/CRT와 실제HP/frame/type메타데이터 쓰기만 허용한다."""
        if any(low <= address and address + size <= high for low, high in ((0x30000000, 0x30010000), (THREAD, THREAD + 0x100), (OBJECT + 26, OBJECT + 30), (OBJECT + 36, OBJECT + 40), (TYPES, TYPES + 0x40000))):
            return
        raise RuntimeError(f'CreateAt 미명시 쓰기 {address:08x}/{size}')

    def on_instruction(self, mu, address, size, data):
        """계산은 원문에 맡기고 생성/가상효과/lookup만 독립 경계로 기록한다."""
        if address in (0x44c660, 0x44c540, 0x40da00, 0x4adc60, 0x4adcb0, 0x4adca0, 0x425830, 0x4ac550, 0x49fce0):
            key=f'{address:08x}'
            self.calls[key] = self.calls.get(key, 0) + 1
        if address == 0x4e0620:
            self.event('assert', self.args(3)[2])
            if self.boss == 2:
                self.word(0x540bc4, 1)
            self.returned()
            return
        if address == 0x49a860:
            name = self.read_string(self.args(1)[0])
            self.event('type', name.hex() or '-')
            self.returned(NAMES.get(name.lower(), 0xffffffff))
            return
        if address == 0x49a840:
            self.returned(TYPES + self.args(1)[0] * 500)
            return
        if address == 0x4af530:
            typ, flags = self.args(2)
            self.event('create', typ, flags)
            if not self.null:
                raw = bytearray(50)
                struct.pack_into('<I', raw, 0, TABLE)
                raw[10] = typ
                raw[11] = 4
                struct.pack_into('<I', raw, 36, 0xdeadbeef)
                mu.mem_write(OBJECT, bytes(raw))
            self.returned(0 if self.null else OBJECT)
            return
        if address == 0x40da00:
            self.event('surface', *self.args(2))
        if address == 0x44c713 and self.null:
            self.event('NULL')
            raise NullObject()
        if address == OWNER:
            value = self.args(1)[0]
            self.event('owner', value)
            mu.mem_write(OBJECT + 34, bytes([value & 255]))
            if self.mutation == 1:
                mu.mem_write(OBJECT + 10, b'\x4c')
            if self.mutation == 2:
                typ = mu.mem_read(OBJECT + 10, 1)[0]
                self.word(TYPES + typ * 500, 701)
            self.returned(purge=4)
            return
        if address == 0x4adc60:
            self.event('hasHp')
        if address == 0x4adcb0:
            self.event('maxHp')
        if address == 0x4adca0:
            self.event('hp', self.args(1)[0])
        if address == 0x425830:
            self.event('directional', self.args(1)[0])
        if address == 0x44c754:
            offset = (0x15c + self.word_register(UC_X86_REG_EAX) * 4) & 0xffffffff
            self.event('frameField', offset)
        if address == POP:
            self.event('pop', *self.args(3))
            mu.mem_write(OBJECT + 14, struct.pack('<2I', *self.args(2)))
            self.returned(purge=12)
            return
        super().on_instruction(mu, address, size, data)

    def word_register(self, register):
        """현재 실제32BIT 레지스터의 값만 읽는다."""
        return self.mu.reg_read(register)

    def sample(self, case, precision):
        """독립 source/권한/HP/지면/가상변이/NULL 입력에서 실제 부모를 실행한다."""
        text, self.boss, hp, terrain, self.mutation, self.null = case
        self.trace = []
        self.mu.mem_write(OBJECT, bytes([0xcd]) * 50)
        self.word(0x540bc4, int(self.boss == 1))
        self.word(0x5c8464, POOL)
        self.word(0x5c847c, 4)
        self.word(0x5c84bc, MAP)
        self.word(0x59ab20, TYPES)
        self.word(0x5e4794, 0)
        self.word(0x5ca8f0, 0)
        self.mu.mem_write(MAP, bytes(0x20000))
        if terrain:
            self.mu.mem_write(MAP + (21 * 256 + 20) * 2, struct.pack('<H', 2))
        self.mu.mem_write(POOL + 2 * 50 + 34, b'\x03')
        for index, typ in enumerate((76, 129, 131, 140, 142)):  #모두 독립 메타데이터이며 실제 게임 생성/공격/MP 대상이 아니다.
            pointer = TYPES + typ * 500
            self.mu.mem_write(pointer, bytes(500))
            self.word(pointer, 203 if typ == 76 else 503)
            self.word(pointer + 0xe8, 0x10 if hp else 0)
            code = frame_codes(typ == 76)
            code_address = TYPES + 0x30000 + index * 0x400
            self.mu.mem_write(code_address, code)
            self.word(pointer + 0x114, len(code) // 4)
            self.word(pointer + 0x124, code_address)
            self.call('BuildFrames', [], pointer)
        self.trace = []
        self.mu.mem_write(TEXT, text.encode('ascii') + b'\0')
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        outcome = 'ok'
        try:
            self.call('CreateAt', [TEXT])
        except NullObject:
            outcome = 'null'
        assert self.mu.reg_read(UC_X86_REG_FPCW) == precision and not self.mu.reg_read(UC_X86_REG_FPSW) & 0x3800
        return '\t'.join((outcome, ';'.join(self.trace) or '-', bytes(self.mu.mem_read(OBJECT, 50)).hex()))


def main():
    """최초 원문 결과를 새 work에만 기록하며 기존 fixture/제품/빌드는0이다."""
    global WORK  # 독립 출력 경로에서만 최초 fixture를 작성한다.
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)  # 제품과 기존 fixture는 덮어쓰지 않는다.
    parser.add_argument('--output',required=True,type=Path)
    WORK=parser.parse_args().output.resolve()
    WORK.mkdir(parents=True,exist_ok=True)
    fixture = WORK / 'modified/cpppj/tests/fixtures/create-at-parent-x86.tsv'
    report = WORK / 'native.json'
    assert not fixture.exists() and not report.exists()
    texts = ['', '(0x21)provider/0#1', '(-1x21)provider#1', '(20x0)provider#1', '(20x21)missing#1', '(20x21)provider/9#0', '(20x21)provider/2#1', '(20x21)sunCannon/0#1', '(20x21)sunCannon/3#0', '(20x21)special2/-3#2', '(20x21)special3/10#1', '(20x21)special4/41#1', '(16777217x2147483647)provider#1', '(20x21)provider/1#-1', '(20x21)"sunCannon"/-24#1', '(20x21)provider#1\0trailing']
    cases = [(text, boss, hp, terrain, mutation, 0) for text, boss, hp, terrain, mutation in itertools.product(texts, (0, 1, 2), (0, 1), (0, 1), (0, 1, 2))]
    cases += [('(20x21)provider#' + str(owner), 1, 1, terrain, 0, 1) for owner, terrain in itertools.product((0, 1), (0, 1))]
    oracle = Oracle()
    lines = ['# source,boss,hp,terrain,mutation,NULL — outcome,ordered effects,current50BYTE; actual parser/parent/HP/meta, lookup/new/virtual effects are explicit boundaries']
    for case in cases:  #기대 분기/정수/프레임/float 비트는 원문 CPU에서만 읽는다.
        first = oracle.sample(case, 0x27f)
        assert first == oracle.sample(case, 0x37f), case
        lines.append(json.dumps(case, separators=(',', ':')) + '\t' + first)
    fixture.parent.mkdir(parents=True, exist_ok=True)
    fixture.write_text('\n'.join(lines) + '\n', 'utf-8', newline='\n')
    record = dict(unit=770, host=socket.gethostname(), lastDecompile='HJOW-Athlon / 2026-10-23', inputs=len(cases), observations=len(cases) * 2, actual='0044c660 + actual0044c540/CRT token/atol +0040da00/004adc60/004adcb0/004adca0/00425830/004ac550/0049fce0/0049b060', boundary='TYPE name/getter, new and virtual owner/Pop, TLS/assert are explicit independent boundaries; synthetic raw TYPE/objects only; NULL stops before first dereference, no actual allocator/virtual World/MP/combat/OS/SEH', calls=oracle.calls, gameExecutions=0, multiplayerTests=0, builds=0, fixtureSha256=hashlib.sha256(fixture.read_bytes()).hexdigest())
    report.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', 'utf-8')
    print(json.dumps(dict(inputs=len(cases), observations=len(cases) * 2)))


if __name__ == '__main__':
    main()
