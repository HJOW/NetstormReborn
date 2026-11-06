#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 SID 할당의 단언 복귀/현재값/최대50회수와 실제 Native 반납을 창 없이 동결한다."""
import csv
import hashlib
import json
import re
import socket
import struct
from pathlib import Path
from decomp_sid_oracle import SidOracle,ROOT,POOL,SPECS,SCRATCH,STACK,STOP
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

WORK=ROOT/'extracted/cpp-resume-976'  # 현재 호스트의 정밀 범위/원문 관측/초기 실패를 보관한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/sid-allocation-recovery-x86.tsv'  # 기존 SID 정상 시퀀스 fixture와 구별한다.
FAKE_TABLE,FAKE_KILL,RELEASE_RETURN=SCRATCH+0x1000,SCRATCH+0x1100,SCRATCH+0x1200  # 실제 가상 삭제는 Native Release를 호출하는 명시 외부 효과다.
BOSS,BRIDGE,DISALLOW=0x540bc4,0x5411a0,0x5c8458  # 원문의 현재 권한/전체 DWORD 종류/금지 전역이다.
CAPACITY=23005  # 양수 일반 풀과 마지막 예측 슬롯을 작게 재현하는 합법 용량이다.


class CallbackStop(Exception):
    """가상 삭제 외부 장치의 예외 뒤 원문 부분 효과를 보존한다."""


