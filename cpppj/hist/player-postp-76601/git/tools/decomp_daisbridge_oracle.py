#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다이스 자격을 충족한 Bridge 명령의 실제 PathStart/BFS/끝점/Prepare를 별도 제한 x86에서 관찰한다."""
import argparse
import csv
import itertools
import json
from pathlib import Path
from decomp_pathstart_oracle import StartOracle,ROOT,POOL,HEAP,MAP,VTABLE,PATH,OLD,DISTANCE,QUEUE,ATTACH,bits,sha
from decomp_bridgedecay_oracle import TYPES,CODES,SPOTS,FPU_CONTROLS,UC_X86_REG_EAX,UC_X86_REG_ECX
from decomp_pathruntime_oracle import dbits
from decomp_templeritual_oracle import SPEC as TEMPLE

SHP=0x11800000 # Path 배열과 겹치지 않는 합성 SHP 프레임/핫스폿 입력이다.
BRIDGE_SPOTS=SHP+0x20000 # hash1과 겹치지 않는 실제 지면 입력의 독립 주소다.
FIXTURE=ROOT/'cpppj/tests/fixtures/daisbridge-x86.tsv' # 실제 상위 명령과 Path 전체 결과의 새 입력 자료다.
REPORT=ROOT/'cpppj/daisbridge.json' # 기존 관찰 수와 합산하지 않는 별도 근거다.
SPEC={'originals':dict(bridge=0x449900,altar=0x5412e8,contained=0x541088,alliances=0x595200,depth=0x186d8),
      'originalCD':dict(bridge=0x481ab0,altar=0x51cbd4,contained=0x51c978,alliances=0x50f6e0,depth=0x2f18)} # 상위 명령/현재 전역과 기존 깊이 필드다.

def inputs():
    """정상 stage0..3의 다음/현재 목표와 기존 경로 깊이/권한/실패 입력을 교차한다."""
    return [dict(stage=stage,current=current,legacy=legacy,depth=depth,explicit=explicit,boss=boss,blocked=blocked)
            for stage,current,legacy,depth,explicit,boss,blocked in itertools.product(range(4),(0,1),(0,1),range(4),(0,1),(0,1),(0,1))
            if legacy or depth==0]

