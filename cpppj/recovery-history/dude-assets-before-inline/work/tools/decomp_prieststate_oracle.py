#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 HP/지면 상태와 HP setter의 전체 몸체를 실제 세 PE에서 대조한다. 게임/OS 실행은 없다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path

from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,TYPES,PLAYERS,TARGET,STACK,STOP,digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 실제 두 판본의 조회/setter 및 HP 모드/spot/권한 전역이다.
STATE={
    'originals':dict(wrapper=0x427030,ground=0x492090,setter=0x4942b0,vtable=0x50f210,spot=0x5c7c44,
        quarter=0x5ca8f0,priest=0x5412d0,boss=0x540bc4,player_flag=0x7c),
    'originalCD':dict(wrapper=0x4e5150,ground=0x40d7b0,setter=0x40ca40,vtable=0x5003e0,spot=0x52fe48,
        quarter=0x511c90,priest=0x51cbbc,boss=0x540a2c,player_flag=0x74),
}
STATE['original1037']=STATE['originalCD']
# 가상 spot 메모리, 두 x87 정밀도와 저장 경로다.
SPOTS=0x13000000
CONTROLS=(0x027f,0x037f)
FIXTURE=ROOT/'cpppj/tests/fixtures/prieststate-x86.tsv'
REPORT=ROOT/'cpppj/recovery-prieststate-evidence.json'
# 경계 근처·정수/소수 좌표다. 거의 올림의 float 재저장 오류를 구별하는 20.000099도 넣는다.
COORDS=((20.75,21.9),(20.0,21.0),(20.000099182128906,21.000099182128906),(0.0,0.0),(254.0,254.0))


