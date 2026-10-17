#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본427340 요청/428570 수신과40a850 헤더를 격리 실행하여 독립 기대값을 기록한다."""
import argparse,collections,itertools,json,socket,struct,zlib
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,TYPES,digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as PreviousOracle,POOL
from decomp_nugget_regular import bits
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP

FIXTURE=ROOT/'cpppj/tests/fixtures/carrier-move-x86.tsv' #기존 원문과 분리한 요청/수신 결과다.
REPORT=ROOT/'cpppj/carrier-move.json' #현재 보호PE/도구/원문 해시와 실제 정상반환이다.
ENTRIES=(0x427340,0x428570,0x40a850,0x4df230,0x4ac200,0x49a840,0x40eb80) #큰 하위 경계 이외 전체 실행할 몸체다.
PACKET,INPUT,THUNK=STOP+0x900,STOP+0x940,STOP+0x980 #고정 cdecl 호출자와 원본이 다시 읽는 입력이다.
PATH=0x14500000 #100080BYTE Path의100076BYTE 접두를 격리한 주소이며 호스트 포인터와 무관하다.


def cases():
    """관문/경로 중복/NaN/콜백 뒤 현재 상태와 패킷 재독을 독립 입력한다."""
    #op,genus,state,owner,sender,fort,selected,connected,path,candidate,return,geyser,altar,allowed,sacrifice,stop,move,mutation,x,y,oldx,oldy,protocol,message,length,startResult
    base=[0,0x10000,0,1,1,0,50,1,0,0,0,0,0,0,0,0,0,0,bits(22.5),bits(23.5),bits(22.5),bits(23.5),1,0x40,18,1];rows=[]
    #같은 목적지 억제와 부동소수 비교는 부호0/NaN/무한대도 원본에서 관찰한다.
    for genus,selected,connected,path,point in itertools.product((0,0x10000,0x20000,0x30000),(0,50,51),(0,1),(0,1),range(5)):
        r=base.copy();r[1]=genus;r[6:9]=selected,connected,path
        if point==1:r[20]=bits(23.5)
        if point==2:r[18]=r[20]=0x7fc01234
        if point==3:r[18]=0;r[20]=0x80000000
        if point==4:r[18]=r[20]=0x7f800000
        rows.append(r)
    #헤더의 실제 BYTE/WORD는 요청마다 읽으며 진단/선택/연결/경로 조회는 현재 타입을 바꿀 수 있다.
    for m in range(1,11):
        r=base.copy();r[17]=m;rows.append(r)
    for protocol,length in itertools.product((0,1,0x80,0xff),(0,3,4,18,0xffff)):
        r=base.copy();r[22]=protocol;r[24]=length;rows.append(r)
    #수신은 free/dead/genus/정지/전체DWORD 소유자와 fort모드의 순서대로 관문을 검사한다.
    for genus,state,owner,sender,fort,stop,move in itertools.product((0,0x10000,0x200000|0x10000),(0,1,2,3,4,8),(1,2),(1,0x101),(0,1),(0,1),(0,1)):
        r=base.copy();r[0]=1;r[1:6]=genus,state,owner,sender,fort;r[15:17]=stop,move;rows.append(r)
    #각 서버 후속 분기와 반환null을 교차한다. 경로/공간 결과는 독립 외부 입력이다.
    for candidate,place,geyser,altar,allowed,sacrifice,start in itertools.product((0,60),(0,61),(0,62),(0,63),(0,1),(0,1),(0,1)):
        r=base.copy();r[0]=1;r[1]=0x210000;r[9:15]=candidate,place,geyser,altar,allowed,sacrifice;r[25]=start;rows.append(r)
    #하위 효과가 caller소유 부모WORD/좌표/희생DWORD를 바꾼 뒤 재독하는지 검사한다.
    for m in range(11,21):
        r=base.copy();r[0]=1;r[1]=0x210000;r[17]=m;r[12:15]=63,1,1;rows.append(r)
    return list(dict.fromkeys(tuple(r) for r in rows))


