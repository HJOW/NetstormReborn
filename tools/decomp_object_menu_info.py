#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78 객체메뉴 제목/요약/포획표시와 실제 문자열 helper를 격리 실행한다."""
import argparse
import collections
import itertools
import json
import re
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, POOL, TYPES, VTABLE, STACK, STOP, digest
from decomp_cursor_position import ranges
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

TEXT = 0x12800000  # 반환번역/설명/설정치환문자열은서로다른주소에서호출끝까지유지한다.
PLAYERS = 0x12900000  # 59534c는배열자체가아닌이실제Player9×180배열의포인터다.
COUNT = STOP + 0x500  # 현재객체의실제가상+80을명시입력경계로둔다.
ENTRIES = (0x4ad780, 0x4ada70, 0x42a2f0, 0x4a8980, 0x4a8940, 0x4a8800, 0x484480, 0x4e49c0)  # 전체부모/숫자·원소/이름복사/_ftol이다.
OPERATIONS = ENTRIES[:5]  # Describe/Summary/CaptureStatus/LevelName/ThemeName의외부진입순서다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/object-menu-info-x86.tsv'  # 기존메뉴원문은덮어쓰지않는다.
REPORT = ROOT / 'cpppj/object-menu-info.json'  # 실제PC·범위·정상반환·명시경계와불변근거SHA다.


