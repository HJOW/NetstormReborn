#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PATCH 로그인 타이머의 미션 선택/집계/만료/실제Begin·Abort를 격리 관찰한다."""
import csv
import itertools
import json
import struct
import sys
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, PLAYERS, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# 실제 읽기/쓰기 전역과 보고 자료다. 모드는 별도 bool 현재 조회로 제품에 연결한다.
END, TIMEOUT, BEGIN, BREAK, PHASE, CLOCK = 0x5c85f0, 0x5c8938, 0x5c7a74, 0x542544, 0x5c85ac, 0x594fb4
MODES = {0x5c85d4:'saved', 0x540bc0:'server', 0x594fa0:'mission', 0x594fa4:'post', 0x540bc4:'geysers', 0x5e4794:'asserts'}
FLAGS = (0, 1, 2, 3, 0x40, 0x41, 0x42, 0x43, 0x100, 0x101, 0x102, 0x103, 0x140, 0x142, 0x143, 0xffffffff)
COUNTS = (0x490ab0, 0x490a00, 0x490bc0, 0x490b20, 0x490970)  # sub/participating/disconnected/ready/loggedIn의 실제 함수다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/mission-login-timer-x86.tsv'  # 전체 상태와 경계 순서를 보존한다.
REPORT = ROOT / 'cpppj/mission-login-timer.json'  # 정상 반환 관찰과 명시 외부/미복원 범위를 분리한다.


def bits(value):
    """double의 실제8BYTE를 반올림 없는 입력/출력으로 보존한다."""
    return struct.pack('<d', value).hex()


def players(pattern):
    """집계 기대값을 계산하지 않고 subscriber/flags의 명시 원문 입력을 만든다."""
    data = bytearray((index * 37 + pattern * 11) & 255 for index in range(9 * 0xb4))
    for owner in range(9):struct.pack_into('<I', data, owner * 0xb4 + 0x2c, 0);struct.pack_into('<I', data, owner * 0xb4 + 0x50, 0)
    struct.pack_into('<I', data, 0x2c, 0xdeadbeef);struct.pack_into('<I', data, 0x50, 0xffffffff)
    values = {0:(), 1:((1, 7, 3),), 2:((1, 7, 0x43),), 3:((1, 7, 3), (2, 8, 3)), 4:((1, 7, 0x43), (2, 8, 0x102)), 5:((1, 7, 2),), 6:((1, 7, 0x100),), 7:tuple((owner, owner, 0) for owner in range(1, 9)), 8:tuple((owner, owner, 0x43) for owner in range(1, 9)), 9:tuple((owner, 0, 0xffffffff) for owner in range(1, 9))}
    if pattern >= 16:
        ordinal = pattern - 16;owner = ordinal // 48 + 1;sub = (0, 7, 0xffffffff)[ordinal // 16 % 3];values[pattern] = ((owner, sub, FLAGS[ordinal % 16]),)
    for owner, sub, flags in values[pattern]:
        struct.pack_into('<I', data, owner * 0xb4 + 0x2c, sub);struct.pack_into('<I', data, owner * 0xb4 + 0x50, flags)
    return bytes(data)


