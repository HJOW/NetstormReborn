#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Chat 초기화도 사용하는 실제 점 명령 등록0043f660의141BYTE를 제품 전에 관측한다."""
import argparse
from collections import Counter
import itertools
import json
import socket
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, TEXT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

BASE, COUNT, SIZE = 0x531b30, 0x532534, 0xa08  # 이름64×32/함수64/표시포인터65/현재count의 인접 실제 물리다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/dot-command-register-x86.tsv'  # 부모 제품 작성 전에 동결하는 최초 관측이다.
REPORT = ROOT / 'cpppj/dot-command-register-native.json'  # 원문 입력·명시 CRT 하위·최초 도구 SHA를 기록한다.


def cases():
    """정상 상한·31문자 경계·비영 플래그와 복사 callback의 현재count 변경을 독립 입력한다."""
    result = []  # 부모의 결과는 계산하지 않고 입력만 정한다.
    for count, length, flags, callback in itertools.product((0, 1, 63, 64, 65), (0, 1, 30, 31, 32, 64), (0, 1), (0, 0xabcdef01)):  # 미선택 copy와 count63의 마지막 terminator를 포함한다.
        result.append((count, length, flags, callback, 'none'))
    for mutation in ('copy-count0', 'copy-count2', 'copy-count63', 'copy-count64', 'copy-stop'):  # 현재count를 다시 읽는 효과와 물리 인접 count 덮어쓰기도 보존한다.
        result.append((1, 32, 1, 0xabcdef01, mutation))
    return result


class Oracle(MemoryOracle):
    """등록 부모만 실행하고 strncpy31은 명시적인 순방향 복사 장치로 둔다."""
    def __init__(self):
        """보호 PE의 독립 메모리 복사와 현재 표·스택의 CPU 쓰기 경계를 구성한다."""
        super().__init__()
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)
        self.total, self.completions = 0, Counter()

    def guard(self, mu, access, address, size, value, data):
        """실제 인접 표와 스택 외의 쓰기는 별도 구현으로 확대하지 않는다."""
        if not (BASE <= address and address + size <= BASE + SIZE or STACK - 512 <= address and address + size <= STACK + 32):
            raise RuntimeError(f'점 명령 등록 CPU 쓰기 범위: {address:x}/{size}')

    def instruction(self, mu, address, size, data):
        """부모의 직접 쓰기·현재count 재조회는 PE가 실행하고 외부 CRT만 명시 응답한다."""
        self.instructions += 1
        self.total += 1
        if self.instructions > 1000:
            raise RuntimeError('점 명령 등록 명령 상한')
        if 0x43f660 <= address < 0x43f6ed:
            return
        if address != 0x4e51f0:
            raise RuntimeError(f'점 명령 등록 허용되지 않은 하위: {address:x}')
        destination, source, count = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 3)
        self.trace.append(f'COPY:{destination-BASE}:{source-TEXT}:{count}')
        if count != 31:
            raise RuntimeError('점 명령 등록 strncpy 폭')
        ended = False  # 명시 CRT는 NUL 뒤 남은 count를0으로 채운다.
        for index in range(count):  # 처음31BYTE만 순방향으로 읽고 쓴다.
            value = 0 if ended else bytes(mu.mem_read(source + index, 1))[0]
            ended = ended or value == 0
            mu.mem_write(destination + index, bytes((value,)))
        mutation = self.case[-1]
        if mutation.startswith('copy-count'):
            self.put(COUNT, int(mutation.removeprefix('copy-count')))
        elif mutation == 'copy-stop':
            self.cause = 'copy-stop'
            mu.emu_stop()
            return
        self.ret(destination)

    def sample(self, case, control):
        """미작성 표의 비영 입력까지 매 실행 독립 초기화하고 전체2568BYTE를 보존한다."""
        count, length, flags, callback, _ = case
        self.case, self.trace, self.cause, self.instructions = case, [], 'return', 0
        self.mu.mem_write(BASE, bytes((165 + index * 17) & 255 for index in range(SIZE)))
        self.put(COUNT, count)
        self.mu.mem_write(TEXT, b'Q' * length + b'\0' + b'\xd7' * 128)
        self.mu.mem_write(STACK - 512, b'\xcd' * 544)
        self.put(STACK, STOP)
        self.put(STACK + 4, TEXT)
        self.put(STACK + 8, callback)
        self.put(STACK + 12, flags)
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):  # 이전 관측의 레지스터를 이어받지 않는다.
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.emu_start(0x43f660, STOP)
        if self.cause == 'return' and self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError('점 명령 등록 실제ret 불일치')
        self.completions[self.cause] += 1
        return '\t'.join((str(control), ','.join(map(str, case)), self.cause, str(self.mu.reg_read(UC_X86_REG_EAX)), bytes(self.mu.mem_read(BASE, SIZE)).hex(), ';'.join(self.trace)))


def main():
    """최초 파일 덮어쓰기를 거부하며 이후에는 보존된 모든 SHA만 검증한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    arguments = parser.parse_args()
    if arguments.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for path, expected in report['sha256'].items():  # 원문·도구·fixture를 빠짐없이 대조한다.
            if digest(ROOT / path) != expected:
                raise RuntimeError('점 명령 등록 최초 SHA 불일치: ' + path)
        print(json.dumps({'verified': True, 'observations': report['observations']}))
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('점 명령 등록 최초 자료 덮어쓰기 금지')
    oracle, inputs = Oracle(), cases()
    rows = ['# control / count,nameLength,flags,callback,mutation / completion / eax / full2568hex / trace']
    for case in inputs:  # 두 정밀도에서 같은 입력을 독립 실행하며 직접 정수 본문의 전체 표를 대조한다.
        pair = [oracle.sample(case, control) for control in (0x27f, 0x37f)]
        if pair[0].split('\t', 1)[1] != pair[1].split('\t', 1)[1]:
            raise RuntimeError('점 명령 등록 정밀도 차이')
        rows.extend(pair)
    FIXTURE.write_text('\n'.join(rows) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_dot_command_register.py', FIXTURE.relative_to(ROOT).as_posix())
    report = {'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-23', 'entry': '0043f660', 'cdEntry': '0043ee80', 'bodyBytes': 141, 'uniqueInputs': len(inputs), 'observations': len(rows)-1, 'instructions': oracle.total, 'precisionDifferences': 0, 'completions': dict(oracle.completions), 'scope': '부모만 실행/strncpy31은 명시 하위/게임·GUI·OS 아님/인접 전체2568BYTE·callback뒤현재count·표시포인터와종단쓰기. 유효한 초기count0..65이며 음수/잘못된physical은 별도 계약이다.', 'sha256': {path: digest(ROOT / path) for path in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('uniqueInputs', 'observations', 'instructions', 'completions')}))


if __name__ == '__main__':
    main()
