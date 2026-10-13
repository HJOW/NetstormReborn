#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""같은 실제 표면 alias/spot/타입/프레임의 BFS·경로 축약을 두 보호 PE 기계어로 실행한다."""
import argparse
import collections
import csv
import json
import struct
from decomp_pathruntime_oracle import PathOracle,PATH,dbits,fnv
from decomp_reward_oracle import ROOT,ARENA,TYPES,STACK,STOP,digest,bits
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# BFS의 별도 격리 메모리다. 실제 hash0와 SurfaceAt alias는 둘 다 같은 MAP을 가리킨다.
DISTANCE,QUEUE,MAP,RAWPOOL,CODES=ARENA+0x60000,ARENA+0x80000,ARENA+0xa0000,ARENA+0xc0000,ARENA+0xc8000
# 원본 lazy 배열/현재 정책/표면/Graph guard 전역이다.
SPEC={
 'originals':dict(search=0x488af0,kernel=0x488370,work=0x59514c,queue=0x595150,map=0x5c84bc,alias=0x542514,graphs=0x540414,graphbase=0x562c18,
    enabled=0x540be4,avoid=0x540be8,policy=0x5318ec,mask=0x540c24,order=0x540c44,counter=0x460d70),
 'originalCD':dict(search=0x402100,kernel=0x402750,work=0x50f308,queue=0x50f30c,map=0x52d590,alias=0x5670cc,graphs=0x5207f8,graphbase=0x5207f0,
    enabled=0x50f280,avoid=0x50f284,policy=None,mask=0x50f2c8,order=0x50f2e8,counter=0x4011b0),
}
# 부모/표면의 입력 타입과 raw frame 코드다. 알고리즘 결과를 미리 계산하지 않는다.
TYPE_INPUTS={82:4,83:2,84:0x40000}
FRAME_INPUTS=[('A','A'),('A','P'),('M','A'),('P','P'),('K','A'),('N','A')]
FIXTURE=ROOT/'cpppj/tests/fixtures/pathsearch-x86.tsv'
EVIDENCE=ROOT/'cpppj/pathsearch.json'
SOURCES=['tools/decomp_pathsearch_oracle.py','tools/decomp_pathruntime_oracle.py','tools/decomp_reward_oracle.py','tools/ghidra/pathsearch-functions.json',
 'cpppj/src/o/RawPathSearch.h','cpppj/src/o/RawPathSearch.cpp','cpppj/src/o/Bridge.cpp','cpppj/tests/RawPathSearchTests.cpp']


def scene(profile,owner):
    """장애물/동일 SID/프레임 접합/Graph guard만 입력하며 경로를 계산하지 않는다."""
    nodes=[];cells=[];side=1 if profile==4 else 3 if profile==5 else 5;next_id=6
    # 같은 보드 셀마다 실제 raw 타입/프레임/Graph/owner/flag 입력을 공급한다.
    for y in range(10,10+side):
        for x in range(10,10+side):
            spot=6;type_=83;frame=0;flag=0;graph=3;sid=6 if profile==5 else next_id
            if profile==1 and (x,y) in ((11,11),(12,12),(13,13)):spot=8
            if profile==2 and x==12:spot=0
            if profile==3 and (x+y)%3==0:flag=2
            if profile==6 and x==12 and y<14:type_=82;frame=4
            if profile==7 and (x,y)==(12,12):type_=84
            if profile==8 and (x,y)==(14,14):graph=254
            if profile==9 and (x,y)==(14,14):graph=4
            cells.append((x,y,sid,spot))
            if profile!=5 or not nodes:nodes.append((sid,type_,flag,owner,graph,x,y,frame))
            next_id+=1
    # 실제 wrapper 입력은 소수 좌표를 거의 올림하며 시작/끝을 뒤집어 BFS한다.
    start=(9.2,9.3);destination=(9.2+side-1,9.3+side-1)
    return nodes,cells,start,destination


