#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의 전체489ff0을 실행해 운반 대상 사망 분기의 조회 순서/갱신/삭제를 관측한다."""
import argparse,collections,itertools,json,socket,struct,zlib
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,TYPES,VTABLE,digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as PreviousOracle,POOL
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP

PATH,THUNK,HALT=0x14900000,STOP+0xb00,STOP+0xe00 # 격리 Runtime/고정 thiscall 호출자/부모 가상 Halt 경계다.
SIZE=0x186f0 # PATCH Path 이미지의 실제 폭이다.
A,B,DEAD=0x186cc,0x186d0,0x186ec # 실제 간헐천/귀환 대상과 stack deadFlag 필드다.
ENTRIES=(0x489ff0,0x4ac200,0x49a840) # 순수 분기와 TYPE 조회의 실제 본문이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/path-retarget-x86.tsv' # 기존 pathruntime 원문과 구별되는 새 분기 자료다.
REPORT=ROOT/'cpppj/path-retarget.json' # 실제 반환/효과/필드/읽기·쓰기 범위와 SHA다.


def cases():
    """dead2/free1/빈 대상/두 부모/내용물/선택 실패/새 이동의 삭제를 교차한다."""
    rows=[]
    # A/B/stateA/stateB/currentParent/find1/find2/returnChoice/geyserChoice/delete/mutation/nuggetType다.
    for a,b,sa,sb,parent,f1,f2,home,geyser,drop in itertools.product((0,60),(0,61),range(4),range(4),(50,51),(0,97),(0,97),(0,62),(0,63),(0,1)):
        rows.append((a,b,sa,sb,parent,f1,f2,home,geyser,drop,0,154))
    # 효과 뒤 현재 필드는 다시 읽되 최초 대상 포인터/현재 부모 캡처는 유지한다.
    for mutation,a,f1,f2,home,geyser,drop in itertools.product(range(1,12),(0,60),(0,97),(0,97),(0,62),(0,63),(0,1)):
        rows.append((a,61,2,2,51,f1,f2,home,geyser,drop,mutation,154))
    # 살아 있는 대상은 잘못된 현재 부모 번호도 실제 조회하지 않는다.
    rows.append((60,61,0,1,0xffffffff,0,0,0,0,0,0,154))
    return rows


