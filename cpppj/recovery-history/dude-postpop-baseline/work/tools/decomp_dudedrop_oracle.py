#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Dude 드롭 부모→난수/진입점→factory/Pop→일반task 사슬을 두 PE로 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from decomp_dudespawn_oracle import DudeSpawnOracle
from decomp_creation_oracle import TYPES
from decomp_sid_oracle import POOL
from decomp_postpop_oracle import ARENA
from decomp_oracle import ROOT,STACK,STOP
from decomp_owner_oracle import digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 실제 두드롭/task와 현재게임시각/난수·힙/프로세스부착경계다.
SPEC={
 'originals':dict(drop=0x450b30,pointDrop=0x450c20,task=0x450670,masked=0x4afbe0,new=0x4e4391,attach=0x41bd20,now=0x55b4d0,random=0x532710),
 'originalCD':dict(drop=0x4e8d20,pointDrop=0x4e8e00,task=0x4e8ba0,masked=0x4ac9c0,new=0x4f1650,attach=0x48f590,now=0x5484a8,random=0x5308ec),
}
# 기존격리 Player/목록뒤의 실제Regular 객체다.
REGULAR=ARENA+0x9000
KEYS=('random','index','width','height','genus','owner','state','sid','seed','capacity','count','blocked','now')
FIXTURE=ROOT/'cpppj/tests/fixtures/dudedrop-x86.tsv'
REPORT=ROOT/'cpppj/recovery-dudedrop-evidence.json'


def bits(value):
    """현재좌표의 원본float 저장비트다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def wide(value):
    """현재시각의 원본double 저장비트다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def sample(**kwargs):
    """source/세계/난수 입력이며 새좌표나지연 기대값은 계산하지 않는다."""
    case=dict(zip(KEYS,(1,1,3,3,0x40000000,1,2,5,0,3,0,0,wide(16777217.125))));case.update(kwargs);return case


def inputs():
    """두부모/발자국0/한도/차단/free/dead/SID0와두float지연을 교차한다."""
    rows=[]
    # 정상PostDestroy의dead source도free가아니면 실제원본이처리한다.
    for random,index,foot,owner,state,seed in itertools.product((0,1),(0,1,7),((0,0),(3,3)),(0,1,8),(0,2),(0,0x12345678)):
        rows.append(sample(random=random,index=index,width=foot[0],height=foot[1],owner=owner,state=state,seed=seed))
    # 실패생성/CDtaskSID0 자연반환과source자연반환/패치진단은별도입력이다.
    for random,mode in itertools.product((0,1),range(5)):
        rows.append(sample(random=random,capacity=0 if mode==0 else 3,count=3 if mode==1 else 0,blocked=int(mode==2),state=1 if mode==3 else 2,sid=0 if mode==4 else 5))
    return list({tuple(case[key] for key in KEYS):case for case in rows}.values())


class DudeDropOracle(DudeSpawnOracle):
    """실제드롭/생성/공간/프레임/taskbody와명시Regular조회·힙·부착만 관찰한다."""
    def __init__(self,edition):
        """같은현호스트 Task3/3과진입점ftol/소유자/타입보조의 실제몸체를 연결한다."""
        super().__init__(edition);self.d=SPEC[edition];self.dropReturns=0;self.dropAssertions=0;self.boundaries=collections.Counter();self.events=[]
        for name in ('dude-task','gunimpact-dude-helper','burning-sources-helper'):
            paths=[ROOT/f'extracted/{name}/{edition}/{part}' for part in ('creation.c','functions.tsv')];self.exports.extend(paths)
            with paths[1].open(encoding='utf-8') as fp:
                # 실제내보낸함수의 불연속범위만 허용한다.
                for row in csv.DictReader(fp,delimiter='\t'):
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def on_instruction(self,mu,address,size,data):
        """실제task의현재조회/힙/부착은순서/인자를기록하며새Dude생성/Pop은직접실행한다."""
        if not hasattr(self,'d') or address not in (self.spec['assert'],self.d['masked'],self.d['new'],self.d['attach']):super().on_instruction(mu,address,size,data);return
        esp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(esp);this=mu.reg_read(UC_X86_REG_ECX);args=[self.read(esp+4+index*4) for index in range(4)];purge=0;value=0
        if address==self.spec['assert']:
            self.events.append('A:'+str(args[2]));self.dropAssertions+=1;mu.emu_stop();return
        if address==self.d['masked']:
            self.events.append('R:6:'+str(args[0])+':'+str(args[1]));purge=8
        elif address==self.d['new']:
            if args[0]!=40:raise RuntimeError('Dude drop 미허용heap크기')
            self.events.append('N:40');value=REGULAR
        elif address==self.d['attach']:
            self.events.append('B:'+':'.join(map(str,args)));mu.mem_write(this+4,struct.pack('<II',7,args[1]));mu.mem_write(POOL+args[1]*self.stride+6,struct.pack('<H',7));purge=16
        else:super().on_instruction(mu,address,size,data);return
        self.boundaries[f'{address:08x}']+=1;mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def prepare_drop(self,case):
        """현재source의raw/발자국/genus/owner와같은생성상태·double시각/난수를 공급한다."""
        super().prepare(dict(capacity=case['capacity'],count=case['count'],blocked=case['blocked'],x=bits(20.75),y=bits(21.9),owner=case['owner'],repeat=0));mu=self.mu;patch=self.edition=='originals';typ=TYPES+89*self.creation['type_stride'];mu.mem_write(typ+0xec,struct.pack('<I',case['genus']));mu.mem_write(typ+self.space['foot'],struct.pack('<ii',case['width'],case['height']));raw=bytearray(self.stride);raw[10]=89;raw[11]=case['state'];raw[34 if patch else 32]=case['owner'];struct.pack_into('<ff',raw,14,20.75,21.9);mu.mem_write(POOL+5*self.stride,bytes(raw));mu.mem_write(REGULAR,bytes(40));mu.mem_write(self.d['now'],struct.pack('<Q',case['now']));mu.mem_write(self.d['random'],struct.pack('<I',case['seed']));mu.mem_write(0x541128 if patch else 0x51ca18,struct.pack('<I',46));mu.mem_write(0,bytes(4));self.events=[];self.assertStart=self.dropAssertions;self.write_ranges.extend([(REGULAR,REGULAR+40),(self.d['random'],self.d['random']+4)])

    def run_drop(self,case):
        """드롭상위의정상RET/진단과새Dude/Regular·난수/공간/장부를독립기록한다."""
        self.prepare_drop(case);self.registers();self.mu.mem_write(STACK,struct.pack('<III',STOP,case['sid'],case['index']));self.mu.emu_start(self.d['drop'] if case['random'] else self.d['pointDrop'],STOP,count=300000);asserted=self.dropAssertions!=self.assertStart
        if not asserted:
            if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Dude drop 정상RET/스택 오류')
            self.dropReturns+=1
        born=6 if self.read(POOL+6*self.stride+10,1)==70 else 0;self.sid=born or 6;population=','.join(map(str,struct.unpack('<'+'I'*case['capacity'],self.mu.mem_read(self.read(self.p['population']),case['capacity']*4)))) if case['capacity'] else '-';regular=','.join(map(str,(self.read(REGULAR+16,8),self.read(REGULAR+24),self.read(REGULAR+28),self.read(REGULAR+32)))) if any(event.startswith('B:') for event in self.events) else '-'
        return ['assert' if asserted else 'return',str(born),';'.join(self.events),regular,str(self.read(self.d['random'])),str(self.read(self.p['count'])),population,self.raw(),*map(str,self.output())]


