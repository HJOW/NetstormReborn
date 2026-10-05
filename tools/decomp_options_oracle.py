#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 설정 저장·섹션 선택·XOR 쓰기 기계어를 격리 실행한다. 원본 게임·OS API는 실행하지 않는다.

BaseFile의 파일 열기·닫기·ftell·fwrite는 가상 파일로 대체한다. XOR는 두 판본 원본 기계어가 수행한다.
파일 이름 객체는 마지막 구분자 뒤 이름만 돌려주는 대체를 사용한다. 실제 디스크는 읽기 전용이다.
python tools/decomp_options_oracle.py
"""
import argparse
import csv
import hashlib
import json
import struct
from pathlib import Path

from decomp_config_oracle import ConfigOracle, OBJECTS, SOURCE
from decomp_oracle import ROOT
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP
from cpp_config_smoke import run
from exe_callscan import load_sections, dump_table, read_cstring, scan_calls, disassemble

# 검토한 저장 진입점·XOR 쓰기·CD 정리 꼬리. 섹션 읽기와 스택 확보는 ConfigOracle의 원본 몸체를 쓴다.
FUNCTIONS = {'originals': [0x440f30, 0x41a910],
             'originalCD': [0x4a9270, 0x4bb180, 0x4a93a8, 0x4a93ba]}
# 저장 버퍼·변경 표시를 가진 원본 전역 설정 객체.
CONFIGURATION = {'originals': 0x558de8, 'originalCD': 0x56f190}
# 가상 파일과 이름 객체 대체 함수: (역할, ret N). cdecl은 ret 0이다.
STUBS = {
    'originals': {0x45a220: ('filename', 4), 0x459b20: ('basename', 4), 0x4595f0: ('noop', 0),
                  0x41a5a0: ('construct', 0), 0x41b4c0: ('open', 12), 0x41a620: ('key', 4),
                  0x41b410: ('close', 0), 0x4e7300: ('strncmp', 0), 0x4e566c: ('tell', 0),
                  0x4e44a6: ('write', 0)},
    'originalCD': {0x49c680: ('filename', 4), 0x49cbf0: ('basename', 4), 0x49c6b0: ('noop', 0),
                   0x4ba590: ('construct', 0), 0x4ba700: ('open', 12), 0x4ba680: ('key', 4),
                   0x4ba5a0: ('close', 0), 0x4f2720: ('strncmp', 0), 0x4baf40: ('tell', 0),
                   0x4f21e0: ('write', 0)},
}
# 생성 결과·실제 파일 대조를 남길 Git 제외 폴더.
OUTPUT = ROOT / 'extracted/cpp-options-oracle'
# 재현 가능한 저장 입력 수. 실제 두 판본 설정 버퍼 2개를 별도로 더한다.
SYNTHETIC_CASES = 14


class OptionsOracle(ConfigOracle):
    """설정 저장의 파일 경계를 가상화하고 저장 함수·XOR 루프는 원본 기계어 그대로 실행한다."""

    def __init__(self, edition):
        """기존 설정 oracle의 허용 몸체에 검토한 저장·쓰기 몸체만 추가한다."""
        super().__init__(edition)
        with (ROOT / f'extracted/refined/{edition}/functions.tsv').open(encoding='utf-8') as file:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(file, delimiter='\t')}
        # CD의 정리 꼬리가 함수 범위 밖에 남는 경우에도 명시적으로 검토한 구간만 허용한다.
        for address in FUNCTIONS[edition]:
            # 함수가 가진 모든 실제 몸체 구간을 허용 목록에 넣는다.
            for part in facts[address]['ranges'].split(';'):
                lo, hi = (int(value, 16) for value in part.split('-'))
                self.allowed.append((lo, hi + 1))
        self.entries['Save'] = FUNCTIONS[edition][0]
        self.options_stubs = STUBS[edition]
        self.names = {}
        self.output = bytearray()
        self.closed = False

    def on_instruction(self, mu, address, size, data):
        """가상 파일 I/O와 이름 객체만 대체한다. 나머지 주소는 기존 oracle의 허용 검사를 받는다."""
        stub = self.options_stubs.get(address)
        if stub is None:
            return super().on_instruction(mu, address, size, data)
        role, purge = stub
        esp = mu.reg_read(UC_X86_REG_ESP)
        ret, a, b, c, d = struct.unpack('<IIIII', mu.mem_read(esp, 20))
        this = mu.reg_read(UC_X86_REG_ECX)
        value = 0
        if role == 'filename':
            self.names[this] = a
        elif role == 'basename':
            path = self.names[this]
            name = self.read_string(path)
            value = path + max(name.rfind(b'\\'), name.rfind(b':')) + 1
        elif role == 'construct':
            mu.mem_write(this, bytes(24))
        elif role == 'open':
            if self.read_string(a) != self.file_name or (b, c) != (2, 1):
                raise AssertionError('원본 저장의 파일 이름·w+b 모드 불일치')
            mu.mem_write(this, struct.pack('<IIIIII', 1, 0, 0, 0, 0, 0))
        elif role == 'key':
            key = self.read_string(a)
            if key != b'mydoghasfleas':
                raise AssertionError('원본 XOR 키 불일치')
            mu.mem_write(this + 0x10, struct.pack('<II', a, len(key)))
        elif role == 'close':
            self.closed = True
        elif role == 'strncmp':
            left, right = self.read_bounded(a, c), self.read_bounded(b, c)
            value = (left > right) - (left < right)
        elif role == 'tell':
            value = len(self.output)
        elif role == 'write':
            if d != 1:
                raise AssertionError('가상 파일 밖 쓰기')
            self.output += bytes(mu.mem_read(a, b * c))
            value = c
        mu.reg_write(UC_X86_REG_EAX, value & 0xffffffff)
        mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge)
        mu.reg_write(UC_X86_REG_EIP, ret)

    def save(self, text, filename=b'd\\options.cfg'):
        """원본 설정 객체에 평문을 놓고 저장 결과·변경 표시·정상 닫기를 확인한다."""
        self.setup([(None, text, False)])
        self.mu.mem_write(CONFIGURATION[self.edition], bytes(self.mu.mem_read(OBJECTS, 24)))
        self.mu.mem_write(CONFIGURATION[self.edition] + 0x14, struct.pack('<I', 1))
        self.mu.mem_write(SOURCE, filename + b'\0')
        self.file_name = filename
        self.names.clear()
        self.output.clear()
        self.closed = False
        self.call('Save', [SOURCE])
        if not self.closed or bytes(self.mu.mem_read(CONFIGURATION[self.edition] + 0x14, 4)) != bytes(4):
            raise AssertionError('원본 저장 후 닫기·변경 표시 해제 누락')
        return bytes(self.output)


def inputs():
    """서명·빈 섹션·파일 이름·모든 비ASCII 바이트·4KB XOR 블록 경계를 포함한다."""
    cases = [
        b'', b'[END]\r\n', b'[options.cfg]\r\n[END]\r\n',
        b'[options.cfg]\r\nW=800\r\n[END]\r\nH=600\r\n',
        b'[options.cfg]\r\nmQdsTW=800\r\n[END]\r\nH=600\r\n',
        b'[options.cfg]\r\nmQds\r\n[END]\r\n',
        b'[END]\r\nInstallDir="C:\\NS"\r\n',
        b'[options.cfg]\r\nmQdsTInstallDir="old"\r\n[setup.cfg]\r\nIgnored=1\r\n[END]\r\nInstallDir="new"\r\n',
        b'[OPTIONS.CFG]\r\nName="caf\xe9 \x80"\r\n[END]\r\n',
        b'[options.cfg]\r\n' + bytes(range(0x80, 0x100)) + b'\r\n[END]\r\n',
        b'[options.cfg]\r\n' + b'x' * 4095 + b'\r\n[END]\r\nLast=1\r\n',
        b'[options.cfg]\r\nmQdsT' + b'y' * 8192 + b'\r\n[END]\r\nLast=2\r\n',
        b'[options.cfg]\r\nFirst=1\r\n[options.cfg]\r\nSecond=2\r\n[END]\r\n',
        b'[other.cfg]\r\nW=640\r\n[END]\r\nH=480\r\n',
    ]
    if len(cases) != SYNTHETIC_CASES:
        raise AssertionError('저장 oracle 입력 수 불일치')
    return cases


def static_facts():
    """경로 포인터·직접 호출 지점·전체화면 분기를 원본 PE에서 읽어 재현 가능한 정적 근거로 남긴다."""
    facts = {}
    # 두 판본의 경로 전역과 저장·시작 표시 함수 대응은 역어셈블로 확인했다.
    for edition, path_globals, functions, branch in (
        ('originals', (0x540cec, 0x540cf8), (0x441d10, 0x440f30, 0x441de0, 0x436dd0, 0x436df0, 0x436e40), (0x4395a4, 0x439602)),
        ('originalCD', (0x510940, 0x51094c), (0x4a9090, 0x4a9270, 0x4a9c30, 0x487670, 0x4876a0, 0x487720), (0x486253, 0x4862b0)),
    ):
        binary = ROOT / ('originals/Netstorm.exe' if edition == 'originals' else 'originalCD/NETSTORM.EXE')
        data = binary.read_bytes()
        sections = load_sections(data)
        paths = {}
        # 전역 자체가 아니라 전역에 저장된 문자열 포인터를 한 번 역참조한다.
        for address in path_globals:
            pointer = dump_table(data, sections, address, 1)[0][2]
            paths[f'{address:08x}'] = {'pointer': f'{pointer:08x}', 'text': read_cstring(data, sections, pointer)}
        facts[edition] = {'paths': paths,
                          'direct_calls': {f'{address:08x}': [f'{call:08x}' for call in scan_calls(data, sections, address)]
                                           for address in functions},
                          'fullscreen_branch': [f'{address:08x} {raw} {instruction}'
                                                for address, raw, instruction in disassemble(data, sections, *branch)]}
    return facts


def main():
    """두 판본의 같은 출력만 UTF-8 소스에서 읽을 수 있는 16진수 기대값과 실행 근거로 남긴다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT / 'cpppj/build/bin/Release/NetstormCpp.exe')
    args = parser.parse_args()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    oracles = {edition: OptionsOracle(edition) for edition in FUNCTIONS}
    cases = inputs()
    comparisons = {}
    # 실제 설정도 C++가 읽은 버퍼를 같은 원본 x86 저장 함수 두 개에 넣어 교차 대조한다.
    for edition in FUNCTIONS:
        flag = ['--cd'] if edition == 'originalCD' else []
        text = run(args.exe, '--config-dump', ROOT / edition, *flag).stdout.decode('utf-8').encode('cp1252')
        cases.append(text)
        saved = OUTPUT / f'{edition}-options.cfg'
        run(args.exe, '--config-save', ROOT / edition, saved, *flag)
        expected = oracles[edition].save(text)
        if saved.read_bytes() != expected:
            raise AssertionError(f'C++ 실제 저장 바이트 불일치: {edition}')
        comparisons[edition] = {'bytes': len(expected), 'sha256': hashlib.sha256(expected).hexdigest()}
    rows = ['# filename_hex configuration_cp1252_hex encoded_x86_hex (두 판본 출력 일치)']
    # 빈 출력도 '-'로 표시해 TSV 칸을 유지한다.
    for text in cases:
        filename = b'C:\\NS\\d\\options.cfg'
        outputs = [oracle.save(text, filename) for oracle in oracles.values()]
        if outputs[0] != outputs[1]:
            raise AssertionError('판본별 저장 출력 불일치')
        rows.append('\t'.join(value.hex() or '-' for value in (filename, text, outputs[0])))
    fixture = ROOT / 'cpppj/tests/fixtures/options-x86.tsv'
    fixture.write_text('\n'.join(rows) + '\n', encoding='utf-8', newline='\n')
    evidence = {'kind': 'isolated-x86-options-save', 'cases': len(cases), 'functions': FUNCTIONS,
                'binary_sha256': {edition: oracle.sha256 for edition, oracle in oracles.items()},
                'fixture_sha256': hashlib.sha256(fixture.read_bytes()).hexdigest(),
                'real_files': comparisons, 'original_game_started': False,
                'original_machine_code': 'Save → Section → BaseFile::Write(XOR 포함)',
                'stubs': STUBS, 'static_facts': static_facts(),
                'limits': '파일 이름 객체·열기·닫기·tell·write·strncmp는 대체; 실제 OS I/O 검증은 별도 스모크'}
    (ROOT / 'cpppj/recovery-options-evidence.json').write_text(
        json.dumps(evidence, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'cases': len(cases), 'real_files': comparisons}, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
