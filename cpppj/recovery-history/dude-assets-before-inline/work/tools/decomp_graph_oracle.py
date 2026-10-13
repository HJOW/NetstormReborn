#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""정수 표면 스냅샷의 그래프 할당·flood·병합을 두 원본의 실제 x86으로 검증한다."""
import argparse
import collections
import csv
import hashlib
import json
import struct
import zlib
from pathlib import Path
from decomp_surface_oracle import SurfaceOracle, node, packed, POOL
from decomp_oracle import ROOT
from unicorn.x86_const import UC_X86_REG_FPCW

# 기존 표면 메모리와 겹치지 않는 그래프 표/4096 DWORD 스택의 격리 주소다.
TABLE, FLOOD = 0x11000000, 0x11002000
# 함수 대응과 그래프 표/스택/한계/무효 번호/로그 수준 전역이다.
SPECS = {
 'originals':dict(entries=dict(Add=0x4633c0,Flood=0x462f10,Allocate=0x463330,Free=0x462e50,Remove=0x462a50),
   table=0x562c18,stack=0x562c20,maximum=0x54040c,invalid=0x540410,log=0x542658),
 'originalCD':dict(entries=dict(Add=0x45b7d0,Flood=0x45bb90,Allocate=0x45c390,Free=0x45c460,Remove=0x45b530),
   table=0x5207f0,stack=0x520800,maximum=0x5207ec,invalid=0x5207f4,log=0x5308cc),
}


class GraphOracle(SurfaceOracle):
    """표면 탐색도 실제 함수로 실행하며 assert·할당·로그 I/O는 대체하지 않는다."""
    def __init__(self, edition):
        """새 Ghidra 몸체를 추가하고 허용 쓰기/함수를 제한한다."""
        super().__init__(edition)
        self.stubs = {}; self.spec = SPECS[edition]; self.facts = []; self.entries.update(self.spec['entries'])
        self.mu.mem_map(TABLE,0x10000)
        with (ROOT/f'extracted/graph/{edition}/functions.tsv').open(encoding='utf-8') as fp:
            # 몸체 사이의 미검토 코드는 허용하지 않는다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.facts.append(row)
                for part in row['ranges'].split(';'):
                    lo,hi=(int(v,16) for v in part.split('-')); self.allowed.append((lo,hi+1))
        self.write_ranges.extend([(TABLE,TABLE+255*6),(FLOOD,FLOOD+4096*4),(POOL,POOL+32*self.squid_stride)])
        # 이미 확보한 표/스택을 주어 malloc 분기는 실행하지 않는다.
        for address,value in [(self.spec['table'],TABLE),(self.spec['stack'],FLOOD),
                              (self.spec['maximum'],251),(self.spec['invalid'],254),(self.spec['log'],0)]:
            self.mu.mem_write(address,struct.pack('<I',value))

    def prepare(self,objects,spots,members,records,control):
        """합성 입력을 준비하며 그래프 결과는 Python에서 계산하지 않는다."""
        self.setup(objects,spots,()); self.members=members
        self.mu.mem_write(TABLE,bytes(records)); self.mu.mem_write(FLOOD,bytes(4096*4))
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        # raw 그래프 byte와 state를 원본 슬롯에 넣는다.
        for oid,graph,state in members:
            self.mu.mem_write(POOL+oid*self.squid_stride+(30 if self.edition=='originals' else 28),bytes([graph]))
            self.mu.mem_write(POOL+oid*self.squid_stride+11,bytes([state]))

    def step(self,name,arg):
        """원본 호출 규약·정상 반환과 표/슬롯/전체 스택을 관찰한다."""
        if name=='Remove': value=self.call(name,[],TABLE+arg*6)
        else: value=self.call(name,arg if isinstance(arg,list) else ([] if name=='Allocate' else [arg]))
        if name=='Allocate': value=(value-TABLE)//6
        if name not in ('Allocate','Flood'): value='-'
        members=[(oid,self.mu.mem_read(POOL+oid*self.squid_stride+(30 if self.edition=='originals' else 28),1)[0],
                  self.mu.mem_read(POOL+oid*self.squid_stride+11,1)[0]) for oid,_,_ in self.members]
        return [value,zlib.adler32(self.mu.mem_read(TABLE,255*6)),zlib.adler32(self.mu.mem_read(FLOOD,4096*4)),packed(members)]


def records_for(members,seed):
    """그래프 개수/사용 여부와 보존할 세 번째 WORD를 입력으로만 만든다."""
    records=[[0,0,(seed+i*7)&0xffff] for i in range(255)]
    # 각 입력 membership의 개수를 기존 표에 넣는다.
    for _,graph,_ in members:
        if graph!=254: records[graph][0]+=1; records[graph][1]=1
    records[254][:2]=[0x7dfd,0x7dfd]
    return b''.join(struct.pack('<3H',*r) for r in records)


