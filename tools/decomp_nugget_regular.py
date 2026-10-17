#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호너겟485230/프레임/낙하/지면/_ftol/정수게임난수를독립관측한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, TYPES, digest
from decomp_cursor_position import ranges
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG

POOL, SPOTS = 0x13d00000,0x13e00000  # 실제원본과분리한raw/점유지도입력이다.
DESTROY,UNPOP,POP,OTHER_POP,REFRESH,OTHER_REFRESH = (STOP+v for v in (0x300,0x320,0x340,0x360,0x380,0x3a0))  # 현재가상효과의서로다른경계다.
CALLER,RESULT,FLOAT_VALUE,FLOAT12,ALT_VT = (STOP+v for v in (0x500,0x580,0x590,0x600,0x800))  # 실제callerfloat32/기본float반환/변경VT다.
FIXTURE=ROOT/'cpppj/tests/fixtures/nugget-regular-x86.tsv'  # 새원문이며기존fixture는바꾸지않는다.
REPORT=ROOT/'cpppj/nugget-regular.json'  # 함수범위/보호SHA/두정밀도근거다.


def bits(value):
    """단정도명시입력만원문비트로만든다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def positions():
    """정상/끝/음수색인별칭/무효ftol하위DWORD의입력이다."""
    return ((bits(20.5),bits(21.25)),(bits(1),bits(1)),(bits(254.9),bits(254.99)),(bits(-2),bits(1)),(0x7fc01234,bits(20)),(bits(20),0x7fc01234),(0x7f800000,bits(1)),(bits(1),0xff800000),(bits(2**32),bits(1)),(bits(1),bits(2**32)))


def cases():
    """초기관문/프레임감김/할당실패/실제RNG/낙하/256spotBYTE를교차한다."""
    base=[0,0,1,bits(0.3),1,4,0,*positions()[0],1,0,1,0,0,0];rows=[]
    # op,event,count,payload,frame,Acount,spot,x,y,boss,state,allocate,seed,mutation,mapPattern다.
    for event,count,payload in itertools.product((0,1,2,3,0xffffffff,0x80000000),(0,1,2,10,99,100,101,0x7fffffff,0x80000000,0xffffffff),(0,bits(0.3),bits(-.25),0x7f800000,0x7fc01234)):
        row=base.copy();row[1:4]=event,count,payload;rows.append(tuple(row))
    # 양끝/정수감김/각할당실패/실제RNG와프레임효과의현재VT변경을구분한다.
    for event,frame,frames,allocation,seed,mutation in itertools.product((0,1),(0,1,2,3,4,0x7fffffff,0x80000000,0xffffffff),(0,1,4,0x80000000),(0,1),(0,1,0xffffffff),(0,1)):
        row=base.copy();row[1]=event;row[4:6]=frame,frames;row[11:14]=allocation,seed,mutation;rows.append(tuple(row))
    # 실제낙하의경계횟수/권한/상태/점유비트와Unpop재진입을교차한다.
    for count,point,spot,boss,state,mutation in itertools.product((1,9,10,11,99,100,101,0x80000000,0xffffffff),positions(),(0,2,4,6,255),(0,1),(0,2,4),(0,2)):
        row=base.copy();row[1:3]=3,count;row[6:11]=spot,*point,boss,state;row[13]=mutation;rows.append(tuple(row))
    # 좌표색인은제품계산으로주입하지않는다. 지도의각바이트를고정패턴/xor입력으로채운다.
    for point,spot in itertools.product(positions(),range(256)):
        row=base.copy();row[0]=1;row[6:9]=spot,*point;row[14]=1;rows.append(tuple(row))
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """9개실제몸체를실행하고할당/Regular/프레임/가상효과/Damageable만경계로둔다."""
    def __init__(self):
        """실제float10→callerfloat32와기본몸체float반환만격리스텁으로연결한다."""
        super().__init__();self.mu.mem_map(POOL,0x10000);self.mu.mem_map(SPOTS,0x10000);self.allowed=[];ranges(self,(0x485230,0x4850a0,0x485170,0x484ec0,0x485fe0,0x49a840,0x4ac550,0x4558f0,0x4e49c0));self.instructions=0;self.calls=collections.Counter()
        self.mu.mem_write(CALLER,b'\xd9\x1d'+struct.pack('<I',RESULT)+b'\xe9'+struct.pack('<i',STOP-(CALLER+11)));self.mu.mem_write(FLOAT12,b'\xd9\x05'+struct.pack('<I',FLOAT_VALUE)+b'\xc2\x0c\0')
        # 원본VT와재진입이바꾸는VT의효과만서로다른격리경계로연결한다.
        for table,refresh,pop in ((0x50dc30,REFRESH,POP),(ALT_VT,OTHER_REFRESH,OTHER_POP)):
            self.put(table+0x10,DESTROY);self.put(table+0x48,UNPOP);self.put(table+0x90,pop);self.put(table+0x88,refresh);self.put(table+0xb4,0x485fe0)
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read);self.pattern=bytes(((i*17)^((i>>8)*93))&255 for i in range(65536))

    def instruction(self,mu,address,size,data):
        """원본이선택한현재효과를기록한다. 정수게임난수와_ftol은대체하지않는다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<3I',mu.mem_read(sp+4,12));this=mu.reg_read(UC_X86_REG_ECX);sid=(this-POOL)//50
        if address in (0x485230,0x4850a0,0x485170,0x484ec0,0x485fe0,0x49a840,0x4ac550,0x4558f0,0x4e49c0):self.calls[f'{address:08x}']+=1
        if CALLER<=address<CALLER+11 or FLOAT12<=address<FLOAT12+9:return
        if address==0x4558f0:self.event('I',*args[:2])
        if address==0x4acee0:
            self.event('F',sid,*args[:2]);self.put(this+36,args[0])
            if self.case[13]==1:self.put(this,ALT_VT);self.put(this+10,155,1)
            self.ret(0,8);return
        if address in (REFRESH,OTHER_REFRESH):self.event('R' if address==REFRESH else 'X',sid);self.ret();return
        if address==0x4e4391:self.event('A',args[0]);self.ret(0x13f00000 if self.case[11] else 0);return
        if address==0x496e80:self.event('C',*args);self.ret(this,12);return
        if address==DESTROY:self.event('D',sid,args[0]);self.put(this+11,(self.get(this+11)&255)|2,1);self.ret(0,4);return
        if address==UNPOP:
            self.event('U',sid,args[0]);self.put(this+11,(self.get(this+11)&255)|4,1)
            if self.case[13]==2:self.put(this,ALT_VT);self.put(this+14,bits(42.5));self.put(this+18,bits(43.5))
            self.ret(0,4);return
        if address in (POP,OTHER_POP):
            self.event('P' if address==POP else 'Y',sid,*args);self.put(this+14,args[0]);self.put(this+18,args[1]);self.put(this+11,(self.get(this+11)&255)&~4,1);self.ret(0,12);return
        if address==0x44b900:self.event('B',sid,*args);self.put(FLOAT_VALUE,self.case[3]);mu.reg_write(UC_X86_REG_EIP,FLOAT12);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'너겟Regular범위밖실행{address:08x}')

    def write(self,mu,access,address,size,value,data):
        """실제쓰기를z1BYTE/RNG/SEH/caller결과/스택으로제한한다."""
        if STACK-0x7f000<=address<=STACK+0x1000-size or address==0 and size==4 or address in (RESULT,0x532710) and size==4 or address==POOL+50*50+35 and size==1:return
        raise RuntimeError(f'너겟Regular범위밖쓰기{address:08x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """보호이미지/TYPE/raw/지도/호출자/스택만읽게한다."""
        regions=((0,4),(0x400000,0x600000),(POOL,POOL+0x10000),(TYPES,TYPES+188*500),(SPOTS,SPOTS+65536),(STOP,STOP+0x1000),(STACK-0x7f000,STACK+0x1000))
        if not any(low<=address and address+size<=high for low,high in regions):raise RuntimeError(f'너겟Regular범위밖읽기{address:08x}+{size}')

    def invoke(self,entry,args,precision,floating):
        """retN/보존GP/canary/제어어/빈x87스택을실제caller경계에서검사한다."""
        registers={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x3456789a,UC_X86_REG_EBP:0x456789ab}
        self.mu.mem_write(STACK,struct.pack('<I',CALLER if floating else STOP)+args+b'\xc0\xde\xc0\xde')
        # 독립입력마다레지스터와FPU를새로설정한다.
        for register,value in registers.items():self.mu.reg_write(register,value)
        # 호출규약의휘발레지스터와제어/상태/태그를독립초기화한다.
        for register,value in ((UC_X86_REG_EAX,0),(UC_X86_REG_EDX,0),(UC_X86_REG_ECX,POOL+50*50),(UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,precision),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):
            self.mu.reg_write(register,value)
        self.mu.emu_start(entry,STOP,count=1000000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+len(args)
        assert bytes(self.mu.mem_read(STACK+4+len(args),4))==b'\xc0\xde\xc0\xde'
        # 모든비휘발레지스터가원래sentinel로돌아왔는지확인한다.
        for register,value in registers.items():assert self.mu.reg_read(register)==value
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision and not self.mu.reg_read(UC_X86_REG_FPSW)&0x3800 and self.mu.reg_read(UC_X86_REG_FPTAG)==0xffff

    def sample(self,case,precision):
        """예상좌표/프레임/난수를계산하지않고입력과실제반환/raw/RNG를보존한다."""
        self.case=case;self.events=[];raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,0x50dc30);raw[10]=154;raw[11]=case[10];struct.pack_into('<II',raw,14,*case[7:9]);struct.pack_into('<I',raw,36,case[4]);self.mu.mem_write(POOL+50*50,bytes(raw))
        self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES);self.put(TYPES+154*500+0x170,case[5]);self.put(0x5c7c44,SPOTS);self.put(0x540bc4,case[9]);self.put(0x532710,case[12]);self.put(0,0x87654321)
        board=self.pattern.translate(bytes(value^case[6] for value in range(256))) if case[14] else bytes([case[6]])*65536;self.mu.mem_write(SPOTS,board)
        if case[0]==0:self.invoke(0x485230,struct.pack('<3I',*case[1:4]),precision,True);result=self.get(RESULT)
        else:self.invoke(0x485fe0,b'',precision,False);result=self.mu.reg_read(UC_X86_REG_EAX)
        self.returns+=1;assert self.get(0)==0x87654321
        return str(result),';'.join(self.events) or '-',bytes(self.mu.mem_read(POOL+50*50,50)).hex(),str(self.get(0x532710))


