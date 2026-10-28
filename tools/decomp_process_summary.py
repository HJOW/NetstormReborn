#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 프로세스 요약의39999슬롯·현재 필드·전체 이름·서식 인자를 독립 실행한다."""
import csv
import hashlib
import json
import socket
import struct
from decomp_bridgedecay_oracle import (DecayOracle, ROOT, POOL, TYPES, HEAP,
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP)

# 원문 함수와 데이터 표는 실제PE/완료된 정밀 디컴파일에서 읽는다.
ENTRIES=(0x471890,0x41bff0,0x49a830,0x49a840)
TABLE=0x56b808  # 슬롯0은 방문하지 않는 실제40000개 포인터 표다.
WORK=ROOT/'extracted/cpp-process-summary-553-20261029'
FIXTURE=ROOT/'cpppj/tests/fixtures/process-summary-x86.tsv'


class SummaryOracle(DecayOracle):
    """외부 CRT파일 세 함수만 대체하며 순회·필드/이름 조회는 원문을 실행한다."""
    def __init__(self):
        super().__init__('originals')
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            rows={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        self.bodies=[]  # 함수별 불연속 원문 바이트를 실행 전후 확인한다.
        for entry in ENTRIES:  # 필요한 네 몸체만 기존 허용 범위에 더한다.
            for part in rows[entry]['ranges'].split(';'):
                low,high=(int(v,16) for v in part.split('-'))
                self.allowed[low-self.base:high+1-self.base]=b'\x01'*(high+1-low)
                self.bodies.append((low,bytes(self.mu.mem_read(low,high+1-low))))
        self.spec=dict(self.spec,entries=dict(self.spec['entries'],Summary=0x471890))

    def number(self,address,value=None,width=4):
        """독립 입력/명시 외부 효과의 little endian 정수만 읽고 쓴다."""
        if value is None:return int.from_bytes(self.mu.mem_read(address,width),'little')
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def text(self,address):
        """원문 %s가 읽는 첫NUL까지의 바이트를 변환 없이 읽는다."""
        result=bytearray()
        while (value:=self.number(address+len(result),width=1))!=0:result.append(value)  # 입력 문자열의 끝만 찾는다.
        return bytes(result)

    def pointer(self,handle):
        """합성 프로세스는 원본+4/+8의 물리 필드만 사용한다."""
        return HEAP+0x100+handle*0x20 if handle else 0

    def returned(self,value=0):
        """CRT의cdecl 반환만 수행하며 호출자의 스택 정리는 원문에 맡긴다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);target=self.number(sp)
        self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_ESP,sp+4);self.mu.reg_write(UC_X86_REG_EIP,target)

    def mutate(self):
        """첫 fprintf의 명시 외부 효과가 다음 실제 슬롯/필드에 반영되는지 검사한다."""
        mode=self.mutation
        if mode==1:self.number(TABLE+2*4,0)
        elif mode==2:self.number(TABLE+100*4,self.pointer(4))
        elif mode==3:self.number(self.pointer(2)+8,0)
        elif mode==4:self.number(POOL+51*50+10,54,1)
        elif mode==5:self.number(POOL+61*50+10,157,1)
        elif mode==6:self.number(TABLE+2*4,self.pointer(3))
        elif mode==7:self.number(TABLE+1*4,self.pointer(4))
        elif mode==8:self.number(POOL+50*50+10,62,1)
        elif mode==13:self.number(TABLE+39999*4,0)
        elif mode==14:self.number(TABLE+3*4,self.pointer(4))

    def on_instruction(self,mu,address,size,data):
        """파일 경계의 실제 원문 인자를 기록하고 선택한 입력 시점에만 메모리를 바꾼다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if address==0x4e545a:
            path,mode=self.text(self.number(sp+4)),self.text(self.number(sp+8))
            self.events.append(f'O:{path.hex()}:{mode.hex()}:{self.file}');self.returned(self.file);return
        if address==0x4e4c36:
            stream,fmt=self.number(sp+4),self.text(self.number(sp+8));name=self.text(self.number(sp+12))
            if fmt==b'pType=%s sid=0, attachToType=NULL\n':line=fmt%name
            else:
                assert fmt==b'pType=%s sid=%d, attachToType=%s\n'
                parent=self.number(sp+16);parent=parent if parent<0x80000000 else parent-0x100000000
                line=fmt%(name,parent,self.text(self.number(sp+20)))
            self.events.append(f'W:{stream}:{line.hex()}');self.lines+=1
            if self.lines==1:self.mutate()
            self.returned(len(line));return
        if address==0x4e54b9:
            self.events.append(f'C:{self.number(sp+4)}');self.returned();return
        if address==0x49a830:
            self.name_calls+=1
            if self.mutation==9 and self.name_calls==1:self.number(TABLE+4,self.pointer(2))
        if address==0x4718cf and not self.parent_mutated and self.mutation in (10,11):
            self.parent_mutated=True
            if self.mutation==10:self.number(mu.reg_read(UC_X86_REG_EAX)+8,61)
            else:self.number(TABLE+4,self.pointer(2))
        super().on_instruction(mu,address,size,data)

    def sample(self,scenario,mutation,file,control):
        """순회 결과를 계산하지 않고 독립 슬롯/프로세스/현재TYPE 입력을 준비한다."""
        self.file=file;self.mutation=mutation;self.lines=0;self.name_calls=0;self.parent_mutated=False  # CRT반환토큰/변형번호·현재출력/이름조회횟수·부모변형완료여부다.
        self.mu.mem_write(TABLE,bytes(40001*4));self.mu.mem_write(POOL,bytes(24000*50));self.mu.mem_write(TYPES,bytes(188*500))
        for kind,name in ((82,b'bridge'),(94,b'island'),(157,b'aLongParentNameWith%and\tText'),(0,b'none\0ignored')):  # TYPE+4는20글자 제한 없이 C 문자열로 읽힌다.
            self.mu.mem_write(TYPES+kind*500+4,name+b'\0')
        for sid,kind in ((60,82),(61,94),(62,157),(63,0)):self.number(POOL+sid*50+10,kind,1)  # 부모의 free/dead 상태는 필터가 아니다.
        for handle,kind,parent in ((1,46,60),(2,10,61),(3,55,0),(4,54,62),(5,0,63)):  # 기본 다섯 프로세스의 Form/부모다.
            self.number(self.pointer(handle)+4,49+handle);self.number(self.pointer(handle)+8,parent);self.number(POOL+(49+handle)*50+10,kind,1)
        slots={0:5,40000:5}
        if scenario==1:slots[1]=1;self.number(self.pointer(1)+8,0);self.number(POOL+50*50+11,1,1)
        elif scenario==2:slots.update({1:1,2:2,3:3,39999:5})
        elif scenario==3:
            for kind in range(63):  # 전체63개 원래 포인터 이름을 한 번씩 실제 조회한다.
                handle=10+kind;sid=100+kind;slots[1+kind]=handle;self.number(self.pointer(handle)+4,sid);self.number(self.pointer(handle)+8,(0,60,61,62,63)[kind%5]);self.number(POOL+sid*50+10,kind,1);self.number(POOL+sid*50+11,kind%16,1)
        elif scenario==4:slots.update({1:3,39998:1,39999:2})
        for index,handle in slots.items():self.number(TABLE+index*4,self.pointer(handle))  # 머리/끝 밖의 두 sentinel도 명시 입력이다.
        self.call('Summary',control=control)
        assert all(bytes(self.mu.mem_read(address,len(body)))==body for address,body in self.bodies)
        return ';'.join(self.events)


def main():
    """기본/전체이름/경계와 현재 메모리 변형을 두 정밀도에서 먼저 동결한다."""
    oracle=SummaryOracle();WORK.mkdir(parents=True,exist_ok=True)
    cases=[(scenario,0,700) for scenario in range(5)]+[(0,0,0),(2,0,0)]+[(2,mutation,700) for mutation in (*range(1,12),13,14)]
    rows=['# scenario\tmutation\tfile => 실제 CRT 호출/바이트 기록'];differences=[]
    for case in cases:  # 같은 입력을 독립53/64비트 상태에서 다시 준비한다.
        a,b=(oracle.sample(*case,control) for control in (0x27f,0x37f))
        if a!=b:differences.append(case)
        rows.append('\t'.join(map(str,(*case,a))))
    assert not differences
    FIXTURE.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    report=dict(host=socket.gethostname(),lastFullDecompilation='HJOW-Athlon / 2026-10-23',uniqueInputs=len(cases),nativeObservations=len(cases)*2,precisionDifferences=differences,binarySha256=oracle.sha256,fixtureSha256=hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),boundary='00471890/0041bff0/0049a830/0049a840 원문 실행; fopen/fprintf/fclose만 명시 CRT경계. 실제 OS파일/게임 실행 없음.')
    (WORK/'native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False),flush=True)


if __name__=='__main__':main()
