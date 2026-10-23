#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""단위377 준비·미등록: 전체 Objects 저장 부모004be430의 독립 x86 관측이다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

HEADERS = 0x5c89f0  # 실제20BYTE Template 표이며 고정 this의 슬롯을 여기서 계산한다.
VERSION, MINE, FORT = 0x5c89c0, 0x540cac, 0x594fb8  # 부모가 직접 읽고 쓰는 실제 전역이다.
NODES, BUFFERS = POOL + 0x4000, POOL + 0x8000  # 격리된21개16BYTE stream과64BYTE 물리 출력이다.
MANAGERS = {0: 0, 11: POOL + 0x100, 22: POOL + 0x200}  # 명시 하위 장치에서만 사용하는 manager 식별자다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/fort-objects-parent-x86.tsv'  # 최초 원문 결과를 덮어쓰지 않는다.
REPORT = ROOT / 'cpppj/fort-objects-parent-native.json'  # 실제 본체와 명시 하위 경계를 구별한 보고서다.


def cases():
    """고정 슬롯/clamp·진단·존재 여부·빈 선택·현재 manager/version/fort 변경을 교차한다."""
    rows = []
    # 존재 진단과 isMine 진단이 각각 발생하는 입력도 그대로 정상 복귀시킨다.
    for slot, manager, matching, fort, pattern, state in itertools.product((-2, 0, 1, 8, 9), (0, 11), (0, 1), (0, 1), range(3), ((0, 0, 64), (3, 8, 64))):
        rows.append((slot, manager, slot & 0xffffffff if matching else 7, fort, pattern, *state, 'none'))
    mutations = ('assert-manager22', 'find1-manager22', 'create1-manager22', 'temp1-version171', 'territory1-version127', 'territory20-fort1', 'select1-manager22', 'rename1-fort0', 'chaff-version255', 'capacity-current', 'capacity-stop', 'territory1-stop', 'select1-stop')
    # callback은 지정된 한 위치에서만 효과를 내며 반복/조건은 원문이 결정한다.
    for mutation, pattern, state in itertools.product(mutations, range(3), ((0, 0, 64), (0, 0, 0), (3, 0xffffffff, 0xffffffff))):
        rows.append((1, 0 if mutation == 'assert-manager22' else 11, 1, 1, pattern, *state, mutation))
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """실제471바이트 부모만 실행하고 DataManager/영역·Chaff writer를 명시 경계로 둔다."""
    def __init__(self):
        """보호 PE를 가상 메모리에 복사하고 CPU 쓰기 범위를 제한한다."""
        super().__init__()
        self.allowed = ((0x4be430, 0x4be4ad), (0x4be4b0, 0x4be60a))
        self.total_instructions = 0
        self.normal_returns = 0
        self.stops = 0
        self.boundaries = Counter()
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def guard(self, mu, access, address, size, value, data):
        """원본 CPU 쓰기를 지역 스택·명시 stream/출력·version 전역으로만 허용한다."""
        ranges = ((STACK - 0x1800, STACK + 8), (NODES, NODES + 22 * 32), (BUFFERS, BUFFERS + 22 * 64), (VERSION, VERSION + 4))
        if not any(low <= address and address + size <= high for low, high in ranges):
            raise RuntimeError(f'Objects 부모 쓰기 범위 밖 {address:08x}/{size}')

    def event(self, kind, text):
        """명시 실제 하위 진입의 순서와1기반 호출 수를 기록한다."""
        self.events.append(text)
        self.calls[kind] += 1
        self.boundaries[kind] += 1
        return self.calls[kind]

    def mutate(self, boundary):
        """입력으로 지정한 외부효과만 공급하며 부모의 분기·다음manager 선택은 계산하지 않는다."""
        mutation = self.case[8]
        if mutation == 'assert-manager22' and boundary == 'assert':
            self.put(self.this, MANAGERS[22])
        elif mutation == boundary + '-manager22':
            self.put(self.this, MANAGERS[22])
        elif mutation == 'temp1-version171' and boundary == 'temp1':
            self.put(VERSION, 0x123456ab)
        elif mutation == 'territory1-version127' and boundary == 'territory1':
            self.put(VERSION, 0xff00007f)
        elif mutation == 'territory20-fort1' and boundary == 'territory20':
            self.put(FORT, 1)
        elif mutation == 'rename1-fort0' and boundary == 'rename1':
            self.put(FORT, 0)
        elif mutation == 'chaff-version255' and boundary == 'chaff':
            self.put(VERSION, 0x765432ff)
        elif mutation == 'capacity-current' and boundary == 'capacity':
            self.put(self.active_node + 4, 5)
            self.put(self.active_node + 8, 2)
            self.put(self.active_node + 12, 64)
            self.put(VERSION, 0x987654ee)
        elif mutation == boundary + '-stop':
            self.stopped = True
            self.mu.emu_stop()

    def append(self, node, marker):
        """영역/Chaff 하위 writer의 명시 출력1BYTE만 현재 stream에 추가한다."""
        pointer, position, end, capacity = self.words(node, 4)
        if not BUFFERS <= pointer < BUFFERS + 22 * 64 or position >= 64:
            raise RuntimeError('명시 하위 writer의 출력 범위 오류')
        self.mu.mem_write(pointer + position, bytes((marker,)))
        self.put(node + 4, position + 1)
        if struct.unpack('<i', struct.pack('<I', end))[0] < position + 1:
            self.put(node + 8, position + 1)

    def instruction(self, mu, address, size, data):
        """인라인 version/pos/end 쓰기는 실제 명령이며 원문 밖 CALL만 명시 공급한다."""
        self.instructions += 1
        self.total_instructions += 1
        if self.instructions > 9000:
            raise RuntimeError('Objects 부모 명령 상한 초과')
        args = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 5)
        ecx = mu.reg_read(UC_X86_REG_ECX)
        if address == 0x4a89c0:
            assert self.string(args[1]) == b'Terr%02d'
            mu.mem_write(args[0], ('Terr%02d' % args[2]).encode() + b'\0')
            self.ret(args[0])
        elif address == 0x4e0620:
            line = args[2]
            self.event('assert', f'assert:{line}')
            self.active_node = mu.reg_read(UC_X86_REG_ESI)
            self.mutate('capacity' if line == 36 else 'assert')
            if not self.stopped:
                self.ret()
        elif address in (0x44ce60, 0x44d1b0, 0x44ceb0, 0x44d160):
            manager = next(key for key, pointer in MANAGERS.items() if pointer == ecx)
            name = self.string(args[0]).decode()
            if address == 0x44ce60:
                region = int(name[4:])
                found = self.case[4] == 1 or self.case[4] == 2 and region % 2 == 0
                index = self.event('find', f'find:{manager}:{name}')
                self.currents[manager] = region + 1 if found else 0
                self.mutate('find' + str(index))
                self.ret(1 if found else 0, 4)
            elif address == 0x44d1b0:
                index = self.event('create', f'create:{manager}:{name}')
                if name == 'Temp':
                    self.region += 1
                    token = self.region + 1
                    self.temp_calls += 1
                else:
                    token = 21 if name == 'Chaff' else int(name[4:]) + 1
                node = NODES + token * 32
                self.currents[manager] = token
                if name in ('Temp', 'Chaff'):
                    mu.mem_write(node, struct.pack('<4I', BUFFERS + token * 64, *self.case[5:8]))
                self.mutate('create' + str(index))
                if name == 'Temp':
                    self.mutate('temp' + str(self.temp_calls))
                elif name == 'Chaff':
                    self.mutate('chaff')
                self.ret(node, 4)
            elif address == 0x44ceb0:
                assert name == 'Temp' and args[1] == 1
                token = self.region + 1
                node = NODES + token * 32
                index = self.event('select', f'select:{manager}:Temp:1')
                self.currents[manager] = token
                if self.case[4] == 2:
                    if self.region % 3 == 0:
                        self.put(node, 0)
                    elif self.region % 3 == 1:
                        self.put(node + 8, 0)
                self.mutate('select' + str(index))
                if not self.stopped:
                    self.ret(node, 8)
            else:
                source = self.string(args[1]).decode()
                assert source == 'Temp'
                index = self.event('rename', f'rename:{manager}:{name}:{source}')
                self.mutate('rename' + str(index))
                self.ret(purge=8)
        elif address == 0x4be230:
            token = (args[0] - NODES) // 32
            index = self.event('territory', f'territory:{token}:{args[1]}:{args[2]}')
            self.append(args[0], 0x40 + args[2])
            self.mutate('territory' + str(index))
            if not self.stopped:
                self.ret()
        elif address == 0x4be350:
            token = (args[0] - NODES) // 32
            self.event('chaffbody', f'chaff:{token}')
            self.append(args[0], 0x7f)
            self.ret()
        elif not any(low <= address < high for low, high in self.allowed):
            raise RuntimeError(f'검토하지 않은 Objects 부모 명령 {address:08x}')

    def sample(self, case, control):
        """원본 기대 결과 없이 입력 저장소를 준비하고 정상반환/부분중단의 전 상태를 읽는다."""
        self.case = case
        self.this = HEADERS + case[0] * 20
        self.events = []
        self.calls = Counter()
        self.currents = {0: 90, 11: 91, 22: 92}
        self.instructions = 0
        self.stopped = False
        self.region = -1
        self.temp_calls = 0
        self.put(self.this, MANAGERS[case[1]])
        self.put(MINE, case[2])
        self.put(FORT, case[3])
        self.put(VERSION, 0xaabbccdd)
        # 사용하지 않은 stream과 모든 출력의 주변 바이트도 매 입력 초기화한다.
        for token in range(1, 22):
            self.mu.mem_write(NODES + token * 32, struct.pack('<4I', BUFFERS + token * 64, 0, 0, 64))
            self.mu.mem_write(BUFFERS + token * 64, b'\xcc' * 64)
        # 이전 행의 CPU 상태를 다음 관측에 전달하지 않는다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_ECX, self.this)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.mem_write(STACK, struct.pack('<I', STOP))
        self.mu.emu_start(0x4be430, STOP)
        if self.stopped:
            self.stops += 1
        else:
            assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
            self.normal_returns += 1
        manager = next(key for key, pointer in MANAGERS.items() if pointer == self.words(self.this, 1)[0])
        state = [manager, self.words(MINE, 1)[0], self.words(VERSION, 1)[0], self.words(FORT, 1)[0], *self.currents.values()]
        raw = ':'.join(bytes(self.mu.mem_read(NODES + token * 32, 16)).hex() + bytes(self.mu.mem_read(BUFFERS + token * 64, 64)).hex() for token in range(1, 22))
        return '\t'.join((str(control), ','.join(map(str, case)), str(int(not self.stopped)), ','.join(map(str, state)), raw, ';'.join(self.events)))


