#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 원본의 sentinel/Free를 격리 실행하고 진단 복귀 뒤 현재 표 쓰기를 관측한다."""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 원문 함수 대응이며 CD의 sentinel은 빈 몸체다.
PAIRS = [{'name': 'Check', 'originals': '00462ac0', 'originalCD': '0045b560'},
         {'name': 'Free', 'originals': '00462e50', 'originalCD': '0045c460'}]
# 원문255행과 진단 입력 DWORD는 같은 격리 메모리의 서로 다른 범위를 쓴다.
SENTINEL = SCRATCH + 254 * 6
CHECKING = SCRATCH + 0x1000


class SentinelOracle(Oracle):
    """진단 UI만 반환 장치로 대체하며 표 검사/Free 쓰기는 실제 원문을 실행한다."""
    def __init__(self, edition):
        """실제 전역 표 포인터와 checking 주소를 격리 입력에 연결한다."""
        super().__init__(edition, PAIRS, [])
        self.checking = 0x5e4794 if edition == 'originals' else CHECKING  # CD에는 checking reader가 없다.
        self.mu.mem_write(0x562c18 if edition == 'originals' else 0x5207f0, struct.pack('<I', SCRATCH))
        self.action = 0  # 진단 복귀 시 적용할 입력 변경 종류다.
        self.events = []  # 원문 식/파일/행을 호출 순서로 보관한다.

    def on_instruction(self, mu, address, size, data):
        """진단 인자를 관찰하고 지정한 현재값 변경 후 원래 호출자로 복귀한다."""
        if self.stubs.get(address) != 'assert':
            return super().on_instruction(mu, address, size, data)
        esp = mu.reg_read(UC_X86_REG_ESP)  # 실제 cdecl 진단 스택이다.
        ret, expression, file, line = struct.unpack('<IIII', mu.mem_read(esp, 16))
        self.events.append(f'{line}:{self.read_string(expression).decode()}:{self.read_string(file).decode()}')
        if self.action == 1:
            mu.mem_write(self.checking, struct.pack('<I', 0))
        elif self.action == 2:
            mu.mem_write(SENTINEL + 2, struct.pack('<h', 0x7dfd))
        elif self.action == 3:
            mu.mem_write(SENTINEL + 2, struct.pack('<h', -1))
        elif self.action == 4:
            mu.mem_write(SCRATCH + 3 * 6, struct.pack('<hhh', 9, -1, -310))
        mu.reg_write(UC_X86_REG_EAX, 0)
        mu.reg_write(UC_X86_REG_ESP, esp + 4)
        mu.reg_write(UC_X86_REG_EIP, ret)

    def observe(self, operation, surfaces, used, checking, action):
        """입력을 매번 초기화하고 모델 계산 없이 기계어의 현재 표 결과를 반환한다."""
        self.action, self.events = action, []
        self.mu.mem_write(SCRATCH + 3 * 6, struct.pack('<hhh', 7, 1, 19433))
        self.mu.mem_write(SENTINEL, struct.pack('<hhh', surfaces, used, 27499))
        self.mu.mem_write(self.checking, struct.pack('<I', checking))
        self.call('Check' if operation == 'Check' else 'Free', [] if operation == 'Check' else [254 if operation == 'Invalid' else 3])
        return [bytes(self.mu.mem_read(SENTINEL, 6)).hex(), bytes(self.mu.mem_read(SCRATCH + 3 * 6, 6)).hex(),
                str(struct.unpack('<I', self.mu.mem_read(self.checking, 4))[0]), '|'.join(self.events) or '-']


def main():
    """새 출력 또는 기존 fixture 읽기 대조를 수행하며 기존 증거를 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--fixture', type=Path, default=ROOT / 'cpppj/tests/fixtures/graph-sentinel-checking-x86.tsv')
    parser.add_argument('--report', type=Path, default=ROOT / 'cpppj/graph-sentinel-checking-native-20261103.json')
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    rows, editions = [], {}  # 독립 원문 관측 행과 판본별 바이너리 해시다.
    # 양판본의 빈 sentinel·비영 DWORD·현재 행/두 번째 검사 변경을 같은 격자로 관측한다.
    for edition in ('originals', 'originalCD'):
        oracle = SentinelOracle(edition)
        # 결과는 원문 실행이 만들며 Python은 입력만 열거한다.
        for op, count, used, checking, action in itertools.product(('Check', 'Free', 'Invalid'),
                (0x7dfd, 0, -32768), (0x7dfd, 0, -1), (0, 1, 0x80000000, 0xffffffff), range(5)):
            rows.append([edition, op, str(count), str(used), str(checking), str(action)] + oracle.observe(op, count, used, checking, action))
        editions[edition] = oracle.sha256
    output = ('\n'.join('\t'.join(row) for row in rows) + '\n').encode()
    if args.verify:
        assert args.fixture.read_bytes() == output, '원문 재관측 불일치'
    else:
        if args.fixture.exists() or args.report.exists():
            raise FileExistsError('기존 산출물에는 --verify를 사용하십시오')
        args.fixture.write_bytes(output)
        args.report.write_text(json.dumps({'host': 'HJOW-Athlon', 'lastFullDecompile': 'HJOW-Athlon / 2026-11-03',
            'editions': editions, 'rows': len(rows), 'fixtureSha256': hashlib.sha256(output).hexdigest(),
            'boundary': '게임/GUI/OS/소켓 미실행·진단 UI만 명시 장치·정상 번호3/무효254'}, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'rows': len(rows), 'verify': args.verify, 'sha256': hashlib.sha256(output).hexdigest()}))


if __name__ == '__main__':
    main()
