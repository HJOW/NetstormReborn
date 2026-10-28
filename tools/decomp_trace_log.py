#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 로그의 필터·마크업/들여쓰기·개행과 현재 공유 상태를 독립 관측한다."""
import hashlib
import json
import socket
from collections import deque
from decomp_trace_crt import TraceCrtOracle
from decomp_bridgedecay_oracle import DecayOracle, ROOT, HEAP, UC_X86_REG_ESP

# 실제 현재 로그 전역 주소다. 별도 엔진 입력과 원문CRT 출력 버퍼를 혼동하지 않는다.
BUFFER, STREAM, INDENT, LIMIT, MIRROR, FLUSH = 0x5ca4d0, 0x5ca8d0, 0x5ca8d4, 0x542658, 0x5ca8dc, 0x5ca8d8
# 실제 CRT FILE상수와 관측기 전용 입력 메모리다.
STDOUT, STDERR, FORMAT, BODY = 0x543770, 0x543790, HEAP+0x8000, HEAP+0x9000
# 원시 기록과 새 고정 fixture 경로다.
WORK = ROOT/'extracted/cpp-trace-log-557-20261029'
FIXTURE = ROOT/'cpppj/tests/fixtures/trace-log-x86.tsv'


class LogOracle(TraceCrtOracle):
    """로그/게임formatter/CRT는 실제 코드로 실행하고 세 출력 장치만 기록한다."""
    def __init__(self):
        super().__init__()
        self.last = deque(maxlen=12) # 예상 밖 외부 경계의 직전 원문 위치를 보존한다.
        self.number(HEAP+0xa000+0x64, self.number(0x543efc)) # 기본 C locale을 가리키는 CRT thread context 입력이다.
        self.spec = dict(self.spec, entries=dict(self.spec['entries'], Log=0x4c2630))

    def on_write(self, mu, access, address, size, value, data):
        """로그1024BYTE버퍼와실제상태/formatter전역의쓰기를허용한다."""
        if BUFFER <= address and address+size <= BUFFER+1024:
            return
        if address in (STREAM, INDENT, LIMIT, MIRROR, FLUSH) and size == 4:
            return
        if 0x5e1ab8 <= address and address+size <= 0x5e1abc: # 이관측기의접두제외format은항상%s/NUL의3BYTE다.
            return
        super().on_write(mu, access, address, size, value, data)

    def handle(self, stream):
        """PE의 두 고정 FILE주소만 안정된1/2별칭으로 바꾼다."""
        return {STDOUT: 1, STDERR: 2}.get(stream, stream)

    def on_instruction(self, mu, address, size, data):
        """장치진입인자와그시점의공유문자열을기록하고명시외부효과만적용한다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if hasattr(self, 'last'):
            self.last.append(f'{address:08x}')
        if address == 0x4ea61d:
            self.returned(HEAP+0xa000) # OS의TLS조회만대체하며isdigit/locale표의계산은원문을실행한다.
            return
        if address == 0x434f40:
            self.events.append('C:'+self.text(self.number(sp+4)).hex())
            if self.mutation == 1:
                self.number(STREAM, 2000)
            elif self.mutation == 2:
                self.number(MIRROR, 1)
                self.number(FLUSH, 1)
            elif self.mutation == 3:
                self.number(STREAM, 0)
                self.mu.mem_write(BUFFER, b'changed\n\0')
            elif self.mutation == 4:
                self.mu.mem_write(BUFFER, b'nested replacement\0')
            elif self.mutation == 8:
                self.number(LIMIT, 0)
                self.number(INDENT, 99)
            self.returned()
            return
        if address == 0x4e4c36:
            stream = self.number(sp+4)
            assert self.text(self.number(sp+8)) == b'%s'
            self.events.append(f'W:{self.handle(stream)}:'+self.text(self.number(sp+12)).hex())
            self.writes += 1
            if self.writes == 1 and self.mutation == 5:
                self.number(STREAM, 3000)
                self.number(FLUSH, 1)
                self.mu.mem_write(BUFFER, b'after mirror\0')
            elif self.writes == 1 and self.mutation == 6:
                self.number(FLUSH, 0)
            self.returned()
            return
        if address == 0x4e4bdd:
            self.events.append(f'F:{self.handle(self.number(sp+4))}')
            if self.mutation == 7:
                self.number(STREAM, 0)
            self.returned()
            return
        super().on_instruction(mu, address, size, data)

    def sample(self, case, control):
        """명시 입력만 설치하고 로그 본체가 필터/문자열/현재 전역 결과를 계산한다."""
        level, limit, indent, stream, mirror, flush, prefix, text, self.mutation = case
        self.writes = 0
        self.mu.mem_write(BUFFER, b'old\0'+bytes([0xa5])*1020)
        for address, value in ((LIMIT, limit), (INDENT, indent), (STREAM, {1: STDOUT, 2: STDERR}.get(stream, stream)), (MIRROR, mirror), (FLUSH, flush)):
            self.number(address, value)
        self.mu.mem_write(FORMAT, text[:prefix]+b'%s\0')
        self.mu.mem_write(BODY, text[prefix:]+b'\0')
        try:
            self.call('Log', [level, FORMAT, BODY], control=control)
        except Exception:
            print(case, list(self.last), flush=True)
            raise
        final = [self.handle(self.number(STREAM)), self.number(LIMIT), self.number(INDENT), self.number(MIRROR), self.number(FLUSH)]
        return ';'.join(self.events)+'|' + ','.join(map(str, final))+'|'+bytes(self.mu.mem_read(BUFFER, 1024)).hex()


def cases():
    """필터경계·색상접두·양/음들여쓰기·개행·파일경계와콜백변경을입력한다."""
    result = []
    for level in (-2147483648, -1, 0, 29, 30, 2147483647):
        for limit in (-1, 0, 29, 30):
            result.append((level, limit, 0, 0, 0, 0, 0, b'hello\r\n', 0))
    for indent in (0, 1, 3, 4, 7, 16, 1000, 0x80000000, 0xffffffff):
        for prefix in (b'', b'~1', b'~1~y'):
            result.append((29, 29, indent, 1000, 0, 0, len(prefix), prefix+b'body\n', 0))
    for text in (b'', b'\n', b'\r\n', b'\r\r\n', b'\n\rx\r\n', b'a\0ignored', b'~1~ypayload', b'%foo', b'%%', b'x~y~n\n'):
        result.append((29, 29, 3, 1000, 0, 0, 0, text, 0))
    for stream in (0, 1000, 2):
        for mirror in (0, 1):
            for flush in (0, 1):
                result.append((29, 29, 3, stream, mirror, flush, 4, b'~1~ybody\n', 0))
    for mutation in range(1, 9):
        result.append((29, 29, 3, 1000, 1, 1, 4, b'~1~ybody\n', mutation))
    return result


def main():
    """두정밀도에서실제출력/공유버퍼/전역이같은고유입력을고정한다."""
    oracle = LogOracle()
    inputs = cases()
    rows = ['# level limit indent stream mirror flush prefixBytes textHex mutation => 실제장치/최종전역/1024BYTE버퍼']
    for case in inputs:  # 같은독립입력에서원문코드를두번실행한다.
        a, b = (oracle.sample(case, control) for control in (0x27f, 0x37f))
        assert a == b
        rows.append('\t'.join(map(str, (*case[:7], case[7].hex() or '-', case[8], a))))
    FIXTURE.write_text('\n'.join(rows)+'\n', encoding='utf-8', newline='\n')
    report = dict(host=socket.gethostname(), lastFullDecompilation='HJOW-Athlon / 2026-10-23',
                  uniqueInputs=len(inputs), nativeObservations=len(inputs)*2,
                  fixtureSha256=hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),
                  binarySha256=oracle.sha256,
                  boundary='004c2630/004de490/원본CRT실제실행. 기본C locale의CRT thread context입력, 434f40/CRT fprintf/fflush는외부출력기록. 게임/OS0.')
    WORK.mkdir(parents=True, exist_ok=True)
    (WORK/'native.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False), flush=True)


if __name__ == '__main__':
    main()
