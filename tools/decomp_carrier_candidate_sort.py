#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의 실제 골렘 후보 주소 비교기와 구형 CRT qsort를 독립 관측한다."""
import argparse,collections,itertools,json,socket,struct,zlib
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as PreviousOracle,POOL
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP

INPUT=0x14500000 # 고정 호출자에 공급하는 DWORD 입력이다.
BUFFERS=(0x14700000,0x051eb854) # 일반 힙 주소와 주소×50이 풀 안으로 감기는 주소다.
THUNKS=(STOP+0xa00,STOP+0xb00) # 코드를 입력마다 수정하지 않는 cdecl 호출자다.
ENTRIES=(0x428020,0x4e6010,0x4e5fa0) # 비교기와 CRT의 전체 실제 몸체다.
CAPACITY=24000 # PATCH의 실제 슬롯 수와50바이트 물리 폭을 사용한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/carrier-candidate-sort-x86.tsv' # 새로운 독립 관측 원문이다.
REPORT=ROOT/'cpppj/carrier-candidate-sort.json' # 범위/반환/비교 횟수/보호 SHA 기록이다.


def pool_bytes(pattern):
    """출력 판단과 무관한 물리 풀 입력을 만들며 비정렬 WORD도 서로 다르게 채운다."""
    return bytes(((i*37+(i//50)*11+pattern*19)&255) if pattern else 0 for i in range(CAPACITY*50))


def values(count,pattern):
    """배열 값이 비교 대상이 아님을 확인하는 세 종류의 독립 DWORD 입력이다."""
    return [i+1 if pattern==0 else count-i if pattern==1 else (0xf0000000+i*0x10203)&0xffffffff for i in range(count)]


def cases():
    """작은 정렬/분할 경계/500개 상한과 직접 주소의 감김·범위를 교차한다."""
    rows=[]
    # 열: op,buffer,count,rawPattern,valuePattern,arg1,arg2,poolAddress다.
    for count,buffer,raw,pattern in itertools.product((0,1,2,3,7,8,9,10,15,16,17,31,32,33,63,64,65,127,128,129,255,256,257,499,500),BUFFERS,(0,1),(0,2)):
        rows.append((0,buffer,count,raw,pattern,0,0,POOL))
    direct=(0,1,2,23998,23999,24000,0xffffffff,BUFFERS[0],BUFFERS[1],BUFFERS[1]+4,BUFFERS[1]+8,0x80000000)
    # 직접 비교는 두 유효성의 네 조합과 실제 unsigned WORD 차이를 모두 남긴다.
    for first,second,raw in itertools.product(direct,direct,(0,1)):
        rows.append((1,BUFFERS[0],0,raw,0,first,second,POOL))
    # 풀 상한이 감기는 경우 unsigned 양끝 검사 자체가 모두 거짓이다.
    for first,second in itertools.product((0,1,2,23999,0x051eb854),(0,1,2,23999,0x851eb854)):
        rows.append((1,BUFFERS[0],0,1,0,first,second,0xffff0000))
    return rows


class Oracle(BaseOracle):
    """원본 코드를 실행하며 비교 인자 순서를 기록하고 실제 계산/교환은 대체하지 않는다."""
    def __init__(self):
        """독립 풀/두 배열/고정 입력과 실제 cdecl 호출자를 설치한다."""
        super().__init__();self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter();self.code_spans=[]
        # 풀은 실제 전체 폭을 확보하고 배열 앞뒤에는 canary 공간을 둔다.
        for address,size in ((POOL,0x126000),(INPUT,0x1000),(BUFFERS[0]-0x1000,0x3000),(0x051eb000,0x3000)):self.mu.mem_map(address,size)
        self.pools=[pool_bytes(p) for p in range(2)]
        self.install(THUNKS[0],0x4e6010,[('imm',0x428020),('imm',4),('mem',INPUT+8),('mem',INPUT+4)],16)
        self.install(THUNKS[1],0x428020,[('mem',INPUT+24),('mem',INPUT+20)],8)
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def install(self,address,entry,args,cleanup):
        """실제 스택 규약을 코드로 실행하며 인자 복구까지 검사한다."""
        code=b''
        # 명시 인자는 이미 스택에 올리는 역순이다.
        for kind,value in args:code+=(b'\xff\x35' if kind=='mem' else b'\x68')+struct.pack('<I',value)
        code+=b'\xe8'+struct.pack('<i',entry-(address+len(code)+5))+b'\x83\xc4'+bytes((cleanup,))+b'\xc3'
        self.mu.mem_write(address,code);self.code_spans.append((address,address+len(code)))

    def instruction(self,mu,address,size,data):
        """비교기 인자는 관찰만 하고 어느 키나 비교 결과도 주입하지 않는다."""
        self.instructions+=1
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address==0x428020:
            sp=mu.reg_read(UC_X86_REG_ESP);self.trace.extend(struct.unpack('<2I',mu.mem_read(sp+4,8)))
        if any(low<=address and address+size<=high for low,high in self.allowed+self.code_spans):return
        raise RuntimeError(f'후보 정렬 범위 밖 실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """정렬은 스택과 현재 배열 prefix에만 쓸 수 있다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000:return
        if self.case[1]<=address and address+size<=self.case[1]+self.case[2]*4:return
        raise RuntimeError(f'후보 정렬 범위 밖 쓰기 {address:08x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """보호 이미지/전체 물리 풀/배열/호출자 외의 읽기를 거부한다."""
        spans=((0x400000,0x600000),(POOL,POOL+CAPACITY*50),(INPUT,INPUT+32),(STACK-0x7f000,STACK+0x1000),(STOP,STOP+0x1000),(BUFFERS[0],BUFFERS[0]+2000),(BUFFERS[1],BUFFERS[1]+2000))
        if not any(low<=address and address+size<=high for low,high in spans):raise RuntimeError(f'후보 정렬 범위 밖 읽기 {address:08x}+{size}')

    def sample(self,case,precision):
        """배열 전체/풀 불변/canary와 실제 비교 호출의 원문을 저장한다."""
        self.case=case;self.trace=[];op,buffer,count,raw,pattern,first,second,base=case
        self.mu.mem_write(INPUT,struct.pack('<8I',*case));self.put(0x5c8464,base);self.put(0x5c847c,CAPACITY);self.put(0,0x87654321)
        self.mu.mem_write(POOL,self.pools[raw]);initial=struct.pack('<500I',*values(500,pattern));self.mu.mem_write(buffer-16,b'\xa5'*16+initial+b'\x5a'*16)
        PreviousOracle.invoke(self,THUNKS[op],b'',precision,False);self.returns+=1
        assert bytes(self.mu.mem_read(POOL,CAPACITY*50))==self.pools[raw] and self.get(0)==0x87654321
        assert bytes(self.mu.mem_read(buffer-16,16))==b'\xa5'*16 and bytes(self.mu.mem_read(buffer+2000,16))==b'\x5a'*16
        assert bytes(self.mu.mem_read(buffer+count*4,2000-count*4))==initial[count*4:]
        result=struct.unpack('<i',struct.pack('<I',self.mu.reg_read(UC_X86_REG_EAX)))[0] if op else 0
        output=bytes(self.mu.mem_read(buffer,count*4))
        return str(result),','.join(map(str,struct.unpack(f'<{count}I',output))) or '-',','.join(map(str,self.trace)) or '-',str(zlib.adler32(output)&0xffffffff)


def main():
    """새 관측과 기존 원문 감사를 분리하고 두 x87 제어어의 정수 처리 불변을 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'));rows=[s.split('\t') for s in FIXTURE.read_text('utf-8').splitlines() if s and not s.startswith('#')]
        # 감사 모드는 보호 원문/도구/입력 순서를 확인하며 어느 파일도 덮어쓰지 않는다.
        for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
        assert [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()] and len(rows)*2==report['normalReturns']
        assert [i for i,r in enumerate(rows) if r[1:5]!=r[5:9]]==report['precisionDifferentInputs'];print('후보 정렬 원본 감사:',len(rows),'입력/',report['normalReturns'],'정상 반환');return
    oracles=[Oracle(),Oracle()];rows=[];different=[]
    # 제품 C++나 정렬 알고리즘으로 기대값을 만들지 않고 실제 PE의 순서/비교 인자를 저장한다.
    for index,case in enumerate(cases()):
        outputs=[o.sample(case,p) for o,p in zip(oracles,(0x27f,0x37f))]
        if outputs[0]!=outputs[1]:different.append(index)
        rows.append((','.join(map(str,case)),*outputs[0],*outputs[1]))
    FIXTURE.write_text('# input return53 array53 calls53 adler53 return64 array64 calls64 adler64\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_carrier_candidate_sort.py','tools/decomp_carrier_falling.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/carrier-candidate-sort-x86.tsv')
    report=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),precisionDifferentInputs=different,boundaries='428020/4e6010/4e5fa0 전체 실제 코드. 하위 계산/키/비교 결과/교환을 대체하지 않았다. GP/cdecl 스택/canary/SEH/x87 CW·TOP·tag/코드·읽기·쓰기 범위와 전체 raw 풀 불변을 검사한다. 정렬 입력의 명시 주소와 물리 바이트만 경계다. 상위4280a0/귀환선택/게임창/자연운반 검증은 아니다.',sha256={p:digest(ROOT/p) for p in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('uniqueInputs','normalReturns','instructions','nativeCalls','precisionDifferentInputs')},ensure_ascii=False))


if __name__=='__main__':main()
