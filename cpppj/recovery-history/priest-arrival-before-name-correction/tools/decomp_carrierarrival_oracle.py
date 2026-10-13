#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Carrier 도착의 실제 조회/깊이/권한/끝 회피 검색을 별도 제한 x86으로 관찰한다."""
import argparse
import csv
import itertools
import json
from pathlib import Path
from decomp_priesthalt_oracle import HaltOracle,ROOT,POOL,HEAP,TYPES,HALT_SPOTS,FPU_CONTROLS,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,bits,sha

PATH=0x11870000 # lookup 경계가 돌려주는 별도 실제 레이아웃 경로 입력이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/carrierarrival-x86.tsv' # 이전Halt/BFS/Bridge 관찰을 다시 합산하지 않는 새 상위 입력이다.
REPORT=ROOT/'cpppj/recovery-carrierarrival-evidence.json' # 현재 원본/큰 외부 효과 경계와 반환/전체부모raw를 기록한다.
SPEC={
    'originals':dict(arrival=0x429800,path=0x427ce0,escape=0x429010,players=0x59534c,playerStride=0xb4,depth=0x186d8,helper=0x186d4,allow=0x5453e4,controlledOffset=0xb0),
    'originalCD':dict(arrival=0x4e66a0,path=0x4e7610,escape=0x4e5c10,players=0x50f834,playerStride=0xac,depth=0x2f18,helper=0x2f14,allow=0x541578,controlledOffset=0xa8,setting=0x4a9400),
} # 원본 CarrierArrival/현재Path/회피/Player AI 조회와 각 판본 레이아웃이다.
PLAYER_INPUT=0x11890000 # 실제 전역 Player 포인터가 읽는 별도0 초기화 입력 목록이다.

def inputs():
    """현재 도착 좌표에서 실제 호출되는 조회/깊이/목적지 방해 조건만 준비한다."""
    values=[]
    # 현재Path 없거나 깊이1인상태와 중립/권한/사제/일반운반 및 회피 반환을 교차한다.
    for scene,path,owner,boss,genus,reply in itertools.product(range(6),(0,1),(0,1),(0,1),(0x10000,0x210000),(1000,1001)):
        values.append(dict(scene=scene,path=path,owner=owner,boss=boss,genus=genus,reply=reply,helper=0,depth=1,controlled=0))
    # 사제 currentPath helper와 Player AI의 조기/끝 검색 생략 조건을 별도 입력으로 준비한다.
    for helper,controlled,depth in itertools.product((0,99),(0,1),(0,1,3)):
        values.append(dict(scene=0,path=1,owner=1,boss=1,genus=0x210000,reply=1001,helper=helper,depth=depth,controlled=controlled))
    unique={json.dumps(value,sort_keys=True,separators=(',',':')):value for value in values} # 같은 전체 입력은 최초 순서 한 번만 관찰한다.
    return list(unique.values())

