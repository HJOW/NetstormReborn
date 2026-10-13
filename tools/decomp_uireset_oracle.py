#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 Gump 전체 삭제 요청·루트 교체와 실제 하위 수명을 격리 실행한다. 게임/GUI/OS는 실행하지 않는다."""
import argparse
import collections
import copy
import csv
import json
import random
import socket
import struct
from decomp_gumplifetime_oracle import (GumpLifetimeOracle, SPECS as LIFETIME_SPECS,
    DATA, SIZE, COUNT, TOKENS, VTABLE, VIRTUAL, node, changed, state, encoded)
from decomp_process_oracle import (ProcessOracle, ROOT, BINARIES, CONTROLS, STACK, STOP,
    STUBS, POOL, digest, UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW,
    UC_X86_REG_FPSW)
from decomp_drawsort_oracle import REGISTERS

# 실제 두 PE의 전체 등록표 삭제 요청/루트 교체·serial·제외 타입 BYTE 주소다.
SPECS = {
    'originals': dict(KillExceptType=0x4666d0, ReplaceRoot=0x4666a0,
        serial=0x540418, excluded=0x540426),
    'originalCD': dict(KillExceptType=0x492a60, ReplaceRoot=0x492a20,
        serial=0x535e98, excluded=0x535ed4),
}
# 완성 입력 재현 시드와 실제 관찰을 저장하는 독립 fixture/근거 파일이다.
SEED = 0x4666d078
FIXTURE = ROOT / 'cpppj/tests/fixtures/uireset-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-uireset-evidence.json'
PLAN = ROOT / 'tools/ghidra/uireset-functions.json'


