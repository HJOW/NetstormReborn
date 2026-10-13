#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Damageable 효과/소리 접두와 Carrier 연결을 실제 세 PE에서 대조한다.

효과·소리·공통 pre/Carrier 전역 후처리 호출과 권한 해방의 중간 코드 구간만 대체한다.
spot의 +0.9999·CRT 절삭·contained Begin/Next/true 필터·바깥 분기는 실제 실행한다.
원본 게임/OS/창 실행은 없다.
"""
import argparse
import collections
import csv
import itertools
import json
import random
import struct
from pathlib import Path
from decomp_owner_oracle import (OwnerOracle, ROOT, SPECS, POOL, STOP, STACK, TARGET, digest,
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI,
    UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)
from unicorn.x86_const import UC_X86_REG_FPCW, UC_X86_REG_FPSW

# 독립 메모리·관찰 슬롯·두 x87 정밀도와 입력 시드다.
SPOTS, CAPACITY, IDS, CONTROLS, SEED=0x14000000,24000,(50,60,61,62,63),(0x027f,0x037f),0x44b4b0
# 판본별 실제 진입/외부 경계와 명시적으로 대체할 권한 해방 구간의 양끝이다.
PRE={
 'originals':dict(entry=0x44b4b0,carrier=0x426890,query=0x4ac9f0,begin=0x4b2270,next=0x4b1b20,filter=0x44daa0,ftol=0x4e49c0,
    collapse=0x4605f0,explosion=0x4605d0,at=0x4a9d70,free=0x4a9cb0,base=0x4b0950,found=0x485e40,
    release=0x44b626,resume=0x44b80c,boss=0x540bc4,type=0x541088,priest=0x5412d0,spots=0x5c7c44),
 'originalCD':dict(entry=0x4615e0,carrier=0x4e43b0,query=0x4ac560,begin=0x4eb8d0,next=0x4eb900,filter=0x40f230,ftol=0x4f161c,
    collapse=0x454800,explosion=0x4547d0,at=0x438c00,free=0x438b50,base=0x4add20,found=0x4151d0,
    release=0x46174e,resume=0x46195a,boss=0x540a2c,type=0x51c978,priest=0x51cbbc,spots=0x52fe48),
}
PRE['original1037']=dict(PRE['originalCD'])
# 앞 단계와 분리된 원본 관찰/감사 기록이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/damageablepredestroy-x86.tsv'
REPORT=ROOT/'cpppj/recovery-damageablepredestroy-evidence.json'


def bits(value):
    """좌표 입력을 단정도 비트로 보존한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def inputs():
    """각 분기와 효과 중 좌표/전역/체인 변경을 만든다. 기대 판단은 원본이 한다."""
    chains=[(0,''),(60,'60,6,8,0,158,1'),(60,'60,46,8,61,158,1;61,6,1,62,158,1;62,6,6,63,159,1;63,6,9,0,158,1'),
        (60,'60,6,0,61,158,1;61,6,2,62,158,1;62,6,4,63,158,1;63,6,8,0,159,1')]
    maps=('20,21,6','21,22,6','20,21,16;21,22,22','20,21,0;21,22,0')
    positions=((20,21),(20.0001,21.0001),(20.00015,21.00015),(20.75,21.9))
    cases=[]
    # ordinary 진입·두 효과의 독립 flags·지면 경계·현재 권한을 교차한다.
    for chain,flags,extra,marks,boss in itertools.product(chains,(0,0x200000,0x100000,0x300000,0x300800,0xffffffff),(0,1,8,9),maps,(0,1)):
        x,y=positions[(len(cases)//2)%len(positions)]
        cases.append(dict(state=2,extra=extra,boss=boss,type=6,priest=158,flags=flags,x=bits(x),y=bits(y),marks=marks,head=chain[0],nodes=chain[1],style=0))
    rng=random.Random(SEED)
    # 1: 좌표/extra 변경, 2: 소리 중 saved next/현재 전역 변경, 3: 공통 body 뒤 Carrier 조회 변경이다.
    for i in range(48):
        style=1+i%3
        cases.append(dict(state=rng.choice((0,2,4,6)),extra=0,boss=i%2,type=6 if style==2 else rng.choice((6,46,256,0xffffffff)),
            priest=158 if style==2 else rng.choice((158,159,65536,0xffffffff)),flags=rng.choice((0x300000,0x300800)) if style!=2 else 0,
            x=bits(20.00015),y=bits(21.00015),marks='20,21,6;21,22,6;24,25,6',head=60,
            nodes=chains[3][1],style=style))
    return cases


class DamageableOracle(OwnerOracle):
    """읽기 전용 원본 PE에서 실제 접두/순회와 바깥 조건만 허용한다."""
    def __init__(self,edition):
        """세 판본 내보내기 범위와 합성 raw/map을 준비한다."""
        super().__init__(edition);self.p=PRE[edition];base=list(self.exports);self.stub_calls=collections.Counter()
        self.mu.mem_map(POOL+0x2000,((CAPACITY*self.stride+4095)&~4095)-0x2000);self.mu.mem_map(SPOTS,0x10000)
        self.mu.mem_write(self.p['spots'],struct.pack('<I',SPOTS))
        self.exports=[ROOT/f'extracted/damageablepredestroy/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 내보낸 몸체 밖의 실행을 허용하지 않는다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.exports=base+self.exports
        if edition=='originals':self.mu.mem_write(0x5e4794,struct.pack('<I',1))

    def set_coords(self,x,y):
        """외부 효과가 현재 raw 좌표를 바꾸는 합성 입력이다."""
        self.mu.mem_write(self.slot(TARGET)+14,struct.pack('<II',bits(x),bits(y)))

    def mutation(self,marker):
        """기대 분기 없이 경계마다 같은 raw/전역 변경 입력을 넣는다."""
        style=self.case['style']
        if style==1:
            if marker=='C':self.set_coords(21.00001,22.00001);self.mu.mem_write(self.slot(TARGET)+self.o['extra'],bytes((9,)))
            if marker=='A0':self.set_coords(22.75,23.9);self.mu.mem_write(self.p['boss'],struct.pack('<I',1-self.case['boss']))
            if marker=='X':self.set_coords(23,24)
            if marker=='A1':self.set_coords(24.00001,25.00001)
        if style==2 and marker=='F' and self.free_count==1:
            self.mu.mem_write(self.p['boss'],struct.pack('<I',1-self.case['boss']));self.mu.mem_write(self.p['type'],struct.pack('<I',46));self.mu.mem_write(self.p['priest'],struct.pack('<I',159))
            self.mu.mem_write(self.slot(TARGET)+6,bytes(2));self.mu.mem_write(self.slot(60)+4,bytes(2))
            self.mu.mem_write(self.slot(61)+10,bytes((46,)));self.mu.mem_write(self.slot(61)+18,struct.pack('<H',159))
        if style==3 and marker=='B':
            self.mu.mem_write(self.p['boss'],struct.pack('<I',1));self.mu.mem_write(self.p['type'],struct.pack('<I',6))
            self.mu.mem_write(self.slot(TARGET)+6,struct.pack('<H',63));self.mu.mem_write(self.slot(63)+10,bytes((6,)))
            self.mu.mem_write(self.slot(63)+20,struct.pack('<H',1))

    def ret(self,purge=0):
        """함수 경계의 반환 주소와 callee 정리만 대체한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',self.mu.mem_read(esp,4))[0]
        self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,ret)

    def on_instruction(self,mu,address,size,data):
        """효과/소리/공통 body 및 권한 해방 구간을 명시적으로 대체한다."""
        p=self.p;esp=mu.reg_read(UC_X86_REG_ESP)
        if address in (p['collapse'],p['explosion']):
            sid=struct.unpack('<I',mu.mem_read(esp+4,4))[0]
            if sid!=TARGET:raise RuntimeError('효과 SID 오류')
            marker='C' if address==p['collapse'] else 'X';self.events.append(f'{marker}:{sid}');self.stub_calls[marker]+=1;self.mutation(marker);self.ret();return
        if address==p['at']:
            x,y,name,flags=struct.unpack('<4I',mu.mem_read(esp+4,16))
            filename=bytes(mu.mem_read(name,32)).split(b'\0')[0].decode('ascii')
            marker='A0' if filename=='collapse.wav' else 'A1' if filename=='explosion.wav' else None
            if marker is None or flags:raise RuntimeError('좌표 소리 인자 오류')
            self.events.append(f'{marker}:{x}:{y}');self.stub_calls[marker]+=1;self.mutation(marker);self.ret();return
        if address==p['free']:
            name,*args=struct.unpack('<6I',mu.mem_read(esp+4,24))
            if bytes(mu.mem_read(name,15)).split(b'\0')[0]!=b'priestFree.wav' or args!=[0,0,0,1,0]:raise RuntimeError('전역 해방 소리 인자 오류')
            self.events.append('F');self.free_count+=1;self.stub_calls['F']+=1;self.mutation('F');self.ret();return
        if address==p['release']:
            if mu.reg_read(UC_X86_REG_ESI)!=self.slot(TARGET):raise RuntimeError('해방 구간 부모 오류')
            self.events.append(f'R:{TARGET}');self.stub_calls['R-region']+=1
            # 구간 첫 명령 전에 건너뛰므로 원래 push/pop·x87 균형을 그대로 유지한다.
            mu.reg_write(UC_X86_REG_EIP,p['resume']);return
        if address==p['base']:
            flags=struct.unpack('<I',mu.mem_read(esp+4,4))[0]
            if flags!=self.case['flags'] or mu.reg_read(UC_X86_REG_ECX)!=self.slot(TARGET):raise RuntimeError('공통 pre 인자 오류')
            self.events.append(f'B:{TARGET}:{flags}');self.stub_calls['B']+=1;self.mutation('B');self.ret(4);return
        if address==p['found']:
            self.events.append('T');self.stub_calls['T']+=1;self.ret();return
        super().on_instruction(mu,address,size,data)

    def run(self,case,mode,control):
        """직접 Damageable 또는 실제 Carrier 호출의 전체 정상 반환을 관찰한다."""
        self.case=case;self.events=[];self.free_count=0;self.instructions=0;self.write_ranges=[]
        for key in ('boss','type','priest'):self.mu.mem_write(self.p[key],struct.pack('<I',case[key]))
        self.mu.mem_write(SPOTS,bytes(0x10000))
        # 입력한 map 셀만 표시해 +0.9999의 경계 선택을 원본이 결정하게 한다.
        for mark in case['marks'].split(';'):
            x,y,value=map(int,mark.split(','));self.mu.mem_write(SPOTS+y*256+x,bytes((value,)))
        for sid in IDS:
            raw=bytearray([0xab]*self.stride);raw[10]=158;raw[11]=case['state'] if sid==TARGET else 0;raw[4:8]=bytes(4)
            raw[self.o['extra']]=case['extra'] if sid==TARGET else 0;self.mu.mem_write(self.slot(sid),bytes(raw))
        self.mu.mem_write(self.slot(TARGET)+14,struct.pack('<II',case['x'],case['y']));self.mu.mem_write(self.slot(TARGET)+6,struct.pack('<H',case['head']))
        # 종속의 상태도 입력 그대로 두며 후보 제거 조건을 만들지 않는다.
        for node in filter(None,case['nodes'].split(';')):
            sid,typ,state,next_sid,kind,mask=map(int,node.split(','));slot=self.slot(sid)
            self.mu.mem_write(slot+4,struct.pack('<H',next_sid));self.mu.mem_write(slot+10,bytes((typ,state)));self.mu.mem_write(slot+18,struct.pack('<HH',kind,mask))
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        for reg,value in preserved.items():self.mu.reg_write(reg,value)
        self.mu.reg_write(UC_X86_REG_EAX,0);self.mu.reg_write(UC_X86_REG_EDX,0);self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,self.slot(TARGET));self.mu.mem_write(STACK,struct.pack('<II',STOP,case['flags']))
        self.mu.emu_start(self.p['entry' if mode=='D' else 'carrier'],STOP,count=30000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+8:raise RuntimeError('Damageable 전체 반환/스택 오류')
        if any(self.mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('Damageable 보존 레지스터 오류')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('Damageable x87 복구 오류')
        globals_out=','.join(str(struct.unpack('<I',self.mu.mem_read(self.p[key],4))[0]) for key in ('boss','type','priest'))
        return [globals_out,';'.join(self.events),'|'.join(bytes(self.mu.mem_read(self.slot(sid),self.stride)).hex() for sid in IDS)]


def generate(smoke=False):
    """두 정밀도의 관찰이 같은 입력을 fixture와 SHA 근거에 저장한다."""
    cases=inputs();rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/ghidra/damageablepredestroy-functions.json',FIXTURE}
    if smoke:cases=cases[::51]
    # 실제 세 PE에서 Damageable와 Carrier 전체 호출을 별도로 대조한다.
    for edition in SPECS:
        oracle=DamageableOracle(edition)
        for case in cases:
            for mode in ('D','C'):
                values=[oracle.run(case,mode,control) for control in CONTROLS]
                if values[0]!=values[1]:raise RuntimeError('Damageable x87 정밀도 차이')
                rows.append([edition,mode,case['state'],case['extra'],case['boss'],case['type'],case['priest'],case['flags'],case['x'],case['y'],case['marks'],case['head'],case['nodes'],case['style'],*values[0]])
        editions[edition]=dict(cases=2*len(cases),native_calls=dict(oracle.native_calls),substitutions=dict(oracle.stub_calls),assertions=oracle.assertions,
            replaced_region=[f'{oracle.p["release"]:08x}',f'{oracle.p["resume"]:08x}'])
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: Damageable/Carrier {2*len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 Damageable 소리 접두·contained 순회·Carrier 전체 반환. 권한 해방 중간 구간 및 하위 효과/소리/공통 body를 대체.\n# edition mode state extra boss type priest flags x y marks head nodes style globalsOut events slots\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-08',normal_return=True,os_calls=0,controls=list(CONTROLS),editions=editions,
        files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        limits=['효과/소리/공통 pre/Carrier 전역 후처리 함수 호출을 기록 대체','권한 해방 구간은 첫 명령에서 종료 구간으로 건너뛰며 조건만 실제 실행',
            'spot +0.9999·CRT 절삭·contained 순회/true 필터·바깥 분기는 실제 실행','일반 공간 수명·파편/소리 출력·해방 생성/배치의 완성을 뜻하지 않음']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA·행·정밀도·실제 진입과 명시적 구간/함수 대체 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
    if len(rows)!=report['total'] or not report['normal_return'] or report['os_calls'] or report['controls']!=list(CONTROLS):raise RuntimeError('Damageable 증거 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];p=PRE[edition];n=len(selected)
        if n!=item['cases'] or item['assertions'] or item['native_calls'].get(f'{p["entry"]:08x}')!=2*n:raise RuntimeError('Damageable 실제 진입 오류')
        if item['native_calls'].get(f'{p["carrier"]:08x}')!=n or item['substitutions'].get('B')!=2*n:raise RuntimeError('Carrier/공통 호출 오류')
        for marker in ('C','X','A0','A1','F','R-region','B','T'):
            event='R' if marker=='R-region' else marker
            count=sum(sum(part.split(':')[0]==event for part in row[15].split(';')) for row in selected)*2
            if count!=item['substitutions'].get(marker,0):raise RuntimeError(f'대체 수 오류: {marker}')
        for key in ('begin','next','filter','ftol'):
            if not item['native_calls'].get(f'{p[key]:08x}'):raise RuntimeError(f'실제 조회/절삭 누락: {key}')
        if item['replaced_region']!=[f'{p["release"]:08x}',f'{p["resume"]:08x}']:raise RuntimeError('해방 구간 대체 경계 오류')
        if any(item['native_calls'].get(f'{p[key]:08x}',0) for key in ('collapse','explosion','at','free','base','found')):raise RuntimeError('외부 대체의 실제 실행 혼합')
    print(f'damageablepredestroy 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
