#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 두 판본의 4단계 공간 해시와 발자국 점유 비트 계산을 정상 반환까지 검증한다.

원본/복사본 게임이나 OS를 실행하지 않는다. 초기화는 이미 할당된 배열 경로만 실행한다.
지도 주소, 타입/Squid/프레임 크기는 격리 메모리 입력이며 기대값은 실제 기계어에서 얻는다.
"""
import collections
import hashlib
import json
import struct
from decomp_oracle import Oracle, ROOT, SCRATCH
from decomp_bridge_oracle import BridgeOracle, float_bits
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_FPCW, UC_X86_REG_FPSW

# 확인할 진입점과 스택 인자 제거 크기. Init의 malloc 분기는 검증하지 않는다.
PAIRS = [
    {'name':'Init','originals':'004b28f0','originalCD':'00479ad0','purge':0},
    {'name':'At','originals':'004b29a0','originalCD':'00479be0','purge':8},
    {'name':'FloatBucket','originals':'004b2a90','originalCD':'00479d20','purge':12},
    {'name':'IntBucket','originals':'004b2b00','originalCD':'00479d90','purge':12},
    {'name':'Level','originals':'004ace40','originalCD':'004acb00','purge':0},
    {'name':'Genus','originals':'004afd30','originalCD':'004acfc0','purge':8},
]
# Init은 할당 없는 경로만 확인하므로 완전 함수 대응 앵커에서 제외한다.
REVIEWED = PAIRS[1:]
# 타입/발자국/프레임 크기와 실제 CRT float→int 보조 함수만 허용한다.
HELPERS = {'originals':[0x49a840,0x49a900,0x419850,0x4e49c0],
           'originalCD':[0x4abae0,0x4abf50,0x4ae480,0x4442b0,0x4f161c]}
# 원본 표의 버킷 한 변 크기. 0단계 배열이 표면 조회 전역에도 연결된다.
SCALES = (1,2,4,16)
# 타입 표/보드 한 변/표면 포인터/객체 해시 단계 필드의 판본별 주소다.
GLOBALS = {'originals':(0x59ab20,0x531928,0x5c84bc,0x21),
           'originalCD':(0x51c960,0x52e9a8,0x52d590,0x1f)}
# 격리 메모리의 서로 겹치지 않는 객체·해시·타입·SHP 헤더·배열 영역이다.
OBJECT,HASH,TYPE,SHAPE,MAP = SCRATCH+0x100,SCRATCH+0x1000,SCRATCH+0x4000,SCRATCH+0x8000,SCRATCH+0x20000
# 타입 번호는 실제 자산 타입의 정상 범위 안에 둔다.
TYPE_ID = 70
# 초기화가 4개 배열의 모든 short를 쓰도록 미리 채우는 입력이다.
SENTINEL = 0xa55a
# 이번 검증의 x87 입력: 모든 예외 마스크·nearest·53비트 중간 정밀도다.
# Unicorn 초기값 0은 예외 마스크/정밀도가 달라 CD _ftol 경계 결과를 왜곡한다.
FPU_CONTROL = 0x027f
# float 경로는 64비트 중간 정밀도에서도 같은 결과인지 별도로 검사한다.
FPU_CONTROLS = (FPU_CONTROL,0x037f)


class HashOracle(Oracle):
    """버킷/점유 함수와 지정한 출력 쓰기만 실행한다."""
    def __init__(self,edition):
        """판본별 정상 타입과 미리 할당된 4개 배열을 준비한다."""
        super().__init__(edition,PAIRS,helpers=HELPERS[edition])
        self.stubs={addr:name for addr,name in self.stubs.items() if name=='assert'}
        self.mu.mem_map(MAP,0x2b000)
        self.bases=[]
        address=MAP
        # 각 배열을 연속 배치하되 주소 결과는 판본과 무관한 배열 인덱스로 정규화한다.
        for scale in SCALES:
            self.bases.append(address)
            address+=(256//scale)**2*2
        base,board,surface,level=GLOBALS[edition]
        self.mu.mem_write(base,struct.pack('<I',TYPE-TYPE_ID*(500 if edition=='originals' else 468)))
        self.mu.mem_write(board,struct.pack('<I',256))
        self.mu.mem_write(HASH,struct.pack('<7I',0,1,256,*self.bases))
        self.write_ranges=[(0x30000000,0x30010000),(HASH,HASH+28),(MAP,address),
            (surface,surface+4),(OBJECT+level,OBJECT+level+1)]
        self.writes=set()
        self.control_word=FPU_CONTROL
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.on_write)

    def on_write(self,_mu,_access,address,size,_value,_data):
        """지도 외의 입력 타입/프레임/객체 필드를 원본이 쓰면 실패한다."""
        if not any(lo<=address and address+size<=hi for lo,hi in self.write_ranges):
            raise RuntimeError(f'예상 밖 쓰기: {self.edition} {address:08x}+{size}')
        self.writes.add((address,size))

    def call(self,name,args,this=0,purge=0):
        """기존 실행기와 같은 코드 범위·명령 수·복귀 EIP/ESP/ret N 제한을 적용한다."""
        self.mu.reg_write(UC_X86_REG_FPCW,self.control_word)
        top=self.mu.reg_read(UC_X86_REG_FPSW)&0x3800
        value=BridgeOracle.call(self,name,args,this,purge)
        assert self.mu.reg_read(UC_X86_REG_FPCW)==self.control_word,name
        assert self.mu.reg_read(UC_X86_REG_FPSW)&0x3800==top,name
        return value

    def init(self,selected):
        """전체 배열 초기화·선택 단계 보존·표면 전역 연결을 실제 함수로 확인한다."""
        self.mu.mem_write(HASH,struct.pack('<7I',selected,SCALES[selected],256//SCALES[selected],*self.bases))
        # 모든 단계를 0이 아닌 입력으로 채워 부분 초기화도 탐지한다.
        for level,scale in enumerate(SCALES):
            self.mu.mem_write(self.bases[level],struct.pack('<H',SENTINEL)*(256//scale)**2)
        self.call('Init',[],HASH)
        fields=struct.unpack('<3I',self.mu.mem_read(HASH,12))
        # 원본이 쓰는 모든 short를 확인한다. malloc은 호출되지 않아야 한다.
        for level,scale in enumerate(SCALES):
            assert self.mu.mem_read(self.bases[level],(256//scale)**2*2)==bytes((256//scale)**2*2)
        surface=GLOBALS[self.edition][2]
        assert struct.unpack('<I',self.mu.mem_read(surface,4))[0]==self.bases[0]
        return fields

    def bucket(self,name,x,y,level):
        """실제 반환 포인터를 지정한 단계의 short 인덱스로 변환한다."""
        if name=='At':
            self.mu.mem_write(HASH,struct.pack('<3I',level,SCALES[level],256//SCALES[level]))
            address=self.call(name,[x,y],HASH,8)
        else:
            args=[float_bits(x),float_bits(y),level] if name=='FloatBucket' else [x,y,level]
            address=self.call(name,args,HASH,12)
        offset=address-self.bases[level]
        assert offset>=0 and offset%2==0 and offset<(256//SCALES[level])**2*2
        return offset//2

    def setup_object(self,flags,width,height,x=20.0,y=20.0):
        """타입/Squid 발자국 입력을 만들고 실물 SHP 크기 헤더를 연결한다."""
        self.mu.mem_write(TYPE,bytes(500 if self.edition=='originals' else 468))
        self.mu.mem_write(OBJECT,bytes(64))
        self.mu.mem_write(TYPE+0xec,struct.pack('<I',flags))
        self.mu.mem_write(TYPE+0x114,struct.pack('<I',1))
        self.mu.mem_write(TYPE+0xdc,struct.pack('<I',SHAPE))
        self.mu.mem_write(SHAPE+8,struct.pack('<I',0x100))
        self.mu.mem_write(SHAPE+0x100-0x24,struct.pack('<ff',width,height))
        foot=0x1d4 if self.edition=='originals' else 0x1b4
        self.mu.mem_write(TYPE+foot,struct.pack('<II',int(width),int(height)))
        self.mu.mem_write(OBJECT+10,bytes([TYPE_ID]))
        self.mu.mem_write(OBJECT+0xe,struct.pack('<ff',x,y))

    def level(self,flags,width,height):
        """실제 프레임 크기/다리·섬 분기가 저장한 해시 단계 바이트를 읽는다."""
        self.setup_object(flags,width,height)
        self.call('Level',[],OBJECT)
        return self.mu.mem_read(OBJECT+GLOBALS[self.edition][3],1)[0]

    def genus(self,flags,width,height,x,y,cx,cy):
        """실제 발자국 안쪽/지붕 보정이 반환한 전체 uint 비트를 읽는다."""
        self.setup_object(flags,width,height,x,y)
        return self.call('Genus',[cx,cy],OBJECT,8)


def compare_float(a,b,method,*args):
    """두 판본·정상 x87 53/64비트 환경의 동일 입력을 모두 비교한다."""
    values=[]
    # FPCW=0인 에뮬레이터 기본값을 실제 판본 차이로 오인하지 않는다.
    for control in FPU_CONTROLS:
        # 실행 상태를 명시한 뒤 각 판본의 실제 기계어를 실행한다.
        for oracle in (a,b):
            oracle.control_word=control
            values.append(getattr(oracle,method)(*args))
    assert all(value==values[0] for value in values),(method,args,values)
    return values[0]


def main():
    """두 판본의 정상 반환 결과와 제한·해시를 TSV/JSON에 기록한다."""
    a,b=HashOracle('originals'),HashOracle('originalCD')
    rows=['# 원본 게임 실행 없음. Init은 미리 할당된 배열, 다른 함수는 정상 입력의 전체 반환.']
    counts=collections.Counter()
    # 네 초기 선택 단계에서 전체 초기화·복원 결과를 확인한다.
    for selected in range(4):
        va,vb=a.init(selected),b.init(selected)
        assert va==vb
        rows.append('Init\t'+str(selected)+'\t'+'\t'.join(map(str,va)))
        counts['init']+=1
    # 각 단계의 시작·경계·끝 버킷과 fractional 좌표를 실제 반환 주소로 비교한다.
    for level,scale in enumerate(SCALES):
        side=256//scale
        cells=sorted(set([0,1,side//2,side-2,side-1]))
        # IntBucket/At의 입력은 월드 칸이 아니라 해당 단계의 버킷 칸이다.
        for x in cells:
            # 배열 안의 모든 대표 행을 확인한다.
            for y in cells:
                # 선택 단계 조회와 명시 단계 조회는 서로 다른 실제 함수다.
                for name in ['At','IntBucket']:
                    value=a.bucket(name,x,y,level)
                    assert value==b.bucket(name,x,y,level)
                    rows.append(f'{name}\t{level}\t{x}\t{y}\t{value}')
                    counts[name.lower()]+=1
        coordinates=sorted(set([0.0,0.9999,1.0,1.0001,scale-0.0001,float(scale),scale+0.0001,
            127.9999,128.0,128.0001,254.9999,255.0,255.9999]))
        # plain _ftol인 해시 좌표를 표면 조회의 +0.9999와 혼동하지 않는다.
        for x in coordinates:
            # 경계 바로 앞/뒤의 행도 같은 방식으로 검증한다.
            for y in coordinates:
                value=compare_float(a,b,'bucket','FloatBucket',x,y,level)
                rows.append(f'FloatBucket\t{level}\t{float_bits(x)}\t{float_bits(y)}\t{value}')
                counts['floatbucket']+=1
    print('버킷/초기화 완료',flush=True)
    sizes=[0.0,0.9999,1.0,1.9999,2.0,2.0001,3.9999,4.0,4.0001,8.0,16.0]
    # 섬/다리의 0단계와 일반 프레임의 2/4 크기 경계를 확인한다.
    for flags in [0,2,4,6,0x40000]:
        # 폭과 높이를 따로 경계에 놓는다.
        for width in sizes:
            # 세로 크기도 같은 경계를 독립적으로 지난다.
            for height in sizes:
                va=vb=compare_float(a,b,'level',flags,width,height)
                rows.append(f'Level\t{flags}\t{float_bits(width)}\t{float_bits(height)}\t{va}\t{vb}')
                counts['level']+=1
    print('해시 단계 완료',flush=True)
    # 실제 건물군 마스크/지붕과 byte로 저장되지 않는 상위 비트도 확인한다.
    for width,height in [(1,1),(2,2),(3,3),(4,4),(5,5),(3,5),(5,3),(7,7)]:
        # 발자국마다 실제 건물군/지붕/그 외 genus 비트를 확인한다.
        for flags in [0,2,4,0x10,0x100,0x200,0x4000,0x40000,0x400000,0x400200,0x50444200,0xffffffff]:
            # 소수 기준점도 원본의 0 방향 절삭으로 발자국을 결정한다.
            for x,y in [(20.0,20.0),(20.9999,20.0001)]:
                # 입력 발자국의 각 행에서 전체 결과를 얻는다.
                for cy in range(int(y)-height+1,int(y)+1):
                    # 발자국 경계/중심/지붕의 모든 칸에서 반환 비트를 비교한다.
                    for cx in range(int(x)-width+1,int(x)+1):
                        va=vb=compare_float(a,b,'genus',flags,width,height,x,y,cx,cy)
                        rows.append(f'Genus\t{flags}\t{width}\t{height}\t{float_bits(x)}\t{float_bits(y)}\t{cx}\t{cy}\t{va}\t{vb}')
                        counts['genus']+=1
    assert a.assertions==b.assertions==0
    fixture=ROOT/'cpppj/tests/fixtures/hash-x86.tsv'
    fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    sources=['tools/decomp_hash_oracle.py','tools/decomp_bridge_oracle.py','tools/decomp_oracle.py']
    report={'schema':1,'functions':PAIRS,'reviewed_pairs':REVIEWED,'helpers':HELPERS,'cases':dict(counts),
        'total_cases':sum(counts.values()),'x87_control_word':hex(FPU_CONTROL),
        'x87_checked_control_words':[hex(control) for control in FPU_CONTROLS],
        'binary_sha256':{'originals':a.sha256,'originalCD':b.sha256},
        'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),
        'source_sha256':{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in sources},
        'assert_reports':{'originals':a.assertions,'originalCD':b.assertions},
        'limits':['Init은 기존 배열 초기화만; malloc/실제 게임 미실행',
            '정상 비음수 좌표/크기; 공간 체인 삽입·제거와 일반 탐색은 후속',
            'Level/Genus는 판본별 기대값을 별도 저장한다']}
    (ROOT/'cpppj/recovery-hash-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'cases':dict(counts),'assert_reports':report['assert_reports']}),flush=True)


# 직접 실행했을 때만 원본에서 기대값을 다시 얻는다.
if __name__=='__main__':
    main()
