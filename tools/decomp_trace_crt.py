#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본에 정적으로 들어 있는 sprintf/실수 변환을 OS 없이 직접 실행한다."""
import csv
from decomp_process_summary import SummaryOracle
from decomp_bridgedecay_oracle import DecayOracle, ROOT, HEAP

# 원문 sprintf의 입력/출력/문자열을 분리한 에뮬레이터 전용 메모리다.
FORMAT, OUTPUT, STRINGS = HEAP+0x1000, HEAP+0x2000, HEAP+0x4000


class TraceCrtOracle(SummaryOracle):
    """시작 시의 실제 CRT 부동소수 dispatch 설치만 실행하고 sprintf 전체 몸체를 보존한다."""
    def __init__(self):
        super().__init__()
        self.executed = set()  # 실제 관측에 사용한 모든 원문 명령 주소다.
        self.crt_writes = set()  # _cinit 안의 dispatch 설치가 바꾼 실제 여섯 DWORD다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream, delimiter='\t'):  # 정밀 디컴파일의 실제 몸체만 실행한다.
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed[low-self.base:high+1-self.base] = b'\x01'*(high+1-low)
        self.spec = dict(self.spec, entries=dict(self.spec['entries'], Crt=0x4e58e9, FloatDispatch=0x4e3de3))
        self.call('FloatDispatch')  # 실제__fpmath의 첫 몸체다. x87 제어값은 각 독립 호출에서 설치한다.

    def on_instruction(self, mu, address, size, data):
        """CRT 장치를 대체하지 않고 실제 실행 주소만 수집한다."""
        self.executed.add(address)
        DecayOracle.on_instruction(self, mu, address, size, data)

    def on_write(self, mu, access, address, size, value, data):
        """실제 dispatch 설치의 여섯 DWORD 외의 전역 쓰기는 기존 보호 규칙으로 거부한다."""
        if 0x543b2c <= address <= 0x543b40 and size == 4:
            self.crt_writes.add(address)
            return
        super().on_write(mu, access, address, size, value, data)

    def format(self, fmt, arguments, control=0x27f):
        """명시 C문자열/정수/double 바이트만 같은 cdecl 스택에 옮겨 원문 CRT를 실행한다."""
        self.mu.mem_write(FORMAT, fmt+b'\0')
        words = [OUTPUT, FORMAT]
        next_string = STRINGS
        for kind, value in arguments:  # 숫자값은 계산하지 않고 원문 varargs 비트를 그대로 옮긴다.
            if kind == 's':
                self.mu.mem_write(next_string, value+b'\0')
                words.append(next_string)
                next_string += len(value)+1
            else:
                words.extend(value)
        size = self.call('Crt', words, control=control)
        assert size < 4096
        return self.text(OUTPUT)