class SearchOracle(PathOracle):
    """기존 보호 PE 로더와 primitive body만 재사용하고 새 BFS/접합/Graph 몸체를 추가한다."""
    def __init__(self,edition):
        """별도 작업 배열과 실제 함수 범위/허용 쓰기를 준비한다."""
        super().__init__(edition);self.search_spec=SPEC[edition];self.mu.mem_map(ARENA+0x60000,0x70000)
        with (ROOT/f'extracted/pathsearch/{edition}/functions.tsv').open(encoding='utf-8') as stream:
            # 고유 정밀 함수 범위 밖 실행은 기존 도구처럼 즉시 실패한다.
            for row in csv.DictReader(stream,delimiter='\t'):
                self.entry_names[int(row['entry'],16)]=row['entry']
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.external.add(self.search_spec['counter']);self.writes.extend([(ARENA+0x60000,ARENA+0xd0000),(self.search_spec['mask'],self.search_spec['mask']+32)])

    def on_instruction(self,mu,address,size,data):
        """경로 계산과 접합은 원본이 수행한다. 프로파일링 시계만 명시적으로 0을 입력한다."""
        if address!=self.search_spec['counter']:return super().on_instruction(mu,address,size,data)
        sp=mu.reg_read(UC_X86_REG_ESP);ret=self.u32(sp);output=self.u32(sp+4)
        mu.mem_write(output,bytes(8));mu.reg_write(UC_X86_REG_EAX,output);mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,ret)
        self.stub_hits['profiling_clock']+=1

    def setup(self,profile,owner,avoid,policy,legacy,guard):
        """입력 장면을 same pool/hash0/spot/현재 프레임에 기록한다."""
        s=self.search_spec;m=self.m;nodes,cells,start,destination=scene(profile,owner)
        self.mu.mem_write(PATH,bytes(m['size']));self.mu.mem_write(RAWPOOL,bytes(0x10000));self.mu.mem_write(MAP,bytes(0x20000));self.mu.mem_write(DISTANCE,bytes(0x28000));self.mu.mem_write(ARENA+0x40000,bytes(65536))
        self.put(self.s['pool_ptr'],RAWPOOL);self.put(s['map'],MAP);self.put(s['alias'],MAP);self.put(s['work'],DISTANCE);self.put(s['queue'],QUEUE)
        self.put(m['spots'],ARENA+0x40000);self.put(m['typeptr'],TYPES);self.put(s['graphs'],0 if guard==1 else 1);self.put(s['graphbase'],ARENA+0x1d000)
        self.put(s['enabled'],0 if guard==2 else 1);self.put(s['avoid'],avoid)
        if s['policy']:self.put(s['policy'],policy)
        # 원본 mutable 방향 전역의 정상 값은 PE 초기값과 같으며 정책 분기의 변경은 원본이 한다.
        for index,value in enumerate((0,5,0,20,0,80,0,65)):self.put(s['mask']+index*4,value)
        for index,value in enumerate((0,2,4,6,1,3,5,7)):self.put(s['order']+index*4,value)
        # 정적 프레임 표는 모든 표면 타입에서 같은 입력 코드 목록을 쓴다.
        for number,genus in TYPE_INPUTS.items():
            address=TYPES+number*self.s['type_stride'];self.put(address+0xe8,0x800);self.put(address+0xec,genus);self.put(address+0x124,CODES);self.put(address+0x114,len(FRAME_INPUTS))
        for index,(side,variant) in enumerate(FRAME_INPUTS):self.put(CODES+index*4,ord(side),1);self.put(CODES+index*4+1,ord(variant),1)
        # raw slots를 실제 판본의 Graph/owner/frame 폭대로 쓴다.
        for sid,type_,flag,owner_,graph,x,y,frame in nodes:
            raw=RAWPOOL+sid*self.s['stride'];self.put(raw+10,type_,1);self.put(raw+12,flag,1);self.put(raw+14,bits(x));self.put(raw+18,bits(y));self.put(raw+(30 if self.patch else 28),graph,1)
            self.put(raw+(34 if self.patch else 32),owner_,1);self.put(raw+(36 if self.patch else 34),frame,4 if self.patch else 1)
        for x,y,sid,spot in cells:self.put(MAP+(y*256+x)*2,sid,2);self.put(ARENA+0x40000+y*256+x,spot,1)
        self.put(RAWPOOL+5*self.s['stride']+10,158 if owner%2==0 else 134,1)
        # 기존 경로 append도 원본 입력으로 제공하며 수정하지 않은 이전 double 시각을 대조한다.
        if legacy:self.path([(9,9)],[1],[12.5])
        return nodes,cells,start,destination

    def execute(self,args):
        """wrapper는 cdecl이며 계산 종료/스택/x87을 확인한다."""
        return self.invoke(self.search_spec['search'],args)

    def observed(self):
        """원본 꼭짓점/방향/double 시각과 전체 거리/큐 byte를 관찰한다."""
        count=self.u32(PATH+0x10);rows=[]
        for index in range(count):rows.append(','.join(map(str,[self.u32(PATH+0x14+index*8),self.u32(PATH+0x18+index*8),self.u32(PATH+self.m['directions']+index*4),struct.unpack('<Q',self.mu.mem_read(PATH+self.m['times']+index*8,8))[0]])))
        return ';'.join(rows) or '-',fnv(self.mu.mem_read(DISTANCE,0x20000)),fnv(self.mu.mem_read(QUEUE,0x8000)),self.u32(self.search_spec['mask']+12)


