#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Tutorial Init 전체 제어/이름 정규화·실제 옵션/CRT와 명시 로드 요청 경계를 제품 변경 전에 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TEXT, STACK, STOP, digest
from decomp_script_command import Oracle as InvokeOracle
from decomp_cursor_position import ranges
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

MISSION, THREAD = POOL, POOL + 0x2000  # 격리 미션0xd0와 C locale TLS다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/tutorial-init-x86.tsv'  # 기존 demo 내부108입력/기대값과 별개의 첫 전체 제어 관측이다.
REPORT = ROOT / 'cpppj/tutorial-init-native.json'  # 큰 템플릿/Config/로드/헤더 경계와 실제 명령을 구별한다.


def cases():
    """이름/숫자/NUL·초기 필드와 효과 뒤 현재 재조회를 교차한다."""
    result = []
    # 실제 영어 이름 모양과 짧은 이름/경로/확장자/비ASCII C locale 입력을 보존한다.
    for name, tutorial, demo in itertools.product(('TheWarBegins', 'DEMOStormRages', '', 'a', 'ab', 'abc', 'aBcD', 'dir/FOO.bar', '\x80\xffAz'), ('', '0', '6', '-1'), ('', '0', ' +7rest')):
        result.append((name, tutorial, '257', demo, '', 9, 0xa5a50000, 7, 99, 1, 0))
    # 실제 atol의 감김/공백/잘못된 숫자와 loadFort 덮어쓰기를 별도로 교차한다.
    for tutorial, mission, demo, fort in itertools.product(('junk', '4294967296', '2147483648'), ('', '-2', '4294967295'), ('', '-1'), ('', 'OTHER_Map')):
        result.append(('Official11', tutorial, mission, demo, fort, 0xffffffff, 0xfffffffe, 0, 11, 3, 0))
    # 큰 효과가 바꾼 현재 이름/flags/구독자 입력은 다음 원본 조회에서 읽어야 한다.
    for variant, demo in itertools.product((1, 2, 3, 4), ('', '3')):
        result.append(('BEFORE_Name', '1', '2', demo, '', 5, 0x12345678, 0, 17, 1, variant))
    return result


