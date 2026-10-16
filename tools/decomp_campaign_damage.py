#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재10.78 TYPE131의 실제 일반 피격·기본 방어를 독립 캠페인 입력으로 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from pathlib import Path
from decomp_damageablehit_oracle import GeneralHitOracle, HIT, KEYS as BASE_KEYS, GROUND, sample, double_bits
from decomp_priestdamage_oracle import SPEC as DAMAGE, CREATED, DESTROY
from decomp_prieststate_oracle import STATE
from decomp_owner_oracle import OwnerOracle, ROOT, TYPES, STACK, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG)

# 기존5846행과 별도로 기록하는 새 입력 열·fixture·보고서다.
KEYS=(*BASE_KEYS,'group')
FIXTURE=ROOT/'cpppj/tests/fixtures/campaign-damage-x86.tsv'
REPORT=ROOT/'cpppj/campaign-damage.json'


def inputs():
    """일반HP/NoDamage·rank·경고·held/공간/burning 및 서버 flags를 독립 방문한다."""
    rows=[]
    # 실제 일반 피격의 HP1/생존/서버 사망과 피해0/24/200을 교차한다.
    for hp,damage,server,nodamage,disabled in itertools.product((1,150),(0,24,200),(0,1),(0,1),(0,1)):
        rows.append(sample(kind=1,current=hp,damage=damage,server=server,nodamage=nodamage,disabled=disabled,group=0))
    # Challenge rank 변환·보정 차단과 중립 소유자 자연 분기를 구별한다.
    for ranks,disabled,owners in itertools.product(((3,1),(1,3),(11,0),(0,11)),(0,1),((2,1),(0,1),(2,0),(9,1))):
        rows.append(sample(kind=1,arank=ranks[0],vrank=ranks[1],disabled=disabled,attackerOwner=owners[0],victimOwner=owners[1],group=1))
    # 주 섬/다른 섬·3x3/다리와 경고 마감 전·동률·후를 관찰한다.
    for deadline,genus,primary in itertools.product((0,10.1,10.2),(2,0x1000002,4),(3,4)):
        rows.append(sample(kind=1,localOwner=1,warning=double_bits(deadline),islandGenus=genus,primary=primary,group=2))
    # 현재 extra의 held/free/unrealize와 HP보유/dais 조건을 따로 방문한다.
    for extra,state,hasHP,dais in ((1,0,1,0),(9,0,1,0),(4,0,1,0),(0,1,1,0),(0,0,0,0),(0,0,0,1),(1,0,0,1)):
        rows.append(sample(kind=1,extra=extra,state=state,hasHP=hasHP,dais=dais,group=3))
    # 실제 burning 판단을 실행하며 생성/삭제 하위는 명시 경계로 기록한다.
    for current,existing,damage in itertools.product((99,150),(0,1),(0,24)):
        rows.append(sample(kind=1,current=current,existing=existing,damage=damage,effect=1,server=0,group=3))
    # 서버의 일반 폭발/공격자 bomb genus와 중립 owner mask를 구별한다.
    for owner,genus in itertools.product((0,2),(0,0x100)):
        rows.append(sample(kind=1,current=1,damage=200,attackerOwner=owner,attackerGenus=genus,group=3))
    # 겹치는 원시 입력은 한 번만 저장한다. group도 검사 분류를 위한 독립 열이다.
    return list({tuple(row[key] for key in KEYS):row for row in rows}.values())


