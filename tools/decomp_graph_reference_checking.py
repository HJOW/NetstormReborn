#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""양판본 Graph 참조 감소/SID0 조회를 원문 기계어로 실행하고 진단 복귀를 관찰한다."""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 기본 출력은 새 관측이며 기존 파일은 --verify로 읽기 비교만 한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/graph-reference-checking-x86.tsv'
REPORT = ROOT / 'cpppj/graph-reference-checking-native-20261103.json'
# 전체 정밀 산출물에 있는 실제 두 부모와 CD의 빈/free 하위다.
PAIRS = [{'name': 'Remove', 'originals': '00462a50', 'originalCD': '0045b530'},
         {'name': 'Zero', 'originals': '00462bc0', 'originalCD': '0045b6f0'}]


class GraphOracle(Oracle):
    """진단 UI만 명시 callback으로 반환하며 현재6BYTE 행과 checking 쓰기를 관찰한다."""
    def __init__(self, edition):
        """허용한 실제 부모/하위 범위와 격리된 원본6BYTE 입력을 구성한다."""
        super().__init__(edition, PAIRS, [] if edition == 'originals' else [0x45b520, 0x45b4d0])
        self.checking = 0x5e4794 if edition == 'originals' else SCRATCH + 0x100  # CD 몸체에는 checking 참조가 없다.
        self.action = 0  # 진단 callback의 명시 상태 변경 입력이다.
        self.events = []  # 실제 assertion의 식/파일/행을 순서대로 보존한다.

    def on_instruction(self, mu, address, size, data):
        """실제 진단 인자를 읽고 지정한 변경 뒤 cdecl로 원래 명령에 복귀한다."""
        if self.stubs.get(address) != 'assert':
            return super().on_instruction(mu, address, size, data)
        esp = mu.reg_read(UC_X86_REG_ESP)  # 원문 call의 현재 스택이다.
        ret, expression, file, line = struct.unpack('<IIII', mu.mem_read(esp, 16))
        self.events.append(f'{line}:{self.read_string(expression).decode()}:{self.read_string(file).decode()}')
        if self.action == 1:
            mu.mem_write(self.checking, struct.pack('<I', 0))
        elif self.action == 2:
            mu.mem_write(SCRATCH, struct.pack('<hhh', 3, 1, -310))
        elif self.action == 3:
            mu.mem_write(SCRATCH, struct.pack('<h', 4))
        elif self.action == 4:
            mu.mem_write(SCRATCH + 2, struct.pack('<h', 1))
        mu.reg_write(UC_X86_REG_EAX, 0)
        mu.reg_write(UC_X86_REG_ESP, esp + 4)
        mu.reg_write(UC_X86_REG_EIP, ret)

    def observe(self, operation, surfaces, used, checking, action):
        """행/전역을 매 입력 초기화하고 기대값 계산 없이 원문 출력만 반환한다."""
        self.action, self.events = action, []
        self.mu.mem_write(SCRATCH, struct.pack('<hhh', surfaces, used, 19433))
        self.mu.mem_write(self.checking, struct.pack('<I', checking))
        result = self.call(operation, [] if operation == 'Remove' else [0], SCRATCH if operation == 'Remove' else 0)
        return [bytes(self.mu.mem_read(SCRATCH, 6)).hex(),
                str(struct.unpack('<I', self.mu.mem_read(self.checking, 4))[0]),
                '|'.join(self.events) or '-', str(result) if operation == 'Zero' else '-']


def generate():
    """검토한 signed 경계/비영 DWORD/진단 중 쓰기 격자를 두 PE에서 독립 실행한다."""
    rows, editions = [], {}  # 입력/원문 출력과 판본별 바이너리/추출물 근거다.
    for edition in ('originals', 'originalCD'):
        oracle = GraphOracle(edition)
        # CD도 같은 입력을 주되 진단 없는 원문이 callback을 호출하지 않음을 기록한다.
        for count, used, checking, action in itertools.product((-32768, -1, 0, 1, 2, 32767), (0, 1, -1),
                                                               (0, 1, 0x80000000, 0xffffffff), range(5)):
            rows.append([edition, 'Remove', str(count), str(used), str(checking), str(action)] +
                        oracle.observe('Remove', count, used, checking, action))
        # SID0은 진단에서 돌아와도 다른 TYPE/좌표/풀 함수를 호출하지 않고0으로 끝난다.
        for checking, action in itertools.product((0, 1, 0x80000000, 0xffffffff), range(5)):
            rows.append([edition, 'Zero', '0', '0', str(checking), str(action)] +
                        oracle.observe('Zero', 0, 0, checking, action))
        facts = ROOT / f'extracted/refined/{edition}/functions.tsv'
        editions[edition] = {'binarySha256': oracle.sha256, 'rows': sum(r[0] == edition for r in rows),
                             'functionsSha256': hashlib.sha256(facts.read_bytes()).hexdigest()}
    return ('\n'.join('\t'.join(row) for row in rows) + '\n').encode(), editions


def main():
    """별도 출력/읽기 검증을 지원하고 기존 fixture를 자동 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--fixture', type=Path, default=FIXTURE)
    parser.add_argument('--report', type=Path, default=REPORT)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    output, editions = generate()
    if args.verify:
        assert args.fixture.read_bytes() == output, '기존 fixture와 원문 재관측 불일치'
    else:
        if args.fixture.exists() or args.report.exists():
            raise FileExistsError('기존 산출물에는 --verify를 사용하십시오')
        args.fixture.parent.mkdir(parents=True, exist_ok=True)
        args.fixture.write_bytes(output)
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps({'host': 'HJOW-Athlon', 'lastFullDecompile': 'HJOW-Athlon / 2026-10-23',
            'editions': editions, 'rows': output.count(b'\n'), 'fixtureSha256': hashlib.sha256(output).hexdigest(),
            'toolSha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'parentToolSha256': hashlib.sha256((ROOT / 'tools/decomp_oracle.py').read_bytes()).hexdigest(),
            'boundary': '실제 PE/허용 함수 명령만 실행·진단 UI만 관찰/변경/복귀·게임/OS/소켓 미실행'},
            ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'rows': output.count(b'\n'), 'sha256': hashlib.sha256(output).hexdigest(), 'verify': args.verify}))


if __name__ == '__main__':
    main()