class Oracle(BaseOracle):
    """보호 함수/ret ABI를 그대로 실행하고 큰 경로/조회/전송만 외부 효과로 기록한다."""
    def __init__(self):
        """단일 PE와 고정 호출 코드를 준비하고 쓰기/실행 범위를 제한한다."""
        super().__init__();self.mu.mem_map(POOL,0x10000);self.mu.mem_map(PATH,0x19000);self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter()
        code=b'\x68'+struct.pack('<I',PACKET)+b'\x6a\x00\xff\x35'+struct.pack('<I',INPUT)+b'\xe8'+struct.pack('<i',0x428570-(THUNK+18))+b'\x83\xc4\x0c\xc3'
        assert len(code)==22;self.mu.mem_write(THUNK,code);self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def mutate(self,where):
        """명시 입력으로 현재raw/TYPE/선택/헤더/packet을 바꾸며 결과는 계산하지 않는다."""
        m=self.case[17]
        if m==1 and where=='select':self.put(TYPES+133*500+0xec,0)
        if m==2 and where=='assert':self.put(TYPES+133*500+0xec,0x20000)
        if m==3 and where=='connected':self.put(POOL+50*50+10,134,1)
        if m==4 and where=='movement':self.put(0x52eded,0xff,1);self.put(0x52edf0,3,2)
        if m==5 and where=='path':self.put(PATH+0x186dc,self.case[18]);self.put(PATH+0x186e0,self.case[19])
        if m==6 and where=='select':self.put(POOL+50*50+14,bits(24.5))
        if m==7 and where=='select':self.put(TYPES+133*500+0xec,0x20000)
        if m==8 and where=='select':self.put(0x52edec,0x55,1)
        if m==9 and where=='assert':self.put(TYPES+133*500+0xec,0x10000)
        if m==10 and where=='connected':self.put(TYPES+133*500+0xec,0)
        if m==11 and where=='stop':self.put(POOL+50*50+34,2,1)
        if m==12 and where=='stop':self.put(0x594fb8,1)
        if m==13 and where=='move':self.put(PACKET+4,51,2);self.put(PACKET+6,bits(24.5))
        if m==14 and where=='candidate':self.put(PACKET+4,51,2);self.put(PACKET+10,bits(25.5))
        if m==15 and where=='return':self.put(PACKET+4,51,2);self.put(PACKET+6,bits(26.5))
        if m==16 and where=='genus':self.put(PACKET+4,51,2);self.put(PACKET+10,bits(27.5))
        if m==17 and where=='allowed':self.put(PACKET+14,0)
        if m==18 and where=='allowed':self.put(PACKET+4,51,2)
        if m==19 and where=='candidate':self.put(TYPES+133*500+0xec,0x10000)
        if m==20 and where=='return':self.put(PACKET+14,0)

    def instruction(self,mu,address,size,data):
        """원본 호출 인자와 효과 뒤 재독 순서를 기록한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);a=struct.unpack('<8I',mu.mem_read(sp+4,32));sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50;c=self.case
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address==0x4d5430:self.event('S');self.ret(c[6]);return
        if address==0x4d5c90:self.event('C',*a[:3]);self.mutate('select');self.ret();return
        if address==0x4e0620:self.event('I',a[2]);assert a[2]==0x571;self.mutate('assert');self.ret();return
        if address==0x488340:self.event('L',*a[:4]);self.mutate('connected');self.ret(c[7]);return
        if address==0x488fe0:self.event('M',*a[:3]);self.mutate('movement');self.ret(40);return
        if address==0x4b1a90:self.event('P',*a[:2]);self.mutate('path');self.ret(70 if c[8] else 0);return
        if address==0x4ae2d0:self.ret(PATH);return
        if address==0x4e2dc0:self.packet=bytes(mu.mem_read(a[1],18)).hex();self.events.append('T:'+str(a[0])+':'+self.packet);self.ret();return
        if address==0x4afb40:self.event('E',sid,a[0]);self.mutate('stop' if a[0]==0x5105 else 'move');self.ret(c[15] if a[0]==0x5105 else c[16],4);return
        if address==0x425ed0:self.event('E',a[0],0x493);self.mutate('move');self.ret(c[16]);return
        if address==0x426900:self.event('Q',sid,a[1],a[2]);self.mutate('candidate');self.put(a[0],c[9]);self.put(a[0]+4,0x1234);self.ret(a[0],12);return
        if address==0x4263a0:self.event('R',*a[:3]);self.mutate('return');self.ret(c[10]);return
        if address==0x4894e0:self.event('D',*a[:4]);self.ret(0);return
        if address==0x4b2070:self.event('G',*a[:3]);self.mutate('genus');self.ret(c[11] if a[2]==0x10000000 else c[12]);return
        if address==0x427600:self.event('Y',sid,a[0]);self.ret(PATH if c[25] else 0,4);return
        if address==0x4498e0:self.event('A',*a[:2]);self.mutate('allowed');self.ret(c[13]);return
        if address==0x494eb0:self.event('F',*a[:2]);self.ret();return
        if address==0x489080:self.event('B',*a);self.ret(PATH if c[25] else 0);return
        if THUNK<=address<THUNK+22:return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'이동 원본 실행 범위 밖 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """원본의 세 경로DWORD/스택/SEH만 쓰도록 검사한다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000 or address==0 and size==4 or size==4 and address in (PATH+0x186cc,PATH+0x186d0,PATH+0x186d8):return
        raise RuntimeError(f'이동 원본 쓰기 범위 밖 {address:08x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """명시 격리 자료와 보호 이미지/스택만 읽도록 검사한다."""
        spans=((0,4),(0x400000,0x600000),(POOL,POOL+0x10000),(TYPES,TYPES+0x20000),(PATH,PATH+0x19000),(STACK-0x7f000,STACK+0x1000),(STOP,STOP+0x1000))
        if not any(lo<=address and address+size<=hi for lo,hi in spans):raise RuntimeError(f'이동 원본 읽기 범위 밖 {address:08x}+{size}')

    def sample(self,c,precision):
        """입력만 초기화하고 전체 두 슬롯/Path와 요청 바이트/현재 전역을 관측한다."""
        self.case=c;self.events=[];self.packet='-';self.mu.mem_write(TYPES,bytes(0x20000));self.put(TYPES+133*500+0xec,c[1]);self.put(TYPES+134*500+0xec,0x20000)
        #50은 현재 수신 부모,51은 callback이 packet에서 바꾼 별도 부모다.
        for sid in (50,51):
            raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,0x12200000);raw[10]=133;raw[11]=c[2];raw[34]=c[3];struct.pack_into('<II',raw,14,bits(20.5),bits(21.5));self.mu.mem_write(POOL+sid*50,bytes(raw))
        self.mu.mem_write(PATH,b'\xa5'*0x186ec);self.put(PATH+0x186dc,c[20]);self.put(PATH+0x186e0,c[21])
        self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES);self.put(0x594fb8,c[5]);self.put(0x52edec,c[23],1);self.put(0x52eded,c[22],1);self.put(0x52edf0,c[24],2);self.put(INPUT,c[4]);self.put(0,0x87654321)
        self.mu.mem_write(PACKET,struct.pack('<BBH H III',c[22]&0x7f,c[23],c[24],50,c[18],c[19],c[14]));args=struct.pack('<III',c[18],c[19],c[14]) if c[0]==0 else b''
        PreviousOracle.invoke(self,0x427340 if c[0]==0 else THUNK,args,precision,False);self.returns+=1;assert self.get(0)==0x87654321
        state=','.join(map(str,(self.get(0x594fb8),self.get(TYPES+133*500+0xec),self.mu.mem_read(0x52eded,1)[0],self.mu.mem_read(0x52edec,1)[0],struct.unpack('<H',self.mu.mem_read(0x52edf0,2))[0])))
        raws=','.join(bytes(self.mu.mem_read(POOL+sid*50,50)).hex() for sid in (50,51));path=str(zlib.adler32(bytes(self.mu.mem_read(PATH,0x186ec))))
        return ';'.join(self.events) or '-',state,self.packet,raws,path


