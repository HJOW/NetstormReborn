#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ResetClock13BYTE와기존공개시계호출열을제품전PE실행으로독립고정한다."""
import argparse
from collections import Counter
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TEXT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

# 기존현재clock범위와추가메모리는이원문전체와겹치지않는다.
CLOCK, CLOCK_SIZE = 0x55b4a0, 96
OUTPUT, TIME = POOL+0x8000, TEXT+0x1000
RANDOMS = (0x532710,0x5c89a4,0x5c89a8,0x5c89b4)
ENTRIES = {'Game':0x460cd0,'Wall':0x460d70,'IsPaused':0x460de0,'Pause':0x460df0,'Resume':0x460e30,'Reset':0x460e70}
RANGES = ((0x460cd0,0x460d67),(0x460d70,0x460dd2),(0x460de0,0x460dee),(0x460df0,0x460e23),(0x460e30,0x460e68),(0x460e70,0x460e7d))
FIXTURE = ROOT/'cpppj/tests/fixtures/game-clock-reset-x86.tsv'
REPORT = ROOT/'cpppj/game-clock-reset-native.json'


def public_cases():
    """기존GameClock표현범위인epoch/adjustment0의공개호출입력만정한다."""
    return [
        ('running',1000,'Wall:1500|Game:1500|Pause:2000|Resume:5000|Game:6000|Reset:0|Game:6000|Wall:6000|IsPaused:6000'),
        ('paused',1000,'Pause:2000|Resume:5000|Pause:6000|Reset:0|Game:9000|IsPaused:9000|Pause:10000|Reset:0|Resume:11000|Game:12000|Resume:13000|Game:14000'),
        ('nested',0,'Pause:1000|Pause:2000|Resume:3000|Reset:0|Game:4000|IsPaused:4000|Resume:5000|Game:6000|Reset:0|Game:6000'),
        ('repeated',0,'Pause:1000|Resume:4000|Game:5000|Reset:0|Reset:4294967295|Game:5000|Resume:9000|Wall:9000'),
        ('fresh',0,'Reset:0|IsPaused:0|Game:0|Wall:0|Reset:9999|Game:1|Game:999|Game:1001'),
        ('wrap',0xfffffff0,'Pause:16|Pause:32|Resume:48|Reset:0|Game:64|Resume:80|Game:96|Reset:4294967295|Game:96|Wall:112'),
    ]


class Oracle(MemoryOracle):
    """기존시계/Reset전체PE를CPU실행하고timeGetTime만명시DWORD장치로둔다."""
    def __init__(self):
        """원문/전체clock/독립출력/스택과같은현재시각장치를구성한다."""
        super().__init__();self.running=False;self.put(0x500394,TIME);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def write(self,mu,access,address,size,value,data):
        """시계현재전역/출력/스택밖쓰기를거부해난수나원점을다른전역으로바꾸지못하게한다."""
        if self.running and not any(first<=address and address+size<=last for first,last in ((CLOCK,CLOCK+CLOCK_SIZE),(OUTPUT,OUTPUT+8),(STACK-4096,STACK+64))):
            raise RuntimeError('ResetClock예상밖CPU쓰기:'+hex(address))

    def instruction(self,mu,address,size,data):
        """현재원문명령과명시timeGetTime호출수만기록한다."""
        self.instructions+=1
        if self.instructions>500:raise RuntimeError('ResetClock명령상한')
        if address==TIME:self.timeCalls+=1;self.ret(self.now);return
        if not any(first<=address<last for first,last in RANGES):raise RuntimeError('ResetClock허용밖명령:'+hex(address))

    def initialize(self,raw):
        """계속되는호출열의최초실제전역을한번만준비한다."""
        self.running=False;self.mu.mem_write(CLOCK,raw)
        for index,address in enumerate(RANDOMS):self.put(address,0x89abcdef+index*17)

    def sample(self,operation,now,control):
        """한호출전체ret/비휘발/정밀도/TOP와모든현재전역을보존한다."""
        self.instructions=self.timeCalls=0;self.now=now;self.running=False;self.mu.mem_write(OUTPUT,b'\xa5'*8)
        self.mu.mem_write(STACK-4096,b'\xcd'*(4096+64));self.mu.mem_write(STACK,struct.pack('<II',STOP,OUTPUT))
        for register,value in ((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,0),(UC_X86_REG_EDX,0),(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)):
            self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.running=True
        self.mu.emu_start(ENTRIES[operation],STOP);self.running=False
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('ResetClockcdecl스택차이')
        for register,value in ((UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)):
            if self.mu.reg_read(register)!=value:raise RuntimeError('ResetClock비휘발차이')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or (self.mu.reg_read(UC_X86_REG_FPSW)>>11)&7:raise RuntimeError('ResetClockFPU균형차이')
        result=bytes(self.mu.mem_read(OUTPUT,8)).hex() if operation in ('Game','Wall') else str(self.mu.reg_read(UC_X86_REG_EAX)) if operation=='IsPaused' else '-'
        clock=bytes(self.mu.mem_read(CLOCK,CLOCK_SIZE)).hex();randoms=''.join(bytes(self.mu.mem_read(address,4)).hex() for address in RANDOMS)
        return result,clock,randoms,str(self.timeCalls),str(self.instructions)


