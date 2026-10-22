#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78의41e100 상태 효과 갱신/만료와 콜백 뒤 현재 값 재조회를 독립 관측한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_arc_spire import Oracle as BaseOracle,ROOT,STOP,HEAP,VTABLE,STACK
from decomp_cursor_position import ranges
from decomp_particle_gradient import digest
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP

POOL=0x15000000  # 대상 물리 풀은 기존 보조 타입 표/힙과 겹치지 않는다.
FIELDS,MEMBERS=HEAP+0x800,HEAP+0x1000  # 실제40BYTE 객체와 별도 DWORD 대상 배열이다.
UNPOP,REPOP,REFRESH=STOP+0x700,STOP+0x720,STOP+0x740  # 현재 가상48/4c/88의 명시 효과 경계다.
IDS=(1,60,127)  # 전체 바이트를 비교할 세 물리 슬롯이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/bomb-effect-timer-x86.tsv'  # 새 원문만 생성하며 기존 fixture는 보존한다.
REPORT=ROOT/'cpppj/bomb-effect-timer-20261023.json'  # 호스트/보호PE/정상반환/SHA 근거다.


def cases():
    """비트·활성 상태·만료 경계와 중복 목록·선택 비트·콜백 변화를 교차한다."""
    result=list(itertools.product(range(4),range(8),(0,16,32,48),(0,0x200000),(133,136),(-1,0,1),(0,),(1,)))
    result+=list(itertools.product((0,1,2,3,0x80000003),(0,),(4,20,36,52),(0,0x200000),(133,136),(0,2),range(6),(0,2,3)))
    return list(dict.fromkeys(result))


def members(kind):
    """목록 순서/중복/빈 입력은 원본 생성자에 공급할 명시 입력이다."""
    return () if kind==0 else (60,) if kind==1 else (60,127,60) if kind==2 else (60,1,127)


