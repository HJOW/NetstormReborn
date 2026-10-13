#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""감시 쌍·ContentForm 정렬/피해를 두 실제 PE의 제한 x86으로 독립 관찰한다.

사용법: 읽기 전용 formlifecycle 내보내기 후 python -X utf8 tools/decomp_formlifecycle_oracle.py
게임/GUI/OS API/C++ 코드는 실행하지 않는다. 부모 알림·전송·선택·진단 출력만 명시 경계다.
"""
import csv
import argparse
import hashlib
import json
import socket
import struct
from collections import Counter
from decomp_formtake_oracle import FormOracle, FORMS, CAPACITY, SAVED, encode_slots, NOTICE
from decomp_creation_oracle import TYPES
from decomp_oracle import ROOT, STOP, STACK, SCRATCH
from decomp_sid_oracle import POOL
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS

# 합성 부모 가상 표/알림 경계이며 Form 생성자와 내부 호출은 실제 PE를 사용한다.
PARENT, WATCH_NOTICE = SCRATCH + 0x300, STOP + 0x200
# 원본 함수·전역·외부 진단 함수의 판본별 실제 주소다.
LIFECYCLE = {
    'originals': dict(entries=dict(Pair=0x4b1510, Destroy=0x4af780, Notify=0x4ae410, Has=0x4ae900,
        Remove=0x4ae780, Attach=0x4aec10, Robust=0x4aea60, Damage=0x4af070),
        pre=0x5c8480, post=0x5c8484, pop=0x5c8488, depend=0x541074, content=0x541088,
        transmit=0x4ac260, selected=0x4d5430, log=0x4c2630, label=0, order=23,
        content_pre=0x4aeeb0),
    'originalCD': dict(entries=dict(Pair=0x4af940, Destroy=0x4ab7e0, Notify=0x4af600, Has=0x4afbc0,
        Remove=0x4afa50, Attach=0x4afff0, Robust=0x4afe20, Damage=0x4b0350),
        pre=0x5395f8, post=0x5395fc, pop=0x539600, depend=0x51c964, content=0x51c978,
        transmit=0x4abb40, selected=0x41ea30, log=0x48ca70, label=0x4abf50, order=22,
        content_pre=0x4b0300),
}


class LifecycleOracle(FormOracle):
    """이전 Form/SID 실행기에 실제 추가 몸체와 외부 경계 감사만 연결한다."""
    def __init__(self, edition):
        """판본별 내보내기 범위 밖 실행을 차단하며 부모 가상 표를 명시 입력으로 구성한다."""
        super().__init__(edition)
        self.life = LIFECYCLE[edition]
        with (ROOT / f'extracted/formlifecycle/{edition}/functions.tsv').open(encoding='utf-8') as stream:
            # 새 명령 몸체의 불연속 범위만 허용한다.
            for row in csv.DictReader(stream, delimiter='\t'):
                self.facts.append(row)
                # 함수 사이 빈 주소를 실행 가능 범위로 넓히지 않는다.
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))
        table = bytearray(self.mu.mem_read(self.creation['vtable'], 0xcc))
        struct.pack_into('<I', table, 0x24, NOTICE)
        struct.pack_into('<I', table, 0x34, WATCH_NOTICE)
        self.mu.mem_write(PARENT, bytes(table))
        self.events, self.boundaries, self.native = [], Counter(), Counter()
        self.mutation = 0

    def begin(self, capacity, server):
        """초기화 뒤 사용될 타입 번호/삭제·부착 깊이를 원본 주소에 놓는다."""
        super().begin(capacity, server)
        # 실제 코드가 쓰는 공통 깊이는 개별 DWORD 범위로만 허용한다.
        for field in ('pre', 'post', 'pop'):
            address = self.life[field]
            self.mu.mem_write(address, bytes(4))
            self.write_ranges.append((address, address + 4))
        self.mu.mem_write(self.life['depend'], struct.pack('<I', 1))
        self.mu.mem_write(self.life['content'], struct.pack('<I', 6))

    def on_instruction(self, mu, address, size, data):
        """알림 당시 슬롯을 기록하고 선택/전송/진단만 대체한다. 내부 가상 삭제는 실제로 실행한다."""
        if not hasattr(self, 'life'):
            return super().on_instruction(mu, address, size, data)
        if address in self.life['entries'].values():
            self.native[next(name for name, value in self.life['entries'].items() if value == address)] += 1
        esp, ecx = mu.reg_read(UC_X86_REG_ESP), mu.reg_read(UC_X86_REG_ECX)
        stub = None
        if address == NOTICE:
            stub = 'content'
        elif address == WATCH_NOTICE:
            stub = 'watch'
        elif address == self.life['transmit']:
            stub = 'transmit'
        elif address == self.life['selected']:
            stub = 'selected'
        elif address == self.life['log']:
            stub = 'log'
        elif address == self.life['label'] and address:
            stub = 'label'
        if stub is None:
            return super().on_instruction(mu, address, size, data)
        self.instructions += 1
        self.boundaries[stub] += 1
        ret, a0, a1, a2, a3 = struct.unpack('<5I', mu.mem_read(esp, 20))
        purge = 0
        sid = (ecx - POOL) // self.stride
        if stub in ('content', 'watch'):
            other = a0
            self.events.append(f'{stub}:{sid}:{a0}:{a1}:{self.slot(sid).hex()}:{self.slot(a1 if stub == "watch" else other).hex()}')
            purge = 8
            # 별도 사례에서 부모 알림 뒤 현재 payload를 다시 읽는지 확인한다. 결과는 실제 후속 명령이 만든다.
            if self.mutation and stub == 'content' and a1 == 2:
                mu.mem_write(POOL + other * self.stride + 20, struct.pack('<H', self.mutation))
        elif stub == 'transmit':
            self.events.append(f'transmit:{sid}:{a3}:{self.slot(sid).hex()}')
            purge = 16
        elif stub in ('selected', 'label'):
            mu.reg_write(UC_X86_REG_EAX, 0 if stub == 'selected' else TYPES)
            # CD의 타입 label helper는 ret0이며 호출자가 진단 varargs와 함께 인자를 회수한다.
            purge = 0
        mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge)
        mu.reg_write(UC_X86_REG_EIP, ret)

    def invoke(self, op, first, second, flags=0):
        """루트 호출의 반환 위치/스택/보존 레지스터를 감사한다. 기대 동작을 호스트에서 만들지 않는다."""
        self.events, self.instructions = [], 0
        # 각 루트는 같은 명시 레지스터 입력으로 시작한다.
        for register, value in SAVED.items():
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_EAX, 0)
        self.mu.reg_write(UC_X86_REG_EDX, 0)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, POOL + first * self.stride)
        args = [first, second, flags] if op in ('Pair', 'Has', 'Remove') else [second, flags]
        if op == 'Damage':
            args = [0xabcdef, second & 0xffffffff, flags]
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * (len(args) + 1), STOP, *args))
        purge = dict(Pair=0, Has=0, Remove=0, Destroy=4, Notify=4, Attach=8, Robust=0, Damage=12)[op]
        self.mu.emu_start(self.life['entries'][op], STOP, timeout=30000000, count=1000000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4 + purge:
            raise RuntimeError(f'{self.edition} {op} 정상 반환/ABI 오류')
        # 보존 레지스터는 결과와 별도로 확인한다.
        for register, value in SAVED.items():
            if self.mu.reg_read(register) != value:
                raise RuntimeError(f'{op} 보존 레지스터 오류')
        return (self.mu.reg_read(UC_X86_REG_EAX) & 0xff) if op in ('Has', 'Robust') else 0


def raw(oracle, sid, number, state=4, seed=0):
    """판본별 슬롯에 명시 입력 payload를 만든다. 기대 출력은 여기에 넣지 않는다."""
    result = bytearray((seed + index * 37) & 255 for index in range(oracle.stride))
    table = FORMS[oracle.edition]['vtables'].get(number, PARENT)
    struct.pack_into('<I', result, 0, table)
    struct.pack_into('<HH', result, 4, 0, 0)
    result[10:12] = bytes((number, state))
    # 링크/생산 타입/플래그는 각 사례에서 별도로 지정한다.
    struct.pack_into('<HHHH', result, 14, 0, 0, 80, 0)
    return result


def generate():
    """두 판본에서 감시 생성/양방향 삭제/조회/제거/Content 정렬·상태의 독립 관찰을 저장한다."""
    rows, reports = ['# edition op first second flags server mutation inputs outputs result meta notices'], {}
    # 실제 PE는 별도 에뮬레이터에서 실행하며 C++ 코드/결과를 읽지 않는다.
    for edition in FORMS:
        oracle = LifecycleOracle(edition)
        count, operations = 0, Counter()
        # 권한과 서버/클라이언트 쌍을 각각 독립적으로 만든다.
        for server in (False, True):
            oracle.begin(CAPACITY, server)
            oracle.step('Reset')
            baseline = bytes(oracle.mu.mem_read(POOL, CAPACITY * oracle.stride))
            globals_before = {a: bytes(oracle.mu.mem_read(a, 4)) for a in oracle.spec['globals']}

            def record(op, first, second, flags, inputs, watched=(), mutation=0):
                """동일 초기화 상태에서 루트만 실행하고 전체 슬롯/메타데이터/알림을 저장한다."""
                nonlocal count
                oracle.mu.mem_write(POOL, baseline)
                # 사례 간 전역 카운터와 삭제 기록을 초기화한다.
                for address, value in globals_before.items():
                    oracle.mu.mem_write(address, value)
                for field in ('pre', 'post', 'pop'):
                    oracle.mu.mem_write(oracle.life[field], bytes(4))
                for address in oracle.spec['logs']:
                    oracle.mu.mem_write(address, bytes(160))
                # 입력 슬롯의 byte 전체를 명시한다.
                for sid, value in inputs.items():
                    oracle.mu.mem_write(POOL + sid * oracle.stride, bytes(value))
                oracle.mutation = mutation
                result = oracle.invoke(op, first, second, flags)
                outputs = {sid: oracle.slot(sid) for sid in dict.fromkeys((*inputs, *watched))}
                meta = oracle.metadata() + [struct.unpack('<I', oracle.mu.mem_read(oracle.life[f], 4))[0] for f in ('pre', 'post', 'pop')]
                rows.append('\t'.join(map(str, [edition, op, first, second, flags, int(server), mutation,
                    encode_slots(inputs), encode_slots(outputs), result, ','.join(map(str, meta)), ';'.join(oracle.events)])))
                count += 1
                operations[op] += 1

            # client·서버·혼합 부모의 쌍 생성은 실제 두 할당 및 실제 가상 부착을 실행한다.
            for client_watcher, client_watched in ((True, True), (True, False), (False, True), (False, False)):
                if not server and not (client_watcher or client_watched):
                    continue
                watcher = 100 if client_watcher else oracle.spec['server_first'] + 100
                watched = 200 if client_watched else oracle.spec['server_first'] + 200
                inputs = {watcher: raw(oracle, watcher, 90), watched: raw(oracle, watched, 91)}
                head = 5 if client_watcher or client_watched else oracle.spec['server_first']
                record('Pair', watcher, watched, 0, inputs, (head, head + 1))
            # watcher가 ProcessForm이거나 두 부모가 같은 경우도 원본 생성 자체에는 금지 조건이 없다.
            for same in (False, True):
                inputs = {100: raw(oracle, 100, 90)} if same else {100: raw(oracle, 100, 90), 200: raw(oracle, 200, 2)}
                record('Pair', 100, 100 if same else 200, 0, inputs, (5, 6))
            # 명시 초기 감시 체인에서 첫/상대 삭제와 부모 통지·중복 제거를 독립 호출한다.
            for client in (True, False):
                if not server and not client:
                    # 서버 쌍 통지는 client 권한일 때 생략되는 경로도 검사한다.
                    allowed = ('Notify', 'Has')
                else:
                    allowed = ('Destroy', 'Notify', 'Has', 'Remove')
                base = 50 if client else oracle.spec['server_first'] + 50
                watcher, watched, first, second, extra = base + 10, base + 11, base + 12, base + 13, base + 14
                for dead_parent in (False, True):
                    inputs = {watcher: raw(oracle, watcher, 90), watched: raw(oracle, watched, 91, 2 if dead_parent else 0),
                        first: raw(oracle, first, 1, 8, 0x57), second: raw(oracle, second, 1, 8, 0x69), extra: raw(oracle, extra, 3, 8)}
                    struct.pack_into('<H', inputs[watcher], 6, first)
                    struct.pack_into('<H', inputs[watched], 6, second)
                    struct.pack_into('<HHH', inputs[first], 16, watcher, 0, second)
                    struct.pack_into('<HHH', inputs[second], 16, watched, first, 0)
                    struct.pack_into('<H', inputs[first], 4, extra)
                    struct.pack_into('<HH', inputs[extra], 14, first, watcher)
                    for op in allowed:
                        if op == 'Destroy':
                            record(op, first, 0x10, 0, inputs)
                            record(op, second, 0x10, 0, inputs)
                        elif op == 'Notify':
                            record(op, second, watched, 0, inputs)
                        elif op == 'Has':
                            record(op, watched, watcher, 0, inputs)
                            record(op, watched, watcher + 1, 0, inputs)
                        else:
                            record(op, watcher, watched, 0x10, inputs)
                            if not dead_parent:
                                another, peer = base + 15, base + 16
                                duplicate = {sid: bytearray(value) for sid, value in inputs.items()}
                                duplicate[another], duplicate[peer] = raw(oracle, another, 1, 8), raw(oracle, peer, 1, 8)
                                struct.pack_into('<HHH', duplicate[another], 16, watched, peer, 0)
                                struct.pack_into('<HHH', duplicate[peer], 16, watcher, 0, another)
                                struct.pack_into('<H', duplicate[another], 4, second)
                                struct.pack_into('<H', duplicate[peer], 4, first)
                                struct.pack_into('<H', duplicate[second], 14, another)
                                struct.pack_into('<H', duplicate[first], 14, peer)
                                struct.pack_into('<H', duplicate[watched], 6, another)
                                struct.pack_into('<H', duplicate[watcher], 6, peer)
                                record(op, watcher, watched, 0x10, duplicate)
            # 그룹 플래그/순서 공백/혼합 Form·자산 체인에 대한 정렬 결과를 관찰한다.
            for length in range(5):
                for group in (0x20, 0x120, 0x220, 0x21):
                    # 뒤 삽입·머리 공백·자산 뒤 공백·일반 Form 뒤 공백을 별도 루트로 실행한다.
                    for profile in range(4 if length else 1):
                        parent, form = 100, 80
                        inputs = {parent: raw(oracle, parent, 90), form: raw(oracle, form, 6, 4, 0x35)}
                        struct.pack_into('<H', inputs[form], 20, group)
                        ids = [200 + index for index in range(length)]
                        struct.pack_into('<H', inputs[parent], 6, ids[0] if ids else 0)
                        for index, sid in enumerate(ids):
                            number = ((3, 6, 90, 6) if profile == 3 else (6, 3, 90, 6))[index]
                            inputs[sid] = raw(oracle, sid, number, 8, 0x91 + index)
                            struct.pack_into('<H', inputs[sid], 4, ids[index + 1] if index + 1 < length else 0)
                            struct.pack_into('<H', inputs[sid], 14 if number < 70 else 16, ids[index - 1] if index else 0)
                            struct.pack_into('<H', inputs[sid], 16 if number < 70 else 14, parent)
                            struct.pack_into('<H', inputs[sid], 20, 0x20)
                            inputs[sid][oracle.life['order']] = 1 if profile in (1, 3) else 2 if profile == 2 and index == 3 else index // 2
                        record('Attach', form, parent, 0x10, inputs)
            # Robust의 실제 비트 선택을 전수 low7 플래그에서 확인한다. WORLD assert 입력은 별도 계약이다.
            for flags in range(128):
                inputs = {80: raw(oracle, 80, 6, 4)}
                struct.pack_into('<H', inputs[80], 20, flags | 0x300)
                record('Robust', 80, 0, 0, inputs)
            # 양수 피해의 두 알림/재부착과 제거 알림이 바꾼 payload 재조회를 관찰한다.
            for mutation in (0, 0x321):
                for damage in (1, 3):
                    inputs = {100: raw(oracle, 100, 90), 80: raw(oracle, 80, 6, 8)}
                    struct.pack_into('<H', inputs[100], 6, 80)
                    struct.pack_into('<H', inputs[80], 16, 100)
                    struct.pack_into('<H', inputs[80], 20, 0x120)
                    record('Damage', 80, damage, 0x3f800000, inputs, mutation=mutation)
        reports[edition] = dict(sha256=oracle.sha256, cases=count, operations=dict(operations),
            assertions=oracle.assertions, native=dict(oracle.native), boundaries=dict(oracle.boundaries), functions=oracle.facts)
        # 실행에 사용한 추가 몸체의 원시 바이트 해시를 근거에 보존한다.
        reports[edition]['body_sha256'] = {name: hashlib.sha256((ROOT / f'extracted/formlifecycle/{edition}/{name}').read_bytes()).hexdigest()
            for name in ('creation.c', 'functions.tsv')}
    fixture = ROOT / 'cpppj/tests/fixtures/formlifecycle-x86.tsv'
    fixture.write_text('\n'.join(rows) + '\n', encoding='utf-8')
    report = dict(last_decompile_host=socket.gethostname(), recorded_date='2026-10-13',
        method='실제 두 PE의 Form 생성/가상 부착/조회/감시 중첩 삭제/SID 반납/Content 정렬·양수 피해. C++ 호출 없음.',
        boundaries=['부모 가상 +24/+34 알림 및 선택/전송/진단. Content 특수 preDestroy는 이번 관찰 대상 밖.'],
        fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(), editions=reports)
    (ROOT / 'cpppj/formlifecycle.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print('Form 수명 원본 관찰:', {edition: item['cases'] for edition, item in reports.items()})


def verify():
    """PE/내보내기/fixture의 바이트 해시와 사례 수/정상 명령 실행 감사를 확인한다."""
    report = json.loads((ROOT / 'cpppj/formlifecycle.json').read_text(encoding='utf-8'))
    fixture = ROOT / 'cpppj/tests/fixtures/formlifecycle-x86.tsv'
    if hashlib.sha256(fixture.read_bytes()).hexdigest() != report['fixture_sha256']:
        raise RuntimeError('Form 수명 fixture SHA 불일치')
    counts, operations = Counter(), {edition: Counter() for edition in report['editions']}
    # TSV 실제 행을 다시 집계하여 보고서의 수와 독립적으로 비교한다.
    for line in fixture.read_text(encoding='utf-8').splitlines():
        if line.startswith('#'):
            continue
        row = line.split('\t')
        if len(row) != 12:
            raise RuntimeError('Form 수명 fixture 열 수 불일치')
        counts[row[0]] += 1
        operations[row[0]][row[1]] += 1
    # 같은 보호 PE와 같은 내보내기만 해당 fixture의 근거로 인정한다.
    for edition, evidence in report['editions'].items():
        binary = ROOT / ('originals/Netstorm.exe' if edition == 'originals' else 'originalCD/NETSTORM.EXE')
        if hashlib.sha256(binary.read_bytes()).hexdigest() != evidence['sha256']:
            raise RuntimeError('Form 수명 보호 PE SHA 불일치')
        if counts[edition] != evidence['cases'] or dict(operations[edition]) != evidence['operations'] or evidence['assertions']:
            raise RuntimeError('Form 수명 사례 수/assert 감사 불일치')
        # 관찰 당시 기계어 내보내기 해시가 여전히 일치하는지 검사한다.
        for name, expected in evidence['body_sha256'].items():
            if hashlib.sha256((ROOT / f'extracted/formlifecycle/{edition}/{name}').read_bytes()).hexdigest() != expected:
                raise RuntimeError('Form 수명 몸체 SHA 불일치')
    print('Form 수명 근거 감사 통과:', dict(counts))


# 생성과 저장 근거 감사는 사용자가 명시한 도구 실행에서만 수행한다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    verify() if args.verify else generate()