def main():
    """최초fixture/report덮어쓰기를거부하고추가Reset제품이없을때만관측한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for relative,expected in report['sha256'].items():
            if digest(ROOT/relative)!=expected:raise RuntimeError('ResetClock최초SHA차이:'+relative)
        print(json.dumps({'verified':True,'observations':report['observations']}));return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('ResetClock최초자료덮어쓰기금지')
    if 'void Reset(' in (ROOT/'cpppj/src/o/GameClock.h').read_text(encoding='utf-8') or (ROOT/'cpppj/tests/GameClockResetTests.cpp').exists():raise RuntimeError('ResetClock제품전관측요건위반')
    oracle=Oracle();inputs=[]
    for index in range(12):
        raw=bytearray((index*19+byte*11+37)&255 for byte in range(CLOCK_SIZE));struct.pack_into('<I',raw,16,(0,1,2,0x80000000,0xffffffff)[index%5]);struct.pack_into('<Q',raw,88,(0,0x8000000000000000,0x3ff0000000000000,0x7ff0000000000000,0x7ff8123456789abc,0xffffffffffffffff)[index%6])
        inputs.append(('raw'+str(index),int.from_bytes(raw[:4],'little'),bytes(raw),'Reset:0'))
    for name,origin,actions in public_cases():
        raw=bytearray(CLOCK_SIZE);struct.pack_into('<I',raw,0,origin);struct.pack_into('<I',raw,8,0xffffffff);struct.pack_into('<3d',raw,48,17.25,19.5,-2.25)
        inputs.append((name,origin,bytes(raw),actions))
    rows=['# control / sequence / origin / initialClock96 / actions / step / operation / now / result / clock96 / random16 / timeCalls / instructions'];instructions=resetCalls=0;precisionDifferences=[];operations=Counter()
    for name,origin,initial,actions in inputs:
        pair=[]
        for control in (0x27f,0x37f):
            oracle.initialize(initial);observed=[]
            for step,action in enumerate(actions.split('|')):
                operation,now=action.split(':');before=bytes(oracle.mu.mem_read(CLOCK,CLOCK_SIZE));outputs=oracle.sample(operation,int(now),control);instructions+=oracle.instructions;operations[operation]+=1
                if operation=='Reset':
                    resetCalls+=1;expected=before[:88]+bytes(8)
                    if bytes.fromhex(outputs[1])!=expected or oracle.timeCalls:raise RuntimeError('ResetClock단일double0쓰기불변차이')
                observed.append((name,str(origin),initial.hex(),actions,str(step),operation,now,*outputs))
                rows.append('\t'.join((str(control),*observed[-1])))
            pair.append(observed)
        if pair[0]!=pair[1]:precisionDifferences.append(name)
    FIXTURE.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_game_clock_reset.py',FIXTURE.relative_to(ROOT).as_posix())
    report={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','entry':'00460e70','bodyBytes':13,'cdSourceMapAddress':None,'cdExecuted':False,'rawInputs':12,'publicSequences':len(public_cases()),'observations':len(rows)-1,'resetCalls':resetCalls,'instructions':instructions,'operations':dict(operations),'precisionDifferentSequences':precisionDifferences,'productAbsentBeforeObservation':True,'scope':'Reset13BYTE와기존Game/Wall/Pause/Resume/IsPaused실제PE. timeGetTime만명시응답. raw96BYTE불변과epoch/adjustment0의공개호출열은구별. Capture/FPS/전체clock초기화/난수실행없음.','sha256':{relative:digest(ROOT/relative) for relative in paths}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:report[key] for key in ('observations','resetCalls','instructions','precisionDifferentSequences')}))


if __name__=='__main__':main()
