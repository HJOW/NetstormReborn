#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""004be960의 인라인 SP writer·실제_ftol·동일 GameRandom 전체를 제품 전에 관측한다."""
import argparse
from collections import Counter
import itertools
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

# 실제 함수와 외부 장치, 관측할 같은 물리 세 벌의 경계다.
RANGES = ((0x4be960, 0x4bea6b), (0x4e49c0, 0x4e4a35), (0x4558c0, 0x4558e8))
BUFFERS = (POOL+0x1000, POOL+0x2000, POOL+0x3000)
STREAM = BUFFERS[2]
RANDOM = 0x532710
FIXTURE = ROOT/'cpppj/tests/fixtures/fort-sp-write-x86.tsv'
REPORT = ROOT/'cpppj/fort-sp-write-native.json'


def cases():
    """원시 float32·signed 진단·현재 장치 변경/중단만 입력하고 기대 출력을 계산하지 않는다."""
    floats = (0, 0x80000000, 1, 0x3f000000, 0x3fc00000, 0x40200000, 0xbf400000, 0xbfc00000,
              0x477fff00, 0x4effffff, 0x4f000000, 0xcf000000, 0x4f800000, 0x5effffff,
              0x5f000000, 0xdf000000, 0xdf000001, 0x7f800000, 0xff800000, 0x7fc12345, 0x7f812345)
    base = [0x477fff00, 55, 3, 9, 128, 165, 0, 'none']
    rows = [(bits, seed, 3, 9, 128, 165, 0, 'none') for bits, seed in itertools.product(floats, (0, 55, 0xffffffff))]
    for capacity in (0, 7, 8, 23, 24, 87, 88, 0xffffffff, 0x80000000):
        row = base.copy();row[2:5] = 0, 0, capacity;rows.append(tuple(row))
    for end in (0, 200, 0x80000000, 0xffffffff):
        row = base.copy();row[3] = end;rows.append(tuple(row))
    for pattern in (0, 165):
        row = base.copy();row[2:7] = 8, 9, 128, pattern, 2;rows.append(tuple(row))
    changes = ('r1-seed0', 'r16-baseB', 'r128-baseB', 'r144-baseA', 'r256-stream', 'r512-position240',
               'r17-capacity0', 'a1-baseB', 'a1-endHigh', 'a1-position17', 'a1-seed0')
    stops = ('r1-stop', 'r16-stop', 'r128-stop', 'r129-stop', 'r256-stop', 'r257-stop', 'r512-stop',
             'a1-stop', 'a9-stop', 'a17-stop')
    for mutation in (*changes, *stops):
        row = base.copy();row[7] = mutation
        if mutation.startswith('a'):row[4] = 0
        rows.append(tuple(row))
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """세 실제 PE 몸체는 CPU 실행하고 진단만 명시 callback으로 분리한다."""
    def __init__(self):
        """독립 PE·스택·현재16BYTE/버퍼에 쓰기 경계와 현재 입력을 구성한다."""
        super().__init__();self.running = False;self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def write(self, mu, access, address, size, value, data):
        """실제 RNG DWORD·동일 stream/세 버퍼·스택 밖의 CPU 쓰기는 거부한다."""
        if self.running and not any(first <= address and address+size <= last for first, last in
                                    (*((base, base+256) for base in BUFFERS), (RANDOM, RANDOM+4), (STACK-4096, STACK+64))):
            raise RuntimeError('SP writer 예상 밖 쓰기: '+hex(address))

    def mutate(self, boundary):
        """명시 하위 복귀 뒤 같은 전역/필드를 바꾸거나 중단하여 부모의 재조회를 관측한다."""
        trigger, _, action = self.case[7].partition('-')
        if trigger != boundary:return
        if action == 'stop':self.status = 'stop';self.mu.emu_stop()
        elif action == 'seed0':self.put(RANDOM, 0)
        elif action in ('baseA', 'baseB'):
            self.put(STREAM, BUFFERS[action == 'baseB']);self.put(STREAM+4, 5);self.put(STREAM+8, 1);self.put(STREAM+12, 128)
        elif action == 'stream':self.put(STREAM, STREAM);self.put(STREAM+4, 8)
        elif action == 'position240':self.put(STREAM+4, 240);self.put(STREAM+12, 256)
        elif action == 'capacity0':self.put(STREAM+12, 0)
        elif action == 'endHigh':self.put(STREAM+8, 0x80000000)
        elif action == 'position17':self.put(STREAM+4, 17);self.put(STREAM+12, 256)
        else:raise RuntimeError('SP writer 미해석 변경: '+action)

    def instruction(self, mu, address, size, data):
        """실제_ftol 반환과512난수/진단 순서를 기록하며 결과를 대신 계산하지 않는다."""
        self.instructions += 1
        if self.instructions > 30000:raise RuntimeError('SP writer 명령 상한')
        if address == 0x4be972:
            self.converted = mu.reg_read(UC_X86_REG_EAX);self.trace.append('F:'+str(self.converted))
        if address == 0x4558c0:
            if self.words(mu.reg_read(UC_X86_REG_ESP)+4, 1)[0] != 32:raise RuntimeError('SP writer 난수 상한')
            self.randoms += 1
        if address == 0x4be99b:
            result = mu.reg_read(UC_X86_REG_EAX);state = self.words(RANDOM, 1)[0]
            self.equal += result == mu.reg_read(UC_X86_REG_EBP)
            self.trace.append(f'R:{self.randoms}:{result}:{state}');self.mutate('r'+str(self.randoms))
        if any(first <= address < last for first, last in RANGES):return
        if address != 0x4e0620:raise RuntimeError('SP writer 허용 밖 명령: '+hex(address))
        condition, source, line = self.words(mu.reg_read(UC_X86_REG_ESP)+4, 3)
        if self.string(condition) != b'pos+length <= alloc' or self.string(source) != b'c:\\ns\\o\\memstream.h' or line != 36:
            raise RuntimeError('SP writer 실제 진단 문자열/줄 차이')
        self.assertions += 1;self.trace.append('A:'+str(line));self.mutate('a'+str(self.assertions))
        if self.status == 'normal':self.ret()

    def sample(self, case, control):
        """매 입력 실제 전체물리/ABI/FPU를 초기화하고 정상 또는 명시 부분중단을 보존한다."""
        self.case = case;self.instructions = self.randoms = self.assertions = self.equal = 0
        self.status = 'normal';self.trace = [];self.converted = None;self.running = False
        bits, seed, position, end, capacity, pattern, mode, _ = case
        for index, base in enumerate(BUFFERS):self.mu.mem_write(base, bytes((pattern+index*37+byte*11)&255 for byte in range(256)))
        self.mu.mem_write(STREAM, struct.pack('<4I', BUFFERS[mode], position, end, capacity));self.put(RANDOM, seed)
        self.mu.mem_write(STACK-4096, b'\xcd'*(4096+64));self.mu.mem_write(STACK, struct.pack('<3I', STOP, STREAM, bits))
        for register, value in ((UC_X86_REG_EAX, 0), (UC_X86_REG_EBX, 0x12345678), (UC_X86_REG_ECX, 0),
                                (UC_X86_REG_EDX, 0), (UC_X86_REG_ESI, 0x23456789), (UC_X86_REG_EDI, 0x3456789a), (UC_X86_REG_EBP, 0x456789ab)):
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0);self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff);self.running = True
        self.mu.emu_start(0x4be960, STOP);self.running = False
        if self.status == 'normal':
            if self.mu.reg_read(UC_X86_REG_ESP) != STACK+4 or self.mu.reg_read(UC_X86_REG_EIP) != STOP:raise RuntimeError('SP writer cdecl 스택 차이')
            for register, value in ((UC_X86_REG_EBX, 0x12345678), (UC_X86_REG_ESI, 0x23456789), (UC_X86_REG_EDI, 0x3456789a), (UC_X86_REG_EBP, 0x456789ab)):
                if self.mu.reg_read(register) != value:raise RuntimeError('SP writer 비휘발 레지스터 차이')
            if self.randoms != 512 or self.mu.reg_read(UC_X86_REG_FPCW) != control or (self.mu.reg_read(UC_X86_REG_FPSW)>>11)&7:raise RuntimeError('SP writer RNG/FPU 균형 차이')
        outputs = [bytes(self.mu.mem_read(base, 256)).hex() for base in BUFFERS]
        return '\t'.join((str(control), ','.join(map(str, case)), self.status, str(self.converted), str(self.words(RANDOM, 1)[0]),
                          str(self.randoms), str(self.assertions), str(self.equal), *outputs, ';'.join(self.trace), str(self.instructions)))


