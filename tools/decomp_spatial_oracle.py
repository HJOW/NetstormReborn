#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 Pop/Unpop에서 객체 체인·spot·상태 갱신을 격리 검증한다.

게임/OS는 실행하지 않는다. 가상 화면 갱신·firstPop·postPop 및 영역 조회/표면 알림은
명시한 계약으로 대체한다. 타입/좌표/발자국/genus/해시/체인/Activate는 실제 기계어다.
따라서 전체 Squid/영역/렌더링 복원이나 완전 함수 대응의 근거로 사용하지 않는다.
"""
import argparse
import collections
import hashlib
import json
import struct
from decomp_oracle import Oracle, ROOT, SCRATCH, STOP
from decomp_bridge_oracle import BridgeOracle, float_bits
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

# 격리된 검증 대상의 실제 진입점과 정상 반환 스택 제거 크기다.
PAIRS = [
    {'name':'Pop','originals':'004b02d0','originalCD':'004ad490','purge':12},
    {'name':'Unpop','originals':'004afe50','originalCD':'004ad0b0','purge':4},
]
# 실제 보조 함수를 허용한다. 영역/UI 효과는 아래 별도 대체 목록에만 있다.
HELPERS = {
    'originals':[0x4210a0,0x49a840,0x481510,0x40e800,0x4afd30,0x4e49c0,0x40da00,
                 0x425df0,0x4ac1e0,0x40eaf0,0x4ace40,0x49a900,0x419850,
                 0x4b2a90,0x4aba70,0x4acd20,0x4ac560],
    'originalCD':[0x4abf50,0x4abae0,0x442f30,0x4abac0,0x4acfc0,0x4f161c,
                  0x4acb00,0x4ae480,0x4442b0,0x479d20,0x4aca80,0x4abfb0],
}
# 판본별 pool/개수/타입/보드/spot/표면/해시/dirty/로그 억제/전투/postPop 전역이다.
GLOBALS = {
    'originals':(0x5c8464,0x5c847c,0x59ab20,0x531928,0x5c7c44,0x5c84bc,0x542508,
                 0x5949d0,0x594fc8,0x540bc0,0x5c8488,0x542514),
    'originalCD':(0x5395dc,0x5395f4,0x51c960,0x52e9a8,0x52fe48,0x52d590,0x5670c0,
                  0x51c54c,0x540a24,0x540a28,0x539600,0x5670cc),
}
# 각 메모리 영역은 서로 겹치지 않는다. 0번 객체와 타입 앞부분은 번호 산술용 여유다.
POOL, TYPES, SHAPE, VTABLE = SCRATCH+0x100, SCRATCH+0x4000, SCRATCH+0x14000, SCRATCH+0x18000
HEADS, SPOTS, GRID = SCRATCH+0x20000, SCRATCH+0x50000, SCRATCH+0x60000
# 버킷 크기·검증 객체 수·유효 타입 번호 시작값이다.
SCALES, POOL_COUNT, TYPE_ID = (1,2,4,16), 16, 70
# 가상 함수 네 개의 식별 주소다. 실제 원본 vtable/파생 객체는 이번 범위가 아니다.
CALLBACKS = {STOP+0x100:'update88',STOP+0x110:'update8c',STOP+0x120:'first',STOP+0x130:'post'}
# 영역 소속 조회와 표면 변경 알림은 별도의 미복원 의존성 계약이다.
DEPENDENCIES = {'originals':{0x46f6e0:'region',0x4214a0:'notify'},
                'originalCD':{0x4bdd80:'region',0x448c10:'notify'}}
# 실제 게임의 FP 초기화를 주장하지 않는다. 두 정상 x87 정밀도에서 대조한다.
CONTROLS = (0x027f,0x037f)


class SpatialOracle(Oracle):
    """전체 진입점의 정상 반환까지 공간 등록 부분과 대체 효과 호출 순서를 기록한다."""
    def __init__(self,edition):
        """PE를 읽고 쓰기/코드 범위와 대체 가상 함수를 준비한다."""
        super().__init__(edition,PAIRS,helpers=HELPERS[edition])
        self.stubs={address:name for address,name in self.stubs.items() if name=='assert'}
        self.mu.mem_map(HEADS,0x60000)
        self.stride=50 if edition=='originals' else 36
        self.type_stride=500 if edition=='originals' else 468
        self.extra=0x28 if edition=='originals' else 0x23
        self.level=0x21 if edition=='originals' else 0x1f
        self.bases=[]
        address=HEADS
        # 4단계 배열 주소를 서로 다른 크기의 연속 영역에 둔다.
        for scale in SCALES:
            self.bases.append(address)
            address+=(256//scale)**2*2
        pool,count,types,board,spots,surface,hash_addr,dirty,log,battle,post,grid=GLOBALS[edition]
        self.post=post
        # 전역 입력은 에뮬레이터 메모리에만 쓴다. dirty 큐/전투 복제는 비활성이다.
        for ptr,value in [(pool,POOL),(count,POOL_COUNT),(types,TYPES-TYPE_ID*self.type_stride),
                          (board,256),(spots,SPOTS),(surface,self.bases[0]),(dirty,0),(log,1),
                          (battle,0),(post,0),(grid,GRID)]:
            self.mu.mem_write(ptr,struct.pack('<I',value))
        self.mu.mem_write(hash_addr,struct.pack('<7I',0,1,256,*self.bases))
        self.mu.mem_write(VTABLE,bytes(0x90))
        # 효과 대체는 스택 제거 크기와 firstPop 반환값을 명시한다.
        for offset,address in [(0x88,STOP+0x100),(0x8c,STOP+0x110),(0x30,STOP+0x120),(0x20,STOP+0x130)]:
            self.mu.mem_write(VTABLE+offset,struct.pack('<I',address))
        self.ids=[]
        self.write_ranges=[(0x30000000,0x30010000),(HEADS,HEADS+86272*2),(SPOTS,SPOTS+65536),(post,post+4)]
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.on_write)
        self.control=CONTROLS[0]

    def on_write(self,_mu,_access,address,size,_value,_data):
        """입력 타입/코드/다른 전역 및 별도 grid 쓰기를 거부한다."""
        ranges=list(self.write_ranges)
        # 입력으로 등록한 객체의 공간 필드만 쓸 수 있다. vtable/type/미등록 풀 슬롯은 읽기 전용이다.
        for sid in self.ids:
            obj=POOL+sid*self.stride
            ranges.extend([(obj+4,obj+6),(obj+8,obj+10),(obj+11,obj+0x1a),
                           (obj+self.level,obj+self.level+1),(obj+self.extra,obj+self.extra+1)])
        if not any(lo<=address and address+size<=hi for lo,hi in ranges):
            raise RuntimeError(f'예상 밖 공간 쓰기: {self.edition} {address:08x}+{size}')

    def on_instruction(self,mu,address,size,data):
        """미복원 효과만 계약으로 대체하고 나머지 명령은 허용된 원본 몸체로 제한한다."""
        name=CALLBACKS.get(address) or DEPENDENCIES[self.edition].get(address)
        if name:
            esp=mu.reg_read(UC_X86_REG_ESP)
            ret=struct.unpack('<I',mu.mem_read(esp,4))[0]
            purge,value=0,0
            if name=='region':
                self.events.append(f'region:{int(self.supported)}')
                purge,value=8,int(self.supported)
            elif name=='notify':
                sid=struct.unpack('<I',mu.mem_read(esp+4,4))[0]
                self.events.append(f'notify:{sid}')
            elif name=='first':
                self.events.append(f'first:{self.first_flags}')
                value=self.first_flags
            elif name=='post':
                flags=struct.unpack('<I',mu.mem_read(esp+4,4))[0]
                self.events.append(f'post:{flags}')
                depth=struct.unpack('<I',mu.mem_read(self.post,4))[0]
                assert depth==1
                mu.mem_write(self.post,struct.pack('<I',depth-1))
                purge=4
            else:
                self.events.append(name)
            mu.reg_write(UC_X86_REG_EAX,value)
            mu.reg_write(UC_X86_REG_ESP,esp+4+purge)
            mu.reg_write(UC_X86_REG_EIP,ret)
            return
        super().on_instruction(mu,address,size,data)

    def begin(self,supported,first_flags,seed):
        """각 시나리오의 지도와 등록 풀을 초기화한다."""
        self.supported,self.first_flags,self.ids=supported,first_flags,[]
        self.mu.mem_write(HEADS,bytes(86272*2))
        self.mu.mem_write(SPOTS,bytes(65536))
        self.mu.mem_write(GRID,bytes(131072))
        self.mu.mem_write(POOL,bytes(POOL_COUNT*self.stride))
        # 중복 점유 사례의 초기 지도만 격리 입력으로 놓는다.
        for index,value in seed:
            self.mu.mem_write(SPOTS+index,bytes([value]))

    def add(self,sid,flags1,flags2,width,height,frame_width,frame_height,state=4,extra=0,word=0xffff,island=23):
        """판본별 raw 객체·타입·실물 SHP 크기 헤더를 연결한다. SID 할당은 대체 입력이다."""
        assert 0<sid<POOL_COUNT
        self.ids.append(sid)
        obj=POOL+sid*self.stride
        typ=TYPES+(sid-1)*self.type_stride
        shape=SHAPE+(sid-1)*0x200
        self.mu.mem_write(obj,bytes(self.stride))
        self.mu.mem_write(obj,struct.pack('<I',VTABLE))
        self.mu.mem_write(obj+8,struct.pack('<HBBHff',island,TYPE_ID+sid-1,state,word,20.,20.))
        self.mu.mem_write(obj+self.extra,bytes([extra]))
        self.mu.mem_write(typ,bytes(self.type_stride))
        self.mu.mem_write(typ+0xe8,struct.pack('<II',flags1,flags2))
        self.mu.mem_write(typ+0x114,struct.pack('<I',1))
        self.mu.mem_write(typ+0xdc,struct.pack('<I',shape))
        foot=0x1d4 if self.edition=='originals' else 0x1b4
        self.mu.mem_write(typ+foot,struct.pack('<II',width,height))
        self.mu.mem_write(shape+8,struct.pack('<I',0x100))
        self.mu.mem_write(shape+0x100-0x24,struct.pack('<ff',frame_width,frame_height))

    def step(self,name,sid,x,y,flags):
        """명령 상한·코드/쓰기 범위·복귀·ret N·x87 상태를 검사한다."""
        self.events=[]
        self.mu.reg_write(UC_X86_REG_FPCW,self.control)
        top=self.mu.reg_read(UC_X86_REG_FPSW)&0x3800
        args=[float_bits(x),float_bits(y),flags] if name=='Pop' else [flags]
        BridgeOracle.call(self,name,args,POOL+sid*self.stride,12 if name=='Pop' else 4)
        assert self.mu.reg_read(UC_X86_REG_FPCW)==self.control
        assert self.mu.reg_read(UC_X86_REG_FPSW)&0x3800==top
        assert struct.unpack('<I',self.mu.mem_read(self.post,4))[0]==0
        return self.snapshot()

    def snapshot(self):
        """전체 지도와 관련 객체의 0이 아닌 필드를 순서 있는 열로 정규화한다."""
        objects,heads,spots=[],[],[]
        # 체인 끝·섬 번호·상태/부가 비트·화면 좌표 캐시·해시 단계·좌표·표면 word를 비교한다.
        for sid in self.ids:
            data=self.mu.mem_read(POOL+sid*self.stride,self.stride)
            values=[sid,struct.unpack_from('<H',data,4)[0],struct.unpack_from('<H',data,8)[0],
                    data[11],data[self.extra],*struct.unpack_from('<hh',data,0x16),data[self.level],
                    *struct.unpack_from('<II',data,0xe),struct.unpack_from('<H',data,0xc)[0]]
            objects.append(','.join(map(str,values)))
        # 전체 배열을 검사하여 예상 지점 이외의 쓰기도 출력 대조에 포함한다.
        for level,scale in enumerate(SCALES):
            values=struct.unpack('<'+'H'*(256//scale)**2,self.mu.mem_read(self.bases[level],(256//scale)**2*2))
            heads.extend(f'{level}:{index}:{sid}' for index,sid in enumerate(values) if sid)
        spots.extend(f'{index}:{value}' for index,value in enumerate(self.mu.mem_read(SPOTS,65536)) if value)
        return [';'.join(objects) or '-', ';'.join(heads) or '-', ';'.join(spots) or '-', ';'.join(self.events) or '-']


def scenarios():
    """같은 버킷의 연결·경계·점유·재등록·인접 표면 입력을 만든다."""
    # 기본 기록: SID, flags1/2, 발자국 폭/높이, SHP 폭/높이, 상태, 부가 비트, word, 섬 번호.
    default=(1,0,0,1,1,1.,1.,4,0,0xffff,23)
    # 각 해시 단계에서 머리/중간/끝 삭제와 재등록·이동을 독립 실행한다.
    for level,size,genus in [(0,1.,4),(1,1.,0),(2,3.,0),(3,5.,0)]:
        # 제거 위치를 바꾸어 같은 체인의 끝/중간/머리를 각각 검증한다.
        for removed in (1,2,3):
            objects=[(sid,0,genus,1,1,size,size,4,0,0xffff,23) for sid in (1,2,3)]
            if level==0:
                objects=[(*obj[:8],8,*obj[9:]) for obj in objects]
            steps=[('Pop',sid,32.,32.,0) for sid in (1,2,3)]
            steps += [('Unpop',removed,0.,0.,0x2000),('Unpop',removed,0.,0.,0),
                      ('Pop',removed,40.,41.,8),('Unpop',removed,0.,0.,0)]
            yield f'chain-{level}-{removed}',False,0,[],objects,steps
    # 소수 좌표에서는 footprint의 절삭과 genus의 덧셈을 구별한다.
    for flags2 in (0,2,4,0x10,0x100,0x200,0x4000,0x40000,0x400200,0x50444200):
        # 정사각형과 직사각형 발자국의 경계/내부 칸을 모두 포함한다.
        for width,height in ((1,1),(2,2),(3,3),(5,3),(3,5),(5,5)):
            # 정수, 작은 소수, 편향 덧셈이 다음 칸으로 넘어가는 경계를 입력한다.
            for x,y in ((20.,20.),(20.0001,20.9999),(20.99995,21.00005)):
                obj=(1,0,flags2,width,height,float(width),float(height),4,0,0xffff,23)
                yield f'foot-{flags2}-{width}-{height}-{x}-{y}',False,0,[],[obj],[
                    ('Pop',1,x,y,0),('Unpop',1,0.,0.,0),('Pop',1,x+8,y+8,0x2000),('Unpop',1,0.,0.,0x2000)]
    # buried는 spot을 쓰지 않지만 등록 체인과 활성화는 진행한다.
    for flags1,flags2,extra,state in ((0,2,8,4),(0,0x200,8,4),(0x800,0,0,4),(0,0,0x80,4),(0,0,0,6)):
        obj=(1,flags1,flags2,3,3,3.,3.,state,extra,0xffff,23)
        yield f'flags-{flags1}-{flags2}-{extra}-{state}',False,0x100,[],[obj],[
            ('Pop',1,24.,25.,0x2000),('Unpop',1,0.,0.,0),('Pop',1,28.,29.,8),('Unpop',1,0.,0.,0)]
    # 잘못된 좌표는 로그를 억제하고 원본의 (10,10) 복구를 확인한다.
    for x,y in ((0.,20.),(-1.,20.),(256.,20.),(20.,0.),(20.,256.),(0.5,0.5),(255.9999,255.9999)):
        yield f'position-{x}-{y}',False,0,[],[default],[('Pop',1,x,y,0),('Unpop',1,0.,0.,0)]
    # 건물이 아닌 타입에서도 low byte와 flags1(surface)의 spot 선택을 검사한다.
    for bit in (1,2,4,8,16,32,64,128):
        # 첫 칸/그 다음 칸/가운데/마지막 칸에서 충돌시켜 부분 쓰기를 확인한다.
        for offset in (0,1,4,8):
            index=(18+offset//3)*256+18+offset%3
            obj=(1,0,bit,3,3,3.,3.,4,0,0xffff,23)
            yield f'collision-{bit}-{offset}',False,0,[(index,bit)], [obj],[('Pop',1,20.,20.,0)]
    # 건물 양쪽 변의 표면 word와 영역 조회/알림의 순서를 비교한다.
    for support in (False,True):
        surfaces=[(1,0x800,4,1,1,1.,1.,4,0,0xffff,23),(2,0x800,4,1,1,1.,1.,4,0,0xffff,23)]
        building=(3,0,0x200,3,3,3.,3.,4,0,0xffff,23)
        yield f'surface-{support}',support,0,[],surfaces+[building],[
            ('Pop',1,18.,19.,0),('Pop',2,20.,19.,0),('Pop',3,20.,20.,0),('Unpop',3,0.,0.,0)]


def verify():
    """게임/기계어를 다시 실행하지 않고 검증 기록과 fixture·원본·도구의 일관성을 검사한다."""
    report=json.loads((ROOT/'cpppj/recovery-spatial-evidence.json').read_text(encoding='utf-8'))
    assert report['schema']==1 and report['functions']==PAIRS and report['reviewed_pairs']==[]
    assert report['classification']=='scoped-integration' and report['helpers']==HELPERS
    assert report['x87_checked_control_words']==[hex(c) for c in CONTROLS]
    assert report['assert_reports']=={'originals':0,'originalCD':0}
    assert report['replaced_dependencies']['virtual_offsets']==['0x88','0x8c','0x30','0x20']
    fixture=ROOT/'cpppj/tests/fixtures/spatial-x86.tsv'
    assert hashlib.sha256(fixture.read_bytes()).hexdigest()==report['fixture_sha256']
    # 해시는 최초 기록의 원본/생성 환경과 지금 검사의 입력이 같은지를 확인한다.
    for edition,sha in report['binary_sha256'].items():
        path=ROOT/edition/('Netstorm.exe' if edition=='originals' else 'NETSTORM.EXE')
        assert hashlib.sha256(path.read_bytes()).hexdigest()==sha,edition
    # 생성기와 공통 실행 도구도 최초 기록 이후 바뀌지 않았는지 검사한다.
    for path,sha in report['source_sha256'].items():
        assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==sha,path
    counts=collections.Counter()
    differences=0
    # 불완전 fixture·행 누락·판본별 결과 열 누락도 검사한다.
    for line in fixture.read_text(encoding='utf-8').splitlines():
        if not line or line.startswith('#'):
            continue
        row=line.split('\t')
        if row[0]=='Begin':
            assert len(row)==5
            counts['scenarios']+=1
        elif row[0]=='Object':
            assert len(row)==12
        else:
            assert row[0]=='Step' and len(row)==14 and row[1] in ('Pop','Unpop')
            counts[row[1].lower()]+=1
            differences+=int(row[6:10]!=row[10:14])
    assert dict(counts)==report['cases']
    assert counts['pop']+counts['unpop']==report['total_cases'] and differences==report['edition_differences']
    print(json.dumps({'verified':True,'cases':dict(counts),'edition_differences':differences}),flush=True)


def main():
    """두 판본과 x87 정밀도별 실제 결과·제한·SHA를 저장한다."""
    oracles=[SpatialOracle('originals'),SpatialOracle('originalCD')]
    rows=['# Pop/Unpop 기계어: 공간 갱신만. 화면/firstPop/postPop/영역/알림은 계약 대체.']
    counts=collections.Counter()
    differences=0
    # 각 입력 시퀀스의 객체와 순서 있는 전이를 별도 fixture 행으로 저장한다.
    for case,supported,first_flags,seed,objects,steps in scenarios():
        rows.append(f'Begin\t{case}\t{int(supported)}\t{first_flags}\t'+(';'.join(f'{i}:{v}' for i,v in seed) or '-'))
        # 테스트가 같은 타입/객체 입력을 사용할 수 있도록 입력 비트를 기록한다.
        for obj in objects:
            values=[*obj[:5],float_bits(obj[5]),float_bits(obj[6]),*obj[7:]]
            rows.append('Object\t'+'\t'.join(map(str,values)))
        results=[]
        # 상태 전이가 있는 시퀀스 전체를 각 정밀도에서 다시 초기화한다.
        for control in CONTROLS:
            edition_results=[]
            # 각 판본을 별도 초기 상태에서 실행하여 앞선 시퀀스의 흔적을 없앤다.
            for oracle in oracles:
                oracle.control=control
                oracle.begin(supported,first_flags,seed)
                # 원본 번호 풀에 실제 입력 객체를 넣고 전이 시퀀스를 실행한다.
                for obj in objects:
                    oracle.add(*obj)
                edition_results.append([oracle.step(*step) for step in steps])
            results.append(edition_results)
        assert results[0]==results[1],case
        # 판본 차이를 숨기지 않고 각각의 출력 열로 저장한다.
        for index,step in enumerate(steps):
            name,sid,x,y,flags=step
            a,b=results[0][0][index],results[0][1][index]
            differences+=int(a!=b)
            rows.append('\t'.join(['Step',name,str(sid),str(float_bits(x)),str(float_bits(y)),str(flags),*a,*b]))
            counts[name.lower()]+=1
        counts['scenarios']+=1
        if counts['scenarios']%50==0:
            print(f"공간 시퀀스 {counts['scenarios']}개 완료",flush=True)
    assert all(oracle.assertions==0 for oracle in oracles)
    fixture=ROOT/'cpppj/tests/fixtures/spatial-x86.tsv'
    fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    sources=['tools/decomp_spatial_oracle.py','tools/decomp_bridge_oracle.py','tools/decomp_oracle.py']
    report={'schema':1,'classification':'scoped-integration','functions':PAIRS,'reviewed_pairs':[], 'helpers':HELPERS,
            'cases':dict(counts),'total_cases':counts['pop']+counts['unpop'],
            'edition_differences':differences,'x87_checked_control_words':[hex(c) for c in CONTROLS],
            'binary_sha256':{o.edition:o.sha256 for o in oracles},
            'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),
            'source_sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sources},
            'assert_reports':{o.edition:o.assertions for o in oracles},
            'replaced_dependencies':{'virtual_offsets':['0x88','0x8c','0x30','0x20'],
                'firstPop':'입력 반환 비트만; 실제 파생 객체 미실행',
                'postPop':'호출 비트 기록·curPostPop 감소 계약만',
                'region':'입력 bool만; 소유자/영역 계산 미실행','notify':'SID 기록만; 화면/부착 효과 미실행'},
            'limits':['SID/pool/타입/SHP/지도는 격리 입력; 할당/세대/삭제 미실행',
                'dirty 큐·Battle 복제 비활성, 별도 grid 조회 지도는 비어 있음',
                'Pop/Unpop 정상 반환 확인은 위 의존성 대체 조건부이며 전체 함수 검토 앵커로 추가하지 않음',
                '유효 발자국·비 contained 객체만; overlap 조기 반환의 부분 spot 변경도 보존',
                '실제 게임/원본 복사본/GUI 실행 없음']}
    (ROOT/'cpppj/recovery-spatial-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'cases':dict(counts),'edition_differences':differences,'assert_reports':report['assert_reports']}),flush=True)


# 직접 실행한 경우에만 fixture를 생성한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true',help='기록·fixture·원본·도구 해시만 검사한다')
    args=parser.parse_args()
    if args.verify:
        verify()
    else:
        main()
