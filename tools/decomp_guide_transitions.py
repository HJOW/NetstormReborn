#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Guide Begin/Fork/Restart/End의 실제 분배·교체와 원본 명령 이름 표를 독립 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_mission_abort import Oracle as BaseOracle, ROOT, POOL, TEXT, STACK, STOP, GUIDE, MISSION, TABLE, EVENT, digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

SECOND,NEW,TARGET=POOL+0x88000,POOL+0x8a000,TEXT+0x6000  # 재진입/공장 반환 객체와 해제 때 무효화할 설정 버퍼다.
FIXTURE=ROOT/'cpppj/tests/fixtures/guide-transitions-x86.tsv'  # 제품 전 보존하는 전체 효과/최종 현재 상태다.
REPORT=ROOT/'cpppj/guide-transitions-native.json'  # 실제 몸체와 명시 외부 효과를 구분한다.


class Oracle(BaseOracle):
    """현재 함수 표의 실제 Guide 다섯 몸체와 빈 종료·기존 실제 문자열 처리기를 실행한다."""
    def __init__(self):
        """기존 격리 PE/원본 C locale와 낮은/음수 진단 입력 주소를 준비한다."""
        super().__init__();ranges(self,(0x463b70,0x463cf0,0x463da0,0x4639d0,0x4437f0));self.operation='T';self.outerReturns=0;self.formatCalls=0
        self.mu.mem_map(0,0x1000);self.mu.mem_map(0x80000000,0x1000)
        self.put(TABLE+0x1c,0x4437f0)
        for pointer in (SECOND,NEW):self.put(pointer,TABLE)

    def identifier(self,pointer):
        """명시한 세 미션 객체의 원본 주소를 관측용 번호로만 바꾼다."""
        if pointer not in (0,MISSION,SECOND,NEW):raise RuntimeError('Guide 현재 포인터 범위 오류')
        return (0,MISSION,SECOND,NEW).index(pointer)

    def instruction(self,mu,address,size,data):
        """해제/공장/설정/서식/진단만 명시 공급하고 교체 분기·복사는 실제 PE에 맡긴다."""
        if self.operation=='T':
            sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,3)
            if address==0x4437f0:self.events.append('F,'+str(self.identifier(mu.reg_read(UC_X86_REG_ECX))))
            if address==0x463ab0:
                self.events.append('L,'+str(self.identifier(self.words(GUIDE+0x800,1)[0])));self.put(GUIDE+0x800,0);self.mu.mem_write(GUIDE,b'\0')
                self.mu.mem_write(TARGET,b'invalidated\0')
                if self.reentrant:self.put(GUIDE+0x800,SECOND);self.reentrant=0
                self.ret();return
            if address==0x482fb0:
                name=self.string(args[0]);self.events.append('C,'+name.hex());self.put(NEW+0x64,0);self.put(NEW+0x68,0);self.ret(NEW);return
            if address==0x482c90:self.events.append('G,'+self.string(args[0]).hex());self.ret(TARGET,4);return
            if address==0x4a89c0:
                if self.string(args[1])!=b'fork%d':raise RuntimeError('Fork 실제 서식 인자 오류')
                self.formatCalls+=1;number=struct.unpack('<i',struct.pack('<I',args[2]))[0];mu.mem_write(args[0],('fork'+str(number)).encode('ascii')+b'\0');self.ret(args[0]);return
            if address==0x4e0620:self.events.append('A');self.ret();return
        super().instruction(mu,address,size,data)

    def sample(self,case):
        """독립 이름/필드/진단 주소와 해제 재진입을 공급하고 정상 반환/최종 객체를 읽는다."""
        operation,active,mode,demo,reentrant,parameter,nameIndex,flags,x=case;self.operation='T';self.events=[];self.reentrant=reentrant
        name=(b'Tutorial2',b'MasterOfWhirligigs')[nameIndex];self.mu.mem_write(TARGET,name+b'\0');self.mu.mem_write(TEXT,name+b'\0')
        self.mu.mem_write(GUIDE,b'TheWarBegins\0');self.put(GUIDE+0x800,MISSION if active else 0)
        for pointer in (MISSION,SECOND,NEW):self.put(pointer,TABLE);self.put(pointer+0x64,mode);self.put(pointer+0x68,demo)
        if operation==0:result=self.call(0x463b70,[TEXT],GUIDE,4);kind='void'
        elif operation==1:result=self.call(0x463da0,[parameter],GUIDE,4);kind='void'
        elif operation==2:result=self.call(0x463cf0,[],GUIDE);kind='void'
        else:
            command=(0x200008b,0x200008e,0x200008d,0x200008f)[operation-3]
            if command==0x200008b:self.mu.mem_write(parameter,name+b'\0')
            self.mu.mem_write(EVENT,struct.pack('<3I',command|flags,x,parameter));result=self.call(0x463e40,[EVENT],GUIDE,4);kind=str(result)
        self.outerReturns+=1;current=self.words(GUIDE+0x800,1)[0]
        return '|'.join((kind,str(self.identifier(current)),self.string(GUIDE).hex(),','.join(map(str,self.words(NEW+0x64,2))),';'.join(self.events) or '-'))