def main():
    """독립관측을기록하거나기존입력/기대값/SHA만변경없이감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text('utf-8').splitlines() if r and not r.startswith('#')]
        # 보호원본/도구/원문관측의바이트해시를각각확인한다.
        for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
        assert [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()] and len(rows)==report['uniqueInputs'] and 2*len(rows)==report['normalReturns']
        assert [i for i,r in enumerate(rows) if r[1:5]!=r[5:9]]==report['precisionDifferentInputs']
        print('너겟Regular원본감사:',len(rows),'입력/',report['normalReturns'],'정상반환');return
    oracles=[Oracle(),Oracle()];rows=[];different=[]
    # 두정밀도를각각실행하며제품C++나분기계산을기대값으로사용하지않는다.
    for index,case in enumerate(cases()):
        outputs=[o.sample(case,p) for o,p in zip(oracles,(0x27f,0x37f))]
        if outputs[0]!=outputs[1]:different.append(index)
        rows.append((','.join(map(str,case)),*outputs[0],*outputs[1]))
    FIXTURE.write_text('# input return53 effects53 raw53 rng53 return64 effects64 raw64 rng64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_nugget_regular.py','tools/decomp_cursor_lifecycle.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/nugget-regular-x86.tsv')
    report=dict(host=socket.gethostname(),date='2026-10-17',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),controls=[0x27f,0x37f],uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),precisionDifferentInputs=different,boundaries='485230/4850a0/485170/484ec0/485fe0/TYPEgetter/4558f0/_ftol전체실행. 할당/Regular생성/프레임/표시/Unpop/Pop/삭제/Damageable는명시경계. mapPattern은고정입력이며좌표색인을대신계산하지않는다. 실제callerfloat32/GP/스택/canary/SEH/x87/코드·읽기·쓰기범위검사. free부모/지도물리밖조회는입력범위아님. 원본/OS창실행없음.',sha256={p:digest(ROOT/p) for p in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('uniqueInputs','normalReturns','instructions','nativeCalls','precisionDifferentInputs')},ensure_ascii=False))


if __name__=='__main__':main()