class BridgeOracle(StartOracle):
    """이전 Path의 실제 몸체와 현재 Dais/Temple 몸체를 같은 격리 메모리에 연결한다."""
    def __init__(self,edition):
        """현재 함수 범위만 실행 허용하며 SHP는 Path와 별도 주소에 준비한다."""
        super().__init__(edition)
        self.bridge=SPEC[edition]
        self.temple=TEMPLE[edition]
        self.bridge_returns=0
        self.mu.mem_map(SHP,0x30000)
        # 이전 함수 전체 관찰은 재합산하지 않으며 상위 명령에 실제 필요한 현재 내보내기만 추가한다.
        for name in ('daisregular','templeritual','pathcleanup','runepostpop'):
            manifest=ROOT/f'tools/ghidra/{name}-functions.json'
            table=ROOT/f'extracted/{name}/{edition}/functions.tsv'
            self.body_paths.extend((manifest,table,table.with_name('creation.c')))
            selected={int(entry,16) for entry in json.loads(manifest.read_text(encoding='utf-8'))['editions'][edition]}
            with table.open(encoding='utf-8') as stream:
                # 불연속 함수 몸체 외 PE 명령은 기존 실행 제한기가 거부한다.
                for row in csv.DictReader(stream,delimiter='\t'):
                    if int(row['entry'],16) not in selected:continue
                    self.entries[int(row['entry'],16)]=row['entry']
                    for span in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in span.split('-'))
                        self.allowed[low-self.base:high+1-self.base]=b'\1'*(high+1-low)

    def table(self,typ,pattern):
        """프레임 방향표와 첫번호/길이 및 hotspot을 합성 입력으로 준비한다."""
        table=TYPES+typ*self.type_stride
        codes=CODES+(typ-158)*0x1000
        shape=SHP+(typ-158)*0x3000
        self.put(table,200)
        self.put(table+0xe8,16)
        self.put(table+0xdc,shape)
        self.put(table+0x114,len(pattern))
        self.put(table+0x124,codes)
        # SHP byte offset와 signed short 핫스폿은 실제 원본 좌표 helper가 읽는다.
        for index,letter in enumerate(pattern):
            self.put(codes+index*4,ord(letter)|(ord('P')<<8)|((index+1)<<16))
            self.put(shape+8+index*8,0x800+index*12)
            self.put(shape+0x800+index*12-4,(index%5)*16,2)
            self.put(shape+0x800+index*12-2,(index%5)*11,2)
        # 모든 방향의 입력 첫번호/길이는 기존 RiftTypeFrames와 같은 표다.
        for index,letter in enumerate('ABCDEFGHIJKLMNOP'):
            self.put(table+0x130+index*4,pattern.find(letter))
            self.put(table+0x170+index*4,pattern.count(letter))

    def on_instruction(self,mu,address,size,data):
        """실제 상위 호출 뒤 Prepare가 읽는 명시 Host 부착 경계의 부모 링크를 공급한다."""
        attached=address==ATTACH
        old_killed=address==self.r['kill'] and mu.reg_read(UC_X86_REG_ECX)==OLD
        super().on_instruction(mu,address,size,data)
        if attached:self.put(self.slot(5)+6,777,2)
        if old_killed:self.put(self.slot(5)+6,0,2)

    def run_bridge(self,value,control):
        """같은 입력에서 실제 명령 반환/끝점/시간/깊이/raw와 명시 효과 요청을 읽는다."""
        previous_calls=self.native_calls.copy()
        previous_stubs=self.stub_calls.copy()
        # 기존 Start 호출은 지면/시계/작업 배열 준비에만 사용하며 관찰 수를 되돌린다.
        self.put(self.r['spots'],SPOTS)
        StartOracle.run(self,(158,0,1,0,1,0),control)
        self.native_calls=previous_calls
        self.stub_calls=previous_stubs
        self.mu.mem_write(PATH,bytes(0x80000))
        self.mu.mem_write(POOL,bytes(300*self.stride))
        objects=[(5,158,0,0,bits(9.2),bits(9.3),1,1,0,1),
                 (60,165,0,0,bits(11.75),bits(11.25),3,3,0,1),
                 (61,164,0,0,bits(11.75),bits(11.25),1,1,0,1)]
        next_sid=100
        # 매 셀은 고유 표면 SID/Graph3/spot6이며 원본 BFS가 길을 결정한다.
        for y in range(9,20):
            for x in range(9,20):
                objects.append((next_sid,83,0,0,bits(x),bits(y),1,1,0,0))
                next_sid+=1
        self.setup(objects)
        for sid in range(100,next_sid):self.put(self.slot(sid)+self.o['graph'],3,1)
        self.put(self.r['spots'],BRIDGE_SPOTS)
        spots=BRIDGE_SPOTS
        self.mu.mem_write(spots,bytes(65536))
        # 실제 spot 입력도 해시 표면과 같은 셀 범위에만 기록한다.
        for y in range(9,20):
            for x in range(9,20):self.put(spots+y*256+x,6,1)
        self.table(165,'A'*180)
        self.table(166,'A'*6)
        self.table(158,'AEKLMNOP'+'P'*24)
        self.table(164,'A'*3+'O'*4+'M'*3+'L'*3+'P'*3)
        self.put(TYPES+158*self.type_stride+0xec,0x210000)
        for sid in (5,60,61):self.put(self.slot(sid)+(34 if self.patch else 32),1,1)
        self.put(self.slot(5)+26,200,4 if self.patch else 2)
        self.put(self.slot(60)+self.o['frame'],3+11*value['stage'],self.o['frame_size'])
        self.put(self.slot(60)+6,63,2)
        self.put(self.slot(63),VTABLE)
        self.put(self.slot(63)+10,6,1)
        self.put(self.slot(63)+12,2,2)
        self.put(self.slot(63)+18,158,2)
        self.put(self.slot(63)+20,32,2)
        self.put(VTABLE+0x80,self.temple['count'])
        self.put(self.bridge['altar'],164)
        self.put(self.bridge['contained'],6)
        self.mu.mem_write(self.bridge['alliances'],bytes(81*4))
        self.put(self.temple['scaleX'],bits(16))
        self.put(self.temple['scaleY'],bits(11))
        self.put(self.r['boss'],value['boss'])
        self.legacy=bool(value['legacy'])
        self.parent_type=158
        self.pending={604:0,605:0}
        self.event_objects={}
        self.put(OLD+self.bridge['depth'],value['depth'])
        if self.legacy:self.put(self.slot(5)+6,77,2)
        if value['blocked']:
            # 표면 없는 경로 실패 입력이다. 원본의 clearBlocked 분기에 맞는 blocking genus는 넣지 않는다.
            self.mu.mem_write(MAP,bytes(self.bases[1]-MAP))
        self.spec=dict(self.spec,entries=dict(self.spec['entries'],Bridge=self.bridge['bridge']))
        self.events=[]
        self.call('Bridge',[5 if value['explicit'] else 0,value['current']],self.slot(60),8,control)
        self.bridge_returns+=1
        count=self.read(PATH+0x10)
        points=[]
        # 모든 꼭짓점/방향/시간은 원본이 실제 쓴 비트이며 C++ 기대 경로를 계산하지 않는다.
        for index in range(count):
            points.append(','.join(map(str,(self.read(PATH+0x14+index*8),self.read(PATH+0x18+index*8),self.read(PATH+self.r['directions']+index*4),self.read(PATH+self.r['times']+index*8,8)))))
        command=[self.read(PATH+offset) for offset in ((0x186cc,0x186d0,0x186d4,0x186e4,0x186e8,0x186d8) if self.patch else (0x2f0c,0x2f10,0x2f14,0x2f1c,0x2f20,0x2f18))]
        return [';'.join(self.events) or '-',str(count),','.join(map(str,command)),';'.join(points) or '-',
                ';'.join(bytes(self.mu.mem_read(self.slot(sid),self.stride)).hex() for sid in (5,60,61,63))]

