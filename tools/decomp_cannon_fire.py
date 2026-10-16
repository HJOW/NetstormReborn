#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""초기화 이후 썬/썬더 캐논의 실제 공통 Fire·가상 단계·ctor/launch를 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_crossbow_animation import Oracle as BaseOracle,ROOT,OBJECT,ALLOC,VTABLE,PARENT_QUERY,OUTPUT,TYPES,STACK,STOP,POOL,bits
from decomp_owner_oracle import digest
from decomp_gunanimation_oracle import UNPOP,REPOP
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG)

# 파생 종류·몸체·명시 raw 입력과 원문 출력 자료다.
KEYS=('profile','kind','frame','due','targetstate','hp','mode','argument','growl','roar','mutation')
FIXTURE=ROOT/'cpppj/tests/fixtures/cannon-fire-x86.tsv'
REPORT=ROOT/'cpppj/cannon-fire.json'


def sample(**values):
    """원문 결과는 계산하지 않고 기본 독립 입력만 공급한다."""
    row=dict(zip(KEYS,(1,0,0,1,0,1,0,0,5,1,0)));row.update(values);return row


def inputs():
    """모든 정상 프레임·due·생존·HP·문자 구간과 가상 단계 인수/소리 중첩을 방문한다."""
    rows=[]
    for profile,count,modes in ((0,48,(0,)),(1,32,range(4))): # 실제 두 파생의 전체 정상 물리 프레임을 방문한다.
        for frame,due,dead,hp,mode in itertools.product(range(count),(0,1),range(4),(0,1),modes):
            rows.append(sample(profile=profile,frame=frame,due=due,targetstate=dead,hp=hp,mode=mode))
    for mode,frame,argument,offsets in itertools.product(range(4),range(32),(-1,0,7,15,31),((5,1),(0,0),(1,1),(-1,0))): # 부모 프레임과 별도 인수를 교차한다.
        rows.append(sample(kind=2,frame=frame,argument=argument,mode=mode,growl=offsets[0],roar=offsets[1]))
    for profile in (0,1): # 실제 두 생성자와 공통 launch의 HP/죽은 표적을 대조한다.
        rows.append(sample(profile=profile,kind=1))
        for dead,hp in itertools.product(range(4),(0,1)):rows.append(sample(profile=profile,kind=3,targetstate=dead,hp=hp))
    for profile,frame,mutation in itertools.product((0,1),(0,1,2,5,6,7,15,23,24,31),range(1,8)): # 외부 효과 뒤의 현재 필드/전역 재조회를 방문한다.
        rows.append(sample(profile=profile,frame=frame,mutation=mutation))
    return rows


