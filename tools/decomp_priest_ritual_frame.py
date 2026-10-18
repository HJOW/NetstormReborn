#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 사제604의 실제 K..O 반복 프레임/605 조건/위치 소리를 격리 x86에서 관측한다."""
import argparse,hashlib,itertools,json,socket,struct
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,POOL,TYPES,STACK,STOP
from decomp_cursor_position import ranges
from shp import load_type_texts,read_clusters
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP

FIXTURE=ROOT/'cpppj/tests/fixtures/priest-ritual-frame-x86.tsv' # 기존 Begin/Continue 원문과 별도의604 몸체 관측이다.
REPORT=ROOT/'cpppj/priest-ritual-frame.json' # 보호 PE/실제 코드표와 하위 효과 경계다.
CODE,THUNK,OUT=0x12a00000,STOP+0x800,STOP+0x900 # 실제 TYPE 코드 입력과 x87 반환의 float 저장 호출자다.
ENTRY=0x494160 # 원본 사제 Handle604가 부르는 실제 함수다.

def sha(path):
    """원문 바이트의 SHA256이다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

class Oracle(BaseOracle):
    """방향/TYPE/프레임 계산은 실제 몸체이며 setter/FindEvent/위치 소리만 선언 경계다."""
    def __init__(self):
        """보호 PE를 읽고 실제604/방향/TYPE getter 범위만 허용한다."""
        super().__init__();self.mu.mem_map(CODE,0x10000);self.allowed=[];ranges(self,(ENTRY,0x49417a,0x4ad710,0x4ac550,0x49a840));self.instructions=0
        caller=b'\xb9'+struct.pack('<I',POOL+50*50)+b'\xe8'+struct.pack('<i',ENTRY-(THUNK+10))+b'\xd9\x1d'+struct.pack('<I',OUT)+b'\xc3'
        self.mu.mem_write(THUNK,caller);self.allowed.append((THUNK,THUNK+len(caller)));self.actualCodes=''.join(row[0][0] for row in read_clusters(load_type_texts(str(ROOT/'originals'))['priest']))

    def instruction(self,mu,address,size,data):
        """실제 호출 순서를 기록하고 선언한 callback 뒤의 현재 입력만 바꾼다."""
        sp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<4I',mu.mem_read(sp+4,16));sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50
        if address==0x4afb40:
            assert sid==50 and args[0]==605;self.event('E',sid,args[0])
            if self.case[4]==1:self.put(POOL+50*50+14,0x41b20000);self.put(POOL+50*50+18,0xc18c0000);self.put(POOL+50*50+36,99)
            self.ret(self.case[3],4);return
        if address==0x4a9d70:
            self.event('S',args[0],args[1],self.string(args[2]),args[3])
            if self.case[4]==2:self.put(POOL+50*50+10,133,1);self.put(POOL+50*50+36,77)
            self.ret();return
        if address==0x4acee0:
            assert sid==50 and args[1]==0;self.event('F',sid,args[0],args[1]);self.put(POOL+50*50+36,args[0]);self.ret(0,8);return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'사제604 범위 밖 실행 {address:08x}')
        self.instructions+=1

    def write(self,mu,access,address,size,value,data):
        """실제 명령의 쓰기는 격리 스택과 x87 반환 저장소에만 허용한다."""
        if not (STACK-0x7f000<=address and address+size<=STACK+0x1000) and not (OUT<=address and address+size<=OUT+4):raise RuntimeError(f'사제604 범위 밖 쓰기 {address:08x}')

    def sample(self,case,precision):
        """현재 raw/코드표를 준비하고 전체 반환 바이트·호출·raw를 원본에서 읽는다."""
        self.case=case;self.events=[];mode,frame,side,event,mutation=case;codes=self.actualCodes if mode==0 else chr(side)
        raw=bytearray(b'\xa5'*50);raw[10]=158;struct.pack_into('<II',raw,14,0x41a40000,0x41ac0000);struct.pack_into('<I',raw,36,frame);self.mu.mem_write(POOL+50*50,bytes(raw));self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES)
        for type in (133,158): # callback의 타입 변이는 선언 입력이며 다음 setter는 계산해 둔 인자를 받는다.
            table=TYPES+type*500;self.put(table+0x124,CODE);self.put(table+0x114,len(codes))
            for index,letter in enumerate('ABCDEFGHIJKLMNOP'): # 원본이 읽는 실제 first/count 표의 입력이다.
                self.put(table+0x130+index*4,codes.find(letter));self.put(table+0x170+index*4,codes.count(letter))
        self.mu.mem_write(CODE,b''.join(bytes((ord(c),0,0,0)) for c in codes));self.call(THUNK,0,b'',0,precision)
        return bytes(self.mu.mem_read(OUT,4)).hex(),';'.join(self.events) or '-',bytes(self.mu.mem_read(POOL+50*50,50)).hex()

def main():
    """대응 C++ 전에 원문을 만들거나 두 정밀도/출처를 읽기 전용으로 재검증한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();oracle=Oracle();cases=[];rows=[]
    cases.extend((0,frame,0,event,mutation) for frame,event,mutation in itertools.product(range(len(oracle.actualCodes)),(0,1),range(3)))
    cases.extend((1,0,side,event,mutation) for side,event,mutation in itertools.product((0,65,74,75,79,80,255),(0,1),range(3)))
    for case in cases: # 기대 수식을 만들지 않고 실제 두 실행 결과를 대조한다.
        a=oracle.sample(case,0x27f);b=oracle.sample(case,0x37f);assert a==b;rows.append(','.join(map(str,case))+'\t'+'\t'.join(a))
    fixture=('# mode,frame,side,event605,mutation\tfloatReturnBytes\ttrace\traw50\n'+'\n'.join(rows)+'\n').encode('utf-8')
    paths=['originals/Netstorm.exe','originals/netstorm.tarc','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priest_ritual_frame.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py','tools/shp.py']
    report=dict(edition='10.78',host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',lastDecompileDate='2026-10-16',uniqueInputs=len(cases),normalReturns=2*len(cases),bodyAndHelperInstructions=oracle.instructions,precisionDifferences=0,actualPriestCodes=oracle.actualCodes,scope='실제494160과 실제 도달한 내부49417a/방향·TYPE getter.4acee0 setter/4afb40 Find605/4a9d70 소리는 인자·순서 관찰 경계. 합성 단일K/O의짧은Run은setter경계 뒤의 표시 유효성을 주장하지 않는다.604상위Handle/원본 전체게임/CD판은 미검증.',sha256={p:sha(ROOT/p) for p in paths});report['sha256'][FIXTURE.relative_to(ROOT).as_posix()]=hashlib.sha256(fixture).hexdigest()
    if args.verify:assert FIXTURE.read_bytes()==fixture and json.loads(REPORT.read_text('utf-8'))==report
    else:FIXTURE.write_bytes(fixture);REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n')
    print('사제604 원본',len(cases),'입력/',len(cases)*2,'정상 반환/',oracle.instructions,'명령')

if __name__=='__main__':main()
