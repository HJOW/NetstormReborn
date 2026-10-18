#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 플레어 생성/Regular의 실제 x86·난수·클램프·float 반환을 독립 관측한다."""
import argparse
import collections
import csv
import json
import socket
import struct
from pathlib import Path
from decomp_priestmenu import Oracle as BaseOracle, ROOT, POOL, TYPES, TEXT, STACK, STOP, digest, fbits
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

# 현재 정밀 함수표에서 선택한 실제 전체 몸체이며 게임/OS 진입은 없다.
ENTRIES=(0x45a250,0x45a480,0x45a6c0,0x455930,0x41d0d0,0x4ad710,0x4ac550,0x49a840,0x4ac200,0x4e49c0,0x496e70)
FIXTURE=ROOT/'cpppj/tests/fixtures/flare-x86.tsv' # C++ 구현 전에 동결할 원문이다.
REPORT=ROOT/'cpppj/flare.json' # 실행 범위/정상 반환/입력 해시를 기록한다.
IDS=(50,*range(100,110)) # 부모와 최대6개 초기 플레어의 실제 관측 슬롯이다.


def inputs():
    """출력 판단 없이 억제/개수/좌표/깊이/헤더/난수/사건/단계의 명시 입력을 만든다."""
    base=['Spawn',6,2,0,0,0,0,fbits(100.25),fbits(101.75),0,64,32,0,63]
    rows=[base.copy()]
    # 억제double은 비트로 입력하며 음수0/NaN과 실제 로딩 깊이를 구분한다.
    for column,values in ((1,(-1,0,1)),(3,(0x3ff0000000000000,0x8000000000000000,0x7ff8000000000042)),(4,(1,)),(5,(-1,29999,30000,30001)),(6,(0x30000,)),(7,(fbits(0),fbits(256),fbits(float('nan')))),(8,(fbits(0),fbits(256))), (9,(127,128,255)),(10,(-1,0)),(11,(-1,0)),(12,(1,0xffffffff,0xdeadbeef)),(13,(0,0x15))):
        # 한 입력 축만 바꿔 단락 평가와 개별 예약 실패를 관측한다.
        for value in values:
            row=base.copy();row[column]=value;rows.append(row)
    # 프레임 구간A/B/C와 사건의 재생성·소리·삭제 경계다.
    for state in (0,1,2):
        # event1은 후속을 만들지 않으며 event2/3은 별도 후속 사건을 예약한다.
        for event in (0,1,2,3,0xffffffff):
            # 음수count와 마지막 전/마지막/구간 끝/끝 뒤를 따로 공급한다.
            for count in (-1,0,3,6,7,8):rows.append(['Regular',state,event,count,0,fbits(-2.75),fbits(3.75),0,1,fbits(.2)])
    # 제한/할당 실패/난수 시드와 정수 좌표 변환의 독립 입력이다.
    for column,values in ((4,(-1,30000,30001)),(5,(fbits(0),fbits(255.9),fbits(2147483648),fbits(-2147483904),fbits(1e30),fbits(-1e30),fbits(float('nan')),fbits(float('inf')))), (6,(fbits(-.25),)),(7,(1,0xffffffff,0xdeadbeef)),(8,(0,)),(9,(0,0xbf800000,0x7fc12345))):
        # Regular 마지막 프레임의 분기만 바꾸어 재생성 조건을 관측한다.
        for value in values:
            row=['Regular',0,2,6,0,fbits(-2.75),fbits(3.75),0,1,fbits(.2)];row[column]=value;rows.append(row)
    return list(dict.fromkeys(tuple(row) for row in rows))


