#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Core9기록의실제PE를제품전에고정하고별도43b540표도귀속만감사한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

# Core9기록과lastKnown10/별도owner색표를분리된물리의전체BYTE로관측한다.
RECORDS, WINDOW, CORE, COLORS = 0x595360,0x595320,POOL+0x2000,0x531b08
ENTRIES = (0x42ce70,0x48f350,0x4feac0,0x4feaf0,0x43b540)
FIXTURE=ROOT/'cpppj/tests/fixtures/player-core-records-x86.tsv'
REPORT=ROOT/'extracted/cpp-player-core-records-20261024-native.json'


def cases():
    """반환기대없이초기전체패턴/주소산술/원문값인자만준비한다."""
    rows=[]
    for mode,seed in itertools.product((0,2,3,4),(0,165,255)):
        rows.append((mode,seed,0,0,0))
    for seed,owner,subscriber,core_seed in itertools.product((0,165),(0,1,8,9,0x10000000,0xffffffff,0xfffffffe),(0,0xffffffff,0x12345678),(0,127)):
        rows.append((1,seed,owner,subscriber,core_seed))
    return rows


class Oracle(MemoryOracle):
    """실제다섯몸체의직접store만관측하고외부계산장치를사용하지않는다."""
    def __init__(self):
        """보호PE/격리물리/정밀함수범위를읽기만한다."""
        super().__init__();self.running=False;self.total=0
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as file:
            facts={int(row['entry'],16):row for row in csv.DictReader(file,delimiter='\t')}
        self.ranges=[]
        for entry in ENTRIES:  # 문서의추측주소가아닌같은정밀함수범위를그대로허용한다.
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.ranges.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def write(self,mu,access,address,size,value,data):
        """순서감사는직접CPUstore의주소/폭/값을원시로남긴다."""
        if not self.running or STACK-4096<=address<address+size<=STACK+32:return
        if WINDOW<=address<address+size<=WINDOW+256:token='R'+str(address-RECORDS)
        elif CORE<=address<address+size<=CORE+16:token='C'+str(address-CORE)
        elif COLORS-4<=address<address+size<=COLORS+40:token='O'+str(address-COLORS)
        else:raise RuntimeError('Core장부원문store범위밖:'+hex(address))
        self.trace.append(f'{token}:{size}:{value&((1<<(size*8))-1)}')

    def instruction(self,mu,address,size,data):
        """외부hook없이원문의모든계산/반복을CPU로수행한다."""
        self.instructions+=1;self.total+=1
        if self.instructions>2000:raise RuntimeError('Core장부원문명령상한')
        if not any(low<=address<high for low,high in self.ranges):raise RuntimeError('Core장부범위밖명령:'+hex(address))

    def sample(self,case,control):
        """인접padding/lastKnown/색표를포함한초기물리와최종전체를고정한다."""
        mode,seed,owner,subscriber,core_seed=case;self.running=False;self.trace=[];self.instructions=0
        self.mu.mem_write(WINDOW,bytes((seed+byte*17)&255 for byte in range(256)))
        self.mu.mem_write(CORE,bytes((seed+byte*13)&255 for byte in range(16)))
        self.mu.mem_write(COLORS-4,bytes((seed+byte*7)&255 for byte in range(44)))
        core=bytes((core_seed+byte*23)&255 for byte in range(10));first,second=struct.unpack('<II',core[:8]);last=int.from_bytes(core[8:],'little')
        arguments=(owner,subscriber,first,second,last) if mode==1 else ()
        self.mu.mem_write(STACK-4096,bytes(4128))
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,CORE if mode==0 else 0);self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(1+len(arguments)),STOP,*arguments));entry=(0x42ce70,0x48f350,0x4feac0,0x4feaf0,0x43b540)[mode]
        self.running=True;self.mu.emu_start(entry,STOP);self.running=False
        if self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Core장부원문스택복귀')
        return '\t'.join((str(control),','.join(map(str,case)),bytes(self.mu.mem_read(WINDOW,256)).hex(),bytes(self.mu.mem_read(CORE,16)).hex(),bytes(self.mu.mem_read(COLORS-4,44)).hex(),';'.join(self.trace),str(self.instructions)))


def main():
    """제품전최초자료만한번저장하고후속호출은입력SHA만검증한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in report['sha256'].items():assert digest(ROOT/name)==expected,name
        print(json.dumps({'verified':True,'observations':report['observations']}));return
    assert not FIXTURE.exists() and not REPORT.exists(),'Core장부최초자료덮어쓰기금지'
    for name in ('cpppj/src/o/RawPlayerCoreRecords.h','cpppj/src/o/RawPlayerCoreRecords.cpp','cpppj/tests/PlayerCoreRecordsTests.cpp'):
        assert not (ROOT/name).exists(),'Core장부제품전선관측위반'
    inputs=cases();oracle=Oracle();lines=['# control / mode,initialSeed,owner,subscriber,coreSeed / recordsWindow256 / standaloneCore16 / unrelatedColors44 / CPUstoreTrace / instructions'];modes=Counter()
    for case in inputs:  # 양x87모드의실제원시출력이같음을비교한다.
        first,second=oracle.sample(case,0x27f),oracle.sample(case,0x37f);assert first.split('\t')[1:]==second.split('\t')[1:]
        lines.extend((first,second));modes[case[0]]+=2
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_player_core_records.py','cpppj/tests/fixtures/player-core-records-x86.tsv')
    report={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','sourceAbsentBeforeObservation':True,'entries':{'42ce70':16,'48f350':44,'4feac0':39,'4feaf0':10,'43b540':91},'uniqueInputs':len(inputs),'observations':len(lines)-1,'modeCounts':dict(modes),'instructions':oracle.total,'precisionDifferences':0,'gameOrOsExecuted':False,'scope':'Core44BYTEmetadata/16BYTEinit/39BYTE9recordCRT/10BYTElastKnownCRT를구별한다.43b540은별도ownerColors9DWORD identity표로귀속만관측하며Core모듈본문에포함하지않는다.','sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps({'observations':report['observations'],'modes':dict(modes),'instructions':oracle.total}))


if __name__=='__main__':main()
