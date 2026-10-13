#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""건물 Pop/Unpop 옆면을 실제 두 PE에서 관찰한다. 기존 pop 근거와 도구는 변경하지 않는다."""
import argparse
import collections
import csv
import hashlib
import json
import struct
import zlib
from decomp_pop_oracle import PopOracle, HEADS, HEAD_COUNT, SPOTS, bits
from decomp_sid_oracle import POOL
from decomp_creation_oracle import TYPES
from decomp_oracle import ROOT

# 독립 입력의 전체 풀 크기, 직접 비교할 앞 슬롯 수와 실제 섬/다리 타입 입력이다.
CAPACITY=32768
PREFIX=192
ISLAND_TYPE=82
BRIDGE_TYPE=81
# 새 경계의 실제 주소는 현재 PC의 정밀 functions.tsv에서만 허용한다.
HELPERS={
    'originals':(0x4214a0,0x46f680,0x46f6e0),
    'originalCD':(0x448c10,0x4bdd20,0x4bdd80),
}
GLOBALS={'originals':(0x5411dc,0x5411a0),'originalCD':(0x51cac8,0x51ca8c)}


class BuildingOracle(PopOracle):
    """기존 실제 공간 몸체에 옆면의 섬 확인/무상태 진단 함수만 더한다."""
    def __init__(self,edition):
        """원본 바이트와 현재 PC의 실제 범위만 읽어 격리 실행한다."""
        super().__init__(edition)
        with (ROOT/f'extracted/refined/{edition}/functions.tsv').open(encoding='utf-8') as fp:
            facts={int(row['entry'],16):row for row in csv.DictReader(fp,delimiter='\t')}
        # 실제 세 함수의 불연속 몸체도 하나씩 허용한다.
        for entry in HELPERS[edition]:
            row=facts[entry];self.facts.append(row)
            # 사이 코드나 전체 이미지 실행을 허용하지 않는다.
            for part in row['ranges'].split(';'):
                lo,hi=(int(value,16) for value in part.split('-'));self.allowed.append((lo,hi+1))
        self.helper_calls=collections.Counter()

    def on_instruction(self,mu,address,size,data):
        """섬 확인과 옆면 진단의 실제 호출만 센다. 함수/반환값을 대체하지 않는다."""
        if address in HELPERS[self.edition]: self.helper_calls[f'{address:08x}']+=1
        super().on_instruction(mu,address,size,data)

    def snapshot(self,sid,result):
        """모든 후보 raw 바이트와 전체 풀/해시/spot 체크섬을 저장한다."""
        return [result,zlib.adler32(self.mu.mem_read(POOL,self.capacity*self.stride)),
            zlib.adler32(self.mu.mem_read(HEADS,HEAD_COUNT*2)),zlib.adler32(self.mu.mem_read(SPOTS,65536)),
            bytes(self.mu.mem_read(POOL,PREFIX*self.stride)).hex()]