def inputs():
    """실제 선택 가능한 모드/경계/시간 입력을 수집한다. 결과는 원문으로만 관찰한다."""
    cases = set()
    def add(kind='R', pattern=1, end=10.0, wall=10.0, timeout=30, saved=0, server=1, mission=1, post=1, begin=0, geysers=0, checking=0, connection=1, trigger=0, action=0, value=0, budget=1):
        """명시 정수14개와double2개의 중복 없는 입력을 보존한다."""
        numbers = (pattern, timeout & 0xffffffff, saved, server, mission, post, begin, geysers, checking, connection, trigger, action, value, budget)
        cases.add((kind, ','.join(map(str, numbers)) + '|' + bits(end) + '|' + bits(wall)))
    # 모든 owner/0·음수 subscriber와16flags 조합을 실제5함수에 입력한다. Player0 stale 값은 항상 켜 둔다.
    for pattern in (*range(10), *range(16, 16 + 8 * 48)):add(kind='C', pattern=pattern)
    # 예약은 실제 설정int32와 벽시각의 합이며 기존end/다른 모드를 읽지 않는다.
    for timeout, wall in itertools.product((0, 1, -1, 30, 0x7fffffff, -0x80000000), (0.0, -1.25, 16777217.25)):
        add(kind='S', timeout=timeout, wall=wall)
    # 실제<=경계와NaN/무한대 입력은 직접 만료 함수로 관찰한다.
    for end, wall in itertools.product((0.0, -0.0, 10.0, 10.5, 11.0, float('inf'), float('-inf'), float('nan')), (10.0, float('nan'))):add(kind='E', end=end, wall=wall)
    # inactive0/저장게임/서버·원격/ready·만료/missionPost의 서로 다른 선택이다.
    for pattern, end, saved, server, post, begin, connection in itertools.product(range(10), (0.0, 9.5, 10.0, 10.5), (0, 1), (0, 1), (0, 1), (0, 1), (0, 1)):
        add(pattern=pattern, end=end, saved=saved, server=server, post=post, begin=begin, connection=connection)
    # Begin은 진단/송신이 모드를 바꾸어도 실제 후속쓰기 순서를 유지한다.
    for checking, server, trigger, action in itertools.product((0, 1), (0, 1), (0, 4, 9), (0, 1, 2, 5, 6, 9)):
        add(kind='B', checking=checking, server=server, trigger=trigger, action=action, value=7)
    # Abort 하위는 phase를 다시 바꿀 수 있다. close 뒤에phase4를 재쓰기하지 않는다.
    for trigger, action in itertools.product((0, 7, 11), (0, 5, 6, 8, 9)):
        add(kind='A', begin=1, trigger=trigger, action=action, value=7)
    # 각 외부 경계 뒤 현재 모드/상태를 다시 읽는 사례다. 미복원 일반 초기화 진입은 별도 명시 거부로 남긴다.
    for trigger, action, post in itertools.product(range(1, 12), (1, 2, 3, 5, 6, 7, 8, 9), (0, 1)):
        add(pattern=3, trigger=trigger, action=action, value=7, post=post)
    # connection 호출 뒤 server를 재조회하며 원격경로에서도 missionPost 현재값을 읽는다.
    for action in (1, 2, 3, 5, 6, 7):add(server=0, trigger=2, action=action, value=1)
    # 만료 판정은 wall 경계 뒤 같은end를 다시 읽는다.
    for action in (5, 6, 7):add(kind='E', trigger=10, action=action, value=11)
    # 미션 분기는 initialGeysers가 켜져도 일반 랜덤생성을 호출하지 않는다.
    for saved, post in itertools.product((0, 1), (0, 1)):add(pattern=2, saved=saved, post=post, geysers=1)
    return sorted(cases)


class UnsupportedGeneral(RuntimeError):
    """미복원 일반 전투 초기화에 실제 도달한 입력은 성공 fixture로 저장하지 않는다."""


