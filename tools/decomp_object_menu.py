#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 보호 PE의 객체·Damageable·회수 메뉴를 격리 실행한다. 게임/OS 진입은 금지한다."""
import csv
import hashlib
import json
import random
import struct
import sys
from pathlib import Path

# 현재 저장소와 기존 격리 의존성 위치다.
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# PE와 겹치지 않는 실제 의미별 가상 입력 공간이다.
POOL, TYPES, TEXT, VTABLE, CALLBACK, STACK, STOP = 0x12000000, 0x12100000, 0x12200000, 0x12300000, 0x12400000, 0x3001f000, 0x20000000
# 두 판본의 원본 함수/전역 대응이다. 계산은 이 표에 넣지 않는다.
PATCH = dict(stride=50, owner=34, typebytes=500, base=0x4b1270, damage=0x44cbd0, salvage=0x44ca00,
             pool=0x5c8464, local=0x540c70, editor=0x5c85a4, special=0x55a480, charge=0x594fa4,
             deny=0x595078, help=0x54db38, flags=0x4ac200, type=0x4ac550, gettype=0x49a840,
             hp=0x4adc60, refund=0x44c240, translate=0x4de9a0, append=0x4a89e0, format=0x4a89c0,
             items=0x475c50, separator=0x478260, item=0x477cb0, submenu=0x478160, config=0x4414d0,
             name=0x4844f0, player=0x4c9700, growing=0x5412b0, nugget=0x5412c0, suppress=0x541240,
             commands=[0x542a7c, 0x54274c, 0x5429b0, 0x542754, 0x542948, 0x54286c])
CD = dict(stride=36, owner=32, typebytes=468, base=0x4ae780, damage=0x462a70, salvage=0x4628a0,
          pool=0x5395dc, local=0x50f6c8, editor=0x518904, special=0x537e8c, charge=0x52e170,
          deny=0x518678, help=0x52e8cc, flags=0x4abae0, type=0x4abf50, gettype=0, types=0x51c960,
          hp=0x4aecf0, refund=0x462610, translate=0x4bcb20, append=0x420590, format=0x420560,
          items=0x4510c0, separator=0x4512e0, item=0x451100, submenu=0x451270, config=0x4a9930,
          name=0x4019e0, player=0x4c4cd0, growing=0x51cb9c, nugget=0x51cbac,
          commands=[0x512c04, 0x5128d4, 0x512b38, 0x5128dc, 0x512ad0, 0x5129f4])