class Oracle(MemoryOracle):
    """실제4c3290/옵션/대소문자/atol만 실행하고 큰 효과는 인자·순서로 보존한다."""
    call = InvokeOracle.call  # 정상 반환/스택/비영 보존 레지스터/x87 검사를 공유한다.

    def __init__(self):
        """현재 PC 정밀 범위와 원본 C locale을 한정된 TLS 경계로 공급한다."""
        super().__init__()
        self.allowed = []
        ranges(self, (0x4c3290, 0x425d80, 0x425db0, 0x41cc10, 0x41cb90, 0x4e6656, 0x4e5314, 0x4e5eb2))
        self.allowed.extend(((0x4e66de, 0x4e66e3), (0x4e53dc, 0x4e53fe), (0x4e5f7b, 0x4e5f9d)))
        self.put(THREAD + 0x64, self.words(0x543efc, 1)[0])
        self.normal = 0
        self.instructions = 0
        self.crt = dict(atol=0, lower=0, upper=0)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def guard(self, mu, access, address, size, value, data):
        """실제 명령의 현재 필드/옵션/한정 스택 밖 쓰기를 거부한다."""
        allowed = ((STACK - 0x2000, STACK + 0x100), (MISSION + 0x54, MISSION + 0x58), (MISSION + 0x68, MISSION + 0x6c),
                   (MISSION + 0xb0, MISSION + 0xb4), (0x5ca8e0, 0x5ca8f4), (0x540c74, 0x540c78), (0x52f7e0, 0x52f837))
        if not any(low <= address and address + size <= high for low, high in allowed):
            raise RuntimeError(f'Init 실제 쓰기 범위 밖 {address:08x}/{size}')

    def instruction(self, mu, address, size, data):
        """Config/템플릿/로드/헤더는 명시 효과이며 실제 현재 값 읽기를 대신 계산하지 않는다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        args = self.words(sp + 4, 3)
        if address == 0x4ea61d:
            self.ret(THREAD)
            return
        if address == 0x4bfb00:
            self.events.append('C')
            if self.case[10] == 1:
                self.mu.mem_write(MISSION + 4, b'AFTER_CLEAR\0')
            self.ret()
            return
        if address in (0x482ce0, 0x482c90):
            key = self.string(args[0]).decode('ascii')
            fallback = self.string(args[1]) if address == 0x482ce0 else b''
            value = dict(tutorialNumber=self.case[1], missionNumber=self.case[2], demo=self.case[3], loadFort=self.case[4])[key].encode('latin1').split(b'\0')[0]
            if not value:
                value = fallback
            pointer = TEXT + self.text_index * 512
            self.text_index += 1
            self.mu.mem_write(pointer, value + b'\0')
            self.put(MISSION + 0x60, pointer)
            self.events.append('G,' + key + ',' + fallback.hex() + ',' + value.hex())
            self.ret(pointer, 8 if address == 0x482ce0 else 4)
            return
        if address == 0x41cc10:
            self.events.append('O')
            if self.case[10] == 2:
                self.mu.mem_write(MISSION + 4, b'AFTER_OPTIONS\0')
                self.put(0x54265c, 7)
        if address == 0x4404e0:
            self.events.append('S,' + self.string(args[0]).hex() + ',' + self.string(args[1]).hex())
            self.scratch = bytes(mu.mem_read(args[1], 80)).hex()
            if self.case[10] == 3:
                self.mu.mem_write(MISSION + 4, b'AFTER_SET\0')
                self.put(0x54265c, 0x76543210)
            self.ret(0, 8)
            return
        if address == 0x4b51e0:
            self.events.append('L,' + self.string(args[0]).hex())
            if self.case[10] == 4:
                self.put(MISSION + 0x54, 0xfffffffe)
            self.ret()
            return
        if address == 0x482eb0:
            self.events.append('H')
            if self.case[10] == 4:
                self.put(MISSION + 0x54, self.words(MISSION + 0x54, 1)[0] ^ 0x102)
            self.ret()
            return
        if address in (0x4e6656, 0x4e53dc, 0x4e5f7b):
            self.crt[{0x4e6656: 'atol', 0x4e53dc: 'lower', 0x4e5f7b: 'upper'}[address]] += 1
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'미검토 Init 명령 {address:08x}')
        self.instructions += 1

    def sample(self, case):
        """정상 반환 뒤 필드/전역/정규화80BYTE·원래 이름/옵션/큰 효과 순서를 보존한다."""
        self.case = case
        self.events = []
        self.text_index = 0
        self.scratch = ''
        self.mu.mem_write(STACK - 0x2000, b'\xa5' * 0x2100)
        self.mu.mem_write(MISSION, b'\xcc' * 0xd0)
        self.mu.mem_write(MISSION + 4, case[0].encode('latin1') + b'\0')
        self.put(MISSION + 0x5c, POOL + 0x800)
        self.put(MISSION + 0x54, case[6])
        self.put(MISSION + 0x68, case[5])
        self.put(0x5ca8e0, 0x11111111)
        self.put(0x5ca8e4, 0x22222222)
        self.put(0x5ca8f0, case[7])
        self.put(0x540c74, case[8])
        self.put(0x54265c, case[9])
        self.mu.mem_write(0x52f7e0, bytes((index * 11 + 17) & 255 for index in range(87)))
        self.call(0x4c3290, [], MISSION)
        self.normal += 1
        fields = [self.words(MISSION + offset, 1)[0] for offset in (0x54, 0x68, 0xb0)] + [self.words(address, 1)[0] for address in (0x5ca8e4, 0x5ca8e0, 0x5ca8f0, 0x540c74)]
        return [','.join(map(str, fields)), self.scratch, self.string(MISSION + 4).hex(), bytes(self.mu.mem_read(0x52f7e0, 87)).hex(), ';'.join(self.events)]


def main():
    """기존 독립 원문은 덮어쓰지 않고 새 전체 제어 관측만 처음 한 번 저장한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    if parser.parse_args().verify:
        record = json.loads(REPORT.read_text('utf-8'))
        for name, expected in record['sha256'].items():
            assert digest(ROOT / name) == expected, name
        print('Tutorial Init 최초 전체 제어 관측 보존 확인')
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('최초 관측 덮어쓰기 금지')
    oracle = Oracle()
    lines = []
    # 입력/출력은 제품과 무관하며 실제 명령의 반환 상태를 기록한다.
    for case in cases():
        lines.append(json.dumps(case, ensure_ascii=True, separators=(',', ':')) + '\t' + '\t'.join(oracle.sample(case)))
    FIXTURE.write_text('# 실제 Init 전체 제어/CRT/옵션과 명시 큰 효과 경계다.\n' + '\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    names = ['originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_tutorial_init.py', 'cpppj/tests/fixtures/tutorial-init-x86.tsv']
    record = {'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-20', 'uniqueInputs': len(lines), 'normalFunctionReturns': oracle.normal, 'instructions': oracle.instructions, 'crtCalls': oracle.crt,
              'gameExecuted': False, 'osExecuted': False, 'scope': '실제4c3290 전체 제어/425d80·425db0·C locale lower/upper/atol/41cc10·41cb90. TLS 포인터만 공급하고 템플릿 정리/Config/로드 요청·기술 Header는 명시 효과다. 전체 월드/State7/실제 화면 관측이 아니다.',
              'sha256': {name: digest(ROOT / name) for name in names}}
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: record[key] for key in ('uniqueInputs', 'normalFunctionReturns', 'instructions', 'crtCalls')}))


if __name__ == '__main__':
    main()
