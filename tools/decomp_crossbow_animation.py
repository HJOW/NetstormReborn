#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""크로스보우 회전·내림 발사·생성자의 실제10.78 명령을 독립 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_gunarcher_oracle import ArcherOracle, ARCHER, OBJECT, ALLOC, VTABLE, PARENT_QUERY, OUTPUT, TYPES, STACK, STOP, ROOT, bits
from decomp_gunanimation_oracle import SPEC, UNPOP, REPOP
from decomp_owner_oracle import OwnerOracle, POOL, digest
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG)

# 입력 필드와 독립 관찰 파일이다. 기대값은 실제 원본 실행에서만 얻는다.
KEYS = ('kind', 'frame', 'due', 'targetstate', 'hp', 'directions', 'frames', 'tx', 'ty', 'aim', 'target', 'mutation')
FIXTURE = ROOT / 'cpppj/tests/fixtures/crossbow-animation-x86.tsv'
REPORT = ROOT / 'cpppj/crossbow-animation.json'


def sample(**values):
    """기대 결과를 계산하지 않고 독립 입력의 기본 raw 값만 만든다."""
    result = dict(zip(KEYS, (0, 0, 1, 0, 1, 20, 60, bits(60), bits(50), 0, 60, 0)))
    result.update(values)
    return result


def inputs():
    """모든 물리 프레임·지연·죽은 표적·HP·방향 자료와 외부 재진입을 방문한다."""
    rows = []
    for frame, point, due, dead in itertools.product(range(40), ((60,50),(50,40),(40,50),(50,60),(60,60)), (0,1), (0,2)):
        rows.append(sample(frame=frame, tx=bits(point[0]), ty=bits(point[1]), due=due, targetstate=dead))
    for aim, frame in itertools.product(range(4), (0, 10, 38)):
        rows.append(sample(frame=frame, target=0, aim=aim))
    for directions, per in ((20,3), (16,2), (20,1)):
        for frame, due, dead, hp in itertools.product(range(directions*per), (0,1), range(4), (0,1)):
            rows.append(sample(kind=1, frame=frame, due=due, targetstate=dead, hp=hp, directions=directions, frames=directions*per))
    for kind in (2,3):
        rows.append(sample(kind=kind))
    for dead, hp in itertools.product(range(4), (0,1)):
        rows.append(sample(kind=4, targetstate=dead, hp=hp))
    for mutation, frame in itertools.product(range(1,7), (0,1,3,4)):
        rows.append(sample(kind=1, frame=frame, mutation=mutation))
    return rows


