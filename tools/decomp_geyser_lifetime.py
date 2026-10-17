#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE 간헐천485580/485540과 실제40ea00 목록 압축을 독립 실행한다."""
import argparse,collections,itertools,json,socket,struct
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as PreviousOracle,POOL
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP
DATA=0x14500000 # 실제 목록의8DWORD 물리 배열과 앞뒤 canary 페이지다.
F,U=STOP+0x800,STOP+0x820 # 기본 프레임 getter와 가상 표시 갱신의 명시 경계다.
ENTRIES=(0x485580,0x485540,0x40ea00) # 두 전체 고유 수명과 실제 중복 제거 helper다.
FIXTURE=ROOT/'cpppj/tests/fixtures/geyser-lifetime-x86.tsv' # 수량/통지 원문과 구별하는 고유 수명 원문이다.
REPORT=ROOT/'cpppj/geyser-lifetime.json' # 명령/정밀도/전체 raw/목록/현재 전역/PE 가상 표의 출처다.
PATTERNS=((),(50,77,50,0,50,88),(77,88,0,99)) # 중복과0을 포함한 명시 원래 목록 입력이다.


def cases():
    """현재extra/편집모드/프레임DWORD/목록/효과뒤변경/flags를 교차한다."""
    return list(itertools.product(range(2),(0,1,8,9,2,10),(0,1),(0,0xffffffff),range(3),range(7),(0x1000,0x12345678)))