def generate():
    """발자국·묻힘·표면 플래그·섬 지면·겹침을 실제 native 입력으로만 변형한다."""
    rows=['# 실제 건물 Pop/Unpop 옆면. 합성 타입/슬롯/지도 입력, 표시/postPop 억제, 비전투 null dirty.']
    editions={};counts=collections.Counter()
    # 두 판본을 각각 독립적인 가상 메모리에서 실행한다.
    for edition in HELPERS:
        oracle=BuildingOracle(edition)
        oracle.setup_space(CAPACITY,0x037f)
        # 실제 Reset이 만든 동일 준비 입력을 재사용한다. Reset 자체는 기존 SID oracle의 대조 범위다.
        initial_pool=bytes(oracle.mu.mem_read(POOL,CAPACITY*oracle.stride))
        initial_globals={address:bytes(oracle.mu.mem_read(address,4)) for address in oracle.spec['globals']}
        # 타입 입력 64개는 C++ 구현이나 출력값에서 생성하지 않는다.
        for case in range(64):
            oracle.mu.mem_write(POOL,initial_pool)
            oracle.mu.mem_write(HEADS,bytes(HEAD_COUNT*2));oracle.mu.mem_write(SPOTS,bytes(65536))
            # 할당 머리/꼬리/카운터도 같은 최초 실제 Reset 입력으로 복원한다.
            for address,value in initial_globals.items(): oracle.mu.mem_write(address,value)
            island_global,bridge_global=GLOBALS[edition]
            oracle.mu.mem_write(island_global,struct.pack('<I',ISLAND_TYPE))
            oracle.mu.mem_write(bridge_global,struct.pack('<I',BRIDGE_TYPE))
            f2=(0x4000,0x200,0x400000,0x4000000,0x40000000,0x10000000,0x40000,0x80040000)[case%8]|0x10
            width=1+case%5;height=1+(case//5)%5
            oracle.type_space(74,0,f2,width,height,4.,4.)
            # 이 두 후보 타입은 surface 여부만 바꾸며 원본 파생 ctor 실행이라고 주장하지 않는다.
            oracle.type_space(BRIDGE_TYPE,0x800 if case%3 else 0,4,1,1,1.,1.)
            oracle.type_space(ISLAND_TYPE,0x800,2,1,1,1.,1.)
            # 세 타입의 합성 SHP 버퍼가 공유되므로 마지막에 현재 Pop 건물의 실제 입력 크기를 다시 지정한다.
            oracle.type_space(74,0,f2,width,height,4.,4.)
            rows.append(f'Begin\t{edition}\t{case}\t{f2}\t{width}\t{height}\t{int(case%3!=0)}')
            sid=oracle.step_space('Create',0,74,30.,45.)[0]
            extra=0x88 if case%11==0 else 0x80
            oracle.fill_space(sid,case*3&255,4,74,oracle.creation['vtable'],extra)
            rows.append(f'Building\t{sid}\t{bytes(oracle.mu.mem_read(POOL+sid*oracle.stride,oracle.stride)).hex()}')
            left,top=31-width,46-height;candidate=100
            # 모서리와 내부를 포함해 모든 칸에 후보를 입력하므로 옆면 필터를 독립 검사한다.
            for y in range(top,46):
                # 같은 줄의 각 후보는 서로 다른 WORD/상태/coords를 갖는다.
                for x in range(left,31):
                    oracle.fill_space(candidate,(case+candidate)&255,(0,1,2,4,8)[(candidate+case)%5],BRIDGE_TYPE,oracle.creation['vtable'],0)
                    addr=POOL+candidate*oracle.stride
                    oracle.mu.mem_write(addr+12,struct.pack('<H',0xb5ff-(candidate%8)*8))
                    oracle.mu.mem_write(addr+14,struct.pack('<2f',65.75 if case%2 else 66.25,70.5))
                    oracle.mu.mem_write(HEADS+(y*256+x)*2,struct.pack('<H',candidate))
                    rows.append(f'Candidate\t{candidate}\t{x}\t{y}\t{bytes(oracle.mu.mem_read(addr,oracle.stride)).hex()}')
                    candidate+=1
            # 섬 타입 boolean 확인은 후보의 현재 좌표 지면을 다시 읽는다.
            oracle.fill_space(150,37,4,ISLAND_TYPE,oracle.creation['vtable'],0)
            oracle.mu.mem_write(HEADS+(70*256+65)*2,struct.pack('<H',150))
            rows.append(f'Candidate\t150\t65\t70\t{bytes(oracle.mu.mem_read(POOL+150*oracle.stride,oracle.stride)).hex()}')
            # 실제 호출자는 후보가 아닌 건물 this 좌표를 섬 확인에 넘긴다. 홀수 입력은 그 칸에 섬을 둔다.
            if case%2:
                oracle.mu.mem_write(HEADS+(45*256+30)*2,struct.pack('<H',150))
                rows.append(f'Candidate\t150\t30\t45\t{bytes(oracle.mu.mem_read(POOL+150*oracle.stride,oracle.stride)).hex()}')
            # 패치 겹침은 마지막 칸에서 발생시키며 앞선 옆면 쓰기까지 보존한다.
            if case%13==1:
                oracle.mu.mem_write(SPOTS+45*256+30,b'\xff');rows.append('Spot\t30\t45\t255')
            actual,out=oracle.step_space('Pop',sid,8,30.,45.)
            rows.append('\t'.join(map(str,['Step','Pop',actual,*out])));counts['Pop']+=1
            if not oracle.mu.mem_read(POOL+sid*oracle.stride+11,1)[0]&4:
                actual,out=oracle.step_space('Unpop',sid,8,30.,45.)
                rows.append('\t'.join(map(str,['Step','Unpop',actual,*out])));counts['Unpop']+=1
        editions[edition]=dict(binary_sha256=oracle.sha256,function_ranges=oracle.facts,
            actual_calls=dict(oracle.actual_calls),helper_calls=dict(oracle.helper_calls),assert_reports=oracle.assertions)
        assert oracle.assertions==0
        print(f'{edition}: 건물 옆면 native64입력 완료',flush=True)
    fixture=ROOT/'cpppj/tests/fixtures/buildingspace-x86.tsv'
    fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    sources=['tools/decomp_buildingspace_oracle.py','tools/decomp_pop_oracle.py','tools/decomp_creation_oracle.py',
        'tools/decomp_sid_oracle.py','tools/decomp_oracle.py','extracted/refined/originals/functions.tsv','extracted/refined/originalCD/functions.tsv']
    report=dict(schema=1,classification='actual-building-pop-unpop-side-effects',host='VM-W11-CODEX',
        cases=dict(counts),total_cases=sum(counts.values()),editions=editions,
        artifact_sha256={'cpppj/tests/fixtures/buildingspace-x86.tsv':hashlib.sha256(fixture.read_bytes()).hexdigest()},
        source_sha256={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in sources},
        limits=['합성 타입/SHP/후보 raw/map 입력, 표시/postPop 억제 및 null dirty의 실제 함수 실행',
            '전체 후보 앞192슬롯 바이트 직접 비교; 전체 풀/해시/spot은 Adler32',
            '별도 원본 프로세스/GUI/OS/게임 진입점 실행 없음; 미션 완주와 건물별 파생 연결 증명 아님'])
    (ROOT/'cpppj/recovery-buildingspace-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(cases=dict(counts),total_cases=report['total_cases'])),flush=True)


def verify():
    """입력/기록/원본 바이트와 실제 호출 개수를 재확인한다."""
    report=json.loads((ROOT/'cpppj/recovery-buildingspace-evidence.json').read_text(encoding='utf-8'))
    # 생성 당시 모든 도구/범위/fixture를 SHA로 확인한다.
    for group in ('artifact_sha256','source_sha256'):
        # 항목을 빠뜨리지 않고 현재 바이트와 비교한다.
        for name,digest in report[group].items(): assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==digest,name
    # 두 보호 PE와 assert 미도달을 확인한다.
    for edition,data in report['editions'].items():
        binary=ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE')
        assert hashlib.sha256(binary.read_bytes()).hexdigest()==data['binary_sha256'] and data['assert_reports']==0
    counts=collections.Counter(line.split('\t')[1] for line in (ROOT/'cpppj/tests/fixtures/buildingspace-x86.tsv').read_text(encoding='utf-8').splitlines() if line.startswith('Step\t'))
    assert dict(counts)==report['cases'] and sum(counts.values())==report['total_cases']
    print(json.dumps(dict(verified=True,cases=dict(counts),total_cases=report['total_cases'])),flush=True)


# 명시적인 실행에서만 격리 관찰 또는 기록 검증을 수행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate()
