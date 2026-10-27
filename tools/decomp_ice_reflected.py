#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제10.78 반사 Missile 생성/비행/사거리/피해 조건과 효과 순서를 격리 관측한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_ice_missile_initial import Oracle as IceOracle, KEYS as INITIAL_KEYS
from decomp_thunder_missile import Oracle as BaseOracle, sample
from decomp_gunmissile_oracle import OBJECT, VTABLE
from decomp_owner_oracle import ROOT, TYPES, STOP, digest
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ECX

KEYS=(*INITIAL_KEYS,'sourceFree','targetZero','damage','alternate','alternateDamage','targetFlags2','missed','freshBox','durationZero','displayOwner')  # 원문에 직접 공급하는 독립 상태/타입 입력이다.
DAMAGE=STOP+0x160  # 표적의 실제 가상98 호출 인자를 관측하는 경계다.
FIXTURE=ROOT/'cpppj/tests/fixtures/ice-reflected-x86.tsv'  # 기존 원문 기대값을 덮어쓰지 않는다.
REPORT=ROOT/'cpppj/ice-reflected-native-20261027.json'  # 실제 전체 몸체/외부 경계·정밀도·해시 기록이다.

def bits(value):
    """입력 float의 비트만 옮기며 기대 결과를 계산하지 않는다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]

def item(**changes):
    """기존 독립 입력 형식에 실제 반사 수명 인자를 추가한다."""
    row=sample(range=bits(5),displaySound=0,soundRoll=1000,sourceFree=0,targetZero=0,damage=bits(11.25),alternate=0,alternateDamage=bits(7.75),targetFlags2=0,missed=0,freshBox=0,durationZero=0,displayOwner=3)
    row.update(changes)
    return row

def inputs():
    """명시 반사 포구/코너 무시·target0·죽음/만료·현재 소유자/피해 분기를 교차한다."""
    rows=[]
    # 생성자는 실제 반사1/명시 포구를 사용하고 SHP 보정과 사정선을 생략한다.
    for point,end,zero,corner,hot,sound in itertools.product(((50,50),(80.25,64.75),(1.25,1.25)),((50,90),(100,100),(2,2)),(0,1),(0,1),((0,0),(16,11)),(0,123)):
        rows.append(item(x=bits(point[0]),y=bits(point[1]),endx=bits(end[0]),endy=bits(end[1]),targetZero=zero,corner=corner,hotx=hot[0],hoty=hot[1],firingSound=sound))
    # 독립 frame 이미지는 생성자 출력이 아니며 새 상자/기존 상자와 dead/free를 구분한다.
    for fraction,state,free,zero,previous,box,distance in itertools.product((0,1,3,4,5,8,12),range(4),(0,1),(0,1),(0,1),(0,1),(5,40)):
        rows.append(item(kind=1,endx=bits(50),endy=bits(90),fraction=fraction,targetstate=state,sourceFree=free,targetZero=zero,previous=previous,freshBox=box,range=bits(distance)))
    # 실제0.6F 피해 계수와 별도 DPS 교체의 float 저장/절삭 경계다.
    for free,state,damage,alternate,flags in itertools.product((0,1),range(4),(-3.75,0,1.6666666,11.25,100.25),(0,1),(0,0x20000,0x100000)):
        rows.append(item(kind=1,endx=bits(50),endy=bits(90),fraction=4,targetstate=state,sourceFree=free,previous=1,freshBox=1,damage=bits(damage),alternate=alternate,targetFlags2=flags,impactSound=456))
    # 빗나감 표시와 invalid/0기간 이동의 공통 종료 음향도 실제 몸체로 관찰한다.
    for missed,fraction,zero,duration in itertools.product((0,1,2),(0,4,12),(0,1),(0,1)):
        rows.append(item(kind=1,endx=bits(50),endy=bits(90),fraction=fraction,targetZero=zero,missed=missed,durationZero=duration,impactSound=456))
    return list({tuple(row[key] for key in KEYS):row for row in rows}.values())

class Oracle(IceOracle):
    """기존 실제 공통 몸체를 실제130/반사1 입력으로 실행하고 가상 피해만 추가 관찰한다."""
    def __init__(self,control):
        """현재 타입 flags2 helper의 정확한 정밀 범위도 허용한다."""
        super().__init__(control)
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        self.entries.add(0x4ac200)
        for segment in facts[0x4ac200]['ranges'].split(';'):  # 실제 HP 대체 분기의 flags2 getter만 추가한다.
            low,high=(int(value,16) for value in segment.split('-'));self.allowed.append((low,high+1))

    def run_body(self,case):
        """선행 초기129 치환을 사용하지 않고 첫 실제 명령 직전에 반사 입력만 설치한다."""
        self.reflectedPrimed=False
        self.primed=True
        return BaseOracle.run_body(self,case)

    def on_instruction(self,mu,address,size,data):
        """반사 초기 입력·외부 가상98/재반사만 관찰하고 전체 수명 계산은 실행한다."""
        if not self.reflectedPrimed:
            self.reflectedPrimed=True;c=self.case
            raw=bytearray(mu.mem_read(TYPES+131*500,500));struct.pack_into('<I',raw,0x9c,10);struct.pack_into('<I',raw,0x48,c['damage']);struct.pack_into('<II',raw,0x1b0,c['alternate'],c['alternateDamage'])
            display=bytes(mu.mem_read(TYPES+132*500,500));raw[0x74:0x78]=display[0x74:0x78];raw[0x110:0x118]=display[0x110:0x118];mu.mem_write(TYPES+130*500,bytes(raw))
            mu.mem_write(self.slot(50)+10,bytes([130,c['sourceFree']]));mu.mem_write(self.slot(90)+10,bytes([130]));mu.mem_write(self.slot(90)+34,bytes([c['displayOwner']]))
            mu.mem_write(TYPES+129*500,bytes(raw));mu.mem_write(TYPES+71*500+0xec,struct.pack('<I',c['targetFlags2']));mu.mem_write(0x54125c,struct.pack('<I',129))
            mu.mem_write(VTABLE+0x98,struct.pack('<I',DAMAGE));mu.mem_write(self.slot(0),bytes(50))
            if c['kind']==0:
                esp=mu.reg_read(UC_X86_REG_ESP)
                for offset,value in ((8,0 if c['targetZero'] else 60),(16,130),(20,0),(40,1),(44,c['x']),(48,c['y'])):mu.mem_write(esp+offset,struct.pack('<I',value))  # 12개 실제 ctor 인자 중 반사 관련 입력만 바꾼다.
            else:
                mu.mem_write(OBJECT+12,struct.pack('<I',1));mu.mem_write(OBJECT+24,struct.pack('<I',0 if c['targetZero'] else 60));mu.mem_write(OBJECT+40,struct.pack('<II',int(c['missed']==1),int(c['missed']==2)))
                if c['freshBox']:mu.mem_write(OBJECT+88,struct.pack('<4f',49,89,51,91))
                if c['durationZero']:mu.mem_write(OBJECT+104,struct.pack('<d',10))
            self.write_ranges.append((self.slot(0),self.slot(0)+50))
        if address==DAMAGE:
            esp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX);self.events.append('D:'+str((this-self.slot(0))//50)+':'+':'.join(str(self.read(esp+4+i*4)) for i in range(3)))
            mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EIP,self.read(esp));mu.reg_write(UC_X86_REG_ESP,esp+16);return
        if address==0x482190:
            esp=mu.reg_read(UC_X86_REG_ESP);self.events.append('R:'+str(self.read(OBJECT+8))+':'+str(self.read(OBJECT+24)));mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EIP,self.read(esp));mu.reg_write(UC_X86_REG_ESP,esp+4);return
        super().on_instruction(mu,address,size,data)

def main():
    """두 정밀도의 원문 반환만 새 fixture로 보존하거나 현재 해시를 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        for path,expected in report['sha256'].items():assert digest(ROOT/path)==expected,path  # 기대값은 재계산하지 않는다.
        assert sum(bool(line) and not line.startswith('#') for line in FIXTURE.read_text('utf-8').splitlines())==report['uniqueInputs'];print('반사탄 원문 SHA 확인',report['uniqueInputs']);return
    selected=inputs()
    if args.smoke:selected=[selected[0],next(row for row in selected if row['kind']==1),next(row for row in selected if row['kind']==1 and row['durationZero']),next(row for row in selected if row['kind']==1 and row['sourceFree'] and row['targetstate']==0 and row['previous'] and row['impactSound'])]
    oracles=[Oracle(0x27f),Oracle(0x37f)];rows=[];differences=[]
    for index,case in enumerate(selected):  # 두 정밀도에서 같은 원본 몸체를 독립 실행한다.
        try:output=[oracle.run_body(dict(case)) for oracle in oracles]
        except Exception as error:raise RuntimeError(f'반사탄 입력{index} {case}: {error}') from error
        if output[0]!=output[1]:differences.append(dict(index=index,secondary=output[1]))
        rows.append('\t'.join([str(case[key]) for key in KEYS]+output[0]))
    if args.smoke:print('반사탄 smoke',len(rows),rows);return
    with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:stream.write('# 보호10.78 반사1/130 전체 ctor/frame. 일반 상자/공간/사운드/가상98/재반사는 명시 경계다.\n'+'\n'.join(rows)+'\n')
    paths=('originals/Netstorm.exe','originals/netstorm.tarc','extracted/refined/originals/functions.tsv','tools/decomp_ice_reflected.py','tools/decomp_ice_missile_initial.py','tools/decomp_thunder_missile.py','tools/decomp_crossbow_missile.py','tools/decomp_gunmissile_oracle.py','tools/decomp_owner_oracle.py',FIXTURE.relative_to(ROOT).as_posix())
    report=dict(host=socket.gethostname(),lastFullDecompilation='HJOW-Athlon / 2026-10-23',uniqueInputs=len(rows),normalReturns=len(rows)*2,precisionDifferences=differences,inputKinds=dict(Counter(str(row['kind']) for row in selected)),nativeCalls=dict(oracles[0].native_calls),boundaries=dict(oracles[0].boundaries),damageScaleBits=bytes(oracles[0].mu.mem_read(0x50d650,4)).hex(),limits='실제 반사 ctor/frame/타입·방향·치수·중심·flags2·CRT를 실행한다. 일반 충돌 상자/표시·공간·소리·가상98 피해·재반사00482190은 명시 경계다. 실제 세션/완주 증거가 아니다.',sha256={path:digest(ROOT/path) for path in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('반사탄 고유입력',len(rows),'정상반환',len(rows)*2,'정밀도차이',len(differences))

if __name__=='__main__':main()
