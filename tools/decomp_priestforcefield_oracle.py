#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 보호막 조회/삭제 준비를 세 PE의 실제 좌표·finder·필터 명령으로 대조한다.

직접 조회에는 대체가 없다. preDestroy에서는 보호막 가상 삭제와 Carrier 몸체만 기록 대체한다.
원본 게임/OS/창을 실행하지 않는다.
"""
import argparse
import collections
import csv
import itertools
import json
import random
import struct
from pathlib import Path
from decomp_owner_oracle import (OwnerOracle, ROOT, SPECS, POOL, STOP, STACK, LISTS, TARGET, digest,
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI,
    UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)
from unicorn.x86_const import UC_X86_REG_FPCW, UC_X86_REG_FPSW

# 메모리 배치/원본 단계별 버킷 폭·두 x87 정밀도·합성 슬롯 번호다.
MAP, VTABLE, DESTROY = 0x14000000, 0x11010000, STOP+0x300
SCALES, CONTROLS, IDS = (1,2,4,16), (0x027f,0x037f), (50,60,61,62,63,64,65)
# 기존 C++ 월드와 같은 풀 크기와 입력 시드다. 기대 결과는 원본이 정한다.
CAPACITY, SEED = 24000, 0x4918e0
# 판본별 실제 함수와 공간/목록/타입 전역이다.
FIND = {
 'originals':dict(entry=0x4918e0,snap=0x41d770,begin=0x4b16d0,next=0x4b1810,ftol=0x4e49c0,filter=0x44daa0,
    pre=0x4919b0,carrier=0x426890,head=0x5954c4,capacity=0x5c847c,grid=0x542508,board=0x531928,type=0x5412f4,table=0x50f210,foot=0x1d4),
 'originalCD':dict(entry=0x40bf00,snap=0x440a50,begin=0x4eae20,next=0x4eafe0,ftol=0x4f161c,filter=0x40f180,
    pre=0x40c330,carrier=0x4e43b0,head=0x549188,capacity=0x5395f4,grid=0x5670c0,board=0x52e9a8,type=0x51cbe0,table=0x5003e0,foot=0x1b4),
}
FIND['original1037']=dict(FIND['originalCD'])
# 새 독립 입력/근거는 앞 단계 fixture를 바꾸지 않는다.
FIXTURE=ROOT/'cpppj/tests/fixtures/priestforcefield-x86.tsv'
REPORT=ROOT/'cpppj/recovery-priestforcefield-evidence.json'


def bits(value):
    """단정도 좌표 입력의 실제 비트를 만든다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def scenes(smoke=False):
    """타입/소유자·순서·발자국·상태·좌표 입력만 조합한다. 결과는 계산하지 않는다."""
    result=[]
    positions=[(20.75,21.9),(20.0,21.0),(20.99999,21.00001),(1.0,1.0),(254.9,254.25)]
    # 체인 순서와 단계 이동은 첫 일치 SID를 바꿀 수 있다.
    for position,layout,owner,type_number,extra in itertools.product(positions,range(8),(0,1,8,255),(167,168,256),(0,1,8)):
        x,y=position;nodes=[]
        for i,sid in enumerate(IDS[1:]):
            ax,ay=x,y
            if i==4:ax=min(255,x+1)
            if i==5:ay=min(255,y+1)
            width,height=1+layout%3,1+(layout//2)%3
            typ=(166,167,167,168,167,167)[i]
            own=owner if i in (1,3,4,5) else (owner+1)&255
            state=(0,1,2,4,8,6)[(layout+i)%6]
            buried=8 if layout==2 and i in (1,4) else 0
            if layout==3:own=(owner+1)&255
            if layout==4:typ=166
            nodes.append((sid,typ,state,buried,bits(ax),bits(ay),width,height,own,(layout+i)%4))
        if layout==1:nodes.reverse()
        result.append((bits(x),bits(y),owner,(0,2,4,6)[layout%4],extra,type_number,1,nodes))
    # 보드 가장자리/음수 -1 sentinel 및 BYTE로 줄이면 오답인 전역 타입을 포함한다.
    rng=random.Random(SEED)
    for i in range(48):
        x,y=rng.choice(((0.99,0.99),(-1.9,21.9),(20.75,-1.25),(-2.75,-2.25),(256.0,256.0),(300.75,21.9)))
        owner=rng.choice((0,1,8,255));nodes=[]
        footprints={typ:((1,1) if typ==158 else (rng.choice((1,2,3)),rng.choice((1,2,3)))) for typ in (158,166,167,168)}
        for j,sid in enumerate(IDS[1:]):
            ax,ay=rng.choice(((1.0,1.0),(20.75,21.9),(21.0,22.0),(255.5,255.25)))
            typ=rng.choice((158,166,167,168));width,height=footprints[typ]
            nodes.append((sid,typ,rng.choice((0,1,2,4,8,6)),rng.choice((0,1,8,9,32)),bits(ax),bits(ay),width,height,rng.choice((owner,owner,(owner+1)&255)),j%4))
        result.append((bits(x),bits(y),owner,rng.choice((0,2,4,6)),rng.choice((0,1,8,9,32)),rng.choice((158,167,256,0xffffffff)),0,nodes))
    if smoke:return result[::101]
    return result


class ForcefieldOracle(OwnerOracle):
    """원본 코드 범위만 실행하고 lookup에서는 stack 이외 쓰기를 허용하지 않는다."""
    def __init__(self,edition):
        """실제 네 해시와 새 내보내기, 두 파생 삭제 효과의 대체 주소를 준비한다."""
        super().__init__(edition);self.f=FIND[edition];base_exports=list(self.exports)
        self.mu.mem_map(POOL+0x2000,((CAPACITY*self.stride+4095)&~4095)-0x2000)
        self.mu.mem_map(MAP,0x40000);self.mu.mem_map(VTABLE,0x1000)
        self.bases=[];address=MAP
        for scale in SCALES:
            self.bases.append(address);address+=(256//scale)**2*2
        self.map_end=address;self.mu.mem_write(self.f['grid'],struct.pack('<7I',0,1,256,*self.bases))
        self.mu.mem_write(self.f['board'],struct.pack('<I',256));self.mu.mem_write(self.f['capacity'],struct.pack('<I',CAPACITY))
        self.mu.mem_write(VTABLE+16,struct.pack('<I',DESTROY));self.mu.mem_write(self.f['table']+16,struct.pack('<I',DESTROY))
        paths=[ROOT/f'extracted/priestforcefield/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.exports=base_exports+paths;self.allowed=[];self.entries=set();self.stub_calls=collections.Counter()
        with paths[1].open(encoding='utf-8') as fp:
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def on_instruction(self,mu,address,size,data):
        """lookup/finder/CRT/기본 true 필터는 대체하지 않는다. 삭제/Carrier만 기록한다."""
        if address not in (DESTROY,self.f['carrier']):super().on_instruction(mu,address,size,data);return
        if self.kind!='Pre':raise RuntimeError('직접 lookup에 외부 효과 호출')
        esp=mu.reg_read(UC_X86_REG_ESP);ret,flags=struct.unpack('<II',mu.mem_read(esp,8))
        sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//self.stride;marker='D' if address==DESTROY else 'C'
        if marker=='D' and flags:raise RuntimeError('보호막 삭제 flags는 0이어야 함')
        if marker=='C' and (sid!=TARGET or flags!=self.flags):raise RuntimeError('Carrier this/flags 오류')
        self.events.append(f'{marker}:{sid}:{flags}');self.stub_calls[marker]+=1
        mu.reg_write(UC_X86_REG_ESP,esp+8);mu.reg_write(UC_X86_REG_EIP,ret)

    def setup(self,scene):
        """합성 raw·타입 발자국·버킷 머리 입력만 만든다. 공간 반환 순서는 원본이 정한다."""
        x,y,owner,state,extra,typ,registered,nodes=scene
        self.mu.mem_write(MAP,bytes(self.map_end-MAP));self.mu.mem_write(self.f['type'],struct.pack('<I',typ))
        for sid in IDS:self.mu.mem_write(self.slot(sid),bytes(self.stride))
        root=(TARGET,158,state,extra,x,y,1,1,owner,0)
        self.put(root,self.f['table'],False)
        for node in ([root] if registered else [])+nodes:self.put(node,self.f['table'] if node[0]==TARGET else VTABLE,True)
        self.mu.mem_write(self.f['head'],struct.pack('<III',LISTS,6,6));self.mu.mem_write(LISTS,struct.pack('<6I',50,60,50,61,50,62))

    def put(self,node,table,registered):
        """같은 등록 입력을 구성한다. 타입별 발자국이 서로 충돌하는 입력은 만들지 않는다."""
        sid,typ,state,extra,x,y,width,height,owner,level=node
        raw=bytearray(self.stride);struct.pack_into('<I',raw,0,table);raw[10]=typ;raw[11]=state
        raw[self.o['extra']]=extra;raw[self.o['owner']]=owner;struct.pack_into('<II',raw,14,x,y)
        self.mu.mem_write(self.slot(sid),bytes(raw));self.mu.mem_write(0x11100000+typ*self.type_stride+self.f['foot'],struct.pack('<ii',width,height))
        if registered:
            cx,cy=(int(struct.unpack('<f',struct.pack('<I',value))[0]) for value in (x,y));scale=SCALES[level]
            address=self.bases[level]+((cy//scale)*(256//scale)+cx//scale)*2
            previous=struct.unpack('<H',self.mu.mem_read(address,2))[0]
            self.mu.mem_write(self.slot(sid)+4,struct.pack('<H',previous));self.mu.mem_write(address,struct.pack('<H',sid))

    def run(self,kind,scene,flags,control):
        """정상 반환/레지스터·x87 복구와 lookup의 풀/해시 읽기 전용 계약을 검사한다."""
        self.kind=kind;self.flags=flags;self.events=[];self.setup(scene)
        self.write_ranges=[] if kind=='Find' else [(LISTS,LISTS+24),(self.f['head']+8,self.f['head']+12)]
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        for reg,value in preserved.items():self.mu.reg_write(reg,value)
        self.mu.reg_write(UC_X86_REG_EAX,0);self.mu.reg_write(UC_X86_REG_EDX,0);self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,self.slot(TARGET))
        self.mu.mem_write(STACK,struct.pack('<II',STOP,flags));self.mu.emu_start(self.f['entry'] if kind=='Find' else self.f['pre'],STOP,count=50000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+(4 if kind=='Find' else 8):raise RuntimeError('보호막 lookup/pre 반환 스택 오류')
        if any(self.mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('보호막 보존 레지스터 오류')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('보호막 x87 복구 오류')
        entries=struct.unpack('<6I',self.mu.mem_read(LISTS,24));count=struct.unpack('<I',self.mu.mem_read(self.f['head']+8,4))[0]
        slots=';'.join(f'{sid}:'+bytes(self.mu.mem_read(self.slot(sid),self.stride)).hex() for sid in IDS)
        return [self.mu.reg_read(UC_X86_REG_EAX) if kind=='Find' else '-',count,','.join(map(str,entries)),';'.join(self.events) or '-',slots]


def generate(smoke=False):
    """두 정밀도의 실제 명령 출력이 같은 입력만 별도 UTF-8 fixture로 저장한다."""
    cases=scenes(smoke);rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/ghidra/priestforcefield-functions.json',FIXTURE}
    for edition in SPECS:
        oracle=ForcefieldOracle(edition)
        for index,scene in enumerate(cases):
            for kind in ('Find','Pre'):
                flags=(0,0x200000,0xffffffff)[index%3];values=[oracle.run(kind,scene,flags,c) for c in CONTROLS]
                if values[0]!=values[1]:raise RuntimeError(f'보호막 x87 결과 차이: {edition}/{index}/{kind}')
                rows.append([kind,edition,index,flags,*values[0]])
        editions[edition]=dict(cases=2*len(cases),native_calls=dict(oracle.native_calls),substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 보호막 {2*len(cases)}개 통과',flush=True)
    if smoke:return
    scene_rows=[['Scene',i,*scene[:-1],';'.join(','.join(map(str,node)) for node in scene[-1])] for i,scene in enumerate(cases)]
    FIXTURE.write_text('# 실제 보호막 lookup/finder/CRT/기본 true 필터. Pre의 가상 삭제/Carrier만 대체.\n# Scene id x y owner state extra type registered objects\n# Find/Pre edition scene flags result count entries events slots\n'+
        '\n'.join('\t'.join(map(str,row)) for row in scene_rows+rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),scenes=len(cases),host='HJOW-Athlon',date='2026-10-08',normal_return=True,os_calls=0,controls=list(CONTROLS),editions=editions,
        files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},limits=['조회·좌표/일반 finder/필터·pre 목록 제거/범위 검사 실제 실행',
            '직접 조회에는 대체 없음; Pre는 보호막 가상 삭제/Carrier 내부 몸체만 대체',
            '합성 발자국/해시 등록 입력; 실제 Pop/전체 보호막 수명·GUI 미복원']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA·행 수·실제 조회/좌표/Begin/Next/CRT 실행과 외부 대체 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    scenes_by_id={r[1]:r for r in rows if r[0]=='Scene'};rows=[r for r in rows if r[0]!='Scene']
    if len(rows)!=report['total'] or len(scenes_by_id)!=report['scenes'] or not report['normal_return'] or report['os_calls']:raise RuntimeError('보호막 행/반환 증거 오류')
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[1]==edition];find=FIND[edition]
        calls=sum(r[0]=='Find' or (int(scenes_by_id[r[2]][6])&9)==0 for r in selected)
        if item['cases']!=len(selected) or item['assertions']:raise RuntimeError('보호막 실행 수 오류')
        for address in (find['entry'],find['snap'],find['begin']):
            if item['native_calls'].get(f'{address:08x}')!=2*calls:raise RuntimeError('보호막 실제 조회/좌표/Begin 실행 수 오류')
        if any(not item['native_calls'].get(f'{find[name]:08x}') for name in ('next','ftol','filter')):raise RuntimeError('보호막 실제 Next/CRT/true 필터 실행 누락')
        pre_count=sum(r[0]=='Pre' for r in selected)
        if item['native_calls'].get(f'{find["pre"]:08x}')!=2*pre_count or item['substitutions'].get('C')!=2*pre_count:
            raise RuntimeError('보호막 연결 preDestroy/Carrier 실행 수 오류')
        if set(item['substitutions'])!={'D','C'} or any(r[0]=='Find' and r[7]!='-' for r in selected):raise RuntimeError('직접 조회에 외부 대체가 포함됨')
        for marker,count in item['substitutions'].items():
            if count!=2*sum(sum(part.startswith(marker+':') for part in r[7].split(';')) for r in selected):raise RuntimeError('보호막 외부 대체 수 오류')
    print(f'priestforcefield 검증 통과: {len(rows)}개')


def main():
    """전체 생성/무저장 표본 검사/저장 증거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
