#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""일반 Dude task setter의 두 실제 PE 본체와 Regular 생성/재예약을 제한 x86으로 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from decomp_owner_oracle import OwnerOracle,ROOT,TYPES,POOL,STACK,STOP,digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 실제 일반 task setter/조회/힙/부착/표면 통지와 현재 전역의 판본별 주소다.
SPEC={
 'originals':dict(entry=0x450670,masked=0x4afbe0,new=0x4e4391,attach=0x41bd20,surface=0x4214a0,now=0x55b4d0,bridge=0x5411a0,regular=0x541128),
 'originalCD':dict(entry=0x4e8ba0,masked=0x4ac9c0,new=0x4f1650,attach=0x48f590,surface=0x448c10,now=0x5484a8,bridge=0x51ca8c,regular=0x51ca18),
}
# 격리된 실제 Regular 객체와 입력/출력 파일이다.
REGULAR=0x11318000
KEYS=('sid','state','task','event','regular','type','delay','now')
FIXTURE=ROOT/'cpppj/tests/fixtures/dudetask-x86.tsv'
REPORT=ROOT/'cpppj/dudetask.json'


def bits(value):
    """지연 입력의 원본 float 저장 비트다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def wide(value):
    """게임 시각 입력의 원본 double 저장 비트다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def inputs(edition):
    """새/기존 Regular, 낙하 유지/교체, WORD 절삭, 다리, 시각 정밀도를 교차한다."""
    rows=[]
    # PATCH FALL 교체 입력은 별도 assert 관찰이며 CD는 정상 FALL 유지다.
    for task,event,regular,typeid,delay,now in itertools.product((0x5101,0x5105,0xcafe),(0x5103,0x5104,0x5105,0x12345103),(0,1),(70,82),(0,.1,-.125,333),(10,16777217.125)):
        rows.append(dict(zip(KEYS,(50,0,task,event,regular,typeid,bits(delay),wide(now)))))
    if edition=='originalCD':
        # CD의 빈 SID/free/dead 자연반환은 Regular/새 event와 무관한 원본 분기다.
        for sid,state,regular in itertools.product((0,50),(1,2,3),(0,1)):
            rows.append(dict(zip(KEYS,(sid,state,0x5105,0x5103,regular,82,bits(.1),wide(16777217.125)))))
    return rows


