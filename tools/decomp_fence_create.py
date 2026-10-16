#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""표식 생성00456460·포스필드mode1·재생성00457460을 실제10.78 명령으로 관찰한다."""
import csv
import json
import socket
import struct
import sys
from collections import Counter
from pathlib import Path
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, PLAYERS, TABLE, STACK, STOP, digest
from decomp_fence_line import cases as line_cases
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

# 원본과 분리한 부모/새 객체/가상 표 및 두 출력 파일이다.
VTABLE=TABLE+0xf0000
FIXTURE=ROOT/'cpppj/tests/fixtures/fence-create-x86.tsv'
REPORT=ROOT/'cpppj/fence-create.json'


def bits(value):
    """float 입력을 binary32 비트로 보존한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


class Oracle(BaseOracle):
    """현재 생성 제어/좌표/태그는 실제 명령이고 객체·통지·소유자·Pop는 명시 외부 경계다."""
    def __init__(self):
        """현재 정밀 표의 부모와 직접 수학/필드 helper만 실행을 허용한다."""
        super().__init__();self.allowed=[];self.returns=0  #실제 반개방 범위와 정상 반환 누계다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            functions={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        selected=(0x456460,0x455b20,0x457460,0x4adea0,0x49a900,0x4ac550,0x49a840,0x41ce90,0x40e880,0x40e8f0,0x41d0d0,0x40e800,0x455a40,0x43fa50,0x4e49c0,0x4adca0,0x4ad3e0)
        for address in selected:  #선택한 실제 부모와 직접 helper의 범위를 그대로 읽는다.
            for part in functions[address]['ranges'].split(';'):  #불연속 몸체를 합치거나 넓히지 않는다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self,mu,access,address,size,value,data):
        """실제 명령은 스택과 방금 생성된 객체 기록만 쓸 수 있다."""
        if STACK-4096<=address and address+size<=STACK+128:return
        if self.created and PLAYERS+60*50<=address and address+size<=PLAYERS+(60+self.created)*50:return
        raise RuntimeError(f'펜스 생성 선택 밖 쓰기 {address:x}/{size}')

    def instruction(self,mu,address,size,data):
        """실제 인수를 기록하고 선언한 후속 현재 상태 변경만 공급한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,9);this=mu.reg_read(UC_X86_REG_ECX);parent=PLAYERS+50*50
        if self.kind=='R':
            if address==0x455fb0:
                self.direction=args[0];self.events.append('Neighbor,'+str(args[0]));result=51+args[0]//2 if self.mask&(1<<(args[0]//2)) else 0
                if self.mutation==1 and args[0]==0:self.put(parent+14,bits(120.0))
                self.ret(result,purge=4);return
            if address==0x455f00:
                self.events.append('Range,'+','.join(map(str,args[:4])))
                if self.mutation==2:self.put(PLAYERS+(51+self.direction//2)*50+40,0x21,1)
                self.ret(1 if self.rangeMask&(1<<(self.direction//2)) else 0,purge=16);return
            if address==0x456460:
                self.events.append('Markers,'+str(args[0]))
                if self.mutation==3:self.put(parent+34,5,1);self.put(PLAYERS+args[0]*50+18,bits(125.0))
                self.ret(purge=4);return
            if address==0x455b20:
                assert args[4]==1;self.events.append('Shields,'+','.join(map(str,args)))
                if self.mutation==4:self.put(parent+40,9,1)
                self.ret();return
        if address==0x4af530:
            assert args[1]==2 and self.created<100;sid=60+self.created;self.created+=1;self.events.append(f'Create,{args[0]},{args[1]},{sid}')
            raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=args[0];raw[11]=0;raw[34]=0;raw[40]=32 if self.kind=='S' and self.blockAt==self.created else 0;struct.pack_into('<I',raw,36,17)
            if self.kind=='S' and self.mutation==1:raw[10]=171
            self.mu.mem_write(PLAYERS+sid*50,bytes(raw))
            if self.kind=='S' and self.mutation==3 and self.created==1:self.put(0x541300,171)
            self.ret(PLAYERS+sid*50);return
        if address==0x4214a0:
            self.events.append('Notify,'+str(args[0]))
            if self.kind=='M' and self.mutation==1:self.put(parent+34,7,1)
            self.ret();return
        if address==STOP+0x200:
            sid=(this-PLAYERS)//50;self.events.append(f'Owner,{sid},{args[0]}');self.put(this+34,args[0],1)
            if self.kind=='M' and self.mutation==2:self.put(parent+34,6,1)
            if self.kind=='S' and self.mutation==2:self.put(parent+14,bits(199.0))
            self.ret(purge=4);return
        if address==STOP+0x300:
            sid=(this-PLAYERS)//50;assert args[2]==64;self.events.append(f'Pop,{sid},{args[0]},{args[1]},{args[2]}');self.put(this+14,args[0]);self.put(this+18,args[1])
            if self.kind=='M' and self.mutation==3:self.put(parent+14,bits(199.0));self.put(PLAYERS+self.other*50+18,bits(200.0))
            self.ret(purge=12);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'펜스 생성 미검토 명령 {address:x}')

    def run(self,case,control):
        """입력만 설치하고 실제 부모/생성 기록/효과/호출 규약을 관찰한다."""
        self.kind,values=case;self.events=[];self.created=0;self.other=51;self.blockAt=0;self.mutation=0  #이번 요청과 원본이 실제 선택한 생성 수다.
        self.put(0x5c8464,PLAYERS);self.put(0x59ab20,TABLE);self.put(0x5411c0,90);self.put(0x541300,170);self.put(0x541288,140);self.put(0x531928,256);self.put(0x54040c,255);self.put(0x540410,255)
        self.put(VTABLE+0x74,STOP+0x200);self.put(VTABLE+0x90,STOP+0x300)
        for number in (90,136,170,171):  #부모 중심과 생성TYPE의 현재 J/K 첫 번호다.
            self.put(TABLE+number*500+0x1d4,1);self.put(TABLE+number*500+0x1d8,1);self.put(TABLE+number*500+0x154,13 if number==171 else 8);self.put(TABLE+number*500+0x158,21 if number==171 else 16)
        if self.kind=='M':self.other=values[7];self.mutation=values[6]
        if self.kind=='S':self.blockAt=values[8];self.mutation=values[9]
        if self.kind=='R':extra,self.mask,self.rangeMask,packed,self.mutation=values
        ids=tuple(range(50,55)) if self.kind=='R' else (50,self.other);self.parents=ids
        for sid in ids:  #참조하는 부모/이웃의 모든 보존 바이트도 결과에 포함한다.
            raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=136;raw[11]=0;raw[40]=0;raw[34]=1
            x,y=(float(sid)+.25,float(sid)+.5) if self.kind=='R' else struct.unpack('<ff',struct.pack('<II',*values[0:2] if sid==50 else values[2:4]))
            struct.pack_into('<ff',raw,14,x,y)
            if self.kind=='R':raw[40]=extra if sid==50 else (packed>>((sid-51)*8))&255
            else:raw[34]=values[4] if sid==50 else values[5]
            self.mu.mem_write(PLAYERS+sid*50,bytes(raw))
        if self.kind=='M':entry=0x456460;receiver=PLAYERS+50*50;args=[self.other];purge=4
        elif self.kind=='S':entry=0x455b20;receiver=0;args=[*values[:4],1,*values[4:8]];purge=0
        else:entry=0x457460;receiver=PLAYERS+50*50;args=[];purge=0
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_FPSW,0)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0x12340000+reg)  #비휘발 입력을 구별한다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,receiver);self.mu.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),STOP,*args));self.mu.emu_start(entry,STOP,count=100000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge and self.mu.reg_read(UC_X86_REG_FPCW)==control and self.mu.reg_read(UC_X86_REG_FPSW)&0x3800==0
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(reg)==0x12340000+reg  #정상 호출 규약을 매번 확인한다.
        self.returns+=1
        return [self.kind,','.join(map(str,values)),b''.join(bytes(self.mu.mem_read(PLAYERS+sid*50,50)) for sid in ids).hex(),b''.join(bytes(self.mu.mem_read(PLAYERS+sid*50,50)) for sid in range(60,60+self.created)).hex() or '-',';'.join(self.events) or '-']


def cases():
    """기존99개 좌표와 실제 소유자/태그/현재 효과/차단/각 재생성 관문을 교차한다."""
    coordinates=list(line_cases())
    for points in coordinates:yield 'M',(*points,1,2,0,51)  #현재 중심은 입력raw에서0.5를 빼는 실제 helper가 계산한다.
    for mutation in (1,2,3):yield 'M',(*(bits(x) for x in (50.25,50.5,67.25,50.5)),1,2,mutation,51)  #각 효과 뒤 현재 owner와 고정 이동 경로다.
    for other in (0x1234,0x3a99):yield 'M',(*(bits(x) for x in (50.25,50.5,58.75,50.5)),1,2,0,other)  #끝점 SID의 high8/low8 압축을 확인한다.
    for x,y in ((56.75,55.5),(45.75,56.5)):yield 'M',(*(bits(z) for z in (50.25,50.5,x,y)),1,2,0,51)  #사각형 대각 두 방향에서도 원본4방향 우선을 유지한다.
    for points in coordinates:  #포스 필드는 동일 좌표에서 소유자 선택의 세 가지 경로를 구별한다.
        for first,second,a,b in ((1,1,50,51),(1,2,100,20),(1,2,50,51)):yield 'S',(*points,first,second,a,b,0,0)
    for mutation in (1,2,3):yield 'S',(*(bits(x) for x in (50.25,50.5,67.25,50.5)),1,2,50,51,0,mutation)  #생성TYPE/고정좌표/현재 타입 전역 변경이다.
    for blocked in (1,3,5):yield 'S',(*(bits(x) for x in (50.25,50.5,67.25,50.5)),1,2,50,51,blocked,0)  #bit20은 새 객체를 만든 뒤 전체 생성을 중단한다.
    for a,b in ((0xffffffff,1),(1,0xffffffff),(50,50)):yield 'S',(*(bits(x) for x in (50.25,50.5,58.75,50.5)),1,2,a,b,0,0)  #signed SID 비교와 동일값의 선택이다.
    for extra in (0,1,8,9,32,33):  #부모는bit9만 검사한다.
        for mask in (0,5,15):  #현재 동맹 최근접 결과다.
            for distance in (0,5,15):  #각 방향의 실제 거리 반환이다.
                for packed in (0,0x21080100,0x20002000):yield 'R',(extra,mask,distance,packed,0)  #대상extra21/허용bit8을 구별한다.
    for mutation in (1,2,3,4):yield 'R',(0,15,15,0,mutation)  #거리/표식/포스필드 효과 뒤 현재 재조회다.


def generate():
    """고유 입력의 두 x87 정상 결과와 명시 외부 경계를 저장한다."""
    oracle=Oracle();rows=[]
    for case in cases():  #같은 입력의 두 정밀도는 새 고유 입력으로 더하지 않는다.
        first=oracle.run(case,0x027f);second=oracle.run(case,0x037f)
        if first!=second:raise RuntimeError('펜스 생성 x87 정밀도 차이')
        rows.append(first)
    FIXTURE.write_text('# kind inputs parents created trace\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files=[Path(__file__),ROOT/'tools/decomp_local_mission_transport.py',ROOT/'tools/decomp_fence_line.py',ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',FIXTURE,ROOT/'cpppj/src/o/RawFenceCreate.h',ROOT/'cpppj/src/o/RawFenceCreate.cpp',ROOT/'cpppj/src/o/RawFenceRebuild.h',ROOT/'cpppj/src/o/RawFenceRebuild.cpp',ROOT/'cpppj/tests/FenceCreateTests.cpp']
    REPORT.write_text(json.dumps(dict(host=socket.gethostname(),last_full_decompile_host='VM-W11-Codex',last_full_decompile_date='2026-10-16',total=len(rows),scopes=dict(Counter(row[0] for row in rows)),normal_returns=oracle.returns,os_calls=0,sha256={path.relative_to(ROOT).as_posix():digest(path) for path in files},limits=['생성/통지/소유자/Pop은 현재 원본 부모가 호출한 인수와 명시 raw변경의 외부 경계다.','M/S는 실제 좌표/중심/방향/Rect/해시구간/태그/Frame/owner선택이고 R은 실제 제어에서 이웃/거리/하위 생성을 명시 입력으로 둔다.','새 일반객체의 전체 원본 생성자/공간/표시를 이번 관찰에서 실행한 것으로 세지 않는다.','CD판/사건3 잔류x87/이웃재Pop/전체 게임 완주·그래픽동일성은 후속이다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('펜스 생성',len(rows),dict(Counter(row[0] for row in rows)),oracle.returns,'정상 반환')


def verify():
    """모든 현재 출처와 고유 입력/두 정밀도 정상 반환 수를 감사한다."""
    record=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in record['sha256'].items():  #관찰기/원본/제품/검사를 전수 비교한다.
        if digest(ROOT/name)!=value:raise RuntimeError('SHA 불일치 '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==record['total'] and len({tuple(row[:2]) for row in rows})==len(rows) and dict(Counter(row[0] for row in rows))==record['scopes'] and record['normal_returns']==len(rows)*2
    print('펜스 생성',len(rows),'개 SHA/반환/고유 입력 감사 통과')


# 원본 게임/OS/창이 아닌 명시한 PE 명령 범위만 격리 실행한다.
if __name__=='__main__':verify() if '--verify' in sys.argv else generate()
