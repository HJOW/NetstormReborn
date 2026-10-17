#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Early Mission 로그인에 필요한 원본44d290의 짧은State 보존/0확장을 관측한다."""
import argparse,collections,json,socket,struct
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as PreviousOracle,POOL
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP
FIXTURE=ROOT/'cpppj/tests/fixtures/player-state-expand-x86.tsv' #새독립원문이며기존State fixture는읽기만한다.
REPORT=ROOT/'cpppj/player-state-expand.json' #보호원본/현재관측기/원문 해시와범위다.
OLD,TEMP,OLD_BYTES,NEW_BYTES=POOL+0x4000,POOL+0x4100,POOL+0x5000,POOL+0x6000 #독립MemStream둘과실제입력/새할당이다.
ENTRIES=(0x44d290,0x44cce0) #실제선택/현재길이재독/복사/0채움과position/end 쓰기를실행한다.


class Oracle(BaseOracle):
    """기존DataManager lookup/할당/record이름교체만큰외부경계로둔다."""
    def __init__(self):
        """보호PE와독립record/버퍼를준비한다."""
        super().__init__();self.mu.mem_map(POOL,0x10000);self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter()

    def instruction(self,mu,address,size,data):
        """현재실제인자와할당/교체경계를관측한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);a=struct.unpack('<4I',mu.mem_read(sp+4,16));this=mu.reg_read(UC_X86_REG_ECX)
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address==0x44ce60:self.ret(OLD,4);return
        if address==0x4e4391:assert a[0]==0x3c;self.allocations+=1;self.ret(TEMP);return
        if address==0x44d070:
            assert this==TEMP and a[1]==28;self.mu.mem_write(TEMP,struct.pack('<4I',NEW_BYTES,0,0,28)+bytes(0x2c));self.ret(TEMP,8);return
        if address==0x44ce10:self.put(this+8,a[0]);self.ret(TEMP,4);return
        if address==0x44d160:self.ret(0,8);return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'State확장 원본실행범위밖 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """스택/SEH/새버퍼28BYTE와새stream pos/end 이외쓰기를거부한다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000 or address==0 and size==4 or NEW_BYTES<=address and address+size<=NEW_BYTES+28 or size==4 and address in (TEMP+4,TEMP+8):return
        raise RuntimeError(f'State확장 원본쓰기범위밖 {address:08x}+{size}')

    def sample(self,length,pattern,precision):
        """명시원문만입력하고현재record/실제버퍼를읽는다."""
        raw=bytes((i*17+pattern)&255 for i in range(64));self.mu.mem_write(OLD_BYTES,raw);self.mu.mem_write(NEW_BYTES,b'\xa5'*64);self.mu.mem_write(OLD,struct.pack('<4I',OLD_BYTES,0,length,64));self.put(POOL+50*50+8,OLD);self.put(0,0x87654321);self.allocations=0
        PreviousOracle.invoke(self,0x44d290,struct.pack('<II',0x5147ac,28),precision,False);self.returns+=1;assert self.get(0)==0x87654321
        record=self.get(POOL+50*50+8);assert self.mu.reg_read(UC_X86_REG_EAX)==record;fields=struct.unpack('<4I',self.mu.mem_read(record,16));return str(self.allocations),str(fields[1]),str(fields[2]),bytes(self.mu.mem_read(fields[0],28)).hex()


def cases():
    """필요한원문길이와미사용바이트를독립입력한다."""
    return [(n,p) for n in range(1,34) for p in (0,1,0xa5)]


def main():
    """새원문을독립기록하거나기존입력전수/현재해시를검증한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text('utf-8').splitlines() if r and not r.startswith('#')]
        #기대값은변경하지않고현재원문/입력순서만검사한다.
        for path,want in report['sha256'].items():assert digest(ROOT/path)==want,path
        assert [(int(r[0]),int(r[1])) for r in rows]==cases() and len(rows)==report['uniqueInputs'] and report['normalReturns']==len(rows)*2;assert all(r[2:6]==r[6:10] for r in rows);print('State확장 원본감사',len(rows),report['normalReturns']);return
    oracles=[Oracle(),Oracle()];rows=[]
    #C++결과없이두x87정밀도에서원본의정상반환만관측한다.
    for n,p in cases():
        outputs=[o.sample(n,p,cw) for o,cw in zip(oracles,(0x27f,0x37f))];assert outputs[0]==outputs[1];rows.append((str(n),str(p),*outputs[0],*outputs[1]))
    FIXTURE.write_text('# length pattern alloc53 pos53 end53 bytes53 alloc64 pos64 end64 bytes64\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_player_state_expand.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_carrier_falling.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/player-state-expand-x86.tsv')
    report=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),precisionDifferentInputs=[],boundaries='44d290/44cce0 전체실행. lookup/record·28BYTE 할당·ctor/현재record교체·이름교체는명시큰하위경계다. 복사/0채움/현재pos·end와정상retN/GP/canary/SEH/x87/실행·쓰기범위를검사했다. 없는State 초기화/파일저장/전체로그인 증거가아니다.',sha256={p:digest(ROOT/p) for p in paths});REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('uniqueInputs','normalReturns','instructions','nativeCalls')},ensure_ascii=False))


if __name__=='__main__':main()
