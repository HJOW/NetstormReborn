#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""공통 삭제의 Graph 분할/Free와 주변 Add를 세 PE에서 격리 실행한다. 게임/OS/창 실행 없음."""
import argparse
import collections
import csv
import json
import struct
import zlib
from pathlib import Path

from decomp_destroylifecycle_oracle import LifecycleOracle,list_input
from decomp_destroy_oracle import VTABLE,CONTROLS,sha
from decomp_graphremove_oracle import BINARIES,DEPENDENCIES
from decomp_rawgraph_oracle import TABLE,FLOOD,CODES
from decomp_graph_oracle import records_for
from decomp_postpop_oracle import PostOracle,ARENA,PROVIDERS,FACTORIES,OWNERS
from decomp_creation_oracle import TYPES
from decomp_sid_oracle import POOL,SidOracle
from decomp_pop_oracle import bits
from decomp_surface_oracle import packed
from decomp_oracle import ROOT

# 삭제 전후 특수 타입/영역 전역과 실제 공간 탐색 진입점이다.
SPECS={
 'originals':dict(region=0x5424b8,special=0x5412cc,begin=0x4b16d0,next=0x4b1810,footprint=0x4ade70),
 'originalCD':dict(region=0x5395cc,special=0x51cbb8,begin=0x4eae20,next=0x4eafe0,footprint=0x4aeec0)}
# 입력은 세 공개 경로 각각 96개, 세 실제 PE와 두 x87 정밀도에서 실행한다.
EXPECTED_CASES={'Pre':576,'Post':576,'Destroy':576}
# 각 PE 내부 도달 수다. 공개 입력 1,728개에 내부 호출을 더하지 않는다.
EXPECTED_NATIVE={'Destroy':192,'Unpop':180,'Release':192}
EXPECTED_LIFECYCLE={'Pre':384,'Post':384,'ai':168}
EXPECTED_GRAPH={'Detach':220,'Begin':480,'Next':1280,'Footprint':2238,'Allocate':504,'Flood':512,'Free':20,'Add':360}
EXPECTED_SUBSTITUTIONS={'selected':576,'clear':192,'log':1064}
FIXTURE=ROOT/'cpppj/tests/fixtures/destroygraph-x86.tsv'
REPORT=ROOT/'cpppj/recovery-destroygraph-evidence.json'


def case_input(case,kind):
    """분할/직접 해제/전후 억제/특수 프레임/발자국/머리 순서를 합성 입력으로만 만든다."""
    mode=case%16;block=case//16
    x,y=(1,1) if mode==1 else (255,255) if mode==12 else (20,20)
    coords=[(x,y),(max(1,x-1),y),(min(255,x+1),y),(x,max(1,y-1)),(x,min(255,y+1)),(max(1,x-2),y),(30,30)]
    if mode==13:coords[1]=(x,y)
    if mode==14:coords=[(20,20),(18,18),(19,18),(20,18),(18,19),(19,19),(20,19)]
    number=157 if mode in (8,9) else 162 if mode==10 else 74
    flags=0x200000 | (0x2000000 if mode==5 else 0x2000800 if mode==4 else 0)
    if block==5:flags&=~0x200000
    root_f2=4 if mode==11 else 8
    root_width=3 if mode in (10,12,14) else 1
    nodes=[]
    # 네 해시 단계와 동일 버킷의 역순 체인을 입력한다. 결과 순서는 계산하지 않는다.
    for i,(ax,ay) in enumerate(coords):
        f1=0 if (i==0 and mode==11) or (i==6 and mode!=14) else 0x800
        f2=root_f2 if i==0 else 4
        state=(2 if kind=='Pre' else 6 if kind=='Post' else 0) if i==0 else 0
        if i==0 and mode==11:state|=4
        if mode==15 and i in (2,3):state=2 if i==2 else 4
        graph=254 if mode==3 else (i%3 if mode==2 else 1 if mode==13 and i==1 else 0)
        side=ord('H') if i==0 and mode==8 else ord('A')
        extra=(8 if mode==6 else 1 if mode==7 else 0) if i==0 else (8 if mode==15 and i==4 else 0)
        nodes.append([ax,ay,root_width if i==0 else 1,root_width if i==0 else 1,f1,f2,side,ord('P'),0,state,extra,graph,0 if i==0 else (block+i)%4])
    # 초기 표의 reserved와 스택 꼬리는 사례마다 다르게 채운다.
    seed=case*17+10;members=[(5+i,n[11],n[9]) for i,n in enumerate(nodes) if n[4]&0x800]
    records=bytearray(records_for(members,case*11))
    if mode==10 and nodes[0][11]!=254:
        offset=nodes[0][11]*6;old=struct.unpack_from('<H',records,offset)[0];struct.pack_into('<H',records,offset,old+8)
    spots=[(nodes[4][0],nodes[4][1],8)] if mode==15 else []
    args=[number,nodes[0][4],root_f2,bits(1.25),10,1,1,nodes[0][10],flags,int(block%2==0),int(block%2==1),0,0,148,
          block,123,0xffffffff if block%2 else 0,int(bool(nodes[0][9]&4)),case*31337,
          int(block!=4),int(block%2==0),4 if mode==11 else 0x50444200,seed,3,8]
    return args,nodes,spots,bytes(records)