def generate():
    """두 판본/정밀도에서 생긴 결과를 판본별로 저장한다."""
    rows=['# 기존 표/스택·정수 표면·소진 전. 모든 탐색/그래프는 실제 x86, 대체 함수 없음.']
    counts=collections.Counter(); reports={}
    # 같은 입력을 두 판본×53/64비트 x87에서 실행한다.
    for edition in SPECS:
        oracle=GraphOracle(edition)
        for control in (0x027f,0x037f):
            rows.append(f'Begin\t{edition}\t{control}')
            # 체인/고리/떨어진 무리/여러 칸/내부/죽음과 동률/서로 다른 기존 그래프를 섞는다.
            for case in range(128):
                mode=case%4
                coords=([(20+i,20) for i in range(7)] if mode==0 else
                        [(20,20),(19,20),(21,20),(20,19),(20,21),(19,19),(21,21)] if mode==1 else
                        [(20,20),(21,20),(21,21),(20,21),(30,30),(31,30),(32,30)] if mode==2 else
                        [(20,20),(18,20),(21,20),(20,19),(20,21),(30,30),(31,30)])
                objects=[node(i+1,x,y,side=('A','F','G','L','M','N','O','P')[(i+case)%8] if case%6==0 and i%2==0 else 'A',
                              flags=4 if case%6==0 and i%2==0 else 8,width=3 if mode==3 and i==0 else 1,dead=int(i==6 and case%7==0))
                         for i,(x,y) in enumerate(coords)]
                spots=[(21,20,8)] if case%11==0 else []
                members=[(i+1,254 if case%5==0 else (i+case)%3,2 if obj[10] else 0) for i,obj in enumerate(objects)]
                if case%17==0: members[0]=(1,members[0][1],4)
                if case%19==0: objects[0]=node(1,0,20,flags=8); members[0]=(1,members[0][1],0)
                records=bytearray(records_for(members,case*11))
                if case%23==1: struct.pack_into('<h',records,0,32767)
                oracle.prepare(objects,spots,members,records,control)
                rows.append('\t'.join(['Setup',packed(objects),packed(spots),packed(members),records.hex()]))
                # 재등록/이동이 아닌 원본 그래프 단계의 순서다. 무효 번호로 flood/free하는 경로도 포함한다.
                target=(0,1,2,254)[case%4]
                for name,args in [('Add',[1]),('Flood',[1,target]),('Remove',target if target!=254 else 0),('Free',target),('Allocate',[])]:
                    # Remove는 유효 그래프에 대한 직접 helper이며 무효 번호는 Free에서 검사한다.
                    rows.append('\t'.join(map(str,[name,packed([args]) if isinstance(args,list) and args else str(args) if isinstance(args,int) else '-',*oracle.step(name,args)])))
                    counts[name]+=1
        reports[edition]=dict(binary_sha256=oracle.sha256,function_ranges=oracle.facts,assert_calls=oracle.assertions)
        print(f'{edition}: 그래프 실제 x86 완료',flush=True)
    fixture=ROOT/'cpppj/tests/fixtures/graph-x86.tsv'; fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    deps=('decomp_graph_oracle.py','decomp_oracle.py','decomp_bridge_oracle.py','decomp_surface_oracle.py')
    report=dict(schema=1,public_calls=dict(counts),total_calls=sum(counts.values()),sequences=4,editions=reports,stubbed=[],
                fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
                sources={n:hashlib.sha256((ROOT/'tools'/n).read_bytes()).hexdigest() for n in deps},
                limitations=['기존 표/스택·정수 표면 스냅샷·소진 전; graph 복구/삭제 분할/영역 통지/postPop/raw 월드 연결 미검증',
                             '표/전체 스택은 Adler-32, membership graph/state는 모든 항목 직접 비교'])
    (ROOT/'cpppj/recovery-graph-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'graph x86 fixture: {report["total_calls"]}회 {dict(counts)}')


def verify():
    """게임 실행 없이 원본/도구/기대값 SHA와 호출 수를 확인한다."""
    r=json.loads((ROOT/'cpppj/recovery-graph-evidence.json').read_text(encoding='utf-8'))
    for ed,data in r['editions'].items():
        p=ROOT/('originals/Netstorm.exe' if ed=='originals' else 'originalCD/NETSTORM.EXE')
        assert data['binary_sha256']==hashlib.sha256(p.read_bytes()).hexdigest() and data['assert_calls']==0
    for n,digest in r['sources'].items(): assert digest==hashlib.sha256((ROOT/'tools'/n).read_bytes()).hexdigest()
    p=ROOT/'cpppj/tests/fixtures/graph-x86.tsv'; assert r['fixture_sha256']==hashlib.sha256(p.read_bytes()).hexdigest()
    counts=collections.Counter(l.split('\t')[0] for l in p.read_text(encoding='utf-8').splitlines())
    assert {k:counts[k] for k in r['public_calls']}==r['public_calls']
    assert sum(r['public_calls'].values())==r['total_calls'] and r['stubbed']==[]
    print(f'graph 근거 SHA/호출 수 확인: {r["total_calls"]}회')


if __name__=='__main__':
    # --verify는 새 기계어를 실행하지 않는다.
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify',action='store_true'); args=parser.parse_args()
    verify() if args.verify else generate()