def letters(mode):
    """실제 코드/구간 후처리에 공급할 명시 프레임 문자 입력이다."""
    if mode==0:return [65+i//8 for i in range(32)]
    if mode==1:return [65]*32
    if mode==2:return [65+i%2 for i in range(32)]
    return [65]+[66]*5+[67]*13+[68]*13


class Oracle(BaseOracle):
    """실제 공통 Fire와 파생 가상 getter를 실행하고 월드 효과만 명시 입력으로 준다."""
    def __init__(self,control):
        """현재 전체 정련 범위와 직접 검토한 짧은 파생 단계를 추가한다."""
        super().__init__(control)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for entry in (0x467330,0x467fb0,0x468460,0x4684d0,0x4ad710,0x468510,0x4684b0,0x466a40): # 실제 원문 함수의 불연속 구간을 그대로 쓴다.
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.allowed.extend(((0x468500,0x468507),(0x468640,0x468647),(0x468520,0x4685d4),(0x4684a0,0x4684a6),(0x468490,0x468497)))
        assert self.read(0x50b370+0x18)==0x467330 and self.read(0x50b370+0x48)==0x468520

    def mutate(self,where):
        """호출 경계에서만 현재 원문 입력을 바꾸며 제어 결과는 대신 계산하지 않는다."""
        mut=self.case['mutation'];mu=self.mu
        if where=='Center' and mut==1:mu.mem_write(self.slot(50)+36,struct.pack('<I',2))
        if where=='Center' and mut==2:mu.mem_write(OBJECT+44,struct.pack('<H',61))
        if where=='Unpop' and mut==3:mu.mem_write(self.p['now'],struct.pack('<d',100));mu.mem_write(OBJECT+48,struct.pack('<d',777))
        if where=='Repop' and mut==4:mu.mem_write(self.slot(60)+11,bytes([2]))
        if where=='Point' and mut==5:
            mu.mem_write(OBJECT+8,struct.pack('<I',51));mu.mem_write(OBJECT+44,struct.pack('<H',61));mu.mem_write(OBJECT+12,struct.pack('<ff',77.25,78.5));mu.mem_write(0x501a54,struct.pack('<f',37));mu.mem_write(0x541268,struct.pack('<I',133));mu.mem_write(0x541198,struct.pack('<I',82))
        if where=='Missile' and mut==6:mu.mem_write(OBJECT+42,struct.pack('<H',71))
        if where=='Sound' and mut==7:mu.mem_write(0x540478,struct.pack('<i',self.case['growl']));mu.mem_write(OBJECT+8,struct.pack('<I',51));mu.mem_write(self.p['now'],struct.pack('<d',100))

    def on_instruction(self,mu,address,size,data):
        """기존 공통 경계에 원본 소리 호출 뒤의 재진입 입력만 추가한다."""
        if address in (UNPOP,REPOP):
            esp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(esp);sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50;flags=self.read(esp+4 if address==UNPOP else esp+12);self.events.append(('U:' if address==UNPOP else 'P:')+str(sid)+':'+str(flags));mu.reg_write(UC_X86_REG_ESP,esp+(8 if address==UNPOP else 16));mu.reg_write(UC_X86_REG_EIP,ret);self.mutate('Unpop' if address==UNPOP else 'Repop');return
        sound=address==self.p['sound'];super().on_instruction(mu,address,size,data)
        if sound:self.mutate('Sound')

    def run_body(self,case):
        """실제 원문 반환 뒤 객체64BYTE·네 슬롯200BYTE·효과와 ABI를 확인한다."""
        self.case=case;self.events=[];case['direction']=0;mu=self.mu;sun=case['profile']==0;kind=case['kind'];typeid=128 if sun else 131
        fields=bytearray((i*13+7)&255 for i in range(64));struct.pack_into('<III',fields,0,0x50b318 if sun else 0x50b370,81,50);struct.pack_into('<ffff',fields,12,60,60,59.5,59.5);struct.pack_into('<I',fields,32,0x50b310 if sun else 0x50b368);fields[40]=0x20;struct.pack_into('<HH',fields,42,70,60);struct.pack_into('<dI',fields,48,10 if case['due'] else 10.25,0);mu.mem_write(OBJECT,bytes(fields));self.point=(60,50)
        for sid,t in ((50,typeid),(51,typeid),(60,158),(61,158),(70,34)): # 같은 실제 번호의 현재 슬롯을 독립 입력으로 준비한다.
            raw=bytearray(50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=t;raw[11]=case['targetstate'] if sid==60 else 0;raw[12]=0xa5;raw[34]=1;struct.pack_into('<ff',raw,14,50.5 if sid<60 else 60,50.5 if sid<60 else 60);struct.pack_into('<I',raw,36,case['frame']);mu.mem_write(self.slot(sid),bytes(raw))
        mu.mem_write(VTABLE,struct.pack('<I',PARENT_QUERY));mu.mem_write(VTABLE+0x48,struct.pack('<I',UNPOP));mu.mem_write(VTABLE+0x90,struct.pack('<I',REPOP));mu.mem_write(OBJECT+0x100,struct.pack('<I',VTABLE+0x200));mu.mem_write(VTABLE+0x21c,struct.pack('<I',STOP+0x100))
        source=TYPES+typeid*self.type_stride;mu.mem_write(source,bytes(self.type_stride));codes=letters(case['mode']);codeaddr=OUTPUT+0x100;mu.mem_write(codeaddr,b''.join(bytes((letter,0,0,0)) for letter in codes));mu.mem_write(source+0x124,struct.pack('<I',codeaddr))
        for letter in range(65,81): # 원본 TYPE+130/+170 표의 명시 입력을 만든다. 기대 출력은 계산하지 않는다.
            indices=[i for i,value in enumerate(codes) if value==letter];first=indices[0] if indices else -1;mu.mem_write(source+0x130+(letter-65)*4,struct.pack('<i',first));mu.mem_write(source+0x170+(letter-65)*4,struct.pack('<i',len(indices)))
        for address,value in ((self.p['gun'],34),(self.p['special'],148),(0x5410fc,35),(0x541198,80),(0x54109c,11),(0x541268,132),(0x541264,131),(0x540474,case['growl']),(0x540478,case['roar'])): # 실제 비교/getter 전역은 현재 입력이다.
            mu.mem_write(address,struct.pack('<I',value&0xffffffff))
        mu.mem_write(self.p['now'],struct.pack('<d',10));mu.mem_write(0x501a54,struct.pack('<f',20));mu.mem_write(0,bytes(4));self.write_ranges=[(0,4),(OBJECT,OBJECT+0x100),(ALLOC,ALLOC+0x200)]+[(self.slot(sid),self.slot(sid)+50) for sid in (50,51,60,61)]
        saved={reg:0x12340000+i for i,reg in enumerate((UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP))}
        for reg,value in saved.items():mu.reg_write(reg,value) # 비휘발 레지스터 보존도 반환 조건으로 검사한다.
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EDX):mu.reg_write(reg,0) # 이전 관찰의 임시 레지스터가 입력이 되지 않게 한다.
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,self.control);mu.reg_write(UC_X86_REG_FPTAG,0xffff);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_ECX,OBJECT);mu.reg_write(UC_X86_REG_ESP,STACK)
        args=[70,60] if kind==1 else [case['argument']&0xffffffff] if kind==2 else [];mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));entry=0x467330 if kind==0 else (0x468460 if sun else 0x4684d0) if kind==1 else (0x4684b0 if sun else 0x468520) if kind==2 else 0x466f50;mu.emu_start(entry,STOP,count=100000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+4+len(args)*4,(case,'return')
        assert all(mu.reg_read(reg)==value for reg,value in saved.items()),(case,'nonvolatile')
        assert not (mu.reg_read(UC_X86_REG_FPSW)&0x3800) and mu.reg_read(UC_X86_REG_FPCW)==self.control and self.read(0)==0,(case,'x87/SEH')
        slots=b''.join(bytes(mu.mem_read(self.slot(sid),50)) for sid in (50,51,60,61)).hex()
        return [str(mu.reg_read(UC_X86_REG_EAX)) if kind in (2,3) else 'return',';'.join(self.events),bytes(mu.mem_read(OBJECT,64)).hex(),slots]


def generate(smoke):
    """두 x87 정밀도의 실제 결과가 같은 고유 입력만 원문 fixture로 저장한다."""
    cases=inputs();cases=[sample(),sample(profile=0),sample(kind=1),sample(kind=2),sample(kind=3)] if smoke else cases
    assert len({tuple(case[key] for key in KEYS) for case in cases})==len(cases)
    oracles=[Oracle(0x027f),Oracle(0x037f)];lines=[]
    for index,case in enumerate(cases): # C++ 구현을 호출하지 않고 실제 PE 두 정밀도만 비교한다.
        output=[oracle.run_body(dict(case)) for oracle in oracles]
        if output[0]!=output[1]:raise RuntimeError('캐논 발사 x87 차이 '+str(index))
        lines.append('\t'.join([str(case[key]) for key in KEYS]+output[0]))
    if smoke:print('캐논 발사 원문 smoke 통과',len(cases));return
    FIXTURE.write_text('# 실제 초기화 이후10.78 썬/썬더 캐논 공통 Fire/단계/ctor/launch. Missile ctor는 외부 입력이다.\n'+'\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=set(oracles[0].exports+[ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__).resolve(),ROOT/'tools/decomp_crossbow_animation.py',ROOT/'tools/decomp_gunarcher_oracle.py',ROOT/'tools/decomp_gunanimation_oracle.py',ROOT/'tools/decomp_owner_oracle.py',FIXTURE])
    report={'host':socket.gethostname(),'date':'2026-10-16','last_full_refined_host':'VM-W11-Codex','unique_inputs':len(cases),'normal_returns':len(cases)*2,'precision_controls':[0x027f,0x037f],'precision_differences':0,'profiles':{str(profile):sum(c['profile']==profile for c in cases) for profile in (0,1)},'kinds':{str(kind):sum(c['kind']==kind for c in cases) for kind in range(4)},'limits':'실제 공통00467330과 단계/생성자/launch/getter/타입 코드 조회를 실행한다. 공간/중심/HP/Host/난수/format/Missile ctor는 명시 외부 입력이며 전체 투사체/게임 완주가 아니다. 전체64BYTE/네 슬롯200BYTE/효과/반환/스택/비휘발/x87/SEH/쓰기guard를 확인한다. 초기화 접미부004af182-004af196은 준비2회이며 관찰 수에 더하지 않는다.','game_gui_runs':0,'sha256':{path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('캐논 발사 원문 입력',len(cases),'정상 반환',len(cases)*2)


def verify():
    """원문 근거 SHA·고유 입력·행 수만 확인하며 기대값을 재계산하지 않는다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['sha256'].items(): # 실제 원본·관찰기·fixture 바이트를 고정한다.
        if digest(ROOT/name)!=expected:raise RuntimeError('캐논 발사 SHA 불일치 '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==report['unique_inputs'] and len({tuple(row[:len(KEYS)]) for row in rows})==len(rows);print('캐논 발사 원문/SHA 감사 통과',len(rows))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)
