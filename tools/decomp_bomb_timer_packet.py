#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의 상태 타이머 생성/벡터/송수신/예약 부모를 실제 명령으로 독립 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_arc_spire import Oracle as BaseOracle, ROOT, STOP, HEAP, TYPES, VTABLE, STACK
from decomp_cursor_position import ranges
from decomp_particle_gradient import digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

POOL = 0x15000000  # 실제 파일과 분리한50BYTE 물리 풀이다.
FIELDS, MEMBERS, INPUT, LIST, PACKET = (HEAP+x for x in (0x100, 0x500, 0x1000, 0x1800, 0x2000))  # 객체/100칸/명시목록/목록헤더/패킷이다.
FACTORY = STOP+0x720  # TYPE+1d0의 큰 새 객체 경계다.
ROOTS = (0x420a40, 0x420920, 0x41e280, 0x41e230, 0x41e320, 0x41e0a0, 0x40b3d0, 0x4df590)  # 제어·벡터·헤더는 실제 명령을 실행한다.
FIXTURE = ROOT/'cpppj/tests/fixtures/bomb-timer-packet-x86.tsv'  # 새 독립 원문이다.
REPORT = ROOT/'cpppj/bomb-timer-packet-20261023.json'  # 호스트/명령/정밀도/해시 근거다.


def cases():
    """원본 기대값을 계산하지 않고 실제 경계 양쪽의 입력을 교차한다."""
    rows=[]
    for count,delay,now,flags in itertools.product((0,1,3,99,100,101,105),(-2147483648,-1,0,1,2147483647),(0,1,2),(1,2)):
        rows.append(('C',count,delay,now,flags))  # 초기100개 한도와 signed 지연을 관측한다.
    for count,length,protocol in itertools.product((0,1,3,100,105),(0,2,22,65535),(1,255)):
        rows.append(('W',count,length,protocol,0))  # 선언 길이/실제 반환 길이/상위 프로토콜 비트를 구별한다.
    for capacity,start,count,repeat,nan in itertools.product((0,1,100),(0,1,3,99,100),(0,1,3,99,100,101),(0,1),(0,1)):
        if start<=capacity:rows.append(('R',capacity,start,count,repeat,nan))  # 재수신에서도 기존 목록을 지우지 않는지 본다.
    for state,boss,mutation in itertools.product(range(8),(0,1),range(4)):
        rows.append(('A',state,boss,mutation,0))  # 새 Form은 free1만 검사하며 큰 호출 뒤 현재 패킷/종류를 읽는다.
    for listing,genus,state in itertools.product(range(4),range(3),range(8)):
        rows.append(('P',listing,genus,state,0))  # 부모 선택에 상태 필터가 없음을 관측한다.
    return rows


def items(count):
    """WORD를 넘는 SID·0·중복을 포함하는 명시 DWORD 입력이다."""
    return tuple((60,127,60,0,65535,0x12345678)[i%6] for i in range(count))


