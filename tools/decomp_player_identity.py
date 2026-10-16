#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 실제 PlayerFinder/초기화/네 getter와 객체 메뉴의 DWORD 전달을 격리 관찰한다."""
import csv
import hashlib
import json
import random
import struct
import sys
from pathlib import Path
from decomp_object_menu import Oracle as CommonOracle, ROOT, POOL, TYPES, TEXT, STACK, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# 기존 격리 공간에서 Player9/metadata40/출력 레코드를 서로 분리한다.
PLAYERS, META, RECORD, OUTPUT = POOL + 0x4000, TYPES, TEXT + 0x8000, TEXT + 0x9000
# 계산을 포함하지 않는 두 판본의 실제 주소/배치 대응이다.
PATCH = dict(ctor=0x484190, resolve=0x4841c0, subscriber=0x4844f0, owner=0x484550, address=0x484520, name=0x484410,
             compare=0x4e65ed, players=0x59534c, stride=180, metaptr=0x5467d8, metastride=460, metaname=38, active=100,
             chat=0x558ed0, localname=0x540ca8, localaddress=0x5953fc, localsubscriber=0x540c74, unknown=0x540bd0,
             battle=0x594fbc, metamap=0x594fc8)
CD = dict(ctor=0x4015e0, resolve=0x401610, subscriber=0x4019e0, owner=0x401ad0, address=0x401aa0, name=0x401890,
          compare=0x4f2980, players=0x50f834, stride=172, metaptr=0x530930, metastride=296, metaname=8, active=72,
          chat=0x565d18, localname=0x50f6d0, localaddress=0x5490d8, localsubscriber=0x50f6cc, unknown=0x50f270,
          battle=0x540a20, metamap=0x540a24)
# fixture13개 독립 입력 열이다. 문자열은 바이트hex이며 '-'는 빈 문자열이다.
MODES, TABLE, LOCAL, SUB, ADDRESS, NAME, QOWNER, QSUB, QADDR, QNAME, ARG, RAWOWNER, INITMODE = range(13)