def cases():
    """검색 분기와 비용/소유자/기존 경로 입력을 교차한다."""
    for profile in range(10):
        for flags in (0,1):
            for avoid in (0,1):
                for policy in (5,6):
                    for owner in (1,2):
                        for legacy in (0,1):yield profile,flags,avoid,policy,owner,legacy,0
    # Graph 비활성은 실제 자연 반환이다. kenGo 비활성 assert는 별도 진단 입력으로 보존한다.
    for guard in (1,2):yield 0,0,1,6,1,0,guard


def generate():
    """출력을 구현으로 계산하지 않고 보호 PE의 실제 검색/접합/역추적을 실행한다."""
    rows=[];hits=collections.Counter();external=collections.Counter();edition_counts={}
    for edition in SPEC:
        oracle=SearchOracle(edition);count=0
        # 모든 장면은 원본 함수의 관찰만 저장한다.
        for profile,flags,avoid,policy,owner,legacy,guard in cases():
            nodes,cells,start,destination=oracle.setup(profile,owner,avoid,policy,legacy,guard)
            args=[PATH+0x10,bits(start[0]),bits(start[1]),bits(destination[0]),bits(destination[1]),flags,5]
            try:result=oracle.execute(args);assertions=0
            except RuntimeError as error:
                if guard!=2 or not str(error).startswith('원본 assert'):raise
                result=0;assertions=1
            points,distance,queue,mask=oracle.observed()
            rows.append([edition,profile,flags,avoid,policy,owner,legacy,guard,result,assertions,points,distance,queue,mask]);count+=1
        hits.update(oracle.hits);external.update(oracle.stub_hits);edition_counts[edition]=count;print(f'{edition} 실제 BFS/축약 {count}개',flush=True)
    FIXTURE.write_text('# edition profile flags avoid policy owner legacy guard result assert points distanceFNV queueFNV mask3\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files=list(SOURCES)
    for edition in SPEC:
        files.extend([f'{edition}/{"Netstorm.exe" if edition=="originals" else "NETSTORM.EXE"}',f'extracted/pathsearch/{edition}/creation.c',f'extracted/pathsearch/{edition}/functions.tsv'])
        # 기존 precise getter/ftol body를 재사용하며 새 관찰 수에 기존 입력을 더하지 않는다.
        for name in ('pathruntime','pathruntimehelpers','pathmath'):files.extend([f'extracted/{name}/{edition}/creation.c',f'extracted/{name}/{edition}/functions.tsv'])
    report=dict(host='VM-W11-CODEX',date='2026-10-13',cases=len(rows),edition_cases=edition_counts,entries=dict(hits),external=dict(external),fixture_sha256=digest(FIXTURE),files={path:digest(ROOT/path) for path in files},
        limits=['실제 BFS/표면 Graph getter/접합/거리/비용/축약을 실행한다. 프로파일링 시계만 대체한다.','보드 가장자리의 원본 손상 포인터/queue 읽기 넘침은 정상 입력에 포함하지 않는다.'])
    EVIDENCE.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """PE/source/body SHA와 유일한 전체 입력을 감사한다."""
    report=json.loads(EVIDENCE.read_text('utf-8'));assert digest(FIXTURE)==report['fixture_sha256']
    for path,expected in report['files'].items():assert digest(ROOT/path)==expected,path
    rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==report['cases']==len(list(cases()))*2;assert len({tuple(row[:8]) for row in rows})==len(rows)
    assert collections.Counter(row[0] for row in rows)==report['edition_cases'];print(f'실제 BFS/축약 {len(rows)}개 SHA/입력 감사 통과')


def main():
    """새 관찰 생성 또는 기존 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()


if __name__=='__main__':main()
