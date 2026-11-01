#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문 기술 후보/소유 필터와 원소별 HTML 목록을 실제 x86에서 격리 실행한다(705)."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_oracle import Oracle as BaseOracle, SCRATCH, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

POOL,TYPES,HEADER,ARRAY,VTABLE,LEVEL,DESCRIPTION=0x12000000,SCRATCH+0x6000,SCRATCH+0x100,SCRATCH+0x200,SCRATCH+0x4000,STOP+0x100,SCRATCH+0x3100  #모든 실제 슬롯/목록/타입과 가상 레벨/문자열은 격리 주소다.

def cases():
    """기대 분기를 계산하지 않고 필터·고정 용량과 UI 경계의 독립 입력을 만든다."""
    rows=[]  # op/theme/level/typeCount/ownership/outputCapacity/mutation의7DWORD다.
    for op,theme,level,count,owned in itertools.product((0,1),(-1,0,1,5,6),(-1,0,2,3),(0,16,80),(0,1)):  #전체 원소5는 level을 무시하며 잘못된 level 진단 뒤 계속하는 입력도 포함한다.
        rows.append((op,theme,level,count,owned,40,0))
    for theme,level,count,owned in itertools.product((0,1,3,5),(-1,0,1,2,3),(0,16,80),(0,1)):  #원소 목록은 실제 가상level-1과 고정40칸 넘침을 교차한다.
        rows.append((2,theme,level,count,owned,40,0))
    for op,capacity in itertools.product((0,1),(0,1,3,40)): rows.append((op,5,-1,80,1,capacity,0))  #NULL 목록과 작은 고정 용량의 전체 일치 수/저장 수를 구별한다.
    rows.append((2,5,2,80,1,40,1))  #설명 경계 뒤 level이 바뀌어도 앞서 push한 표시 레벨을 보존한다.
    return list(dict.fromkeys(rows))

class Oracle(BaseOracle):
    """세 원문 몸체를 실행하고 큰 게임 조회·번역/printf·메뉴·할당만 명시 경계로 둔다."""
    def __init__(self):
        """보호 PE와 같은 PC의 정밀 범위를 읽기 전용 적재한다."""
        super().__init__('originals',[dict(name='known',originals='004cb320'),dict(name='candidates',originals='004cb3f0'),dict(name='theme',originals='004cb4b0')],helpers=[]);self.mu.mem_map(0,0x1000);self.mu.mem_map(POOL,0x10000);self.row=[];self.trace=[];self.allocations=0
        self.put(0x5c8464,POOL);self.put(POOL+10*50,VTABLE);self.put(VTABLE+0x80,LEVEL);self.put(0x5412d0,5);self.put(0x54126c,9);assert self.get(0x542a48)==0x020000d5

    def put(self,address,value):
        """원문 하위32비트 DWORD를 저장한다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))

    def get(self,address):
        """비정렬 원문 DWORD를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def event(self,name,*args):
        """실제 실행된 경계의 순서와 인자를 기록한다."""
        self.trace.append(':'.join(map(str,(name,*args))))

    def on_instruction(self,mu,address,size,data):
        """선택한 외부 장치만 명시 공급하고 세 함수의 필터/목록/수명 제어는 그대로 실행한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);ret,a,b,c,d,e,f=struct.unpack('<7I',mu.mem_read(sp,28));this=mu.reg_read(UC_X86_REG_ECX);value=0;purge=0
        if address==LEVEL: self.event('L',10);value=(self.row[2]+1)&0xffffffff
        elif address==0x4e4391:
            assert a==0xa0;value=SCRATCH+0x1000+self.allocations*0x1000;self.allocations+=1
        elif address==0x4e4354: pass  #큰 할당/반납 경계이며 게임/OS heap을 실행하지 않는다.
        elif address==0x48fc60: self.event('T');value=50
        elif address==0x49a840: value=TYPES+a*500
        elif address==0x4acac0:
            assert a==4 and c==0;self.event('O',(this-POOL)//50,b);value=b if self.row[4] and b%3==1 else 0;purge=12
        elif address==0x4de9a0:
            assert self.read_string(a)==b'%s (Level %d)';self.event('X');value=a;purge=4
        elif address==0x49af80:
            kind=(this-TYPES)//500;self.event('D',kind);self.mu.mem_write(DESCRIPTION,f'T{kind}'.encode()+b'\0');value=DESCRIPTION
            if self.row[6]: self.put(this+0x94,7)
        elif address==0x4a89c0:
            assert self.read_string(b)==b'%s (Level %d)';desc=self.read_string(c).decode();self.event('F',int(desc[1:]),d);self.mu.mem_write(a,f'{desc} (Level {d})'.encode()+b'\0');value=a
        elif address==0x477cb0:
            assert d==1 and f==0;self.event('I',b,c,e)
        elif address==0x4e0620: self.event('A',c)
        else: return super().on_instruction(mu,address,size,data)
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,row):
        """독립 TYPE와 목록을 준비하고 실제 반환·저장·호출을 읽는다."""
        self.row,self.trace,self.allocations=row,[],0;self.put(0x541348,row[3]);self.put(HEADER,ARRAY);self.put(HEADER+4,row[5]);self.put(HEADER+8,0);self.mu.mem_write(ARRAY,bytes([0xa5])*160)
        for kind in range(80):  #독립 타입 번호이며 실제 공격형 공중 유닛을 만들거나 행동시키지 않는다.
            base=TYPES+kind*500;self.put(base+0x94,kind%4-1);self.put(base+0x98,kind%6);self.put(base+0x9c,10 if kind%11==0 else 0)
        result=self.call('theme' if row[0]==2 else 'known' if not row[0] else 'candidates',[10,row[1]] if row[0]==2 else [HEADER if row[5] else 0,row[1],row[2]])
        out=[0 if row[0]==2 else result,self.get(HEADER+8)]
        out.extend(struct.unpack('<40I',self.mu.mem_read(ARRAY,160)))
        return ','.join(map(str,row)),';'.join(self.trace) or '-',','.join(map(str,out))

def main():
    """기존 fixture를 덮지 않고 새 실제 관측/해시만 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();assert not args.fixture.exists() and not args.report.exists();oracle=Oracle();rows=['# input7 trace return_stored_array40']
    for row in cases(): rows.append('\t'.join(oracle.run(row)))  #제품 모델을 기대값 생성에 사용하지 않는다.
    args.fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n');record=dict(unit=705,date='2026-11-02',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),inputs=len(rows)-1,nativeBodies=['004cb320','004cb3f0','004cb4b0'],explicitBoundaries=['virtual level+80','TYPE address0049a840','current technology0048fc60','owned kind004acac0','translate004de9a0','description0049af80','printf004a89c0','menu00477cb0','heap','assert'],gameExecutions=0,peSha256=oracle.sha256,fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    args.report.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(record,ensure_ascii=False))

if __name__=='__main__':
    main()