def digest(path):
    """기존 원문/도구/fixture의 바이트 계보를 보존한다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def unhex(text):
    """원문 문자열 입력만 디코딩하고 시스템 문자 인코딩을 적용하지 않는다."""
    return b'' if text == '-' else bytes.fromhex(text)


class Oracle(CommonOracle):
    """실제 조회 몸체와 getter를 실행하며 ASCII C 비교/GUI만 외부 경계로 둔다."""
    def __init__(self, edition):
        """기존 보호 메모리/반환 도구를 빌려 정확한 실행 구간만 사용한다."""
        super().__init__(edition)
        self.d = PATCH if edition == 'originals' else CD  # 현재 판본의 식별자 배치다.
        self.allowed = []  # 실제 새 몸체/객체 기본 메뉴만 허용한다.
        with (ROOT / f'extracted/refined/{edition}/functions.tsv').open(encoding='utf-8') as fp:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(fp, delimiter='\t')}
        # 초기화/검색/네 getter와 실제 객체 메뉴를 독립 근거로 실행한다.
        for address in [self.d[key] for key in ['ctor', 'resolve', 'subscriber', 'owner', 'address', 'name']] + [self.p['base']]:
            for part in facts[address]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))

    def instruction(self, mu, address, size, data):
        """정수/포인터 의미를 바꾸지 않고 실제 문자열 비교와 GUI 효과만 관찰한다."""
        p, d = self.p, self.d
        if address == d['compare']:
            esp = mu.reg_read(UC_X86_REG_ESP);first, second = struct.unpack('<II', mu.mem_read(esp + 4, 8))
            self.ret(int(self.string(first).lower() != self.string(second).lower()))
        elif address in (p['translate'], p['item'], p['player'], p['submenu']):
            esp = mu.reg_read(UC_X86_REG_ESP);args = struct.unpack('<IIIIII', mu.mem_read(esp + 4, 24))
            if address == p['translate']:
                self.emit('T', self.string(args[0]).hex());self.ret(args[0], 4)
            elif address == p['item']:
                value = 'N:' + str(args[2]) if args[2] < 0x10000 else 'S:' + self.string(args[2]).hex()
                assert args[3:5] == (1, 1)
                if self.edition == 'originals':
                    assert args[5] == 0
                self.emit('I', self.string(args[0]).hex(), args[1], value);self.ret()
            elif address == p['player']:
                assert args[1:3] == (1, 0)
                self.emit('P', args[0], 1, 0);self.ret(123)
            else:
                assert args[2:4] == (1, 0)
                self.emit('S', self.string(args[0]).hex(), args[1]);self.ret()
        else:
            assert any(low <= address and address + size <= high for low, high in self.allowed), f'{self.edition}: 미등록 {address:08x}'
            self.instructions += 1

    def text(self, address, value):
        """명시 입력 바이트와 NUL만 쓰며 뒤의 표식은 보존한다."""
        self.mu.mem_write(address, value + b'\0')

    def prepare(self, v):
        """검색 결과를 계산하지 않고 현재 전역과 세 실제 기록표를 독립 입력으로 채운다."""
        p, d = self.p, self.d
        self.events = []  # 이번 실제 객체 메뉴 효과다.
        self.mu.mem_write(PLAYERS, bytes(9 * d['stride']));self.mu.mem_write(META, bytes(40 * d['metastride']));self.mu.mem_write(d['chat'], bytes(100 * 48))
        self.put(d['players'], PLAYERS);self.put(d['metaptr'], META - 0xb70);self.put(p['local'], v[LOCAL]);self.put(d['localsubscriber'], v[SUB]);self.put(d['localaddress'], v[ADDRESS])
        self.text(TEXT, unhex(v[NAME]));self.put(d['localname'], TEXT);self.text(TEXT + 100, b'SOMEONE');self.put(d['unknown'], TEXT + 100)
        self.put(d['battle'], v[MODES] & 1);self.put(p['editor'], v[MODES] & 2);self.put(d['metamap'], v[MODES] & 4);self.put(p['special'], 0);self.put(p['charge'], 0)
        # 표의 처음/마지막/중복/불완전 기록은 이름/숫자의 독립 입력이다.
        for bit, owner, subscriber, name in [(1, 2, 0x80000002, b'Alpha'), (2, 8, 0xfffffff0, b'LAST'), (4, 3, 0x80000002, b'Duplicate'), (8, 4, 0, b'OnlyName'), (2048, 2, 0x80000055, b'localuser')]:
            if v[TABLE] & bit:
                row = PLAYERS + owner * d['stride'];self.put(row + 44, subscriber);self.text(row + 12, name)
        for bit, index, owner, subscriber, name, active in [(16, 0, 6, 0x80000002, b'MetaFirst', 1), (32, 39, 7, 0xfffffff1, b'MetaLast', 1), (64, 1, 5, 0x80000100, b'Inactive', 2)]:
            if v[TABLE] & bit:
                row = META + index * d['metastride'];self.put(row, owner);self.put(row + 4, subscriber);self.text(row + d['metaname'], name);self.put(row + d['active'], active)
        for bit, index, subscriber, name in [(128, 0, 0x80000002, b'ChatFirst'), (256, 99, 0xfffffff2, b'ChatLast'), (512, 1, 0, b'ZeroSub'), (1024, 2, 0x80000002, b'ChatSecond')]:
            if v[TABLE] & bit:
                row = d['chat'] + index * 48;self.put(row + 44, subscriber);self.text(row, name)
        self.mu.mem_write(RECORD, bytes([0xa5]) * 60);self.put(RECORD, v[QOWNER]);self.put(RECORD + 4, v[QSUB]);self.text(RECORD + 8, unhex(v[QNAME]));self.put(RECORD + 48, v[QADDR]);self.put(RECORD + 52, 0xdeadbeef)
        self.mu.mem_write(OUTPUT, bytes([0x5a]) * 128)
        self.put(p['pool'], POOL);self.put(POOL + 50 * p['stride'] + p['owner'], v[RAWOWNER], 1);self.put(POOL + 50 * p['stride'] + 10, 158, 1)
        self.text(TEXT + 200, b'AboutType');self.put(p['help'], TEXT + 200)

    def run(self, kind, v, precision):
        """실제 정상 반환/스택/레지스터와 전체60바이트 또는 getter DWORD를 읽는다."""
        self.prepare(v);d = self.d
        saved = {UC_X86_REG_EBX: 0x123456, UC_X86_REG_ESI: 0x234567, UC_X86_REG_EDI: 0x345678, UC_X86_REG_EBP: 0x456789}
        # callee 보존 레지스터와 x87 환경을 확인한다.
        for register, value in saved.items():
            self.mu.reg_write(register, value)
        control = 0x027f if precision == 53 else 0x037f
        self.mu.reg_write(UC_X86_REG_FPCW, control);self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff);self.mu.reg_write(UC_X86_REG_ESP, STACK)
        ecx = RECORD if kind in ['ctor', 'resolve'] else POOL + 50 * self.p['stride']
        args = [v[INITMODE]] if kind == 'ctor' else [OUTPUT, v[ARG]] if kind == 'name' else [] if kind in ['resolve', 'menu'] else [v[ARG]]
        self.mu.reg_write(UC_X86_REG_ECX, ecx);self.mu.mem_write(STACK, struct.pack('<' + 'I' * (1 + len(args)), STOP, *args))
        self.mu.emu_start(self.p['base'] if kind == 'menu' else d[kind], STOP, count=100000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + (8 if kind == 'ctor' else 4)
        assert all(self.mu.reg_read(register) == value for register, value in saved.items()) and self.mu.reg_read(UC_X86_REG_FPCW) == control
        result = self.string(OUTPUT).hex() if kind == 'name' else '0' if kind in ['ctor', 'menu'] else str(self.mu.reg_read(UC_X86_REG_EAX))
        raw = bytes(self.mu.mem_read(RECORD, 60)).hex() if kind in ['ctor', 'resolve'] else '-'
        return result, raw, ';'.join(self.events) or '-'


def cases():
    """이름/정수/세 기록표의 실제 입력만 생성하며 예상 검색 결과는 계산하지 않는다."""
    base = [1, 2047, 1, 0xfedcba98, 0xc0a8005e, b'LocalUser'.hex(), 2, 0, 0, '-', 0x80000002, 2, 2]
    resolved = [tuple(base)]
    # 로컬 네 match·세 기록표 우선순위·부분완료·첫/마지막 기록을 독립 변화시킨다.
    for column, options in {MODES: range(8), TABLE: [0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4095], LOCAL: [0, 2], SUB: [0, 1, 0xffffffff], QOWNER: [0, 1, 3, 4, 8, 0xffffffff], QSUB: [1, 0xfedcba98, 0x80000002, 0xfffffff0, 0xfffffff1, 0xfffffff2, 0x80000100], QADDR: [0xc0a8005e, 1], QNAME: [x.hex() for x in [b'localuser', b'ALPHA', b'last', b'MetaFirst', b'MetaLast', b'ChatLast', b'OnlyName', b'ZeroSub', b'Missing']]}.items():
        for value in options:
            row = base.copy();row[column] = value;resolved.append(tuple(row))
    rng = random.Random(0x4841c0)
    # 여러 식별자와 검색 모드가 충돌하는 교차 입력이다.
    for index in range(128):
        row = base.copy();row[MODES] = rng.randrange(8);row[TABLE] = rng.randrange(4096);row[QOWNER] = rng.choice([0, 1, 2, 4, 8, 99]);row[QSUB] = rng.choice([0, 0xfedcba98, 0x80000002, 0xfffffff1, 0xfffffff2]);row[QNAME] = rng.choice(['-', b'ALPHA'.hex(), b'localuser'.hex(), b'ZeroSub'.hex(), b'Missing'.hex()]);resolved.append(tuple(row))
    result = [('resolve', row) for row in dict.fromkeys(resolved)]
    # getter는 사용하지 않는 query 입력을0으로 정규화해 관찰 건수를 부풀리지 않는다.
    for kind in ['subscriber', 'owner', 'address', 'name']:
        seen = set()
        for source in resolved:
            row = list(source);row[QOWNER] = row[QSUB] = row[QADDR] = 0;row[QNAME] = '-';row[ARG] = source[QOWNER] if kind == 'subscriber' else source[QSUB] or 0x80000002
            seen.add(tuple(row))
        result.extend((kind, row) for row in sorted(seen))
    # 생성자는 미작성 바이트까지 전체60바이트를 비교한다.
    for mode in [0, 1, 2, 0xffffffff]:
        row = base.copy();row[INITMODE] = mode;result.append(('ctor', tuple(row)))
    # 실제 공통 객체 몸체가 getter DWORD를 Player 메뉴로 전달하는 입력이다.
    for modes in [0, 1, 2, 4, 5, 7]:
        for owner in [0, 1, 2, 8]:
            row = base.copy();row[MODES] = modes;row[RAWOWNER] = owner;result.append(('menu', tuple(row)))
    return result


def main():
    """새 원문 관찰/fixture를 저장하거나 원문·제품의 현재 SHA를 감사한다."""
    reportpath = ROOT / 'cpppj/player-identity.json';fixture = ROOT / 'cpppj/tests/fixtures/player-identity-x86.tsv'
    if '--verify' in sys.argv:
        report = json.loads(reportpath.read_text(encoding='utf-8'))
        # 원문과 제품 pin의 서로 다른 검증 범위를 구분한다.
        for hashes in [report['sha256'], report.get('product_validation', {}).get('sha256', {})]:
            for path, expected in hashes.items():
                assert digest(path) == expected, f'Player 식별자 SHA 불일치: {path}'
        with fixture.open(encoding='utf-8') as fp:
            rows = list(csv.reader(fp, delimiter='\t'))[1:]
        assert len(rows) == report['observations'] == len({tuple(row[:-3]) for row in rows})
        print(json.dumps({'verified': True, 'observations': len(rows)}));return
    rows, instructions = [], 0
    # 두 실제 PE에서 같은 정수 입력을 두 제어어로 정상 반환한다.
    for edition in ['originals', 'originalCD']:
        oracle = Oracle(edition)
        for kind, values in cases():
            observed = oracle.run(kind, values, 53);assert observed == oracle.run(kind, values, 64), (edition, kind, values)
            rows.append([edition, kind, *values, *observed])
        instructions += oracle.instructions
    with fixture.open('w', encoding='utf-8', newline='') as fp:
        writer = csv.writer(fp, delimiter='\t', lineterminator='\n');writer.writerow(['edition', 'kind', 'modes', 'table', 'local', 'subscriber', 'address', 'name', 'queryowner', 'querysub', 'queryaddress', 'queryname', 'argument', 'rawowner', 'initmode', 'result', 'record', 'trace']);writer.writerows(rows)
    paths = ['originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'extracted/refined/originals/functions.tsv', 'extracted/refined/originalCD/functions.tsv', 'tools/decomp_object_menu.py', 'tools/decomp_player_identity.py', 'cpppj/tests/fixtures/player-identity-x86.tsv']
    counts = {kind: sum(row[1] == kind for row in rows) for kind in ['ctor', 'resolve', 'subscriber', 'owner', 'address', 'name', 'menu']}
    report = dict(host='HJOW-Athlon', date='2026-10-17', last_full_decompile_host='HJOW-Athlon', observations=len(rows), counts=counts, normal_returns=len(rows) * 2, precisions=[53, 64], precision_differences=0, instructions=instructions, os_calls=0,
                  boundaries=['C locale ASCII stricmp 비교', '객체 메뉴 번역·항목/하위 메뉴 작성·Player 메뉴 전체'], limits=['실제 식별자 조회/초기화/네 getter와 공통 객체 몸체를 실행', 'Player 전체/실제 Session/GUI 픽셀/건설은 별도', '게임/OS 호출 없음'], sha256={path: digest(path) for path in paths})
    reportpath.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n');print(json.dumps({'observations': len(rows), 'counts': counts, 'normal_returns': len(rows) * 2, 'instructions': instructions}))


if __name__ == '__main__':
    main()