class Oracle(BaseOracle):
    """부모 Halt=10/boss/carrier/extra20 없음 계약에서 전체 원본 Halt를 실행한다."""
    def __init__(self):
        """원본 코드와 명시 입력만 설치하고 큰 내용물/선택/이동 효과는 기록한다."""
        super().__init__();self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter();self.deletionChecks=0
        # 독립 부모/대상과 실제100080바이트 Path를 확보한다.
        for address,size in ((POOL,0x10000),(PATH-0x1000,0x1a000)):self.mu.mem_map(address,size)
        code=b'\xb9'+struct.pack('<I',PATH)+b'\xe8'+struct.pack('<i',0x489ff0-(THUNK+10))+b'\xc3';self.mu.mem_write(THUNK,code);self.allowed.append((THUNK,THUNK+len(code)))
        self.code_high={a:hi for lo,hi in self.allowed for a in range(lo,hi)} # 기존 범위의 끝 주소를 매 명령에 확인한다.
        self.entries=frozenset(ENTRIES);self.boundaries=frozenset((HALT,0x4ac9f0,0x428ea0,0x429420,0x4276a0,0x427600)) # 입력이 필요한 실제 경계 진입만 분리한다.
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def fields(self):
        """현재 세 DWORD를 읽으며 float/호스트 포인터로 해석하지 않는다."""
        return self.get(PATH+A),self.get(PATH+B),self.get(PATH+8)

    def mutate(self,phase):
        """선택 결과를 계산하지 않는 명시 효과 입력이다."""
        mode=self.case[10]
        if phase=='find' and self.finds==1:
            if mode==1:self.put(PATH+A,0)
            if mode==2:self.put(PATH+A,62)
            if mode==3:self.put(PATH+B,0)
            if mode==4:self.put(PATH+8,52);self.put(0x5412c0,155)
            if mode==8:self.put(POOL+self.case[0]*50+11,0,1)
            if mode==9:self.put(POOL+self.case[1]*50+11,0,1)
        if phase=='find' and self.finds==1 and mode==10:self.put(0x5412c0,155)
        if phase=='find' and self.finds==1 and mode==11:self.put(PATH+B,62)
        if phase=='return' and mode==5:self.put(PATH+A,62);self.put(PATH+B,0)
        if phase=='geyser' and mode==6:self.put(PATH+B,62);self.put(PATH+A,0)
        if phase=='go' and mode==7:self.put(PATH+A,64);self.put(PATH+B,65)

    def instruction(self,mu,address,size,data):
        """실제 분기·캡처·재조회·필드 쓰기와 deadFlag 확인을 대체하지 않는다."""
        self.instructions+=1
        if address in self.entries:self.calls[f'{address:08x}']+=1
        if address not in self.boundaries:
            if address+size<=self.code_high.get(address,0):return
            raise RuntimeError(f'대상 재확인 범위 밖 실행 {address:08x}')
        sp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<3I',mu.mem_read(sp+4,12));this=mu.reg_read(UC_X86_REG_ECX);parent=(this-POOL)//50;c=self.case
        if address==HALT:
            assert parent==50;self.put(PATH+8,c[4]);self.ret(10);return
        if address==0x4ac9f0:
            self.finds+=1;self.event('F',parent,*args);assert args[0]==0x20 and args[2]==0;self.mutate('find');self.ret(c[5] if self.finds==1 else c[6],12);return
        if address in (0x428ea0,0x429420):
            home=address==0x428ea0;self.event('C' if home else 'Y',parent);self.mutate('return' if home else 'geyser');self.ret(c[7] if home else c[8]);return
        if address in (0x4276a0,0x427600):
            self.event('R' if address==0x4276a0 else 'G',parent,args[0],*self.fields());self.mutate('go');self.snapshot=self.fields()
            if c[9]:
                watch=self.get(PATH+DEAD);assert STACK-0x7f000<=watch<STACK;self.put(watch,1);self.deleted=True;self.deletionChecks+=1;mu.mem_write(PATH,b'\xdd'*SIZE)
            self.ret(0,4);return
        raise RuntimeError(f'대상 재확인 미처리 경계 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """원본은 stack/deadFlag/두 대상 DWORD만 쓸 수 있다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000:return
        if address in (PATH+A,PATH+B,PATH+DEAD) and size==4 and not self.deleted:return
        raise RuntimeError(f'대상 재확인 범위 밖 쓰기 {address:08x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """이동 경계가 삭제/poison한 Runtime은 원본도 다시 읽으면 실패한다."""
        if PATH<=address and address+size<=PATH+SIZE:
            if self.deleted:raise RuntimeError(f'삭제 뒤 Runtime 읽기 {address:08x}+{size}')
            return
        spans=((0x400000,0x600000),(POOL,POOL+0x10000),(TYPES,TYPES+0x20000),(VTABLE,VTABLE+0x1000),(STACK-0x7f000,STACK+0x1000),(STOP,STOP+0x1000))
        if not any(lo<=address and address+size<=hi for lo,hi in spans):raise RuntimeError(f'대상 재확인 범위 밖 읽기 {address:08x}+{size}')

    def sample(self,c,precision):
        """명시 Runtime과 물리 raw 입력을 설치하고 정상 반환/효과/필드만 기록한다."""
        self.case=c;self.events=[];self.finds=0;self.deleted=False;self.snapshot=None;self.mu.mem_write(POOL,b'\x2a'*0x10000);self.mu.mem_write(PATH,b'\xa5'*SIZE);self.mu.mem_write(PATH-16,b'\x5a'*16);self.mu.mem_write(PATH+SIZE,b'\x5a'*16)
        self.put(PATH+4,5);self.put(PATH+8,50);self.put(PATH+A,c[0]);self.put(PATH+B,c[1]);self.put(PATH+DEAD,0);self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES);self.put(TYPES+133*500+0xec,0x10000);self.put(0x540bc4,1);self.put(0x5e4794,1);self.put(0x5412c0,c[11]);self.put(VTABLE+0x38,HALT);self.put(0,0x87654321)
        # 실제 부모 가상과 두 대상 상태의 입력이다.0번 슬롯 중첩도 원래 주소 그대로다.
        for sid in (50,51,52):
            self.put(POOL+sid*50,VTABLE);self.put(POOL+sid*50+10,133,1);self.put(POOL+sid*50+11,0,1);self.put(POOL+sid*50+40,0,1)
        self.put(POOL+c[0]*50+11,c[2],1);self.put(POOL+c[1]*50+11,c[3],1)
        PreviousOracle.invoke(self,THUNK,b'',precision,False);self.returns+=1;assert self.get(0)==0x87654321 and bytes(self.mu.mem_read(PATH-16,16))==b'\x5a'*16 and bytes(self.mu.mem_read(PATH+SIZE,16))==b'\x5a'*16
        fields=self.snapshot if self.deleted else self.fields();result=self.mu.reg_read(UC_X86_REG_EAX);assert result==int(self.deleted)
        if not self.deleted:assert self.get(PATH+DEAD)==0
        state=(result,*fields,self.get(0x5412c0),self.finds,zlib.adler32(bytes(self.mu.mem_read(POOL,0x10000)))&0xffffffff)
        return ';'.join(self.events) or '-',','.join(map(str,state))


def main():
    """새 독립 관측 또는 기존 원문의 읽기 전용 SHA/입력 감사를 수행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'));rows=[s.split('\t') for s in FIXTURE.read_text('utf-8').splitlines() if s and not s.startswith('#')]
        # 감사는 기대값이나 보호 자료를 덮어쓰지 않는다.
        for p,want in report['sha256'].items():assert digest(ROOT/p)==want,p
        assert [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()] and len(rows)*2==report['normalReturns'];assert all(r[1:3]==r[3:5] for r in rows);print('대상 재확인 원본 감사:',len(rows),'입력/',report['normalReturns'],'정상 반환');return
    oracles=[Oracle(),Oracle()];rows=[];different=[]
    # 원본 전체 Halt를 실행하며 새 C++를 기대값 계산에 사용하지 않는다.
    for i,c in enumerate(cases()):
        outputs=[o.sample(c,p) for o,p in zip(oracles,(0x27f,0x37f))]
        if outputs[0]!=outputs[1]:different.append(i)
        rows.append((','.join(map(str,c)),*outputs[0],*outputs[1]))
    FIXTURE.write_text('# input effects53 state53 effects64 state64\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_path_retarget.py','tools/decomp_carrier_falling.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/path-retarget-x86.tsv')
    report=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),deletedRuntimeNoRead=sum(o.deletionChecks for o in oracles),precisionDifferentInputs=different,boundaries='전체489ff0/4ac200/49a840 실행. 입력 계약은 부모Halt10/extra20없음/boss1/carrier다. 새C++비교범위는그안의대상사망fragment이며바깥Halt기존검증과구별한다. 부모Halt효과/내용물조회/두선택/두이동은명시경계다. 이동경계의Runtime삭제/poison뒤원본무접근,GP/retN/SEH/canary/x87/명령·읽기·쓰기범위를검사했다. 비교필드는A/B/현재부모/현재nuggetType/호출순서/물리raw64KB Adler다.전체Runtime생성자/실제게임창/자연운반완주검증은아니다.',sha256={p:digest(ROOT/p) for p in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('uniqueInputs','normalReturns','instructions','nativeCalls','deletedRuntimeNoRead','precisionDifferentInputs')},ensure_ascii=False))


if __name__=='__main__':main()