class Oracle(BaseOracle):
    """원본 Frame을 실행하며 가상 효과·타입 조회·자기 삭제만 경계로 둔다."""
    def __init__(self):
        """정확한 코드 범위와 현재 원문/슬롯의 읽기·쓰기 가드를 설치한다."""
        super().__init__();self.mu.mem_map(POOL,0x200000);self.allowed=[];ranges(self,(0x41e100,));self.returns=0

    def number(self,address,width=4):
        """비정렬 WORD/DWORD/double 비트를 지정 폭으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def read(self,mu,access,address,size,value,data):
        """원본 이미지/명시 풀/객체/스택 밖 접근을 거부한다."""
        if any(lo<=address and address+size<=hi for lo,hi in ((0x400000,0x600000),(POOL,POOL+0x200000),(HEAP,HEAP+0x10000),(VTABLE,VTABLE+0x1000),(STACK-0x7f000,STACK+0x1000))):return
        raise RuntimeError(f'상태 타이머 범위 밖 읽기 {address:x}+{size}')

    def write(self,mu,access,address,size,value,data):
        """프레임의 물리 슬롯/객체/스택 외 쓰기는 허용하지 않는다."""
        if any(lo<=address and address+size<=hi for lo,hi in ((POOL,POOL+0x200000),(HEAP,HEAP+0x10000),(STACK-0x7f000,STACK+0x1000))):return
        raise RuntimeError(f'상태 타이머 범위 밖 쓰기 {address:x}+{size}')

    def instruction(self,mu,address,size,data):
        """기한과 비트 분기는 원본 명령으로 실행하고 순서 있는 큰 호출만 기록한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);arg=self.number(sp+4)
        if address==0x41e100:self.calls['0041e100']+=1
        if address in (UNPOP,REPOP,REFRESH):
            sid=(ecx-POOL)//50;tag={UNPOP:'U',REPOP:'R',REFRESH:'G'}[address];self.event(tag,sid,*(() if tag=='G' else (arg,)))
            if tag=='U' and not self.did_u:
                self.did_u=True
                if self.mutation==3:self.put(MEMBERS,127)
            if tag=='R' and not self.did_r:
                self.did_r=True
                if self.mutation==1:mu.mem_write(FIELDS+24,struct.pack('<d',1000.25))
                if self.mutation==2:self.put(FIELDS+32,3);self.put(POOL+sid*50+40,48,1)
            if tag=='G' and not self.did_g:
                self.did_g=True
                if self.mutation==4:self.put(POOL+sid*50+10,136,1)
                if self.mutation==5:self.put(POOL+sid*50+11,self.number(POOL+sid*50+11,1)|2,1)
            self.ret(purge=0 if tag=='G' else 4);return
        if address==0x4ac200:
            kind=self.number(ecx+10,1);self.ret(self.genus^(0x200000 if kind==136 else 0));return
        if address==0x457890:self.event('F',arg);self.ret();return
        if address==0x41bf10:self.event('K',arg);self.ret(purge=4);return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'상태 타이머 미검토 명령 {address:x}')

    def sample(self,values,control):
        """같은 명시40BYTE/대상 입력에서 효과와 변경된 전체 슬롯을 반환한다."""
        flags,state,extra,self.genus,kind,phase,self.mutation,listing=values;self.events=[];self.did_u=self.did_r=self.did_g=False;items=members(listing)
        self.mu.mem_write(FIELDS,bytes(40));self.put(FIELDS,0x50330c);self.put(FIELDS+12,MEMBERS);self.put(FIELDS+16,100);self.put(FIELDS+20,len(items));self.put(FIELDS+32,flags)
        self.mu.mem_write(FIELDS+24,struct.pack('<d',float('nan') if phase==2 else 100.25+phase));self.mu.mem_write(0x55b4d0,struct.pack('<d',100.25))
        for index,sid in enumerate(items):self.put(MEMBERS+index*4,sid)  # 명시 목록을 그대로 복사하며 중복을 줄이지 않는다.
        for sid in IDS:  # 방문 여부와 관계없이 전체50BYTE를 원문 입력으로 설치한다.
            raw=bytearray(50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=kind;raw[11]=state;raw[40]=extra;self.mu.mem_write(POOL+sid*50,bytes(raw))
        for address,value in ((0x5c8464,POOL),(0x541278,136),(VTABLE+0x48,UNPOP),(VTABLE+0x4c,REPOP),(VTABLE+0x88,REFRESH)):self.put(address,value)  # 현재 전역과 실제 가상 호출 표다.
        self.call(0x41e100,FIELDS,b'',0,control);self.returns+=1;count=self.number(FIELDS+20)
        fields=','.join(map(str,(self.number(FIELDS+24,8),self.number(FIELDS+32),*(self.number(MEMBERS+i*4) for i in range(count)))))
        return ';'.join(self.events) or '-',fields,''.join(bytes(self.mu.mem_read(POOL+sid*50,50)).hex() for sid in IDS)


def main():
    """새 원문 생성과 기존 근거의 읽기 전용 검증을 분리한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in {**facts['sha256'],**facts.get('implementationSha256',{})}.items():assert digest(ROOT/path)==wanted,path  # C++ 출력으로 기대값을 다시 만들지 않는다.
        assert len([line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')])==facts['uniqueInputs'];print('상태 타이머 원문/SHA 확인');return
    first,second=Oracle(),Oracle();rows=[]
    for values in cases():  # 두 x87정밀도에서 순서/기한/목록/전체 바이트가 같아야 한다.
        a=first.sample(values,0x27f);b=second.sample(values,0x37f);assert a==b,values;rows.append((','.join(map(str,values)),*a))
    FIXTURE.write_text('# input effects endBits,flags,members slots1/60/127\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_bomb_effect_timer.py','tools/decomp_arc_spire.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py',FIXTURE.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',uniqueInputs=len(rows),normalReturns=first.returns+second.returns,nativeInstructions=first.instructions+second.instructions,precisionDifferentInputs=[],nativeCalls=dict(first.calls+second.calls),frame='0041e100',constructor='00420a40',vtable='0050330c',sha256={p:digest(ROOT/p) for p in paths},limitations=['41e100 전체 Frame을 실행한다. 가상48/4c/88·457890·41bf10 및 현재TYPE genus 조회는 명시 큰 경계다.','생성자 메모리 확보/벡터 복사, 네트워크 직렬화/수신 및117/118 주문 생산 전체의 완료 증거는 아니다.','손상된 DWORD SID의 주소 감김이나 임의 메모리 접근은 관측하지 않는다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('상태 타이머:',len(rows),facts['normalReturns'],facts['nativeInstructions'])


if __name__=='__main__':main()
