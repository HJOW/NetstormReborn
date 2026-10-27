#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78의 아이스 초기 투사체 생성·비행·포구·사정선과 반사 호출 위치를 관찰한다."""
import argparse
import json
import socket
import struct
from pathlib import Path
from decomp_thunder_missile import Oracle as BaseOracle,cases,KEYS as BASE_KEYS,TYPES
from decomp_owner_oracle import ROOT,digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP

KEYS=(*BASE_KEYS,'soundRoll')  # 기존 독립 입력 뒤 전용 효과음 정수 난수 입력을 추가한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/ice-missile-initial-x86.tsv'  # 기존 기대값을 덮어쓰지 않는다.
REPORT=ROOT/'cpppj/ice-missile-initial-native-20261027.json'  # 실제 몸체/외부 경계와 근거 해시다.

def inputs():
    """기존 공통 수치 입력과1..4 전용 효과음 경계를 원문 입력으로 방문한다."""
    result=[dict(case,soundRoll=1000) for case in cases()]
    sounded=[case for case in result if case['kind']==1 and case['impactSound'] and not case['corner']]
    result += [dict(case,soundRoll=roll) for case in sounded for roll in (1999,2000,3000,4000,4999)]
    return result

class Oracle(BaseOracle):
    """같은 원문 일반 몸체에 실제129/130 입력과 전용 사운드/반사 하위 경계를 공급한다."""
    def __init__(self,control):
        """실제 아이스 가상 포구가 원시 좌표 getter인지 보호PE에서 확인한다."""
        super().__init__(control)
        assert self.read(0x50b9b8+0xb4)==0x41c110

    def run_body(self,case):
        """입력 준비와 첫 실제 명령을 구별하고 전체128BYTE/세 슬롯/효과를 반환한다."""
        self.primed=False
        return super().run_body(case)

    def name(self,address):
        """원본이 전달한 NUL 문자열만 읽으며 출력 이름을 추정하지 않는다."""
        data=bytearray()
        for offset in range(256):  # 원본 지역 문자열의 실제 NUL까지 읽는다.
            value=self.read(address+offset,1)
            if not value:return data.decode('ascii')
            data.append(value)
        raise RuntimeError('원문 효과음 NUL 범위')

    def on_instruction(self,mu,address,size,data):
        """첫 명령 전 타입 입력과 선언한 외부 사운드/난수/반사 하위만 공급한다."""
        if not self.primed:
            self.primed=True
            for source,destination in ((131,129),(132,130)):  # 수치 보충 입력을 실제 아이스 번호에 설치한다.
                mu.mem_write(TYPES+destination*self.type_stride,bytes(mu.mem_read(TYPES+source*self.type_stride,self.type_stride)))
            mu.mem_write(self.slot(50)+10,bytes([129]));mu.mem_write(self.slot(90)+10,bytes([130]))
            if self.case['kind']==0:mu.mem_write(mu.reg_read(UC_X86_REG_ESP)+16,struct.pack('<I',130))
        if address in (0x482190,0x4558f0,0x4a89c0,0x4a9d70):
            esp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(esp);advance=4;value=0
            if address==0x482190:
                self.events.append('R:90:60')  # 실제 호출 위치만 관찰한다. 반사 생성 몸체를 완료로 계산하지 않는다.
            elif address==0x4558f0:
                low,high=self.read(esp+4),self.read(esp+8);assert (low,high)==(1000,5000)
                self.events.append('N:1000:5000');value=self.case['soundRoll'];assert low<=value<high
            elif address==0x4a89c0:
                destination=self.read(esp+4);assert self.name(self.read(esp+8))=='iceCannonImpact%d.wav'
                mu.mem_write(destination,('iceCannonImpact'+str(self.read(esp+12))+'.wav\0').encode('ascii'));value=destination
            else:
                self.events.append('F:'+str(self.read(esp+4))+':'+str(self.read(esp+8))+':'+self.name(self.read(esp+12)));assert self.read(esp+16)==0
            mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+advance);mu.reg_write(UC_X86_REG_EIP,ret);return
        super().on_instruction(mu,address,size,data)

def main():
    """두 정밀도의 실제 출력을 새 자료로 보존하거나 저장된 SHA만 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        for path,expected in report['sha256'].items():assert digest(ROOT/path)==expected,path  # 원문 기대값은 다시 계산하지 않는다.
        rows=[line for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==report['uniqueInputs'];print('아이스 초기 투사체 SHA',len(rows));return
    selected=inputs();selected=[next(case for case in selected if case['kind']==kind) for kind in range(4)] if args.smoke else selected
    assert len({tuple(case[key] for key in KEYS) for case in selected})==len(selected)
    oracles=[Oracle(0x027f),Oracle(0x037f)];rows=[];differences=[]
    for index,case in enumerate(selected):  # 기대값은 실제 보호PE에서만 얻는다.
        try:output=[oracle.run_body(dict(case)) for oracle in oracles]
        except Exception as error:raise RuntimeError(f'아이스 초기 입력{index} {case}: {error}') from error
        if output[0]!=output[1]:differences.append(dict(index=index,secondary=output[1]))
        rows.append('\t'.join([str(case[key]) for key in KEYS]+output[0]))
    if args.smoke:print('아이스 초기 투사체 smoke',len(rows));return
    with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:stream.write('# 보호10.78 초기 아이스 투사체. 반사00482190/월드 가상 효과/사운드 장치는 명시 외부 경계다.\n'+'\n'.join(rows)+'\n')
    paths=('originals/Netstorm.exe','originals/netstorm.tarc','extracted/refined/originals/functions.tsv','tools/decomp_ice_missile_initial.py','tools/decomp_thunder_missile.py','tools/decomp_crossbow_missile.py','tools/decomp_gunmissile_oracle.py','tools/decomp_owner_oracle.py',FIXTURE.relative_to(ROOT).as_posix())
    report=dict(host=socket.gethostname(),lastFullDecompilation='HJOW-Athlon / 2026-10-23',uniqueInputs=len(rows),normalReturns=len(rows)*2,precisionDifferences=differences,limits='실제 초기129→130 ctor/frame/포구/사정선/방향/타입·치수·중심과 충돌 순서를 실행한다. 난수/format/파일명 음향/반사00482190/외부 자산·공간·피해는 명시 입력이다. 반사 생성·반사탄 ctor/frame·냉각 표시·전체 게임 완주 증거가 아니다.',sha256={path:digest(ROOT/path) for path in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('아이스 초기 입력',len(rows),'두 정밀도 반환',len(rows)*2,'차이',len(differences))

if __name__=='__main__':main()