class DestroyGraphOracle(LifecycleOracle):
    """기존 장부/삭제 실행기에 실제 Graph 및 위치/발자국 helper만 더한다."""
    def __init__(self,binary):
        """새 불연속 몸체만 실행 허용하고 그래프 결과를 대체하지 않는다."""
        super().__init__(binary);self.dg=SPECS[self.edition];self.graph_native=collections.Counter()
        directory=ROOT/f'extracted/destroygraph/{binary}'
        self.body_paths.extend(directory/name for name in ('creation.c','functions.tsv'))
        with (directory/'functions.tsv').open(encoding='utf-8') as fp:
            # 함수 사이 주소를 허용 범위에 포함하지 않는다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.facts.append(row)
                # Ghidra 포함 끝 주소를 열린 끝 주소로 바꾼다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.graph_entries={self.delete[0]:'Detach',self.dg['begin']:'Begin',self.dg['next']:'Next',self.dg['footprint']:'Footprint'}
        self.graph_entries.update({entry:name for name,entry in self.graph['entries'].items()})

    def on_instruction(self,mu,address,size,data):
        """추가 실제 Graph 도달만 관찰하고 기존 외부 효과 경계를 유지한다."""
        if hasattr(self,'graph_entries') and address in self.graph_entries:self.graph_native[self.graph_entries[address]]+=1
        return super().on_instruction(mu,address,size,data)

    def prepare_case(self,args,nodes,spots,records):
        """한 풀/해시/타입/장부를 공유하는 입력을 준비한다. 원본 결과는 읽기만 한다."""
        self.prepare_destroy(True,0,args[8],0,'Destroy');self.sid=self.root
        self.prepare_delete(nodes,spots,records,args[22],args[20],1)
        PostOracle.state(self,6,6,6,0,args[15],args[16],args[18],args[6],args[19],args[10],3)
        self.mu.mem_write(VTABLE,bytes(self.table));self.selected_sid=self.root if args[9] else 0
        number=args[0];node=nodes[0];typ=TYPES+number*self.creation['type_stride']
        # 루트 타입 번호를 바꾸어도 raw 슬롯과 위치 정보의 실제 타입은 따로 보존한다.
        self.input_type(number,node[4],node[5],1.25)
        self.mu.mem_write(typ+self.space['foot'],struct.pack('<2I',node[2],node[3]))
        self.mu.mem_write(typ+0x114,struct.pack('<I',2));self.mu.mem_write(typ+0x124,struct.pack('<I',CODES))
        self.mu.mem_write(typ+0x9c,struct.pack('<I',args[4]));self.mu.mem_write(POOL+self.root*self.stride+10,bytes([number]))
        # H 프레임의 위치 정보만 162로 치환한다. 원본 slot의 type 바이트는 바꾸지 않는다.
        if number!=162:
            self.input_type(162,0x800,args[24],1.25)
            replacement=TYPES+162*self.creation['type_stride']
            self.mu.mem_write(replacement+self.space['foot'],struct.pack('<2I',args[23],args[23]))
            self.mu.mem_write(replacement+0x114,struct.pack('<I',2));self.mu.mem_write(replacement+0x124,struct.pack('<I',CODES+0x100))
            self.mu.mem_write(CODES+0x100,b'AP\x01\0AP\x02\0')
        for a,value in ((self.dg['region'],args[21]),(self.dg['special'],157),(self.delete[2],162)):
            self.mu.mem_write(a,struct.pack('<I',value))
        # 장부의 활성 항목과 inactive 꼬리를 별도로 채워 Graph 억제와 통계 억제를 구별한다.
        for pointer,count_address,base in [(PROVIDERS,self.post['providers'][2],0x60000000),
            (FACTORIES,self.post['factories'][2],0x61000000),
            *((OWNERS+i*0x100,ARENA+i*self.post['stride']+8,0x70000000+i*100) for i in range(9))]:
            count,values=list_input(args[14],base,self.root)
            self.mu.mem_write(pointer,struct.pack('<6I',*values));self.mu.mem_write(count_address,struct.pack('<I',count))
        for a,value in zip((*self.life['placement'],*self.life['lost']),(bits(-7.25),bits(8.5),bits(90.25),bits(-91.5))):
            self.mu.mem_write(a,struct.pack('<I',value))
        for a in self.life['suppress']:self.mu.mem_write(a,b'\0'*4)
        if self.life['silent']:self.mu.mem_write(self.life['silent'],struct.pack('<I',148))


