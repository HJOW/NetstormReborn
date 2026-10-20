#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Guide 반복과 Tutorial의 얇은 단계·월드 진입 Pause를 하나의 호출 흐름으로 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from decomp_script_command import Oracle as InvokeOracle
from decomp_cursor_position import ranges
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

MISSION,GUIDE,TABLE=POOL,POOL+0x1000,POOL+0x2000  # 격리 미션/현재 포인터/가상 표다.
INIT,START,FRAME=STOP+0x100,STOP+0x120,STOP+0x140  # 큰 초기화/Start/Frame의 명시 경계다.
FIXTURE=ROOT/'cpppj/tests/fixtures/mission-lifecycle-x86.tsv'  # 제품 작성 전에 동결하는 새 전체 상태/효과다.
REPORT=ROOT/'cpppj/mission-lifecycle-native.json'  # 실제 얇은 몸체와 큰 외부 경계를 구별한다.


class Oracle(MemoryOracle):
    """Guide/First/단계2·5/빈6/Advance/Postpone/진입 Pause를 실제 PE로 실행한다."""
    def __init__(self):
        """현재 PC 전체 정밀 범위와 원본 가상 표의 실제 순서만 공급한다."""
        super().__init__();self.allowed=[];ranges(self,(0x463bb0,0x482c80,0x463a40,0x4c2b00,0x4c31a0,0x4c2a40,0x4437f0))
        self.allowed.extend(((0x4c2a70,0x4c2a81),(0x4c3170,0x4c3193)));self.calls={};self.returns=0  # 정밀 표에서 빠진 실제 First/Postpone 몸체다.
        # 단계1/3/4의 큰 효과는 명시 경계이며 그 외는 실제 몸체다.
        for index,entry in enumerate((0x4c2a70,INIT,0x4c2b00,START,FRAME,0x4c31a0,0x4437f0,0x4c3170)):
            self.put(TABLE+4+index*4,entry)
        self.put(TABLE+0x28,0x4c2a40);self.mu.hook_add(UC_HOOK_MEM_READ,self.read);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def read(self,mu,access,address,size,value,data):
        """실제 현재 미션 포인터 읽기 위치를 순서대로 기록한다."""
        if address==GUIDE+0x800:self.events.append('C,'+str(bool(self.words(address,1)[0])*1))

    def guard(self,mu,access,address,size,value,data):
        """원본 쓰기는 flags/순번/기한/자기 정지와 한정 스택 이외에 허용하지 않는다."""
        if STACK-0x2000<=address and address+size<=STACK+0x100:return
        if any(low<=address and address+size<=high for low,high in ((MISSION+0x54,MISSION+0x5c),(MISSION+0x78,MISSION+0x80),(MISSION+0xb8,MISSION+0xbc))):return
        raise RuntimeError(f'미션 수명 원문 쓰기 범위 밖 {address:08x}/{size}')

    def instruction(self,mu,address,size,data):
        """큰 경계의 명시 효과만 기록하고 원본 현재 재조회/단계/정지 계산은 유지한다."""
        if address in (0x463bb0,0x482c80,0x463a40,0x4c2a70,0x4c2b00,0x4c31a0,0x4c3170,0x4c2a40,0x4437f0):self.calls[f'{address:08x}']=self.calls.get(f'{address:08x}',0)+1
        if address==0x4b32e0:self.events.append('P');self.ret(self.case[4]);return
        if address==0x4c8e90:self.events.append('D');self.ret(self.case[5]);return
        if address==0x463ab0:self.events.append('L');self.put(GUIDE+0x800,0);self.ret();return
        if address==0x4e0620:self.events.append('A');self.ret();return
        if address==INIT:
            self.events.append('I');self.put(MISSION+0x54,self.words(MISSION+0x54,1)[0]|1)
            mu.reg_write(UC_X86_REG_ECX,GUIDE);mu.reg_write(UC_X86_REG_EIP,0x463a40);return
        if address==START:
            self.events.append('S');self.put(MISSION+0x88,1);mu.reg_write(UC_X86_REG_ECX,MISSION);mu.reg_write(UC_X86_REG_EIP,0x482c80);return
        if address==FRAME:
            self.events.append('F')
            if self.finish:self.put(MISSION+0x54,self.words(MISSION+0x54,1)[0]|1);self.finish=0
            self.ret();return
        if address in (0x4cf970,0x482de0,0x4c2a90):
            if address==0x4c2a90:self.events.append('Show');self.ret();return
            pointer=self.words(mu.reg_read(UC_X86_REG_ESP)+4,1)[0];self.events.append(('G,' if address==0x4cf970 else 'T,')+self.string(pointer).hex());self.ret(self.case[6],4 if address==0x482de0 else 0);return
        if address==0x466710:self.events.append('K');self.ret();return
        if address==0x460de0:self.events.append('Q');self.ret(int(self.depth!=0));return
        if address==0x460df0:self.events.append('H');self.depth+=1;self.ret();return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'미등록 수명 명령 {address:08x}')
        self.instructions+=1

    def sample(self,case):
        """한정된 입력을 실제 함수에 전달하고 전체208바이트와 정지/현재/순서를 읽는다."""
        self.case=case;operation,flags,sequence,current,pending,dialogs,tell,depth,owned,finish,runs,nextBits=case
        self.events=[];self.depth=depth;self.finish=finish;raw=bytearray((index*37+41)&255 for index in range(0xd0))
        for offset,value,width in ((0,TABLE,4),(0x54,flags,4),(0x58,sequence,4),(0x78,nextBits,8),(0x80,0x3f000000,4),(0x88,0,4),(0xb8,owned,4)):
            raw[offset:offset+width]=int(value).to_bytes(width,'little')
        self.mu.mem_write(MISSION,bytes(raw));self.put(GUIDE+0x800,MISSION if current else 0);self.mu.mem_write(0x55b4d0,struct.pack('<d',100.125))
        # 호출마다 정상 반환/보존 레지스터/x87 제어어를 독립 검사한다.
        for unused in range(runs):InvokeOracle.call(self,0x463a40 if operation else 0x463bb0,[],GUIDE);self.returns+=1
        return '|'.join((str(int(self.words(GUIDE+0x800,1)[0]!=0)),str(self.depth),bytes(self.mu.mem_read(MISSION,0xd0)).hex(),';'.join(self.events) or '-'))


