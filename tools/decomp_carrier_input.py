#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 운반체 커서 평가/강조/클릭 명령/이동 음향을 격리 실행한다."""
import argparse,collections,itertools,json,socket,struct
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,TYPES,digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as PreviousOracle,POOL
from decomp_nugget_regular import bits
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EAX
FIXTURE=ROOT/'cpppj/tests/fixtures/carrier-input-x86.tsv' #새 부모의 독립 원문이다.
REPORT=ROOT/'cpppj/carrier-input.json' #실제 보호 원본/도구/입력/정상 반환 기록이다.
ENTRIES=(0x4277f0,0x4284a0,0x42a360,0x426290,0x4ac200,0x4ac1e0,0x4ac550,0x49a840,0x41d7a0,0x4e49c0,0x4df230,0x40e880,0x40e8f0,0x40e960,0x40e800,0x41d0d0,0x4487a0)
POINT,THUNK=STOP+0x900,STOP+0xa00 #실제 writable 점과 cdecl 호출자다.

def cases():
    """각 관문/현재 타입/순서/점 수정/모드/소리 인자를 독립 입력한다."""
    #op,genus,flags,extra,owner,local,immobile,content,candidate,nugget,graph,sameGraph,connected,surface,geyser,valid,return,dais,allowed,early,checking,multiple,markers,force,sid,cursor,stamp,frame,soundIndex,type,x,y,mutation
    base=[0,0x10000,0,0,1,1,0,0,0,0,1,1,1,2,0,0,0,0,0,0,0,0,0,0,50,6,0,7,0xffffffff,133,bits(22.5),bits(23.5),0];rows=[]
    #각 상위의 모든 독립 입력 변화와 특이 좌표를 먼저 대조한다.
    changes={1:(0,0x10000,0x20000,0x210000),2:(0,0x1000000),3:(0,1,0x20),4:(1,2),5:(1,0x101),6:(0,1),7:(0,60),8:(0,61),9:(0,1),10:(1,0),11:(0,1),12:(0,1),13:(0,2,8,10),14:(0,62),15:(0,1),16:(0,63),17:(0,64),18:(0,1),19:(0,1),20:(0,1),21:(0,1),22:(0,1),23:(0,1),24:(0,50),25:(3,6,8,9,10,11,12,13,14,17,18),26:(0,7),28:(0,1,2,4,5,0xffffffff),29:(111,133,158),30:(bits(20),bits(19),bits(1),0x7fc01234,0x7f800000,0x80000000),31:(bits(21),bits(20),bits(1),0x7fc01234)}
    for op in range(4):
        r=base.copy();r[0]=op;rows.append(r)
        for field,values in changes.items():
            for value in values:
                r=base.copy();r[0]=op;r[field]=value;rows.append(r)
    #포획/운반 중 자기 사각형과 바깥 클릭의 모든 큰 분기를 교차한다.
    for op,genus,content,candidate,where,connected,dais in itertools.product((0,2),(0x10000,0x20000,0x210000),(0,60),(0,61),(0,1),(0,1),(0,64)):
        r=base.copy();r[0]=op;r[1]=genus;r[7]=content;r[8]=candidate;r[12]=connected;r[17]=dais;r[18]=1
        if where:r[30:32]=bits(20),bits(21)
        rows.append(r)
    #진단/현재 조회/음향 효과 뒤 실제 raw/TYPE/현재 owner를 다시 읽는다.
    for op,m in itertools.product(range(4),range(1,9)):
        r=base.copy();r[0]=op;r[32]=m;r[8]=61;r[17]=64;r[18]=1;r[20]=1;rows.append(r)
    return list(dict.fromkeys(tuple(r) for r in rows))