class Oracle(BaseOracle):
    """좌표/난수/필드·제어는 실제 명령이며 생성/Pop/프레임·수명/소리/예약만 명시 경계다."""
    def __init__(self):
        """보호 PE와 현재 함수 범위/기록용 float 반환 trampoline을 준비한다."""
        super().__init__();self.mu.mem_map(0,0x1000);self.allowed=[] # 원본 SEH와 이번 실행 범위다.
        self.hits=collections.Counter();self.boundaries=collections.Counter();self.returns=0 # 실제 명령/경계 호출/정상 반환 누계다.
        self.recent=collections.deque(maxlen=18) # 실패 시 실제 마지막 명령을 보존한다.
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        # 전체 정밀 범위의 불연속 구간도 빠짐없이 선택한다.
        for entry in ENTRIES:
            # 한 함수에 여러 원문 구간이 있을 수 있다.
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(word,16) for word in part.split('-'));self.allowed.append((low,high+1))
        self.mu.mem_write(STOP+0x100,b'\xd9\x1d'+struct.pack('<I',TEXT+0xff00)+b'\xe9'+struct.pack('<i',STOP-(STOP+0x10b)))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self,mu,access,address,size,value,data):
        """실제 명령은 선택 슬롯/게임 난수/SEH/스택/반환 float 이외에 쓰지 못한다."""
        ranges=[(0,4),(STACK-4096,STACK+128),(TEXT+0xff00,TEXT+0xff04),(0x532710,0x532714)]+[(POOL+sid*50,POOL+(sid+1)*50) for sid in IDS]
        if not any(low<=address and address+size<=high for low,high in ranges):raise RuntimeError('플레어 미검토 쓰기 '+hex(address)+'/'+str(size))

    def event(self,label,*values):
        """호출 순서와 DWORD 인자의 원래 비트를 기록한다."""
        self.events.append(label+(','+','.join(map(str,values)) if values else ''))

    def instruction(self,mu,address,size,data):
        """허용한 실제 몸체를 실행하고 외부 효과의 호출 인자/순서만 관측한다."""
        self.recent.append(hex(address))
        if address in (STOP+0x100,STOP+0x106):return
        if any(low<=address and address+size<=high for low,high in self.allowed):
            self.instructions+=1
            if address in ENTRIES:self.hits[hex(address)]+=1
            return
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,8);ecx=mu.reg_read(UC_X86_REG_ECX);sid=(ecx-POOL)//50;self.boundaries[hex(address)]+=1
        if address==0x471ab0:self.event('K',self.kernel);self.ret(self.kernel)
        elif address==0x4af530:
            self.created+=1;created=99+self.created;self.event('C',args[0],args[1],created);self.ret(POOL+created*50)
        elif address==0x419850:
            assert ecx==TYPES+85*500 and args[:2]==(0,0);self.event('H');self.ret(TEXT+0xc000,8)
        elif address==0x4ac220:self.event('O',sid,args[0]);self.put(ecx+34,args[0],1);self.ret(purge=4)
        elif address==0x4b02d0:
            self.event('P',sid,*args[:3]);self.put(ecx+14,args[0]);self.put(ecx+18,args[1]);self.ret(purge=12)
        elif address==0x4e4391:
            assert args[0]==40;success=(self.allocations>>self.reserved)&1;self.reserved+=1;self.event('R',success);self.ret(TEXT+0xe000 if success else 0)
        elif address==0x496e80:self.event('A',args[1],args[0],args[2]);self.ret(purge=12)
        elif address==0x4acee0:self.event('F',sid,*args[:2]);self.put(ecx+36,args[0]);self.ret(purge=8)
        elif address==0x4ad500:self.event('U',sid);self.ret()
        elif address==0x4af780:self.event('D',sid,args[0]);self.ret(purge=4)
        elif address==0x4a9d70:
            assert self.string(args[2])==b'powerup.wav';self.event('S',args[0],args[1],args[3]);self.ret()
        else:raise RuntimeError('플레어 미검토 실제 명령 '+hex(address))

    def run(self,row,control):
        """명시 입력을 공급하고 실제 풀/난수/효과/float 반환/정상 호출 규약을 관측한다."""
        self.events=[];self.created=0;self.reserved=0 # 이번 호출의 실제 경계 누계다.
        self.mu.mem_write(POOL,bytes(0x10000));self.mu.mem_write(TYPES,bytes(188*500));self.put(0,0x12345678);self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES);self.put(0x541200,106)
        # 생성 경계가 반환하는 서로 다른 실제 슬롯의 보존 패턴이다.
        for sid in IDS:
            raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,0x5096a0);raw[10]=106;raw[11]=0;raw[34]=8;raw[35]=99;raw[40]=32;struct.pack_into('<I',raw,36,77);self.mu.mem_write(POOL+sid*50,bytes(raw))
        # 실제 방향 getter가 읽는16구간과3×7개 코드를 공급한다.
        for state in range(16):
            self.put(TYPES+106*500+0x130+state*4,state*7);self.put(TYPES+106*500+0x170+state*4,7)
        self.put(TYPES+106*500+0x124,TEXT);self.mu.mem_write(TEXT,b''.join(bytes((65+i//7,65,i,0)) for i in range(112)))
        if row[0]=='Spawn':
            count,event,suppressed,loading,self.kernel,genus,x,y,depth,hotspot_x,hotspot_y,seed,self.allocations=row[1:]
            self.mu.mem_write(0x5c85f0,struct.pack('<Q',suppressed));self.put(0x5c89b8,loading);self.put(TYPES+85*500+0xec,genus);self.put(POOL+50*50+10,85,1);self.put(POOL+50*50+14,x);self.put(POOL+50*50+18,y);self.put(POOL+50*50+35,depth,1);self.put(POOL+50*50+36,0)
            self.put(TEXT+0xc01c,hotspot_x,2);self.put(TEXT+0xc01e,hotspot_y,2);address=0x45a6c0;args=(50,count&0xffffffff,event);ecx=0;purge=0;stop=STOP
        else:
            state,event,count,self.kernel,x,y,seed,self.allocations,payload=row[1:];self.put(POOL+50*50+14,x);self.put(POOL+50*50+18,y);self.put(POOL+50*50+36,state*7)
            address=0x45a250;args=(event,count&0xffffffff,payload);ecx=POOL+50*50;purge=12;stop=STOP+0x100
        self.put(0x532710,seed);self.put(TEXT+0xff00,0xdeadbeef)
        saved={reg:0x12340000+reg for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)} # 보존 레지스터의 독립 입력이다.
        # 매 호출에서 비휘발 레지스터를 새로 공급한다.
        for reg,value in saved.items():self.mu.reg_write(reg,value)
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_ECX,ecx);self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.mem_write(STACK,struct.pack('<4I',stop,*args));types=bytes(self.mu.mem_read(TYPES,188*500))
        try:self.mu.emu_start(address,STOP,count=1000000)
        except Exception:print('최근 실제 명령',list(self.recent),flush=True);raise
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge and self.words(0,1)[0]==0x12345678
        assert all(self.mu.reg_read(reg)==value for reg,value in saved.items()) and self.mu.reg_read(UC_X86_REG_FPCW)==control and self.mu.reg_read(UC_X86_REG_FPTAG)==0xffff
        assert types==bytes(self.mu.mem_read(TYPES,188*500));self.returns+=1
        return [self.words(TEXT+0xff00,1)[0] if row[0]=='Regular' else 0,self.words(0x532710,1)[0],b''.join(bytes(self.mu.mem_read(POOL+sid*50,50)) for sid in IDS).hex(),';'.join(self.events) or '-']


def generate():
    """C++ 기대값을 계산하지 않고 독립 원본 반환과 전체 슬롯/효과를 보존한다."""
    oracle=Oracle();rows=[]
    # 같은 입력을 두 제어어로 각각 초기화한 뒤 실제 원문을 실행한다.
    for row in inputs():
        first=oracle.run(row,0x27f);second=oracle.run(row,0x37f);assert first==second,row;rows.append([row[0],','.join(map(str,row[1:])),*map(str,first)])
    FIXTURE.write_text('# 구분 입력 반환float 난수 전체슬롯 호출순서\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),ROOT/'tools/decomp_priestmenu.py',ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',ROOT/'extracted/refined/originals/Netstorm.c',FIXTURE]
    report=dict(host=socket.gethostname(),date='2026-10-18',unit=173,rows=len(rows),counts=dict(collections.Counter(row[0] for row in rows)),normalReturns=oracle.returns,instructions=oracle.instructions,controls=[0x27f,0x37f],precisionDifferences=0,nativeCalls=dict(oracle.hits),boundaries=dict(oracle.boundaries),sha256={path.relative_to(ROOT).as_posix():digest(path) for path in paths},limits=['실제 생성/Regular/난수/좌표 클램프/프레임 방향·정수 변환을 실행한다. 생성·프레임 지정/Pop·소유자/수명/소리/Kernel 개수·예약은 명시 경계다.','게임/OS/원본 파일 변경·게임 창 실행은 없다. CD10.72와 전체 게임 화면 동일성의 증거는 아니다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('플레어 원본 관측',len(rows),oracle.returns,oracle.instructions)


def verify():
    """고유 입력과 독립 원본/관측의 현재 바이트만 검사하고 기대값을 재생성하지 않는다."""
    r=json.loads(REPORT.read_text('utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
    # 모든 원문 입력과 fixture를 각각 해시 대조한다.
    for path,wanted in r['sha256'].items():assert digest(ROOT/path)==wanted,path
    assert len(rows)==r['rows']==len(inputs()) and r['normalReturns']==len(rows)*2 and not r['precisionDifferences']
    # 실제 입력 목록은 fixture의 열과 일치해야 한다.
    for row,case in zip(rows,inputs()):assert row[:2]==[case[0],','.join(map(str,case[1:]))] and len(row)==6
    print('플레어 원본/SHA/입력 감사',len(rows))


# 명시 호출만 새 관측 또는 기존 원문 검사를 실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