def cases():
    """Guide 상태 전환/차단/반복과 직접 월드 진입 Pause의 현재/중첩 상태를 교차한다."""
    rows=[]
    # 시작 순번0에서 실제0→1→2→3→4와 종료5/6,차단/취소/진단을 구별한다.
    for flags,sequence,pending,dialogs,tell,finish,runs in itertools.product((0,1,4,5,8),(0,4,5,6,7),(0,1),(0,1),(0,1),(0,1),(1,2)):
        rows.append((0,flags,sequence,1,pending,dialogs,tell,0,0,finish,runs,0))
    # 빈 현재/기존 자기 정지/다른 소유자 정지와 이미 있는 기한의 재지연을 관측한다.
    for operation,current,depth,owned,nextBits in itertools.product((0,1),(0,1),(0,2),(0,1),(0,int.from_bytes(struct.pack('<d',3.25),'little'))):
        rows.append((operation,1,0,current,1,0,1,depth,owned,0,2,nextBits))
    return list(dict.fromkeys(rows))


def main():
    """새 원문만 최초 한 번 쓰거나 현재 보호 해시를 확인한다. 게임/C++검사는 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text('utf-8'))
        for name,expected in record['sha256'].items():
            if digest(ROOT/name)!=expected:raise RuntimeError('보호 해시 불일치: '+name)
        print('미션 수명 최초 Guide/단계/진입 Pause 원문 보존 확인');return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('최초 원문 덮어쓰기 금지')
    oracle=Oracle();rows=cases();lines=['# 실제 Guide/얇은 단계/진입 Pause이며 큰 Init/Start/Frame/외부 UI는 명시 경계다.']
    for row in rows:lines.append(','.join(map(str,row))+'\t'+oracle.sample(row))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_mission_lifecycle.py','cpppj/tests/fixtures/mission-lifecycle-x86.tsv')
    record={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-20','uniqueInputs':len(rows),'normalReturns':oracle.returns,'instructions':oracle.instructions,'calls':oracle.calls,'gameExecuted':False,'osExecuted':False,
        'scope':'실제 Guide/Advance/First/단계2·5/빈6/Postpone/463a40→Pause. 큰 Init/Start/Frame·Tell·UI/시계 효과/해제는 명시 경계이며 실제 월드/게임 실행 근거가 아니다.','sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:record[key] for key in ('uniqueInputs','normalReturns','instructions','calls')}))


if __name__=='__main__':
    main()