class Oracle(BaseOracle):
    """전체 원문 root를 실행하고 힙/진단/TYPE 조회/factory/부착만 큰 경계로 둔다."""
    def __init__(self):
        """검토한 코드/명시 자료만 읽고 쓰도록 격리한다."""
        super().__init__();self.mu.mem_map(POOL,0x200000);self.allowed=[];ranges(self,ROOTS);self.returns=0

    def number(self,address,width=4):
        """현재 비정렬 필드의 정확한 바이트 폭을 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def read(self,mu,access,address,size,value,data):
        """SEH/원본/격리 자료와 스택 밖 읽기를 거부한다."""
        if any(lo<=address and address+size<=hi for lo,hi in ((0,0x1000),(0x400000,0x600000),(POOL,POOL+0x200000),(HEAP,HEAP+0x10000),(TYPES,TYPES+0x20000),(VTABLE,VTABLE+0x1000),(STACK-0x7f000,STACK+0x1000))):return
        raise RuntimeError(f'타이머 패킷 읽기 범위 {address:x}+{size}')

    def write(self,mu,access,address,size,value,data):
        """원본의 객체/패킷/격리SEH/스택 쓰기만 허용한다."""
        if any(lo<=address and address+size<=hi for lo,hi in ((0,0x1000),(HEAP,HEAP+0x10000),(STACK-0x7f000,STACK+0x1000))):return
        raise RuntimeError(f'타이머 패킷 쓰기 범위 {address:x}+{size}')

    def instruction(self,mu,address,size,data):
        """큰 효과만 기록하며 벡터/헤더/상태·목록 순회는 실제 명령을 실행한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);args=[self.number(sp+4+i*4) for i in range(5)]
        if address in ROOTS:self.calls[f'{address:08x}']+=1
        if address==0x4e4391:self.event('N',args[0]);self.ret(MEMBERS);return
        if address==0x4e0620:
            self.event('D',args[2])
            if args[2]==0xcc and self.mutation==1:self.put(POOL+50*50+11,1,1)
            self.ret();return
        if address==0x49a840:self.event('T',args[0]);self.ret(TYPES);return
        if address==FACTORY:
            self.event('C',*args[:2]);self.put(FIELDS+12,MEMBERS);self.put(FIELDS+16,100);self.put(FIELDS+20,0)
            if self.mutation==2:self.put(0x541154,58)
            self.ret(FIELDS);return
        if address==0x41e230 and self.mode=='A' and self.mutation==3:self.put(PACKET+4,51,2);self.put(PACKET+6,70,2)
        if address==0x41bd20:self.event('A',*args[:4]);self.ret(purge=16);return
        if address==0x4ac200:self.ret((0,0x40000000,0)[self.number(ecx+10,1)-130]);return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'타이머 패킷 미검토 명령 {address:x}')

    def sample(self,row,control):
        """명시 입력만 설치하고 전체 출력·원문 필드·순서 있는 호출을 반환한다."""
        self.mode,*a=row;self.events=[];self.mutation=a[2] if self.mode=='A' else 0
        self.mu.mem_write(FIELDS,b'\xa5'*40);self.put(FIELDS,0x50330c);self.put(FIELDS+4,0x12340032);self.put(FIELDS+8,0xabcd003c);self.put(FIELDS+12,MEMBERS);self.put(FIELDS+16,100);self.put(FIELDS+20,0);self.put(FIELDS+32,0x80000003)
        self.mu.mem_write(FIELDS+24,struct.pack('<Q',0x7ff8000000001234));self.mu.mem_write(PACKET,b'\xa5'*256);self.mu.mem_write(MEMBERS,b'\xa5'*440)
        self.put(PACKET+4,50,2);self.put(PACKET+6,60,2);self.put(PACKET+16,3,2);self.put(PACKET+18,2);self.mu.mem_write(PACKET+8,struct.pack('<d',123.25))
        for i,item in enumerate(items(105)):
            self.put(INPUT+4*i,item);self.put(PACKET+22+2*i,item,2)  # 변환 전에 DWORD 목록과 별도WORD 패킷을 만든다.
        self.put(LIST,INPUT);self.put(LIST+4,105);self.put(LIST+8,105)
        self.put(0x5c8464,POOL);self.put(0x541154,57);self.put(0x540bc4,0);self.put(TYPES+0x1d0,FACTORY)
        result=0
        if self.mode=='C':
            count,delay,now,flags=a;self.put(LIST+8,count);self.mu.mem_write(0x55b4d0,struct.pack('<d',(0.,100.25,1e12)[now]));self.call(0x420a40,FIELDS,struct.pack('<III',delay&0xffffffff,LIST,flags),12,control);self.returns+=1
        elif self.mode=='W':
            count,length,protocol,_=a;self.put(FIELDS+20,count)
            for i,item in enumerate(items(count)):self.put(MEMBERS+4*i,item)  # writer의 명시 현재 DWORD 목록이다.
            self.put(0x52ee9a,0x7d,1);self.put(0x52ee9b,protocol,1);self.put(0x544c98,length,2);self.call(0x41e280,FIELDS,struct.pack('<III',PACKET,1,1),12,control);result=self.mu.reg_read(UC_X86_REG_EAX);self.returns+=1
        elif self.mode=='R':
            capacity,start,count,repeat,nan=a;self.put(FIELDS+16,capacity);self.put(FIELDS+20,start);self.put(PACKET+16,count,2)
            if nan:self.mu.mem_write(PACKET+8,struct.pack('<Q',0x7ff8000000004321))
            for i in range(start):self.put(MEMBERS+4*i,900+i)  # 수신 전에 존재하는 목록을 명시 입력으로 둔다.
            for _ in range(1+repeat):self.call(0x41e230,FIELDS,struct.pack('<I',PACKET),4,control);self.returns+=1  # 같은 버퍼의 재수신을 그대로 실행한다.
        elif self.mode=='A':
            state,boss,_,_=a;self.put(POOL+50*50+11,state,1);self.put(0x540bc4,boss);self.call(0x41e320,0,struct.pack('<III',0,0,PACKET),0,control);self.returns+=1
        elif self.mode=='P':
            listing,genus,state,_=a;chosen=((),(60,),(60,127,1),(1,127,60))[listing];self.put(0x59a9d8,INPUT);self.put(0x59a9e0,len(chosen))
            for i,sid in enumerate(chosen):self.put(INPUT+4*i,sid)  # 순서/중복을 그대로 사용한다.
            for sid,kind in ((60,130),(127,131),(1,132)):
                self.put(POOL+sid*50+10,kind,1);self.put(POOL+sid*50+11,state,1)  # 상태값은 부모 후보에서 제외하지 않는다.
            if genus==1:self.put(POOL+60*50+10,131,1)
            if genus==2:self.put(POOL+127*50+10,130,1)
            self.call(0x41e0a0,0,b'',0,control);result=self.mu.reg_read(UC_X86_REG_EAX);self.returns+=1
        count=self.number(FIELDS+20);packed=','.join(map(str,(self.number(FIELDS+16),self.number(FIELDS+24,8),self.number(FIELDS+32),*(self.number(MEMBERS+i*4) for i in range(count)))))
        return str(result) if self.mode in ('W','P') else '-', ';'.join(self.events) or '-',packed,bytes(self.mu.mem_read(PACKET,256)).hex()


