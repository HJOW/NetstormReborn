#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Session의 실패 입력만 원본 Pop에 주고 전체 raw/점유표 사후 상태를 독립 대조한다."""
import argparse
import hashlib
import json
from pathlib import Path
import socket
import struct
import zlib
from decomp_oracle import Oracle as BaseOracle,ROOT,STACK
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_FPCW,UC_X86_REG_FPSW

POOL,TYPES,SPOTS=0x12000000,0x13000000,0x14000000  # 다른 원문/호스트 주소와 분리된 실제 폭의 입력 장치다.
HELPERS=(0x4210a0,0x49a840,0x4e49c0,0x40e800,0x4afd30,0x49a900)  # 좌표·타입·정수화·기본 점/분류의 순수 실제 몸체다.


class Oracle(BaseOracle):
    """실제 Pop의 조기 종료까지 실행하며 대체 함수/정답 계산을 넣지 않는다."""
    def __init__(self):
        """완료된 정밀 디컴파일 범위와 고정 물리 입력만 준비한다."""
        super().__init__('originals',[dict(name='Pop',originals='004b02d0')],list(HELPERS))
        self.mu.mem_map(POOL,0x1000)
        self.mu.mem_map(TYPES,0x20000)
        self.mu.mem_map(SPOTS,0x10000)
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def put(self,address,value):
        """원문 입력의 little endian DWORD만 설치한다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))

    def write(self,mu,access,address,size,value,data):
        """실제 부모는 이 객체/점유표/스택 밖의 상태를 바꿀 수 없다."""
        if 0x30000000<=address<=address+size<=0x30010000 or POOL+50<=address<=address+size<=POOL+100 or SPOTS<=address<=address+size<=SPOTS+65536:
            return
        raise RuntimeError(f'Pop89 미명시 쓰기 {address:08x}+{size}')

    def on_instruction(self,mu,address,size,data):
        """허용한 실제 하위 호출만 기록하고 진단/기타 효과는 즉시 거부한다."""
        if address==0x4e0620:
            raise RuntimeError('Pop89 원문 진단 경로')
        if address in (0x4b02d0,*HELPERS):
            self.calls.append(f'{address:08x}')
        super().on_instruction(mu,address,size,data)

    def observe(self,row,control):
        """C++ snapshot은 입력으로만 쓰고 원문이 만든 전체 출력 바이트를 읽는다."""
        self.calls=[]  # 이번 실제 원문 호출의 순서다.
        raw=bytes.fromhex(row[8])
        assert len(raw)==50 and raw[10]==89
        self.mu.mem_write(POOL,bytes(150))
        self.mu.mem_write(POOL+50,raw)
        self.mu.mem_write(TYPES,bytes(0x20000))
        typ=TYPES+89*500
        for offset,value in ((0xe8,int(row[2])),(0xec,int(row[3])),(0x1d4,int(row[4])),(0x1d8,int(row[5]))):
            self.put(typ+offset,value)  # 실제 읽는 TYPE 필드만 같은 원시 값으로 설치한다.
        self.mu.mem_write(SPOTS,bytes.fromhex(row[9]))
        for address,value in ((0x5c8464,POOL),(0x5c847c,3),(0x59ab20,TYPES),(0x5c7c44,SPOTS),(0x531928,256),(0x594fc8,1),(0x59a8b0,1)):
            self.put(address,value)  # 같은 물리 입력/비전투 로그·표시 억제 조건이다.
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.call('Pop',[struct.unpack('<I',struct.pack('<f',112))[0],struct.unpack('<I',struct.pack('<f',118))[0],0],POOL+50,12)
        return bytes(self.mu.mem_read(POOL+50,50)),bytes(self.mu.mem_read(SPOTS,65536)),self.calls


def main():
    """기존 snapshot을 원문 기대값으로 사용하지 않고 별도 PE 출력과만 비교한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    out=args.output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    lines=args.input.read_text('utf-8').splitlines()
    row=next(line.split('\t') for line in lines if line.startswith('POP89_INPUT\t'))
    observed=next(line.split('\t') for line in lines if line.startswith('POP89_OUTPUT\t'))
    oracle=Oracle()
    results=[oracle.observe(row,control) for control in (0x27f,0x37f,0x27f)]
    assert results[0]==results[1]==results[2]
    raw,spots,calls=results[0]
    assert raw.hex()==observed[2] and spots.hex()==observed[3],'원문/C++ 전체 출력 차이'
    before=bytes.fromhex(row[9])
    changes=[dict(index=index,x=index%256,y=index//256,before=a,after=b) for index,(a,b) in enumerate(zip(before,spots)) if a!=b]
    fixture=out/'pop89-context-x86.tsv'
    schema=('rawBefore','rawAfter','spotsBeforeAdler','spotsAfterAdler','changes','flags1','flags2','footX','footY','widthBits','heightBits')
    values=(row[8],raw.hex(),zlib.adler32(before),zlib.adler32(spots),';'.join(f'{item["index"]}:{item["before"]}:{item["after"]}' for item in changes),*row[2:8])
    fixture.write_text('# '+'\t'.join(schema)+'\n'+'\t'.join(map(str,values))+'\n',encoding='utf-8',newline='\n')
    report=dict(unit=691,date='2026-11-01',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),case='TheWarBegins LoadLocalConnection/Create89 flags0/owner1/Pop(112,118,0)',inputRole='C++가제공한물리입력만사용. 기대출력은보호PE가생성.',observations=3,precisionOrRepeatDifferences=0,cppOutputDifferences=0,cppResult=int(observed[1]),typeName=row[1],flags1=int(row[2]),flags2=int(row[3]),foot=[int(row[4]),int(row[5])],nativeRawBefore=row[8],nativeRawAfter=raw.hex(),spotsBeforeSha256=hashlib.sha256(before).hexdigest(),spotsAfterSha256=hashlib.sha256(spots).hexdigest(),changedSpots=changes,nativeCalls=calls,fixtures={fixture.name:hashlib.sha256(fixture.read_bytes()).hexdigest()},boundaries='실제4b02d0/4210a0/49a840/ftol/40e800/4afd30/49a900을대체없이실행했다. 같은TYPE4필드와전체점유표/50BYTE가입력이다. 진단/미허용하위/외부상태쓰기는거부한다. C++세계와원본게임의자연생성동일성증거가아니며공격/승리/GUI/통신/MP는0이다.',sha256={str(path):hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in (Path('originals/Netstorm.exe'),Path('extracted/refined/originals/functions.tsv'),Path('tools/decomp_pop89_context.py'),Path('tools/decomp_oracle.py'),args.input.resolve().relative_to(ROOT))})
    (out/'native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(observations=3,cppOutputDifferences=0,cppResult=int(observed[1]),changedSpots=changes)))


if __name__=='__main__':
    main()
