#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""썬 바리케이드 Handle의0/1/2/기본 사건 제어를 실제10.78 명령으로 격리 관찰한다."""
import csv
import json
import socket
import struct
import sys
from collections import Counter
from pathlib import Path
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, PLAYERS, TABLE, IMAGE, HEAP, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

# 다섯 입력 슬롯과 실제48개 프레임 코드의 별도 메모리다.
IDS=tuple(range(50,55))
CODES=TABLE+0x80000
FIXTURE=ROOT/'cpppj/tests/fixtures/fence-regular-x86.tsv'
REPORT=ROOT/'cpppj/fence-regular.json'


class Oracle(BaseOracle):
    """실제 부모/방향/TYPE 읽기만 실행하고 선/이웃/예약/표시/Damageable은 선언된 경계다."""
    def __init__(self):
        """현재 정밀 범위에서 선택한 네 함수만 실행하고 상태 쓰기를 제한한다."""
        super().__init__();self.allowed=[];self.returns=0  # 실제 범위와 정상 반환 수다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            functions={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for address in (0x457a50,0x4ad710,0x4ac550,0x49a840):  # 실제 Handle와 방향/TYPE getter다.
            for part in functions[address]['ranges'].split(';'):  # 불연속 몸체도 모두 선택한다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)
        self.mu.mem_write(STOP+0x100,b'\xd9\x1d'+struct.pack('<I',IMAGE)+b'\xe9'+struct.pack('<i',STOP-(STOP+0x100+11)))
        self.mu.mem_write(STOP+0x200,b'\xd9\x44\x24\x0c\xc2\x0c\x00')  # 명시 Damageable 경계는 전달 payload를 x87로 반환한다.

    def guard(self,mu,access,address,size,value,data):
        """SEH/입력 슬롯/반환 float/스택 이외의 실제 명령 쓰기는 거부한다."""
        ranges=[(0,4),(STACK-4096,STACK+128),(IMAGE,IMAGE+4)]+[(PLAYERS+sid*50,PLAYERS+(sid+1)*50) for sid in IDS]
        if not any(low<=address and address+size<=high for low,high in ranges):raise RuntimeError(f'바리케이드 사건 선택 밖 쓰기 {address:x}/{size}')

    def instruction(self,mu,address,size,data):
        """각 경계의 실제 인수를 기록하고 명시한 현재 상태 변경 입력만 적용한다."""
        if address in (STOP+0x100,STOP+0x106,STOP+0x200,STOP+0x204):return
        sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,9);this=mu.reg_read(UC_X86_REG_ECX);raw=PLAYERS+50*50
        if address==0x4acee0:
            assert this==raw and args[1]==0;self.events.extend(('U:'+str(self.words(raw+36)[0]),'U:'+str(args[0])));self.put(raw+36,args[0])
            if self.mutation==1:self.put(raw+34,7,1)
            self.ret(purge=8);return
        if address==0x456220:
            self.events.append('N:'+str(args[0]))
            if self.mutation==2 and args[0]==0:self.put(raw+34,6,1)
            self.ret(51+args[0]//2 if self.mask&(1<<(args[0]//2)) else 0,purge=4);return
        if address==0x4566d0:
            self.events.append('M:'+str(args[0]));self.markers+=1
            if self.mutation==3 and self.markers==1:self.put(PLAYERS+51*50+18,struct.unpack('<I',struct.pack('<f',125.0))[0]);self.put(raw+14,struct.unpack('<I',struct.pack('<f',120.0))[0])
            self.ret(purge=4);return
        if address==0x455b20:
            assert args[4:]==(0,0,0,0,0);self.events.append('L:'+','.join(map(str,args[:4])));self.ret();return
        if address==0x4573d0:
            self.events.append('C');self.connectionCalls+=1
            if self.mutation==5:self.put(raw+34,5,1)
            if self.mutation==9:self.put(raw+46,self.first)
            self.ret(self.first if self.connectionCalls==1 else self.second);return
        if address==0x4e4391:
            assert args[0]==40;self.events.append('R');success=(self.allocations>>self.reserves)&1;self.reserves+=1
            if self.mutation==4 and self.reserves==1:self.put(raw+46,77);self.put(raw+34,7,1)
            self.ret(HEAP+self.reserves*64 if success else 0);return
        if address==0x496e80:
            assert args[:3]==(2,50,0x3dcccccd);self.events.append('A:2:50:'+str(args[2]));self.ret(purge=12);return
        if address in (0x4578f0,0x457460):
            self.events.append('F' if address==0x4578f0 else 'B')
            if self.mutation==6:self.put(raw+12,0x7777,2)
            if self.mutation==7:self.put(raw+36,22)
            self.ret();return
        if address==0x44b900:
            self.events.append('D:'+','.join(map(str,args[:3])))
            if self.mutation==8:self.put(raw+12,0x8888,2)
            mu.reg_write(UC_X86_REG_EIP,STOP+0x200);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'바리케이드 사건 미검토 명령 {address:x}')

    def run(self,case,control):
        """독립 입력에서 원본 전체250BYTE·반환 float·효과/SEH/정상 반환을 관찰한다."""
        kind,values=case;event,extra,frame,ownerCache,countCache,self.first,self.second,self.mask,self.allocations,self.mutation,payload,count=values
        self.events=[];self.markers=0;self.reserves=0;self.connectionCalls=0  # 실제로 선택한 호출의 현재 누계다.
        self.put(0,0x12345678);self.put(0x5c8464,PLAYERS);self.put(0x59ab20,TABLE)
        for sid in IDS:  # 부모/네 이웃의 서로 다른 좌표와 보존 패턴을 공급한다.
            raw=bytearray(b'\xa5'*50);struct.pack_into('<IHHHBBHff',raw,0,0x509470,0,0,0,136,0,114,float(sid)+.25,float(sid)+.5);raw[33]=1;raw[34]=1;raw[40]=extra if sid==50 else 0;struct.pack_into('<I',raw,36,frame);struct.pack_into('<II',raw,42,ownerCache,countCache);self.mu.mem_write(PLAYERS+sid*50,bytes(raw))
        self.put(TABLE+136*500+0x124,CODES)
        self.mu.mem_write(CODES,b''.join(bytes((65+i//8,65,i,0)) for i in range(48)))
        for direction in range(16):self.put(TABLE+136*500+0x170+direction*4,8 if direction<6 else 0)  # 실제 글자 구간 수를 입력한다.
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_FPSW,0)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0x12340000+reg)  # 비휘발 입력을 구별한다.
        self.mu.reg_write(UC_X86_REG_ECX,PLAYERS+50*50);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<4I',STOP+0x100,event,count,payload));self.mu.emu_start(0x457a50,STOP,count=100000)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+16 and self.words(0)[0]==0x12345678 and self.mu.reg_read(UC_X86_REG_FPCW)==control and self.mu.reg_read(UC_X86_REG_FPSW)&0x3800==0
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(reg)==0x12340000+reg  # 원본 호출 규약을 매번 확인한다.
        self.returns+=1
        return [kind,','.join(map(str,values)),b''.join(bytes(self.mu.mem_read(PLAYERS+sid*50,50)) for sid in IDS).hex(),self.words(IMAGE)[0],';'.join(self.events) or '-']


def cases():
    """owner/연결수 두 독립 변경·각 할당 실패·현재 효과 이후 재조회와 기본 위임을 교차한다."""
    for frame in (0,7,8,47):  # 현재 방향과 절대 나머지 프레임을 구별한다.
        for owner in (0,1,0x1000001):  # BYTE owner와 DWORD 캐시 비교다.
            for cached in (0,2,0xffffffff):  # 이전 연결 수의 전체DWORD다.
                for first,second in ((0,0),(2,2),(2,3)):  # 두 번째 현재 조회의 다른 값도 보존한다.
                    for mask in (0,5,15):  # 네 방향 스냅샷의 없음/두 방향/전체다.
                        for allocate in range(4):yield 'A',(0,0,frame,owner,cached,first,second,mask,allocate,0,0x3e000000,0)  # 두 독립 new 성공 비트다.
    for mutation in range(1,10):yield 'A',(0,0,7,0,0,2,3,15,3,mutation,0x3e000000,0)  # 각 외부 효과 이후 현재 상태를 바꾸며 첫 연결 조회가 캐시도 바꾼다.
    for event in (1,2):  # 실제 이웃 갱신/재생성 위임을 구별한다.
        for payload in (0,0x3f800000,0x7fc12345):  # 이 사건의 원본 반환0과 입력payload가 다름을 확인한다.
            for mutation in (0,6,7):yield 'E',(event,0,7,1,0,0,0,15,3,mutation,payload,0)
    for event,extra in ((0,32),(1,32),(2,32),(3,32),(99,0),(99,32)):  # bit20은 사건 번호보다 먼저 위임한다.
        for payload in (0,0x80000000,0xbf800000,0x7fc12345):  # 일반 Damageable 경계의 float 비트다.
            for count in (0,0xffffffff):yield 'D',(event,extra,7,1,0,0,0,0,0,0,payload,count)
    yield 'D',(99,32,7,1,0,0,0,0,0,8,0x3e000000,17)  # 위임 효과 이후 실제 raw 변경도 비교한다.


def generate():
    """사건3의 미정 반환을 제외한 고유 입력의 두 x87 정상 결과를 기록한다."""
    oracle=Oracle();rows=[]
    for case in cases():  # 같은 입력의 두 정밀도 재생은 새 고유 입력으로 더하지 않는다.
        first=oracle.run(case,0x027f);second=oracle.run(case,0x037f)
        if first!=second:raise RuntimeError('바리케이드 사건 x87 정밀도 차이')
        rows.append(first)
    FIXTURE.write_text('# kind inputs objects return trace\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files=[Path(__file__),ROOT/'tools/decomp_local_mission_transport.py',ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',FIXTURE,ROOT/'cpppj/src/o/RawFenceRegular.h',ROOT/'cpppj/src/o/RawFenceRegular.cpp',ROOT/'cpppj/tests/FenceRegularTests.cpp',ROOT/'cpppj/src/client/RawCampaignSession.cpp',ROOT/'cpppj/src/o/CMakeLists.txt',ROOT/'cpppj/tests/CMakeLists.txt',ROOT/'cpppj/tests/FenceLineTests.cpp']
    REPORT.write_text(json.dumps(dict(host=socket.gethostname(),last_full_decompile_host='VM-W11-Codex',last_full_decompile_date='2026-10-16',total=len(rows),scopes=dict(Counter(row[0] for row in rows)),normal_returns=oracle.returns,os_calls=0,sha256={path.relative_to(ROOT).as_posix():digest(path) for path in files},limits=['실제 Handle/TYPE/방향 getter 명령이며 이웃/연결수/선·mark삭제/힙/예약/이웃갱신/재생성/프레임·Damageable은 명시 외부 경계다.','C++ 비교는 실제 SquidFrame을 사용한다. Session은 기존 실제 이웃/연결선/Kernel을 사용하고 미복원 하위는 오류로 유지한다.','보호bit20 없는 사건3의 잔류ST0 반환은 미관찰·미복원이며 임의0/-1로 대신하지 않는다.','전체 썬 바리케이드 공격/표시/자연 게임 완주의 증거가 아니다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('바리케이드 사건',len(rows),dict(Counter(row[0] for row in rows)),oracle.returns,'정상 반환')


def verify():
    """고유 입력/정상 반환/선언한 출처의 현재 바이트를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in report['sha256'].items():  # 원본/관찰/제품/검사를 전수 비교한다.
        if digest(ROOT/name)!=value:raise RuntimeError('SHA 불일치 '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==report['total'] and len({tuple(row[:2]) for row in rows})==len(rows) and dict(Counter(row[0] for row in rows))==report['scopes'] and report['normal_returns']==len(rows)*2 and report['os_calls']==0
    print('바리케이드 사건',len(rows),'개 SHA/반환/고유 입력 감사 통과')


# 명시 명령에서만 보호 PE를 읽으며 게임/OS/창 진입은 없다.
if __name__=='__main__':verify() if '--verify' in sys.argv else generate()