def validate_calls(binary,record):
    """고정 공개 입력/실제 호출/대체 경계를 별도 예상 수와 대조한다."""
    if record['asserts']:raise RuntimeError('원본 assert 도달: '+binary)
    # 생성 시 확정된 수는 감사 시에도 같은 경계를 요구한다.
    if record['native_calls']!=EXPECTED_NATIVE or record['lifecycle_calls']!=EXPECTED_LIFECYCLE or record['graph_calls']!=EXPECTED_GRAPH:
        raise RuntimeError('실제 삭제/Graph 호출 감사 실패: '+binary)
    if record['substitutions']!=EXPECTED_SUBSTITUTIONS:raise RuntimeError('외부 효과 감사 실패: '+binary)


def generate():
    """직접 전/후 훅과 실제 Unpop/Release 통합 경로의 고정 결과를 기록한다."""
    rows=[];reports={};counts=collections.Counter();paths=set()
    for binary in BINARIES:
        oracle=DestroyGraphOracle(binary)
        # x87 53/64비트의 입력/관찰 경로를 분리한다.
        for control in CONTROLS:
            oracle.start_lifecycle(control)
            # 각 공개 경로는 독립된 같은 초기 상태에서 시작한다.
            for kind in EXPECTED_CASES:
                for case in range(96):
                    args,nodes,spots,records=case_input(case,kind);oracle.prepare_case(args,nodes,spots,records)
                    entry={'Pre':'pre','Post':'post','Destroy':'destroy'}[kind]
                    oracle.invoke(oracle.d[entry],[args[8]],POOL+oracle.root*oracle.stride,4)
                    rows.append([kind,binary,control,','.join(map(str,args)),*oracle.output(),oracle.bookkeeping_output(),oracle.context_output(),
                        packed(nodes),packed(spots),records.hex(),bytes(oracle.mu.mem_read(TABLE,255*6)).hex(),zlib.adler32(oracle.mu.mem_read(FLOOD,4096*4))])
                    counts[kind]+=1
        reports[binary]=dict(binary_sha256=oracle.sha256,native_calls=dict(oracle.native),lifecycle_calls=dict(oracle.lifecycle_native),
            graph_calls=dict(oracle.graph_native),substitutions=dict(oracle.substitutions),asserts=oracle.assertions)
        validate_calls(binary,reports[binary]);paths.update(oracle.body_paths)
        for folder in ('sid','creation','graphremove'):
            directory=ROOT/f'extracted/{folder}/{oracle.edition if folder!='graphremove' else binary}'
            paths.update(directory.rglob('*.c'));paths.update(directory.rglob('*.tsv'))
        print(binary,'공통 삭제 Graph 실제 명령 대조 완료',dict(counts),flush=True)
    if dict(counts)!=EXPECTED_CASES:raise RuntimeError('공개 입력 수 불일치')
    FIXTURE.write_text('# 실제 공통 삭제 Graph/공간/장부. 보상·소리·UI·전파/로그 대체, 게임/창 실행 없음.\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths.update([FIXTURE,Path(__file__),ROOT/'tools/ghidra/ExportCreation.java',ROOT/'tools/ghidra/run_script.ps1',ROOT/'tools/ghidra/destroygraph-functions.json'])
    paths.update(ROOT/'tools'/name for name in (*DEPENDENCIES,'decomp_bridge_oracle.py','decomp_destroy_oracle.py','decomp_graphremove_oracle.py','decomp_destroylifecycle_oracle.py'))
    report=dict(cases=dict(counts),total=sum(counts.values()),fpu_controls=list(CONTROLS),editions=reports,
        files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(paths)},
        limits=['실제 pre/post Graph Detach/Free·발자국/일반 finder/표면 Add·장부·destroy/Unpop/Release와 CRT 기록 이동',
            '합성 타입/프레임/SHP·정수 발자국/지도 경계·확보한 client 풀·동결 시계·표시 억제·AI null·비전투 null 큐·Graph 소진 전',
            '보상/SP·실제 소리·선택 UI·전파·로그는 사건 대체; 실제 표면 그래프 함수/탐색은 대체 없음',
            '직접 Pre/Post의 깊이 underflow와 통합 Destroy 균형 깊이를 구별; 전체 풀/공간/장부/스택은 Adler-32',
            '게임/OS/창 실행·별도 FS 감사·raw GameWorld 연결·미션 완주 검증 없음'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('새 공통 삭제 Graph x86:',report['total'])


def verify():
    """원본 PE/도구/몸체/fixture 바이트와 고정 행 수·호출 경계만 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in report['files'].items():
        if sha(ROOT/name)!=value:raise RuntimeError('SHA 불일치: '+name)
    for binary,record in report['editions'].items():
        if sha(ROOT/BINARIES[binary])!=record['binary_sha256']:raise RuntimeError('PE SHA 불일치: '+binary)
        validate_calls(binary,record)
    counts=collections.Counter(line.split('\t')[0] for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#'))
    if dict(counts)!=EXPECTED_CASES or dict(counts)!=report['cases'] or report['total']!=1728 or report['fpu_controls']!=list(CONTROLS):
        raise RuntimeError('행 수/정밀도 불일치')
    print('공통 삭제 Graph SHA/행 수/호출 경계 감사:',report['total'])


if __name__=='__main__':
    # 기대값 생성과 읽기 전용 감사를 구별한다.
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate()
