#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""미션 Init의 실제87바이트 옵션 초기화와 기본 인덱스3 범위·비용 파생을 제품 변경 전에 관측한다."""
import argparse
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from decomp_script_command import Oracle as InvokeOracle
from decomp_cursor_position import ranges
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP

LOCAL = POOL + 0x1000  # 직접 생성자 입력의 실제87바이트와 양쪽8바이트 이웃이다.
GLOBAL = 0x52f7e0  # Init0041cc10이 재설정하는 동일 원본 옵션 주소다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/mission-option-reset-x86.tsv'  # 기존3305행을 바꾸지 않는 새 첫 관측이다.
REPORT = ROOT / 'cpppj/mission-option-reset-native.json'  # 실행한 명령과 제품/게임 미실행을 구별한다.


def cases():
    """초기 모든 바이트/전역 래퍼·파생 호출과 네 번째 실제float를 입력으로 준비한다."""
    result = []
    # 전체 BYTE 초기값과 reserved 보존을 교차한다.
    for seed in range(256):
        result.append(('R', seed, 3, 0x42180000))
    # 전역 래퍼는 별도 정상 반환 경로로 관측한다.
    for seed in range(16):
        result.append(('W', seed, 3, 0x42180000))
    # 원본 기본 초기화 후 실제 범위/비용/보상 값을 연속 계산한다.
    for seed in (0, 1, 7, 31, 127, 128, 254, 255):
        result.append(('P', seed, 3, 0x42180000))
    # 기본3번이 읽는 네 번째float와 기존3개를 비교하는 원본 clamp를 관측한다.
    for bits in (0, 0x80000000, 1, 0x80000001, 0xc1200000, 0x41600000, 0x41700000, 0x41b00000,
                 0x41f00000, 0x42180000, 0x7f7fffff, 0xff7fffff, 0x7f800000, 0xff800000, 0x7fc12345, 0xffc12345):
        result.append(('F', 37, 3, bits))
    return result


class Oracle(MemoryOracle):
    """실제 초기화/전역 래퍼·범위/비용/보상 몸체만 실행하고 다른 주소는 거부한다."""
    call = InvokeOracle.call  # 정상 반환/스택/보존 레지스터/x87 검사를 같은 원본 호출기로 사용한다.

    def __init__(self):
        """보호 PE에서22개 초기 BYTE와 실제4float를 그대로 보관한다."""
        super().__init__()
        self.allowed = []
        ranges(self, (0x41cb90, 0x41cc10, 0x4b4860, 0x41c8b0, 0x41ca70))
        self.defaults = bytes(self.mu.mem_read(0x52f5e0 + index * 24, 1)[0] for index in range(22))
        self.range_table = bytes(self.mu.mem_read(0x5425f4, 16))
        self.normal = 0
        self.instructions = 0
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def guard(self, mu, access, address, size, value, data):
        """검토된 함수는 실제 옵션87/두float/한정 스택만 쓸 수 있다."""
        if STACK - 0x1000 <= address and address + size <= STACK + 0x100:
            return
        if self.target <= address and address + size <= self.target + 87:
            return
        if self.operation in ('P', 'F') and 0x52f488 <= address and address + size <= 0x52f490:
            return
        raise RuntimeError(f'옵션 초기화 쓰기 범위 밖 {address:08x}/{size}')

    def instruction(self, mu, address, size, data):
        """Config/GUI/OS 대체 없이 실제 순수 함수만 허용한다."""
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'미검토 옵션 초기화 명령 {address:08x}')
        self.instructions += 1

    def invoke(self, address, ecx=0):
        """정상 함수 반환만 세고 관측한 반환 DWORD를 그대로 돌려준다."""
        value = self.call(address, [], ecx)
        self.normal += 1
        return value

    def sample(self, case):
        """입력 후 전체87바이트/양쪽 이웃/두float와 실제 비용·보상을 수집한다."""
        self.operation, seed, selected, fourth = case
        self.target = LOCAL if self.operation == 'R' else GLOBAL
        self.mu.mem_write(STACK - 0x1000, b'\xa5' * 0x1100)
        self.mu.mem_write(0x5425f4, self.range_table)
        self.put(0x542600, fourth)
        if self.operation == 'R':
            self.mu.mem_write(LOCAL - 8, bytes((seed + index * 37) & 255 for index in range(103)))
        else:
            self.mu.mem_write(GLOBAL, bytes((seed + index * 37) & 255 for index in range(87)))
        before = bytes(self.mu.mem_read(self.target - 8, 103))
        cost = reward = '-'
        if self.operation == 'R':
            self.invoke(0x41cb90, self.target)
        elif self.operation == 'W':
            self.invoke(0x41cc10)
        elif self.operation == 'P':
            self.invoke(0x41cc10)
            self.invoke(0x4b4860)
            cost = str(self.invoke(0x41c8b0))
            reward = str(self.invoke(0x41ca70))
        elif self.operation == 'F':
            self.mu.mem_write(GLOBAL + 4, bytes([selected]))
            self.invoke(0x4b4860)
        else:
            raise ValueError(self.operation)
        after = bytes(self.mu.mem_read(self.target - 8, 103))
        assert before[:8] == after[:8] and before[-8:] == after[-8:]
        floats = bytes(self.mu.mem_read(0x52f488, 8)).hex() if self.operation in ('P', 'F') else '-'
        return [*map(str, case), before.hex(), after.hex(), floats, cost, reward]


def main():
    """최초 원문을 한 번 쓰거나 해시만 확인하며 제품/게임 실행 파일은 시작하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    if parser.parse_args().verify:
        record = json.loads(REPORT.read_text('utf-8'))
        for name, expected in record['sha256'].items():
            assert digest(ROOT / name) == expected, name
        print('미션 옵션 초기화 최초296입력/기본4float 관측 보존 확인')
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('최초 관측 덮어쓰기 금지')
    oracle = Oracle()
    rows = []
    # 제품 코드를 참조하지 않고 실제 원본 명령의 전체 출력만 저장한다.
    for case in cases():
        rows.append(oracle.sample(case))
    FIXTURE.write_text('# 실제 옵션 초기화/전역 래퍼/기본 파생/네 번째float 관측이다.\n' + '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    paths = ['originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_mission_option_reset.py', 'cpppj/tests/fixtures/mission-option-reset-x86.tsv']
    record = {'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-20', 'uniqueInputs': len(rows), 'normalFunctionReturns': oracle.normal, 'instructions': oracle.instructions,
              'gameExecuted': False, 'osExecuted': False, 'defaults': list(oracle.defaults), 'rangeTableHex': oracle.range_table.hex(),
              'scope': '실제0041cb90/41cc10의87바이트 초기화와004b4860/41c8b0/41ca70의 기본 파생 및 네 번째float. 외부 효과 대체가 없고 전체 Init/로그인·실제 게임/화면 관측은 아니다.',
              'sha256': {name: digest(ROOT / name) for name in paths}}
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: record[key] for key in ('uniqueInputs', 'normalFunctionReturns', 'instructions', 'defaults', 'rangeTableHex')}))


if __name__ == '__main__':
    main()
