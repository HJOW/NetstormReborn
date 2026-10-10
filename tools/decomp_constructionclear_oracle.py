#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""배치 통지 전의 로컬 예측 조각 정리(Construction.cpp 00441fd0 / CD 004cff80)를 세 실제 PE에서 대조한다.

실제 명령으로 실행하는 것: 정리 전체 몸체(칸 순회·후보 조건·타입/프레임/방향 글자/소유자 비교), CanonDecoder 생성/진행,
칸 범위 계산과 좌표 자르기·유효성, 절삭/올림, 일반 탐색기 Begin/Next와 기본 필터, 방향 글자 조회.
대체하는 것(명시 경계): 후보의 가상 삭제(+0x10)뿐이다. 탐색기 Begin의 네 정수 인자는 대체 없이 관찰만 한다.
원본 게임/OS는 실행하지 않는다.

python -X utf8 tools/decomp_constructionclear_oracle.py            # 기대값·근거 기록 생성
python -X utf8 tools/decomp_constructionclear_oracle.py --smoke    # 저장 없이 표본만 실행
python -X utf8 tools/decomp_constructionclear_oracle.py --verify   # 저장된 기록·입력 파일의 SHA/행 수/사건 수 확인
"""
import argparse
import collections
import csv
import itertools
import json
import random
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,TYPES,POOL,STOP,digest
from decomp_constructionplace_oracle import PlaceOracle,VTABLE,PLAIN,BRIDGE,NOISLAND,frame_codes
from decomp_priestgeometry_oracle import DEC
from decomp_damageablepredestroy_oracle import CONTROLS,bits
from unicorn.x86_const import UC_X86_REG_ECX

# 판본별 진입점·탐색기 Begin·전역 주소와 프레임 필드 배치다.
SPEC={
 'originals':dict(entry=0x441fd0,begin=0x4b16d0,authority=0x540bc4,grid=0x542508,board=0x531928,capacity=0x5c847c,frame=0x24,frame_width=4),
 'originalCD':dict(entry=0x4cff80,begin=0x4eae20,authority=0x540a2c,grid=0x5670c0,board=0x52e9a8,capacity=0x5395f4,frame=0x22,frame_width=1),
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 네 단계 해시 배열의 입력 주소, 가상 삭제 경계 주소, 단계별 버킷 폭, 후보로 섞는 다른 타입 번호다.
MAP,KILL,SCALES,OTHER=0x14000000,STOP+0x400,(1,2,4,16),102
# 후보 입력을 만드는 고정 시드다. 기대 결과는 원본 기계어가 정한다.
SEED=0x441fd0
FIXTURE=ROOT/'cpppj/tests/fixtures/constructionclear-x86.tsv'
REPORT=ROOT/'cpppj/recovery-constructionclear-evidence.json'
# 입력 칸 순서다. 후보 칸은 따로 적는다(번호,타입,extra,x,y,소유자,프레임,해시 단계).
KEYS=('type','argument','direction','x','y','player','authority','footX','footY','frames','default','otherFootX','otherFootY')


def clear_case(**changes):
    """정리 입력의 기본값에 바꿀 칸만 덮어쓴다. 기본은 발자국 1×1의 한 칸 타입이다."""
    case=dict(type=PLAIN,argument=7,direction=0,x=bits(20.5),y=bits(21.25),player=3,authority=1,footX=1,footY=1,frames=0,default=3,
        otherFootX=1,otherFootY=1)
    case.update(changes)
    return case


def inputs(edition):
    """시작 좌표·발자국·타입 부류·권한·플레이어 폭을 교차한 입력이다. 후보는 실제 칸을 센 뒤 따로 붙인다."""
    cases=[];directions=(0,1,2,3,4,5,6,7) if edition=='originals' else (0,2,4,6)
    positions=((20.5,21.25),(20.0,21.0),(1.0,1.0),(254.5,254.9),(0.5,0.5),(0.0,5.0),(256.0,300.0),(-3.5,-2.5),(255.99998,0.000001),(128.75,3.0))
    footprints=((1,1),(2,2),(3,1),(1,4))
    # 한 칸 타입: 좌표 × 발자국 × 권한 × 다른 타입의 발자국.
    for position,foot,authority,other in itertools.product(positions,footprints,(0,1),((1,1),(3,3))):
        cases.append(clear_case(x=bits(position[0]),y=bits(position[1]),footX=foot[0],footY=foot[1],authority=authority,otherFootX=other[0],otherFootY=other[1]))
    # 플레이어 인자는 DWORD로 비교한다. 바이트로 줄이면 같아 보이는 값을 넣는다.
    for player,authority in itertools.product((0,1,8,0x103,0xffffff03),(0,1)):
        cases.append(clear_case(player=player,authority=authority))
        cases.append(clear_case(type=NOISLAND,argument=0,player=player,authority=authority))
    # noIsland 아홉 칸: 방향과 가장자리 좌표.
    for direction,position in itertools.product(directions,((20.5,21.25),(1.0,1.0),(253.5,253.25),(0.0,0.0))):
        cases.append(clear_case(type=NOISLAND,argument=0,direction=direction,x=bits(position[0]),y=bits(position[1])))
    # 다리: 모든 패턴 × 방향에 프레임 표를 돌려 가며 넣는다. 프레임이 달라도 방향 글자가 같으면 같은 조각으로 본다.
    for index,(pattern,direction) in enumerate(itertools.product(range(26),directions)):
        cases.append(clear_case(type=BRIDGE,argument=pattern,direction=direction,frames=index%2,authority=(index//2)%2,player=1+index%8))
    return cases


def candidates(edition,case,index,cells):
    """실제 칸 목록 주변에 후보 조각 입력을 만든다. 어떤 후보가 지워지는지는 계산하지 않는다."""
    rng=random.Random(SEED+index*7919);first=SPECS_FIRST[edition];codes=frame_codes(case['frames']);count=len(codes)//4;nodes=[]
    spots=[(struct.unpack('<f',struct.pack('<I',x))[0],struct.unpack('<f',struct.pack('<I',y))[0],frame) for frame,x,y in cells]
    if not spots:spots=[(struct.unpack('<f',struct.pack('<I',case['x']))[0],struct.unpack('<f',struct.pack('<I',case['y']))[0],0)]
    # 후보마다 칸·어긋남·abstract·번호 영역·타입·프레임·소유자·해시 단계를 고른다.
    for number in range(rng.choice((0,1,2,3,4,6))):
        x,y,frame=rng.choice(spots);x+=rng.choice((0,0,0,0.4,-0.4,1,-1,3));y+=rng.choice((0,0,0,0.4,-0.4,1,-1,3))
        x=min(max(x,1.0),255.0);y=min(max(y,1.0),255.0)
        typ=case['type'] if rng.random()<0.75 else OTHER
        choice=rng.random();value=frame
        if choice>=0.5 and count:
            value=(frame+1)%count
            if choice>=0.75:
                # 방향 글자는 같고 번호가 다른 프레임을 찾는다. 없으면 한 칸 옆 프레임을 그대로 쓴다.
                side=codes[frame*4] if 0<=frame<count else None
                same=[i for i in range(count) if i!=frame and codes[i*4]==side]
                if same:value=rng.choice(same)
        nodes.append(dict(sid=(50 if rng.random()<0.75 else first+2)+number,type=typ,extra=rng.choice((1,1,1,1,0,3,9)),x=bits(x),y=bits(y),
            owner=case['player']&0xff if rng.random()<0.67 else (case['player']+1)&0xff,frame=value&(0xffffffff if edition=='originals' else 0xff),level=rng.randrange(4)))
    return nodes


# 판본별 서버 영역의 첫 번호다. 후보 번호를 클라이언트/서버 영역으로 나누는 데 쓴다.
SPECS_FIRST={'originals':15000,'originalCD':6000,'original1037':6000}


class ClearOracle(PlaceOracle):
    """정리 전체 몸체와 실제 decoder·칸 범위·탐색기를 실행하고 가상 삭제만 기록한다."""
    def __init__(self,edition):
        """새 내보내기 범위, 네 단계 해시 배열과 그 전역, 가상 삭제 경계를 준비한다."""
        super().__init__(edition);self.x=x=SPEC[edition];mu=self.mu
        self.exports=[ROOT/f'extracted/constructionclear/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 내보낸 함수의 불연속 몸체만 실행을 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 범위 끝 주소는 포함이므로 반열린 구간으로 바꿔 둔다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        mu.mem_map(MAP,0x40000);self.bases=[];address=MAP
        # 단계마다 (256/폭)² 개의 WORD 머리를 둔다.
        for scale in SCALES:
            self.bases.append(address);address+=(256//scale)**2*2
        self.map_end=address;mu.mem_write(x['grid'],struct.pack('<7I',0,1,256,*self.bases))
        mu.mem_write(x['board'],struct.pack('<I',256));mu.mem_write(x['capacity'],struct.pack('<I',self.p['server_first']+32))
        mu.mem_write(VTABLE+0x10,struct.pack('<I',KILL))
        self.stub_calls=collections.Counter();self.returns=collections.Counter();self.native_calls=collections.Counter();self.areas=0

    def on_instruction(self,mu,address,size,data):
        """가상 삭제는 기록 대체하고 탐색기 Begin은 인자만 관찰한 뒤 그대로 실행한다."""
        if address==KILL:
            flags,=self.args(1);self.events.append(f'K:{self.sid(mu.reg_read(UC_X86_REG_ECX))}:{flags}');self.stub_calls['K']+=1;self.ret(purge=4);return
        if address==self.x['begin'] and self.observing:
            area=struct.unpack('<4i',struct.pack('<4I',*self.args(4)));self.events.append('A:'+':'.join(map(str,area)));self.areas+=1
        OwnerOracle.on_instruction(self,mu,address,size,data)

    def prepare(self,case,nodes):
        """타입 표·프레임 표·발자국·권한 전역과 후보 슬롯/해시 등록 입력을 쓴다."""
        mu=self.mu;self.observing=False;self.setup(dict(case,flags1=0,flags2=0,maxHp=0,cost=0));self.case=case
        mu.mem_write(self.x['authority'],struct.pack('<I',case['authority']))
        mu.mem_write(TYPES+case['type']*self.type_stride+self.s['foot'],struct.pack('<2i',case['footX'],case['footY']))
        other=TYPES+OTHER*self.type_stride;mu.mem_write(other,bytes(self.type_stride))
        mu.mem_write(other+self.s['foot'],struct.pack('<2i',case['otherFootX'],case['otherFootY']))
        mu.mem_write(MAP,bytes(self.map_end-MAP))
        # 후보마다 raw 슬롯을 쓰고 지정한 단계의 버킷 체인 머리에 잇는다.
        for node in nodes:
            raw=bytearray(self.stride);struct.pack_into('<I',raw,0,VTABLE);raw[10]=node['type'];raw[self.o['extra']]=node['extra']
            raw[self.o['owner']]=node['owner'];struct.pack_into('<2I',raw,14,node['x'],node['y'])
            raw[self.x['frame']:self.x['frame']+self.x['frame_width']]=node['frame'].to_bytes(self.x['frame_width'],'little')
            cx,cy=(int(struct.unpack('<f',struct.pack('<I',value))[0]) for value in (node['x'],node['y']));scale=SCALES[node['level']]
            address=self.bases[node['level']]+((cy//scale)*(256//scale)+cx//scale)*2
            struct.pack_into('<H',raw,4,struct.unpack('<H',mu.mem_read(address,2))[0])
            mu.mem_write(self.slot(node['sid']),bytes(raw));mu.mem_write(address,struct.pack('<H',node['sid']))

    def cells(self,case):
        """실제 decoder 생성/진행으로 칸마다 (프레임, x, y)를 읽는다. 후보 입력을 놓을 자리를 정하는 데만 쓰며 호출 집계에서 뺀다."""
        saved=collections.Counter(self.native_calls);self.prepare(case,[]);self.mu.mem_write(DEC,bytes(84));self.write_ranges=[]
        self.call(self.s['begin'],[case[k] for k in ('type','argument','direction','x','y')]+[0],CONTROLS[0]);result=[]
        # 유효 칸이 남아 있는 동안 진행한다. 패턴은 최대 15칸이다.
        while struct.unpack('<I',self.mu.mem_read(DEC+20,4))[0]:
            result.append((struct.unpack('<i',self.mu.mem_read(DEC+16,4))[0],*struct.unpack('<2I',self.mu.mem_read(DEC+32,8))))
            self.call(self.s['next'],[],CONTROLS[0])
            if len(result)>15:raise RuntimeError('패턴 칸 수 초과')
        self.native_calls=saved
        return result

    def clear(self,case,nodes,control):
        """정리 전체를 실행하고 사건(칸마다 탐색 범위, 삭제한 후보)을 돌려준다. 풀과 해시는 쓰지 않아야 한다."""
        mu=self.mu;self.prepare(case,nodes);self.write_ranges=[];before=bytes(mu.mem_read(MAP,self.map_end-MAP));self.observing=True
        try:self.invoke(self.x['entry'],[case[k] for k in ('type','x','y','argument','direction','player')],control)
        finally:self.observing=False
        if before!=bytes(mu.mem_read(MAP,self.map_end-MAP)):raise RuntimeError('정리 몸체가 해시를 수정함')
        return ';'.join(self.events) or '-'


def node_text(nodes):
    """후보 입력을 fixture 한 칸으로 만든다."""
    return ';'.join(','.join(str(node[k]) for k in ('sid','type','extra','x','y','owner','frame','level')) for node in nodes) or '-'


def generate(smoke=False):
    """세 실제 PE를 두 x87 정밀도로 실행해 같은 관찰만 fixture와 SHA 근거로 저장한다."""
    rows=[];editions={}
    paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_priestgeometry_oracle.py',ROOT/'tools/decomp_constructionplace_oracle.py',
        ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/constructionclear-functions.json',FIXTURE}
    # 판본마다 입력을 독립 실행한다.
    for edition in SPECS:
        oracle=ClearOracle(edition);cases=inputs(edition);cell_total=0;node_total=0
        if smoke:cases=cases[::9]
        # 칸은 실제 decoder로 읽고, 그 주변에 후보를 놓은 뒤 전체 몸체를 실행한다.
        for index,case in enumerate(cases):
            cells=oracle.cells(case);nodes=candidates(edition,case,index,cells)
            observed=[oracle.clear(case,nodes,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError(f'정리 x87 정밀도 차이: {edition} {case}')
            rows.append([edition,','.join(str(case[k]) for k in KEYS),node_text(nodes),observed[0]]);cell_total+=len(cells);node_total+=len(nodes)
        editions[edition]=dict(cases=len(cases),cells=cell_total,nodes=node_total,areas=oracle.areas,returns=dict(oracle.returns),
            native_calls=dict(oracle.native_calls),substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: constructionclear {len(cases)}개 통과',flush=True)
    if smoke:return
    header=['# 실제 로컬 예측 조각 정리 전체 몸체. 가상 삭제(+0x10)만 명시 경계이며 탐색기 Begin 인자는 대체 없이 관찰.',
        '# 판본 입력(쉼표: '+' '.join(KEYS)+') 후보(번호,타입,extra,x,y,소유자,프레임,해시단계;...) 사건(A:탐색 범위, K:번호:삭제 플래그)']
    FIXTURE.write_text('\n'.join(header)+'\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-10',decompile_date='2026-10-10',controls=list(CONTROLS),seed=SEED,
        os_calls=0,editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        limits=['정리 전체 몸체/칸 순회/후보 조건/타입·프레임·방향 글자·소유자 비교와 CanonDecoder·칸 범위·좌표 자르기/유효성·절삭/올림·일반 탐색기·기본 필터 정상 반환/ABI/x87',
            '후보의 가상 삭제(+0x10)만 명시 경계이며 삭제 뒤에도 후보는 해시에 남는다(실제 삭제의 공간 해제는 미실행)',
            '합성 타입/프레임 표/해시 등록 입력, 유한 시작 좌표, 후보 좌표는 1 이상 255 이하(CD 탐색기의 좌표 assert 제외)',
            '통지 처리기 004441b0·요청 00444760·실제 삭제/GUI 미실행']),
        ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(total=len(rows)),ensure_ascii=False))


def verify():
    """SHA·입력 순서·정상 반환 수·탐색 범위/삭제 사건 수를 감사한다. 기대 결과를 다시 계산하지 않는다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 저장된 원본 출력과 도구/내보내기의 변경을 검출한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls'] or report['seed']!=SEED:raise RuntimeError('예측 정리 근거 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];x=SPEC[edition];cases=inputs(edition)
        if len(selected)!=len(cases) or item['cases']!=len(cases) or item['assertions']:raise RuntimeError('예측 정리 행/assert 오류')
        nodes=0
        # 입력 칸이 생성 순서와 같은지 본다. 후보 칸은 실제 칸에 따라 만든 입력이라 개수만 센다.
        for row,case in zip(selected,cases,strict=True):
            if len(row)!=4 or row[1].split(',')!=[str(case[k]) for k in KEYS]:raise RuntimeError('예측 정리 입력/열 오류')
            nodes+=0 if row[2]=='-' else len(row[2].split(';'))
        events=[event for row in selected for event in row[3].split(';') if event!='-']
        areas=sum(event.startswith('A:') for event in events);kills=sum(event.startswith('K:') for event in events)
        if nodes!=item['nodes'] or areas!=item['cells'] or item['areas']!=2*areas:raise RuntimeError('예측 정리 후보/칸/탐색 수 오류')
        if item['substitutions'].get('K',0)!=2*kills or item['returns'].get(f'{x["entry"]:08x}',0)!=2*len(selected):raise RuntimeError('예측 정리 삭제/반환 수 오류')
        if item['native_calls'].get(f'{x["entry"]:08x}',0)!=2*len(selected) or item['native_calls'].get(f'{x["begin"]:08x}',0)!=2*areas:raise RuntimeError('실제 정리/탐색 진입 수 오류')
    print(f'constructionclear 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 근거 감사 중 하나를 실행한다. 게임/창은 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
