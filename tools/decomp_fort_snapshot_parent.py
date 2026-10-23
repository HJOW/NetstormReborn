#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""전체 Fort/Battle Snapshot 및 현재칸 두 wrapper의 최초 독립 PE 관측이다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

HEADERS, MINE, FORT, BATTLE = 0x5c89f0, 0x540cac, 0x594fb8, 0x594fbc  # 원본 Template/현재 owner/두 모드의 실제 주소다.
FIRST_SLOT, LAST_SLOT = -2, 9  # 물리적으로 별도 준비한 this 관측 범위이며 실제 배열 밖 진단 입력도 포함한다.
MANAGERS = {0: 0, 11: POOL + 0x100, 22: POOL + 0x200}  # 실제 하위 함수 대신 명시 공급하는 manager 식별자다.
STREAMS = POOL + 0x1000  # 하위 Create가 돌려주는 가상 stream 주소이며 부모는 그 메모리에 접근하지 않는다.
ENTRIES = (0x4bf9a0, 0x4bf880, 0x4bfac0, 0x4bfb50)  # 입력 kind0..3의 실제 PE 진입점이다.
RANGES = ((0x4bf880, 0x4bf991), (0x4bf9a0, 0x4bfab8), (0x4bfac0, 0x4bfafb), (0x4bfb50, 0x4bfb98))  # tail jump까지 포함하고 정렬 INT3는 제외한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fort-snapshot-parent-x86.tsv'  # 최초 관측은 덮어쓰지 않는다.
REPORT = ROOT / 'cpppj/fort-snapshot-parent-native.json'  # 실제 부모/명시 child를 구별한 최초 보고서다.


