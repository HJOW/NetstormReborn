#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Dot 등록의 count65 인접 쓰기와 음수의 미지원 물리 경계를 교정 제품 전에 별도로 관측한다."""
import argparse
import json
import socket
from decomp_dot_command_register import Oracle as FirstOracle, BASE, COUNT, SIZE
from decomp_priestmenu import ROOT, STACK, TEXT, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP

# 첫250관측은 읽기만 하며 새 보충 관측을 같은 파일에 덮어쓰지 않는다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/dot-command-register-v2-x86.tsv'
REPORT = ROOT / 'cpppj/dot-command-register-v2-native.json'
BEFORE = ROOT / 'extracted/cpp-dot-command-register-20261024-before-correction'


class PhysicalBoundary(Exception):
    """부모의 물리 밖 CPU쓰기 또는 외부 strncpy 진입을 명시 경계에서 중단한다."""


class Oracle(FirstOracle):
    """기존 부모 PE/CRT 명시장치를 재사용하고 첫 표 밖 접근만 새 관측 범위로 기록한다."""
    def guard(self, mu, access, address, size, value, data):
        """count65가 만든 실제 마지막 DWORD store 앞까지의 인접 표 전체를 보존한다."""
        if not (BASE <= address and address + size <= BASE + SIZE or STACK - 512 <= address and address + size <= STACK + 32):
            self.cause = 'physical-store-stop'
            self.boundary = f'STORE:{address:x}:{size}:{value}'
            raise PhysicalBoundary(self.boundary)

    def instruction(self, mu, address, size, data):
        """음수 count의 표 앞 strncpy는 실제 CRT 성공으로 가장하지 않고 진입 인자만 기록한다."""
        if address == 0x4e51f0:
            destination, source, count = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 3)
            if not (BASE <= destination and destination + count <= BASE + SIZE):
                self.instructions += 1
                self.total += 1
                self.trace.append(f'COPY:{destination-BASE}:{source-TEXT}:{count}')
                self.cause = 'name-copy-stop'
                self.boundary = f'COPY:{destination:x}:{count}'
                raise PhysicalBoundary(self.boundary)
        super().instruction(mu, address, size, data)

    def sample(self, case, control):
        """교정 전 제품은 실행하지 않고 실제 원문 부모의 부분효과와 중단 주소만 반환한다."""
        self.boundary = '-'
        try:
            return super().sample(case, control) + '\t' + self.boundary
        except PhysicalBoundary:
            self.completions[self.cause] += 1
            return '\t'.join((str(control), ','.join(map(str, case)), self.cause,
                              str(self.mu.reg_read(UC_X86_REG_EAX)), bytes(self.mu.mem_read(BASE, SIZE)).hex(),
                              ';'.join(self.trace), self.boundary))


def main():
    """기존 자료와 교정 전 제품 SHA를 확인하고 최초 v2 관측만 작성한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for relative, expected in report['sha256'].items():
            if digest(ROOT / relative) != expected:
                raise RuntimeError('Dot v2 최초 SHA 불일치: ' + relative)
        print(json.dumps({'verified': True, 'observations': report['observations']}))
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('Dot v2 최초 관측 덮어쓰기 금지')
    baseline = json.loads((BEFORE / 'sha256.json').read_text(encoding='utf-8'))
    product = ('cpppj/src/client/RawDotCommandRegister.h', 'cpppj/src/client/RawDotCommandRegister.cpp', 'cpppj/tests/DotCommandRegisterTests.cpp')
    for relative in product:
        if digest(ROOT / relative) != baseline[relative]:
            raise RuntimeError('Dot 교정 전 관측 요건 위반: ' + relative)
    first = json.loads((ROOT / 'cpppj/dot-command-register-native.json').read_text(encoding='utf-8'))
    for relative, expected in first['sha256'].items():
        if digest(ROOT / relative) != expected:
            raise RuntimeError('Dot 최초250 자료 불변 요건 위반: ' + relative)
    inputs = [(1, 32, flag, 0xabcdef01, mutation) for flag in (0, 1) for mutation in ('copy-count64', 'copy-count65')]
    inputs += [(0xffffffff, 32, flag, 0xabcdef01, 'none') for flag in (0, 1)]
    lines = ['# control / count,nameLength,flags,callback,mutation / completion / eax / full2568hex / trace / physicalBoundary']
    completions = {}
    instructions = 0
    for case in inputs:
        pair = []
        for control in (0x27f, 0x37f):
            oracle = Oracle()
            line = oracle.sample(case, control)
            pair.append(line)
            instructions += oracle.instructions
            completions[oracle.cause] = completions.get(oracle.cause, 0) + 1
        if pair[0].split('\t', 1)[1] != pair[1].split('\t', 1)[1]:
            raise RuntimeError('Dot v2 정밀도 차이')
        lines.extend(pair)
    FIXTURE.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'tools/decomp_priestmenu.py', 'tools/decomp_dot_command_register.py',
             'tools/decomp_dot_command_register_v2.py', 'cpppj/dot-command-register-native.json',
             'cpppj/tests/fixtures/dot-command-register-x86.tsv', FIXTURE.relative_to(ROOT).as_posix(),
             'extracted/cpp-dot-command-register-20261024-before-correction/sha256.json')
    report = {'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-23',
              'entry': '0043f660', 'cdEntry': '0043ee80', 'bodyBytes': 141, 'cdExecuted': False,
              'uniqueInputs': len(inputs), 'observations': len(lines)-1, 'instructions': instructions,
              'precisionDifferences': 0, 'completions': completions, 'correctionAbsentBeforeObservation': True,
              'scope': 'count64 정상/복사뒤count65의표밖종단store직전부분효과/진입count-1은strncpy실행전표앞인자경계. 실제CRT·OS접근위반은실행하지않음.',
              'beforeProductSha256': {relative: baseline[relative] for relative in product},
              'sha256': {relative: digest(ROOT / relative) for relative in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('uniqueInputs', 'observations', 'instructions', 'completions')}))


if __name__ == '__main__':
    main()