def main():
    """새 원문 생성과 기존 원문/SHA의 읽기 전용 검증을 분리한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in {**facts['sha256'],**facts.get('implementationSha256',{})}.items():assert digest(ROOT/path)==wanted,path  # C++ 결과로 원문을 재작성하지 않는다.
        assert len([s for s in FIXTURE.read_text(encoding='utf-8').splitlines() if s and not s.startswith('#')])==facts['uniqueInputs'];print('타이머 패킷 원문/SHA 확인');return
    first,second=Oracle(),Oracle();rows=[]
    for row in cases():
        a=first.sample(row,0x27f);b=second.sample(row,0x37f);assert a==b,row;rows.append((','.join(map(str,row)),*a))  # 두 x87정밀도 모두 정상 반환하고 결과가 같아야 한다.
    FIXTURE.write_text('# input result effects capacity,endBits,flags,members packet256\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_bomb_timer_packet.py','tools/decomp_arc_spire.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py',FIXTURE.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',uniqueInputs=len(rows),normalReturns=first.returns+second.returns,nativeInstructions=first.instructions+second.instructions,precisionDifferentInputs=[],nativeCalls=dict(first.calls+second.calls),sha256={p:digest(ROOT/p) for p in paths},limitations=['420a40/420920/41e280/41e230/41e320/41e0a0/40b3d0/4df590 전체 root를 실행한다. 힙/TYPE 조회/factory/진단/부착은 큰 경계다.','Session57 서버 부착/실제 네트워크 수신과117/118 전체 효과 및 전체 GUI 플레이 완료 근거는 아니다.','손상된 음수 목록 길이/범위 밖 SID 주소 감김과 운영체제 메모리 오류는 복원하지 않는다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('타이머 패킷:',len(rows),facts['normalReturns'],facts['nativeInstructions'])


if __name__=='__main__':main()