def main():
    """첫 기대값을 독립 실행하거나 기존 원문/SHA/입력순서를 읽기 전용으로 검증한다."""
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--verify',action='store_true');args=p.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'));rows=[l.split('\t') for l in FIXTURE.read_text('utf-8').splitlines() if l and not l.startswith('#')]
        #원문 결과를 다시 쓰지 않고 현재 입력/보고서 해시를 검증한다.
        for path,want in report['sha256'].items():assert digest(ROOT/path)==want,path
        assert [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()] and len(rows)==report['uniqueInputs'] and len(rows)*2==report['normalReturns'];assert all(r[1:6]==r[6:11] for r in rows);print('운반체 이동 원본 감사',len(rows),report['normalReturns']);return
    oracles=[Oracle(),Oracle()];rows=[]
    #C++ 결과를 읽지 않고53/64 정밀도에서 각각 정상 반환을 관측한다.
    for c in cases():
        outputs=[o.sample(c,p) for o,p in zip(oracles,(0x27f,0x37f))];assert outputs[0]==outputs[1];rows.append((','.join(map(str,c)),*outputs[0],*outputs[1]))
    FIXTURE.write_text('# input trace53 state53 packet53 raws53 pathAdler53 trace64 state64 packet64 raws64 pathAdler64\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_carrier_move.py','tools/decomp_carrier_falling.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/carrier-move-x86.tsv')
    report=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),precisionDifferentInputs=[],boundaries='427340/428570/40a850/4df230/4ac200/49a840/40eb80 전체 실행. 선택/연결/경로종류·종속조회/송신/사건/후보/지점/접근점·간헐천·희생·경로시작은명시큰하위경계. GP/retN/canary/SEH/x87 및실행·읽기·쓰기범위/전체부모50BYTE둘/100076BYTE경로접두Adler/현재전역/패킷/효과뒤재독을검사한다. 실제게임/사용자UI/자연완주증거가아니다.',sha256={s:digest(ROOT/s) for s in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('uniqueInputs','normalReturns','instructions','nativeCalls')},ensure_ascii=False))


if __name__=='__main__':main()