def main():
    """단위377 최초 독립 원문을 보존하거나 모든 입력·fixture SHA를 읽기 전용 검사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        record = json.loads(REPORT.read_text(encoding='utf-8'))
        # 제품 변경으로 최초 원문 근거를 새 기대값에 맞추지 못하게 모든 입력을 확인한다.
        for name, expected in record['sha256'].items():
            if digest(ROOT / name) != expected:
                raise RuntimeError('Objects 부모 최초 SHA 불일치: ' + name)
        print('Objects 부모 최초 원문 SHA 보존 확인:', record['observations'])
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('최초 Objects 부모 fixture/보고서 덮어쓰기 금지')
    oracle = Oracle()
    rows = cases()
    lines = ['# 단위377 준비·미등록: control / slot,manager,mine,fort,pattern,pos,end,alloc,mutation / completed / 현재상태7DWORD / 21 stream+물리출력 / 실제 호출순서다.']
    # 두 x87 제어어의 결과를 독립 실행하고 integer 부모의 같은 동작도 확인한다.
    for case in rows:
        first = oracle.sample(case, 0x27f)
        second = oracle.sample(case, 0x37f)
        assert first.split('\t')[1:] == second.split('\t')[1:]
        lines.extend((first, second))
    FIXTURE.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_fort_objects_parent.py', 'cpppj/tests/fixtures/fort-objects-parent-x86.tsv')
    report = {'status': '단위377 준비·미등록', 'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-23', 'entry': '004be430', 'bodyBytes': 471, 'uniqueInputs': len(rows), 'observations': len(rows) * 2, 'normalReturns': oracle.normal_returns, 'explicitStops': oracle.stops, 'instructions': oracle.total_instructions, 'precisionDifferences': 0, 'boundaries': dict(oracle.boundaries), 'scope': '실제004be430의20영역/Temp버전BYTE·pos/end/현재manager재조회/빈선택·rename/최종fort조건. Find/Create/Select/Rename·영역4be230·Chaff4be350·서식/진단은 명시 하위 경계. DataManager 실제노드 및 전체영역·Chaff순회/단일청크/파일저장/OS/게임은 실행하지 않는다.', 'gameExecuted': False, 'osExecuted': False, 'sha256': {name: digest(ROOT / name) for name in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('uniqueInputs', 'observations', 'normalReturns', 'explicitStops', 'instructions')}))


if __name__ == '__main__':
    main()