class TimerOracle(MemoryOracle):
    """실제 타이머/집계/만료/Begin/Abort/패킷writer만 실행하고 시간·전송·사제·UI는 명시 외부다."""
    def __init__(self):
        """같은 PC 전체 정밀 함수표의11몸체만 허용하고 실제 전역 쓰기/모드 읽기를 감시한다."""
        super().__init__();self.allowed = [];self.calls = {};self.effects = {}
        selected = (*COUNTS, 0x4180a0, 0x4b61f0, 0x4b6780, 0x4b67c0, 0x40b010, 0x4df230)
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream, delimiter='\t'):
                if int(row['entry'], 16) in selected:
                    for part in row['ranges'].split(';'):
                        low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))
        self.selected = selected;self.mu.hook_add(UC_HOOK_MEM_READ, self.read);self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def snap(self):
        """외부 경계 시점의 실제쓰기 순서를 원문 상태로 함께 보존한다."""
        return bytes(self.mu.mem_read(END, 8)).hex() + ',' + ','.join(str(self.words(address, 1)[0]) for address in (BEGIN, BREAK, PHASE, CLOCK))

    def read(self, mu, access, address, size, value, data):
        """실제 모드 DWORD 읽기와 현재 bool getter의 호출 순서를 일대일로 관찰한다."""
        if address in MODES:
            if size != 4:raise RuntimeError('로그인 모드 읽기 폭 오류')
            self.events.append('G,' + MODES[address] + ',' + str(int(self.words(address, 1)[0] != 0)))

    def guard(self, mu, access, address, size, value, data):
        """실제 명령은 한정스택과 타이머/Begin/Abort의 지정 전역에만 쓸 수 있다."""
        if STACK - 0x2000 <= address and address + size <= STACK + 8:return
        if END <= address and address + size <= END + 8:return
        if any(item <= address and address + size <= item + 4 for item in (TIMEOUT, BEGIN, BREAK, PHASE, CLOCK)):return
        raise RuntimeError(f'로그인 타이머 원문쓰기 범위 밖 {address:08x}/{size}')

    def mutate(self, trigger):
        """실제 외부효과의 명시 변경 입력만 적용한다. 타이머/ready 결과는 계산하지 않는다."""
        if trigger != self.numbers[10] or self.budget == 0:return
        self.budget -= 1;action, value = self.numbers[11:13]
        toggles = {1:0x540bc0, 2:0x594fa4, 3:0x5c85d4, 4:0x594fa0}
        if action in toggles:self.put(toggles[action], int(self.words(toggles[action], 1)[0] == 0))
        elif action == 5:self.put(BEGIN, value)
        elif action == 6:self.mu.mem_write(END, struct.pack('<d', float(value)))
        elif action == 7:self.put(PLAYERS + 0xb4 + 0x50, 0x43)
        elif action == 8:self.put(PHASE, value)
        elif action == 9:self.put(BREAK, value)

    def event(self, text, trigger=0):
        """로그와 외부효과 진입 시점의 상태를 기록한 다음 지정 외부 입력 변경을 적용한다."""
        self.events.append(text + ',' + self.snap());self.mutate(trigger)

    def instruction(self, mu, address, size, data):
        """실제 명령/원본 패킷writer와 외부 큰효과 대체를 구분하고 미검토 진입은 거부한다."""
        if address in self.selected:self.calls[address] = self.calls.get(address, 0) + 1
        if address in (0x45ad00, 0x4a68e0, 0x441090, 0x460d70, 0x4e78d0, 0x4e2b10, 0x4b6180, 0x4e2e30, 0x495060, 0x4b54e0, 0x4c8d90, 0x4340d0, 0x4b6350, 0x4e0620, 0x4b68d6):
            self.effects[address] = self.effects.get(address, 0) + 1
        esp = mu.reg_read(UC_X86_REG_ESP);args = self.words(esp + 4, 3)
        if address in (0x45ad00, 0x4a68e0):
            self.event('L,' + str(int(address == 0x4a68e0)) + ',' + self.string(args[0]).hex(), 8 if address == 0x4a68e0 else 0);self.ret();return
        if address == 0x441090:
            if self.string(args[0]) != b'loginTimeout':raise RuntimeError('미검토 로그인 설정 키')
            self.event('Q');self.ret(self.numbers[1]);return
        if address == 0x460d70:
            self.event('W', 10);mu.mem_write(args[0], self.wall);self.ret(args[0]);return
        if address == 0x4e78d0:self.event('C', 1);self.ret(0x87654321);return
        if address == 0x4e2b10:
            if args[0] != 9:raise RuntimeError('로그인 연결 번호 오류')
            self.event('N,9', 2);self.ret(self.numbers[9]);return
        if address == 0x4b6180:
            if args != (9, 0, 0):raise RuntimeError('로그인 원격 상태 인자 오류')
            self.event('R,9,0,0', 3);self.ret();return
        if address == 0x4e2e30:
            packet = bytes(mu.mem_read(args[0], 8))
            if packet != bytes.fromhex('016e080000000080'):raise RuntimeError('로그인 Begin 실제 packet 오류')
            self.event('B,' + packet.hex(), 4);self.ret();return
        if address == 0x495060:self.event('P,' + str(args[0]), 5);self.ret();return
        if address == 0x4b54e0:self.event('S', 6);self.ret();return
        if address == 0x4c8d90:
            if args[0] != 0x5c85b0:raise RuntimeError('로그인 진행 창 인자 오류')
            self.event('X', 7);self.ret();return
        if address == 0x4340d0:self.event('V');self.ret();return
        if address == 0x4b6350:self.event('A', 11);self.ret();return
        if address == 0x4e0620:
            if args != (0x507248, 0x5134a4, 0x1153):raise RuntimeError('로그인 Begin 서버 진단 인자 오류')
            self.event('J', 9);self.ret();return
        if address == 0x4b68d6:
            # 미복원 일반 진행 UI꼬리는 명시 경계로 호출하고 원본 정상 epilogue로만 이동한다.
            self.event('U');mu.reg_write(UC_X86_REG_EIP, 0x4b6dbb);return
        if address in (0x4b4a20, 0x486440):raise UnsupportedGeneral('실제 일반 초기화 선택')
        if not any(low <= address and address + size <= high for low, high in self.allowed):raise RuntimeError(f'로그인 타이머 미검토 명령 {address:08x}')

    def sample(self, case):
        """명시 입력 뒤 각 실제 호출의 정상 반환/스택/보존4레지스터와 모든 상태를 읽는다."""
        kind, values = case;fields = values.split('|');self.numbers = [int(value) for value in fields[0].split(',')];self.wall = bytes.fromhex(fields[2]);self.budget = self.numbers[13];self.events = []
        self.mu.mem_write(PLAYERS, players(self.numbers[0]));self.put(0x59534c, PLAYERS);self.mu.mem_write(END, bytes.fromhex(fields[1]))
        for address, value in ((TIMEOUT, 0x76543210), (BEGIN, self.numbers[6]), (BREAK, 0xabcdef01), (PHASE, 0x11223344), (CLOCK, 0x12345678)):self.put(address, value)
        for address, value in zip(MODES, (self.numbers[2], self.numbers[3], self.numbers[4], self.numbers[5], self.numbers[7], self.numbers[8])):self.put(address, value)
        entries = COUNTS if kind == 'C' else (0x4b61f0 if kind == 'B' else 0x4b6780 if kind == 'A' else 0x4180a0 if kind == 'E' else 0x4b67c0,)
        results = [];preserved = {UC_X86_REG_EBX:0x11112222, UC_X86_REG_ESI:0x33334444, UC_X86_REG_EDI:0x55556666, UC_X86_REG_EBP:0x77778888}
        for entry in entries:
            for register, value in preserved.items():self.mu.reg_write(register, value)
            self.mu.reg_write(UC_X86_REG_EAX, 0xccccdddd);self.mu.reg_write(UC_X86_REG_EDX, 0xaaaabbbb);self.mu.reg_write(UC_X86_REG_ECX, END)
            self.mu.reg_write(UC_X86_REG_FPCW, 0x27f);self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff);self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.mem_write(STACK, struct.pack('<II', STOP, int(kind == 'S')))
            self.mu.emu_start(entry, STOP, count=100000)
            if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4:raise RuntimeError('로그인 타이머 정상 반환/스택 오류')
            for register, value in preserved.items():
                if self.mu.reg_read(register) != value:raise RuntimeError('로그인 타이머 보존레지스터 오류')
            if kind in ('C', 'E'):results.append(str(self.mu.reg_read(UC_X86_REG_EAX)))
        state = bytes(self.mu.mem_read(END, 8)).hex() + '|' + ','.join(str(self.words(address, 1)[0]) for address in (TIMEOUT, BEGIN, BREAK, PHASE, CLOCK, *MODES))
        return ','.join(results) or '-', state, ';'.join(self.events) or '-'


