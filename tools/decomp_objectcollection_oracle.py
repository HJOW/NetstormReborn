#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""일반 자산/그림자 수집과 SHP/타입/해시/CRT 보조를 세 PE에서 격리 실행한다. 응답 대체는 없다."""
import argparse
import collections
import csv
import json
import random
import struct
from decomp_drawsort_oracle import REGISTERS
from decomp_process_oracle import (ProcessOracle, ROOT, BINARIES, CONTROLS, STACK, STOP, digest, fbits,
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 격리된 출력/타입/물리 SHP/세 해시/슬롯 주소다. 실제 OS 주소로 사용하지 않는다.
DATA, ARRAY, TYPES, SHAPES, POOL = 0x15000000,0x15001000,0x15004000,0x15008000,0x15030000
GRIDS={1:0x15010000,2:0x15020000,3:0x15024000}
CAPACITY, SEED = 64,0x43366022
FIXTURE=ROOT/'cpppj/tests/fixtures/objectcollection-x86.tsv'
REPORT=ROOT/'cpppj/objectcollection.json'
# 전체 수집/현재 해시/크기/월드/풀/타입 배열의 원본 전역과 슬롯 배치다.
SPECS={
    'originals':dict(collect=0x433660,hash=0x542508,scales=0x5424f8,world=0x531928,pool=0x5c8464,types=0x59ab20,stride=50,type_stride=500),
    'originalCD':dict(collect=0x4887a0,hash=0x5670c0,scales=0x52d598,world=0x52e9a8,pool=0x5395dc,types=0x51c960,stride=36,type_stride=468),
}
SPECS['original1037']=dict(SPECS['originalCD'])


def node(sid=5,**fields):
    """좌표/타입/프레임/링크의 완성 입력을 만든다. 기대 목록은 만들지 않는다."""
    value=dict(sid=sid,type=1,frame=0,next=0,depth=127,extra=3,state=255,x=fbits(20),y=fbits(25))
    value.update(fields);return value


def type_input(number,flags1=0,flags2=0):
    """실제 타입 필드와 네 물리 SHP의 float 경계 입력을 만든다."""
    return dict(number=number,flags1=flags1,flags2=flags2,hp=100,count=2,
        geometry=[[fbits(v) for v in values] for values in ((2,3,.25,.5),(4,2,1,.25),(7,8,2,3),(1,1,-2,-1))])


def plan():
    """레이어/깊이/정확한 교차/NaN/판본별 저장/체인/단계/제한에 대한 독립 입력 계획이다."""
    result=[]
    def add(nodes=None,view=(10,20,30,40),cells=None,types=None,limit=64,selected=0):
        """호출 인자와 완성 입력만 복사하고 결과는 PE가 계산하게 한다."""
        result.append(dict(nodes=nodes if nodes is not None else [node()],view=[fbits(v) for v in view],
            cells=cells if cells is not None else [(3,1,1,5)],types=types if types is not None else [type_input(1)],limit=limit,selected=selected))
    # abstract의 경계 전/이후와 그림자, walker의 별도 y 보정, signed BYTE 깊이를 바꾼다.
    for flags in (0,0x40000,0x400000,0x440000):
        frames=(0,1,2,3) if flags==0x400000 else (0,1)
        for walker in (0,0x10000):
            for frame in frames:
                for depth in (128,255,0,1,127): add([node(frame=frame,depth=depth)],types=[type_input(1,flags,walker)])
    # 각 축의 정확한 양 끝과 인접 float를 본체/그림자의 다른 경계에 공급한다.
    for flags in (0,0x40000,0x400000,0x440000):
        for x in (7.249999523162842,7.25,7.250000476837158,30.249998092651367,30.25,30.250001907348633):
            for y in (16.499998092651367,16.5,16.500001907348633,40.499996185302734,40.5,40.500003814697266):
                add([node(x=fbits(x),y=fbits(y))],types=[type_input(1,flags)])
    # 본체/그림자 중 하나만 들어오는 경계와 네 값 모두의 비유한 입력이다.
    for flags in (0,0x40000):
        for field in ('x','y'):
            for value in (-float('inf'),float('inf'),float('nan')): add([node(**{field:fbits(value)})],types=[type_input(1,flags)])
        for axis in range(4):
            for value in (-float('inf'),float('inf'),float('nan')):
                view=[10,20,30,40];view[axis]=value;add(view=view,types=[type_input(1,flags)])
        for axis in range(4):
            for value in (-1,0,float('nan'),float('inf')):
                t=type_input(1,flags);t['geometry'][0][axis]=fbits(value);add(types=[t])
    # 패치 x 차이는 x87, y 차이는 float이며 CD는 top 경계까지 float에 저장한다.
    for axis in (0,1):
        t=type_input(1);t['geometry'][0]=[fbits(2),fbits(2),0,0];t['geometry'][0][axis+2]=fbits(2**-24)
        add([node(x=fbits(16),y=fbits(16))],view=(19 if axis==0 else 0,19 if axis==1 else 0,30,30),types=[t])
    t=type_input(1);t['geometry'][0]=[fbits(2),fbits(.9999999403953552),0,0]
    add([node(y=fbits(14))],view=(0,16,30,30),types=[t])
    # 다음 체인과 단계/y/x 순서를 별도로 관찰한다. 표면 0단계는 일반 수집에 들어오지 않는다.
    nodes=[node(5,next=6,depth=128),node(6,type=2,next=7,depth=1),node(7,type=3,frame=2,depth=255),node(8,type=4),node(9),node(10),node(11)]
    types=[type_input(1),type_input(2,0x40000,0x10000),type_input(3,0x400000),type_input(4,0x440000)]
    for selected in range(4):
        add(nodes,cells=[(0,20,25,11),(1,6,9,8),(1,8,11,5),(2,3,5,9),(3,1,1,10)],types=types,selected=selected)
        add(nodes,cells=[(1,8,11,5),(2,4,6,5),(3,1,1,5)],types=types,selected=selected)
    for extra in (0,1,2,3,255): add([node(extra=extra)],types=[type_input(1,0x40000)])
    for view in ((30,40,10,20),(-100,-100,-1,-1),(255,255,300,300),(0,0,255,255)):
        add(nodes,view=view,cells=[(1,0,0,5),(2,63,63,8),(3,1,1,9)],types=types)
    for limit in (-1,0,1): add(nodes=[],cells=[],limit=limit)
    add(limit=2);add(types=[type_input(1,0x40000)],limit=3)
    rng=random.Random(SEED)
    # 작은 창의 불규칙 체인, 별도 현재 프레임과 물리 레이어 경계를 고정 시드로 넓힌다.
    for _ in range(140):
        nodes=[];cells=[];types=[]
        for number,flags in enumerate((0,0x40000,0x400000,0x440000),1):
            t=type_input(number,flags,rng.choice((0,0x10000)))
            t['geometry']=[[fbits(rng.choice((.25,1,2,4,8))) for _ in range(2)]+[fbits(rng.choice((-1,0,.25,1,3))) for _ in range(2)] for _ in range(4)];types.append(t)
        for sid in range(5,12):
            typ=rng.randrange(1,5);frame=rng.randrange(4 if typ==3 else 2)
            nodes.append(node(sid,type=typ,frame=frame,next=sid+1 if sid<11 and rng.randrange(2) else 0,
                depth=rng.randrange(256),x=fbits(rng.uniform(0,40)),y=fbits(rng.uniform(0,50)),extra=rng.randrange(256)))
        for level in range(1,4): cells.append((level,rng.randrange(3),rng.randrange(3),rng.randrange(5,12)))
        add(nodes,view=(rng.uniform(0,10),rng.uniform(0,10),rng.uniform(10,40),rng.uniform(10,50)),cells=cells,types=types,selected=rng.randrange(4))
    return result


def raw(n,edition):
    """슬롯 전체 바이트를 입력으로 만들며 판본별 frame/extra/depth 폭을 구분한다."""
    stride=SPECS[edition]['stride'];value=bytearray(((i*29+n['sid']*17)&255 for i in range(stride)))
    struct.pack_into('<H',value,4,n['next']);value[10]=n['type'];value[11]=n['state']
    struct.pack_into('<II',value,14,n['x'],n['y'])
    if stride==50: value[35]=n['depth'];struct.pack_into('<I',value,36,n['frame']);value[40]=n['extra']
    else: value[33]=n['depth'];value[34]=n['frame'];value[35]=n['extra']
    return bytes(value)


def encoded(case,edition):
    """호스트 재생용 호출/칸/완성 raw/타입 필드·물리 SHP 경계를 직렬화한다."""
    config=','.join(map(str,(*case['view'],case['limit'],case['selected'])))
    cells=';'.join(','.join(map(str,c)) for c in case['cells']) or '-'
    nodes=';'.join(f'{n["sid"]}={raw(n,edition).hex()}' for n in case['nodes']) or '-'
    types=';'.join(f'{t["number"]}:'+','.join(map(str,(t['flags1'],t['flags2'],t['hp'],t['count'])))+':'+b''.join(struct.pack('<4I',*g) for g in t['geometry']).hex() for t in case['types'])
    return config,cells,nodes,types


class ObjectCollectionOracle(ProcessOracle):
    """전체 실제 수집/프레임/타입/해시/CRT 명령과 입력 보존/ABI만 감사한다."""
    def __init__(self,edition):
        """판본별 읽기 전용 내보내기의 불연속 허용 범위와 격리 저장소를 준비한다."""
        super().__init__(edition);self.d=SPECS[edition];self.extra=[ROOT/f'extracted/objectcollection/{edition}/{n}' for n in ('creation.c','functions.tsv')]
        self.allowed,self.entry_names=[],{}
        with self.extra[1].open(encoding='utf-8') as fp:
            # 원본 함수 사이 코드는 허용하지 않는다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entry_names[int(row['entry'],16)]=row['entry']
                # 불연속 원본 함수 범위를 따로 등록한다.
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.mu.mem_map(DATA,0x40000);self.returns=collections.Counter()

    def on_instruction(self,mu,address,size,data):
        """응답 대체 없이 실제 몸체만 허용하며 assert/OS/누락 함수 진입은 실패한다."""
        if not any(low<=address and address+size<=high for low,high in self.allowed): raise RuntimeError(f'Object 허용 밖 실행: {self.binary} {address:08x}')
        if address in self.entry_names: self.hits[self.entry_names[address]]+=1;self.entries[self.entry_names[address]]+=1

    def run(self,case,control):
        """완성 입력을 설정하고 실제 목록/현재 해시/미사용/보호/읽기 전용 입력과 ABI를 관찰한다."""
        mu,d=self.mu,self.d;self.entries=collections.Counter();encoded_input=encoded(case,self.binary)
        mu.mem_write(DATA,b'\0'*0x40000);mu.mem_write(ARRAY-16,b'\xa5'*(CAPACITY*16+32))
        # 전체 해시를 입력 칸의 SID 머리로만 채운다. 기대 방문이나 수집은 계산하지 않는다.
        for level,x,y,sid in case['cells']:
            if level: self.put(GRIDS[level]+((256//(2,4,16)[level-1])*y+x)*2,sid,2)
        # 판본별 슬롯의 모든 바이트를 그대로 공급한다.
        for n in case['nodes']: mu.mem_write(POOL+n['sid']*d['stride'],raw(n,self.binary))
        # 타입/프레임은 실제 원본 SHP 참조 규칙이 읽을 가상 메모리 입력이다.
        for t in case['types']:
            pointer=TYPES+t['number']*d['type_stride'];shape=SHAPES+t['number']*0x800
            for offset,value in ((0,t['hp']),(0xdc,shape),(0xe8,t['flags1']),(0xec,t['flags2']),(0x114,t['count'])): self.put(pointer+offset,value)
            # VFX 포인터 앞 36바이트 중 첫 16바이트가 월드 표시 경계다.
            for index,g in enumerate(t['geometry']):
                self.put(shape+8+index*8,128+index*64+36);mu.mem_write(shape+128+index*64,struct.pack('<4I',*g))
        self.put(d['pool'],POOL);self.put(d['types'],TYPES);self.put(d['world'],256);mu.mem_write(d['scales'],struct.pack('<4I',1,2,4,16))
        mu.mem_write(d['hash'],struct.pack('<7I',case['selected'],99,7,GRIDS[1],GRIDS[1],GRIDS[2],GRIDS[3]))
        readonly=[(TYPES,0x2000),(SHAPES,0x8000),(GRIDS[1],0x8000),(GRIDS[2],0x2000),(GRIDS[3],0x200),(POOL,0x1000)]
        snapshots=[bytes(mu.mem_read(a,n)) for a,n in readonly]
        self.writes=[(0,4),(STACK-0xf000,STACK+0x1000),(ARRAY,ARRAY+CAPACITY*16),(d['hash'],d['hash']+12)]
        # 비영 보존 레지스터와 cdecl 스택/x87/SEH의 정상 반환을 검사한다.
        for reg,value in REGISTERS.items(): mu.reg_write(reg,value)
        # 호출자 임시 레지스터는 0에서 시작한다.
        for reg in (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX): mu.reg_write(reg,0)
        mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);self.put(0,0xffffffff)
        mu.mem_write(STACK,struct.pack('<7I',STOP,*(v&0xffffffff for v in (*case['view'],ARRAY,case['limit']))));mu.emu_start(d['collect'],STOP,timeout=30000000,count=5000000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4 or any(mu.reg_read(r)!=v for r,v in REGISTERS.items()): raise RuntimeError('Object 정상 반환/스택/보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPSW)&0x3800 or mu.reg_read(UC_X86_REG_FPCW)!=control or self.u32(0)!=0xffffffff: raise RuntimeError('Object x87/SEH 오류')
        count=mu.reg_read(UC_X86_REG_EAX);self.returns[f'{d["collect"]:08x}']+=1
        if not 0<=count<CAPACITY: raise RuntimeError('Object 관찰 개수 범위 오류')
        output=bytes(mu.mem_read(ARRAY,CAPACITY*16));guard=bytes(mu.mem_read(ARRAY-16,16))+bytes(mu.mem_read(ARRAY+CAPACITY*16,16))
        if output[count*16:]!=b'\xa5'*(CAPACITY-count)*16 or guard!=b'\xa5'*32: raise RuntimeError('Object 미사용/보호 출력 변경')
        # 타입/SHP/전체 해시/슬롯의 모든 입력 바이트가 보존되어야 한다.
        for (address,size),before in zip(readonly,snapshots,strict=True):
            if bytes(mu.mem_read(address,size))!=before: raise RuntimeError('Object 읽기 전용 입력 변경')
        state=','.join(map(str,struct.unpack('<3I',mu.mem_read(d['hash'],12))))
        if state!='2,4,64': raise RuntimeError('Object 정상 종료 해시 오류')
        return [*encoded_input,str(count),output.hex(),state,json.dumps(dict(self.entries),sort_keys=True,separators=(',',':')),guard.hex()]


def generate(smoke):
    """두 x87 정밀도의 전체 관찰이 같은 입력만 저장하며 기대 수집은 만들지 않는다."""
    rows,editions=[],{};files={ROOT/'tools/decomp_objectcollection_oracle.py',ROOT/'tools/decomp_process_oracle.py',ROOT/'tools/decomp_drawsort_oracle.py',ROOT/'tools/ghidra/objectcollection-functions.json',FIXTURE}
    # 세 독립 PE의 정상 루트를 동일 입력에서 실행한다.
    for edition in BINARIES:
        oracle=ObjectCollectionOracle(edition);indexed=list(enumerate(plan()))
        if smoke: indexed=indexed[::41]+indexed[-3:]
        # 53/64비트 정밀도의 목록/진입/정상 종료 관찰을 각각 실행한다.
        for index,case in indexed:
            observed=[oracle.run(case,c) for c in CONTROLS]
            if observed[0]!=observed[1]: raise RuntimeError(f'Object x87 차이: {edition} {index}')
            rows.append([edition,str(index),*observed[0]])
        editions[edition]=dict(cases=len(indexed),entries=dict(oracle.hits),returns=dict(oracle.returns));files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(f'{edition}: {len(indexed)}개 통과',flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 일반 자산/그림자 수집 관찰. 프레임/타입/해시/CRT도 모두 실제 몸체다.\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-11',controls=list(CONTROLS),os_calls=0,assertions=0,boundaries=[],editions=editions,
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},limits=['수집/프레임/타입/좌표/해시/CRT 전체 실제 몸체, 응답 대체 0',
            '별도 main/shadow 경계, 판본별 float 저장/NaN, signed 깊이/프레임, 단계/y/x/next와 strict 제한 관찰',
            '전체 출력/미사용/가드/읽기 전용 입력/ABI/x87/SEH/불연속 실행/허용 쓰기/진입/반환/SHA 감사',
            'assert를 넘는 손상 입력은 C++ 계약 검사; 실제 sprite/SHP 장치/GUI 프레임은 후속']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA/완성 입력/폭/보호/정상 종료와 실제 진입/반환을 감사한다. 기대 목록을 계산하지 않는다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[s.split('\t') for s in FIXTURE.read_text(encoding='utf-8').splitlines() if s and not s.startswith('#')]
    # 각 근거 파일은 정확한 바이트 SHA로 대조한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha: raise RuntimeError('Object SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['boundaries'] or report['os_calls'] or report['assertions'] or set(report['editions'])!=set(BINARIES): raise RuntimeError('Object 총계 오류')
    # 각 독립 판본의 행 계획과 실제 집계를 따로 확인한다.
    for edition,item in report['editions'].items():
        chosen=[r for r in rows if r[0]==edition];entries=collections.Counter()
        if len(chosen)!=len(plan()) or item['cases']!=len(chosen): raise RuntimeError('Object 행 수 오류')
        # 결과는 저장 관찰을 사용하고 입력/폭/가드/진입 수만 감사한다.
        for index,(row,case) in enumerate(zip(chosen,plan(),strict=True)):
            if len(row)!=11 or row[1]!=str(index) or tuple(row[2:6])!=encoded(case,edition): raise RuntimeError('Object 입력 오류')
            if len(bytes.fromhex(row[7]))!=CAPACITY*16 or row[8]!='2,4,64' or row[10]!=(b'\xa5'*32).hex(): raise RuntimeError('Object 출력 오류')
            counts=json.loads(row[9])
            if counts.get(f'{SPECS[edition]["collect"]:08x}')!=1: raise RuntimeError('Object 루트 진입 오류')
            for address,count in counts.items(): entries[address]+=count*2
        if dict(entries)!=item['entries'] or item['returns']!={f'{SPECS[edition]["collect"]:08x}':len(chosen)*2}: raise RuntimeError('Object 집계 오류')
    print(f'objectcollection 검증 통과: {len(rows)}개, 두 정밀도 정상 반환 {len(rows)*2}회, 응답/OS/assert 0')


def main():
    """전체 생성/무저장 표본/저장 감사 중 하나를 실행한다."""
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--smoke',action='store_true');p.add_argument('--verify',action='store_true');args=p.parse_args()
    verify() if args.verify else generate(args.smoke)


if __name__=='__main__': main()