class AllocationOracle(SidOracle):
    """현재 정밀 범위의 실제 할당/iterator/반납/기록 CRT를 실행하고 단언/가상 효과만 경계로 둔다."""
    def __init__(self):
        """실행할 함수와 호출 의존성의 현재 canonical 범위만 독립 파일로 선별한다."""
        WORK.mkdir(parents=True,exist_ok=True)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as source:
            rows={int(row['entry'],16):row for row in csv.DictReader(source,delimiter='\t')}
        text=(ROOT/'extracted/refined/originals/Netstorm.c').read_text(encoding='utf-8')
        wanted={0x4abb10,0x4af1d0,0x4abf60,0x4aba70,0x40eb40,0x4e5b40};pending=list(wanted)  # 실제 Reset/Allocate/Release/헤더 setter/iterator/기록 겹친 복사다.
        while pending:  # 원문 FUN 호출의 의존성을 추가하되 기대값/제품 코드를 읽지 않는다.
            entry=pending.pop();tag=f'// ==== FUN_{entry:08x} @ {entry:08x}';start=text.find(tag)
            if start<0:continue
            end=text.find('// ==== ',start+len(tag));body=text[start:end if end>=0 else None]
            for called in re.findall(r'FUN_([0-9a-f]{8})\(',body):  # 원문 본문의 직접 호출 주소만 의존성 후보로 읽는다.
                target=int(called,16)
                if target in rows and target not in wanted:wanted.add(target);pending.append(target)
        path=WORK/'allocation-functions.tsv'
        with path.open('w',encoding='utf-8',newline='') as output:
            writer=csv.DictWriter(output,fieldnames=['entry','ranges'],delimiter='\t',lineterminator='\n');writer.writeheader()
            for entry in sorted(wanted):writer.writerow({key:rows[entry][key] for key in ('entry','ranges')})  # 범위는 현재 정밀 산출물의 원문 값이다.
        super().__init__('originals',path)
        self.code_base=min(low for low,_ in self.allowed)  #현재정밀허용범위의주소기준이다.
        self.code_allowed=bytearray(max(high for _,high in self.allowed)-self.code_base)  #매명령의선형범위검색을같은바이트허용표로대체한다.
        for low,high in self.allowed:self.code_allowed[low-self.code_base:high-self.code_base]=b'\1'*(high-low)  #허용집합자체는기존정밀범위와같다.
        self.events=[]  # 현재 단언/가상 삭제의 실제 순서와 인자다.
        self.mode=0  # 명시 외부 효과/오염 입력을 선택한다.
        self.kills=0  # 실제 Native 가상 삭제 진입 횟수다.
        self.tracing=False  # 준비 Reset/예측 할당은 최종 호출 사건에 포함하지 않는다.

    def number(self,address,value=None,width=4):
        """가상 원문 메모리의 현재 정수를 little endian으로만 읽고 쓴다."""
        if value is None:return int.from_bytes(self.mu.mem_read(address,width),'little')
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def text(self,address):
        """원문 단언 문자열의 첫NUL까지만 읽는다."""
        result=bytearray()
        while (value:=self.number(address+len(result),width=1))!=0:result.append(value)  # 실제 영어 식/경로만 기록한다.
        return bytes(result)

    def returned(self):
        """단언의 cdecl 정상 복귀만 대체하며 호출자 인자 정리는 원문에 맡긴다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EIP,self.number(sp));self.mu.reg_write(UC_X86_REG_ESP,sp+4);self.mu.reg_write(UC_X86_REG_EAX,0)

    def on_instruction(self,mu,address,size,data):
        """가상 삭제는 원문 Release의 별도 cdecl 호출로 이어가고 단언은 명시 복귀/입력 보수만 한다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if address==self.spec['assert']:
            expression,file,line=self.text(self.number(sp+4)),self.text(self.number(sp+8)),self.number(sp+12)
            if self.tracing:self.events.append(f'A:{line}:{expression.hex()}:{file.hex()}')
            if self.mode==1 and line==0xe8:self.number(DISALLOW,0)
            if self.mode==11 and line==0xf0:self.number(POOL+23001*50+11,1,1)
            self.returned();return
        if address==FAKE_KILL:
            sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50;flags=self.number(sp+4);assert flags==0;self.kills+=1
            if self.tracing:self.events.append(f'K:{sid}:0')
            self.mu.mem_write(sp-8,struct.pack('<II',RELEASE_RETURN,sid));mu.reg_write(UC_X86_REG_ESP,sp-8);mu.reg_write(UC_X86_REG_EIP,self.entries['Release']);return
        if address==RELEASE_RETURN:
            original_sp=sp+4  # Native cdecl Release가 남긴 SID 인자를 원래 가상 호출 스택과 구별한다.
            if self.kills==1:
                if self.mode==7:self.number(BRIDGE,70)
                elif self.mode==8:self.number(POOL+12*50,FAKE_TABLE);self.number(POOL+12*50+10,82,1);self.number(POOL+12*50+11,4,1)
                elif self.mode==9:raise CallbackStop()
            mu.reg_write(UC_X86_REG_EIP,self.number(original_sp));mu.reg_write(UC_X86_REG_ESP,original_sp+8);return  # 원래 virtual+10의 ret4를 수행한다.
        self.instructions+=1
        if address==self.spec['copy']:
            _ret,dest,source,length=struct.unpack('<4I',mu.mem_read(sp,16));assert length==152 and source in self.spec['logs'] and dest==source+8;self.copies+=1
        offset=address-self.code_base
        if not 0<=offset<=len(self.code_allowed)-size or not all(self.code_allowed[offset:offset+size]):raise RuntimeError(f'할당 원문 허용 밖 명령 {address:08x}')

    def observe(self,mode,flags,server,boss):
        """같은 합법 Pool을 준비한 뒤 명시 raw 입력/외부 효과만 적용한다."""
        self.mode=0;self.tracing=False;self.begin(CAPACITY,bool(server));self.number(DISALLOW,0);self.number(BOSS,boss);self.number(BRIDGE,82);self.step('Reset')
        self.number(FAKE_TABLE+0x10,FAKE_KILL);client=bool(flags&2);head=POOL+(0 if client else 14);tail_index=2 if client else 4;tail=(self.number(self.spec['globals'][tail_index])-POOL)//50
        if mode==15:
            for _ in range(2):self.step('Allocate',1)  # 마지막 예측 후보의 커서/카운터도 실제 두 준비 할당으로 만든다.
        if mode==1:self.number(DISALLOW,1)
        if mode==3:self.number(POOL+self.number(head+4, width=2)*50+11,0,1)
        if mode in (4,5,6,7,8,9,17):self.number(head+4,tail,2)  # 리스트 끝의 예약 꼬리만 남은 명시 소진 입력이다.
        bridges=list(range(10,70)) if mode==6 else ([10,11,13] if mode in (5,7,8,9) else [])
        for sid in bridges:  # 이 경계의 삭제 효과는 게임 클래스 대신 실제 Native Release다.
            self.number(POOL+sid*50,FAKE_TABLE);self.number(POOL+sid*50+10,70 if mode==7 and sid==11 else 82,1);self.number(POOL+sid*50+11,4,1)
        if mode==17:self.number(BRIDGE,0x10052)
        if mode==11:self.number(POOL+23001*50+11,0,1)
        if mode==12:self.number(POOL+23002*50+11,0,1)
        if mode in (13,14):self.number(POOL+23002*50+4,23005 if mode==13 else 23001,2)
        if mode==13:self.number(POOL+23002*50+4,23004,2)  # 실제 범위 안의 다른 free 후보로 f7 뒤 정상 복귀한다.
        self.mode=mode;self.events=[];self.kills=0;self.tracing=True
        stopped=False
        try:result=self.step('Allocate',flags)
        except CallbackStop:
            stopped=True;result=self.state_without_result()
        self.tracing=False
        return [int(stopped),*result,self.number(BRIDGE),self.number(DISALLOW),';'.join(self.events) or '-']

    def state_without_result(self):
        """외부 장치 예외이면 반환값을 만들지 않고 이미 수행한 실제 Native 부분 상태만 읽는다."""
        import zlib
        glob=self.spec['globals'];free,cursor=(self.number(glob[i]) for i in (5,7));ch=self.number(POOL+4,width=2);sh=self.number(POOL+18,width=2);ct,st=((self.number(glob[i])-POOL)//50 for i in (2,4));pool=zlib.adler32(self.mu.mem_read(POOL,CAPACITY*50));logs=zlib.adler32(b''.join(bytes(self.mu.mem_read(a,160)) for a in self.spec['logs']))
        return [0,free,cursor,ch,ct,sh,st,pool,logs]


def main():
    """정상/진단 복귀/회수/예측 경계를 제품 코드 없이 먼저 고정한다."""
    oracle=AllocationOracle();cases=[]
    for mode in (0,1,3,4,5,6,7,8,9,17):  # 일반 두 리스트/권한/회수 상한과 부분 실패다.
        for flags in (0,2):cases.append((mode,flags,1,0 if mode==4 else 1))  # 일반 server/client 목록 선택을 각각 관측한다.
    cases.extend([(0,0,0,0),(1,2,0,0),(5,2,0,1)])
    for mode in (10,11,12,13,14,15):  # 예측 우선과 서버 유무를 모두 원문에서 관측한다.
        for flags in (1,3):  # client 비트 동시 입력에서도 predictable 우선을 확인한다.
            for server in (0,1):cases.append((mode,flags,server,1))  # 예측 next 갱신의 권한 분기를 각각 관측한다.
    rows=['# mode\tflags\tserver\tboss\tstop\tresult\tfree\tcursor\tch\tct\tsh\tst\tpoolAdler\tlogsAdler\tbridge\tdisallow\tevents']
    for case in cases:rows.append('\t'.join(map(str,(*case,*oracle.observe(*case)))))  # 47개 명시 입력의 전체 Pool/기록/순서다.
    content=('\n'.join(rows)+'\n').encode('utf-8')
    if FIXTURE.exists():assert FIXTURE.read_bytes()==content,'기존 할당 소진 fixture와 원문 불일치'
    else:FIXTURE.write_bytes(content)
    report=dict(host=socket.gethostname(),lastFullDecompilation='HJOW-Athlon / 2026-11-03',inputs=len(cases),binarySha256=oracle.sha256,fixtureSha256=hashlib.sha256(content).hexdigest(),fakeTable=FAKE_TABLE,boundary='PATCH Allocate/Reset/iterator/Release/기록CRT 실제원문. 단언정상복귀/가상삭제→Native Release/명시외부변형만경계. 실제게임/MP/공격없음.')
    (WORK/'native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False),flush=True)


if __name__=='__main__':main()