def bits(value):
    """float독립입력만DWORD로변환하며원본출력기대값은계산하지않는다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def cases():
    """네인자/클래스/숫자·원소/현재필드재조회와callback변경을독립입력으로조합한다."""
    base = [0, 100, 1, 2, 1, 0x10000, 2, 1, 0, 50, 0, 0, 0, 1, 0, 0, bits(12.5), bits(-3.25), 0, 1]
    rows = []  # op/type/owner/count/class/flags/theme/local/editor/sid/extra/ground/mission/status/ally/mutation/x/y/lang/nameOk다.
    # 제단count감소/너깃소유자0,클래스없는레벨생략과범위밖숫자helper를관측한다.
    for kind, owner, count, category, flags, theme in itertools.product((100, 164, 154), (0, 1, 2), (0, 1, 2, 9, 10, 0x80000000, 0xffffffff), (0, 1), (0, 0x10000, 0x80000000), (0, 4, 5)):
        row = base.copy();row[1:7] = kind, owner, count, category, flags, theme;rows.append(row)
    # 편집좌표는두_ftol/하위DWORD와정밀도비교로읽고SID=-1은생략한다.
    for point, sid, language, name_ok in itertools.product(((bits(-17.75), bits(21.5)), (0x4f000000, 0xcf000000), (0x5f000000, 0x7fc01234)), (50, 0xffffffff), (0, 1), (0, 1)):
        row = base.copy();row[8:10] = 1, sid;row[16:20] = *point, language, name_ok;rows.append(row)
    # Summary는4인자snapshot뒤현재extra를읽고포획표시는현재미션/Player상태를읽는다.
    for extra, owner, ground, mission, status, allied in itertools.product((0, 0x10, 0x20, 0x30), (0, 1, 2), (0, 1), (0, 1), (0, 1), (0, 1)):
        row = base.copy();row[0] = 1;row[2] = owner;row[10:15] = extra, ground, mission, status, allied;rows.append(row)
    # 각경계의변경입력만공급하고원본부모가실제로재조회하는지확인한다.
    for op, mutation, category in itertools.product((0, 1), range(1, 14), (0, 1)):
        row = base.copy();row[0] = op;row[4] = category;row[10:12] = 0x30, 1;row[15] = mutation
        if mutation == 5:
            row[8] = 1
        if mutation == 8:
            row[10] = 0x10
        if mutation == 13:
            row[10] = 0
        rows.append(row)
    # 부모를거치지않는포획표시의거짓조기반환과실제helper원문도관측한다.
    for ground, mission, status in itertools.product((0, 1), (0, 1), (0, 1)):
        row = base.copy();row[0] = 2;row[11:14] = ground, mission, status;rows.append(row)
    # 로마숫자표끝과원본sentinel반환을직접관측할독립입력이다.
    for number in (0, 1, 8, 9, 10, 11, 0x7fffffff, 0xffffffff):
        row = base.copy();row[0] = 3;row[3] = number;rows.append(row)
    # 원소이름8개와원본번역바이트2종의helper호출입력이다.
    for number, language in itertools.product(range(8), (0, 1)):
        row = base.copy();row[0] = 4;row[6] = number;row[18] = language;rows.append(row)
    return list(dict.fromkeys(tuple(row) for row in rows))


class Oracle(BaseOracle):
    """실제제목/요약/포획/문자열helper를실행하고번역/printf/GUI/게임조회만명시경계로둔다."""
    def __init__(self):
        """기존보호PE·격리메모리·GP/스택/canary/x87검사를재사용한다."""
        super().__init__();self.mu.mem_map(TEXT, 0x10000);self.mu.mem_map(PLAYERS,0x1000);self.allowed = [];ranges(self, ENTRIES)
        self.instructions = 0;self.calls = collections.Counter();self.roman = bytes(self.mu.mem_read(0x542420, 33));self.put(VTABLE + 0x80, COUNT)
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def read(self,mu,access,address,size,value,data):
        """이8몸체에는NULL/SEH페이지데이터읽기가없으므로잘못된전역포인터입력을즉시거부한다."""
        if address<0x1000:
            raise RuntimeError(f'객체메뉴정보 NULL페이지읽기 {address:08x}+{size}')

    def string(self, address):
        """원본바이트를Latin-1의일대일값으로읽어로마표sentinel/번역바이트를보존한다."""
        value = bytearray()
        # 원본정보버퍼의명시경계보다큰손상문자열은거부한다.
        for offset in range(1024):
            byte = self.mu.mem_read(address + offset, 1)[0]
            if not byte:
                return value.decode('latin-1')
            value.append(byte)
        raise RuntimeError('메뉴정보 문자열 NUL 없음')

    def text(self, value):
        """살아있는반환문자열마다새주소를주어앞번역결과를덮지않는다."""
        raw = value.encode('latin-1') + b'\0';pointer = TEXT + self.used;self.used += max(256, len(raw));assert self.used < 0x10000
        self.mu.mem_write(pointer, raw);return pointer

    def mutate(self, boundary):
        """선택된명시경계의현재입력변경만적용하며부모분기/서식선택은대신계산하지않는다."""
        kind = self.case[1];type_address = TYPES + kind * 500
        if self.case[15] == 1 and boundary == 'count':
            self.put(POOL + 50 * 50 + 10, 101, 1);self.put(POOL + 50 * 50 + 34, 2, 1);self.put(POOL + 50 * 50 + 40, 0x30, 1)
        if self.case[15] == 2 and boundary == 'description':
            self.mu.mem_write(type_address + 0xaa, b'\0')
        if self.case[15] == 3 and boundary == 'firstTitle':
            self.mu.mem_write(type_address + 0xaa, b'Changed\0');self.put(type_address + 0xec, 0x10000);self.put(type_address + 0x98, 4)
        if self.case[15] == 4 and boundary == 'relation':
            self.put(0x5c85a4, 1);self.put(POOL + 50 * 50 + 14, bits(-3.75));self.put(POOL + 50 * 50 + 18, bits(4.5))
        if self.case[15] == 5 and boundary == 'subId':
            self.put(POOL + 50 * 50 + 14, bits(-17.5));self.put(POOL + 50 * 50 + 18, bits(21.5))
        if self.case[15] == 6 and boundary == 'name':
            self.put(0x540c70, 2);self.put(0x595200 + (self.case[2] * 9 + 2) * 4, 1)
        if self.case[15] == 7 and boundary == 'invisibleHeader':
            self.put(POOL + 50 * 50 + 40, 0, 1)
        if self.case[15] == 8 and boundary == 'invisibleTranslation':
            self.put(0x54122c, 99);self.put(POOL + 50 * 50 + 40, 0x20, 1)
        if self.case[15] == 9 and boundary == 'ground':
            self.put(0x594fa4, 1);self.put(POOL + 50 * 50 + 34, 2, 1)
        if self.case[15] == 10 and boundary == 'alignment':
            self.put(type_address + 0x98, 5)
        if self.case[15] == 11 and boundary == 'class':
            self.mu.mem_write(type_address + 0xaa, b'Replacement % token\0')
        if self.case[15] == 12 and boundary == 'firstHeader':
            self.mu.mem_write(type_address + 0xaa, b'\0')
        if self.case[15] == 13 and boundary == 'lastHeader':
            self.put(POOL + 50 * 50 + 40, 0x30, 1)

    def instruction(self, mu, address, size, data):
        """실제부모가선택한전체인자/문자열/현재필드만경계에전달하고순서를기록한다."""
        self.instructions += 1;esp = mu.reg_read(UC_X86_REG_ESP);args = struct.unpack('<6I', mu.mem_read(esp + 4, 24));this = mu.reg_read(UC_X86_REG_ECX)
        if address in ENTRIES:
            self.calls[f'{address:08x}'] += 1
        if address == 0x49a840:
            self.ret(TYPES + args[0] * 500);return
        if address == 0x49af80:
            kind = (this - TYPES) // 500;self.event('D', kind);self.mutate('description');self.ret(self.text(f'Type{kind}'));return
        if address == COUNT:
            self.event('C', (this - POOL) // 50);self.mutate('count');self.ret(self.case[3]);return
        if address == 0x4841c0:
            owner = self.get(this);self.event('P', owner);self.mutate('name');mu.mem_write(this + 8, f'Player{owner}'.encode() + b'\0');self.ret(self.case[19]);return
        if address == 0x4de9a0:
            key = self.string(args[0]);self.event('L', key.encode('latin-1').hex())
            # 번역자체는명시입력이며이변경은원본의다음현재값읽기에서만관측된다.
            if key in ('Owner', 'Ally', 'Enemy'):
                self.mutate('relation')
            # 각번역경계가선택된때만명시현재값변경을적용한다.
            for source, boundary in [('~y~[I%d.*]Invisible!~.\n', 'invisibleTranslation'), ('\nAlignment: ~i%s~.', 'alignment'), ('\nClass: ~i%s~.', 'class')]:
                if key == source:
                    self.mutate(boundary)
            self.ret(self.text(('\xe9:' if self.case[18] else '') + key),4);return
        if address in (0x4a89c0, 0x4a89e0):
            pattern = self.string(args[1]);values = []
            # 고정서식의외부인자만출력하며type/owner/분기선택은계산하지않는다.
            for index, match in enumerate(re.finditer(r'%(?:%|s|d)', pattern)):
                if match.group() == '%%':
                    continue
                value = args[2 + len(values)];values.append(self.string(value) if match.group() == '%s' else struct.unpack('<i', struct.pack('<I', value))[0])
            output = pattern % tuple(values);mu.mem_write(args[0], output.encode('latin-1') + b'\0')
            if address == 0x4a89c0:
                self.event('F', output.encode('latin-1').hex())
            self.ret(args[0]);return
        if address == 0x441740:
            assert args[1:5] == (1024, 0, 0, 0);text = self.string(args[0]);self.event('S', text.encode('latin-1').hex());self.ret(self.text('[' + text + ']'));return
        if address in (0x477ee0, 0x478020):
            title = address == 0x477ee0;text = self.string(args[0]);self.event('T' if title else 'H', text.encode('latin-1').hex())
            if title:
                self.titles += 1
                if self.titles == 1:
                    self.mutate('firstTitle')
            else:
                self.headers += 1
                if self.headers == 1:
                    self.mutate('firstHeader')
                if text.startswith('\nSubID:'):
                    self.mutate('subId')
                if 'Invisible!' in text:
                    self.mutate('invisibleHeader')
                if text == '\n':
                    self.mutate('lastHeader')
            self.ret();return
        if address == 0x492090:
            self.event('G', args[0]);self.mutate('ground');self.ret(self.case[11]);return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'객체메뉴정보 범위밖실행 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """실제코드는격리스택과로마숫자분리표밖에쓸수없다."""
        if not any(low <= address and address + size <= high for low, high in ((STACK - 0x7f000, STACK + 0x1000), (0x542420, 0x542441))):
            raise RuntimeError(f'객체메뉴정보 범위밖쓰기 {address:08x}+{size}')

    def sample(self, case, precision):
        """모든현재입력을독립설치한뒤원본반환/경계순서/현재필드만출력한다."""
        self.case = case;self.events = [];self.used = 0;self.titles = 0;self.headers = 0;mu = self.mu
        mu.mem_write(0x542420, self.roman);self.put(0x5c8464, POOL);self.put(0x540c70, case[7]);self.put(0x5c85a4, case[8]);self.put(0x594fa4, case[12]);self.put(0x54122c, 117);self.put(0x541230, 118);self.put(0x5412e8, 164);self.put(0x5412c0, 154);self.put(0x540bd0, self.text('UNKNOWN'))
        # 필요한물리슬롯과현재TYPE/Player/동맹을다른입력과공유하지않는다.
        raw = bytearray(50);struct.pack_into('<I', raw, 0, VTABLE);raw[10] = case[1];raw[34] = case[2];raw[40] = case[10];struct.pack_into('<2I', raw, 14, *case[16:18]);mu.mem_write(POOL + 50 * 50, bytes(raw))
        for kind in (100, 101, 154, 164):
            mu.mem_write(TYPES + kind * 500, bytes(500))
        kind_address = TYPES + case[1] * 500;self.put(kind_address + 0xec, case[5]);self.put(kind_address + 0x98, case[6]);mu.mem_write(kind_address + 0xaa, b'Class % value\0' if case[4] else b'\0')
        mu.mem_write(0x595200, bytes(81 * 4));self.put(0x595200 + (case[2] * 9 + case[7]) * 4, case[14])
        self.put(0x59534c,PLAYERS)
        # 실제포인터가가리키는9개Player의상태BYTE를각각독립입력으로쓴다.
        for owner in range(9):
            self.put(PLAYERS + owner * 180 + 0x50, case[13], 1)
        arguments = struct.pack('<4I', case[1], case[3], case[2], case[9]) if case[0] == 0 else struct.pack('<I', case[3] if case[0] == 3 else case[6] if case[0] == 4 else 50)
        self.call(OPERATIONS[case[0]], 0, arguments, 0, precision);self.returns += 1
        result = self.string(mu.reg_read(UC_X86_REG_EAX)).encode('latin-1').hex() if case[0] >= 3 else '-'
        current = bytes(mu.mem_read(POOL + 50 * 50, 50)).hex() + '|' + ','.join(str(self.get(address)) for address in (0x540c70, 0x5c85a4, 0x594fa4, 0x54122c, 0x541230)) + '|' + self.string(kind_address + 0xaa).encode('latin-1').hex() + '|' + str(self.get(kind_address + 0xec)) + ',' + str(self.get(kind_address + 0x98))
        return ';'.join(self.events) or '-', result, current


def main():
    """새독립기대값을생성하거나기존관측의입력/범위/정상반환/SHA를감사한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true');args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text('utf-8'));rows = [row.split('\t') for row in FIXTURE.read_text('utf-8').splitlines() if row and not row.startswith('#')]
        # 원본출력의부모분기를재계산하지않고불변원문과독립입력집합을확인한다.
        for path, wanted in report['sha256'].items():
            assert digest(ROOT / path) == wanted, path
        assert [row[0] for row in rows] == [','.join(map(str, row)) for row in cases()]
        assert len(rows) == report['uniqueInputs'] and report['normalReturns'] == len(rows) * 2 and not report['precisionDifferentInputs']
        assert report['nullPageReadGuard'] is True and report['playerArrayInput']==dict(pointerGlobal='0059534c',base=f'{PLAYERS:08x}',count=9,stride=180,statusOffset=80)
        assert all(len(row) == 4 for row in rows);print('객체메뉴정보 근거 감사:', len(rows), '입력/', report['normalReturns'], '정상반환');return
    oracles = [Oracle(), Oracle()];rows = []
    # C++를읽거나실행하지않고두x87정밀도의실제원본출력만비교한다.
    for case in cases():
        outputs = [oracle.sample(case, precision) for oracle, precision in zip(oracles, (0x27f, 0x37f))]
        assert outputs[0] == outputs[1], case;rows.append((','.join(map(str, case)), *outputs[0]))
    FIXTURE.write_text('# input events result current\n' + '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_particle_gradient.py', 'tools/decomp_cursor_lifecycle.py', 'tools/decomp_cursor_position.py', 'tools/decomp_object_menu_info.py', FIXTURE.relative_to(ROOT).as_posix()]
    report = dict(host=socket.gethostname(), date='2026-10-17', lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-16'), entries=[f'{x:08x}' for x in ENTRIES], controls=[0x27f, 0x37f], uniqueInputs=len(rows), normalReturns=sum(o.returns for o in oracles), instructions=sum(o.instructions for o in oracles), nativeCalls=dict(sum((o.calls for o in oracles), collections.Counter())), precisionDifferentInputs=[], boundaries='번역/순수printf·copy/설명/Config치환/GUI/가상count/PlayerFinder/492090은명시입력. 세부모와숫자·원소·이름복사/_ftol은실제전체실행. C++기대분기계산/게임·OS실행없음.', sha256={path:digest(ROOT / path) for path in paths})
    report['nullPageReadGuard']=True;report['playerArrayInput']=dict(pointerGlobal='0059534c',base=f'{PLAYERS:08x}',count=9,stride=180,statusOffset=80)
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n');print(json.dumps({key:report[key] for key in ('uniqueInputs', 'normalReturns', 'instructions', 'nativeCalls')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