def main():
    """정상 관찰 fixture와 명시 미복원 범위를 보존하거나 현재 SHA만 감사한다."""
    if sys.argv[1:] == ['--verify']:
        facts = json.loads(REPORT.read_text(encoding='utf-8'))
        for name, expected in facts['sha256'].items():
            if digest(ROOT / name) != expected:raise RuntimeError('로그인 타이머 SHA 불일치: ' + name)
        rows = list(csv.reader(FIXTURE.open(encoding='utf-8'), delimiter='\t'))[1:]
        if len(rows) != facts['observations'] or len({tuple(row[:2]) for row in rows}) != len(rows):raise RuntimeError('로그인 타이머 고유입력 수 오류')
        print('Mission login timer native/SHA verified:', len(rows));return
    if sys.argv[1:]:raise SystemExit('사용법: decomp_mission_login_timer.py [--verify]')
    oracle = TimerOracle();rows = [];rejected = []
    for case in inputs():
        try:rows.append((*case, *oracle.sample(case)))
        except UnsupportedGeneral:rejected.append(case)
    with FIXTURE.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n');writer.writerow(('kind', 'input', 'result', 'state', 'trace'));writer.writerows(rows)
    sources = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_mission_login_timer.py', 'cpppj/src/o/RawMissionLoginTimer.h', 'cpppj/src/o/RawMissionLoginTimer.cpp', 'cpppj/tests/MissionLoginTimerTests.cpp', 'cpppj/tests/fixtures/mission-login-timer-x86.tsv')
    facts = {'host':'HJOW-Athlon', 'date':'2026-10-15', 'status':'native-observed-product-unbuilt', 'observations':len(rows), 'kinds':{kind:sum(row[0] == kind for row in rows) for kind in 'RSEBAC'}, 'native_calls':{f'{address:08x}':count for address, count in oracle.calls.items()}, 'explicit_external_boundaries':{f'{address:08x}':count for address, count in oracle.effects.items()}, 'explicit_general_initialization_rejections':len(rejected), 'normal_return_all_saved_observations':True, 'stack_and_preserved_registers_checked':True, 'native_write_guard':'bounded stack, end/timeout/begin/scriptBreak/requestPhase/clock only', 'progress_tail':'explicit external boundary, then actual original epilogue', 'game_gui_os_calls':0, 'sha256':{name:digest(ROOT / name) for name in sources if (ROOT / name).exists()}}
    REPORT.write_text(json.dumps(facts, ensure_ascii=False, indent=2)+'\n', encoding='utf-8');print('Mission login timer native observations:', len(rows), 'unsupported general:', len(rejected))


if __name__ == '__main__':main()
