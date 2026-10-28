#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""객체덤프 부모·실제재귀/로그·후처리를 원본 기계어에서 독립 관측한다."""
import hashlib
import json
import socket
import struct
from decomp_trace_log import LogOracle, BUFFER, STREAM, LIMIT, STDOUT, STDERR
from decomp_bridgedecay_oracle import ROOT, POOL, TYPES, HEAP, UC_X86_REG_ESP, UC_X86_REG_EIP

# 실제 덤프 모드/슬롯 수/재귀 깊이와 격리된 파일/할당 장치 메모리다.
SUPPRESS, COUNT, DEPTH = 0x5c8454, 0x5c847c, 0x55a4a0
PATH, INPUT, ALLOCATION = HEAP+0xb000, HEAP+0xc000, HEAP+0xe000
# 이 단위의 원시 기록과 독립 기대값이다.
WORK = ROOT/'extracted/cpp-object-dump-558-20261029'
FIXTURE = ROOT/'cpppj/tests/fixtures/object-dump-x86.tsv'


class DumpOracle(LogOracle):
    """파일/경로/할당만 명시 장치로 바꾸고 덤프·재귀·로그·CRT는 원문을 실행한다."""
    def __init__(self):
        super().__init__()
        self.spec = dict(self.spec, entries=dict(self.spec['entries'], Dump=0x4d6020))
        for address, size in ((0x4ad000, 9), (0x4ad010, 5)): # Ghidra가별도함수로분리하지않은실제짧은두VT몸체다.
            self.allowed[address-self.base:address+size-self.base] = b'\x01'*size
        self.read_special = None # 전체읽기장치의명시독립반환이며None이면실제wt출력바이트를읽는다.

    def on_write(self, mu, access, address, size, value, data):
        """실제덤프모드/재귀깊이와관측입력의표준trace format scratch만더허용한다."""
        if address in (SUPPRESS, DEPTH) and size == 4:
            return
        if 0x5e1ab8 <= address and address+size <= 0x5e1bb8:
            return # 이관측기의표준format최대길이는256BYTE보다작다.
        super().on_write(mu, access, address, size, value, data)

    def state(self):
        """장치콜백이보는현재level/덤프모드/FILE을동일한별칭으로기록한다."""
        return f'{self.number(LIMIT)}:{self.number(SUPPRESS)}:{self.handle(self.number(STREAM))}'

    def done(self, value=0, purge=0):
        """현재장치의원문cdecl/thiscall반환폭만처리한다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        self.returned(value)
        self.mu.reg_write(UC_X86_REG_ESP, sp+4+purge)

    def changed(self):
        """첫일반trace완료시점의외부로그효과만현재슬롯/끝번호에적용한다."""
        if self.mutation == 1:
            self.number(COUNT, 5)
        elif self.mutation == 2:
            self.number(COUNT, 25)
        elif self.mutation == 3:
            self.number(POOL+7*50+11, 1, 1)
        elif self.mutation == 4:
            self.number(POOL+7*50+10, 0, 1)

    def on_instruction(self, mu, address, size, data):
        """실제파일장치의순서/인자/물리바이트와마지막모드복구를관측한다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x459c60:
            assert self.number(sp+4) == 0xfffffffe and self.number(sp+12) == 0xfffffffd
            assert self.text(self.number(sp+16)) == b'squids.txt' and self.number(sp+20) == 0
            self.events.append('P:'+self.state())
            self.done(PATH)
            return
        if address == 0x4e545a:
            self.events.append('O:'+self.text(self.number(sp+4)).hex()+':'+self.text(self.number(sp+8)).hex()+':'+self.state())
            self.done(self.open_result)
            return
        if address == 0x4e0620:
            self.events.append('A:'+self.text(self.number(sp+4)).hex()+':'+self.state())
            if self.mutation == 5:
                self.number(STREAM, 1000)
            self.done()
            return
        if address == 0x434f40:
            # 외부console의실제계산은이부모범위밖이다. 첫번째일반재귀뒤의구분개행에서만입력을바꾼다.
            value = self.text(self.number(sp+4))
            self.events.append('C:'+value.hex())
            if value == b'\n' and not self.did_change:
                self.did_change = True
                self.changed()
            self.done()
            return
        if address == 0x4e4c36:
            stream = self.number(sp+4)
            assert self.text(self.number(sp+8)) == b'%s'
            value = self.text(self.number(sp+12))
            self.events.append(f'W:{self.handle(stream)}:'+value.hex())
            if stream == 1000:
                self.contents += value.replace(b'\n', b'\r\n') # 원본wtFILE의물리CRLF쓰기경계다.
            self.done()
            return
        if address == 0x4e54b9:
            self.events.append(f'X:{self.handle(self.number(sp+4))}:'+self.state())
            self.done()
            return
        if address == 0x41baa0:
            self.events.append('R:'+self.text(self.number(sp+4)).hex()+':'+str(self.number(sp+8))+':'+str(self.number(sp+16))+':'+self.state())
            value = self.contents if self.read_special is None else self.read_special
            self.mu.mem_write(INPUT, value+b'\0')
            self.number(self.number(sp+12), len(value))
            self.done(INPUT)
            return
        if address == 0x4e4391:
            self.events.append('N:'+str(self.number(sp+4))+':'+self.state())
            self.done(ALLOCATION)
            return
        if address == 0x41a5a0:
            self.events.append('B:'+self.state())
            self.done()
            return
        if address == 0x41b4c0:
            self.events.append('Q:'+self.text(self.number(sp+4)).hex()+':'+str(self.number(sp+8))+':'+str(self.number(sp+12))+':'+self.state())
            self.done(purge=12)
            return
        if address == 0x41a910:
            length = self.number(sp+8)
            value = bytes(self.mu.mem_read(self.number(sp+4), length))
            self.events.append('D:'+value.hex()+':'+str(self.number(sp+12))+':'+self.state())
            self.final_file = value
            self.done(length, 12)
            return
        if address == 0x41b410:
            self.events.append('Z:'+self.state())
            self.done()
            return
        super().on_instruction(mu, address, size, data)

    def sample(self, case, control):
        """독립물리슬롯/파일반환/외부변형만설치하고기대출력은계산하지않는다."""
        count, level, suppress, self.open_result, self.mutation, self.read_special, nodes = case
        self.contents = b'' # 실제CRT wt가작성한물리바이트다.
        self.final_file = b'' # 마크업제거뒤BaseFile이쓴최종바이트다.
        self.did_change = False # 첫일반재귀뒤구분개행의외부효과만한번적용한다.
        self.mu.mem_write(POOL, bytes(24000*50))
        for sid in range(24000): # 물리풀의미사용슬롯을명시free로둔다.
            self.number(POOL+sid*50+11, 1, 1)
        self.mu.mem_write(TYPES, bytes(188*500))
        for kind in range(188):
            self.mu.mem_write(TYPES+kind*500+4, f'kind{kind}'.encode()+b'\0')
        self.number(TYPES+82*500+0xe8, 0x810)
        for sid, raw in nodes:
            self.mu.mem_write(POOL+sid*50, raw)
        self.mu.mem_write(PATH, b'out/squids.txt\0')
        for address, value in ((COUNT, count), (LIMIT, level), (SUPPRESS, suppress), (STREAM, 777), (DEPTH, 0)):
            self.number(address, value)
        self.call('Dump', control=control)
        return ';'.join(self.events)+'|'+self.state()+'|'+self.final_file.hex()


