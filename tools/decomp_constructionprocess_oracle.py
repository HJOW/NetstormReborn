#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""건설 프로세스의 시작/생성자·진행·완료·form 삭제 통지를 실제 PE의 제한 x86으로 대조한다.

실제 명령: 시작(new 뒤 생성자 전체), 진행(RunFrame), 완료, form 삭제 통지, 부모 조회·HP 쓰기·genus/타입 조회·
abstract 지정·CRT 정수 변환, CD의 기본 건설 시간 helper.
명시 경계: operator_new, 건설 효과 생성, 프로세스 부착, 소리 조회/소리 프로세스 생성, 화면 갱신 요청, 최대 HP 조회,
가상 표시/Unpop/Repop, 프로세스 Kill, contained 삭제, 완료 소리, 유닛 밀어내기.
게임/OS/예외 처리기는 실행하지 않는다. --verify는 저장 관찰과 SHA/ABI 집계를 감사한다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,TYPES,POOL,STACK,STOP,digest
from decomp_returnpiece_oracle import ReturnOracle,VTABLE,ALLOCATION
from decomp_damageablepredestroy_oracle import CONTROLS,bits
from unicorn.x86_const import UC_X86_REG_ECX

# 합성 프로세스 객체/소리 프로세스 메모리와 가상 경계 주소, 부착된 form 번호다. 호스트 메모리와 무관하다.
OBJECT,SOUND_OBJECT,FORM=ALLOCATION,ALLOCATION+0x100,90
DISPLAY,UNPOP,REPOP=STOP+0x500,STOP+0x600,STOP+0x700
# 경계가 돌려주는 소리 항목과 경계 변동이 쓰는 고정 입력값이다.
SOUND,CHANGED_SOUND,CHANGED_READY=0x15000040,0x15000200,0x15000400
# 판본별 실제 진입점·경계 함수·전역 주소다.
SPEC={
 'originals':dict(start=0x443fe0,run=0x4428c0,complete=0x443e20,destroy=0x444030,serverFirst=15000,hp=4,
    allocate=0x4e4391,instant=0x423840,effect=0x423900,attach=0x41bd20,lookup=0x4a8ee0,soundProcess=0x4ab240,refresh=0x4da650,
    maxHp=0x4adcb0,kill=0x41bf10,remove=0x4b1a90,done=0x4a9d70,play=0x4a9550,push=0x443800,pushArgs=4,
    globals=dict(now=0x55b4d0,processType=0x5410e8,soundType=0x54112c,local=0x540c70,boss=0x540bc4,pushGenus=0x5325b4,
        dais=0x5412ec,finishAll=0x55a49c,finishGenus=0x55a498,debug=0x5e4794)),
 'originalCD':dict(start=0x4d06a0,run=0x4d0ba0,complete=0x4d09e0,destroy=0x4d08e0,serverFirst=6000,hp=2,
    allocate=0x4f1650,instant=0x4eab60,effect=0x4eac30,attach=0x48f590,lookup=0x437d20,soundProcess=0x453800,refresh=0x41eac0,
    maxHp=0x4aed30,kill=0x48f7b0,remove=0x4eb310,done=0x438c00,play=0x4383f0,push=0x4d2450,pushArgs=3,
    globals=dict(now=0x5484a8,processType=0x51c9d8,soundType=0x51ca1c,local=0x50f6c8,boss=0x540a2c,pushGenus=0x53fc28,
        dais=0x51cbd8,finishAll=0x537eb4,finishGenus=0x537eb0,debug=0x546778)),
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 고정 입력 열 순서다. double은 비트를 두 DWORD로 보존한다.
KEYS=('parent','duration','nowLow','nowHigh','allocation','mutate','type','state','extra','hp','owner','x','y','genus','ready','max',
    'startLow','startHigh','endLow','endHigh','local','boss','pushGenus','dais','finishAll','finishGenus','processType','soundType','flags')
FIXTURE=ROOT/'cpppj/tests/fixtures/constructionprocess-x86.tsv'
REPORT=ROOT/'cpppj/constructionprocess.json'


def double_words(value):
    """입력 double만 DWORD 둘로 분리한다. 기대 결과는 계산하지 않는다."""
    return struct.unpack('<2I',struct.pack('<d',value))


def case(now=2.0,start=0.0,end=8.0,**changes):
    """합성 전역/부모/프로세스 필드의 기본 입력을 만든다."""
    value=dict(parent=60,duration=bits(2.5),allocation=3,mutate=0,type=100,state=0,extra=1,hp=0x11223344,owner=1,
        x=bits(20.5),y=bits(21.25),genus=0,ready=0,max=100,local=1,boss=1,pushGenus=0x10444200,dais=165,
        finishAll=0,finishGenus=0,processType=30,soundType=47,flags=0)
    value['nowLow'],value['nowHigh']=double_words(now);value['startLow'],value['startHigh']=double_words(start)
    value['endLow'],value['endHigh']=double_words(end);value.update(changes);return {key:value[key]&0xffffffff for key in KEYS}


def inputs():
    """지속 시간 분기·할당 실패·진행 비율/한계·완료 조건·경계 뒤 현재 자료 재조회를 교차한다."""
    rows=[]
    # 지속 시간 0/-0, 1.0 경계, 음수, 무한대, 양/음 NaN과 현재 시각·두 할당 실패를 실제 생성자로 실행한다.
    for duration,now,allocation in itertools.product((0,0x80000000,bits(0.6),bits(1.0),0x3f800001,bits(2.5),bits(10.0),bits(-1.0),
            bits(-3.0),0x7f800000,0x7fc00000,0xffc00000),(0.0,2.5,1000.25),(0,1,3)):
        rows.append(('Start',case(now=now,duration=duration,allocation=allocation)))
    # 효과/부착/할당/소리 조회 경계가 현재 시각·타입·소리 필드를 바꾸는 입력이다.
    for mutate,duration,allocation in itertools.product((1,2,3,4,5),(0,bits(0.6),bits(2.5)),(1,3)):
        rows.append(('Start',case(mutate=mutate,duration=duration,allocation=allocation)))
    # 부모 번호와 현재 프로세스 타입 전역은 부착 인자로 그대로 전달된다.
    for parent,process in itertools.product((5,60,15000,20000),(30,31)):
        rows.append(('Start',case(parent=parent,processType=process)))
    times=((0.0,8.0,0.0),(0.0,8.0,2.0),(0.0,8.0,4.0),(0.0,8.0,8.0),(0.0,8.0,10.0),(0.0,8.0,-2.0),(4.0,4.0,4.0),(4.0,4.0,6.0),
        (8.0,0.0,4.0),(2.0,10.0,4.0))
    # 진행 비율의 절삭·0/최대 한계·0으로 나누기와 abstract/권한에 따른 Kill을 비교한다.
    for (start,end,now),maximum,extra,boss in itertools.product(times,(0,1,100,300,0xfffffffb,0x40000000),(0,1),(0,1)):
        rows.append(('Run',case(start=start,end=end,now=now,max=maximum,extra=extra,boss=boss)))
    # 즉시 완성 전역 둘과 genus 0x200 조건, abstract 바이트의 다른 비트 보존을 비교한다.
    for finish,vortex,genus,(start,end,now),extra in itertools.product((0,1),(0,1),(0,0x200,0x4000,0xffffffff),
            ((0.0,8.0,2.0),(0.0,8.0,8.0),(4.0,4.0,6.0)),(0,1,8,9)):
        rows.append(('Run',case(start=start,end=end,now=now,finishAll=finish,finishGenus=vortex,genus=genus,extra=extra)))
    # 최대 HP/표시/Kill 경계 뒤의 현재 시각·abstract·권한 재조회다.
    for mutate,(start,end,now),extra,boss in itertools.product((6,7,8),((0.0,8.0,2.0),(0.0,8.0,8.0)),(0,1),(0,1)):
        rows.append(('Run',case(mutate=mutate,start=start,end=end,now=now,extra=extra,boss=boss)))
    # 완료: 예측/서버 번호 경계·로컬 소유자 DWORD 비교·타입 준비 소리·권한을 교차한다.
    for parent,owner,local,ready,boss in itertools.product((5,60,5999,6000,14999,15000,20000),(1,2),(1,0x101),(0,0x15000300),(0,1)):
        rows.append(('Complete',case(parent=parent,owner=owner,local=local,ready=ready,boss=boss,hp=100)))
    # 현재 HP의 판본별 폭/부호와 최대 HP 일치, abstract 비트를 비교한다.
    for parent,hp,maximum,extra in itertools.product((60,15000,20000),(100,99,0x1ffff,0xffff),(100,0xffffffff,0x1ffff),(0,1,9)):
        rows.append(('Complete',case(parent=parent,hp=hp,max=maximum,extra=extra)))
    # 밀어내기 genus 마스크는 Repop 뒤의 현재 전역과 현재 타입 genus의 DWORD 논리곱이다.
    for parent,genus,push in itertools.product((15000,20000),(0,0x200,0x4000,0x10444200,0x80000000),(0x10444200,0,0xffffffff)):
        rows.append(('Complete',case(parent=parent,hp=100,genus=genus,pushGenus=push)))
    # 완료의 각 경계 뒤 HP/소유자/로컬 플레이어/권한/좌표/abstract/준비 소리 재조회다.
    for mutate,parent,owner in itertools.product((9,10,11,12,13,15,16),(60,20000),(1,2)):
        rows.append(('Complete',case(mutate=mutate,parent=parent,owner=owner,hp=99,genus=0x200,ready=0x15000300)))
    for sound in (47,48):rows.append(('Complete',case(parent=20000,hp=100,soundType=sound)))
    # form 삭제 통지: 완료 뒤 부모 상태 하위 두 비트와 dais 타입 제외, HP 쓰기 폭을 비교한다.
    for state,(kind,dais),parent,flags in itertools.product((0,1,2,4,8),((100,165),(100,100),(165,165)),(60,20000),(0,0x10)):
        rows.append(('Destroy',case(state=state,type=kind,dais=dais,parent=parent,flags=flags)))
    for mutate,parent in itertools.product((14,9,12),(60,20000)):
        rows.append(('Destroy',case(mutate=mutate,parent=parent,hp=99,genus=0x200)))
    for maximum,hp in itertools.product((0,100,0xffffffff,0x12345678),(100,0x11223344)):
        rows.append(('Destroy',case(max=maximum,hp=hp,parent=20000)))
    return rows


class ProcessOracle(ReturnOracle):
    """원본 건설 프로세스 몸체를 실행하고 효과 경계의 인자와 지정된 변동 입력만 공급한다."""
    def __init__(self,edition):
        """공통 로더 뒤 이번 내보내기 범위와 가상 표의 세 경계를 준비한다."""
        super().__init__(edition);self.r=SPEC[edition];self.patch=edition=='originals'
        self.returns=collections.Counter();self.stub_calls=collections.Counter()
        self.exports=[ROOT/f'extracted/constructionprocess/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 실제 함수별 불연속 범위 밖 실행은 거부한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 포함 끝 주소를 반열린 구간으로 바꾼다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        # 부모의 가상 표시(+0x88)·Unpop(+0x48)·Repop(+0x4c)은 합성 가상 표의 경계 주소로 받는다.
        for offset,address in ((0x88,DISPLAY),(0x48,UNPOP),(0x4c,REPOP)):self.mu.mem_write(VTABLE+offset,struct.pack('<I',address))
        self.stubs={self.r[key]:(letter,key) for key,letter in (('allocate','A'),('instant','I'),('effect','E'),('attach','J'),
            ('lookup','S'),('soundProcess','P'),('refresh','R'),('maxHp','H'),('kill','K'),('remove','X'),('done','W'),('play','G'),('push','B'))}
        self.stubs.update({DISPLAY:('D','display'),UNPOP:('U','unpop'),REPOP:('Q','repop')})

    def event(self,letter,*values):
        """모든 원본 인자를 비트 그대로 저장한다."""
        self.events.append(letter+(':'+':'.join(map(str,values)) if values else ''))

    def put(self,key,value):
        """입력/경계 변동으로 현재 전역 DWORD를 바꾼다."""
        self.mu.mem_write(self.r['globals'][key],struct.pack('<I',value&0xffffffff))

    def get(self,key):
        """현재 전역 DWORD를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(self.r['globals'][key],4))[0]

    def text(self,address):
        """경계가 받은 C 문자열 인자를 ASCII로 읽는다."""
        return bytes(self.mu.mem_read(address,32)).split(b'\0')[0].decode('ascii')

    def this_sid(self):
        """thiscall의 this가 부모 슬롯 시작인지 검사하고 번호를 얻는다."""
        address=self.mu.reg_read(UC_X86_REG_ECX)
        if address<POOL or (address-POOL)%self.stride:raise RuntimeError('건설 프로세스 부모 this 정렬 오류')
        return (address-POOL)//self.stride

    def this_process(self):
        """프로세스 가상 함수의 this가 합성 객체의 form 부분인지 검사한다."""
        if self.mu.reg_read(UC_X86_REG_ECX)!=OBJECT+0x28:raise RuntimeError('건설 프로세스 this 오류')

    def stub(self,letter,key):
        """몸체 밖의 효과만 응답한다. 시각/비율/한계/조건 계산은 대체하지 않는다."""
        mu=self.mu;c=self.case;parent=self.slot(c['parent']);o=self.o;result=0;purge=0
        if key=='allocate':
            size,=self.args(1);self.event(letter,size)
            if size==0x38:result=OBJECT if c['allocation']&1 else 0
            else:
                result=SOUND_OBJECT if c['allocation']&2 else 0
                if c['mutate']==3:mu.mem_write(OBJECT+0x34,struct.pack('<I',CHANGED_SOUND))
        elif key in ('instant','effect'):
            self.event(letter,*self.args(1))
            if c['mutate']==1:mu.mem_write(self.r['globals']['now'],struct.pack('<d',7.75))
            if c['mutate']==2:self.put('processType',44)
        elif key=='attach':
            self.this_process();self.event(letter,*self.args(4));purge=16
            mu.mem_write(OBJECT+0x2c,struct.pack('<2I',FORM,c['parent']))
            if c['mutate']==5:mu.mem_write(self.r['globals']['now'],struct.pack('<d',99.5))
        elif key=='lookup':
            self.event(letter,self.text(self.args(1)[0]));result=0 if c['mutate']==4 else SOUND
        elif key=='soundProcess':
            if mu.reg_read(UC_X86_REG_ECX)!=SOUND_OBJECT:raise RuntimeError('소리 프로세스 생성 this 오류')
            self.event(letter,*self.args(4));purge=16;result=SOUND_OBJECT
        elif key=='refresh':
            self.event(letter)
            if c['mutate']==10:self.put('local',2)
        elif key=='maxHp':
            self.event(letter,self.this_sid());self.hp_calls+=1;result=c['max']
            if c['mutate']==6:mu.mem_write(self.r['globals']['now'],struct.pack('<d',6.0))
            if c['mutate']==9:mu.mem_write(parent+26,struct.pack('<I',c['max'])[:self.r['hp']])
            if c['mutate']==14 and self.hp_calls>1:result=(c['max']+7)&0xffffffff
        elif key=='display':
            self.event(letter,self.this_sid())
            if c['mutate']==7:mu.mem_write(parent+o['extra'],bytes([mu.mem_read(parent+o['extra'],1)[0]^1]))
        elif key=='kill':
            self.this_process();self.event(letter,*self.args(1));purge=4
            if c['mutate']==8:self.put('boss',self.get('boss')^1)
        elif key=='remove':
            self.event(letter,*self.args(4))
            if c['mutate']==15:mu.mem_write(parent+o['owner'],bytes([2]))
        elif key=='done':
            x,y,name,loop=self.args(4);self.event(letter,x,y,self.text(name),loop)
            if c['mutate']==11:self.put('boss',self.get('boss')^1)
            if c['mutate']==16:mu.mem_write(TYPES+c['type']*self.type_stride+0x90,struct.pack('<I',CHANGED_READY))
        elif key=='play':self.event(letter,*self.args(6))
        elif key=='unpop':
            self.event(letter,self.this_sid(),*self.args(1));purge=4
            # 실제 Unpop처럼 void를 켠다. 원본의 abstract 지정은 void 상태를 요구한다.
            mu.mem_write(parent+11,bytes([mu.mem_read(parent+11,1)[0]|4]))
            if c['mutate']==13:mu.mem_write(parent+o['extra'],bytes([mu.mem_read(parent+o['extra'],1)[0]|0x80]))
        elif key=='repop':
            self.event(letter,self.this_sid(),*self.args(1));purge=4
            mu.mem_write(parent+11,bytes([mu.mem_read(parent+11,1)[0]&0xfb]))
            if c['mutate']==12:
                mu.mem_write(parent+14,struct.pack('<2f',40.5,41.25));self.put('pushGenus',0x200)
        elif key=='push':self.event(letter,*self.args(self.r['pushArgs']))
        else:raise RuntimeError('알 수 없는 건설 프로세스 경계')
        self.ret(result,purge)

    def on_instruction(self,mu,address,size,data):
        """경계의 실제 몸체는 실행하지 않고 나머지는 허용 명령만 실행한다."""
        found=self.stubs.get(address)
        if found is None:OwnerOracle.on_instruction(self,mu,address,size,data);return
        letter,key=found;self.stub(letter,key);self.stub_calls[letter]+=1

    def run_process(self,operation,c,control):
        """입력 전역/부모/프로세스 필드를 만들고 SEH/ABI/x87 정상 반환과 필드·부모 raw를 기록한다."""
        self.case=c;self.events=[];self.hp_calls=0;mu=self.mu;r=self.r;o=self.o;parent=self.slot(c['parent'])
        # 부모의 현재 타입 genus와 준비 소리 항목만 실제 타입 조회가 읽는다.
        record=TYPES+c['type']*self.type_stride;mu.mem_write(record,bytes(self.type_stride))
        mu.mem_write(record+0xec,struct.pack('<I',c['genus']));mu.mem_write(record+0x90,struct.pack('<I',c['ready']))
        raw=bytearray((i*29+7)&255 for i in range(self.stride));struct.pack_into('<I',raw,0,VTABLE);struct.pack_into('<H',raw,6,FORM)
        raw[10]=c['type'];raw[11]=c['state'];struct.pack_into('<2I',raw,14,c['x'],c['y']);raw[26:26+r['hp']]=struct.pack('<I',c['hp'])[:r['hp']]
        raw[o['owner']]=c['owner']&255;raw[o['extra']]=c['extra']&255;mu.mem_write(parent,bytes(raw))
        # 모든 단계가 읽는 현재 전역이다. 패치 debug와 CD Pentium FDIV 우회 전역은 0이다.
        for key in ('processType','soundType','local','boss','pushGenus','dais','finishAll','finishGenus'):self.put(key,c[key])
        self.put('debug',0);mu.mem_write(r['globals']['now'],struct.pack('<2I',c['nowLow'],c['nowHigh']))
        # 시작은 미초기화 메모리 표식에서 출발하고 나머지는 이미 부착된 프로세스 필드를 공급한다.
        image=bytearray([0xa5])*0x38
        if operation!='Start':
            image[8]=0;struct.pack_into('<4I',image,0x10,c['startLow'],c['startHigh'],c['endLow'],c['endHigh'])
            struct.pack_into('<3I',image,0x2c,FORM,c['parent'],SOUND)
        mu.mem_write(OBJECT,bytes(image));mu.mem_write(0,struct.pack('<I',0x11223344))
        self.write_ranges=[(0,4)]
        if operation=='Start':self.write_ranges+=[(OBJECT,OBJECT+4),(OBJECT+8,OBJECT+9),(OBJECT+0x10,OBJECT+0x20),(OBJECT+0x28,OBJECT+0x38)]
        if operation in ('Run','Destroy'):self.write_ranges.append((parent+26,parent+26+r['hp']))
        if operation in ('Complete','Destroy'):self.write_ranges.append((parent+o['extra'],parent+o['extra']+1))
        self.prepare_registers(control)
        if operation=='Start':entry=r['start'];args=[c['parent'],c['duration']];purge=0
        elif operation=='Run':entry=r['run'];args=[];purge=0;mu.reg_write(UC_X86_REG_ECX,OBJECT+0x28)
        elif operation=='Complete':entry=r['complete'];args=[];purge=0;mu.reg_write(UC_X86_REG_ECX,OBJECT)
        else:entry=r['destroy'];args=[c['flags']];purge=4;mu.reg_write(UC_X86_REG_ECX,OBJECT+0x28)
        mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.emu_start(entry,STOP,count=100000)
        self.check(control,STACK+4+purge);self.returns[f'{entry:08x}']+=1
        if bytes(mu.mem_read(0,4))!=struct.pack('<I',0x11223344):raise RuntimeError('건설 프로세스 SEH 머리 복구 오류')
        dirty=mu.mem_read(OBJECT+8,1)[0];fields='-'
        if operation!='Start' or c['allocation']&1:
            fields=':'.join(map(str,(dirty,*struct.unpack('<4I',mu.mem_read(OBJECT+0x10,16)),struct.unpack('<I',mu.mem_read(OBJECT+0x34,4))[0])))
        return [';'.join(self.events) or '-',fields,bytes(mu.mem_read(parent,self.stride)).hex()]


def generate(smoke=False):
    """세 PE를 각각 두 정밀도로 실행해 출력이 같을 때만 독립 fixture/근거를 저장한다."""
    rows=[];editions={};paths={Path(__file__),FIXTURE,ROOT/'tools/ghidra/constructionprocess-functions.json',
        *[ROOT/f'tools/decomp_{name}_oracle.py' for name in ('owner','priestgeometry','returnpiece','damageablepredestroy')]}
    # 각 실제 PE를 별도 가상 메모리로 실행한다.
    for edition in SPECS:
        oracle=ProcessOracle(edition);plan=inputs()[::23] if smoke else inputs();counts=collections.Counter()
        # 함수 진입/정상 반환과 명시 경계 인자를 두 실행에서 함께 검사한다.
        for operation,c in plan:
            observed=[oracle.run_process(operation,c,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError(f'건설 프로세스 x87 관찰 차이 {edition} {operation} {c}')
            rows.append([edition,operation,','.join(str(c[key]) for key in KEYS),*observed[0]]);counts[operation]+=1
        editions[edition]=dict(cases=dict(counts),returns=dict(oracle.returns),native_calls=dict(oracle.native_calls),
            substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary'])
        # 공통 로더가 실제로 읽는 이전 내보내기도 SHA 근거에 포함한다.
        for group in ('owner','priestgeometry','returnpiece'):
            paths.update(ROOT/f'extracted/{group}/{edition}/{name}' for name in ('creation.c','functions.tsv'))
        print(f'{edition}: constructionprocess {len(plan)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 건설 프로세스 시작/생성자·진행·완료·form 삭제 통지. 효과/부착/소리/최대 HP/가상 공간·표시/Kill은 명시 경계.\n'
        '# 판본 연산 입력('+' '.join(KEYS)+') 사건 필드(dirty:start:end:sound) 부모raw\n'+
        '\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-11',controls=list(CONTROLS),os_calls=0,editions=editions,
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},limits=[
            '실제 시작/생성자·진행·완료·form 삭제 통지 전체와 부모 조회·HP 쓰기·genus/타입 조회·abstract 지정·CRT 정수 변환·CD 기본 시간 helper',
            'operator_new·건설 효과·프로세스 부착·소리 조회/소리 프로세스·갱신·최대 HP·가상 표시/Unpop/Repop·Kill·contained 삭제·완료 소리·밀어내기는 명시 경계',
            '2의 거듭제곱 분모의 유한 합성 시각·부모 5..20000·타입 100/165; 패치 debug와 CD Pentium FDIV 우회 전역 0',
            '원본 assert/게임/OS/예외 처리기 미실행, 정상 SEH/스택/보존 레지스터/x87·허용 실행/쓰기·SHA 감사']),
        ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'total: {len(rows)}',flush=True)


def verify():
    """기대 동작을 재계산하지 않고 SHA·입력·행 폭·진입/반환/경계 집계를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 모든 실행/입력 근거 파일이 생성 시점 그대로인지 검사한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError('SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('건설 프로세스 메타 합계 오류')
    # 원본 출력의 결과 의미를 재생성하지 않고 계획 입력/행 폭/효과 수만 대조한다.
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition];plan=inputs();counts=collections.Counter();events=[]
        if len(chosen)!=len(plan) or item['assertions']:raise RuntimeError('건설 프로세스 행/assert 오류')
        # 필드 열의 항목 수와 부모 raw의 길이는 판본별로 검사한다.
        for row,(operation,c) in zip(chosen,plan,strict=True):
            if len(row)!=6 or row[1]!=operation or row[2]!=','.join(str(c[key]) for key in KEYS):raise RuntimeError('건설 프로세스 입력 순서 오류')
            if row[4]!='-' and len(row[4].split(':'))!=6:raise RuntimeError('건설 프로세스 필드 폭 오류')
            if (row[4]=='-')!=(operation=='Start' and not c['allocation']&1):raise RuntimeError('건설 프로세스 할당 실패 표시 오류')
            if len(bytes.fromhex(row[5]))!=SPECS[edition]['stride']:raise RuntimeError('건설 프로세스 부모 raw 폭 오류')
            counts[operation]+=1;events.extend(event for event in row[3].split(';') if event!='-')
        if dict(counts)!=item['cases']:raise RuntimeError('건설 프로세스 연산 합계 오류')
        # 두 정밀도의 루트 진입/정상 반환을 각각 대조한다. 완료는 form 삭제 통지 안에서도 진입한다.
        for operation,key in (('Start','start'),('Run','run'),('Complete','complete'),('Destroy','destroy')):
            address=f'{SPEC[edition][key]:08x}';expected=2*counts[operation]
            if item['returns'].get(address,0)!=expected:raise RuntimeError('건설 프로세스 정상 반환 오류')
            entered=expected+(2*counts['Destroy'] if key=='complete' else 0)
            if item['native_calls'].get(address,0)!=entered:raise RuntimeError('건설 프로세스 실제 진입 오류')
        for letter in 'AIEJSPRHKXWGDUQB':
            if item['substitutions'].get(letter,0)!=2*sum(event==letter or event.startswith(letter+':') for event in events):raise RuntimeError('건설 프로세스 경계 합계 오류: '+letter)
        if any(item['native_calls'].get(f'{SPEC[edition][key]:08x}',0) for key in ('allocate','instant','effect','attach','lookup',
                'soundProcess','refresh','maxHp','kill','remove','done','play','push')):raise RuntimeError('건설 프로세스 경계 몸체 실행')
    print(f'constructionprocess 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 근거 감사 중 선택한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)


if __name__=='__main__':main()
