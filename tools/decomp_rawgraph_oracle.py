#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 SID 풀·0단계 해시·프레임·postPop/Pop을 연결한 그래프 x86 대조다."""
import argparse
import collections
import csv
import hashlib
import json
import struct
import zlib
from pathlib import Path
from decomp_postpop_oracle import PostOracle
from decomp_graph_oracle import SPECS, records_for
from decomp_surface_oracle import HELPERS, PAIRS, packed
from decomp_pop_oracle import HEADS, SPOTS, SHAPE, bits
from decomp_creation_oracle import TYPES
from decomp_sid_oracle import POOL
from decomp_oracle import ROOT
from unicorn.x86_const import UC_X86_REG_EAX

# 기존 SID/지도/표시/Player 공간과 겹치지 않는 표·스택·실제 프레임 코드 입력이다.
TABLE,FLOOD,CODES=0x13000000,0x13002000,0x13008000


class RawGraphOracle(PostOracle):
    """기존 엄격한 raw 공간 검증에 실제 그래프/표면 몸체만 추가한다."""
    def __init__(self,edition):
        """Ghidra 몸체 범위와 격리 주소를 등록하며 대체 함수를 사용하지 않는다."""
        super().__init__(edition); self.graph=SPECS[edition]; self.graph_calls=collections.Counter()
        self.mu.mem_map(TABLE,0x10000)
        paths=[ROOT/f'extracted/graph/{edition}/functions.tsv']
        with (ROOT/f'extracted/refined/{edition}/functions.tsv').open(encoding='utf-8') as fp:
            facts={int(r['entry'],16):r for r in csv.DictReader(fp,delimiter='\t')}
        wanted=HELPERS[edition]+[int(p[edition],16) for p in PAIRS if p['name'] in ('Connect','Find','Next','Count')]
        additions=[facts[a] for a in wanted]
        # 그래프에 도달하는 실제 helper 외의 코드는 허용하지 않는다.
        for path in paths:
            with path.open(encoding='utf-8') as fp: additions.extend(csv.DictReader(fp,delimiter='\t'))
        for row in additions:
            self.facts.append(row)
            for part in row['ranges'].split(';'):
                lo,hi=(int(v,16) for v in part.split('-')); self.allowed.append((lo,hi+1))

    def on_instruction(self,mu,address,size,data):
        """상위 Pop/postPop 안에 포함된 그래프 호출 수를 별도로 관찰한다."""
        if hasattr(self,'graph'):
            for name,entry in self.graph['entries'].items():
                if address==entry: self.graph_calls[name]+=1
        super().on_instruction(mu,address,size,data)

    def start_graph(self,control):
        """실제 Reset/Create로 7개 SID를 확보하고 표/스택만 추가 허용한다."""
        self.start_post(control); self.ids=[self.sid]
        # 첫 SID는 부모 start가 실제 생성했으며 추가 여섯 개도 실제 base Create를 사용한다.
        for _ in range(6): self.ids.append(self.creation_step('Create',74,2)[0])
        self.write_ranges.extend([(TABLE,TABLE+255*6),(FLOOD,FLOOD+4096*4)])
        # 기존 확보 표/스택·정상 한계·로그 조기 반환 경로를 입력한다.
        for key,value in [('table',TABLE),('stack',FLOOD),('maximum',251),('invalid',254),('log',0)]:
            self.mu.mem_write(self.graph[key],struct.pack('<I',value))

    def prepare_graph(self,nodes,spots,records,stack):
        """실제 할당 슬롯에 입력만 쓰며 그래프 정답을 Python에서 계산하지 않는다."""
        self.state(0,0,0,0,53,0,31337,1,1,0,5)
        self.mu.mem_write(HEADS,bytes(86272*2)); self.mu.mem_write(SPOTS,bytes(65536))
        for i,(x,y,width,height,f1,f2,side,variant,frame,state,extra,graph,level) in enumerate(nodes):
            number=74+i; self.input_type(number,f1,f2,1.25)
            typ=TYPES+number*self.creation['type_stride']
            self.mu.mem_write(typ+self.space['foot'],struct.pack('<2I',width,height))
            self.mu.mem_write(typ+0x114,struct.pack('<I',2))
            self.mu.mem_write(typ+0x124,struct.pack('<I',CODES+i*8))
            self.mu.mem_write(CODES+i*8,bytes([side,variant,1,0,ord('A'),ord('P'),2,0]))
            address=POOL+self.ids[i]*self.stride
            self.mu.mem_write(address,bytes(self.stride)); self.mu.mem_write(address,struct.pack('<I',self.creation['vtable']))
            self.mu.mem_write(address+8,struct.pack('<H',23)); self.mu.mem_write(address+10,bytes([number,state]))
            self.mu.mem_write(address+14,struct.pack('<2f',x,y))
            self.mu.mem_write(address+(30 if self.edition=='originals' else 28),bytes([graph]))
            self.mu.mem_write(address+self.space['level'],bytes([level]))
            self.mu.mem_write(address+(34 if self.edition=='originals' else 32),bytes([1]))
            self.mu.mem_write(address+self.space['frame'],struct.pack('<I',frame) if self.edition=='originals' else bytes([frame]))
            self.mu.mem_write(address+self.space['extra'],bytes([extra]))
            if not state&4 and level==0: self.mu.mem_write(self.bases[0]+(y*256+x)*2,struct.pack('<H',self.ids[i]))
        # 공유 SHP 입력의 물리 4프레임을 지정한다. FrameCode 표와 별도다.
        for i in range(4):
            offset=0x100+i*0x80; metrics=(11,17,12,16) if i%2==0 else (19,23,-3,5)
            self.mu.mem_write(SHAPE+8+i*8,struct.pack('<I',offset))
            self.mu.mem_write(SHAPE+offset-36,struct.pack('<2f',1,1)); self.mu.mem_write(SHAPE+offset-12,struct.pack('<4h',*metrics))
        for x,y,value in spots: self.mu.mem_write(SPOTS+y*256+x,bytes([value]))
        self.mu.mem_write(TABLE,records); self.mu.mem_write(FLOOD,stack)

    def graph_output(self,result):
        """전체 raw 슬롯과 그래프/스택·공간/통계 전체 체크섬·dirty 항목을 관찰한다."""
        p=self.post
        raw=';'.join(bytes(self.mu.mem_read(POOL+sid*self.stride,self.stride)).hex() for sid in self.ids)
        hashes=[zlib.adler32(self.mu.mem_read(a,size)) for a,size in
                [(TABLE,255*6),(FLOOD,4096*4),(HEADS,86272*2),(SPOTS,65536),(POOL,self.capacity*self.stride)]]
        cost=struct.unpack('<i',self.mu.mem_read(p['cost'],4))[0]
        depth=struct.unpack('<I',self.mu.mem_read(self.space['depth'],4))[0]
        counts=[zlib.adler32(self.mu.mem_read(a,1024)) for a in p['counts']]
        return [result,raw,*hashes,cost,depth,*counts,*self.dirty_output()]

    def graph_step(self,name,arg):
        """원본 호출 규약과 x87·정상 반환을 검사하고 관찰값만 돌려준다."""
        result='-'
        if name in ('PostPop','Pop'):
            args=arg if name=='PostPop' else [bits(arg[1]),bits(arg[2]),arg[0]]
            self.invoke(self.space['post'] if name=='PostPop' else self.space['pop'],args,POOL+self.ids[0]*self.stride,4 if name=='PostPop' else 12)
        else:
            self.invoke(self.graph['entries'][name],[self.ids[0]]+arg,0,0)
            if name=='Flood': result=self.mu.reg_read(UC_X86_REG_EAX)
        return self.graph_output(result)