def plan(edition):
    """현재 표/제외 타입/콜백 재진입과 기존 루트 큐 정리의 완성 입력만 만든다."""
    result = []
    a, b, c, d, e, f = TOKENS

    def add(actions, raw=None, responses=(), binding=0):
        """행마다 새 표/전역을 준비하며 예상 삭제·반환 결과는 계산하지 않는다."""
        tables = [[0] * size for size in (1000, LIFETIME_SPECS[edition]['idle_size'], 100, 1000, 16)]
        tables[0][:COUNT] = TOKENS
        values = raw or [node(i, **{'32': 11 if i in (2, 4) else 20 + i}) for i in range(COUNT)]
        case = dict(actions=list(actions), nodes=list(values), tables=tables,
            globals=[6, 0, 0, b, b, c, 0, 0x7fffffff, 11],
            responses=list(responses), binding=binding)
        result.append(case)
        return case

    add([('KillExceptType', 0)])
    # 실제 count와 visible/void/미등록 플래그가 전체 표 순회의 추가 필터가 되지 않아야 한다.
    for count in (0, 1, 6, 999, 0xffffffff):
        # dead만 삭제 요청을 생략한다. 다른 플래그는 실제 하위 Kill에서 처리한다.
        for flags in (0, 1, 8, 9, 0x10009, 0x100009, 0x80000009):
            raw = [node(i, **{'4': flags, '32': 11 if i == 2 else 20 + i}) for i in range(COUNT)]
            case = add([('KillExceptType', 0)], raw); case['globals'][0] = count
    # signed BYTE의 부호 확장은 +20 DWORD 전체와 비교하며 하위 BYTE만 비교하지 않는다.
    for excluded in (0, 11, 127, 128, 255):
        # 동일 하위 BYTE지만 다른 상위 비트도 별도 입력으로 공급한다.
        for command in (excluded, (excluded if excluded < 128 else excluded - 256) & 0xffffffff, 0x12340000 | excluded):
            raw = [node(i, **{'32': command}) for i in range(COUNT)]
            case = add([('KillExceptType', 0)], raw); case['globals'][8] = excluded
    # 첫/마지막·공백·중복 칸은 등록 개수와 무관하게 방문한다. 루트 칸은 제외한다.
    for slot in (1, 2, 998, 999):
        case = add([('KillExceptType', 0)]); case['tables'][0] = [0] * 1000
        case['tables'][0][0] = a; case['tables'][0][slot] = b
    case = add([('KillExceptType', 0), ('KillExceptType', 0)])
    case['tables'][0][999] = b
    # Kill 콜백의 과거/미래 표 쓰기와 타입 BYTE 쓰기는 명시 응답이며 기대 호출 순서는 넣지 않는다.
    raw = [node(i, **{'4': 1, '32': 20 + i}) for i in range(COUNT)]
    add([('KillExceptType', 0)], raw,
        [('Push', b, ((1, 1, f), (1, 2, 0), (1, 999, d), (0, 0, 0)))])
    raw[3] = changed(raw[3], **{'32': 0xffffff80})
    add([('KillExceptType', 0)], raw,
        [('Push', b, ((0, 8, 128), (c, 4, 0x10001)))])
    add([('KillExceptType', 0)], raw,
        [('Push', b, ((d, 32, 11), (e, 4, 0x10001), (1, 4, 0)))])
    # 자식 Kill이 미래 등록 칸의 노드를 dead로 만든 경우 부모 우선 표 방문에서 다시 요청하지 않는다.
    raw = [node(i, **{'32': 20 + i}) for i in range(COUNT)]
    raw[1] = changed(raw[1], **{'24': c}); raw[2] = changed(raw[2], **{'12': d})
    add([('KillExceptType', 0)], raw)
    # 실제 하위 Cleanup·기본 scalar 소멸자도 실행하여 중복 큐/텍스트·표 정리를 관찰한다.
    add([('KillExceptType', 0), ('Cleanup', 0)], binding=1)

    # 새 루트의 기존 등록 칸은 helper에서 제거하지 않으며 serial을 초기화하지 않는다.
    for binding in (0, 1):
        # 루트가 없는 경우에도 이미 대기 중인 삭제 큐는 정리한다.
        for old_root in (0, a):
            case = add([('ReplaceRoot', f)], binding=binding)
            case['tables'][0][0] = old_root
        case = add([('ReplaceRoot', f)], binding=binding)
        case['tables'][0][0] = 0; case['tables'][3][0] = b; case['globals'][1] = 1
        case['nodes'][1] = changed(case['nodes'][1], **{'4': 0x10009})
    # 상위 Uber의 새노드 등록칸 제거 입력을 그대로 공급하며 count 감소를 대신 계산하지 않는다.
    case = add([('ReplaceRoot', f)], binding=1)
    case['tables'][0][5] = 0; case['nodes'][5] = changed(case['nodes'][5], **{'28': 0})
    raw = [node(i, **{'4': 1, '32': 20 + i}) for i in range(COUNT)]
    raw[0] = changed(raw[0], **{'24': b}); raw[1] = changed(raw[1], **{'12': c, '24': d})
    # 정상 부착 객체의 가상 Push 응답이 부모 링크를 분리한다. 뒤 Cleanup은 실제 기본 소멸자를 실행한다.
    add([('ReplaceRoot', f)], raw, [('Push', d, ((b, 24, 0),)),
        ('Push', b, ((a, 24, c), (b, 12, 0))), ('Push', c, ((a, 24, 0),))], binding=1)
    raw = [node(i, **{'32': 20 + i}) for i in range(COUNT)]
    raw[0] = changed(raw[0], **{'4': 1})
    add([('ReplaceRoot', f)], raw, [('Push', a, ((1, 0, b), (f, 28, 777), (0, 7, 0xdeadbeef)))])
    # 소멸자 응답 뒤에도 현재 큐를 끝까지 정리한 뒤 new.id=0을 마지막에 쓴다.
    add([('ReplaceRoot', f)], responses=[('Destructor', a, ((f, 28, 999), (0, 7, 0xffffffff)))])
    case = add([('ReplaceRoot', f)], responses=[('Destructor', a, ((4, 1, b), (0, 1, 2)))])
    case['nodes'][1] = changed(case['nodes'][1], **{'4': 0x10009})
    add([('ReplaceRoot', f), ('ReplaceRoot', a)], binding=1)

    rng = random.Random(SEED)
    # 유효하고 순환 없는 노드/표를 무작위로 공급하여 현재 표 필터와 signed type의 경계를 교차한다.
    for index in range(48):
        raw = [node(i, **{'4': rng.choice((0, 1, 8, 9, 0x10009, 0x100009)),
            '32': rng.choice((0, 11, 127, 128, 0xffffff80, 0xffffffff))}) for i in range(COUNT)]
        case = add([('KillExceptType', 0)], raw)
        case['globals'][8] = rng.choice((0, 11, 127, 128, 255)); case['globals'][0] = rng.getrandbits(32)
        case['tables'][0] = [0] * 1000; case['tables'][0][0] = a
        # 같은 토큰이 중복 등록된 비정상 표도 정상 KillExceptType 반환 범위의 입력으로 보존한다.
        for _ in range(10): case['tables'][0][rng.randrange(1, 1000)] = rng.choice(TOKENS[1:])
    return result