def cases():
    """유효 현재·빈 Begin·해제 재진입·bool 정규화·signed Fork/진단과 modifier를 교차한다."""
    rows=[]
    for operation,active,reentrant,nameIndex in itertools.product((0,1,2),(0,1),(0,1),(0,1)):
        if operation and not active:continue
        for parameter in ((0,1,2,0x80000000,0xffffffff) if operation==1 else (0,)):
            for mode,demo in ((0,0),(7,3),(0xffffffff,0xffffffff)):rows.append((operation,active,mode,demo,reentrant,parameter,nameIndex,0,91))
    # Begin의 진단 뒤 계속은 명시 효과다. 낮은/음수 주소도 격리된 유효 문자열로 준비한다.
    for operation,reentrant,parameter,flags,x in itertools.product((3,4,5,6),(0,1),(0,1,100,101,0x80000100),(0,0xfc000000),(0,77)):
        rows.append((operation,1,7,3,reentrant,parameter,1,flags,x))
    return rows


def main():
    """새 원문을 처음 한 번 보존하거나 보호 해시만 확인하며 게임/OS를 시작하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text('utf-8'))
        for name,expected in record['sha256'].items():
            if digest(ROOT/name)!=expected:raise RuntimeError('보호 해시 불일치: '+name)
        print('Guide 교체 최초 실제 PE/명령 이름/원문 보존 확인');return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('최초 원문 덮어쓰기 금지')
    oracle=Oracle();rows=cases();lines=['# 실제 Guide 분배/교체/복사/빈 종료이며 해제/공장/설정/서식/진단은 명시 경계다.']
    for row in rows:lines.append('T\t'+','.join(map(str,row))+'\t'+oracle.sample(row))
    oracle.operation='S';scriptCount=0
    for name,argument in itertools.product(('MissionBegin','mIsSiOnBeGiN','MissionFork','mIsSiOnFoRk','MissionEnd','mIsSiOnEnD'),('Tutorial2','0','1','-1','')):
        raw=(name+','+argument).encode('ascii');result=oracle.run('Command',raw);lines.append('S\t'+raw.hex()+'\t'+result[6]+'\t'+result[7]);scriptCount+=1
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_guide_transitions.py','cpppj/tests/fixtures/guide-transitions-x86.tsv')
    record={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-20','transitionInputs':len(rows),'scriptInputs':scriptCount,'normalReturns':oracle.outerReturns+scriptCount,'instructions':oracle.instructions,'formatBoundaryCalls':oracle.formatCalls,
        'scope':'실제463e40/463b70/463cf0/463da0/4639d0와 빈4437f0·실제 이름/토큰/큐/CRT. 해제/공장/설정/서식/진단·TLS/할당은 명시 경계다. 낮은/음수 Begin 주소는 진단 뒤 계속 관측용 격리 문자열이며 실제 OS 주소가 아니다.',
        'gameExecuted':False,'osExecuted':False,'sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:record[key] for key in ('transitionInputs','scriptInputs','normalReturns','instructions','formatBoundaryCalls')}))


if __name__=='__main__':
    main()
