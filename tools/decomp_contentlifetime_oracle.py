#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 Content 특수 삭제/구조 연결 검사를 독립 격리 실행한다.

사용법: contentlifetime 내보내기 후 python -X utf8 tools/decomp_contentlifetime_oracle.py [--verify]
게임/GUI/OS API/C++는 실행하지 않는다. 커서 취소 몸체·UI/송신·지갑/SP/소리만 명시 경계다.
"""
import argparse
import csv
import hashlib
import json
import socket
import struct
from collections import Counter
from decomp_formlifecycle_oracle import LifecycleOracle, raw, FORMS, CAPACITY, SAVED, POOL, TYPES, ROOT, STOP, STACK, encode_slots
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW

# 두 판본의 특수 삭제·구조 검사와 명시 외부 효과 주소다.
CONTENT = {
    'originals': dict(linked=0x4ae080, pre=0x4aeeb0, cancel=0x446e70, cursor=0x55a2a8, nugget=0x5412c0,
        local_owner=0x540c70, boss=0x540bc4, notice=0x5429c4, local=0x452fe0, packet=0x40abe0, remote=0x4e2dc0,
        wallet=0x413bb0, local_sp=0x40eec0, remote_sp=0x41df80, sound=0x4a9d70, battle=0x594fbc, capture=0x4927f0),
    'originalCD': dict(linked=0x4af1c0, pre=0x4b0300, cancel=0x43e4e0, cursor=0x51b658, battle=0x540a20, capture=0x40df80),
}
# 새 fixture/보고서 파일이다. 이전 관찰 근거는 바꾸지 않는다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/contentlifetime-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-contentlifetime-evidence.json'


class ContentOracle(LifecycleOracle):
    """공통 Form 실제 삭제에 너겟 preDestroy/연결 검사/실제 중심·CRT를 추가한다."""
    def __init__(self, edition):
        """읽기 전용 내보내기 범위만 허용하며 코드/타입의 우발적인 쓰기는 막는다."""
        super().__init__(edition)
        self.content = CONTENT[edition]
        self.effects, self.content_mutation, self.current, self.current_parent = [], 0, 0, 0
        with (ROOT / f'extracted/contentlifetime/{edition}/functions.tsv').open(encoding='utf-8') as stream:
            # 불연속 함수 몸체만 실행 범위에 더한다.
            for row in csv.DictReader(stream, delimiter='\t'):
                self.facts.append(row)
                # 함수 사이 빈 메모리는 허용하지 않는다.
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))

    def snapshot(self):
        """각 효과가 읽는 원래 Form/부모의 전체 바이트와 현재 전역/너겟 비용을 기록한다."""
        owner, boss, nugget = (self.global_value(name) for name in ('local_owner', 'boss', 'nugget')) if self.edition == 'originals' else (1, 1, 154)
        cost = self.mu.mem_read(TYPES + nugget * self.creation['type_stride'] + 0xc4, 4).hex()
        return f'{self.slot(self.current).hex()}:{self.slot(self.current_parent).hex()}:{owner}:{boss}:{nugget}:{cost}'

    def global_value(self, name):
        """패치의 현재 DWORD 전역 입력을 읽는다."""
        return struct.unpack('<I', self.mu.mem_read(self.content[name], 4))[0]

    def on_instruction(self, mu, address, size, data):
        """명시 외부 경계만 기록/대체하고 Content·커서 조건/좌표/CRT/반납은 실제로 실행한다."""
        if not hasattr(self, 'content'):
            return super().on_instruction(mu, address, size, data)
        c = self.content
        if address in (c['pre'], c['linked']):
            self.native['ContentPre' if address == c['pre'] else 'Linked'] += 1
        stubs = {c['cancel']: 'cancel', c['capture']: 'capture'}
        if self.edition == 'originals':
            stubs.update({c[key]: key for key in ('local', 'packet', 'remote', 'wallet', 'local_sp', 'remote_sp', 'sound')})
        stub = stubs.get(address)
        if stub is None:
            return super().on_instruction(mu, address, size, data)
        self.instructions += 1
        self.boundaries[stub] += 1
        esp, ecx = mu.reg_read(UC_X86_REG_ESP), mu.reg_read(UC_X86_REG_ECX)
        ret, a0, a1, a2, a3 = struct.unpack('<5I', mu.mem_read(esp, 20))
        if stub == 'capture':
            self.effects.append(f'capture:{a0}:{a1}:' + self.snapshot())
        elif stub == 'cancel':
            self.effects.append('cancel:' + self.snapshot())
            # 실제 커서 취소는 끝에 cursor 전역을 0으로 만든다. 추가 합성 mutation은 별도 입력이다.
            mu.mem_write(c['cursor'], bytes(4))
            if self.edition == 'originals' and self.content_mutation == 1:
                mu.mem_write(POOL + self.current * self.stride + 18, struct.pack('<H', 154))
        elif stub == 'local':
            if a1 or a2:
                raise RuntimeError('너겟 로컬 알림 인자 오류')
            self.effects.append(f'local:{a0}:' + self.snapshot())
            if self.content_mutation == 2:
                mu.mem_write(POOL + self.current * self.stride + 22, b'\1')
            elif self.content_mutation == 3:
                mu.mem_write(POOL + self.current_parent * self.stride + 34, b'\3')
            elif self.content_mutation == 7:
                mu.mem_write(POOL + self.current_parent * self.stride + 11, b'\2')
            elif self.content_mutation == 8:
                mu.mem_write(POOL + self.current * self.stride + 18, struct.pack('<H', 153))
        elif stub == 'packet':
            # 빈 수입 패킷 생성/송신은 경계다. payload/소켓을 실행한 관찰이라고 주장하지 않는다.
            mu.mem_write(ecx, bytes(8))
            mu.reg_write(UC_X86_REG_EAX, ecx)
        elif stub == 'remote':
            self.effects.append(f'remote:{a0}:' + self.snapshot())
        elif stub == 'wallet':
            self.effects.append(f'wallet:{a0}:{a1}:' + self.snapshot())
            if self.content_mutation == 4:
                mu.mem_write(POOL + self.current_parent * self.stride + 34, b'\2')
                mu.mem_write(TYPES + 154 * self.creation['type_stride'] + 0xc4, struct.pack('<f', 125.75))
            elif self.content_mutation == 5:
                mu.mem_write(c['local_owner'], struct.pack('<I', 2))
        elif stub in ('local_sp', 'remote_sp'):
            owner, amount = (self.global_value('local_owner'), a0) if stub == 'local_sp' else (a0, a1)
            self.effects.append(f'sp:{owner}:{amount}:{int(stub == "local_sp")}:' + self.snapshot())
            if self.content_mutation == 6:
                mu.mem_write(POOL + self.current_parent * self.stride + 14, struct.pack('<ff', 200.5, 60.25))
        elif stub == 'sound':
            if a3 or bytes(mu.mem_read(a2, 8)) != b'guy.wav\0':
                raise RuntimeError('너겟 소리 이름/마지막 인자 오류')
            self.effects.append(f'sound:{a0}:{a1}:' + self.snapshot())
        # 이 경계들은 cdecl이며 호출자가 인자를 회수한다. stdcall cleanup을 합성하지 않는다.
        mu.reg_write(UC_X86_REG_ESP, esp + 4)
        mu.reg_write(UC_X86_REG_EIP, ret)

    def invoke_content(self, op, form):
        """원본 루트 정상 반환/보존 레지스터와 x87 53비트 입력을 감사한다."""
        self.effects, self.events, self.instructions = [], [], 0
        # 호출마다 같은 ABI와 x87 모드에서 실행한다.
        for register, value in SAVED.items():
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_FPCW, 0x027f)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_ECX, POOL + form * self.stride)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.mem_write(STACK, struct.pack('<2I', STOP, 0x10))
        address = self.life['entries']['Destroy'] if op == 'Destroy' else self.life['entries']['Attach'] if op == 'Attach' else self.content['linked']
        if op == 'Attach':
            self.mu.mem_write(STACK, struct.pack('<3I', STOP, self.current_parent, 0x10))
        self.mu.emu_start(address, STOP, timeout=30000000, count=1000000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + (8 if op == 'Destroy' else 12 if op == 'Attach' else 4):
            raise RuntimeError(f'{self.edition} Content {op} 정상 반환/스택 오류')
        # 비휘발 레지스터는 관찰 결과와 별도로 검사한다.
        for register, value in SAVED.items():
            if self.mu.reg_read(register) != value:
                raise RuntimeError('Content 보존 레지스터 오류')
        return self.mu.reg_read(UC_X86_REG_EAX) if op == 'Linked' else 0


def generate():
    """너겟 삭제/효과 재조회와 전체 부모 체인의 구조 검사에 대한 두 PE 출력을 저장한다."""
    rows, reports = ['# edition op server owner boss cost cursor mutation inputs outputs result meta effects notices'], {}
    # 두 실제 PE를 서로 다른 메모리/호출 상태에서 실행한다.
    for edition in FORMS:
        oracle = ContentOracle(edition)
        counts, total = Counter(), 0
        # 서버/클라이언트 권한도 동일한 삭제 결과/깊이로 비교한다.
        for server in (False, True):
            oracle.begin(CAPACITY, server)
            oracle.step('Reset')
            baseline = bytes(oracle.mu.mem_read(POOL, CAPACITY * oracle.stride))
            globals_before = {address: bytes(oracle.mu.mem_read(address, 4)) for address in oracle.spec['globals']}

            def record(op, inputs, owner=1, boss=True, cost=500.25, cursor=0, mutation=0, capture_profile=None):
                """모든 초기 슬롯/타입/전역은 명시 입력이며 기대 출력은 실제 PE에서만 읽는다."""
                nonlocal total
                oracle.mu.mem_write(POOL, baseline)
                # 앞 사례의 카운터/기록을 섞지 않는다.
                for address, value in globals_before.items():
                    oracle.mu.mem_write(address, value)
                for field in ('pre', 'post', 'pop'):
                    oracle.mu.mem_write(oracle.life[field], bytes(4))
                for address in oracle.spec['logs']:
                    oracle.mu.mem_write(address, bytes(160))
                for sid, value in inputs.items():
                    oracle.mu.mem_write(POOL + sid * oracle.stride, bytes(value))
                oracle.current, oracle.current_parent = 80, struct.unpack_from('<H', inputs[80], 16)[0]
                oracle.content_mutation = mutation
                c = oracle.content
                oracle.mu.mem_write(c['battle'], struct.pack('<I', int(capture_profile is not None and capture_profile[0])))
                oracle.mu.mem_write(c['cursor'], struct.pack('<I', cursor))
                oracle.write_ranges.append((c['cursor'], c['cursor'] + 4))
                if edition == 'originals':
                    for name, value in [('local_owner', owner), ('boss', int(boss)), ('nugget', 154), ('notice', 0x1234)]:
                        oracle.mu.mem_write(c[name], struct.pack('<I', value))
                    oracle.write_ranges.extend([(c['local_owner'], c['local_owner'] + 4), (TYPES + 154 * oracle.creation['type_stride'] + 0xc4, TYPES + 154 * oracle.creation['type_stride'] + 0xc8)])
                oracle.mu.mem_write(TYPES + 154 * oracle.creation['type_stride'] + 0xc4, struct.pack('<f', cost))
                oracle.mu.mem_write(TYPES + 80 * oracle.creation['type_stride'] + 0xec, struct.pack('<I', 0x200000 if capture_profile is not None and capture_profile[1] else 0))
                oracle.mu.mem_write(TYPES + 90 * oracle.creation['type_stride'] + 0xec, struct.pack('<I', 0x10000 if capture_profile is not None and capture_profile[2] else 0))
                # 중심 함수는 실제 타입의 발자국을 읽는다. 소리 좌표 계산은 대체하지 않는다.
                oracle.mu.mem_write(TYPES + 90 * oracle.creation['type_stride'] + (0x1d4 if edition == 'originals' else 0x1b4), struct.pack('<ii', 3, 5))
                result = oracle.invoke_content(op, 80)
                outputs = {sid: oracle.slot(sid) for sid in inputs}
                meta = oracle.metadata() + [struct.unpack('<I', oracle.mu.mem_read(oracle.life[field], 4))[0] for field in ('pre', 'post', 'pop')]
                rows.append('\t'.join(map(str, [edition, op, int(server), owner, int(boss), struct.unpack('<I', struct.pack('<f', cost))[0], cursor, mutation,
                    encode_slots(inputs), encode_slots(outputs), result, ','.join(map(str, meta)), ';'.join(oracle.effects), ';'.join(oracle.events)])))
                counts[op] += 1
                total += 1

            # kind·paid·부모 dead·소유자·권한·커서 조건의 전체 조합이다.
            for kind in (153, 154):
                for paid in (0, 1):
                    for dead in (False, True):
                        for parent_owner in (1, 2):
                            for boss in (False, True):
                                for cursor in (0, 80):
                                    inputs = {100: raw(oracle, 100, 90, 2 if dead else 0), 80: raw(oracle, 80, 6, 8)}
                                    struct.pack_into('<H', inputs[100], 6, 80)
                                    struct.pack_into('<ff', inputs[100], 14, 30.125, 20.75)
                                    inputs[100][34 if edition == 'originals' else 32] = parent_owner
                                    struct.pack_into('<H', inputs[80], 16, 100)
                                    struct.pack_into('<H', inputs[80], 18, kind)
                                    inputs[80][22] = paid
                                    record('Destroy', inputs, boss=boss, cursor=cursor)
            # 각 콜백이 바꾼 kind/paid/소유자/비용/로컬 소유자/좌표를 후속 실제 코드가 재조회한다.
            for mutation in range(1, 9):
                inputs = {100: raw(oracle, 100, 90, 0), 80: raw(oracle, 80, 6, 8)}
                struct.pack_into('<H', inputs[100], 6, 80)
                struct.pack_into('<ff', inputs[100], 14, 30.125, 20.75)
                inputs[100][34 if edition == 'originals' else 32] = 1
                struct.pack_into('<HH', inputs[80], 16, 100, 153 if mutation == 1 else 154)
                inputs[80][22] = 0
                record('Destroy', inputs, cursor=80 if mutation == 1 else 0, mutation=mutation)
            # fractional/음수/하위 DWORD 감김의 비용도 실제 CRT 절삭으로 관찰한다.
            for cost in (0, -123.75, 500, 500.75, 2147483648.0, 4294967296.0):
                inputs = {100: raw(oracle, 100, 90, 0), 80: raw(oracle, 80, 6, 8)}
                struct.pack_into('<H', inputs[100], 6, 80)
                struct.pack_into('<ff', inputs[100], 14, 30.125, 20.75)
                inputs[100][34 if edition == 'originals' else 32] = 1
                struct.pack_into('<HH', inputs[80], 16, 100, 154)
                inputs[80][22] = 0
                record('Destroy', inputs, cost=cost)
            # 커서 helper의 parent==self 경로는 별도 입력이다. dead 부모로 인해 너겟 지급은 생략된다.
            self_parent = {80: raw(oracle, 80, 6, 8)}
            struct.pack_into('<HH', self_parent[80], 16, 80, 154)
            record('Destroy', self_parent)
            # 실제 전투 부착의 사제/워커 조건 및 이전 소유자 low7 전달을 별도 루트로 관찰한다.
            for battle in (False, True):
                for priest in (False, True):
                    for carrier in (False, True):
                        inputs = {100: raw(oracle, 100, 90, 0), 80: raw(oracle, 80, 6, 4)}
                        struct.pack_into('<H', inputs[80], 16, 100)
                        struct.pack_into('<HH', inputs[80], 18, 80, 0)
                        struct.pack_into('<H', inputs[80], 12, 0x182)
                        record('Attach', inputs, mutation=100 + int(battle) * 4 + int(priest) * 2 + int(carrier), capture_profile=(battle, priest, carrier))
            # 구조 연결은 flags나 체인 항목의 상태를 추가로 거르지 않는다. 무효 next는 자기 번호 뒤에도 false다.
            for present in (False, True):
                for invalid_tail in (False, True):
                    for flags in (0, 0x80, 0x34):
                        inputs = {100: raw(oracle, 100, 90, 0), 80: raw(oracle, 80, 6, 8), 200: raw(oracle, 200, 90, 1)}
                        struct.pack_into('<H', inputs[100], 6, 80 if present else 200)
                        struct.pack_into('<H', inputs[80], 16, 100)
                        struct.pack_into('<H', inputs[80], 20, flags)
                        struct.pack_into('<H', inputs[80], 4, 200)
                        struct.pack_into('<H', inputs[200], 4, 65535 if invalid_tail else 0)
                        record('Linked', inputs)
        reports[edition] = dict(sha256=oracle.sha256, cases=total, operations=dict(counts), assertions=oracle.assertions,
            native=dict(oracle.native), boundaries=dict(oracle.boundaries), functions=oracle.facts,
            body_sha256={name: hashlib.sha256((ROOT / f'extracted/contentlifetime/{edition}/{name}').read_bytes()).hexdigest() for name in ('creation.c', 'functions.tsv')})
    FIXTURE.write_text('\n'.join(rows) + '\n', encoding='utf-8')
    REPORT.write_text(json.dumps(dict(last_decompile_host=socket.gethostname(), recorded_date='2026-10-13',
        method='실제 두 PE의 Content 특수 삭제/커서 조건/공통 삭제/Unpop/반납/실제 중심·CRT/구조 검사. C++ 호출 없음.',
        boundaries=['커서 취소 전체 몸체·UI/패킷 생성/송신·AI 지갑/SP·소리. 실제 원본 소켓/소리/장치는 실행하지 않는다.'],
        fixture_sha256=hashlib.sha256(FIXTURE.read_bytes()).hexdigest(), editions=reports), ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print('Content 수명 원본 관찰:', {edition: item['cases'] for edition, item in reports.items()})


def verify():
    """저장 관찰의 PE/몸체/fixture 해시·사례·assert 감사를 재확인한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    if hashlib.sha256(FIXTURE.read_bytes()).hexdigest() != report['fixture_sha256']:
        raise RuntimeError('Content fixture SHA 불일치')
    counts, operations = Counter(), {edition: Counter() for edition in report['editions']}
    # 파일의 실제 행을 독립적으로 집계한다.
    for row in csv.reader(FIXTURE.read_text(encoding='utf-8').splitlines(), delimiter='\t'):
        if row[0].startswith('#'):
            continue
        if len(row) != 14:
            raise RuntimeError('Content fixture 열 수 오류')
        counts[row[0]] += 1
        operations[row[0]][row[1]] += 1
    # 두 판본의 입력/허용 몸체가 관찰 당시와 같아야 한다.
    for edition, evidence in report['editions'].items():
        binary = ROOT / ('originals/Netstorm.exe' if edition == 'originals' else 'originalCD/NETSTORM.EXE')
        if hashlib.sha256(binary.read_bytes()).hexdigest() != evidence['sha256'] or evidence['assertions']:
            raise RuntimeError('Content PE/assert 감사 오류')
        if counts[edition] != evidence['cases'] or dict(operations[edition]) != evidence['operations']:
            raise RuntimeError('Content 사례 수 감사 오류')
        for name, expected in evidence['body_sha256'].items():
            if hashlib.sha256((ROOT / f'extracted/contentlifetime/{edition}/{name}').read_bytes()).hexdigest() != expected:
                raise RuntimeError('Content 허용 몸체 SHA 오류')
    print('Content 수명 근거 감사 통과:', dict(counts))


# 명시 실행에서만 관찰 생성/감사를 수행한다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    verify() if args.verify else generate()
