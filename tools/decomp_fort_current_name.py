#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재Fort이름00460980와동일manager파일명004bf630을파일조작없이독립관측한다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

HEADERS = 0x5c89f0  # 실제9×20BYTE Template표다.
MANAGERS = {0: 0, 11: POOL + 0x1000, 22: POOL + 0x2000}  # 명시manager276BYTE두개와null이다.
CURRENTS = (POOL + 0x4000, POOL + 0x4200)  # 현재Fort포인터가선택하는256BYTE쓰기버퍼다.
NEW_NAME, PATH, STATES = POOL + 0x5000, POOL + 0x6000, POOL + 0x7000  # 외부입력512/PathSpec512/9State28의서로다른주소다.
OWNER, CURRENT, DIRTY = 0x540cac, 0x55b490, 0x542678  # callback뒤재조회하는현재전역이다.
FORT, EDITOR, MISSION = 0x594fb8, 0x5c85a4, 0x594fa0  # 마지막설정관문이다.
SPEC, SPEC_POINTER = 0x5327ac, 0x5327e0  # 본문은고정9BYTE내용이아니라포인터만다시쓴다.
OLD_PATH = STACK - 256  # 원본미초기화지역256BYTE는명시입력으로공급한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fort-current-name-x86.tsv'  # 제품작성전확정할최초관측이다.
REPORT = ROOT / 'cpppj/fort-current-name-native.json'  # CRT/하위경계와실행범위를기록한다.
RANGES = ((0x460980, 0x460ba7), (0x4bf630, 0x4bf68b), (0x4e63d0, 0x4e63d5), (0x4e63e0, 0x4e649e), (0x4ef510, 0x4ef55e))  # parent/setter/실제strchr·C로케일stricmp본체다.


def old_name(kind):
    """원본현재이름의명시입력을만든다."""
    return (b'Old', b'', b'oLd')[kind]


def new_name(kind):
    """NUL뒤/255·256경계/현재이름자기참조를서로구별한다."""
    return (b'New\0', b'Old\0', b'oLd\0', b'Bad.Name\0', b'\0', b'M' * 255 + b'\0', b'M' * 256 + b'\0', b'New\0.after', b'')[kind]


