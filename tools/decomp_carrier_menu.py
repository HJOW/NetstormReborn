#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 원본 Carrier 메뉴와 비용/상태 helper를 격리 실행한다. 게임/OS는 실행하지 않는다."""
import csv
import hashlib
import json
import random
import re
import struct
import sys
from pathlib import Path
from decomp_object_menu import Oracle as CommonOracle, ROOT, POOL, TYPES, TEXT, STACK, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# 판본별 실제 몸체/큰 의존 경계의 주소다. 비용은 실제 ftol까지 실행한다.
PATCH = dict(menu=0x4288f0, flags1=0x4ac1e0, ground=0x492090, begin=0x4b2270, next=0x4b1b20,
             description=0x49af80, movement=0x488fe0, lookup=0x4b1a90, active=0x4ae2d0,
             point=0x426a70, money=0x40ec50, event=0x4afb40, header=0x478020, sprintf=0x4e58e9,
             limit=0x531928, extra=40, helpers=[0x4e49c0, 0x491af0])
CD = dict(menu=0x4e5180, flags1=0x4abac0, ground=0x40d7b0, begin=0x4eb8d0, next=0x4eb900,
          description=0x444cb0, movement=0x47d020, lookup=0x4eb310, active=0x4af4e0,
          point=0x4e46f0, event=0x4ac890, header=0x451210, sprintf=0x4f2830,
          limit=0x52e9a8, extra=35, helpers=[0x4e5150, 0x4e7610, 0x4e3500, 0x4147a0, 0x414620, 0x4f161c])
# 독립 입력22열의 원본 의미다.
OWNER, LOCAL, RAWTYPE, FLAGS1, FLAGS2, EXTRA, GROUND, PATH, ACTIVE, COUNT, CONTENTFLAGS, CONTENTTYPE, X, Y, LIMIT, COST, CDCOST, SP, EDITOR, BLOCKED, BUILDING, MUTATION = range(22)


def bits(value):
    """좌표/SP를 재현 가능한 float 원문 비트로 보존한다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def digest(path):
    """근거의 실제 바이트 SHA를 읽는다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


