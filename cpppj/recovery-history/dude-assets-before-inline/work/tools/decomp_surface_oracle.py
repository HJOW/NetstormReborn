#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""표면 연결·정수 발자국의 이웃 탐색·붕괴 방문 목록을 실제 두 원본 x86에서 얻는다.

원본 게임/OS를 실행하지 않는다. 실제 탐색기/필터/기하/재귀 함수를 실행하며 assert 보고만 대체한다.
접합 칸 고리와 일반 공간 해시 탐색은 대상 밖이다. 기존 다리 기대값은 변경하지 않는다.
"""
import collections
import hashlib
import json
import random
import struct
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH, STACK, STOP
from decomp_bridge_oracle import BridgeOracle
from unicorn import UC_HOOK_MEM_WRITE

# 정상 반환까지 실행하는 세 가지 진입점과 두 판본 대응 주소다.
PAIRS = [
    {'name':'Connect','originals':'00441e40','originalCD':'004d2e00','purge':0},
    {'name':'Find','originals':'004b23e0','originalCD':'004ebad0','purge':8},
    {'name':'Next','originals':'004b1e70','originalCD':'004ec030','purge':0},
    {'name':'Count','originals':'004b2660','originalCD':'004ec150','purge':0},
    {'name':'Collect','originals':'004218b0','originalCD':'00449f10','purge':8},
]
# 실제 이웃 생성/반복에서 호출된 가상 필터와 내부 반복자도 판본 대응이 확인된다.
REVIEWED = PAIRS + [
    {'name':'Filter','originals':'004b1e80','originalCD':'004ec040','purge':4},
    {'name':'SurfaceAdvance','originals':'004b1c20','originalCD':'004ebdd0','purge':0},
]
# flag 8 탐색의 실제 보조 함수만 허용한다. 일반 공간 해시/OS 경로는 포함하지 않는다.
HELPERS = {
 'originals':[0x4b1d70,0x4b1c20,0x4b1e80,0x4ab880,0x4ade70,0x4adea0,
    0x49a840,0x49a900,0x49ae80,0x4ac200,0x4ac1e0,0x4ac550,0x4ad680,
    0x41ce90,0x40e880,0x40e8f0,0x40e800,0x41d0d0,0x4e49c0,0x40ea00],
 'originalCD':[0x4ebf30,0x4ebdd0,0x4ec040,0x487810,0x4aeec0,0x4aef30,
    0x4442b0,0x4449d0,0x4abae0,0x4abac0,0x4abf50,0x4ae3f0,
    0x43f620,0x43f950,0x4f161c],
}
# 타입 배열 포인터·Squid 배열/개수·표면/spot 포인터·보드 크기와 붕괴 목록/플래그 전역이다.
GLOBALS = {
 'originals':(0x59ab20,0x5c8464,0x5c847c,0x5c84bc,0x5c7c44,0x531928,0x5453c0,0x5453a0,0x5453a4),
 'originalCD':(0x51c960,0x5395dc,0x5395f4,0x52d590,0x52fe48,0x52e9a8,0x565ac0,0x565acc,0x565ad4),
}
# 에뮬레이터 내부의 서로 겹치지 않는 입출력 영역이다.
FINDER, TYPES, POOL, VISITED, CODES, MAP, SPOTS = (SCRATCH,SCRATCH+0x4000,SCRATCH+0x14000,
    SCRATCH+0x18000,SCRATCH+0x19000,SCRATCH+0x20000,SCRATCH+0x40000)
# 타입 번호/객체 범위를 정상 원본 배열 안에 둔다.
TYPE_FIRST, POOL_COUNT = 70, 32
# 입력만 생성하는 고정 시드이며 기대 결과는 원본 기계어가 결정한다.
SEED = 0x4b23e0


def packed(rows):
    """정수 입력 목록을 한 TSV 필드로 보존한다."""
    return ';'.join(','.join(map(str,row)) for row in rows) or '-'


class SurfaceOracle(Oracle):
    """실제 함수·정상 배열 입력과 제한한 메모리 쓰기만 허용한다."""
    def __init__(self, edition):
        """판본별 배열 구조와 허용 쓰기 영역을 준비한다."""
        super().__init__(edition,PAIRS,helpers=HELPERS[edition])
        self.stubs={address:name for address,name in self.stubs.items() if name=='assert'}
        self.mu.mem_map(MAP,0x30000)
        self.stride=500 if edition=='originals' else 468
        self.squid_stride=50 if edition=='originals' else 36
        type_ptr,pool_ptr,pool_count,map_ptr,spot_ptr,board,visit,short,progress=GLOBALS[edition]
        self.mu.mem_write(type_ptr,struct.pack('<I',TYPES-TYPE_FIRST*self.stride))
        self.mu.mem_write(pool_ptr,struct.pack('<I',POOL))
        self.mu.mem_write(pool_count,struct.pack('<I',POOL_COUNT))
        self.mu.mem_write(map_ptr,struct.pack('<I',MAP))
        self.mu.mem_write(spot_ptr,struct.pack('<I',SPOTS))
        self.mu.mem_write(board,struct.pack('<I',256))
        self.write_ranges=[(FINDER,FINDER+0x100),(0x30000000,0x30010000),(VISITED,VISITED+400),
            (visit+8,visit+12),(short,short+4),(progress,progress+4)]
        self.writes=set()
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.on_write)

    def on_write(self,_mu,_access,address,size,_value,_data):
        """실제 함수가 입력 객체·지도·타입이나 다른 전역을 쓰면 실패한다."""
        if not any(lo<=address and address+size<=hi for lo,hi in self.write_ranges):
            raise RuntimeError(f'예상 밖 쓰기: {self.edition} {address:08x}+{size}')
        self.writes.add((address,size))

    def call(self,name,args,this=0,purge=0):
        """기존 제한 호출과 동일하게 명령 수·정상 복귀 EIP·ESP/ret N을 검사한다."""
        return BridgeOracle.call(self,name,args,this,purge)

    def setup(self,objects,spots=(),overrides=()):
        """정수 발자국을 실제 타입/Squid 배열·표면 지도에 연결한다."""
        self.mu.mem_write(TYPES,bytes(self.stride*POOL_COUNT))
        self.mu.mem_write(POOL,bytes(self.squid_stride*POOL_COUNT))
        cells=[0]*65536
        flags=bytearray(65536)
        # 객체 한 개마다 정상 타입과 프레임 0을 연결한다.
        for oid,x,y,width,height,f1,f2,side,variant,dead,buried in objects:
            assert 0<oid<POOL_COUNT and 0<=x<256 and 0<=y<256
            t=TYPES+oid*self.stride
            o=POOL+oid*self.squid_stride
            self.mu.mem_write(t+0xe8,struct.pack('<II',f1,f2))
            self.mu.mem_write(t+0x114,struct.pack('<I',1))
            self.mu.mem_write(t+0x124,struct.pack('<I',CODES+oid*4))
            foot=0x1d4 if self.edition=='originals' else 0x1b4
            self.mu.mem_write(t+foot,struct.pack('<II',width,height))
            self.mu.mem_write(CODES+oid*4,bytes([side,variant,1,0]))
            self.mu.mem_write(o+10,bytes([TYPE_FIRST+oid,2 if dead else 0]))
            self.mu.mem_write(o+0xe,struct.pack('<ff',x,y))
            self.mu.mem_write(o+(0x28 if self.edition=='originals' else 0x23),bytes([buried]))
            # 지도는 오른쪽 아래 기준점에서 발자국 전체를 채운다. 중복은 뒤 객체가 덮는다.
            for yy in range(y-height+1,y+1):
                # 같은 발자국 행의 모든 칸에 객체 번호를 적는다.
                for xx in range(x-width+1,x+1):
                    if 0<=xx<256 and 0<=yy<256: cells[yy*256+xx]=oid
        # spot 비트와 명시한 지도 중복 입력은 원본 계산과 무관한 시험 자료다.
        for x,y,value in spots: flags[y*256+x]=value
        # 명시한 지도 중복/덮어쓰기 입력을 적용한다.
        for x,y,value in overrides: cells[y*256+x]=value
        self.mu.mem_write(MAP,struct.pack('<65536H',*cells))
        self.mu.mem_write(SPOTS,bytes(flags))

    def connect(self,side1,variant1,flags1,side2,variant2,flags2,direction):
        """타입/프레임 두 개의 실제 연결 반환값을 읽는다."""
        # 연결 함수는 타입 플래그와 프레임 코드만 읽으므로 지도 입력을 만들 필요가 없다.
        for oid,side,variant,flags in [(1,side1,variant1,flags1),(2,side2,variant2,flags2)]:
            t=TYPES+oid*self.stride
            self.mu.mem_write(t+0xec,struct.pack('<I',flags))
            self.mu.mem_write(t+0x124,struct.pack('<I',CODES+oid*4))
            self.mu.mem_write(CODES+oid*4,bytes([side,variant,1,0]))
        return self.call('Connect',[TYPES+self.stride,0,TYPES+2*self.stride,0,direction])

    def neighbors(self,objects,spots,overrides,root):
        """실제 생성자·가상 필터·다음 호출이 돌려주는 모든 표면 번호를 읽는다."""
        self.setup(objects,spots,overrides)
        self.mu.mem_write(FINDER,bytes(0x100))
        assert self.call('Find',[root,8],FINDER,8)==FINDER
        result=[]
        # 대상 입력에는 이웃이 10개 이하이며 원본 탐색기의 중복 제거도 실행한다.
        for _ in range(11):
            current=struct.unpack('<I',self.mu.mem_read(FINDER+0x34,4))[0]
            if current==0: return result
            result.append(current)
            self.call('Next',[],FINDER)
        raise RuntimeError('이웃 목록이 제한을 넘음')

    def collect(self,objects,spots,overrides,root,capacity):
        """실제 이웃 탐색과 재귀가 남기는 전체 방문 목록·두 플래그를 읽는다."""
        self.setup(objects,spots,overrides)
        visit,short,progress=GLOBALS[self.edition][6:]
        self.mu.mem_write(VISITED,bytes(400))
        self.mu.mem_write(VISITED,struct.pack('<I',root))
        self.mu.mem_write(visit,struct.pack('<III',VISITED,capacity,1))
        self.mu.mem_write(short,bytes(4))
        self.mu.mem_write(progress,bytes(4))
        self.call('Collect',[0,0],POOL+root*self.squid_stride,8)
        count=struct.unpack('<I',self.mu.mem_read(visit+8,4))[0]
        assert count<=capacity
        values=list(struct.unpack('<'+'I'*count,self.mu.mem_read(VISITED,count*4)))
        return struct.unpack('<I',self.mu.mem_read(short,4))[0],struct.unpack('<I',self.mu.mem_read(progress,4))[0],values


def node(oid,x,y,side='A',flags=4,width=1,height=1,variant='P',surface=True,dead=0,buried=0):
    """입력용 표면 레코드를 만든다. 연결/붕괴 정답은 계산하지 않는다."""
    return (oid,x,y,width,height,0x800 if surface else 0,flags,ord(side),ord(variant),dead,buried)


def main():
    """실제 두 판본 결과가 같은 범위의 TSV와 재현 근거를 생성한다."""
    a,b=SurfaceOracle('originals'),SurfaceOracle('originalCD')
    rng=random.Random(SEED)
    rows=['# 원본 게임 실행 없음. 정수 발자국·flag 8 이웃 탐색·접합 고리 없는 재귀 범위.']
    counts=collections.Counter()
    # 기본 16글자 모든 쌍/8방향과, 축마다 다른 방향 글자를 대조한다.
    for first in range(16):
        # 두 번째 표면의 모든 방향 문자와 연결한다.
        for second in range(16):
            # 직선/대각선 여덟 방향을 모두 비교한다.
            for direction in range(8):
                args=(65+first,65+(first+5)%16,4,65+second,65+(second+7)%16,4,direction)
                value=a.connect(*args)
                assert value==b.connect(*args),args
                rows.append('Connect\t'+'\t'.join(map(str,(*args,value))))
                counts['connect']+=1
    # bridge/bomb·섬/건물·emplacement 우선순위와 실제 복합 마스크를 검사한다.
    masks=[0,2,4,0x100,0x200,0x2000,0x4000,0x40000,0x40004,0x1044202,0x104]
    for f1 in masks:
        # 양쪽 타입 마스크가 동시에 걸리는 순서도 검사한다.
        for f2 in masks:
            # 짝수 side/홀수 variant를 모두 사용한다.
            for direction in range(8):
                args=(ord('P'),ord('L'),f1,ord('P'),ord('N'),f2,direction)
                value=a.connect(*args)
                assert value==b.connect(*args),args
                rows.append('Connect\t'+'\t'.join(map(str,(*args,value))))
                counts['connect']+=1
    print(f'연결 {counts["connect"]}개 완료',flush=True)
    # 경계·3×3 발자국·중복 번호·표면/죽음/spot 필터와 스캔 순서를 실제로 실행한다.
    for x,y,width,height in [(20,20,1,1),(20,20,3,3),(0,0,1,1),(255,255,1,1),(1,1,1,1)]:
        left,top=x-width+1,y-height+1
        ring=[(left,top-1),(x,top-1),(left-1,top),(x+1,top),(left-1,y),(x+1,y),(left,y+1),(x,y+1)]
        ring=list(dict.fromkeys((xx,yy) for xx,yy in ring if 0<=xx<256 and 0<=yy<256))
        # 서로 다른 필터/프레임 조합을 원본에 입력한다.
        for mode in range(24):
            objects=[node(1,x,y,width=width,height=height,flags=2 if mode%3==0 else 4)]
            spots=[]
            # 최대 여덟 이웃은 실제 지도 위치/타입/상태를 각각 가진다.
            for index,(xx,yy) in enumerate(ring):
                objects.append(node(index+2,xx,yy,side=rng.choice('ABCDEFGHIJKLMNOP'),
                    flags=rng.choice([2,4,0x200,0x40000]),variant=rng.choice('ABCDEFGHIJKLMNOP'),
                    surface=not(mode==1 and index%2==0),dead=int(mode==2 and index%2==0),buried=int(mode==3)))
                if mode==4: spots.append((xx,yy,8))
            if mode==5: spots.extend((xx,yy,8) for yy in range(top,y+1) for xx in range(left,x+1))
            overrides=[]
            value=a.neighbors(objects,spots,overrides,1)
            assert value==b.neighbors(objects,spots,overrides,1),(objects,spots,value)
            rows.append(f'Neighbor\t{packed(objects)}\t{packed(spots)}\t{packed(overrides)}\t1\t'+','.join(map(str,value)))
            counts['neighbor']+=1
    # 같은 큰 이웃을 세 칸에서 발견해도 한 번만 반환한다. 내부 비트의 부분/전체 AND도 구분한다.
    objects=[node(1,20,20,flags=2,width=3,height=3,variant='A'),
        node(2,20,17,flags=2,width=3,height=3,variant='A'),node(3,17,20,flags=2,width=3,height=3,variant='A'),
        node(4,21,20),node(5,20,21)]
    for spots in [[],[(20,17,8)],[(18,18,8)],[(x,y,8) for y in range(18,21) for x in range(18,21)]]:
        value=a.neighbors(objects,spots,[],1)
        assert value==b.neighbors(objects,spots,[],1)
        rows.append(f'Neighbor\t{packed(objects)}\t{packed(spots)}\t-\t1\t'+','.join(map(str,value)))
        counts['neighbor']+=1
    # 지도에서 같은 번호를 다른 위치에 반복시킨 경우에도 중복 필터를 실제로 실행한다.
    objects=[node(1,20,20),node(2,20,19)]
    overrides=[(19,20,2),(21,20,2),(20,21,2)]
    value=a.neighbors(objects,[],overrides,1)
    assert value==b.neighbors(objects,[],overrides,1)
    rows.append(f'Neighbor\t{packed(objects)}\t-\t{packed(overrides)}\t1\t'+','.join(map(str,value)))
    counts['neighbor']+=1
    # 순회는 물리적인 직선/분기 트리를 쓴다. 접합 칸 고리가 없는 실제 표면 그래프다.
    for length in range(1,9):
        # 접합/판자/끝/섬을 섞어 원본 경계 분기를 지난다.
        for mode in range(12):
            objects=[node(i+1,20+i,20,side=rng.choice('AFIJKLMP')) for i in range(length)]
            if mode%3==0 and length>1: objects[-1]=node(length,20+length-1,20,flags=2,width=1)
            if mode%4==0 and length>2: objects.append(node(length+1,21,19,side='K'))
            # 실제 목록이 꽉 찼을 때도 재귀는 계속 진행하는지 확인한다.
            for capacity in [1,3,100]:
                value=a.collect(objects,[],[],1,capacity)
                assert value==b.collect(objects,[],[],1,capacity),(objects,value)
                rows.append(f'Collect\t{packed(objects)}\t-\t-\t1\t{capacity}\t{value[0]}\t{value[1]}\t'+','.join(map(str,value[2])))
                counts['collect']+=1
    # 긴 접합 사슬과 분기는 짧은 접합/깊이 1/이웃 수 2의 경계 분기를 의도적으로 지난다.
    graphs=[]
    for length in range(2,7):
        graphs.append([node(i+1,20+i,20,side='J' if i==length-1 else 'A') for i in range(length)])
    graphs.append([node(1,20,20),node(2,21,20),node(3,22,20,side='J'),
        node(4,21,19,side='K'),node(5,23,20,side='J')])
    graphs.append([node(1,20,20),node(2,21,20),node(3,22,20,flags=2),node(4,21,19,side='K')])
    # 의도한 접합 사슬/분기별 전체 결과를 대조한다.
    for objects in graphs:
        # 방문 목록의 제한은 재귀 깊이/경계 처리와 별개다.
        for capacity in [1,3,100]:
            value=a.collect(objects,[],[],1,capacity)
            assert value==b.collect(objects,[],[],1,capacity)
            rows.append(f'Collect\t{packed(objects)}\t-\t-\t1\t{capacity}\t{value[0]}\t{value[1]}\t'+','.join(map(str,value[2])))
            counts['collect']+=1
    payload='\n'.join(rows)+'\n'
    fixture=ROOT/'cpppj/tests/fixtures/surface-x86.tsv'
    fixture.write_text(payload,encoding='utf-8',newline='\n')
    sources=['tools/decomp_surface_oracle.py','tools/decomp_bridge_oracle.py','tools/decomp_oracle.py']
    report={'schema':1,'functions':PAIRS,'helpers':HELPERS,'cases':dict(counts),'total_cases':sum(counts.values()),
        'reviewed_pairs':REVIEWED,
        'rejected_pairs':[{'originals':'004b1810','originalCD':'004eae20',
            'reason':'일반 공간 해시 다음 함수(ret 0)와 생성자(ret 16)의 오대응. 이 경로의 실제 동작 검증은 후속.'}],
        'binary_sha256':{'originals':a.sha256,'originalCD':b.sha256},
        'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),
        'source_sha256':{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in sources},
        'assert_reports':{'originals':a.assertions,'originalCD':b.assertions},
        'native_write_addresses_sizes':{'originals':len(a.writes),'originalCD':len(b.writes)},
        'limitations':['정수 기준점·정상 발자국과 flag 8 이웃 경로만','실제 함수 정상 반환까지 검증, assert 보고만 대체',
            '접합 고리·일반 공간 해시·실제 삭제/수명/그림/소유자 전파/배치 UI 미검증']}
    (ROOT/'cpppj/recovery-surface-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'cases':dict(counts),'assertions':report['assert_reports']}))


# 직접 실행했을 때만 기대값을 생성한다.
if __name__=='__main__':
    main()
