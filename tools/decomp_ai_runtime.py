#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본전체AI행동/프레임/Player순회수명과명시하위의순서를동결한다."""
import csv
import itertools
import json
import socket
import struct

from decomp_ai_bridge_initialize import BridgeInitializeOracle, ROOT, BODY, STACK, STOP, SIZE, digest
from decomp_ai_bridge_start import bits
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EBP, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

ENTRIES=(0x417C70,0x417D30,0x417DE0)  # 전체행동/개별Tick/자동Frame의실제진입이다.
BOUNDARIES={0x414AF0:'initialize',0x416EB0:'prepare',0x4139F0:'select',0x415A10:'collectors',0x4136B0:'factories',0x40F2B0:'dry',0x411CD0:'priest',0x4162E0:'drops',0x417BD0:'bridges',0x412130:'carriers',0x4138C0:'economy',0x40EF30:'money',0x460D70:'wall',0x4105B0:'log',0x4E0620:'assert',0x410100:'destruct',0x4E4354:'free'}  # 별도복원한하위의명시입력/출력경계다.
PLAYERS=0x20000000  # 실제9×180BYTE입력이다.
RETURN_FLOAT=STOP+0x100  # FPU반환을실제FLD/RET두명령으로공급한다.
FLOAT_VALUE=BODY+0x7000  # caller가제공한SPfloat입력버퍼다.
FIXTURE=ROOT/'cpppj/tests/fixtures/ai-runtime-x86.tsv'  # 기존기대를변경하지않는새관측이다.
REPORT=ROOT/'cpppj/ai-runtime-20261022.json'  # 실제범위/경계/출처를명시한다.


def put(raw,at,value):
    """명시한DWORD만주변바이트보존으로쓴다."""
    struct.pack_into('<I',raw,at,value&0xFFFFFFFF)