class Oracle(BaseOracle):
    """가상getter·표시/AI/섬/큰Damageable만 경계이며 접두와 목록 쓰기는 원본이다."""
    def __init__(self):
        """원본 실행 범위와 실제 가상 표의 명시 두 효과를 설치한다."""
        super().__init__();self.mu.mem_map(POOL,0x10000);self.mu.mem_map(DATA-0x1000,0x2000);self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter()
        self.code_high={a:hi for lo,hi in self.allowed for a in range(lo,hi)} # 실제 명령 구간의 끝이다.
        self.vtable={hex(o):hex(self.get(0x50da88+o)) for o in (0x10,0x14,0x18,0x78,0x88)};self.put(0x50da88+0x78,F);self.put(0x50da88+0x88,U)
        self.boundaries={F,U,0x416720,0x44b4b0,0x44b9e0,0x46e6c0};self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def instruction(self,mu,address,size,data):
        """가상 효과 뒤의 현재extra/목록과 원본 직접 프레임 저장을 대체하지 않는다."""
        self.instructions+=1
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address not in self.boundaries:
            if address+size<=self.code_high.get(address,0):return
            raise RuntimeError(f'간헐천 수명 범위 밖 실행 {address:08x}')
        sp=mu.reg_read(UC_X86_REG_ESP);a=struct.unpack('<4I',mu.mem_read(sp+4,16));this=mu.reg_read(UC_X86_REG_ECX);sid=(this-POOL)//50;m=self.case[5];slot=POOL+50*50
        if address==F:
            self.event('F',sid)
            if m==1:self.put(slot+40,8,1)
            self.ret(self.case[3]);return
        if address==U:
            self.event('U',sid,self.get(slot+36))
            if m in (2,3):self.put(slot+40,8 if m==2 else 0,1)
            self.ret();return
        if address==0x416720:
            self.event('I',*a[:2])
            if m==4:self.mu.mem_write(DATA,struct.pack('<3I',50,77,50));self.put(0x5950e4,3);self.put(slot+40,8,1)
            if m==5:self.put(0x5c85a4,1);self.put(slot+36,12345)
            self.ret();return
        if address in (0x44b4b0,0x44b9e0):self.event('P' if address==0x44b4b0 else 'T',sid,a[0]);self.ret(0,4);return
        if address==0x46e6c0:
            self.event('D',*a)
            if m==6:self.put(slot+14,0x3fc00000);self.put(slot+18,0x40200000);self.put(slot+34,9,1);self.put(slot+40,9,1)
            self.ret();return

    def write(self,mu,access,address,size,value,data):
        """원본 직접 쓰기는 frameDWORD/현재count/8항목과stack만 허용한다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000 or address in (POOL+50*50+36,0x5950e4) and size==4 or DATA<=address and address+size<=DATA+32:return
        raise RuntimeError(f'간헐천 수명 범위 밖 쓰기 {address:08x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """보호 이미지와 현재 부모/목록/stack만 읽는다."""
        spans=((0x400000,0x600000),(POOL,POOL+0x10000),(DATA,DATA+32),(STACK-0x7f000,STACK+0x1000),(STOP,STOP+0x1000))
        if not any(lo<=address and address+size<=hi for lo,hi in spans):raise RuntimeError(f'간헐천 수명 범위 밖 읽기 {address:08x}+{size}')

    def sample(self,c,precision):
        """동일 입력을 설치하고 전체raw/전체8항목과 현재count/editor를 관측한다."""
        self.case=c;self.events=[];self.mu.mem_write(POOL,b'\xa5'*0x10000);slot=POOL+50*50;self.put(slot,0x50da88);self.put(slot+10,122,1);self.put(slot+11,2,1);self.put(slot+40,c[1],1);self.put(slot+34,3,1);self.put(slot+36,777);self.put(slot+14,0x41a20000);self.put(slot+18,0x41ac0000);self.put(0x5c8464,POOL);self.put(0x5c85a4,c[2]);self.put(0,0x87654321)
        items=list(PATTERNS[c[4]]);self.put(0x5950dc,DATA);self.put(0x5950e0,8);self.put(0x5950e4,len(items));self.mu.mem_write(DATA,struct.pack('<8I',*(items+[99]*(8-len(items)))));self.mu.mem_write(DATA-16,b'\x5a'*16);self.mu.mem_write(DATA+32,b'\x5a'*16)
        PreviousOracle.invoke(self,ENTRIES[c[0]],struct.pack('<I',c[6]),precision,False);self.returns+=1;assert self.get(0)==0x87654321 and bytes(self.mu.mem_read(DATA-16,16))==b'\x5a'*16 and bytes(self.mu.mem_read(DATA+32,16))==b'\x5a'*16
        return ';'.join(self.events),str(self.get(0x5950e4))+','+str(self.get(0x5c85a4)),bytes(self.mu.mem_read(slot,50)).hex(),','.join(map(str,struct.unpack('<8I',self.mu.mem_read(DATA,32))))


def main():
    """독립 원본 관측 또는 읽기 전용 SHA 감사를 선택한다."""
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--verify',action='store_true');args=p.parse_args()
    if args.verify:
        r=json.loads(REPORT.read_text('utf-8'));rows=[s.split('\t') for s in FIXTURE.read_text('utf-8').splitlines() if s and not s.startswith('#')]
        # 이미 관측한 기대값을 다시 만들지 않는다.
        for path,want in r['sha256'].items():assert digest(ROOT/path)==want,path
        assert [x[0] for x in rows]==[','.join(map(str,c)) for c in cases()] and len(rows)*2==r['normalReturns'];assert all(x[1:5]==x[5:9] for x in rows);print('간헐천 수명 원문 감사:',len(rows));return
    oracles=[Oracle(),Oracle()];rows=[];different=[]
    # 두 정밀도의 원본을 별도로 실행하고 제품 계산을 기대값 생성에 쓰지 않는다.
    for i,c in enumerate(cases()):
        out=[o.sample(c,p) for o,p in zip(oracles,(0x27f,0x37f))]
        if out[0]!=out[1]:different.append(i)
        rows.append((','.join(map(str,c)),*out[0],*out[1]))
    FIXTURE.write_text('# input effects53 state53 raw53 list53 effects64 state64 raw64 list64\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_geyser_lifetime.py','tools/decomp_carrier_falling.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/geyser-lifetime-x86.tsv')
    r=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),precisionDifferentInputs=different,originalVtable=oracles[0].vtable,boundaries='485580/485540/40ea00 전체실행. 가상기본프레임/표시/AI/섬정리/큰Damageablepre·post는명시경계다. 직접frameDWORD/효과뒤현재extra·목록/모든중복제거·물리꼬리보존과전체raw/현재editor·count/효과순서를대조했다. GP/retN/SEH/앞뒤canary/x87/명령·읽기·쓰기범위를검사했다. CD/nonnullAI하위/편집섬정리내부/실제게임창·완주검증은아니다.',sha256={path:digest(ROOT/path) for path in paths})
    REPORT.write_text(json.dumps(r,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:r[k] for k in ('uniqueInputs','normalReturns','instructions','nativeCalls','precisionDifferentInputs')},ensure_ascii=False))


if __name__=='__main__':main()