def cases():
    """입력만 작성하며 부모 순서·관문·진단 여부의 기대값은 계산하지 않는다."""
    rows = []
    # 직접 부모는 isMine 일치/불일치와 각 모드·manager0을 서로 교차한다.
    for kind, slot, manager, matching, fort, battle in itertools.product((0, 1), (-2, 0, 1, 8, 9), (0, 11), (0, 1), (0, 1), (0, 1)):
        rows.append((kind, slot, manager, slot & 0xffffffff if matching else 7, fort, battle, 'none'))
    # wrapper는 현재칸을 원문에서 계산하며 두 모드가 모두 켜진 입력도 포함한다.
    for kind, owner, manager, fort, battle in itertools.product((2, 3), (0, 1, 8, 9), (0, 11), (0, 1), (0, 1)):
        rows.append((kind, owner, manager, owner, fort, battle, 'none'))
    changes = ('assert1-manager22', 'assert1-owner8', 'assert1-fort1', 'assert1-battle1', 'assert1-stop',
               'names1-manager0', 'names1-manager22', 'names1-owner8', 'names1-stop',
               'parent1-manager0', 'parent1-manager22', 'parent1-owner8', 'parent1-parent222', 'parent1-return0', 'parent1-stop',
               'create1-manager22', 'create1-manager0', 'create1-parent222', 'create1-current999', 'create1-return0', 'create1-stop',
               'technology1-manager0', 'technology1-owner8', 'technology1-current999', 'technology1-fort0', 'technology1-battle0', 'technology1-stop',
               'deck1-manager0', 'deck1-owner8', 'deck1-fort0', 'deck1-stop',
               'objects1-manager0', 'objects1-owner8', 'objects1-fort1', 'objects1-battle0', 'objects1-stop',
               'territory1-manager0', 'territory1-owner8', 'territory1-stop',
               'create2-manager22', 'create2-current999', 'create2-return0', 'create2-stop',
               'money1-manager0', 'money1-owner8', 'money1-fort1', 'money1-stop',
               'create3-manager22', 'create3-current999', 'create3-return0', 'create3-stop',
               'mission1-manager0', 'mission1-owner8', 'mission1-fort0', 'mission1-fort1', 'mission1-battle0', 'mission1-stop',
               'badges1-owner8', 'badges1-stop', 'core1-owner8', 'core1-fort1', 'core1-fort0', 'core1-stop',
               'notify1-owner8', 'notify1-fort1', 'notify1-fort0', 'notify1-battle1', 'notify1-manager0', 'notify1-stop')
    # 두 wrapper의 두 번째 분기에 앞 child가 끼치는 효과도 원문이 결정한다.
    for kind, change, fort, battle in itertools.product(range(4), changes, (0, 1), (0, 1)):
        rows.append((kind, 1, 0 if change.startswith('assert1-') else 11, 1, fort, battle, change))
    # pointer isMine 비교는 owner DWORD 상위 두 비트가 만든 동일 주소도 구별한다.
    for kind, slot, high in itertools.product(range(4), (0, 1, 8), (0x40000000, 0x80000000, 0xc0000000)):
        rows.append((kind, slot, 11, slot + high, 1, 1, 'none'))
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """네 부모의 실제 본체·메모리 읽기를 실행하며 모든 child는 명시 장치로 둔다."""
    def __init__(self):
        """보호 PE 복사·고정 실행/쓰기 범위와 최초 계수기를 준비한다."""
        super().__init__()
        self.total_instructions = 0
        self.normal_returns = 0
        self.stops = 0
        self.boundaries = Counter()
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)
        self.mu.hook_add(UC_HOOK_MEM_READ, self.read)

    def guard(self, mu, access, address, size, value, data):
        """부모 CPU의 실제 쓰기는 스택 밖으로 나갈 수 없다."""
        if not STACK - 0x1000 <= address < address + size <= STACK + 4:
            raise RuntimeError(f'Snapshot 부모 CPU 쓰기 범위 밖 {address:08x}/{size}')

    def read(self, mu, access, address, size, value, data):
        """실제 CPU의 현재 owner/모드/manager 읽기를 getter 순서로 기록한다."""
        if address in (MINE, FORT, BATTLE):
            if size != 4:
                raise RuntimeError('Snapshot 전역 읽기 폭 오류')
            self.events.append(f'{ {MINE:"owner",FORT:"fort",BATTLE:"battle"}[address]}:{self.words(address, 1)[0]}')
        elif HEADERS + FIRST_SLOT * 20 <= address <= HEADERS + LAST_SLOT * 20 and (address - HEADERS) % 20 == 0:
            if size != 4:
                raise RuntimeError('Snapshot manager 읽기 폭 오류')
            manager = self.manager(self.words(address, 1)[0])
            self.events.append(f'manager:{(address - HEADERS) // 20}:{manager}')

    def manager(self, pointer):
        """명시 manager 포인터를 관측용 ID로만 변환한다."""
        return next(key for key, address in MANAGERS.items() if pointer == address)

    def event(self, kind, text):
        """실제 하위 CALL 진입 순서와 각1기반 횟수를 기록한다."""
        self.events.append(text)
        self.calls[kind] += 1
        self.boundaries[kind] += 1
        return kind + str(self.calls[kind])

    def mutate(self, boundary):
        """지정된 하위 callback의 입력 효과만 공급하며 후속 조건은 원문에 맡긴다."""
        trigger, action = self.case[6].rsplit('-', 1) if '-' in self.case[6] else ('', '')
        if boundary != trigger:
            return
        if action.startswith('manager'):
            self.put(HEADERS + self.active_slot * 20, MANAGERS[int(action[7:])])
        elif action.startswith('owner'):
            self.put(MINE, int(action[5:]))
        elif action.startswith('fort'):
            self.put(FORT, int(action[4:]))
        elif action.startswith('battle'):
            self.put(BATTLE, int(action[6:]))
        elif action.startswith('parent'):
            self.parent = int(action[6:])
        elif action == 'current999':
            self.currents = {key: 999 for key in self.currents}
        elif action == 'stop':
            self.stopped = True
            self.mu.emu_stop()

    def instruction(self, mu, address, size, data):
        """부모의 명령·두 wrapper의 nested CALL은 실제 실행하고 child만 명시 복귀한다."""
        self.instructions += 1
        self.total_instructions += 1
        if self.instructions > 3000:
            raise RuntimeError('Snapshot 부모 명령 상한 초과')
        ecx = mu.reg_read(UC_X86_REG_ECX)
        args = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 4)
        if address in ENTRIES[:2]:
            self.active_slot = (ecx - HEADERS) // 20
        if address == 0x4e0620:
            boundary = self.event('assert', f'assert:{args[2]}')
            self.mutate(boundary)
            if not self.stopped:
                self.ret()
        elif address == 0x48fc60:
            result = self.parent
            boundary = self.event('parent', f'parent:{result}')
            self.mutate(boundary)
            if not self.stopped:
                self.ret(0 if self.case[6] == boundary + '-return0' else result)
        elif address == 0x44d1b0:
            manager, name = self.manager(ecx), self.string(args[0]).decode('ascii')
            if name not in ('Technology', 'Money', 'Mission'):
                raise RuntimeError('Snapshot Create 이름 오류')
            self.created += 1
            token = self.created
            boundary = self.event('create', f'create:{manager}:{name}:{token}')
            self.currents[manager] = token
            self.mutate(boundary)
            if not self.stopped:
                self.ret(0 if self.case[6] == boundary + '-return0' else STREAMS + token * 16, 4)
        elif address in (0x4bcea0, 0x4832c0, 0x463a80):
            if address == 0x4bcea0:
                kind, stream = 'technology', args[1]
                detail = f'technology:{args[0]}:{0 if not stream else (stream - STREAMS) // 16}'
            else:
                kind, stream = ('money' if address == 0x4832c0 else 'mission'), args[0]
                if kind == 'mission' and ecx != 0x562ca0:
                    raise RuntimeError('Snapshot 현재 Guide this 오류')
                detail = f'{kind}:{0 if not stream else (stream - STREAMS) // 16}'
            boundary = self.event(kind, detail)
            self.mutate(boundary)
            if not self.stopped:
                self.ret(purge=4 if kind == 'mission' else 0)
        elif address in (0x4bd6f0, 0x4be430, 0x4bf6d0, 0x4befd0, 0x4be7f0, 0x4beb30, 0x4bd360):
            kind = {0x4bd6f0:'names', 0x4be430:'objects', 0x4bf6d0:'territory', 0x4befd0:'deck', 0x4be7f0:'badges', 0x4beb30:'core', 0x4bd360:'notify'}[address]
            boundary = self.event(kind, f'{kind}:{(ecx - HEADERS) // 20}')
            self.mutate(boundary)
            if not self.stopped:
                self.ret()
        elif not any(low <= address < high for low, high in RANGES):
            raise RuntimeError(f'검토하지 않은 Snapshot 부모 명령 {address:08x}')

    def sample(self, case, control):
        """원본 기대값 없이 전체 입력을 준비하고 실제 trace/최종 메모리를 읽는다."""
        self.case, self.events, self.calls = case, [], Counter()
        self.parent, self.created, self.instructions, self.stopped = 111, 0, 0, False
        self.active_slot = case[1]
        self.currents = {0: 90, 11: 91, 22: 92}
        # 모든 관측 Template의부가16BYTE도고정패턴으로채워예상밖변경을검출한다.
        for slot in range(FIRST_SLOT, LAST_SLOT + 1):
            self.mu.mem_write(HEADERS + slot * 20, bytes(((slot + 2) * 13 + index) & 255 for index in range(20)))
            self.put(HEADERS + slot * 20, MANAGERS[11 if slot % 2 else 22])
        self.put(HEADERS + case[1] * 20, MANAGERS[case[2]])
        self.put(MINE, case[3])
        self.put(FORT, case[4])
        self.put(BATTLE, case[5])
        # 이전 관측의 CPU 상태를 다음 행에 전달하지 않는다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_ECX, HEADERS + case[1] * 20)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.mem_write(STACK, struct.pack('<I', STOP))
        self.mu.emu_start(ENTRIES[case[0]], STOP)
        if self.stopped:
            self.stops += 1
        else:
            assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
            self.normal_returns += 1
        state = [self.words(MINE, 1)[0], self.words(FORT, 1)[0], self.words(BATTLE, 1)[0], self.parent, self.created, *self.currents.values()]
        raw = bytes(self.mu.mem_read(HEADERS + FIRST_SLOT * 20, (LAST_SLOT - FIRST_SLOT + 1) * 20)).hex()
        return '\t'.join((str(control), ','.join(map(str, case)), str(int(not self.stopped)), ','.join(map(str, state)), raw, ';'.join(self.events)))


