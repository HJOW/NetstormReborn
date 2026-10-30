#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78 반사 Generate의 미초기화 좌표 입력과 이후 실제 선택을 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_ice_ricochet import Oracle as BaseOracle, KEYS as BASE_KEYS, sample, node, bits
from decomp_owner_oracle import ROOT, STACK, digest
from unicorn.x86_const import UC_X86_REG_ESP

KEYS = (*BASE_KEYS, 'scratchX', 'scratchY')  # 출력과 무관하게 정한 호출 전 좌표8BYTE 입력이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/ice-ricochet-scratch-x86.tsv'  # 기존 반사 관측을 변경하지 않는 새 최초 자료다.


def cases():
    """변위0의 확률/좌표 유효성/후보·현재 필드와 비영 변위의 덮어쓰기를 분리한다."""
    result = []
    points = ((bits(60),bits(50)),(bits(1),bits(255)),(bits(0),bits(50)),(0x80000000,bits(50)),(bits(-1),bits(50)),(bits(256),bits(50)),(bits(60),bits(256)),(0x7fc12345,bits(50)),(bits(50),0xffc12345),(0x7f800000,bits(50)),(bits(60.25),bits(50.75)))
    for seed,point,nodes in itertools.product((0,1,19,20,98,0xffffffff),points,('-',node(15000),node(15000)+';'+node(15001,x=60.5))):  # 확률 미선택에서도 최초 입력을 보존한다.
        result.append(sample(kind=1,seed=seed,ox=bits(60),oy=bits(50),scratchX=point[0],scratchY=point[1],nodes=nodes))
    for mutation in range(1,5):  # Finder의 다음 후보 뒤 실제 재조회하는 값만 바꾼다.
        result.append(sample(kind=1,seed=20,ox=bits(60),oy=bits(50),scratchX=bits(60),scratchY=bits(50),nodes=node(15000)+';'+node(15001,x=61),mutation=mutation))
    for point in points:  # 변위가 있으면 최초8BYTE가 Point의 실제 출력으로 바뀐다.
        result.append(sample(kind=1,seed=20,scratchX=point[0],scratchY=point[1],nodes=node(15000)))
    return list({tuple(value[key] for key in KEYS):value for value in result}.values())


class Oracle(BaseOracle):
    """호출 전 스택만 입력하고 Point/Valid/Finder 선택/생성 제어는 원문 명령으로 실행한다."""
    def run_body(self,value):
        """새 호출 이미지를 두 정밀도에서 독립 관측한다."""
        self.primed = False  # 이번 호출의 첫 명령 전 입력 설치 여부다.
        self.point_calls = 0  # 실제 Point 진입 횟수이며 C++ 기대값에서 계산하지 않는다.
        self.preserved = None  # Point 반환 후 최초8BYTE와 같은지 관측한 결과다.
        result = super().run_body(value)
        return [*result,bytes(self.mu.mem_read(STACK-0x5c,8)).hex(),str(self.point_calls),str(int(bool(self.preserved)))]

    def on_instruction(self,mu,address,size,data):
        """첫 명령 전에8BYTE를 두고 Point의 미기록을 부작용 없이 관측한다."""
        if not self.primed:
            assert address==0x482190 and mu.reg_read(UC_X86_REG_ESP)==STACK
            self.primed = True
            self.initial = struct.pack('<II',self.case['scratchX'],self.case['scratchY'])  # 원문에 전달할 독립 최초8BYTE다.
            mu.mem_write(STACK-0x5c,self.initial)
        if address==0x482080:
            assert self.read(mu.reg_read(UC_X86_REG_ESP)+4)==STACK-0x5c
            self.point_calls += 1
        if address==0x4821da:
            self.preserved = bytes(mu.mem_read(STACK-0x5c,8))==self.initial
        super().on_instruction(mu,address,size,data)


def main():
    """독립 두 정밀도의 최초 출력을 보존하고 명시 지정에서만 새 기대값을 만든다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',required=True);parser.add_argument('--fixture',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    output=ROOT/args.output;output.mkdir(parents=True,exist_ok=False)
    values=cases();first,replay=Oracle(0x27f),Oracle(0x37f)
    observed=[[str(value[key]) for key in KEYS]+first.run_body(value) for value in values]
    repeated=[[str(value[key]) for key in KEYS]+replay.run_body(value) for value in values]
    for name,result in (('first.json',observed),('replay.json',repeated)):  # 각 관측을 기존 파일에 덮어쓰지 않는다.
        (output/name).write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
    assert observed==repeated
    payload='# '+'/'.join(KEYS)+'/원문 명령 출력7열\n'+''.join('\t'.join(row)+'\n' for row in observed)
    if args.verify: assert FIXTURE.read_bytes()==payload.encode('utf-8')
    elif args.fixture:
        with FIXTURE.open('xb') as stream: stream.write(payload.encode('utf-8'))
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','extracted/refined/originals/Netstorm.c','tools/decomp_ice_ricochet.py','tools/decomp_owner_oracle.py','tools/decomp_ice_ricochet_scratch.py']
    if args.fixture or args.verify: paths.append(FIXTURE.relative_to(ROOT).as_posix())
    report={'unit':612,'host':socket.gethostname(),'lastFullDecompilation':'HJOW-Athlon / 2026-10-23','cases':len(values),'observations':2*len(values),'differences':[],'fixtureWritten':args.fixture,'verificationOnly':args.verify,'sourceSha256':{name:digest(ROOT/name) for name in paths},'scope':'00482190 입구ESP-0x5c의8BYTE만 호출 전 명시 입력으로 두고 실제Point/Valid/난수/후보 선택/생성 인자를 실행한다. 일반Finder 후보와 할당/후속ctor는 기존 경계다. 게임/GUI/특정적공격/포획/희생/승리/신규MP 없음.'}
    destination=ROOT/'cpppj/ice-ricochet-scratch-native-20261030.json' if args.fixture or args.verify else output/'report.json'
    destination.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({'cases':len(values),'observations':2*len(values),'differences':[]}))


if __name__=='__main__':
    main()