def main():
    """제품 작성 전에 최초 자료를 고정하고 이후에는 원문/도구/fixture SHA만 확인한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true');args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for relative, expected in report['sha256'].items():
            if digest(ROOT/relative) != expected:raise RuntimeError('SP writer 최초 SHA 차이: '+relative)
        print(json.dumps({'verified': True, 'observations': report['observations']}));return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('SP writer 최초 자료 덮어쓰기 금지')
    for relative in ('cpppj/src/o/RawFortSpWrite.h', 'cpppj/src/o/RawFortSpWrite.cpp', 'cpppj/tests/FortSpWriteTests.cpp'):
        if (ROOT/relative).exists():raise RuntimeError('SP writer 제품 전 관측 요건 위반')
    inputs = cases();oracle = Oracle();statuses = Counter();instructions = equal = 0
    rows = ['# control / bits,seed,pos,end,capacity,pattern,bufferMode,mutation / status / convertedLow32 / randomState / randomCalls / assertions / equalDraws / bufferA256 / bufferB256 / sameStream256 / trace / instructions']
    for case in inputs:
        pair = []
        for control in (0x27f, 0x37f):
            if getattr(oracle, 'status', 'normal') != 'normal':oracle = Oracle()
            line = oracle.sample(case, control);pair.append(line);statuses[oracle.status] += 1;instructions += oracle.instructions;equal += oracle.equal
        if pair[0].split('\t', 1)[1] != pair[1].split('\t', 1)[1]:raise RuntimeError('SP writer 두 정밀도 차이')
        rows.extend(pair)
    FIXTURE.write_text('\n'.join(rows)+'\n', encoding='utf-8', newline='\n')
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv',
             'tools/decomp_priestmenu.py', 'tools/decomp_fort_sp_write.py', FIXTURE.relative_to(ROOT).as_posix())
    report = {'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-23', 'entry': '004be960', 'bodyBytes': 267,
              'ranges': RANGES, 'cdSourceMapAddress': None, 'cdExecuted': False, 'uniqueInputs': len(inputs), 'observations': len(rows)-1,
              'statuses': dict(statuses), 'instructions': instructions, 'precisionDifferences': 0, 'equalDraws': equal,
              'productAbsentBeforeObservation': True, 'scope': 'SP writer267BYTE/실제_ftol117BYTE/실제GameRandom40BYTE. 진단line36만명시장치. 전체3×256BYTE/같은stream16BYTEalias/현재RNG상태/전체512회순서/부분중단을보존. 게임·GUI·OS없음.',
              'sha256': {relative: digest(ROOT/relative) for relative in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('uniqueInputs', 'observations', 'statuses', 'instructions', 'equalDraws')}))


if __name__ == '__main__':main()