class ArrivalOracle(HaltOracle):
    """기존 실제 finder/HP/contained/Graph getter와 현재 전체도착몸체를 함께 실행한다."""
    def __init__(self,edition):
        """새 actual 함수와 기존 재사용 함수의 정밀 범위만 더하며 큰 효과 경계를 명시한다."""
        super().__init__(edition)
        self.a=SPEC[edition];self.mu.mem_map(PATH,0x20000);self.mu.mem_map(PLAYER_INPUT,0x10000)
        # 실제 contained/지면/귀환장소/Graph 조회는 재사용하며 전체PE를 실행 허용하지 않는다.
        for name in ('carrierpredestroy','carriercheck','pathcleanup','playeranchor','carrierarrival-helper'):
            paths=[ROOT/f'extracted/{name}/{edition}/{leaf}' for leaf in ('creation.c','functions.tsv')]
            self.body_paths.extend(paths)
            with paths[1].open(encoding='utf-8') as stream:
                for row in csv.DictReader(stream,delimiter='\t'):
                    self.entries[int(row['entry'],16)]=row['entry']
                    for span in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in span.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\1'*(high+1-low)
        self.boundaries={self.a['path']:'L',self.a['escape']:'E'}
        if 'setting' in self.a:self.boundaries[self.a['setting']]='O'
        self.spec=dict(self.spec,entries=dict(Arrival=self.a['arrival']))

    def on_instruction(self,mu,address,size,data):
        """현재 Path 포인터/회피/이름있는설정만 명시 경계이며 나머지 조회는 actual body다."""
        boundary=getattr(self,'boundaries',{}).get(address)
        if not boundary:return super().on_instruction(mu,address,size,data)
        sp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(sp);this=mu.reg_read(UC_X86_REG_ECX);purge=0;result=0;self.stub_calls[boundary]+=1
        if boundary=='L':assert this==self.slot(50);result=PATH if self.case['path'] else 0
        elif boundary=='E':
            assert this==self.slot(50);purge=8;self.events.append(f'E:50:{self.read(sp+4)}:{self.read(sp+8)}:{self.case["reply"]}');result=self.case['reply']
        else:result=0
        mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run_arrival(self,value,control):
        """같은 입력의 actual 반환·모든명시효과·부모전체raw를 읽는다."""
        self.case=value;self.mu.mem_write(TYPES,bytes(256*self.type_stride));self.mu.mem_write(POOL,bytes(80*self.stride));self.mu.mem_write(HALT_SPOTS,bytes(65536));self.mu.mem_write(PATH,bytes(0x20000));self.mu.mem_write(PLAYER_INPUT,bytes(0x10000))
        objects=[(50,158,0,0,bits(20.75),bits(21.9),1,1,0,1)];scene=value['scene']
        # 원본 끝 검색의 서로 다른 조건을 각 하나의 후보로 준비하며 결과는 계산하지 않는다.
        if scene in (1,2,3,4):objects.append((60,84 if scene==2 else 150,0,0,bits(20.2),bits(23.2 if scene==4 else 21.3),1,1,0,1))
        elif scene==5:objects.append((60,165,0,0,bits(23.75),bits(24.9),3,3,0,1))
        self.setup(objects);raw=self.slot(50);self.put(raw+26,200,4 if self.stride==50 else 2);self.put(raw+(34 if self.stride==50 else 32),value['owner'],1);self.put(TYPES+158*self.type_stride,200);self.put(TYPES+158*self.type_stride+0xec,value['genus'])
        if scene in (1,2,3,4):self.put(TYPES+(84 if scene==2 else 150)*self.type_stride+0xec,(0,0x10000,0,0x4200,0x200)[scene])
        elif scene==5:self.put(TYPES+165*self.type_stride+0xec,0x400000)
        self.put(self.h['boss'],value['boss']);self.put(self.h['quarter'],0);self.put(self.a['allow'],0);self.put(self.a['players'],PLAYER_INPUT);self.put(PLAYER_INPUT+value['owner']*self.a['playerStride']+self.a['controlledOffset'],value['controlled']);self.put(PATH+self.a['depth'],value['depth']);self.put(PATH+self.a['helper'],value['helper'])
        self.put(HALT_SPOTS+22*256+21,6,1);self.put(HALT_SPOTS+21*256+20,6,1)
        result=self.call('Arrival',(0,0),raw,8,control);self.returns+=1
        return [str(result),';'.join(self.events) or '-',bytes(self.mu.mem_read(raw,self.stride)).hex()]

def main(arguments):
    """현재 원본 관찰을 별도생성하거나 당시 전체SHA를 검증한다."""
    if arguments.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,digest in report['files'].items():assert sha(ROOT/name)==digest,name
        print('Carrier 도착 현재SHA:',report['total']);return
    cases=inputs();cases=cases[::47] if arguments.smoke else cases;rows=[];editions={};files={Path(__file__),FIXTURE,ROOT/'tools/ghidra/carrierarrival-functions.json',ROOT/'tools/decomp_priesthalt_oracle.py'}
    for edition in SPEC:
        oracle=ArrivalOracle(edition)
        for index,value in enumerate(cases):
            result=oracle.run_arrival(value,FPU_CONTROLS[0]);second=oracle.run_arrival(value,FPU_CONTROLS[1]);assert result==second;rows.append([edition,json.dumps(value,sort_keys=True,separators=(',',':')),*result])
            if index%60==0:print(edition,index,len(cases),flush=True)
        editions[edition]=dict(returns=oracle.returns,normal_inputs=len(cases),native_calls=dict(oracle.native_calls),external_calls=dict(oracle.stub_calls));files.update(oracle.body_paths);files.add(ROOT/oracle.spec['binary'])
    FIXTURE.write_text('# edition inputJSON result effects rawParent\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files.update(ROOT/f'cpppj/src/o/RawCarrierArrival{suffix}' for suffix in ('.h','.cpp'))
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',decompile_date='2026-10-14',total=len(rows),smoke=arguments.smoke,game_runs=0,editions=editions,files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(files)},scope='실제 CarrierArrival의 조회/깊이/권한/끝회피검색. 현재 Path lookup/큰 회피 반환/설정은 명시 외부 입력. 포획/운반/SP/프로토콜/실제걷기·전체Arrival 하위효과/캠페인은 후속. 기존Halt864/BFS/Bridge 관찰 재합산0.'),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')

# 명시 호출에서만 실제한정x86을 실행하고 원본게임/GUI를 실행하지 않는다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');main(parser.parse_args())