def cases():
    """분기기본입력과현재값변경을독립구성하고원문이선택한효과만관측한다."""
    rows = []
    modes = ((0, 0, 0, 0), (1, 0, 0, 0), (1, 0, 0, 2), (1, 1, 0, 2), (1, 0, 1, 0), (0, 1, 1, 0))
    # 정상owner1의존재/readonly/빈현재/대소문자·점·빈새이름을교차한다.
    for manager, readonly, old, new, flags in itertools.product((0, 11), (0, 1), (0, 1), range(5), modes):
        rows.append(('current', 1, manager, readonly, 0, old, new, 0, *flags, 'none'))
    # 고정setter는current/global모드와달리중복·manager진단뒤같은header만사용한다.
    for manager, duplicate, new in itertools.product((0, 11), (0, 1), (0, 3, 4, 5, 6, 7)):
        rows.append(('manager', 1, manager, 0, duplicate, 0, new, 0, 0, 0, 0, 0, 'none'))
    # 이름/PathSpec의실제256BYTE경계와같은경로매핑/가변FileSpec을별도선택한다.
    for owner, old, new, path in itertools.product((0, 8), (0, 2), (0, 1, 2, 5, 6, 7, 8), range(5)):
        rows.append(('current', owner, 11, 0, 0, old, new, path, 1, 1, 0, 0, 'none'))
    mutations = ('path1-owner', 'path2-owner', 'path1-current1', 'snapshot1-current1', 'snapshot1-owner', 'snapshot1-manager0', 'snapshot1-readonly1', 'snapshot1-name', 'notify-owner', 'notify-manager22', 'notify-readonly1', 'snapshot2-readonly1', 'title-editor1', 'title-dirty', 'title-owner', 'state-editor1', 'state-current1', 'state-mission1', 'config1-editor1', 'path1-specsolo', 'path1-pointerother', 'snapshot1-stop', 'notify-stop', 'snapshot2-stop', 'delete-stop', 'title-stop', 'state-stop', 'config1-stop')
    # 같은callback위치에서현재값변경을각각한건씩명시한다.
    for mutation in mutations:
        rows.append(('current', 1, 11, 0, 0, 0, 0, 0, 1, 0, 0, 0, mutation))
    rows.extend((('current', 1, 11, 0, 1, 1, 3, 0, 1, 0, 0, 0, 'assert39-owner'), ('current', 1, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0, 'assert88-manager22'), ('current', 1, 11, 0, 0, 0, 3, 0, 1, 0, 0, 0, 'assert88-stop'), ('manager', 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'assert429-manager22'), ('manager', 1, 11, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'assert430-manager22'), ('manager', 1, 11, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'notify-owner')))
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """선택된PE/CRT만실행하고snapshot/notify/삭제·설정·PathSpec는명시경계로둔다."""
    def __init__(self):
        """보호PE를별도가상메모리에읽고CPU쓰기범위를제한한다."""
        super().__init__()
        self.allowed = RANGES
        self.total_instructions = self.normal_returns = 0
        self.stops = Counter()
        self.boundaries = Counter()
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def guard(self, mu, access, address, size, value, data):
        """원본CPU는지역스택/실제header·manager명/현재이름·dirty/FileSpec포인터만쓴다."""
        ranges = ((STACK - 0x200, STACK + 12), (HEADERS, HEADERS + 180), *((base + 12, base + 268) for key, base in MANAGERS.items() if key), *((base, base + 256) for base in CURRENTS), (DIRTY, DIRTY + 4), (SPEC_POINTER, SPEC_POINTER + 4))
        if not any(low <= address and address + size <= high for low, high in ranges):
            raise RuntimeError(f'Fort이름CPU쓰기범위밖 {address:08x}/{size}')

    def record(self, kind, value):
        """실제하위진입순서를기록하고해당경계의1기반순번을돌려준다."""
        self.events.append(value)
        self.calls[kind] += 1
        self.boundaries[kind] += 1
        return self.calls[kind]

    def stop(self, cause):
        """정상반환과구별되는명시callback/물리쓰기중단이다."""
        self.cause = cause
        self.mu.emu_stop()

    def mutate(self, boundary, slot=None):
        """원문분기·주소선택을대신하지않고지정한현재값/버퍼만바꾼다."""
        mutation = self.case[-1]
        selected = self.words(OWNER, 1)[0]
        if mutation == boundary + '-owner':
            self.put(OWNER, self.other)
        elif mutation == boundary + '-current1':
            self.put(CURRENT, CURRENTS[1])
        elif mutation == 'snapshot1-manager0' and boundary == 'snapshot1':
            self.put(HEADERS + selected * 20, 0)
        elif mutation in ('snapshot1-readonly1', 'snapshot2-readonly1', 'notify-readonly1') and boundary == mutation.split('-')[0]:
            self.put(HEADERS + (slot if slot is not None else selected) * 20 + 8, 1)
        elif mutation == 'snapshot1-name' and boundary == 'snapshot1':
            self.mu.mem_write(self.argument, b'Changed\0')
        elif mutation in ('notify-manager22', 'assert429-manager22', 'assert430-manager22', 'assert88-manager22') and boundary == mutation.split('-')[0]:
            self.put(HEADERS + (slot if slot is not None else selected) * 20, MANAGERS[22])
        elif mutation in ('title-editor1', 'state-editor1', 'config1-editor1') and boundary == mutation.split('-')[0]:
            self.put(EDITOR, 1)
        elif mutation == 'title-dirty' and boundary == 'title':
            self.put(DIRTY, 0xffffffff)
        elif mutation == 'state-mission1' and boundary == 'state':
            self.put(MISSION, 1)
        elif mutation == 'path1-specsolo' and boundary == 'path1':
            self.mu.mem_write(SPEC, b'SOLOSpec\0')
        elif mutation == 'path1-pointerother' and boundary == 'path1':
            self.put(SPEC_POINTER, NEW_NAME)
        elif mutation == boundary + '-stop':
            self.stop(boundary)

    def instruction(self, mu, address, size, data):
        """현재이름/파일명복사와재조회·관문은실제PE가수행한다."""
        self.instructions += 1
        self.total_instructions += 1
        if self.instructions > 20000:
            raise RuntimeError('Fort이름명령상한초과')
        if address == 0x4609e0 and mu.reg_read(UC_X86_REG_EDX) >= OLD_PATH + 256:
            self.stop('old-path-size')
            return
        if address == 0x460a47 and not any(base <= mu.reg_read(UC_X86_REG_EDX) < base + 256 for base in CURRENTS):
            self.stop('current-size')
            return
        if address == 0x4bf676:
            target = mu.reg_read(UC_X86_REG_EAX)
            if target == 12:
                self.stop('manager0')
                return
            if not any(base + 12 <= target < base + 268 for key, base in MANAGERS.items() if key):
                self.stop('manager-name-size')
                return
        esp = mu.reg_read(UC_X86_REG_ESP)
        args = self.words(esp + 4, 4)
        if address == 0x4410f0:
            assert args[0] == SPEC and args[2:] == (0, 0)
            key, name = self.string(SPEC), self.string(args[1])
            path_mode = self.case[7]
            if path_mode == 1:
                result = b'C:/fixed.fort'
            elif path_mode in (3, 4):
                length = 255 if path_mode == 3 else 256
                result = b'P' * max(0, length - len(name) - 5) + name + b'.fort'
            else:
                result = (b'C:/work/d/' if key == b'fortSpec' else b'C:/solo/d/') + name + b'.fort'
            if path_mode == 2:
                result += b'\0TAIL'
            index = self.record('path', f'P:{key.hex()}:{name.hex()}:{result.hex()}')
            self.mu.mem_write(PATH, result + b'\0')
            self.mutate('path' + str(index))
            if not self.cause:
                self.ret(PATH)
        elif address == 0x4e0620:
            line = args[2]
            self.record('assert', 'A:' + str(line))
            slot = (mu.reg_read(UC_X86_REG_ESI) - HEADERS) // 20 if line in (429, 430) else None
            self.mutate('assert' + str(line), slot)
            if not self.cause:
                self.ret()
        elif address == 0x4bfac0:
            slot = self.words(OWNER, 1)[0]
            manager = next(key for key, base in MANAGERS.items() if base == self.words(HEADERS + slot * 20, 1)[0])
            index = self.record('snapshot', f'S:{slot}:{manager}')
            self.mutate('snapshot' + str(index))
            if not self.cause:
                self.ret()
        elif address == 0x4bd360:
            slot = (mu.reg_read(UC_X86_REG_ECX) - HEADERS) // 20
            manager = next(key for key, base in MANAGERS.items() if base == self.words(HEADERS + slot * 20, 1)[0])
            self.record('notify', f'N:{slot}:{manager}:{self.string(MANAGERS[manager] + 12).hex()}')
            self.mutate('notify', slot)
            if not self.cause:
                self.ret()
        elif address == 0x4e65ed:
            self.record('compare', 'E:' + self.string(args[0]).hex() + ':' + self.string(args[1]).hex())
            mu.reg_write(UC_X86_REG_EIP, 0x4ef510)  # CRT의TLS/locale선택만C로케일로명시하고실제비교본체를실행한다.
        elif address == 0x460b10:
            self.events.append('Q:' + str(int(mu.reg_read(UC_X86_REG_EAX) == 0)))
        elif address == 0x41aef0:
            self.record('delete', 'D:' + self.string(args[0]).hex())
            self.mutate('delete')
            if not self.cause:
                self.ret()
        elif address == 0x460730:
            self.record('title', 'T:' + self.string(self.words(CURRENT, 1)[0]).hex())
            self.mutate('title')
            if not self.cause:
                self.ret()
        elif address == 0x4bd550:
            slot = (mu.reg_read(UC_X86_REG_ECX) - HEADERS) // 20
            self.record('state', 'G:' + str(slot))
            self.mutate('state')
            if not self.cause:
                self.ret(STATES + slot * 28)
        elif address == 0x4414d0:
            index = self.record('config', 'C:' + self.string(args[0]).decode() + ':' + self.string(args[1]).hex())
            self.mutate('config' + str(index))
            if not self.cause:
                self.ret()
        elif not any(low <= address < high for low, high in self.allowed):
            raise RuntimeError('Fort이름검토범위밖: ' + hex(address))

    def sample(self, case, control):
        """원문지역미정바이트까지명시입력으로공급하고모든부분효과를읽는다."""
        self.case = case
        mode, owner, manager, readonly, duplicate, old, new, path, fort, editor, mission, kind, mutation = case
        self.events, self.calls, self.cause = [], Counter(), ''
        self.instructions = 0
        self.other = (owner + 1) % 9
        self.put(OWNER, owner)
        self.put(CURRENT, CURRENTS[0])
        self.put(DIRTY, 0xfffffffe)
        self.put(FORT, fort)
        self.put(EDITOR, editor)
        self.put(MISSION, mission)
        self.put(SPEC_POINTER, NEW_NAME)
        self.mu.mem_write(SPEC, b'fortSpec\0')
        self.mu.mem_write(HEADERS, bytes((0x70 + index) & 255 for index in range(180)))
        # manager0인나머지헤더도부가필드의원문입력을보존한다.
        for slot in range(9):
            self.put(HEADERS + slot * 20, 0)
            self.mu.mem_write(STATES + slot * 28, bytes((0x80 + index) & 255 for index in range(28)))
            self.put(STATES + slot * 28 + 17, kind, 1)
        self.put(HEADERS + self.other * 20, MANAGERS[22])
        self.put(HEADERS + self.other * 20 + 4, 0)
        self.put(HEADERS + self.other * 20 + 8, 0)
        self.put(HEADERS + owner * 20, MANAGERS[manager])
        self.put(HEADERS + owner * 20 + 4, duplicate)
        self.put(HEADERS + owner * 20 + 8, readonly)
        # NUL뒤manager/current/입력바이트도실제복사폭의증거로남긴다.
        for key, base in MANAGERS.items():
            if key:
                self.mu.mem_write(base, bytes((0x91 + index + key) & 255 for index in range(276)))
                self.mu.mem_write(base + 12, f'C:/prior{key}.fort'.encode() + b'\0')
        for index, base in enumerate(CURRENTS):
            self.mu.mem_write(base, b'\xa7' * 256)
            self.mu.mem_write(base, (old_name(old) if not index else b'Other') + b'\0')
        self.mu.mem_write(NEW_NAME, b'\xb6' * 512)
        self.mu.mem_write(NEW_NAME, new_name(new))
        self.argument = CURRENTS[0] if new == 8 else NEW_NAME
        self.mu.mem_write(OLD_PATH, b'Prior\0' + b'\xcd' * 250)
        # 각입력은이전관측의CPU상태를빌리지않는다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.mem_write(STACK, struct.pack('<2I', STOP, self.argument))
        self.mu.reg_write(UC_X86_REG_ECX, HEADERS + owner * 20)
        self.mu.emu_start(0x460980 if mode == 'current' else 0x4bf630, STOP)
        if self.cause:
            self.stops[self.cause] += 1
        else:
            assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + (4 if mode == 'current' else 8)
            self.normal_returns += 1
        state = (self.words(OWNER, 1)[0], CURRENTS.index(self.words(CURRENT, 1)[0]), self.words(DIRTY, 1)[0], self.words(FORT, 1)[0], self.words(EDITOR, 1)[0], self.words(MISSION, 1)[0], int(self.words(SPEC_POINTER, 1)[0] == SPEC), self.calls['snapshot'], self.calls['notify'], self.calls['path'], self.calls['delete'], self.calls['config'])
        raw = bytes(self.mu.mem_read(HEADERS, 180)).hex() + ':' + ':'.join(bytes(self.mu.mem_read(base, 276)).hex() for key, base in MANAGERS.items() if key) + ':' + ':'.join(bytes(self.mu.mem_read(base, 256)).hex() for base in CURRENTS) + ':' + bytes(self.mu.mem_read(NEW_NAME, 512)).hex() + ':' + bytes(self.mu.mem_read(SPEC, 9)).hex() + ':' + (bytes(self.mu.mem_read(OLD_PATH, 256)).hex() if mode == 'current' else '-')
        return '\t'.join((str(control), ','.join(map(str, case)), self.cause or '1', ','.join(map(str, state)), raw, ';'.join(self.events)))