class Oracle(GeneralHitOracle):
    """기존 수치 관찰 경계를 재사용하되 현재 전체 정밀 범위와 실제 기본 방어를 실행한다."""
    def __init__(self,control):
        """선택 export에 의존하지 않고 같은 PC의 현재 전체 정밀 자료만 읽는다."""
        OwnerOracle.__init__(self,'originals')
        self.p=STATE['originals'];self.d=DAMAGE['originals'];self.h=HIT['originals'];self.control=control
        self.allowed=[];self.virtual={};self.events=[];self.boundaries=Counter();self.stub_calls=Counter();self.returns=0;self.prepared=False
        self.mu.mem_map(0,0x1000);self.mu.mem_map(GROUND,0x20000)
        self.exports.extend([ROOT/'extracted/refined/originals/functions.tsv',ROOT/'extracted/refined/originals/Netstorm.c'])
        with self.exports[-2].open(encoding='utf-8') as stream:
            # 진입 주소로 정밀 범위를 색인해 실제 필요한 몸체만 선택한다.
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        entries=(0x4add10,0x44bfe0,0x41c5b0,0x4ac200,0x49a840,0x44c8e0,0x4adc60,
                 0x44c060,0x4ac550,0x49fce0,0x4acdf0,0x4acda0,0x4180a0,0x418070,0x43d9b0)
        self.entries.update(entries)
        # 검토한 함수들의 불연속 실행 구간만 허용한다.
        for entry in entries:
            # 하나의 함수에 속한 모든 정밀 범위를 보존한다.
            for segment in facts[entry]['ranges'].split(';'):
                # 범위 양끝의 16진수 주소만 정수로 변환한다.
                low,high=(int(value,16) for value in segment.split('-'));self.allowed.append((low,high+1))
        # burning의 명시 생성/삭제 경계가 사용하는 독립 Form90을 준비한다.
        self.mu.mem_write(CREATED+4,struct.pack('<I',90));self.mu.mem_write(self.slot(90),bytes(50));self.mu.mem_write(self.slot(90),struct.pack('<I',CREATED+0x100));self.mu.mem_write(CREATED+0x110,struct.pack('<I',DESTROY))
        assert self.read(0x50bad0+0x98)==0x44c8e0 and self.read(0x50bad0+0x9c)==0x43d9b0

    def on_instruction(self,mu,address,size,data):
        """현재131 가상 표와140 공격자 입력을 설치하고 실제 방어는 대체하지 않는다."""
        if not self.prepared and address in (self.h['base'],self.h['derived']):
            self.prepared=True
            # 기존 실행기 입력의 타입 자료를 현재131/140 번호로 옮기며 기대값은 계산하지 않는다.
            for previous,current in ((71,131),(128,140)):
                mu.mem_write(TYPES+current*self.type_stride,bytes(mu.mem_read(TYPES+previous*self.type_stride,self.type_stride)))
            mu.mem_write(self.slot(50),struct.pack('<I',0x50bad0));mu.mem_write(self.slot(50)+10,bytes([131]));mu.mem_write(self.slot(60)+10,bytes([140]))
            # 각 비휘발 레지스터에 서로 다른 초기값을 대응시킨다.
            self.saved={register:0x12340000+index for index,register in enumerate((UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP))}
            # 다른 초기 비휘발 값을 넣어 호출 보존을 실제 CPU에서 확인한다.
            for register,value in self.saved.items():mu.reg_write(register,value)
            mu.reg_write(UC_X86_REG_FPCW,self.control);mu.reg_write(UC_X86_REG_FPTAG,0xffff)
        if address in (0x4af780,0x4ad500):
            esp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(esp);destroy=address==0x4af780
            self.events.append('D:50:'+str(self.read(esp+4)) if destroy else 'U:50');self.boundaries[f'{address:08x}']+=1
            mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,esp+4+(4 if destroy else 0));mu.reg_write(UC_X86_REG_EIP,ret);return
        super().on_instruction(mu,address,size,data)

    def run_body(self,case):
        """전체 세 슬롯·사망/경고 전역과 반환/비휘발/x87/SEH를 함께 관찰한다."""
        self.prepared=False;self.mu.mem_write(0,bytes(4));output=super().run_body(case)
        # 모든 비휘발 레지스터가 실제 호출 전 값을 보존했는지 검사한다.
        assert all(self.mu.reg_read(register)==value for register,value in self.saved.items()),(case,'nonvolatile')
        assert self.mu.reg_read(UC_X86_REG_FPCW)==self.control and not(self.mu.reg_read(UC_X86_REG_FPSW)&0x3800) and self.read(0)==0,(case,'x87/SEH')
        # 같은 victim/attacker/ground 순서로 전체150BYTE를 기록한다.
        output[2]=b''.join(bytes(self.mu.mem_read(self.slot(sid),50)) for sid in (50,60,70)).hex()
        return output