def main():
    """제품 작성 전에 최초 결과를 고정하거나 이미 고정한 모든 SHA만 검사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        record = json.loads(REPORT.read_text(encoding='utf-8'))
        # 제품 변경으로 원문 최초 근거를 역산 재생성하지 못하게 보존한다.
        for name, expected in record['sha256'].items():
            if digest(ROOT / name) != expected:
                raise RuntimeError('Snapshot 부모 최초 SHA 불일치: ' + name)
        print('Snapshot 부모 최초 SHA 보존:', record['observations'])
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('Snapshot 부모 최초 fixture/보고서 덮어쓰기 금지')
    oracle, inputs = Oracle(), cases()
    lines = ['# control / kind,slot,manager,mine,fort,battle,mutation / completed / 현재전역8DWORD / Template12x20BYTE / 실제메모리읽기와child순서다.']
    # 정수 부모도두x87정밀도에서별도로실행하고관측값차이를검사한다.
    for case in inputs:
        first, second = oracle.sample(case, 0x27f), oracle.sample(case, 0x37f)
        assert first.split('\t')[1:] == second.split('\t')[1:]
        lines.extend((first, second))
    FIXTURE.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_fort_snapshot_parent.py', 'cpppj/tests/fixtures/fort-snapshot-parent-x86.tsv')
    record = {'status':'최초 원문 고정·제품 미작성', 'host':socket.gethostname(), 'lastDecompileHost':'HJOW-Athlon / 2026-10-23', 'entries':[f'{entry:08x}' for entry in ENTRIES], 'ranges':[[f'{low:08x}', f'{high:08x}'] for low, high in RANGES], 'bodyBytes':sum(high-low for low, high in RANGES), 'uniqueInputs':len(inputs), 'observations':len(inputs)*2, 'normalReturns':oracle.normal_returns, 'explicitStops':oracle.stops, 'instructions':oracle.total_instructions, 'precisionDifferences':0, 'boundaries':dict(oracle.boundaries), 'scope':'네 실제 부모의 진단/고정this/현재owner·mode·manager읽기/inlineTechnology·Money·Mission Create/stream캡처/두wrappernested호출/tail복귀. 모든writer·Guide저장·실제DataManager·진단보고는명시외부경계. 제품CPP와기대값을읽지않으며게임/OS실행없음.', 'gameExecuted':False, 'osExecuted':False, 'sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key:record[key] for key in ('uniqueInputs', 'observations', 'normalReturns', 'explicitStops', 'instructions', 'bodyBytes')}))


if __name__ == '__main__':
    main()
