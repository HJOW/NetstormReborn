#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""00435de0의 실제 감지 몸체를 실행한다. ConfigReadString/41ab10은 명시 입력이고 영상/창을 열지 않는다(699)."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_oracle import Oracle as BaseOracle, ROOT
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EIP

AVAILABLE = 0x54db98  # 원본 감지기가 먼저0, 성공이면1을 쓰는 DWORD다.
PATHS = (b'',b'\0hidden',b'intro.smk',b'C:\\CD\\intro.smk',b' movie.smk',b'x'*255)  # 빈/첫NUL/보통/장치/공백/지역 버퍼 끝 경계 입력이다.
SOURCES = (0,1,2,3,4,5,0xffffffff)  # 존재 helper가 반환할 수 있는 출처와 비영 경계다.


class Oracle(BaseOracle):
    """실제 기계어 분기/전역 쓰기를 실행하고 선택한 두 파일 장치만 명시한다."""
    def __init__(self):
        """같은 PC의 정밀 함수 경계와 보호 PE를 읽는다."""
        super().__init__('originals',[dict(name='detect',originals='00435de0')],helpers=[])
        self.path = b''  # 이번 명시 Config 결과 바이트다.
        self.source = 0  # 이번 존재 helper의 명시 반환이다.
        self.probes = 0  # 실제 원문이 호출한 존재 조회 횟수다.

    def on_instruction(self,mu,address,size,data):
        """두 helper의 스택/결과만 공급하고 나머지는 보호 함수 몸체에 맡긴다."""
        if address not in (0x441300,0x41ab10):
            return super().on_instruction(mu,address,size,data)
        esp=mu.reg_read(UC_X86_REG_ESP)
        ret,first,second=struct.unpack('<III',mu.mem_read(esp,12))
        assert struct.unpack('<I',mu.mem_read(AVAILABLE,4))[0] == 0
        if address == 0x441300:
            assert self.read_string(first)==b'introMovie'
            mu.mem_write(second,self.path+b'\0')
            result=1
        else:
            assert self.read_string(first)==self.path.split(b'\0')[0]
            self.probes+=1
            result=self.source
        mu.reg_write(UC_X86_REG_EAX,result)
        mu.reg_write(UC_X86_REG_ESP,esp+4)
        mu.reg_write(UC_X86_REG_EIP,ret)


def main():
    """명시 입력84개와 실제 반환/전역/조회 횟수를 새로운 fixture에만 쓴다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    assert not args.fixture.exists() and not args.report.exists(), '이전 원시/기대값은 보존한다'
    oracle=Oracle()
    rows=['# pathHex source initial returned available probes']
    for path,source,initial in itertools.product(PATHS,SOURCES,(0,0xffffffff)):  # 기대 분기는 만들지 않고 실제 보호 본체만 호출한다.
        oracle.path,oracle.source,oracle.probes=path,source,0
        oracle.mu.mem_write(AVAILABLE,struct.pack('<I',initial))
        result=oracle.call('detect',[])
        value,=struct.unpack('<I',oracle.mu.mem_read(AVAILABLE,4))
        rows.append(f'{path.hex() or "-"}\t{source}\t{initial}\t{result}\t{value}\t{oracle.probes}')
    args.fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    record=dict(unit=699,date='2026-11-02',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),inputs=len(rows)-1,
                nativeBodies=['00435de0'],explicitBoundaries=['00441300 ConfigReadString','0041ab10 file presence'],gameExecutions=0,
                peSha256=oracle.sha256,fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    args.report.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(record,ensure_ascii=False))


if __name__=='__main__':
    main()