class GumpResetOracle(GumpLifetimeOracle):
    """원본 reset 두 몸체와 실제 하위 수명을 실행하고 부모 도구의 가상 경계만 재사용한다."""
    def __init__(self, edition):
        """현재 PC의 고유 정밀 내보내기에서 검토된 불연속 몸체만 허용한다."""
        ProcessOracle.__init__(self, edition)
        self.edition = edition; self.d = dict(LIFETIME_SPECS[edition], **SPECS[edition])
        self.allowed = []; self.entry_names = {}
        self.extra = [ROOT / f'extracted/uireset/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        allowed_entries = {self.d[name] for name in ('KillExceptType', 'ReplaceRoot', 'Kill', 'Cleanup', 'Destroy', 'Base', 'Scalar', 'Idle', 'Overlay', 'Dialog')}
        allowed_entries.add(0x4e5b40 if edition == 'originals' else 0x4f1a60)
        if edition == 'originalCD': allowed_entries.update((0x493a20, 0x493b00))
        # 호출되는 실제 정상 하위 몸체만 허용하며 State/Uber/allocator는 실행하지 않는다.
        with self.extra[1].open(encoding='utf-8') as fp:
            for row in csv.DictReader(fp, delimiter='\t'):
                address = int(row['entry'], 16)
                if address not in allowed_entries: continue
                self.entry_names[address] = row['entry']
                # 함수 사이 빈 바이트는 실행 허용 범위에 포함하지 않는다.
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-')); self.allowed.append((low, high + 1))
        if set(self.entry_names) != allowed_entries: raise RuntimeError('UI reset 내보내기 몸체 누락')
        self.mu.mem_map(DATA, 0x10000)
        self.global_addresses = [self.d[name] for name in ('count', 'kill_count', 'dialog_count', 'focus', 'override', 'current', 'diagnostics', 'serial', 'excluded')]
        self.table_addresses = [self.d[name] for name in ('registry', 'idle', 'overlay', 'kill', 'dialogs')]
        self.table_sizes = [1000, self.d['idle_size'], 100, 1000, 16]
        self.returns, self.boundaries = collections.Counter(), collections.Counter()

    def state(self):
        """serial과 제외 타입 BYTE까지 포함한 현재 전체 스칼라/표 상태를 기록한다."""
        values = [self.u32(address) for address in self.global_addresses[:-1]]
        values.append(self.mu.mem_read(self.global_addresses[-1], 1)[0])
        return state(values, [struct.unpack('<' + 'I' * size, self.mu.mem_read(address, size * 4))
            for address, size in zip(self.table_addresses, self.table_sizes, strict=True)])

    def respond(self, kind, token, args):
        """전체 현재 상태를 기록하고 명시 입력만 쓰며 제외 타입은 BYTE 폭을 보존한다."""
        self.calls.append(','.join(map(str, (kind, token, *args))) + '@' + self.state() + '@' + self.nodes())
        self.boundaries[kind] += 1
        # 응답 쓰기 위치/값은 완성 입력이다. 기대 필터·삭제 상태는 계산하지 않는다.
        for target, offset, value in self.responses.get((kind, token), ()):
            address = self.global_addresses[offset] if target == 0 else self.table_addresses[target - 1] + offset * 4 if target <= 5 else target + offset
            self.put(address, value, 1 if target == 0 and offset == 8 else 4)

    def run(self, case, control):
        """cdecl reset·실제 thiscall 수명의 정상 반환과 전체 노드/표/콜백 상태를 관찰한다."""
        mu, d = self.mu, self.d
        self.entries, self.row_returns = collections.Counter(), collections.Counter()
        self.calls, self.results = [], []
        self.responses = {(kind, token): patches for kind, token, patches in case['responses']}
        mu.mem_write(DATA, b'\xa5' * 0x1000)
        # 전체 입력 객체와 스칼라는 raw 바이트 그대로 준비한다.
        for token, raw in zip(TOKENS, case['nodes'], strict=True): mu.mem_write(token, raw)
        for index, (address, value) in enumerate(zip(self.global_addresses, case['globals'], strict=True)):
            self.put(address, value, 1 if index == 8 else 4)
        # 표마다 실제 활성 길이만 써서 인접 전역을 보존한다.
        for address, values in zip(self.table_addresses, case['tables'], strict=True):
            mu.mem_write(address, struct.pack('<' + 'I' * len(values), *values))
        self.put(d['pool'], POOL); mu.mem_write(POOL, b'\0' * 0x1000)
        # 실제 기본 소멸자는 binding=1에서 PE 몸체를 실행한다. 나머지 세 가상 경계만 입력 응답이다.
        for offset, address in ((0, d['Scalar'] if case['binding'] else STUBS + 0xa20), (16, STUBS + 0xa40), (24, STUBS + 0xa00)):
            self.put(VTABLE + offset, address)
        for sid in range(COUNT): self.put(POOL + sid * d['stride'], VTABLE)
        pool_before = bytes(mu.mem_read(POOL, 0x1000)); vtable_before = bytes(mu.mem_read(VTABLE, 64))
        type_neighbors = bytes(mu.mem_read(d['excluded'] - 1, 5))
        self.writes = [(0, 4), (STACK - 0xf000, STACK + 0x1000)]
        self.writes.extend((token, token + SIZE) for token in TOKENS)
        self.writes.extend((address, address + (1 if index == 8 else 4)) for index, address in enumerate(self.global_addresses))
        self.writes.extend((address, address + size * 4) for address, size in zip(self.table_addresses, self.table_sizes, strict=True))
        occupied = set()
        # 인접 표의 다른 실제 전역을 제외한 미사용 바이트는 독립 보호값을 공급한다.
        for low, high in self.writes: occupied.update(range(low, high))
        guards = sorted({p for address, size in zip(self.table_addresses, self.table_sizes, strict=True)
            for p in (*range(address - 8, address), *range(address + size * 4, address + size * 4 + 8)) if p not in occupied})
        for address in guards: mu.mem_write(address, b'\xa5')
        # 연속 연산 사이에는 노드/전역을 유지하고 호출 프레임/보존 레지스터만 초기화한다.
        for action in case['actions']:
            op, token = action[:2]
            args = (token,) if op == 'ReplaceRoot' else ()
            for reg, value in REGISTERS.items(): mu.reg_write(reg, value)
            for reg in (UC_X86_REG_EAX, UC_X86_REG_EDX): mu.reg_write(reg, 0)
            mu.reg_write(UC_X86_REG_ECX, token); mu.reg_write(UC_X86_REG_ESP, STACK); mu.reg_write(UC_X86_REG_EFLAGS, 2)
            mu.reg_write(UC_X86_REG_FPCW, control); mu.reg_write(UC_X86_REG_FPSW, 0); self.put(0, 0xffffffff)
            mu.mem_write(STACK, struct.pack('<' + 'I' * (len(args) + 1), STOP, *args))
            mu.emu_start(d[op], STOP, timeout=30000000, count=2000000)
            if mu.reg_read(UC_X86_REG_EIP) != STOP or mu.reg_read(UC_X86_REG_ESP) != STACK + 4 or any(mu.reg_read(reg) != value for reg, value in REGISTERS.items()):
                raise RuntimeError('UI reset 정상 반환/ABI 오류')
            if mu.reg_read(UC_X86_REG_FPSW) & 0x3800 or mu.reg_read(UC_X86_REG_FPCW) != control or self.u32(0) != 0xffffffff:
                raise RuntimeError('UI reset x87/SEH 오류')
            self.row_returns[f'{d[op]:08x}'] += 1; self.returns[f'{d[op]:08x}'] += 1; self.results.append('-')
        protected = bytearray(mu.mem_read(DATA, 0x1000))
        # 노드 출력만 보호값 대조에서 제외한다. 가상 표/Pool은 읽기 전용이다.
        for token in TOKENS: protected[token - DATA:token - DATA + SIZE] = b'\xa5' * SIZE
        neighbors_after = bytes(mu.mem_read(d['excluded'] - 1, 5))
        if protected != b'\xa5' * 0x1000 or any(bytes(mu.mem_read(p, 1)) != b'\xa5' for p in guards): raise RuntimeError('UI reset 보호 바이트 변경')
        if pool_before != bytes(mu.mem_read(POOL, 0x1000)) or vtable_before != bytes(mu.mem_read(VTABLE, 64)): raise RuntimeError('UI reset 읽기 전용 입력 변경')
        if type_neighbors[:1] + type_neighbors[2:] != neighbors_after[:1] + neighbors_after[2:]: raise RuntimeError('UI reset 타입 BYTE 인접 바이트 변경')
        return [*encoded(case), self.nodes(), self.state(), '/'.join(self.results), '|'.join(self.calls) or '-',
            json.dumps(dict(self.entries), sort_keys=True, separators=(',', ':')),
            json.dumps(dict(self.row_returns), sort_keys=True, separators=(',', ':'))]


def generate(smoke):
    """두 정밀도 관찰이 같을 때만 독립 fixture를 저장한다. C++ 코드는 읽거나 호출하지 않는다."""
    rows, editions = [], {}
    files = {FIXTURE, PLAN, ROOT / 'tools/decomp_uireset_oracle.py', ROOT / 'tools/decomp_gumplifetime_oracle.py',
        ROOT / 'tools/decomp_process_oracle.py', ROOT / 'tools/decomp_drawsort_oracle.py'}
    # 원본 두 파일을 각각 독립 가상 메모리에 읽고 같은 완성 입력의 실제 몸체를 실행한다.
    for edition in SPECS:
        oracle = GumpResetOracle(edition); indexed = list(enumerate(plan(edition)))
        if smoke: indexed = indexed[::13] + indexed[-4:]
        for index, case in indexed:
            try: observed = [oracle.run(case, control) for control in CONTROLS]
            except Exception as error: raise RuntimeError(f'UI reset 입력 {edition}/{index}: {case["actions"]}') from error
            if observed[0] != observed[1]: raise RuntimeError(f'UI reset x87 차이: {edition} {index}')
            rows.append([edition, str(index), *observed[0]])
        editions[edition] = dict(cases=len(indexed), entries=dict(oracle.hits), returns=dict(oracle.returns), boundaries=dict(oracle.boundaries))
        files.update(oracle.exports + oracle.extra); files.add(ROOT / BINARIES[edition])
        print(f'{edition}: {len(indexed)}개 통과', flush=True)
    if smoke: return
    FIXTURE.write_text('# 두 실제 PE의 Gump 전체 Kill 요청/루트 교체·실제 하위 수명 관찰. 게임/GUI/OS는 실행하지 않는다.\n' +
        '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows), host=socket.gethostname(), date='2026-10-13', controls=list(CONTROLS), os_calls=0, assertions=0,
        editions=editions, files={p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(files)},
        limits=['두 reset 실제 몸체와 Kill/Cleanup/Destroy/기본 scalar/CRT 복사 전체 정상 몸체',
            '전체 여섯 0xa0바이트 노드/다섯 표/serial/현재 제외 signed BYTE와 가상 호출 당시 상태 관찰',
            '가상 Push/파생 소멸자/Form/free는 명시 응답. State 전체·Uber 파생·실제 allocator/OS/게임/assert 실행은 제외',
            '정상 반환/보존 레지스터/스택/x87/SEH/허용 불연속 실행·쓰기/읽기 전용/보호 바이트 감사']),
        ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def verify():
    """저장 SHA/완성 입력/폭/실제 정상 반환·진입·경계 합계를 독립 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 읽은 PE/내보내기/도구/fixture의 바이트가 생성 당시와 같은지 확인한다.
    for name, expected in report['files'].items():
        if digest(ROOT / name) != expected: raise RuntimeError('UI reset SHA 불일치: ' + name)
    if report['total'] != len(rows) or report['controls'] != list(CONTROLS) or report['os_calls'] or report['assertions'] or set(report['editions']) != set(SPECS):
        raise RuntimeError('UI reset 총계 오류')
    # 저장된 실제 관찰만 합산한다. 기대 필터/표 결과는 다시 계산하지 않는다.
    for edition, item in report['editions'].items():
        chosen = [row for row in rows if row[0] == edition]
        entries, returns, boundaries = collections.Counter(), collections.Counter(), collections.Counter()
        if len(chosen) != len(plan(edition)) or item['cases'] != len(chosen): raise RuntimeError('UI reset 판본 행 수 오류')
        for index, (row, case) in enumerate(zip(chosen, plan(edition), strict=True)):
            if len(row) != 13 or row[1] != str(index) or row[2:7] != encoded(case): raise RuntimeError('UI reset 완성 입력 오류')
            if len(row[7].split(';')) != COUNT or any(len(bytes.fromhex(part.split('=')[1])) != SIZE for part in row[7].split(';')): raise RuntimeError('UI reset 노드 폭 오류')
            parts = row[8].split('#')
            if len(parts) != 6 or len(parts[0].split(',')) != 9 or [sum(int(p.split(':')[0]) for p in values.split(';')) for values in parts[1:]] != [1000, LIFETIME_SPECS[edition]['idle_size'], 100, 1000, 16]: raise RuntimeError('UI reset 표 폭 오류')
            for address, count in json.loads(row[11]).items(): entries[address] += count * 2
            for address, count in json.loads(row[12]).items(): returns[address] += count * 2
            expected_returns = collections.Counter(f'{dict(LIFETIME_SPECS[edition], **SPECS[edition])[action[0]]:08x}' for action in case['actions'])
            if dict(expected_returns) != json.loads(row[12]): raise RuntimeError('UI reset 최상위 정상 반환 누락')
            if row[10] != '-':
                for call in row[10].split('|'): boundaries[call.split(',')[0]] += 2
        if dict(entries) != item['entries'] or dict(returns) != item['returns'] or dict(boundaries) != item['boundaries']: raise RuntimeError('UI reset 진입/반환/경계 집계 오류')
    print(f'uireset 검증 통과: {len(rows)}개, 두 정밀도 정상 반환 {sum(sum(item["returns"].values()) for item in report["editions"].values())}회, OS/assert 0')


def main():
    """무저장 표본/전체 독립 관찰/저장 감사 중 요청한 작업만 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--smoke', action='store_true'); parser.add_argument('--verify', action='store_true')
    args = parser.parse_args(); verify() if args.verify else generate(args.smoke)


if __name__ == '__main__': main()