def generate(smoke):
    """새부모입력은이전Task780/Spawn288 재방문과구분해기록한다."""
    rows=[];editions={};files={ROOT/'tools/decomp_dudedrop_oracle.py',ROOT/'tools/decomp_dudespawn_oracle.py',ROOT/'tools/ghidra/dude-spawn-functions.json',ROOT/'tools/ghidra/dude-task-functions.json'}
    for edition in SPEC:
        oracle=DudeDropOracle(edition);cases=[sample(),sample(random=0)] if smoke else inputs()
        # 실제각원본에같은현재source/세계입력을각각한번공급한다.
        for case in cases:
            try:result=oracle.run_drop(case)
            except Exception as error:raise RuntimeError(f'{edition} {case}: {error}') from error
            rows.append([edition,*[case[key] for key in KEYS],*result])
        editions[edition]=dict(returns=oracle.dropReturns,assertions=oracle.dropAssertions,actual_calls=dict(oracle.calls),boundaries=dict(oracle.boundaries));files.update(oracle.exports);files.add(ROOT/edition/('Netstorm.exe' if edition=='originals' else 'NETSTORM.EXE'));files.add(oracle.shapeFile);files.add(ROOT/edition/'netstorm.tarc');print(edition,editions[edition],flush=True)
    if smoke:return
    for path in ('cpppj/src/o/RawDudeDrop.h','cpppj/src/o/RawDudeDrop.cpp','cpppj/tests/DudeDropTests.cpp','cpppj/src/o/RawDudeSpawn.cpp','cpppj/src/o/RawDudeTask.cpp','cpppj/src/o/RawDudeNotice.cpp','cpppj/recovery-dudespawn-evidence.json','cpppj/recovery-dudetask-evidence.json','tools/decomp_owner_oracle.py','tools/decomp_postpop_oracle.py','tools/decomp_display_oracle.py','tools/decomp_creation_oracle.py','tools/decomp_sid_oracle.py','tools/decomp_oracle.py'):files.add(ROOT/path)
    FIXTURE.write_text('# 실제Dude드롭부모/난수/진입점/factory/Pop/task. Regular조회/힙/부착만명시경계다.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8');files.add(FIXTURE)
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',primary_target='10.78',cd_target='10.72',total=len(rows),editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},cpp_tests_completed=0,os_calls=0,limitations=['드롭부모/게임float난수/진입점·Dudefactory/ctor/owner/Pop/Frame/초기장부와task/Regularctor/time은원본몸체다.','현재Regular조회/40바이트힙/ProcessForm부착은명시관찰경계다. C++동일Host등록은후속이다.','source의현재raw/foot/genus/owner는명시입력이고전체원본source파괴를이번몸체로완료했다고세지않는다.','DudePath24/실제task이후이동/부모Damageable PostDestroy의전체실제삭제와캠페인월드는후속이다.'],),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def verify():
    """현재SHA와고유부모입력의정상/진단분류를감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    for path,expected in report['files'].items():
        if digest(ROOT/path)!=expected:raise RuntimeError('Dude drop SHA 불일치 '+path)
    if len(rows)!=report['total'] or report['os_calls']:raise RuntimeError('Dude drop 입력/OS 경계 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len({tuple(row[1:14]) for row in selected})!=len(selected) or sum(row[14]=='return' for row in selected)!=item['returns'] or sum(row[14]=='assert' for row in selected)!=item['assertions']:raise RuntimeError('Dude drop 고유/반환분류 오류')
    print(json.dumps(dict(verified=True,total=len(rows),cpp_completed=False),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