def generate():
    """정수 raw 표면·프레임 변경·일반 표면/매몰 다리 Pop을 네 시퀀스로 검증한다."""
    rows=['# 실제 SID/해시/spot/FrameCode·공통 postPop/Pop/표시 활성; AI 없음·소진 전·대체 함수 없음.']
    counts=collections.Counter(); reports={}
    for edition in SPECS:
        oracle=RawGraphOracle(edition)
        for control in (0x027f,0x037f):
            oracle.start_graph(control); rows.append(f'Begin\t{edition}\t{control}\t'+','.join(map(str,oracle.ids)))
            # 떨어진 무리·고리·동률 병합·다중 칸·죽은/비표면 후보·실제 프레임 번호를 섞는다.
            for case in range(64):
                mode=case%4
                coords=([(20,20),(19,20),(21,20),(20,19),(20,21),(18,20),(30,30)] if mode!=3 else
                        [(20,20),(18,20),(21,20),(20,19),(20,21),(17,20),(30,30)])
                nodes=[]
                for i,(x,y) in enumerate(coords):
                    f1=0 if i==6 and case%5==0 else 0x800
                    f2=0 if i==0 and mode in (1,3) else (4 if i%2 else 2)
                    state=4 if i==0 and mode in (2,3) else (2 if i==4 and case%7==0 else 0)
                    graph=254 if i==0 or case%5==0 else i%3
                    nodes.append([x,y,3 if i==0 and mode==3 else 1,1,f1,f2,
                                  ord('L') if i==1 and case%6==0 else ord('A'),ord('P'),(i+case)%2,state,
                                  8 if i==0 and mode==2 else 0,graph,1 if i==0 and mode in (1,3) else 0])
                spots=[(21,20,8)] if case%11==0 else []
                members=[(oracle.ids[i],n[11],n[9]) for i,n in enumerate(nodes) if n[4]&0x800]
                records=records_for(members,case*11)
                # 스택의 오래된 값을 넣어 매 스냅샷 초기화 오류도 잡는다.
                stack=struct.pack('<4096I',*((case*17+i)&0xffffffff for i in range(4096)))
                oracle.prepare_graph(nodes,spots,records,stack)
                rows.append('\t'.join(['Setup',packed(nodes),packed(spots),records.hex(),stack.hex()]))
                steps=([('Pop',[0x200,20,20])] if mode in (2,3) else [('PostPop',[1])])
                steps += [('PostPop',[0x200]),('Flood',[254]),('Add',[]),('PostPop',[0x2000001]),('Add',[])]
                for name,arg in steps:
                    rows.append('\t'.join(map(str,[name,','.join(map(str,arg)) or '-',*oracle.graph_step(name,arg)]))); counts[name]+=1
        reports[edition]=dict(binary_sha256=oracle.sha256,function_ranges=oracle.facts,assert_calls=oracle.assertions,
            graph_calls=dict(oracle.graph_calls),lifecycle_calls=dict(oracle.actual_calls),display_calls=dict(oracle.display_calls),
            cost_encode_segments=oracle.encodings,preparation_calls=dict(Reset=2,Create=14))
        print(f'{edition}: raw 그래프 실제 x86 완료',flush=True)
    fixture=ROOT/'cpppj/tests/fixtures/rawgraph-x86.tsv'; fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    deps=('decomp_oracle.py','decomp_sid_oracle.py','decomp_creation_oracle.py','decomp_pop_oracle.py',
          'decomp_display_oracle.py','decomp_postpop_oracle.py','decomp_graph_oracle.py','decomp_surface_oracle.py','decomp_bridge_oracle.py')
    report=dict(schema=1,public_calls=dict(counts),total_calls=sum(counts.values()),sequences=4,editions=reports,stubbed=[],
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
        dependencies={n:hashlib.sha256((ROOT/'tools'/n).read_bytes()).hexdigest() for n in deps},
        limitations=['합성 타입/FrameCode/SHP·기존 확보 풀/표·정수 좌표·AI 없음·배치 선택 없음·소진 전',
          '일반 표면과 매몰 다리 Pop만 연결; 정상 다리/섬 Pop의 영역 효과·삭제 분할/소진 복구·GameWorld 미연결',
          '7개 raw 슬롯/dirty 모든 항목 직접 비교; 전체 풀/해시/spot/그래프/스택/통계는 Adler-32',
          '비용 인코딩은 loader 접두 구간이며 준비 Reset/Create 및 내부 graph 호출은 공개 호출 수와 구별'])
    (ROOT/'cpppj/recovery-rawgraph-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'raw 그래프 x86 fixture: {report["total_calls"]}회 {dict(counts)}')


def verify():
    """게임 실행 없이 PE/부모 도구/fixture SHA와 공개 호출 수를 확인한다."""
    r=json.loads((ROOT/'cpppj/recovery-rawgraph-evidence.json').read_text(encoding='utf-8'))
    assert r['tool_sha256']==hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    for edition,data in r['editions'].items():
        p=ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE')
        assert data['binary_sha256']==hashlib.sha256(p.read_bytes()).hexdigest() and data['assert_calls']==0
    for name,digest in r['dependencies'].items(): assert digest==hashlib.sha256((ROOT/'tools'/name).read_bytes()).hexdigest()
    fixture=ROOT/'cpppj/tests/fixtures/rawgraph-x86.tsv'; assert r['fixture_sha256']==hashlib.sha256(fixture.read_bytes()).hexdigest()
    counts=collections.Counter(line.split('\t')[0] for line in fixture.read_text(encoding='utf-8').splitlines())
    assert all(counts[n]==v for n,v in r['public_calls'].items()) and sum(r['public_calls'].values())==r['total_calls'] and r['stubbed']==[]
    print(f'raw 그래프 근거 SHA/호출 수 확인: {r["total_calls"]}회')


if __name__=='__main__':
    # --verify는 새 에뮬레이션 없이 기존 근거만 검증한다.
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify',action='store_true'); args=parser.parse_args()
    verify() if args.verify else generate()