class Oracle(CommonOracle):
    """공통 격리 메모리/반환 도구를 빌리고 Carrier 전용 실행 허용 범위만 설정한다."""
    def __init__(self, edition):
        """같은 PE의 정확한 함수 구간과 기존 보호 메모리 사본을 사용한다."""
        super().__init__(edition)
        self.c = PATCH if edition == 'originals' else CD  # 현재 판본의 Carrier 주소 대응이다.
        self.allowed = []  # 공통 객체 몸체는 이번 관찰의 외부 경계다.
        with (ROOT / f'extracted/refined/{edition}/functions.tsv').open(encoding='utf-8') as fp:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(fp, delimiter='\t')}
        # 분기/실제 ftol·CD 비용/가능 조회 helper까지 실행한다.
        for address in [self.c['menu'], *self.c['helpers']]:
            for part in facts[address]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))

    def instruction(self, mu, address, size, data):
        """전체 Carrier 분기를 실행하고 실제 공간/메뉴/번역/조회 의존만 관찰한다."""
        p, c, v = self.p, self.c, self.values
        esp, ecx = mu.reg_read(UC_X86_REG_ESP), mu.reg_read(UC_X86_REG_ECX)
        args = [self.word(esp + 4 + index * 4) for index in range(7)]
        if address == c['flags1']:
            self.ret(v[FLAGS1])
        elif address == p['flags']:
            self.ret(v[FLAGS2])
        elif address == p.get('gettype'):
            self.ret(TYPES + args[0] * p['typebytes'])
        elif address == c['ground']:
            self.emit('G', args[0]);self.ret(v[GROUND])
        elif address == c['begin']:
            self.cursor = 0
            self.put(ecx + 8, self.nodes[0] if self.nodes else 0);self.ret(purge=4)
        elif address == c['next']:
            self.cursor += 1
            self.put(ecx + 8, self.nodes[self.cursor] if self.cursor < len(self.nodes) else 0);self.ret()
        elif address == c['description']:
            number = (ecx - TYPES) // p['typebytes'];self.emit('D', number)
            if v[MUTATION] & 2:
                self.put(self.raw + p['owner'], 2, 1)
            self.ret(TEXT + 1024 + number * 32)
        elif address == c['movement']:
            assert args[1:3] == [0, 0]
            self.emit('M', args[0])
            if v[MUTATION] & 4:
                self.put(self.raw + 10, 133, 1)
            self.ret(54)
        elif address == c['lookup']:
            assert args[2:4] == [0, 0]
            self.emit('L', *args[:2]);self.ret(70 if v[PATH] else 0)
        elif address == c['active']:
            self.emit('A', (ecx - POOL) // p['stride']);self.ret(v[ACTIVE])
        elif address == c['point']:
            self.emit('P', (ecx - POOL) // p['stride'], args[1])
            self.put(args[0], v[X]);self.put(args[0] + 4, v[Y])
            if v[MUTATION] & 1:
                node = POOL + self.nodes[self.cursor] * p['stride'];self.put(node + 20, self.word(node + 20) ^ 1, 2)
            if v[MUTATION] & 16:
                self.put(p['local'], 2)
            self.ret(purge=8)
        elif address == c.get('money'):
            self.emit('S', args[0]);self.put(args[1], v[SP])
            if v[MUTATION] & 8:
                self.put(p['editor'], 1)
            self.ret()
        elif address == c['event']:
            self.emit('E', (ecx - POOL) // p['stride'], args[0])
            if v[MUTATION] & 32 and args[0] == 0x493:
                self.put(self.raw + 10, 133, 1)
            self.ret(v[BLOCKED] if args[0] == 0x493 else v[BUILDING], 4)
        elif address == p['translate']:
            self.emit('T', self.string(args[0]).hex());self.ret(args[0], 4)
        elif address in (p['format'], c['sprintf']):
            pattern = self.string(args[1]).decode('ascii');values = [];index = 2
            # 번역된 원본 고정 서식의%s/%d만 실제 순서대로 소비한다.
            for match in re.finditer('%[sd]', pattern):
                word = args[index];index += 1
                values.append(self.string(word).decode('ascii') if match[0] == '%s' else struct.unpack('<i', struct.pack('<I', word))[0])
            text = (pattern % tuple(values)).encode('ascii')
            mu.mem_write(args[0], text + b'\0');self.ret(args[0] if address == p['format'] else len(text))
        elif address == c['header']:
            self.emit('H', self.string(args[0]).hex());self.ret()
        elif address == p['item']:
            assert args[3] == 1
            if self.edition == 'originals':
                assert args[5] == 0
            self.emit('I', self.string(args[0]).hex(), args[1], args[2], args[4])
            if v[MUTATION] & 64 and self.cursor + 1 < len(self.nodes):
                node = POOL + self.nodes[self.cursor + 1] * p['stride'];self.put(node + 20, self.word(node + 20) ^ 1, 2)
            self.ret()
        elif address == p['damage']:
            self.emit('B', (ecx - POOL) // p['stride']);self.ret()
        else:
            assert any(low <= address and address + size <= high for low, high in self.allowed), f'{self.edition}: 미등록 {address:08x}'
            self.instructions += 1

    def run(self, values, precision):
        """전체 원본 정상 반환/스택/보존 레지스터/제어어와 실제 메뉴 효과 순서를 읽는다."""
        self.values = values  # 이번 호출의 독립22DWORD 입력이다.
        self.events = []  # 이번 실제 메뉴/조회 효과 순서다.
        self.nodes = list(range(60, 60 + values[COUNT]))  # 명시 외부 finder가 반환할 실제 내용물SID다.
        self.cursor = 0  # 외부 finder의 현재 위치다.
        p, c, v = self.p, self.c, values
        self.raw = POOL + 50 * p['stride']  # 같은 실제 부모 슬롯 주소다.
        self.mu.mem_write(POOL, bytes(0x10000));self.mu.mem_write(TYPES, bytes(0x20000))
        self.put(p['pool'], POOL);self.put(p['local'], v[LOCAL]);self.put(p['editor'], v[EDITOR]);self.put(c['limit'], v[LIMIT])
        self.put(self.raw + 10, v[RAWTYPE], 1);self.put(self.raw + p['owner'], v[OWNER], 1);self.put(self.raw + c['extra'], v[EXTRA], 1)
        self.put(self.raw + 6, self.nodes[0] if self.nodes else 0, 2)
        if 'types' in p:
            self.put(p['types'], TYPES);self.put(0x5120d4, v[SP])
        # 실제 타입/비용/설명과 혼합 내용물 체인은 판단 결과가 아닌 입력이다.
        for number in range(188 if self.edition == 'originals' else 171):
            self.put(TYPES + number * p['typebytes'] + 0xc4, v[CDCOST])
            self.put(0x59ab50 + number * 4, v[COST] + number * 23) if self.edition == 'originals' else None
            self.mu.mem_write(TEXT + 1024 + number * 32, f'Type{number}'.encode('ascii') + b'\0')
        for index, sid in enumerate(self.nodes):
            node = POOL + sid * p['stride'];self.put(node + 10, 6, 1);self.put(node + 18, v[CONTENTTYPE] + index, 2);self.put(node + 20, v[CONTENTFLAGS] ^ (index & 1), 2)
            self.put(node + 4, self.nodes[index + 1] if index + 1 < len(self.nodes) else 0, 2)
        saved = {UC_X86_REG_EBX: 0x123456, UC_X86_REG_ESI: 0x234567, UC_X86_REG_EDI: 0x345678, UC_X86_REG_EBP: 0x456789}
        # 실제 함수의 callee 보존 계약을 검증한다.
        for register, value in saved.items():
            self.mu.reg_write(register, value)
        control = 0x027f if precision == 53 else 0x037f
        self.mu.reg_write(UC_X86_REG_FPCW, control);self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.reg_write(UC_X86_REG_ECX, self.raw);self.put(STACK, STOP)
        self.mu.emu_start(c['menu'], STOP, count=30000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert all(self.mu.reg_read(register) == value for register, value in saved.items())
        assert self.mu.reg_read(UC_X86_REG_FPCW) == control
        return ';'.join(self.events) or '-'


def cases():
    """메뉴 결과를 계산하지 않고 원본 경계·교차·재진입 상태를 고정 시드로 만든다."""
    base = (1, 1, 158, 0, 0x20000, 0, 0, 1, 0, 1, 1, 123, bits(40), bits(50), 256, 400, bits(400), bits(3000), 0, 0, 0, 0)
    result = [base]
    options = {OWNER: [0, 2, 8], LOCAL: [0, 2], RAWTYPE: [133], FLAGS1: [0x1000000], FLAGS2: [0], EXTRA: [0x20], GROUND: [1], PATH: [0], ACTIVE: [1], COUNT: [0, 2, 3], CONTENTFLAGS: [0, 2, 3], X: [bits(0), bits(-0.0), bits(-1), bits(0.5), bits(255.99), bits(256), bits(float('nan')), bits(float('inf'))], Y: [bits(0), bits(-0.0), bits(0.5), bits(256), bits(float('nan'))], LIMIT: [64], COST: [-3, 0, 16777217, 2147483647], CDCOST: [bits(-3), bits(0), bits(16777216), bits(2147483648), bits(float('nan'))], SP: [bits(0), bits(399), bits(400), bits(16777216), bits(float('nan'))], EDITOR: [1], BLOCKED: [1], BUILDING: [1], MUTATION: [1, 2, 4, 8, 16, 32, 64, 127]}
    # 각 열을 다른 상태와 독립적으로 변화시킨다.
    for column, choices in options.items():
        for value in choices:
            row = list(base);row[column] = value;result.append(tuple(row))
    rng = random.Random(0x4288f0)
    # mixed 내용물/이동/부족 금액/재진입이 함께 발생하는 입력이다.
    for index in range(256):
        result.append((rng.choice([0, 1, 2]), 1, rng.choice([133, 158]), rng.choice([0, 0, 0x1000000]), rng.choice([0, 0x20000]), rng.choice([0, 0x20]), rng.randrange(2), rng.randrange(2), rng.randrange(2), rng.randrange(4), rng.randrange(4), 123, rng.choice([bits(0), bits(0.5), bits(40), bits(256), bits(float('nan'))]), rng.choice([bits(50), bits(256), bits(float('nan'))]), 256, rng.choice([-3, 0, 400, 16777217]), rng.choice([bits(-3), bits(0), bits(400), bits(16777216)]), rng.choice([bits(0), bits(400), bits(3000), bits(float('nan'))]), rng.randrange(2), rng.randrange(2), rng.randrange(2), rng.randrange(128)))
    return list(dict.fromkeys(result))


def main():
    """고유 원문 관찰을 저장하거나 원문/제품의 현재 SHA만 감사한다."""
    reportpath = ROOT / 'cpppj/carrier-menu.json'
    fixture = ROOT / 'cpppj/tests/fixtures/carrier-menu-x86.tsv'
    if '--verify' in sys.argv:
        report = json.loads(reportpath.read_text(encoding='utf-8'))
        # 근거와 제품의 서로 다른 검증 범위를 구분한다.
        for hashes in [report['sha256'], report.get('product_validation', {}).get('sha256', {})]:
            for path, expected in hashes.items():
                assert digest(path) == expected, f'Carrier 메뉴 SHA 불일치: {path}'
        with fixture.open(encoding='utf-8') as fp:
            rows = list(csv.reader(fp, delimiter='\t'))[1:]
        assert len(rows) == report['observations'] == len({tuple(row[:-1]) for row in rows})
        print(json.dumps({'verified': True, 'observations': len(rows)}));return
    rows, instructions = [], 0
    # 두 실제 PE/두 정밀도 모두 정상 반환을 확인한다.
    for edition in ['originals', 'originalCD']:
        oracle = Oracle(edition)
        for values in cases():
            trace = oracle.run(values, 53);assert trace == oracle.run(values, 64), (edition, values)
            rows.append([edition, *values, trace])
        instructions += oracle.instructions
    with fixture.open('w', encoding='utf-8', newline='') as fp:
        writer = csv.writer(fp, delimiter='\t', lineterminator='\n');writer.writerow(['edition', 'owner', 'local', 'rawtype', 'flags1', 'flags2', 'extra', 'ground', 'path', 'active', 'count', 'contentflags', 'contenttype', 'x', 'y', 'limit', 'cost', 'cdcost', 'sp', 'editor', 'blocked', 'building', 'mutation', 'trace']);writer.writerows(rows)
    paths = ['originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'extracted/refined/originals/functions.tsv', 'extracted/refined/originalCD/functions.tsv', 'tools/decomp_object_menu.py', 'tools/decomp_carrier_menu.py', 'cpppj/tests/fixtures/carrier-menu-x86.tsv']
    report = dict(host='HJOW-Athlon', date='2026-10-17', last_full_decompile_host='HJOW-Athlon', observations=len(rows), normal_returns=len(rows) * 2, precisions=[53, 64], precision_differences=0, instructions=instructions, os_calls=0,
                  boundaries=['TYPE/description·contained finder 결과', '지면 불가·현재 이동 종류/종속/process 활성·사건', '전체 내려놓기 위치·PATCH SP', '번역/CRT 문자열 서식·실제 메뉴 작성·Damageable 기본 메뉴'],
                  limits=['Carrier 전체 몸체와 실제 ftol/CD 비용·SP/상태 wrapper를 실행', '공간 배치/이동/기본 객체/GUI 픽셀 및 실제 Session/명령 실행은 별도', '원본 게임/OS 실행 없음'], sha256={path: digest(path) for path in paths})
    reportpath.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'observations': len(rows), 'normal_returns': len(rows) * 2, 'instructions': instructions}))


if __name__ == '__main__':
    main()
