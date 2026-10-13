#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 두 PE의 지형 flood/프레임/경계 후보를 격리된 Unicorn x86으로 관찰한다."""
import argparse
import collections
import csv
import hashlib
import itertools
import json
import struct
import sys
from pathlib import Path

# 저장소와 분석용 의존성은 원본 게임의 실행 환경과 분리한다.
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_INVALID
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW

# 모든 주소는 에뮬레이터 안의 합성 저장소이며 호스트 포인터가 아니다.
TYPES, POOL, CODES, HEAP, LISTS, GRID, DRAW = 0x11100000, 0x12000000, 0x16000000, 0x17000000, 0x18000000, 0x19000000, 0x1a000000
STOP, OWNER, STACK_BASE, STACK = 0x20000000, 0x20001000, 0x30000000, 0x3003f000
# 실제 지면·edgeFarm·잘못된 표면 타입과 입력 SID 배치를 고정한다.
TERRAIN, EDGE, OTHER = 83, 84, 85
POINTS = [(10,10),(11,10),(10,11),(11,11),(12,10),(12,11)]
# 각각 빈 지도, 단일 칸, 긴 연결, 사각형, owner 장애, 잘못된 시작 표면이다.
SCENES = [[],[0],[0,1,4],[0,1,2,3],[0,1,2,3,4,5],[0,1,2,3,4,5]]
# 프레임0은 AA, 1..144는 JA, 마지막은 비연결 PP다. 실제 J 시작 표는1이다.
FRAME_CODES = [bytes((65,65,0,0))] + [bytes((74,65,i&255,0)) for i in range(144)] + [bytes((80,80,i,0)) for i in range(12)]
# 실제 entry/전역/필드와 도장의 primitive 경계를 판본별로 지정한다.
SPECS = {
 'originals':dict(binary='originals/Netstorm.exe',entry=0x470900,stride=50,type_stride=500,pool=0x5c8464,types=0x59ab20,grid=0x5c84bc,isle=0x5411dc,edge=0x54123c,revision=0x59a8b0,rng=0x532710,owner=34,frame=36,core=0x568b2c,cache=0x568b30,boundary=0x55a6f8,affected=0x55a718),
 'originalCD':dict(binary='originalCD/NETSTORM.EXE',entry=0x4bd2d0,stride=36,type_stride=468,pool=0x5395dc,types=0x51c960,grid=0x52d590,isle=0x51cac8,revision=0x52039c,owner=32,frame=34),
}
# 생성 결과와 바이트 근거는 다른 작업의 fixture와 분리한다.
FIXTURE = ROOT/'cpppj/tests/fixtures/regionpaint-x86.tsv'
REPORT = ROOT/'cpppj/regionpaint.json'