class Oracle(BaseOracle):
    """원본 전체 부모와 순수 기하를 실행하며 기존 큰 월드/UI/장치 하위만 기록한다."""
    def __init__(self):
        """보호 PE와 분리된 현재 raw/점/호출자를 만들고 접근 범위를 제한한다."""
        super().__init__();self.mu.mem_map(POOL,0x10000);self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter();self.mu.hook_add(UC_HOOK_MEM_READ,self.read)
    def mutate(self,where):
        """효과 뒤 원문이 다시 읽을 물리 입력만 바꾼다."""
        m=self.case[32]
        if m==1 and where=='assert':self.put(TYPES+self.case[29]*500+0xec,0x20000)
        if m==2 and where=='immobile':self.put(POOL+50*50+40,0x20,1)
        if m==3 and where=='candidate':self.put(TYPES+self.case[29]*500+0xec,0x20000)
        if m==4 and where=='candidate':self.put(TYPES+self.case[29]*500+0xe8,0x1000000)
        if m==5 and where=='ack':self.put(POOL+50*50+34,2,1)
        if m==6 and where=='allowed':self.put(0x540c70,2)
        if m==7 and where=='select':self.put(0x5453d8,18)
        if m==8 and where=='early':self.put(POOL+50*50+34,2,1)
    def instruction(self,mu,address,size,data):
        """실제 호출 인자/조회/쓰기 순서를 보존하며 큰 하위를 명시 경계로 기록한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);a=struct.unpack('<8I',mu.mem_read(sp+4,32));sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50;c=self.case
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address==0x4e0620:self.event('I',a[2]);self.mutate('assert');self.ret();return
        if address==0x492090:self.event('H',a[0]);self.mutate('immobile');self.ret(c[6]);return
        if address==0x4ac9f0:self.event('C',sid,*a[:3]);self.ret(c[7] if a[0]==0x20 else 0,12);return
        if address==0x426900:self.event('Q',sid,a[1],a[2]);self.mutate('candidate');self.put(a[0],c[8]);self.put(a[0]+4,c[9]);self.ret(a[0],12);return
        if address==0x426a70:self.event('D',sid,a[1]);self.put(a[0],bits(30));self.put(a[0]+4,bits(31));self.ret(0,8);return
        if address==0x462c80:self.event('G',a[0]);self.ret(c[10] if a[0]==50 else c[10] if c[11] else c[10]+1);return
        if address==0x488340:self.event('L',*a[:4]);self.ret(c[12]);return
        if address==0x40d9d0:self.event('S',*a[:2]);self.ret(c[13]);return
        if address==0x4b1fa0:self.event('Y',*a[:3]);self.ret(c[14]);return
        if address==0x427580:self.event('V',sid,a[0]);self.ret(c[15],4);return
        if address==0x4263a0:self.event('R',*a[:3]);self.ret(c[16]);return
        if address==0x4b2070:self.event('A',*a[:3]);self.ret(c[17]);return
        if address==0x4498e0:self.event('F',*a[:2]);self.mutate('allowed');self.ret(c[18]);return
        if address==0x447a50:self.event('E');self.mutate('early');self.ret(c[19]);return
        if address==0x4d5c90:self.event('U',*a[:3]);self.mutate('select');self.ret();return
        if address==0x4d5bb0:self.event('B',a[0]);self.ret();return
        if address==0x427340:self.event('T',sid,*a[:3]);self.ret(0,12);return
        if address==0x428e00:self.event('P',sid);self.ret();return
        if address==0x427070:self.event('N',sid,a[0]);self.ret(0,4);return
        if address==0x4cf960:self.event('W');self.ret();return
        if address==0x4c9150:self.event('J',a[0],self.string(a[1]));self.ret();return
        if address==0x4cfc10:self.event('K',a[0]);self.ret();return
        if address==0x4d5580:self.event('M');self.ret(c[21]);return
        if address==0x434970:self.event('Z',*a[:3]);self.ret();return
        if address==0x4a46d0:self.event('O');self.ret();return
        if address==0x4a4410:self.event('X',a[0]);self.ret();return
        if address==0x4558f0:self.event('?',*a[:2]);self.ret(1000);return
        if address in (0x4a9cb0,0x4a9550):self.event('a' if address==0x4a9cb0 else 's',(self.string(a[0]) if a[0]>=0x400000 else '@'+str(a[0])) if address==0x4a9cb0 else a[0],*a[1:6]);self.mutate('ack');self.ret();return
        if THUNK<=address<THUNK+100:return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'입력 원본 실행 범위 밖 {address:08x}')
    def write(self,mu,access,address,size,value,data):
        """원본 점/세 전역/타입 소리stamp/스택/SEH만 쓰도록 제한한다."""
        spans=((STACK-0x7f000,STACK+0x1000),(POINT,POINT+8),(0x5453d8,0x545420),(TYPES,TYPES+0x20000),(0,4))
        if not any(lo<=address and address+size<=hi for lo,hi in spans):raise RuntimeError(f'입력 원본 쓰기 범위 밖 {address:08x}+{size}')
    def read(self,mu,access,address,size,value,data):
        """보호 PE와 격리한 입력/현재 저장소 밖 접근을 거부한다."""
        spans=((0,4),(0x400000,0x600000),(POOL,POOL+0x10000),(TYPES,TYPES+0x20000),(STACK-0x7f000,STACK+0x1000),(STOP,STOP+0x1000))
        if not any(lo<=address and address+size<=hi for lo,hi in spans):raise RuntimeError(f'입력 원본 읽기 범위 밖 {address:08x}+{size}')
    def sample(self,c,precision):
        """C++ 없이 입력을 준비하고 EAX/전체raw/점/현재전역/타입stamp/효과를 관측한다."""
        self.case=c;self.events=[];self.mu.mem_write(TYPES,bytes(0x20000));self.put(TYPES+c[29]*500+0xec,c[1]);self.put(TYPES+c[29]*500+0xe8,c[2]);self.put(TYPES+c[29]*500+0x1f0,c[26]);self.put(TYPES+c[29]*500+0x78,123);self.put(TYPES+c[29]*500+0x80,124)
        for sid in (50,60,61,62,63,64):
            raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,0x12200000);raw[10]=c[29];raw[34]=c[4];raw[40]=c[3];struct.pack_into('<II',raw,14,bits(20),bits(21));self.mu.mem_write(POOL+sid*50,bytes(raw))
        self.put(POOL+60*50+18,134,2);self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES);self.put(0x540c70,c[5]);self.put(0x5e4794,c[20]);self.put(0x5453d8,c[25]);self.put(0x5453dc,77);self.put(0x5453e0,c[22]);self.put(0x5453ec,bits(11));self.put(0x5453f0,bits(12));self.put(0x54541c,1);self.put(0x531858,2);self.put(0x54d928,POINT+32);self.put(POINT+32,99);self.put(POINT+36,100);self.put(0x55b4a8,c[27]);self.put(0x5412d0,158);self.put(0x54126c,133);self.put(0,0x87654321);self.mu.mem_write(POINT,struct.pack('<II',c[30],c[31]))
        if c[0]==0:entry=0x4277f0;args=struct.pack('<I',POINT)
        else:
            target=(0,0x4284a0,0x42a360,0x426290)[c[0]];values=[c[24],c[30],c[31],c[23]] if c[0]==1 else [c[24],c[30],c[31]] if c[0]==2 else [50,c[28]];code=b''.join(b'\x68'+struct.pack('<I',v) for v in reversed(values));code+=b'\xe8'+struct.pack('<i',target-(THUNK+len(code)+5))+b'\x83\xc4'+bytes([4*len(values)])+b'\xc3';self.mu.mem_write(THUNK,code);self.mu.ctl_remove_cache(THUNK,THUNK+100);entry=THUNK;args=b''
        PreviousOracle.invoke(self,entry,args,precision,False);self.returns+=1;assert self.get(0)==0x87654321
        result=str(self.mu.reg_read(UC_X86_REG_EAX)) if c[0]==0 else '-';state=','.join(str(self.get(a)) for a in (0x5453d8,0x5453dc,0x5453ec,0x5453f0,0x54541c,0x540c70));types=','.join(str(self.get(TYPES+c[29]*500+o)) for o in (0xe8,0xec,0x1f0));raws=';'.join(bytes(self.mu.mem_read(POOL+sid*50,50)).hex() for sid in (50,60,61,62,63,64));return result,';'.join(self.events) or '-',bytes(self.mu.mem_read(POINT,8)).hex(),state,types,raws

def main():
    """첫 관측을 생성하거나 기존 독립 원문과 입력/보호 해시를 읽기 전용 감사한다."""
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--verify',action='store_true');a=p.parse_args()
    if a.verify:
        report=json.loads(REPORT.read_text('utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text('utf-8').splitlines() if r and not r.startswith('#')]
        for path,want in report['sha256'].items():assert digest(ROOT/path)==want,path
        assert [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()] and len(rows)==report['uniqueInputs'] and len(rows)*2==report['normalReturns'];assert all(r[1:7]==r[7:13] for r in rows);print('운반체 사용자입력 원문 감사',len(rows),report['normalReturns']);return
    oracles=[Oracle(),Oracle()];rows=[]
    for c in cases():
        outputs=[o.sample(c,p) for o,p in zip(oracles,(0x27f,0x37f))];assert outputs[0]==outputs[1],c;rows.append((','.join(map(str,c)),*outputs[0],*outputs[1]))
    assert all(sum(o.calls[f'{entry:08x}'] for o in oracles)>0 for entry in (0x4277f0,0x4284a0,0x42a360,0x426290))
    FIXTURE.write_text('# input return53 effects53 point53 globals53 type53 raws53 return64 effects64 point64 globals64 type64 raws64\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_carrier_input.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_carrier_falling.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/carrier-input-x86.tsv')
    report=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),precisionDifferentInputs=[],boundaries='4277f0/4284a0/42a360/426290와TYPE/반올림/사각형/제단점전체실행. 기존큰후보·지면·Graph·Drop·이동송신·포획·희생UI·음향·난수는명시경계다. GP/retN/canary/SEH/x87/실행·읽기·쓰기범위/점·현재전역/전체6raw50BYTE/현재TYPE·음향stamp/효과뒤재독을대조했다. 실제게임·자연완주증거가아니다.',sha256={p:digest(ROOT/p) for p in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(report['uniqueInputs'],report['normalReturns'],report['instructions'])
if __name__=='__main__':main()