def bits(value):
    """float 입력을 원본 DWORD 비트로 만든다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def inputs():
    """분기/폭/절반 경계 입력만 만든다. 기대값은 실제 PE가 정한다."""
    cases=[]
    # 모든 조회 경로의 HP/표면/강제 비트와 판본별 부호 확장 경계를 교차한다.
    for hp,quarter,genus,current,extra,spot in itertools.product((200,201,5,-7),(0,1),(0,0x210000),
        (-2147483648,-32769,-32768,-1,0,1,24,25,49,50,99,100,101,200,32767,32768,65535),(0,32),(0,2,4,6,8)):
        for kind in ('Ground','Wrapper'):
            cases.append(dict(kind=kind,hp=hp,quarter=quarter,genus=genus,current=current&0xffffffff,extra=extra,spot=spot,
                new=0,boss=0,allow=0,owner=0,after='-'))
    # HP setter의 양방향 절반 경계·권한/소유자 조건·CD 저장 폭을 대조한다.
    for hp,quarter,current,new,boss,allow in itertools.product((200,201),(0,1),(-1,24,25,99,100,101,199,200,32768),
        (-5,24,25,99,100,101,250,65535,65536),(0,1),(0,1)):
        cases.append(dict(kind='Set',hp=hp,quarter=quarter,genus=0x210000,current=current&0xffffffff,
            extra=0,spot=6,new=new,boss=boss,allow=allow,owner=1,after='-'))
    # Unpop 뒤 현재 HP/좌표 재조회와 중립/마지막 실제 소유자 칸을 검사한다.
    for current,new,owner,after in itertools.product((20,150),(20,150),(0,8),(20,150)):
        cases.append(dict(kind='Set',hp=200,quarter=0,genus=0x210000,current=current,extra=32,spot=6,
            new=new,boss=1,allow=1,owner=owner,after=str(after)))
    # 좌표 비트를 고정 순서로 공급한다. 절삭 칸과 거의 올림 칸의 spot이 서로 다르게 된다.
    for index,case in enumerate(cases):
        x,y=COORDS[index%len(COORDS)];case.update(x=bits(x),y=bits(y))
    return cases


class StateOracle(OwnerOracle):
    """조회는 대체 없이, setter는 두 가상 공간 호출만 기록 대체하여 실행한다."""
    def __init__(self,edition):
        """읽기 전용 함수 범위와 실제 사제 가상 공간 진입 주소를 준비한다."""
        super().__init__(edition)
        self.p=STATE[edition];self.mu.mem_map(SPOTS,0x10000)
        self.mu.mem_write(self.p['spot'],struct.pack('<I',SPOTS))
        self.exports.extend(ROOT/f'extracted/prieststate/{edition}/{name}' for name in ('creation.c','functions.tsv'))
        with self.exports[-1].open(encoding='utf-8') as fp:
            # 불연속 함수 몸체 밖 명령은 허용하지 않는다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.virtual={struct.unpack('<I',self.mu.mem_read(self.p['vtable']+offset,4))[0]:name
            for offset,name in ((0x48,'U'),(0x4c,'R'))}
        self.events=[];self.stub_calls=collections.Counter()

    def hp(self):
        """현재 raw HP를 판본의 원본 폭으로 읽는다."""
        return struct.unpack('<i' if self.stride==50 else '<h',self.mu.mem_read(self.slot(TARGET)+26,4 if self.stride==50 else 2))[0]

    def on_instruction(self,mu,address,size,data):
        """setter의 두 가상 공간 호출만 인자와 호출 시점 raw HP/좌표를 기록한다."""
        if self.case['kind']=='Set' and address in self.virtual:
            esp=mu.reg_read(UC_X86_REG_ESP);ret,flags=struct.unpack('<II',mu.mem_read(esp,8))
            if mu.reg_read(UC_X86_REG_ECX)!=self.slot(TARGET):raise RuntimeError('가상 공간 this 오류')
            marker=self.virtual[address];self.stub_calls[marker]+=1
            x,y=struct.unpack('<II',mu.mem_read(self.slot(TARGET)+14,8))
            self.events.append(f'{marker}:{TARGET}:{flags}:{self.hp()}:{x}:{y}')
            if marker=='U' and self.case['after']!='-':
                mu.mem_write(self.slot(TARGET)+26,struct.pack('<I' if self.stride==50 else '<H',int(self.case['after'])))
                mu.mem_write(self.slot(TARGET)+14,struct.pack('<ff',30.75,31.9))
            mu.reg_write(UC_X86_REG_ESP,esp+8);mu.reg_write(UC_X86_REG_EIP,ret);return
        super().on_instruction(mu,address,size,data)

    def run(self,case,control):
        """실제 몸체 정상 반환·callee 스택·x87 복구·허용 쓰기를 검사한다."""
        self.case=case;self.events=[];slot=self.slot(TARGET)
        raw=bytearray([0xab]*self.stride);struct.pack_into('<I',raw,0,self.p['vtable'])
        raw[10]=158;raw[11]=0;raw[self.o['extra']]=case['extra'];raw[self.o['owner']]=case['owner']
        struct.pack_into('<II',raw,14,case['x'],case['y'])
        struct.pack_into('<I' if self.stride==50 else '<H',raw,26,case['current'] if self.stride==50 else case['current']&0xffff)
        self.mu.mem_write(slot,bytes(raw))
        base=TYPES+158*self.type_stride;self.mu.mem_write(base,bytes(self.type_stride))
        self.mu.mem_write(base,struct.pack('<i',case['hp']));self.mu.mem_write(base+0xe8,struct.pack('<II',0x69012,case['genus']))
        self.mu.mem_write(self.p['quarter'],struct.pack('<I',case['quarter']))
        self.mu.mem_write(self.p['priest'],struct.pack('<I',158));self.mu.mem_write(self.p['boss'],struct.pack('<I',case['boss']))
        self.mu.mem_write(PLAYERS+case['owner']*self.player_stride+self.p['player_flag'],struct.pack('<I',case['allow']))
        self.mu.mem_write(0x5e4794 if self.stride==50 else 0x546778,bytes(4))
        # 체커보드는 절삭과 거의 올림, 한 축만 잘못 읽은 경우를 구별한다.
        board=(bytes((case['spot'],case['spot']^6))*128+bytes((case['spot']^6,case['spot']))*128)*128
        self.mu.mem_write(SPOTS,board)
        self.write_ranges=[] if case['kind']!='Set' else [(slot+14,slot+22),(slot+26,slot+26+(4 if self.stride==50 else 2))]
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,slot)
        if case['kind']=='Wrapper':entry=self.p['wrapper'];self.mu.mem_write(STACK,struct.pack('<I',STOP));end=STACK+4
        elif case['kind']=='Ground':entry=self.p['ground'];self.mu.mem_write(STACK,struct.pack('<II',STOP,TARGET));end=STACK+4
        else:entry=self.p['setter'];self.mu.mem_write(STACK,struct.pack('<II',STOP,case['new']&0xffffffff));end=STACK+8
        self.mu.emu_start(entry,STOP,count=20000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=end:raise RuntimeError('반환/스택 오류')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('x87 복구 오류')
        return [str(self.mu.reg_read(UC_X86_REG_EAX)) if case['kind']!='Set' else '-', ';'.join(self.events) or '-',
            bytes(self.mu.mem_read(slot,self.stride)).hex()]


def generate(smoke=False):
    """두 정밀도가 같을 때만 한 관찰을 저장하고 실제 PE/함수/도구 SHA를 기록한다."""
    cases=inputs();rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/ghidra/prieststate-functions.json',FIXTURE}
    if smoke:cases=cases[::97]
    columns=('hp','quarter','genus','current','extra','spot','x','y','new','boss','allow','owner','after')
    for edition in SPECS:
        oracle=StateOracle(edition)
        for case in cases:
            values=[oracle.run(case,control) for control in CONTROLS]
            if values[0]!=values[1]:raise RuntimeError(f'x87 차이: {edition}')
            rows.append([case['kind'],edition,*[case[name] for name in columns],*values[0]])
        editions[edition]=dict(binary=oracle.spec['binary'],cases=len(cases),native_calls=dict(oracle.native_calls),
            substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary'])
        print(f'{edition}: HP/지면/setter {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 사제 상태 조회/setter. 조회는 대체 없음, setter는 Unpop/Repop만 기록 대체.\n'+
        '# kind edition maxHP quarter genus currentBits extra spot xbits ybits newHP boss allow owner afterHP result events slot\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-08',normal_return=True,os_calls=0,
        controls=list(CONTROLS),editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        limits=['정상 합성 타입/spot/raw 입력, 판본별 HP 폭 및 동일 관찰의 두 x87 정밀도',
            'setter의 사제 Unpop/Repop은 기록/입력 변화 대체, 회복 이벤트/보호막/GUI는 실행하지 않음']),
        ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """입력 SHA와 정상 반환·행 수·실제 진입/가상 호출 증거를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows)!=report['total'] or not report['normal_return'] or report['os_calls']:raise RuntimeError('실행 경계 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[1]==edition]
        if item['cases']!=len(selected) or item['assertions']:raise RuntimeError('판본 관찰 오류')
        for kind,name in (('Wrapper','wrapper'),('Set','setter')):
            if item['native_calls'].get(f'{STATE[edition][name]:08x}',0)!=sum(row[0]==kind for row in selected)*2:
                raise RuntimeError('실제 진입 수 오류')
        for marker in ('U','R'):
            if item['substitutions'].get(marker,0)!=sum(row[-2].count(marker+':') for row in selected)*2:raise RuntimeError('가상 효과 수 오류')
    print(f'prieststate 검증 통과: {len(rows)}개')


def main():
    """전체 생성/저장된 증거 감사/무저장 소규모 실행을 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true')
    args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