def main():
    """최초독립fixture를제품작성전에고정하고이후에는SHA만확인한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 최초입력/관측을제품수정에맞춰바꾸지못하도록함께고정한다.
        for name, expected in report['sha256'].items():
            if digest(ROOT / name) != expected:
                raise RuntimeError('Fort이름최초 SHA불일치: ' + name)
        print('Fort이름최초원문보존:', report['observations'])
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('Fort이름최초fixture/보고서덮어쓰기금지')
    oracle = Oracle()
    inputs = cases()
    lines = ['# control / mode,owner,manager,readonly,duplicate,old,new,path,fort,editor,mission,kind,mutation / completed-or-stop / current12fields / headers+managers+current+input+spec+scratch / trace']
    # 두정밀도는정수/문자열부모에서도독립실행한다.
    for case in inputs:
        first, second = oracle.sample(case, 0x27f), oracle.sample(case, 0x37f)
        assert first.split('\t')[1:] == second.split('\t')[1:]
        lines.extend((first, second))
    FIXTURE.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_fort_current_name.py', 'cpppj/tests/fixtures/fort-current-name-x86.tsv')
    report = {'status': '신규 독립 준비·미등록', 'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-23', 'entries': ['00460980', '004bf630'], 'ranges': [[f'{low:08x}', f'{high:08x}'] for low, high in RANGES], 'bodyBytes': [551, 91], 'uniqueInputs': len(inputs), 'observations': len(inputs) * 2, 'normalReturns': oracle.normal_returns, 'explicitStops': dict(oracle.stops), 'instructions': oracle.total_instructions, 'precisionDifferences': 0, 'boundaries': dict(oracle.boundaries), 'scope': '실제현재이름/setter의주소재조회/필드쓰기/문자복사/2snapshot/Notify/삭제요청/제목·dirty·설정관문과실제strchr·C로케일stricmp. PathSpec/snapshot/Notify/삭제·제목·State/config/진단은명시경계. oldpath미초기화스택은Prior+명시BYTE입력,256BYTE복사한계와manager0은실제쓰기직전명시중단. 실제파일조작/OS/게임/전체snapshot·Notify본문은실행하지않음.', 'gameExecuted': False, 'osExecuted': False, 'filesMutated': False, 'sha256': {name: digest(ROOT / name) for name in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('uniqueInputs', 'observations', 'normalReturns', 'explicitStops', 'instructions')}))


if __name__ == '__main__':
    main()
