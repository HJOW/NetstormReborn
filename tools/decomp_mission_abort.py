#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""미션 Abort의 실제 사건/큐와 세 제어 명령의 실제 문자열 조회·토큰/큐를 독립 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_script_command import Oracle as ScriptOracle, MemoryOracle, ROOT, POOL, TEXT, TYPES, STACK, STOP, THREAD, digest
from decomp_cursor_position import ranges
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

GUIDE,MISSION,TABLE,EVENT=POOL+0x80000,POOL+0x82000,POOL+0x84000,POOL+0x86000  # 다른 원문 입력/할당과 겹치지 않는 Guide/미션/표/사건이다.
INPUT=STOP+0x300  # 하위 미션 입력의 명시 가상 경계다.
FIXTURE=ROOT/'cpppj/tests/fixtures/mission-abort-x86.tsv'  # 제품 작성 전에 동결하는 새 원문이다.
REPORT=ROOT/'cpppj/mission-abort-native.json'  # 실제 반환과 외부 효과 경계를 구분한다.


class Oracle(ScriptOracle):
    """원본 Abort 사건·두 큐 함수와 기존 원본 명령 parser/조회/CRT를 실행한다."""
    def __init__(self):
        """기존 격리 PE 로더/원본 C locale만 재사용하고 OS/게임은 시작하지 않는다."""
        MemoryOracle.__init__(self);self.operation='S';self.allowed=[]
        ranges(self,(0x463e40,0x487620,0x487710,0x441580,0x4c9090,0x4c7910,0x4d52c0,0x452fe0,0x452f90,0x4437f0,0x4e63e0,0x4e6fa1,0x4e6656,0x4e65ed,0x4ef510))
        self.put(THREAD+0x64,self.words(0x543efc,1)[0]);self.reset_ring();self.allocations=0;self.abortReturns=0;self.queueCalls=0
        self.put(MISSION,TABLE);self.put(TABLE+0x24,INPUT);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self,mu,access,address,size,value,data):
        """원본 쓰기는 명시 입력/할당·토큰/고리/큐·스택 영역으로 제한한다."""
        allowed=((POOL,POOL+0x100000),(TEXT,TEXT+0x10000),(TYPES,TYPES+0x20000),(STACK-0x1f000,STACK+0x1000),(0x55a508,0x55a6f0),(0x5ca9e8,0x5cab2c))
        if not any(low<=address and address+size<=high for low,high in allowed):raise RuntimeError(f'제어 명령 쓰기 범위 밖 {address:08x}/{size}')

    def instruction(self,mu,address,size,data):
        """Tell/하위 사건만 명시 공급하고 Abort 분기와 실제 큐 쓰기는 원본에 맡긴다."""
        if address==0x452fe0:self.queueCalls+=1
        if self.operation=='A':
            if address==0x4cf960:
                self.events.append('tell:'+self.string(self.words(mu.reg_read(UC_X86_REG_ESP)+4,1)[0]).hex());self.ret(0);return
            if address==INPUT:
                if mu.reg_read(UC_X86_REG_ECX)!=MISSION:raise RuntimeError('하위 사건 대상 오류')
                pointer=self.words(mu.reg_read(UC_X86_REG_ESP)+4,1)[0]
                self.events.append('input:'+':'.join(map(str,self.words(pointer,3))));self.ret(self.case[5],4);return
        super().instruction(mu,address,size,data)

    def abort(self,case):
        """사건의 세 DWORD와 큐 포화를 독립 공급하고 정상 반환/전체 활성 큐를 읽는다."""
        self.operation='A';self.case=case;self.events=[];active,code,x,y,filled,result=case
        self.put(GUIDE+0x800,MISSION if active else 0);self.mu.mem_write(EVENT,struct.pack('<3I',code,x,y))
        self.put(0x55a508,0);self.put(0x55a50c,filled)
        # 기존 활성 큐는 DEL 치환과 겹치지 않는 독립 번호/인자다.
        for index in range(40):self.mu.mem_write(0x55a510+index*12,struct.pack('<3I',0x300+index,index+10,index+20))
        value=self.call(0x463e40,[EVENT],GUIDE,4);self.abortReturns+=1
        read,write=self.words(0x55a508,2);items=[]
        # 원본의 현재 읽기/쓰기 사이만 읽으며 기대 큐 길이를 다시 계산하지 않는다.
        while read!=write:
            items.append(':'.join(map(str,self.words(0x55a510+read*12,3))));read=(read+1)%40
        return str(value)+'|'+(';'.join(items) or '-')+'|'+(';'.join(self.events) or '-')


def main():
    """처음 관측을 보존하거나 보호 해시만 감사하며 제품/C++검사는 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text('utf-8'))
        for name,expected in record['sha256'].items():
            if digest(ROOT/name)!=expected:raise RuntimeError('보호 해시 불일치: '+name)
        print('Abort/제어 명령 최초 실제 PE·원문 해시 보존 확인');return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('최초 원문 덮어쓰기 금지')
    oracle=Oracle();lines=['# 실제 Abort 사건/두 큐 함수와 실제 제어 이름 조회·토큰·큐의 최초 관측이다.'];count=0
    # 확인0/비제로·상위 플래그·서로 다른 두 인자·현재 미션 부재·39칸 포화를 교차한다.
    for active,code,flags,x,y,filled,result in itertools.product((0,1),(0x200008c,0x2000abc),(0,0x80000000,0x38000000,0xfc000000),(0,71),(0,1,2,0xffffffff,0x80000000),(0,1,39),(0,1)):
        case=(active,code|flags,x,y,filled,result);lines.append('A\t'+','.join(map(str,case))+'\t'+oracle.abort(case));count+=1
    oracle.operation='S';scriptCount=0
    # 명령 코드는 합성하지 않고 실제004d52c0의 원본 이름 표에서 조회한다.
    for name,argument in itertools.product(('MissionRestart','mIsSiOnReStArT','MissionAbort','mIsSiOnAbOrT','LeaveBattle','lEaVeBaTtLe'),('0','1','2','4294967295','2147483648','-1','')):
        source=(name+','+argument).encode('ascii');row=oracle.run('Command',source)
        lines.append('S\t'+source.hex()+'\t'+row[6]+'\t'+row[7]+'\t'+row[8]);scriptCount+=1
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_script_command.py','tools/decomp_mission_abort.py','cpppj/tests/fixtures/mission-abort-x86.tsv')
    record={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-20','abortInputs':count,'scriptInputs':scriptCount,'normalReturns':oracle.abortReturns+scriptCount,'instructions':oracle.instructions,'actualQueueCalls':oracle.queueCalls,
        'scope':'Guide00463e40의 Abort/미처리 사건과 실제452fe0/452f90. 세 이름은 실제4c9090/4d52c0/토큰/큐/CRT다. Tell004cf960·하위 가상 입력·TLS/할당/로그는 명시 경계이며 실제 UI/월드 종료/재시작은 실행하지 않는다.',
        'gameExecuted':False,'osExecuted':False,'sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:record[key] for key in ('abortInputs','scriptInputs','normalReturns','instructions','actualQueueCalls')}))


if __name__=='__main__':
    main()