def digest(path):
    """원본/도구/기록의 SHA-256을 반환한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def inputs(edition):
    """기대 결과를 계산하지 않고 지도/기존 owner/프레임/요청만 교차한다."""
    themes = (0,1,2,3,0xffffffff) if edition=='originals' else (0,)
    # 양 판본에 의미가 같은 입력을 주되 CD에서 무시되는 theme을 중복 합산하지 않는다.
    for scene,variant,owner,theme in itertools.product(range(6),range(3),(0,1,2,8),themes):
        yield scene,variant,owner,theme,10,10
    # 같은 소유자의 기존 장벽과 다른 시작 칸에서 깊이 우선 순회를 관찰한다.
    for variant,owner,(x,y) in itertools.product((0,2),(0,2),POINTS):
        yield 4,variant,owner,0,x,y
    # 네 범위 검사와 최대 유효 좌표의 빈 칸 반환도 별도로 관찰한다.
    for x,y in ((-1,10),(10,-1),(256,10),(10,256),(255,255),(0,0)):
        yield 0,0,1,0,x,y

class PaintOracle:
    """실제 flood/selection/PRNG/영향 목록 중복 검사는 실행하고 가상 효과·할당만 격리한다."""
    def __init__(self, edition):
        """현재 호스트의 명시 함수 범위와 실제 PE를 읽기만 하여 준비한다."""
        self.edition=edition;self.p=SPECS[edition];p=self.p
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);pe=pefile.PE(str(ROOT/p['binary']));data=pe.get_memory_mapped_image()
        self.mu.mem_map(pe.OPTIONAL_HEADER.ImageBase,(len(data)+4095)&~4095);self.mu.mem_write(pe.OPTIONAL_HEADER.ImageBase,data)
        # 합성 슬롯/타입/지도/캐시/CRT stack은 서로 겹치지 않는다.
        for address,size in ((0,4096),(TYPES,0x20000),(POOL,0x10000),(CODES,0x10000),(HEAP,0x10000),(LISTS,0x20000),(GRID,0x20000),(DRAW,4096),(STOP,0x2000),(STACK_BASE,0x40000)):
            self.mu.mem_map(address,size)
        self.allowed=[];self.exports=[ROOT/f'extracted/regionpaint/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with self.exports[1].open(encoding='utf-8') as fp:
            # Ghidra가 계산한 실제 불연속 몸체만 native 명령으로 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                for part in row['ranges'].split(';'):
                    lo,hi=(int(v,16) for v in part.split('-'));self.allowed.append((lo,hi+1))
        self.mu.hook_add(UC_HOOK_CODE,self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_INVALID,self.invalid_memory)
        self.mu.mem_write(p['pool'],struct.pack('<I',POOL));self.mu.mem_write(p['types'],struct.pack('<I',TYPES));self.mu.mem_write(p['grid'],struct.pack('<I',GRID));self.mu.mem_write(p['isle'],struct.pack('<I',TERRAIN))
        self.mu.mem_write(CODES,b''.join(FRAME_CODES));self.mu.mem_write(CODES+0x2000+0x74,struct.pack('<I',OWNER))
        # 실제 타입 getter와 frame selector가 읽는 구조체 입력이다.
        for number in (TERRAIN,EDGE,OTHER):
            base=TYPES+number*p['type_stride'];self.mu.mem_write(base+0x114,struct.pack('<I',len(FRAME_CODES)));self.mu.mem_write(base+0x124,struct.pack('<I',CODES));self.mu.mem_write(base+0x154,struct.pack('<I',1));self.mu.mem_write(base+0xec,struct.pack('<I',2 if number==TERRAIN else 0))
        if edition=='originals':
            self.mu.mem_write(p['edge'],struct.pack('<I',EDGE));self.mu.mem_write(0x50c038,struct.pack('<I',127))
        self.calls=collections.Counter();self.returns=0

    def return_from(self,purge=0,value=0):
        """명시 외부 경계의 x86 반환 주소·callee 정리를 유지한다."""
        mu=self.mu;sp=mu.reg_read(UC_X86_REG_ESP);target=struct.unpack('<I',mu.mem_read(sp,4))[0]
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,target)

    def invalid_memory(self,mu,access,address,size,value,data):
        """미매핑 접근의 명령/독립 입력을 보고하고 분석 범위를 임의 확장하지 않는다."""
        raise RuntimeError(f'미매핑 접근 {address:08x}/{size}, eip={mu.reg_read(UC_X86_REG_EIP):08x}, case={self.case}')

    def instruction(self,mu,address,size,data):
        """원본 명령 범위를 제한하고 외부 효과의 입력/반환과 실제 저장소 쓰기를 관찰한다."""
        p=self.p;sp=mu.reg_read(UC_X86_REG_ESP)
        if address==OWNER:
            sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//p['stride'];owner=struct.unpack('<I',mu.mem_read(sp+4,4))[0]
            self.events.append(f'O:{sid}:{owner}');mu.mem_write(POOL+sid*p['stride']+p['owner'],bytes((owner&255,)));self.calls['virtual_owner']+=1;self.return_from(4);return
        if self.edition=='originals':
            if address in (0x4e66e8,0x4e4391):
                count=struct.unpack('<I',mu.mem_read(sp+4,4))[0];result=self.heap;self.heap+=(count+15)&~15
                if self.heap>HEAP+0x10000:raise RuntimeError('분석 heap 범위 초과')
                self.calls['allocate']+=1;self.return_from(value=result);return
            if address in (0x4e4354,0x4437f0):
                self.calls['free' if address==0x4e4354 else 'allocation_check']+=1;self.return_from();return
            if address==0x46fd20:
                if mu.reg_read(UC_X86_REG_ECX)!=p['boundary']:raise RuntimeError('boundary clear this 오류')
                begin=struct.unpack('<I',mu.mem_read(p['boundary']+4,4))[0];mu.mem_write(p['boundary']+8,struct.pack('<I',begin));self.calls['vector_clear']+=1;self.return_from(12);return
            if address==0x46fa30:
                target,count,source=struct.unpack('<3I',mu.mem_read(sp+4,12))
                if count!=1:raise RuntimeError('boundary append count 오류')
                mu.mem_write(target,bytes(mu.mem_read(source,8)));self.calls['vector_copy']+=1;self.return_from();return
            if address==0x455280:
                source=struct.unpack('<I',mu.mem_read(sp+4,4))[0];end=struct.unpack('<I',mu.mem_read(p['affected']+8,4))[0]
                mu.mem_write(end,bytes(mu.mem_read(source,12)));mu.mem_write(p['affected']+8,struct.pack('<I',end+12));self.calls['affected_append']+=1;self.return_from(4);return
            if address==0x4b1fa0:
                x,y,type_number=struct.unpack('<2fI',mu.mem_read(sp+4,12));sid=self.fallback.get((int(x),int(y)),0)
                if type_number!=EDGE:raise RuntimeError('edgeFarm finder 타입 오류')
                self.events.append(f'F:{int(x)}:{int(y)}:{sid}');self.calls['edge_finder']+=1;self.return_from(value=sid);return
        if not any(lo<=address<hi for lo,hi in self.allowed):raise RuntimeError(f'허용 범위 밖 native 명령 {address:08x}')

    def run(self,case,control):
        """모든 반환/레지스터/x87과 전체 슬롯·원본 전역 출력을 직접 읽는다."""
        self.case=case;scene,variant,owner,theme,start_x,start_y=case;mu=self.mu;p=self.p;patch=self.edition=='originals'
        self.events=[];self.heap=HEAP;self.fallback={(9,10):70,(12,12):71};mu.mem_write(GRID,bytes(0x20000));mu.mem_write(POOL,bytes(0x10000))
        frame=(0,145,3)[variant]
        # 저장 좌표와 owner 장벽을 포함한 기대값 없는 입력 슬롯이다.
        for index in SCENES[scene]:
            x,y=POINTS[index];sid=50+index;raw=bytearray(p['stride']);struct.pack_into('<I',raw,0,CODES+0x2000);raw[10]=OTHER if scene==5 and index==0 else TERRAIN;raw[p['owner']]=(index%3+variant)%3;struct.pack_into('<H',raw,8,7 if x<12 else 9);struct.pack_into('<2f',raw,14,x+0.125,y+0.25)
            if patch:struct.pack_into('<I',raw,p['frame'],frame)
            else:raw[p['frame']]=frame
            mu.mem_write(POOL+sid*p['stride'],bytes(raw));mu.mem_write(GRID+(y*256+x)*2,struct.pack('<H',sid))
        # edgeFarm이 만드는 확장 후보는 일반 표면에 등록하지 않는다.
        for (x,y),sid in self.fallback.items():
            raw=bytearray(p['stride']);struct.pack_into('<I',raw,0,CODES+0x2000);raw[10]=EDGE;raw[p['owner']]=2;struct.pack_into('<2f',raw,14,x+0.125,y+0.25)
            if patch:struct.pack_into('<I',raw,p['frame'],0)
            else:raw[p['frame']]=0
            mu.mem_write(POOL+sid*p['stride'],bytes(raw))
        mu.mem_write(DRAW,struct.pack('<7I',CODES,0xc61c4000,0xc61c4000,0xc61c4000,0xc61c4000,6,3));mu.mem_write(p['revision'],struct.pack('<I',0xffffffff))
        if patch:
            mu.mem_write(p['rng'],struct.pack('<I',0x23456789));mu.mem_write(p['core'],bytes(4));mu.mem_write(p['cache'],bytes(4));mu.mem_write(p['boundary'],struct.pack('<4I',0,LISTS,LISTS,LISTS+0x10000));mu.mem_write(p['affected'],struct.pack('<4I',0,LISTS+0x10000,LISTS+0x1000c,LISTS+0x18000));mu.mem_write(LISTS+0x10000,struct.pack('<3I',0x3f800000,0x40000000,7))
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        # 원본 ABI의 callee 보존과 빈 x87 stack을 독립적으로 검사한다.
        for reg,value in preserved.items():mu.reg_write(reg,value)
        mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_ECX,DRAW);mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EDX,0);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        args=(STOP,start_x&0xffffffff,start_y&0xffffffff,owner,theme) if patch else (STOP,start_x&0xffffffff,start_y&0xffffffff,owner);mu.mem_write(STACK,struct.pack('<'+'I'*len(args),*args))
        try:
            mu.emu_start(p['entry'],STOP,count=100000)
        except Exception as error:
            raise RuntimeError(f'도장 native 입력 {case}, eip={mu.reg_read(UC_X86_REG_EIP):08x}, eax={mu.reg_read(UC_X86_REG_EAX):08x}, ecx={mu.reg_read(UC_X86_REG_ECX):08x}, esp={mu.reg_read(UC_X86_REG_ESP):08x}') from error
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+len(args)*4:raise RuntimeError('도장 정상 반환/ESP 오류')
        if any(mu.reg_read(reg)!=value for reg,value in preserved.items()) or mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('도장 보존 레지스터/x87 오류')
        self.returns+=1;objects=[]
        # 결과는 원본 슬롯에서만 읽으며 Python으로 도장 결과를 계산하지 않는다.
        for sid in list(range(50,56))+[70,71]:
            raw=bytes(mu.mem_read(POOL+sid*p['stride'],p['stride']));f=struct.unpack_from('<I',raw,p['frame'])[0] if patch else raw[p['frame']];objects.append(f'{sid}:{raw[p["owner"]]}:{f}')
        boundary=[];affected=[]
        if patch:
            begin,end=struct.unpack('<2I',mu.mem_read(p['boundary']+4,8));boundary=[f'{x}:{y}' for x,y in struct.iter_unpack('<2i',bytes(mu.mem_read(begin,end-begin)))];begin,end=struct.unpack('<2I',mu.mem_read(p['affected']+4,8));affected=[':'.join(map(str,row)) for row in struct.iter_unpack('<3I',bytes(mu.mem_read(begin,end-begin)))]
        drawer=struct.unpack('<6I',mu.mem_read(DRAW+4,24));revision=struct.unpack('<I',mu.mem_read(p['revision'],4))[0];rng=struct.unpack('<I',mu.mem_read(p['rng'],4))[0] if patch else 0
        return [';'.join(self.events) or '-',','.join(objects),','.join(boundary) or '-',','.join(affected) or '-',':'.join(map(str,drawer)),revision,rng]

def generate(smoke=False):
    """같은 입력의 두 x87 정밀도가 일치한 실제 native 결과와 SHA 근거를 저장한다."""
    rows=[];reports={}
    # 판본마다 실제 함수의 독립 입력을 실행한다.
    for edition in SPECS:
        oracle=PaintOracle(edition);cases=list(dict.fromkeys(inputs(edition)));cases=cases[:3] if smoke else cases
        for case in cases:
            first=oracle.run(case,0x27f);second=oracle.run(case,0x37f)
            if first!=second:raise RuntimeError('지형 도장 x87 정밀도 결과 차이')
            rows.append([edition,*case,*first])
        reports[edition]=dict(cases=len(cases),normal_returns=oracle.returns,calls=dict(oracle.calls))
    if not smoke:
        FIXTURE.write_text('# 실제 PE 지역 도장: 판본 scene variant owner theme x y events slots boundary affected drawer revision rng\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
        paths=[Path(__file__),FIXTURE,ROOT/'tools/ghidra/regionpaint-functions.json',ROOT/'cpppj/src/o/RawRegionPaint.h',ROOT/'cpppj/src/o/RawRegionPaint.cpp']
        # 실제 PE와 이번 호스트의 내보내기를 각 근거에 포함한다.
        for edition in SPECS:paths.extend([ROOT/SPECS[edition]['binary'],ROOT/f'extracted/regionpaint/{edition}/creation.c',ROOT/f'extracted/regionpaint/{edition}/functions.tsv'])
        REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',date='2026-10-13',cases=reports,total=len(rows),files={p.relative_to(ROOT).as_posix():digest(p) for p in paths},limitations=['virtual owner side effects supplied as explicit callback; byte write observed','CRT allocation/vector copy and edgeFarm finder are declared boundaries; actual flood/PRNG/selection/deduplication execute','synthetic terrain/code/map inputs; no GameWorld or original game/GUI execution']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(total=len(rows),editions=reports),ensure_ascii=False))

def verify():
    """저장한 원본/도구/fixture SHA와 고유 입력 수를 확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in report['files'].items():
        if digest(ROOT/name)!=value:raise RuntimeError(f'SHA 불일치 {name}')
    rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows)!=report['total']:raise RuntimeError('지역 도장 행 수 불일치')
    keys=[tuple(row.split('\t')[:7]) for row in rows]
    if len(set(keys))!=len(rows):raise RuntimeError('지역 도장 중복 입력')
    # 각 판본의 정상 반환은 동일 입력의 x87 53/64비트 실행을 별도로 센다.
    for edition,expected in report['cases'].items():
        count=sum(key[0]==edition for key in keys)
        if count!=expected['cases'] or expected['normal_returns']!=count*2:raise RuntimeError('지역 도장 판본/반환 수 불일치')
    print(f'지역 도장 감사 통과: {len(rows)}개')

def main():
    """원본 실행 관찰 생성·스모크·저장 근거 감사 중 하나를 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)

if __name__=='__main__':
    main()
