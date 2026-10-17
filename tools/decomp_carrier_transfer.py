#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의42a0d0/42ac60/426830/40acf0을 독립 실행해 너겟 전달 순서를 관측한다."""
import argparse,collections,itertools,json,socket,struct
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,TYPES,VTABLE,digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as PreviousOracle,POOL
from decomp_nugget_regular import bits
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP

FIXTURE=ROOT/'cpppj/tests/fixtures/carrier-transfer-x86.tsv' # 기존 fixture와 구분한 새 독립 원문이다.
REPORT=ROOT/'cpppj/carrier-transfer.json' # 정상 반환/현재 전역/명령과 외부 경계를 기록한다.
ENTRIES=(0x42a0d0,0x42ac60,0x426830,0x40acf0,0x4df230) # 전체 실행을 허용하는 다섯 실제 몸체다.
DETACH,ATTACH=STOP+0x800,STOP+0x820 # Form의 실제 가상+48/+50 외부 효과다.
PACKET,INPUT,THUNK=STOP+0x900,STOP+0x940,STOP+0x980 # 고정 cdecl 호출 코드와 명시 입력이다.


def cases():
    """권한/내용물/공유 타이머/원본 free 비트/콜백 뒤 현재 조회를 교차한다."""
    base=[0,1,0,0,0,0,1,1,154,0,0,1,1,187,0x8000,0,0,11,1,0x60,10,1,187,154];rows=[]
    # 조기 종료와 signed cooldown을 따로 입력한다.
    for boss,editor,own,cooldown,mode in itertools.product((0,1),(0,1),(0,52),(0,1,2,5,0x7fffffff,0x80000000,0xffffffff),range(3)):
        row=base.copy();row[1:5]=boss,editor,own,cooldown;row[6]=mode;rows.append(row)
    # 실제 성공/불일치와 효과 후 타입/상태 재조회다.
    for exists,kind,owner,typ,flags,target,mutation in itertools.product((0,1),(154,155),(1,2),(187,133),(0,0x8000),(0,70),range(11)):
        row=base.copy();row[7:9]=exists,kind;row[11]=owner;row[13:17]=typ,flags,target,mutation;rows.append(row)
    # 전달 수신은 dead 비트로 차단하지 않고 free 비트만 검사한다.
    for parent,content,mutation in itertools.product((0,1,2,3),(0,1,2,3),range(4,7)):
        row=base.copy();row[0]=2;row[10]=parent;row[9]=content;row[16]=mutation;rows.append(row)
    # 공유 카운터는 inc 감김 뒤 signed 나머지이며 두 부모가 같은 값을 쓴다.
    for counter,boss,reply in itertools.product((0,1,2,3,4,0x7ffffffe,0x7fffffff,0x80000000,0xfffffffc,0xfffffffd,0xfffffffe,0xffffffff),(0,1),(0,10,11,12,0xffffffff)):
        row=base.copy();row[0]=1;row[1]=boss;row[5]=counter;row[17]=reply;rows.append(row)
    # 공통 헤더 생성자는 WORD 길이를4 이상으로 보정하지 않는다.
    for protocol,message,length in itertools.product((0,1,0x80,0xff),(0,0x60,0xff),(0,1,3,4,10,0xffff)):
        row=base.copy();row[18:21]=protocol,message,length;rows.append(row)
    return list(dict.fromkeys(tuple(r) for r in rows))


