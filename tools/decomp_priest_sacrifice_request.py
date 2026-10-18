#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 제단 자격 wrapper와 희생 요청의 호출 순서/현재 owner 재독을 제한 x86에서 관측한다."""
import argparse,hashlib,itertools,json,socket,struct
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,POOL,STACK,STOP,digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP

FIXTURE=ROOT/'cpppj/tests/fixtures/priest-sacrifice-request-x86.tsv' # 새 wrapper 원문이며 기존 Dais 몸체 관측을 덮어쓰지 않는다.
REPORT=ROOT/'cpppj/priest-sacrifice-request.json' # 실행 범위/명시 경계와 원문 SHA다.
ENTRIES=(0x4498e0,0x494eb0) # 실제 wrapper 두 개만 이번 실행 수에 포함한다.

def cases():
    """허용 반환 및 제거/명령 뒤 변경되는 owner·로컬 owner·내용물 종류를 독립 입력한다."""
    result=[(0,owner,local,allowed,0,154) for owner,local,allowed in itertools.product((0,1,2,8),(0,1,2,8),(0,1,0xffffffff))]
    result += [(1,owner,local,0,mutation,kind) for owner,local,mutation,kind in itertools.product((0,1,2,8),(1,2),range(5),(154,0x1234))]
    return result

class Oracle(BaseOracle):
    """큰 Dais/내용물/음악 몸체는 명시 경계이며 wrapper의 포인터와 재독/순서는 실제 명령이다."""
    def __init__(self):
        """보호 PE를 읽고 두 정밀 함수 범위만 실행 가능하게 둔다."""
        super().__init__();self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls={str(e):0 for e in ENTRIES}

    def instruction(self,mu,address,size,data):
        """cdecl/thiscall 경계 인자를 기록하고 원본의 다음 owner 조회 전 명시 변경을 적용한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);a=struct.unpack('<4I',mu.mem_read(sp+4,16));ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==0x449710:self.event('A',(ecx-POOL)//50,a[0]);self.ret(self.case[3],4);return
        if address==0x4b26f0:
            self.event('R',*a)
            if self.case[4]==1:self.put(POOL+50*50+34,2,1)
            if self.case[4]==2:self.put(0x5412c0,77)
            self.ret(0xabcdef);return
        if address==0x449900:
            self.event('C',(ecx-POOL)//50,*a[:2])
            if self.case[4]==3:self.put(POOL+50*50+34,2,1)
            if self.case[4]==4:self.put(0x540c70,2)
            self.ret(0,8);return
        if address==0x469db0:self.event('M',self.string(a[0]));self.ret();return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'희생 요청 범위 밖 실행 {address:08x}')
        self.instructions+=1
        if address in ENTRIES:self.calls[str(address)]+=1

    def write(self,mu,access,address,size,value,data):
        """원본 함수의 쓰기는 격리 스택에만 허용한다. callback 입력은 put으로 별도 공급한다."""
        if not STACK-0x7f000<=address or address+size>STACK+0x1000:raise RuntimeError(f'희생 요청 범위 밖 쓰기 {address:08x}')

    def sample(self,case,precision):
        """raw/현재 전역을 새로 준비하고 실제 wrapper 반환·스택과 보존 레지스터를 확인한다."""
        self.case=case;self.events=[];self.put(0x5c8464,POOL);self.put(0x540c70,case[2]);self.put(0x5412c0,case[5]);self.put(POOL+50*50+34,case[1],1)
        self.call(ENTRIES[case[0]],0,struct.pack('<II',*((60,50) if not case[0] else (50,60))),0,precision)
        result=self.mu.reg_read(UC_X86_REG_EAX)
        return ';'.join(self.events),str(int(bool(result)) if not case[0] else 0),str(self.mu.mem_read(POOL+50*50+34,1)[0]),str(self.get(0x540c70)),str(self.get(0x5412c0))

def main():
    """C++ 전에 생성한 원문을 저장하거나 현재 보호 출처와 두 정밀도 실행을 읽기 전용 대조한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();oracle=Oracle();rows=[]
    for case in cases(): # 결과는 Python 조건식으로 만들지 않고 실제 두 정밀도 실행에서 읽는다.
        first=oracle.sample(case,0x27f);second=oracle.sample(case,0x37f);assert first==second;rows.append(','.join(map(str,case))+'\t'+'\t'.join(first))
    raw=('# input(op,owner,local,allowed,mutation,kind)\ttrace\tallowed\towner\tlocal\tkind\n'+'\n'.join(rows)+'\n').encode('utf-8')
    paths=['originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priest_sacrifice_request.py']
    report=dict(edition='10.78',host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',lastDecompileDate='2026-10-16',uniqueInputs=len(rows),normalReturns=2*len(rows),instructions=oracle.instructions,entries=oracle.calls,gameRuns=0,scope='4498e0/494eb0 wrapper만.449710/내용물4b26f0/449900/음악469db0은 호출 인자·순서 관측 경계이며 기존 Dais/Path/음악 몸체 관측과 중복 합산하지 않는다.',sha256={p:digest(ROOT/p) for p in paths})
    report['sha256'][FIXTURE.relative_to(ROOT).as_posix()]=hashlib.sha256(raw).hexdigest()
    if args.verify:
        assert FIXTURE.read_bytes()==raw;old=json.loads(REPORT.read_text('utf-8'));assert old==report
    else:
        FIXTURE.write_bytes(raw);REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n')
    print('희생 wrapper 원본',len(rows),'입력/',len(rows)*2,'정상 반환')

if __name__=='__main__':main()