class Oracle(ArcherOracle):
    """공통 회전/CRT는 기존 원문, 발사·생성자·가상 getter는 새 검토 원문을 실행한다."""
    def __init__(self, control):
        """현재 PC의 정밀 범위와 직접 검토한 미소유 발사 함수만 허용한다."""
        OwnerOracle.__init__(self,'originals')
        self.control = control;self.a=ARCHER['originals'];self.p=SPEC['originals'];self.allowed=[];self.mu.mem_map(0,0x1000)
        self.boundaries=Counter();self.stub_calls=Counter();self.prefix_stops=0;self.events=[]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for entry in (0x467940,0x4676c0,0x41ce20,0x41cdb0,0x4e5e8a,0x4eded0,0x4ee1b7,0x4edf9a,0x4ac550,0x49a840,0x4ad390,0x468050,0x4687a0,0x466e50,0x466eb0,0x466f50,0x4680d0,0x4429e0):
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'))
                self.allowed.append((low,high+1))
        # 원문 첫 명령부터 각 RET/마지막 RET까지 PE를 직접 검토한 연속 구간이다.
        self.allowed.extend(((0x4674c0,0x46760a),(0x4680e0,0x4680e7),(0x468320,0x468327),(0x468820,0x468827),(0x466ab0,0x466ab4),(0x467f20,0x467f30),(0x467f30,0x467f40),(0x467f70,0x467f74),(0x4e5e94,0x4e5eb2),(0x4ee254,0x4ee25b)))
        assert self.read(0x50b120+0x18)==0x4674c0
        assert self.read(0x50b450+0x18)==0x467940
        # 실제004af170의 두 전역 쓰기 접미부만 실행한다. 앞선 풀/표 초기화는 외부 입력이다.
        self.allowed.append((0x4af182,0x4af197));self.write_ranges=[(0x545370,0x545378)]
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<I',STOP));self.mu.emu_start(0x4af182,STOP,count=8)
        assert self.read(0x545370)==bits(-10000) and self.read(0x545374)==bits(-10000)

    def mutate(self, where):
        """선언한 외부 효과가 현재 필드/시각을 변경하는 입력만 공급한다."""
        mut=self.case['mutation']
        if where=='Center' and mut==1:self.mu.mem_write(self.slot(50)+36,struct.pack('<I',7))
        if where=='Center' and mut==2:self.mu.mem_write(OBJECT+44,struct.pack('<H',61))
        if where=='Unpop' and mut==3:self.mu.mem_write(self.p['now'],struct.pack('<d',100));self.mu.mem_write(OBJECT+48,struct.pack('<d',777))
        if where=='Repop' and mut==4:self.mu.mem_write(self.slot(60)+11,bytes([2]))
        if where=='Point' and mut==5:
            self.mu.mem_write(OBJECT+8,struct.pack('<I',51));self.mu.mem_write(OBJECT+44,struct.pack('<H',61));self.mu.mem_write(OBJECT+12,struct.pack('<ff',77.25,78.5));self.mu.mem_write(0x50b168,struct.pack('<f',37));self.mu.mem_write(0x54119c,struct.pack('<I',82))
        if where=='Missile' and mut==6:self.mu.mem_write(OBJECT+42,struct.pack('<H',71))

    def on_instruction(self,mu,address,size,data):
        """공간/중심/Host/Missile ctor만 경계이며 발사/launch 판단은 실제 CPU다."""
        if address==self.p['parent']:
            esp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(esp)
            mu.reg_write(UC_X86_REG_EAX,self.slot(self.read(OBJECT+8)));mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret);return
        if address==self.p['missile']:
            esp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(esp);args=[self.read(esp+4+i*4) for i in range(12)]
            self.events.append('J:'+','.join(map(str,args)));self.mutate('Missile');mu.reg_write(UC_X86_REG_EAX,ALLOC);mu.reg_write(UC_X86_REG_ESP,esp+52);mu.reg_write(UC_X86_REG_EIP,ret);return
        where = {self.p['center']:'Center',UNPOP:'Unpop',REPOP:'Repop',self.p['point']:'Point'}.get(address)
        super().on_instruction(mu,address,size,data)
        if where:self.mutate(where)

    def run_body(self,case):
        """원문 반환·전체 객체/네 슬롯/효과와 스택·비휘발·x87/SEH를 관찰한다."""
        self.case=case;self.events=[];case['direction']=0;mu=self.mu;kind=case['kind'];fire=kind in (1,2,4)
        fields=bytearray((i*13+7)&255 for i in range(88));struct.pack_into('<III',fields,0,0x50b120 if fire else 0x50b450,81,50)
        struct.pack_into('<ffff',fields,12,60,60,59.5,59.5);struct.pack_into('<I',fields,32,0x50b11c if fire else 0x50b448);fields[40]=0x20
        struct.pack_into('<HH',fields,42,70,case['target']);struct.pack_into('<dI',fields,48,10 if case['due'] else 10.25,0);mu.mem_write(OBJECT,bytes(fields));mu.mem_write(OUTPUT,bytes(4))
        self.point=struct.unpack('<ff',struct.pack('<II',case['tx'],case['ty']))
        for sid,typeid in ((50,140),(51,140),(60,158),(61,158),(70,34)):
            raw=bytearray(50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=typeid;raw[11]=case['targetstate'] if sid==60 else 0;raw[12]=0xa5;raw[34]=1
            struct.pack_into('<ff',raw,14,50.5 if sid<60 else 60,50.5 if sid<60 else 60);struct.pack_into('<I',raw,36,case['frame']);raw[30]=case['aim'];mu.mem_write(self.slot(sid),bytes(raw))
        mu.mem_write(VTABLE,struct.pack('<I',PARENT_QUERY));mu.mem_write(VTABLE+0x48,struct.pack('<I',UNPOP));mu.mem_write(VTABLE+0x90,struct.pack('<I',REPOP));mu.mem_write(OBJECT+0x100,struct.pack('<I',VTABLE+0x200));mu.mem_write(VTABLE+0x21c,struct.pack('<I',STOP+0x100))
        source=TYPES+140*self.type_stride;mu.mem_write(source,bytes(self.type_stride));mu.mem_write(source+0xe8,struct.pack('<I',0x800));mu.mem_write(source+0x110,struct.pack('<ii',case['directions'],case['frames']))
        for address,value in ((self.a['checking'],0),(self.a['fpuBug'],0),(self.p['gun'],34),(self.p['special'],148),(0x5410fc,35),(0x541100,36),(0x54119c,81)):
            mu.mem_write(address,struct.pack('<I',value))
        mu.mem_write(self.p['now'],struct.pack('<d',10));mu.mem_write(0x50b168,struct.pack('<f',35));mu.mem_write(0,bytes(4))
        self.write_ranges=[(0,4),(OBJECT,OBJECT+0x100),(ALLOC,ALLOC+0x200),(OUTPUT,OUTPUT+4)]+[(self.slot(sid),self.slot(sid)+50) for sid in (50,51,60,61)]
        saved={reg:0x12340000+i for i,reg in enumerate((UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP))}
        for reg,value in saved.items():mu.reg_write(reg,value)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EDX):mu.reg_write(reg,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,self.control);mu.reg_write(UC_X86_REG_FPTAG,0xffff);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_ECX,OBJECT);mu.reg_write(UC_X86_REG_ESP,STACK)
        args=[70,case['target']] if kind in (2,3) else [];mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));entry=(0x467940,0x4674c0,0x468050,0x4687a0,0x466f50)[kind]
        mu.emu_start(entry,STOP,count=100000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+4+len(args)*4,(case,'return')
        assert all(mu.reg_read(reg)==value for reg,value in saved.items()),(case,'nonvolatile')
        assert not (mu.reg_read(UC_X86_REG_FPSW)&0x3800) and mu.reg_read(UC_X86_REG_FPCW)==self.control and self.read(0)==0,(case,'x87/SEH')
        image=bytes(mu.mem_read(OBJECT,64 if fire else 88)).hex();slots=b''.join(bytes(mu.mem_read(self.slot(sid),50)) for sid in (50,51,60,61)).hex()
        return [str(mu.reg_read(UC_X86_REG_EAX)) if kind==4 else 'return',';'.join(self.events),image,slots]


def generate(smoke):
    """두 정밀도에서 같은 실제 출력을 확인한 후 고유 입력만 fixture에 저장한다."""
    cases=inputs();cases=cases[:8]+[sample(kind=1),sample(kind=2),sample(kind=3),sample(kind=4)] if smoke else cases
    assert len({tuple(c[key] for key in KEYS) for c in cases})==len(cases)
    oracles=[Oracle(0x027f),Oracle(0x037f)];lines=[]
    for index,case in enumerate(cases):
        output=[oracle.run_body(dict(case)) for oracle in oracles]
        if output[0]!=output[1]:raise RuntimeError('크로스보우 x87 정밀도 차이 '+str(index))
        lines.append('\t'.join([str(case[key]) for key in KEYS]+output[0]))
    if smoke:print('크로스보우 원본 smoke 통과',len(lines));return
    FIXTURE.write_text('# 실제10.78 크로스보우 회전/발사/생성자. Missile 생성자는 선언한 외부 경계다.\n'+'\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=set(oracles[0].exports+[ROOT/'originals/Netstorm.exe',ROOT/'tools/decomp_crossbow_animation.py',ROOT/'tools/decomp_gunarcher_oracle.py',ROOT/'tools/decomp_gunanimation_oracle.py',ROOT/'tools/decomp_owner_oracle.py',ROOT/'extracted/refined/originals/functions.tsv',FIXTURE])
    report={'host':socket.gethostname(),'date':'2026-10-16','last_full_refined_host':'VM-W11-Codex','unique_inputs':len(cases),'normal_returns':len(cases)*2,'precision_controls':[0x027f,0x037f],'precision_differences':0,'kinds':{str(kind):sum(c['kind']==kind for c in cases) for kind in range(5)},'invalid_coordinate_setup':{'entry_fragment':'004af182-004af196','runs':2,'scope':'실제 초기화 함수의 마지막 두 쓰기만 실행한다. 전체 초기화나 새 독립 입력으로 계수하지 않는다.'},'limits':'실제 회전/발사/공통 launch/getter/생성자를 실행한다. 공간/중심/Host/HP/난수/문자열 format/Missile ctor는 명시 외부 입력이다. ctor 경계를 대체했으므로 전체 투사체/게임 완주 증거가 아니다. 전체 객체/네 슬롯200BYTE/순서/반환/스택/비휘발/x87/SEH/쓰기guard를 대조한다.','game_gui_runs':0,'sha256':{path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('크로스보우 실제 원본 입력',len(cases),'정상 반환',len(cases)*2)


def verify():
    """저장된 원문 근거·입력 고유성·행 수만 확인하며 기대 결과는 다시 계산하지 않는다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['sha256'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('크로스보우 SHA 불일치 '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==report['unique_inputs'] and len({tuple(row[:len(KEYS)]) for row in rows})==len(rows)
    print('크로스보우 원본 입력/SHA 검증 통과',len(rows))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)