def digest(path):
    """근거 파일의 실제 바이트 SHA를 읽는다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


class Oracle:
    """세 메뉴 몸체만 실행하고 외부 조회·GUI·설정 효과를 순서대로 관찰한다."""
    def __init__(self, edition):
        """보호 PE를 메모리 사본으로만 읽고 허용 범위를 실제 정밀 함수 표에서 얻는다."""
        self.edition = edition  # 현재 실제 PE 판본의 식별자다.
        self.p = PATCH if edition == 'originals' else CD  # 같은 판본의 함수/전역 대응이다.
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # 게임/OS와 분리된32비트 실행기다.
        path = 'originals/Netstorm.exe' if edition == 'originals' else 'originalCD/NETSTORM.EXE'
        data = pefile.PE(str(ROOT / path)).get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(data) + 4095) & ~4095)
        self.mu.mem_write(0x400000, data)
        # 명시 저장소를 서로 겹치지 않는 페이지에 만든다.
        for address, size in [(POOL, 0x10000), (TYPES, 0x20000), (TEXT, 0x10000), (VTABLE, 0x1000), (CALLBACK, 0x1000), (STACK - 0x1f000, 0x20000), (STOP, 0x1000)]:
            self.mu.mem_map(address, size)
        with (ROOT / f'extracted/refined/{edition}/functions.tsv').open(encoding='utf-8') as fp:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(fp, delimiter='\t')}
        self.allowed = []  # 실제 세 함수의 실행 허용 주소 구간이다.
        # 실제 세 함수의 불연속 범위도 포함한다.
        for name in ['base', 'damage', 'salvage']:
            for part in facts[self.p[name]]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))
        self.instructions = 0  # 명시 경계를 제외하고 실행한 실제 명령 수다.
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)

    def put(self, address, value, width=4):
        """독립 입력 또는 명시 콜백 효과를 little endian 폭으로 쓴다."""
        self.mu.mem_write(address, (value & ((1 << (width * 8)) - 1)).to_bytes(width, 'little'))

    def word(self, address):
        """현재DWORD를 해석한다."""
        return int.from_bytes(self.mu.mem_read(address, 4), 'little')

    def string(self, address):
        """바이트 문자열을 NUL까지 읽고 무한 조회를 거부한다."""
        text = bytearray()
        # 원본 서식의 정상 입력만 허용한다.
        for index in range(4096):
            byte = self.mu.mem_read(address + index, 1)[0]
            if not byte:
                return bytes(text)
            text.append(byte)
        raise ValueError('객체 메뉴 NUL 없는 문자열')

    def emit(self, *values):
        """실제 외부 효과를 계산 없이 정규화한다."""
        self.events.append(','.join(map(str, values)))

    def ret(self, result=0, purge=0):
        """실제 cdecl/thiscall 경계의 반환과 스택 정리를 적용한다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, result & 0xffffffff)
        self.mu.reg_write(UC_X86_REG_EIP, self.word(esp))
        self.mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge)

    def instruction(self, mu, address, size, data):
        """원본 분기/인자 조립은 실행하고 의존 몸체만 명시 경계로 돌려준다."""
        p = self.p
        esp, ecx = mu.reg_read(UC_X86_REG_ESP), mu.reg_read(UC_X86_REG_ECX)
        args = [self.word(esp + 4 + index * 4) for index in range(6)]
        if address == CALLBACK:
            self.emit('L', 50)
            if self.mutation & 1:
                self.put(self.raw + 10, 122, 1)
            self.ret(self.count)
        elif address == p['hp']:
            self.emit('H', 50);self.ret(self.hp)
        elif address == p['flags']:
            self.ret(self.word(TYPES + mu.mem_read(ecx + 10, 1)[0] * p['typebytes'] + 0xec))
        elif address == p['type']:
            self.ret(TYPES + mu.mem_read(ecx + 10, 1)[0] * p['typebytes'])
        elif address == p.get('gettype'):
            self.ret(TYPES + args[0] * p['typebytes'])
        elif address == p['refund']:
            self.emit('R', *args[:3])
            if self.mutation & 2:
                self.put(p['deny'], 1)
            self.ret(self.refund)
        elif address == p['translate']:
            self.emit('T', self.string(args[0]).hex())
            if self.mutation & 16:
                self.put(TYPES + self.passed * p['typebytes'] + 0xec, 0x200)
            self.ret(args[0], 4)
        elif address in (p['append'], p['format']):
            pattern = self.string(args[1]).decode('ascii')
            text = pattern % struct.unpack('<i', struct.pack('<I', args[2]))
            if address == p['append']:
                text = self.string(args[0]).decode('ascii') + text
            mu.mem_write(args[0], text.encode('ascii') + b'\0');self.ret(args[0])
        elif address == p['items']:
            self.emit('Q')
            if self.mutation & 32:
                self.put(TYPES + self.passed * p['typebytes'] + 0xec, 0x4000)
            self.ret(self.items)
        elif address == p['separator']:
            self.emit('X')
            if self.mutation & 4:
                self.put(p['deny'], 0)
            self.ret()
        elif address == p['config']:
            self.emit('C', self.string(args[0]).hex(), self.string(args[1]).hex())
            if self.mutation & 8:
                self.put(self.raw + 10, 122, 1)
            self.ret()
        elif address == p['item']:
            value = 'N:' + str(args[2]) if args[2] < 0x10000 else 'S:' + self.string(args[2]).hex()
            assert args[3:5] == [1, 1]
            if self.edition == 'originals':
                assert args[5] == 0
            self.emit('I', self.string(args[0]).hex(), args[1], value);self.ret()
        elif address == p['name']:
            self.emit('N', args[0]);mu.mem_write(TEXT + 256, f'Player{args[0]}'.encode() + b'\0');self.ret(TEXT + 256)
        elif address == p['player']:
            assert args[1:3] == [1, 0]
            self.emit('P', self.string(args[0]).hex(), 1, 0);self.ret(123)
        elif address == p['submenu']:
            assert args[2:4] == [1, 0]
            self.emit('S', self.string(args[0]).hex(), args[1]);self.ret()
        else:
            assert any(low <= address and address + size <= high for low, high in self.allowed), f'{self.edition}: 미등록 {address:08x}'
            self.instructions += 1

    def run(self, kind, values, precision):
        """독립 입력을 설정하고 원본 정상 반환/레지스터/제어어와 전체 외부 순서를 돌려준다."""
        owner, local, rawtype, self.passed, modes, deny, currentflags, passedflags, cost, self.refund, self.count, self.items, self.hp, self.mutation = values
        p = self.p
        self.events = []
        self.raw = POOL + 50 * p['stride']
        self.mu.mem_write(POOL, bytes(0x10000));self.mu.mem_write(TYPES, bytes(0x20000))
        self.put(p['pool'], POOL);self.put(p['local'], local);self.put(p['help'], TEXT)
        self.mu.mem_write(TEXT, b'AboutType\0')
        if 'types' in p:
            self.put(p['types'], TYPES)
        # 실제 원본 전역의 독립 입력과 모드를 넣는다.
        for key, value in [('editor', modes & 1), ('special', modes & 2), ('charge', modes & 4), ('deny', deny)]:
            self.put(p[key], value)
        self.put(self.raw, VTABLE);self.put(VTABLE + 0x80, CALLBACK)
        self.put(self.raw + 10, rawtype, 1);self.put(self.raw + p['owner'], owner, 1)
        # count 콜백의 타입 변경도 실제로 유효한 TYPE를 가리킨다.
        for number in range(188 if self.edition == 'originals' else 171):
            self.put(TYPES + number * p['typebytes'] + 0xc4, 0x3f800000)
        self.put(TYPES + rawtype * p['typebytes'] + 0xec, currentflags)
        self.put(TYPES + rawtype * p['typebytes'] + 0xc4, cost)
        self.put(TYPES + self.passed * p['typebytes'] + 0xec, passedflags)
        saved = {UC_X86_REG_EBX: 0x123456, UC_X86_REG_ESI: 0x234567, UC_X86_REG_EDI: 0x345678, UC_X86_REG_EBP: 0x456789}
        # 원본 callee 보존 레지스터를 오염 표식으로 설정한다.
        for register, value in saved.items():
            self.mu.reg_write(register, value)
        control = 0x027f if precision == 53 else 0x037f
        self.mu.reg_write(UC_X86_REG_FPCW, control);self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.reg_write(UC_X86_REG_ECX, self.raw)
        args = [50, self.passed, self.count] if kind == 'salvage' else []
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * (1 + len(args)), STOP, *args))
        self.mu.emu_start(p[kind], STOP, count=10000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert all(self.mu.reg_read(register) == value for register, value in saved.items())
        assert self.mu.reg_read(UC_X86_REG_FPCW) == control
        return ';'.join(self.events) or '-'


def cases():
    """예상 메뉴를 계산하지 않는 고정 경계/교차/재진입 입력을 만든다."""
    base = (1, 1, 158, 123, 0, 0, 0, 0, 0x3f800000, 45, 3, 1, 1, 0)
    result = [base]
    # 소유·타입·모드·flags·부호 있는 비용/환불·콜백 시점 경계를 독립 변경한다.
    choices = {0: [0, 2, 8], 1: [0, 2], 2: [122, 150, 154], 3: [122, 150, 154, 158], 4: range(8), 5: [1, 0xffffffff], 6: [0x200, 0x4000, 0x4200], 7: [0x200, 0x4000, 0x4200], 8: [0, 0x80000000, 0x7fc00000], 9: [0, -1, -2147483648, 2147483647], 10: [0, 0xffffffff], 11: [0, 3], 12: [0], 13: [1, 2, 4, 8, 16, 32, 63]}
    for column, options in choices.items():
        for value in options:
            row = list(base);row[column] = value;result.append(tuple(row))
    rng = random.Random(0x4b1270)
    # 세 몸체 모두 같은 독립 상태 조합을 받는다.
    for index in range(192):
        result.append((rng.choice([0, 1, 2]), 1, rng.choice([122, 150, 154, 158]), rng.choice([122, 123, 154, 158]), rng.randrange(8), rng.randrange(2), rng.choice([0, 0x200, 0x4000, 0x4200]), rng.choice([0, 0x200, 0x4000, 0x4200]), rng.choice([0, 0x80000000, 0x3f800000, 0x7fc00000]), rng.choice([-2147483648, -1, 0, 45, 2147483647]), rng.randrange(8), rng.randrange(2), rng.randrange(2), rng.randrange(64)))
    return list(dict.fromkeys(result))


def main():
    """원문을 실행한 새 fixture 또는 기존 새 관찰의 SHA/고유 입력을 감사한다."""
    reportpath = ROOT / 'cpppj/object-menu.json'
    fixture = ROOT / 'cpppj/tests/fixtures/object-menu-x86.tsv'
    if '--verify' in sys.argv:
        report = json.loads(reportpath.read_text(encoding='utf-8'))
        for path, expected in report['sha256'].items():
            assert digest(path) == expected, f'객체 메뉴 SHA 불일치: {path}'
        # 제품 검증 뒤 추가된 실제 소스 핀도 원문 관찰 핀과 구별해 감사한다.
        for path, expected in report.get('product_validation', {}).get('sha256', {}).items():
            assert digest(path) == expected, f'객체 메뉴 제품 SHA 불일치: {path}'
        with fixture.open(encoding='utf-8') as fp:
            rows = list(csv.reader(fp, delimiter='\t'))[1:]
        assert len(rows) == report['observations'] == len({tuple(row[:-1]) for row in rows})
        print(json.dumps({'verified': True, 'observations': len(rows)}));return
    rows, instructions = [], 0
    # 두 판본과 두 x87 정밀도를 각각 실제 명령으로 실행한다.
    for edition in ['originals', 'originalCD']:
        oracle = Oracle(edition)
        for kind in ['base', 'damage', 'salvage']:
            for values in cases():
                trace = oracle.run(kind, values, 53)
                assert trace == oracle.run(kind, values, 64), (edition, kind, values)
                rows.append([edition, kind, *values, trace])
        instructions += oracle.instructions
    with fixture.open('w', encoding='utf-8', newline='') as fp:
        writer = csv.writer(fp, delimiter='\t', lineterminator='\n')
        writer.writerow(['edition', 'kind', 'owner', 'local', 'rawtype', 'type', 'modes', 'deny', 'currentflags', 'passedflags', 'costbits', 'refund', 'count', 'items', 'hp', 'mutation', 'native_trace'])
        writer.writerows(rows)
    paths = ['originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'extracted/refined/originals/functions.tsv', 'extracted/refined/originalCD/functions.tsv', 'tools/decomp_object_menu.py', 'cpppj/tests/fixtures/object-menu-x86.tsv']
    report = dict(host='HJOW-Athlon', date='2026-10-17', last_full_decompile_host='HJOW-Athlon', observations=len(rows), normal_returns=len(rows) * 2, precisions=[53, 64], precision_differences=0, instructions=instructions, os_calls=0,
                  entries={'originals': {key: f'{PATCH[key]:08x}' for key in ['base', 'damage', 'salvage']}, 'originalCD': {key: f'{CD[key]:08x}' for key in ['base', 'damage', 'salvage']}},
                  boundaries=['TYPE/HP/가상 개수·환불 조회', '번역·CRT 고정 정수 서식', 'controlled 메뉴 작성·Player 하위 메뉴', '현재 설정 쓰기'],
                  limits=['세 메뉴 몸체 전체 실행이며 환불/Player 메뉴/GUI 픽셀 몸체의 관찰은 아님', '게임/OS 실행 없음', 'Carrier·Priest·실제 Session·건설 연결은 별도'], sha256={path: digest(path) for path in paths})
    reportpath.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'observations': len(rows), 'normal_returns': len(rows) * 2, 'instructions': instructions}))


if __name__ == '__main__':
    main()