class Oracle(BaseOracle):
    """큰 finder/경제/경로 효과만 분리하며 로컬 명령 수신은 원본을 직접 실행한다."""
    def __init__(self):
        """현재 부모 영역과 고정 호출 코드를 한 번만 설치한다."""
        super().__init__();self.mu.mem_map(POOL,0x10000);self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter();self.packet='-'
        self.put(VTABLE+0x48,DETACH);self.put(VTABLE+0x50,ATTACH)
        # cdecl 세 인자를 고정 입력에서 읽고 caller가 스택을 정리한다. 입력마다 코드를 바꾸지 않는다.
        code=b'\x68'+struct.pack('<I',PACKET)+b'\x6a\x00\xff\x35'+struct.pack('<I',INPUT)+b'\xe8'+struct.pack('<i',0x426830-(THUNK+18))+b'\x83\xc4\x0c\xc3'
        assert len(code)==22;self.mu.mem_write(THUNK,code);self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def mutate(self,where):
        """명시 외부 효과가 바꿀 수 있는 현재 raw/타입/전역만 변경한다."""
        m=self.case[16]
        if m==1 and where=='find-parent':self.put(0x5c85a4,1)
        if m==2 and where=='find-parent':self.put(0x545424,1)
        if m==3 and where=='find-donor':self.put(POOL+52*50+18,155,2);self.put(0x5412c0,155)
        if m==4 and where=='detach':self.put(POOL+52*50+11,1,1);self.put(POOL+50*50+11,1,1)
        if m==5 and where=='attach':self.put(POOL+60*50+10,133,1)
        if m==6 and where=='preview':self.put(POOL+52*50+18,155,2)
        if m==7 and where=='broadcast':self.put(POOL+60*50+10,133,1);self.put(POOL+52*50+18,155,2)
        if m==8 and where=='remove':self.put(POOL+52*50+18,155,2)
        if m==9 and where=='choose':self.put(0x540bc4,0);self.put(0x545424,9)
        if m==10 and where=='go':self.put(POOL+52*50+18,155,2)

    def instruction(self,mu,address,size,data):
        """실제 수신/헤더 본문 외의 가상/검색/이동 효과를 원래 인자로 기록한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);a=struct.unpack('<4I',mu.mem_read(sp+4,16));this=mu.reg_read(UC_X86_REG_ECX);sid=(this-POOL)//50;c=self.case
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address==0x4ac9f0:
            self.event('F',sid,*a[:3]);self.mutate('find-parent' if sid==50 else 'find-donor');self.ret(c[3] if sid==50 else 52 if sid==60 and c[7] else 0,12);return
        if address==0x4202f0:
            self.cursor=this;self.index=0;self.mutate('begin');self.put(this+0x34,self.candidates[0]);self.ret(this,12);return
        if address==0x4b1810:self.index+=1;self.put(this+0x34,self.candidates[self.index] if self.index<len(self.candidates) else 0);self.ret();return
        if address in (DETACH,ATTACH):
            self.event('D' if address==DETACH else 'A',sid,*a[:1 if address==DETACH else 2]);self.mutate('detach' if address==DETACH else 'attach');self.ret(0,4 if address==DETACH else 8);return
        if address==0x426080:self.event('P',a[0]);self.mutate('preview');self.ret();return
        if address==0x4e2e30:self.packet=bytes(mu.mem_read(a[0],10)).hex();self.events.append('S:'+self.packet);self.mutate('broadcast');self.ret();return
        if address==0x488fe0:self.event('M',*a[:3]);self.ret(40);return
        if address==0x4b1a90:self.event('X',*a);self.mutate('remove');self.ret();return
        if address==0x429420:self.event('G',sid);self.mutate('choose');self.ret(c[15]);return
        if address==0x427600:self.event('V',sid,a[0]);self.mutate('go');self.ret(1,4);return
        if address==0x428fd0:self.event('H',sid);self.ret(1);return
        if address==0x49a840:self.ret(TYPES+a[0]*0x1ec);return
        if address==0x42a5f0:self.event('C',sid);self.ret(c[17]);return
        if THUNK<=address<THUNK+22:return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'전달 범위 밖 실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """공유 세 DWORD와 원본 스택/FS만 실제 몸체에서 쓸 수 있다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000 or address==0 and size==4 or address in (0x545420,0x545424,0x54542c) and size==4:return
        raise RuntimeError(f'전달 범위 밖 쓰기 {address:08x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """보호 이미지와 격리 입력/가상 표/TYPE/현재 스택 범위를 감사한다."""
        spans=((0,4),(0x400000,0x600000),(POOL,POOL+0x10000),(TYPES,TYPES+0x20000),(VTABLE,VTABLE+0x1000),(STACK-0x7f000,STACK+0x1000),(STOP,STOP+0x1000))
        if not any(lo<=address and address+size<=hi for lo,hi in spans):raise RuntimeError(f'전달 범위 밖 읽기 {address:08x}+{size}')

    def sample(self,c,precision):
        """분기 결과를 주입하지 않고 현재 입력과 실제 최종 raw/packet을 기록한다."""
        self.case=c;self.events=[];self.packet='-';self.candidates=([50],[60,50],[61,60,50])[c[6]]
        # 실제 물리 슬롯과 두 TYPE genus는 입력이며 반환/호출 순서는 계산하지 않는다.
        for sid,typ,owner in ((50,187,c[12]),(52,6,0),(60,c[13],c[11]),(61,133,2)):
            raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=typ;raw[11]=c[9] if sid==52 else c[10] if sid==50 else 0;raw[34]=owner;raw[40]=0;struct.pack_into('<II',raw,14,bits(20.75),bits(21.5));self.mu.mem_write(POOL+sid*50,bytes(raw))
        self.put(POOL+52*50+18,c[8],2)
        # C++는 실제 일반 finder를 사용하므로 원문 stub의 같은 물리 next 입력도 기록한다.
        for i,sid in enumerate(self.candidates):self.put(POOL+sid*50+4,self.candidates[i+1] if i+1<len(self.candidates) else 0,2)
        self.put(TYPES+154*0x1ec+0xec,c[14]);self.put(TYPES+155*0x1ec+0xec,c[14]^0x8000)
        for a,v in ((0x5c8464,POOL),(0x5c847c,24000),(0x540bc4,c[1]),(0x5c85a4,c[2]),(0x545424,c[4]),(0x54542c,c[5]),(0x545420,77),(0x541344,c[22]),(0x5412c0,c[23]),(0x540c70,c[21])):self.put(a,v)
        self.put(0x52ee3c,c[19],1);self.put(0x52ee3d,c[18],1);self.put(0x52ee40,c[20],2);self.put(INPUT,c[21]);self.mu.mem_write(PACKET,struct.pack('<BB4H',c[18]&0x7f,c[19],c[20],50,52,60));self.put(0,0x87654321)
        PreviousOracle.invoke(self,(0x42a0d0,0x42ac60,THUNK)[c[0]],b'',precision,False);self.returns+=1;assert self.get(0)==0x87654321
        result=0 if c[0]==2 else self.mu.reg_read(UC_X86_REG_EAX)
        state=(result,self.get(0x54542c),self.get(0x545424),self.get(0x545420),self.get(0x540bc4),self.get(0x5c85a4),self.get(0x5412c0))
        return ';'.join(self.events) or '-',','.join(map(str,state)),','.join(bytes(self.mu.mem_read(POOL+sid*50,50)).hex() for sid in (50,52,60,61)),self.packet


def main():
    """고유 입력과 두 정밀도 전체 원문 및 현재 도구/보호 SHA를 기록하거나 감사한다."""
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--verify',action='store_true');args=p.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'));rows=[l.split('\t') for l in FIXTURE.read_text('utf-8').splitlines() if l and not l.startswith('#')]
        # 이전 fixture를 생성기로 덮어쓰지 않고 바이트와 입력 순서를 각각 검사한다.
        for path,want in report['sha256'].items():assert digest(ROOT/path)==want,path
        assert [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()] and len(rows)*2==report['normalReturns'];assert [i for i,r in enumerate(rows) if r[1:5]!=r[5:9]]==report['precisionDifferentInputs'];print('캐리어 전달 원본 감사:',len(rows),'입력/',report['normalReturns'],'정상 반환');return
    oracles=[Oracle(),Oracle()];rows=[];different=[]
    # 두 정밀도에서 독립 실행하며 제품 계산을 기대값 생성에 쓰지 않는다.
    for index,c in enumerate(cases()):
        outputs=[o.sample(c,p) for o,p in zip(oracles,(0x27f,0x37f))]
        if outputs[0]!=outputs[1]:different.append(index)
        rows.append((','.join(map(str,c)),*outputs[0],*outputs[1]))
    FIXTURE.write_text('# input effects53 state53 raws53 packet53 effects64 state64 raws64 packet64\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_carrier_transfer.py','tools/decomp_carrier_falling.py','tools/decomp_cursor_lifecycle.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/carrier-transfer-x86.tsv')
    report=dict(host=socket.gethostname(),date='2026-10-17',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),precisionDifferentInputs=different,boundaries='42a0d0/42ac60/426830/40acf0/4df230 전체. 일반 finder/내용물 조회/가상 detach·attach/426080 예약/송신/타입 getter/경로 선택·삭제·재개/귀환은 명시 효과 경계다. C++에서는 실제 일반 finder를 재사용한다. GP/retN/SEH/canary/x87/코드·읽기·쓰기 범위를 검사했다. 전체 경제/자연 채취·완주/게임 창 실행 검증이 아니다.',sha256={s:digest(ROOT/s) for s in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('uniqueInputs','normalReturns','instructions','nativeCalls','precisionDifferentInputs')},ensure_ascii=False))


if __name__=='__main__':main()