def generate(smoke):
    """새 상위 입력만 두 x87에서 관찰하며 기존 초기화 호출/fixture 입력은 합산하지 않는다."""
    cases=inputs()
    if smoke:cases=cases[::47]
    rows=[]
    editions={}
    differences=[] # 기존 Path와 같은 두 x87 정밀도의 double 시각 차이를 원문 결과로 보존한다.
    files={Path(__file__),FIXTURE}
    # 각 판본은 독립 에뮬레이터이며 원본/복사본 게임과 GUI를 실행하지 않는다.
    for edition in SPEC:
        oracle=BridgeOracle(edition)
        for index,case in enumerate(cases):
            try:
                result=oracle.run_bridge(case,FPU_CONTROLS[0])
                again=oracle.run_bridge(case,FPU_CONTROLS[1])
                assert result[:3]==again[:3] and result[4:]==again[4:]
                if again!=result:
                    first=[point.split(',') for point in result[3].split(';')]
                    second=[point.split(',') for point in again[3].split(';')]
                    assert len(first)==len(second) and all(left[:3]==right[:3] for left,right in zip(first,second))
                    differences.append(dict(edition=edition,input=case,result53=result,result64=again))
            except Exception as error:raise RuntimeError(f'{edition} #{index} {case}: {error}') from error
            rows.append([edition,json.dumps(case,sort_keys=True,separators=(',',':')),*result])
        editions[edition]=dict(returns=oracle.bridge_returns,entries=dict(oracle.native_calls),boundaries=dict(oracle.stub_calls),asserts=oracle.assertions)
        files.update(oracle.body_paths)
        files.add(ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'))
    FIXTURE.write_text('# edition inputJSON effects count commands points raw5,60,61,63\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    # 새 상위 명령에 실제 재사용한 생산 몸체와 Python 입력 준비의 전체 출처를 고정한다.
    for name in ('RawDaisRegular','RawPathStart','RawPathRuntime','RawPathProcess','RawPathSearch','RawTempleRitual','RawRunePostPop','RawContainedLookup'):
        for suffix in ('.h','.cpp'):files.add(ROOT/f'cpppj/src/o/{name}{suffix}')
    files.update(ROOT/name for name in ('cpppj/src/client/RawPriestWorldBindings.h','cpppj/src/client/RawPriestWorldBindings.cpp','tools/decomp_pathstart_oracle.py','tools/decomp_templeritual_oracle.py','tools/decomp_rawfinder_oracle.py','tools/decomp_bridgeeffects_oracle.py','tools/decomp_bridgedecay_oracle.py','tools/decomp_pathruntime_oracle.py','tools/decomp_pathsearch_oracle.py'))
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',decompile_date='2026-10-14',total=len(rows),smoke=smoke,game_runs=0,editions=editions,precision_differences=differences,files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(files)},scope='stage0..3 자격충족 Bridge→실제PathStart/Init/BFS/명령/끝점교체/Prepare/깊이. oldLookup/Host부착/가상공간/Frame/cue장치는 명시 요청 경계. 두 x87 결과의 double시각 차이는 별도 보존하며 같은 입력을 합산하지 않는다. 단계5 손상목표/전체도착·같은Host실제소비자는 후속이다.'),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('다이스 Bridge 실제 Path 상위 관찰:',len(rows))

def verify():
    """새 입력의 고유성/정상 반환/현재 전체 출처를 감사한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'))
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==data['total'] and not data['smoke'] and data['game_runs']==0
    assert len({(row[0],row[1]) for row in rows})==len(rows)
    for name,value in data['files'].items():assert sha(ROOT/name)==value,name
    for edition,item in data['editions'].items():assert item['returns']==2*sum(row[0]==edition for row in rows) and item['asserts']==0
    print('다이스 Bridge 현재 출처/정상반환 감사:',len(rows))

# 명시 CLI 요청에서만 새 독립 관찰을 실행하거나 저장 자료를 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--smoke',action='store_true')
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)