def qbits(value):
    """벽시각/로그인자의원문QWORD다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


class RuntimeOracle(BridgeInitializeOracle):
    """원본세몸체와명시된행동/시계/수명경계만격리실행한다."""

    def __init__(self):
        """원본몸체·Player·float반환용계산버퍼와검토범위를준비한다."""
        super().__init__();self.mu.mem_map(BODY+0x2000,0x6000);self.mu.mem_map(PLAYERS,0x1000)
        self.mu.mem_write(RETURN_FLOAT,b'\xd9\x05'+struct.pack('<I',FLOAT_VALUE)+b'\xc3')
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        self.ranges=[]
        for entry in ENTRIES:
            for part in facts[entry]['ranges'].split(';'):
                lo,hi=(int(word,16) for word in part.split('-'));self.ranges.append((lo,hi+1))
        self.counts={}

    def instruction(self,mu,address,size,data):
        """행동효과입력은명시경계로공급하고원본분기/비교/순회는수정하지않는다."""
        if address in ENTRIES:
            self.counts[hex(address)]=self.counts.get(hex(address),0)+1
            if address!=ENTRIES[2]:self.token=mu.reg_read(UC_X86_REG_ECX)
        if address in BOUNDARIES:
            esp=mu.reg_read(UC_X86_REG_ESP);a=struct.unpack('<6I',mu.mem_read(esp,24));ret=a[0];name=BOUNDARIES[address];purge=0;result=0;token=self.token;effects=self.cfg[7]
            if name in ('initialize','prepare','select','collectors','factories','dry','priest','drops','bridges','carriers','economy'):
                text=f'{name},{token}'
                if name=='priest':text+=f',{a[1]}';purge=4
                self.trace.append(text)
                if name=='initialize':mu.mem_write(token+0x258,struct.pack('<I',self.cfg[2]))
                if name=='prepare' and effects&1:mu.mem_write(token+0x258,bytes(4))
                if name=='collectors' and effects&2:mu.mem_write(token+0x29C,struct.pack('<I',37))
                if name=='dry':
                    result=self.cfg[3]
                    if effects&4:mu.mem_write(token+0x280,struct.pack('<I',result))
                if name=='priest' and effects&8:mu.mem_write(token+0x260,struct.pack('<I',3))
                if name=='economy' and effects&16:mu.mem_write(token+0x2D8,struct.pack('<I',self.cfg[8]))
                if name=='drops' and effects&64:mu.mem_write(token+0x254,struct.pack('<I',1))
                if name=='bridges' and effects&128:mu.mem_write(0x52F3C4,struct.pack('<I',1))
            elif name=='money':
                self.trace.append(f'money,{a[1]}')
                if effects&32:mu.mem_write(token+0x2D8,bytes(4))
                mu.mem_write(FLOAT_VALUE,struct.pack('<I',self.cfg[4]));mu.reg_write(UC_X86_REG_EIP,RETURN_FLOAT);return
            elif name=='wall':
                self.trace.append(f'wall,{self.walls}');value=self.cfg[5 if self.walls%2==0 else 6];self.walls+=1;mu.mem_write(a[1],struct.pack('<Q',value));result=a[1]
            elif name=='log':
                if a[3] in (0x5014CC,0x5014E4):self.trace.append(f'dryNotice,{a[1]},{a[2]},{int(a[3]==0x5014CC)}')
                else:
                    assert a[3]==0x501948;value=a[4]|(a[5]<<32);self.trace.append(f'took,{a[1]},{value}')
            elif name=='assert':self.trace.append(f'assert,{a[3]}')
            elif name=='destruct':self.trace.append(f'destruct,{mu.reg_read(UC_X86_REG_ECX)}')
            elif name=='free':self.trace.append(f'free,{a[1]}');self.freed.add(a[1])
            mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret);return
        if RETURN_FLOAT<=address<RETURN_FLOAT+7:return
        if not any(lo<=address<hi for lo,hi in self.ranges):raise RuntimeError(f'검토범위밖명령 {address:08x}')

    def sample(self,mode,bodies,players,cfg,precision):
        """세진입의전체살아있는본문/Player/수명/순서와정상ABI를관측한다."""
        self.cfg=cfg;self.trace=[];self.freed=set();self.walls=0;self.token=BODY
        self.mu.mem_write(BODY,bytes(bodies));self.mu.mem_write(PLAYERS,bytes(players));self.mu.mem_write(0x59534C,struct.pack('<I',PLAYERS));self.mu.mem_write(0x594FBC,struct.pack('<I',cfg[0]));self.mu.mem_write(0x52F3C4,struct.pack('<I',cfg[1]));self.mu.mem_write(0x52F3DC,struct.pack('<I',99))
        result=0
        for _ in range(cfg[9]):  #같은현재Player와본문으로다음실제프레임을재개한다.
            self.mu.mem_write(STACK,struct.pack('<I',STOP));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,BODY);self.mu.reg_write(UC_X86_REG_FPCW,precision);self.mu.reg_write(UC_X86_REG_FPSW,0);self.mu.reg_write(UC_X86_REG_FPTAG,0xFFFF)
            for register,value in ((UC_X86_REG_EBX,0x12345678),(UC_X86_REG_EBP,0x456789AB),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789A)):self.mu.reg_write(register,value)
            self.mu.emu_start(ENTRIES[mode],STOP,count=100000)
            assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4 and self.mu.reg_read(UC_X86_REG_FPCW)==precision
            for register,value in ((UC_X86_REG_EBX,0x12345678),(UC_X86_REG_EBP,0x456789AB),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789A)):assert self.mu.reg_read(register)==value
            result=self.mu.reg_read(UC_X86_REG_EAX) if mode==1 else 0
        normalized=b''.join(bytes(SIZE) if BODY+index*SIZE in self.freed else bytes(self.mu.mem_read(BODY+index*SIZE,SIZE)) for index in range(2))  #반납된본문은부재0으로정규화하고해제메모리의내용일치를주장하지않는다.
        return normalized.hex(),bytes(self.mu.mem_read(PLAYERS,len(players))).hex(),sum(1<<index for index in range(2) if BODY+index*SIZE in self.freed),result,';'.join(self.trace)


def scene(mode):
    """명시본문두개/Player9/하위입력을만든다.결과는원본이결정한다."""
    bodies=bytearray([0xA5])*(SIZE*2);players=bytearray([0x5A])*(180*9)
    for index in range(2):
        for at,value in ((0x254,0),(0x258,1),(0x280,70),(0x29C,22),(0x260,1 if index==0 else 8),(0x2D8,123)):put(bodies,index*SIZE+at,value)
    for owner in range(9):put(players,owner*180+0xB0,0)
    for owner,token in ((0,BODY),(1,BODY),(8,BODY+SIZE)):put(players,owner*180+0xB0,token)
    cfg=[1,1,1,0,bits(123),qbits(100),qbits(100.05),0,16777217,1]
    return mode,bodies,players,cfg


def main():
    """행동순서/float지갑/기한로그/초기화/삭제/전투관문을두정밀도로동결한다."""
    assert not FIXTURE.exists() and not REPORT.exists();cases=[]
    for wallet,variant in itertools.product((0,123,16777217,0x7FFFFFFF,0x80000000,0xFFFFFFFF),range(3)):
        row=scene(0);put(row[1],0x2D8,wallet);signed=wallet if wallet<0x80000000 else wallet-0x100000000;money=float(signed) if variant==0 else float(signed)+512 if variant==1 else float('nan');row[3][4]=bits(money);cases.append(row)
    for effect,dry in itertools.product((0,1,2,4,8,16,32,64,128,255),(0,70,71)):
        row=scene(1);row[3][3]=dry;row[3][7]=effect
        if effect&16:row[3][4]=bits(16777216)
        if effect&128:row[3][1]=0;row[3][6]=qbits(100.2)
        cases.append(row)
    for quitFlag,initialized,completion,timing,clock in itertools.product((0,1),(0,1),(0,1),(0,1),((0.0,0.1),(100.0,100.05),(100.0,100.2),(100.0,float('nan')))):
        row=scene(1);put(row[1],0x254,quitFlag);put(row[1],0x258,initialized);row[3][1]=timing;row[3][2]=completion;row[3][5:7]=map(qbits,clock);cases.append(row)
    for battle,timing,first,second,repeats in itertools.product((0,1),(0,1),(0,1),(0,1),(1,2)):
        row=scene(2);row[3][0:2]=[battle,timing];row[3][9]=repeats;put(row[1],0x254,first);put(row[1],SIZE+0x254,second);cases.append(row)
    row=scene(2);row[3][7]=64;row[3][9]=2;cases.append(row)
    oracle=RuntimeOracle();rows=[]
    for index,(mode,bodies,players,cfg) in enumerate(cases):
        observed=[oracle.sample(mode,bodies,players,cfg,precision) for precision in (0x27F,0x37F)];assert observed[0]==observed[1]
        rows.append((index,mode,','.join(map(str,cfg)),bodies.hex(),players.hex(),*observed[0]))
    with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# 원본AI:번호/진입/하위입력/본문입력/Player입력/정규화본문출력/Player출력/해제mask/Tick반환/순서\n');stream.writelines('\t'.join(map(str,row))+'\n' for row in rows)
    report={'host':socket.gethostname(),'decompile_host':'VM-W11-Codex','cases':len(rows),'normal_runs':len(rows)*2,'x87':[0x27F,0x37F],'actual_entries':oracle.counts,'boundary_stubs':{hex(key):value for key,value in BOUNDARIES.items()},'freed_body_normalization':'absent body is zero; freed bytes are not compared','game_started':False,'sources':{str(path.relative_to(ROOT)):digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'originalCD/NETSTORM.EXE',ROOT/'tools/decomp_ai_runtime.py',FIXTURE)}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps({'cases':len(rows),'runs':len(rows)*2}))


if __name__=='__main__':main()