def node(sid, kind=82, state=0, x=12.75, y=-3.75, head=0, next_sid=0):
    """독립입력50BYTE를구성하고실제Base/Process VT를지정한다."""
    raw = bytearray(50)
    struct.pack_into('<IHH', raw, 0, 0x512098 if kind == 46 else 0x501dd0, next_sid, head)
    raw[10:12] = bytes((kind, state))
    struct.pack_into('<ff', raw, 14, x, y)
    raw[40] = 128
    return sid, bytes(raw)


def cases():
    """공통/특수/contained/free/번호경계와파일실패·후처리입력을만든다."""
    result = [(20, 29, 0, 1000, 0, None, [])]
    for kind in (0, 82, 97, 102, 121):
        for state in (0, 1, 2, 4, 8):
            result.append((20, 7, 9, 1000, 0, None, [node(1), node(2, kind, state), node(19, kind, state), node(20)]))
    nodes = [node(2), node(7), node(19)]
    for count in (0, 1, 2, 3, 8, 20):
        result.append((count, 29, 0, 1000, 0, None, nodes))
    for mutation in range(1, 5):
        result.append((20, 29, 0, 1000, mutation, None, nodes))
    result.append((20, 29, 0, 1000, 2, None, nodes+[node(20), node(21, state=1)]))
    result.extend([(20, 7, 9, 0, 0, None, nodes), (20, 7, 9, 0, 5, None, nodes)])
    result.append((20, 29, 0, 1000, 0, None, [node(2, head=3), node(3, 46, 8)]))
    result.append((20, 29, 0, 1000, 0, None, [node(2, head=3), node(3, 46, 8, head=4), node(4, 46, 8)]))
    for kind in (97, 102, 121):
        for x, y in ((-0.75, 0.75), (2147483648.0, -2147483648.0), (1e30, -1e30)):
            result.append((20, 29, 0, 1000, 0, None, [node(2, kind, x=x, y=y)]))
    for data in (b'', b'abc', b'~yA~n\r\n', b'A~~B~', b'~', b'A\0B~yC'):
        result.append((20, 29, 0, 1000, 0, data, []))
    return result


def main():
    """두정밀도에서원문전체파일/로그순서/최종전역이같은입력을고정한다."""
    oracle = DumpOracle()
    inputs = cases()
    rows = ['# count level suppress open mutation readOverrideHex nodes => 실제장치/로그순서와최종전역/파일']
    for index, case in enumerate(inputs):
        a, b = (oracle.sample(case, control) for control in (0x27f, 0x37f))
        assert a == b
        read = '-' if case[5] is None else ':'+case[5].hex()
        nodes = '/'.join(f'{sid}:{raw.hex()}' for sid, raw in case[6]) or '-'
        rows.append('\t'.join(map(str, (*case[:5], read, nodes, a))))
    FIXTURE.write_text('\n'.join(rows)+'\n', encoding='utf-8', newline='\n')
    report = dict(host=socket.gethostname(), lastFullDecompilation='HJOW-Athlon / 2026-10-23',
                  uniqueInputs=len(inputs), nativeObservations=len(inputs)*2, binarySha256=oracle.sha256,
                  fixtureSha256=hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),
                  catchType=oracle.text(0x52f460+8).decode('ascii'), catchEntry='004d62a8', catchResume='004d6275',
                  catchRegion=[1, 1], fileUnwind='004fc160',
                  boundary='004d6020/전체trace/로그/CRT실제실행; 기본C locale thread입력. 경로/파일/할당/console는장치기록. EH는정적metadata대조이며실제OS예외실행0.')
    WORK.mkdir(parents=True, exist_ok=True)
    (WORK/'native.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False), flush=True)


if __name__ == '__main__':
    main()