class DudeTaskOracle(OwnerOracle):
    """실제 task/Regular ctor/WORD 쓰기를 실행하고 현재 조회·힙·부착·표면만 관찰 경계로 둔다."""
    def __init__(self,edition):
        """현호스트의 전용 세 함수 범위와 실제 원본 PE 메모리를 연결한다."""
        super().__init__(edition);self.p=SPEC[edition];self.mu.mem_map(0,0x1000);self.boundaries=collections.Counter();self.returns=0;self.taskAssertions=0;self.events=[]
        paths=[ROOT/f'extracted/dude-task/{edition}/{name}' for name in ('creation.c','functions.tsv')];self.exports.extend(paths)
        with paths[1].open(encoding='utf-8') as fp:
            # Ghidra가 내보낸 각 실제 함수의 불연속 몸체만 실행한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def read(self,address,width=4):
        """원본의 비정렬 필드 저장 비트를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def on_instruction(self,mu,address,size,data):
        """현재 Regular 조회/힙/부착/표면 요청을 기록하고 실제 assert에서 별도 중단한다."""
        p=self.p;esp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(esp);this=mu.reg_read(UC_X86_REG_ECX);args=[self.read(esp+4+i*4) for i in range(4)];purge=0;value=0
        if address==self.spec['assert_report']:
            self.events.append('A:'+str(args[2]));self.taskAssertions+=1;mu.emu_stop();return
        if address==p['masked']:
            self.events.append('R:50:65520:20736');value=REGULAR if self.case['regular'] else 0;purge=8
        elif address==p['new']:
            if args[0]!=40:raise RuntimeError('Dude task 실제 new 크기 오류')
            self.events.append('N:40');value=REGULAR
        elif address==p['attach']:
            self.events.append('B:'+':'.join(map(str,args)));mu.mem_write(this+4,struct.pack('<II',83,50));raw=bytearray(self.stride);raw[10]=46;raw[11]=8;mu.mem_write(self.slot(83),bytes(raw));mu.mem_write(self.slot(50)+6,struct.pack('<H',83));purge=16
        elif address==p['surface']:
            self.events.append('S:'+str(args[0]))
        else:super().on_instruction(mu,address,size,data);return
        self.boundaries[f'{address:08x}']+=1;mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def prepare(self,case):
        """입력 raw 슬롯/기존 Regular만 공급하며 기대 event/time을 계산하지 않는다."""
        self.case=case;self.events=[];self.assertStart=self.taskAssertions;mu=self.mu;p=self.p;mu.mem_write(POOL,bytes(128*self.stride));mu.mem_write(REGULAR,bytes(80));mu.mem_write(p['now'],struct.pack('<Q',case['now']));mu.mem_write(p['bridge'],struct.pack('<I',82));mu.mem_write(p['regular'],struct.pack('<I',46));mu.mem_write(0,bytes(4))
        raw=bytearray(self.stride);raw[10]=case['type'];raw[11]=case['state'];struct.pack_into('<H',raw,12,case['task']);struct.pack_into('<ff',raw,14,20.75,21.9)
        if case['regular']:struct.pack_into('<H',raw,6,83)
        mu.mem_write(self.slot(50),bytes(raw));fields=bytearray(40);struct.pack_into('<II',fields,4,83,50);struct.pack_into('<dIII',fields,16,27,0x5104,bits(1),7);mu.mem_write(REGULAR,bytes(fields))
        self.write_ranges=[(0,4),(self.slot(50),self.slot(51)),(self.slot(83),self.slot(84)),(REGULAR,REGULAR+80)]

    def run(self,case):
        """독립 정상 RET/스택/x87 또는 정확 assert 시점과 현재 raw/Regular 결과를 기록한다."""
        self.prepare(case);mu=self.mu;p=self.p;patch=self.edition=='originals';args=[case['event'],case['delay']] if patch else [case['sid'],case['event'],case['delay']]
        # 원본의 유한 일반 시각 입력에는 53비트 x87 환경을 공급한다.
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(reg,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,0x027f);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_ECX,self.slot(case['sid']) if patch else 0);mu.reg_write(UC_X86_REG_ESP,STACK);mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.emu_start(p['entry'],STOP,count=30000)
        asserted=self.taskAssertions!=self.assertStart
        if asserted:
            if mu.reg_read(UC_X86_REG_EIP)!=self.spec['assert_report']:raise RuntimeError('Dude task assert 위치 오류')
        else:
            if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+(12 if patch else 4) or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('Dude task 정상 RET/스택/x87 오류')
            self.returns+=1
        present=case['regular'] or any(event.startswith('B:') for event in self.events)
        fields=','.join(map(str,(self.read(REGULAR+16,8),self.read(REGULAR+24),self.read(REGULAR+28),self.read(REGULAR+32)))) if present else '-'
        return ['assert' if asserted else 'return',';'.join(self.events),bytes(mu.mem_read(self.slot(50),self.stride)).hex(),fields]


def generate(smoke):
    """원본 출력만 별도 fixture에 저장하며 assert 관찰을 정상 완료 수와 분리한다."""
    rows=[];editions={};files={ROOT/'tools/decomp_dudetask_oracle.py',ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/ghidra/dude-task-functions.json'}
    for edition in SPEC:
        oracle=DudeTaskOracle(edition);cases=inputs(edition);cases=cases[:1] if smoke else cases
        # 서로 다른 원본 입력은 각각 한 번만 관찰한다.
        for case in cases:
            try:result=oracle.run(case)
            except Exception as error:raise RuntimeError(f'{edition} {case}: {error}') from error
            rows.append([edition,*[case[key] for key in KEYS],*result])
        editions[edition]=dict(returns=oracle.returns,assertions=oracle.taskAssertions,instructions=oracle.instructions,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.boundaries));files.update(oracle.exports);files.add(ROOT/oracle.spec['binary']);print(edition,editions[edition],flush=True)
    if smoke:return
    # 같은 Host/Regular의 전체 출처와 현재 API lineage를 함께 기록한다.
    for path in ('cpppj/src/o/RawDudeTask.h','cpppj/src/o/RawDudeTask.cpp','cpppj/tests/DudeTaskTests.cpp','cpppj/src/o/SquidProcess.h','cpppj/src/o/SquidProcess.cpp','cpppj/dude-regular-reset-lineage.json','cpppj/regular-initial-count-lineage.json'):files.add(ROOT/path)
    FIXTURE.write_text('# 일반 Dude task setter의 실제 PE 출력. 조회/힙/부착/표면은 명시 관찰 경계이고 낙하 교체 assert는 정상 반환과 별도다.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8');files.add(FIXTURE)
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',primary_target='10.78',cd_target='10.72',total=len(rows),normal=sum(item['returns'] for item in editions.values()),assertions=sum(item['assertions'] for item in editions.values()),editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},cpp_tests_completed=0,os_calls=0,new_dude_factories_completed=0,limitations=['실제 setter/Regular ctor/task WORD 쓰기와 현재 time/event/count/payload는 원본 몸체다.','현재 Regular 조회·힙·Form부착·다리 표면 요청은 명시 관찰 경계이며 C++는 같은 실제 Host 조회/생성을 소비한다.','패치 FALL 교체는 원본 assert에서 멈추며 정상 반환이나 전체 예외 처리 완료로 세지 않는다.','패치 SID0/free/예약 머리의 미정의 쓰기와 비유한 시각/힙 실패는 지원 입력에 포함하지 않는다.','CD SID0/free/dead 자연반환과 FALL 강제 유지, 양판본 DWORD event/하위 WORD 쓰기·분수float 지연+정밀double now를 포함한다.','실제 Dude 생성/Pop/Path24와 전체 드롭 소비자는 별도 후속이다.'],),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def verify():
    """현재 파일 SHA·고유 입력·정상/assert 분리와 실제 Dude 생성 미완료 경계를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    for path,expected in report['files'].items():
        if digest(ROOT/path)!=expected:raise RuntimeError('Dude task SHA 불일치 '+path)
    if len(rows)!=report['total'] or report['new_dude_factories_completed'] or report['os_calls']:raise RuntimeError('Dude task 사례/하위 완료 경계 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len({tuple(row[1:9]) for row in selected})!=len(selected) or sum(row[9]=='return' for row in selected)!=item['returns'] or sum(row[9]=='assert' for row in selected)!=item['assertions']:raise RuntimeError('Dude task 고유 입력/분류 오류')
    print(json.dumps(dict(verified=True,total=len(rows),normal=report['normal'],assertions=report['assertions'],dude_factory_completed=False),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