def generate(smoke):
    """새 대표 입력을 두 정밀도로 실행하고 기존5846행과 구분하여 저장한다."""
    cases=inputs();cases=cases[::17] if smoke else cases;oracles=[Oracle(0x027f),Oracle(0x037f)];lines=[]
    # 모든 독립 입력의 실제 출력과 두 정밀도 일치를 확인한다.
    for index,case in enumerate(cases):
        try:
            # 실행기별 입력 사본을 사용해 외부 준비 상태를 분리한다.
            outputs=[oracle.run_body(dict(case)) for oracle in oracles]
        except Exception as error:raise RuntimeError(f'캠페인 피해 입력{index} {case}: {error}') from error
        if outputs[0]!=outputs[1]:raise RuntimeError('캠페인 피해 x87 정밀도 차이 '+str(index))
        # 입력 열은 KEYS 순서이며 기대값은 기본027f 실제 출력이다.
        lines.append('\t'.join([str(case[key]) for key in KEYS]+outputs[0]))
    if smoke:print('캠페인 피해 원본 smoke 통과',len(lines));return
    FIXTURE.write_text('# 실제10.78 TYPE131 일반 피해/0043d9b0 기본 방어. 삭제/공간/소유자/UI/벽시계/burning 생성은 명시 경계다.\n'+'\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=set(oracles[0].exports+[Path(__file__),FIXTURE,ROOT/'originals/Netstorm.exe',ROOT/'tools/decomp_damageablehit_oracle.py',ROOT/'tools/decomp_priestdamage_oracle.py',ROOT/'tools/decomp_prieststate_oracle.py',ROOT/'tools/decomp_owner_oracle.py'])
    report=dict(schema=1,host=socket.gethostname(),date='2026-10-16',last_full_refined_host='VM-W11-Codex',unique_inputs=len(cases),normal_returns=len(cases)*2,precision_controls=[0x027f,0x037f],precision_differences=0,
                # 검사 분류별 고유 입력 수를 각각 기록한다.
                groups={str(group):sum(case['group']==group for case in cases) for group in range(4)},
                metadata=dict(victimType=131,attackerType=140,vtable='0050bad0',hit='0044c8e0',defense='0043d9b0'),
                # 각 실제 CPU 실행기의 정상 반환/함수/경계/명령 집계를 보존한다.
                executions=[dict(control=oracle.control,returns=oracle.returns,assertions=oracle.assertions,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.boundaries),instructions=oracle.instructions) for oracle in oracles],
                limitations=['현재 전체 정밀 범위와 보호10.78 PE를 사용한다. 기존5846행/보고서 기대값은 변경하지 않는다.','실제TYPE131 가상 표와 기본false 방어·피격/레벨/HP/burning 판단을 실행한다. TYPE 필드·rank·HP·지면 입력은 선언한 합성 자료다.','서버 삭제/공간unrealize·소유자AI·벽시계/알림UI·burning 생성/삭제는 명시 경계이며 전체 실제 피해/죽음 완주로 세지 않는다.','전체150BYTE/사망·경고 전역16BYTE/효과 순서/반환·스택/비휘발/x87/SEH/쓰기guard를 대조한다.'],game_gui_runs=0,
                # 원문/도구/fixture의 실제 바이트를 각각 고정한다.
                sha256={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('캠페인 피해 원본 고유 입력',len(cases),'정상 반환',len(cases)*2)


def verify():
    """새 관찰의 현재 SHA/입력 고유성/정상 반환만 확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    # 고정한 모든 실제 근거 파일을 현재 바이트와 대조한다.
    for name,expected in report['sha256'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('캠페인 피해 SHA 불일치 '+name)
    # 설명 줄을 제외한 입력/관찰 행만 같은 열 구분으로 읽는다.
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 모든 입력 열의 조합으로 고유 관찰 수를 확인한다.
    assert len(rows)==report['unique_inputs'] and len({tuple(row[:len(KEYS)]) for row in rows})==len(rows)
    # 각 정밀도 실행의 정상 반환과 assert0을 확인한다.
    assert report['normal_returns']==len(rows)*2 and all(item['returns']==len(rows) and item['assertions']==0 for item in report['executions'])
    print('캠페인 피해 원본/SHA 검증 통과',len(rows))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');arguments=parser.parse_args()
    verify() if arguments.verify else generate(arguments.smoke)
