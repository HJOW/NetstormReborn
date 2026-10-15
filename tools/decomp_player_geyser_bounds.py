#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 Player/청크 record/실제 Canon 배치 사슬을 보호 PE에서 독립 실행한다."""
import argparse
import collections
import csv
import json
import struct
from pathlib import Path
import decomp_missionframe_oracle as common
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

# 실제 원문과 명시 입력/출력 메모리이며 게임/OS 프로세스를 시작하지 않는다.
ROOT=common.ROOT
TYPES,PLAYERS,CODES,OUTPUT,RECORD=0x13000000,0x14000000,0x15000000,0x16000000,0x16000100
CHUNKS,STOP,STACK=0x54c8a0,common.STOP,common.STACK
FIXTURE=ROOT/'cpppj/tests/fixtures/player-geyser-bounds-x86.tsv'
REPORT=ROOT/'cpppj/player-geyser-bounds.json'
KEYS=('op','owner','shape','rotation','position','region','flags','battle','mission','metamap','mapSize','layout','mapDimensions','foot','frames','mutation','directions')


def case(**changes):
    """예상 사각형 없이 현재 원본 입력만 구성한다."""
    result=dict(zip(KEYS,('B',1,0,0,0x32,7,2,1,1,0,0,0,0,0,0,0,0)));result.update(changes);return result


def cases():
    """64패턴/회전, 전투옵션, 검색순서/부호, 발자국과 진단 후 재조회를 명시한다."""
    result=[]
    # 실제 record의 하위6비트와 상위2비트 전 조합이다.
    for shape in range(64):
        # 위치도 입력으로 변형하지만 정답 좌표는 계산하지 않는다.
        for rotation in range(4):result.append(case(shape=shape,rotation=rotation,position=(shape*37+rotation*13)&255))
    # Player1~8의 실제 방향표와 옵션0~3을 모두 조회한다.
    for owner in range(1,9):
        # 미션/비미션 양쪽에서 현재 Player를 직접 사용한다.
        for map_size in range(4):
            # 미션에서는 mapSize가 사각형 결정에 사용되지 않음을 포함한다.
            for mission in (0,1):result.append(case(owner=owner,mapSize=map_size,mission=mission,shape=17))
    # signed BYTE 비교, 첫 y행/x열 일치 및 record 부재다.
    for layout in range(6):
        # regionDWORD의 부호/바이트 축약 여부를 분리한다.
        for region in (-128,-1,0,7,127,128,255):result.append(case(op='R',layout=layout,region=region))
    # 기존 TYPE 발자국과 회전된 실제 패턴 폭/높이를 사용한다.
    for foot in range(4):
        # direct 배치에서도 원본 Canon 하위를 거친다.
        for rotation in range(4):result.append(case(op='P',shape=14,foot=foot,rotation=rotation,position=255))
    # 원본 flags/전투/메타맵의 진단을 보고한 뒤 계속한다.
    for flags in (0,1,2,32,34,255):
        # battle진단과 metamap진단을 각각 독립 입력한다.
        for battle in (0,1):
            # 실제 Player0은 배치에서 추가 진단되지만 미션에서는 계속 계산한다.
            for metamap in (0,1):result.append(case(flags=flags,battle=battle,metamap=metamap))
    # 모든 프레임 부재에서 진단 뒤 크기 계산이 유지된다.
    for shape in (0,3,17,63):
        # 같은 기존 CanonDecoder의 네 회전을 검사한다.
        for rotation in range(4):result.append(case(shape=shape,rotation=rotation,frames=1))
    # 실제 최소/축소 지도와 Player0의 미션 전용 자연 진행이다.
    for dimensions in (1,2):result.append(case(mapDimensions=dimensions))
    for owner in (-2147483648,-1,0,9,2147483647):result.append(case(op='P',owner=owner))
    result.append(case(owner=0))
    # 현재 방향표도 기본 배치 순서로 고정하지 않는다.
    for owner in range(1,9):result.append(case(owner=owner,mission=0,directions=1,mapSize=3))
    # 앞선 진단 효과 뒤 현재 region/모드/발자국을 다시 읽는 순서를 확인한다.
    result.extend([case(flags=0,layout=3,mutation=1),case(metamap=1,mutation=2),case(battle=0,mission=0,mutation=3),case(frames=1,mutation=4),case(metamap=1,mutation=5)])
    unique={}
    # 동일 입력을 새 관찰 수에 다시 세지 않는다.
    for value in result:unique.setdefault(json.dumps(value,sort_keys=True),value)
    return list(unique.values())


def map_dimensions(profile):
    """유효 ChunkMap의 명시 가로/세로 입력이다."""
    return [(16,16),(3,4),(1,1)][profile]


def record(value,alternate=False):
    """현재 record6BYTE의 실제 입력이다. 출력 범위를 계산하지 않는다."""
    return bytes([((value['shape']+(7 if alternate else 0))&63)|(value['rotation']<<6),0xa5,value['position']^(0x55 if alternate else 0),17,29,41])


def map_entries(value):
    """첫 일치/중복/부재/음수BYTE를 분리하는 raw 지도 입력이다."""
    width,height=map_dimensions(value['mapDimensions']);first=(min(2,width-1),min(3,height-1));layout=value['layout'];items=[]
    if layout==0:items=[(*first,7,record(value))]
    elif layout==1:items=[(min(3,width-1),0,7,record(value)),(0,min(3,height-1),7,record(value,True))]
    elif layout==3:items=[(*first,7,record(value)),(0,0,8,record(value,True))]
    elif layout==4:items=[(*first,255,record(value))]
    elif layout==5:items=[(*first,128,record(value))]
    return items


class Oracle:
    """선택한17함수만 실행하고 assert만 명시 외부 관찰 경계로 둔다."""
    def __init__(self):
        """보호 PE와 현재 선택 export의 정확 범위를 준비한다."""
        if common.digest(common.BINARY)!=common.SHA:raise RuntimeError('Player 범위 보호PE SHA불일치')
        pe=common.pefile.PE(str(common.BINARY));self.data=pe.get_memory_mapped_image();self.base=pe.OPTIONAL_HEADER.ImageBase
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);self.mu.mem_map(self.base,(len(self.data)+4095)&~4095);self.mu.mem_write(self.base,self.data)
        # 입력/출력/프레임과 실제 caller stack만 따로 매핑한다.
        for address,size in ((TYPES,0x20000),(PLAYERS,0x1000),(CODES,0x1000),(OUTPUT,0x1000),(STOP,0x1000),(common.STACK_BASE,0x10000)):self.mu.mem_map(address,size)
        self.table=ROOT/'extracted/player-geyser-bounds/originals/functions.tsv';self.allowed=[];self.entries={}
        with self.table.open(encoding='utf-8') as fp:
            # 같은 정밀 export의 range만 허용하며 새 바이트 대체는 없다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries[int(row['entry'],16)]=row['entry']
                # 분리된 원본 함수 범위도 각각 확인한다.
                for part in row['ranges'].split(';'):
                    low,high=(int(item,16) for item in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
        self.native=collections.Counter();self.returns=0;self.assertions=0

    def get(self,address,width=4):
        """현재 실제 폭의 원문 비트를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self,address,value,width=4):
        """명시 입력/진단 효과만 원본 폭으로 쓴다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def instruction(self,mu,address,size,data):
        """실제 진입/진단 순서를 관찰하고 나머지 함수/OS 호출은 거부한다."""
        if address in self.entries:self.native[self.entries[address]]+=1
        if address==0x4e0620:
            esp=mu.reg_read(UC_X86_REG_ESP);line=self.get(esp+12);kind={0x13b:0,0x134:1,0x3b:2,0x3c:3,299:4}.get(line)
            if kind is None:raise RuntimeError('미지정Player범위진단 '+str(line))
            self.trace.append(str(kind));self.assertions+=1
            if not self.mutated and self.c['mutation']:
                self.mutated=True;mutation=self.c['mutation'];player=PLAYERS+self.player*0xb4
                if mutation==1:self.put(player+0x78,8)
                elif mutation==2:self.put(0x594fa0,0);self.put(0x52f7e6,3,1)
                elif mutation==3:self.put(0x594fa0,1);self.put(TYPES+107*500+0x1d4,3)
                elif mutation==4:self.put(TYPES+107*500+0x1d4,2);self.put(TYPES+107*500+0x1d8,3);self.put(0x541204,108)
                elif mutation==5:
                    width,height=map_dimensions(self.c['mapDimensions']);mu.mem_write(CHUNKS+14+(min(2,width-1)*16+min(3,height-1))*12,record(self.c,True))
            mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EIP,self.get(esp));mu.reg_write(UC_X86_REG_ESP,esp+4);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'미허용Player범위실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """실제 몸체는 stack과 명시 출력16BYTE 밖을 변경하지 않는다."""
        if not (common.STACK_BASE<=address and address+size<=common.STACK_BASE+0x10000 or OUTPUT<=address and address+size<=OUTPUT+16):raise RuntimeError(f'미허용Player범위쓰기 {address:08x}')

    def run(self,value):
        """같은 native 입력에서 출력 전체16BYTE/현재 상태/진단 순서를 얻는다."""
        self.c=dict(value);self.trace=[];self.mutated=False;self.player=value['owner'] if 0<=value['owner']<9 else 1;mu=self.mu
        mu.mem_write(TYPES,bytes(0x20000));mu.mem_write(PLAYERS,bytes(0x1000));mu.mem_write(OUTPUT,struct.pack('<4I',*[0xdeadbeef]*4))
        self.put(0x59ab20,TYPES);self.put(0x541348,188);self.put(0x59534c,PLAYERS);self.put(0x541204,107);self.put(0x5e4794,0)
        self.put(0x594fbc,value['battle']);self.put(0x594fa0,value['mission']);self.put(0x594fc8,value['metamap']);self.put(0x52f7e6,value['mapSize'],1)
        feet=[(1,1),(2,3),(0,1),(-1,2)][value['foot']];codes=bytes() if value['frames'] else b''.join(bytes((side,80,number,0)) for side in range(65,81) for number in range(11))
        mu.mem_write(CODES,codes or bytes(4))
        # 기본 타입과 진단 효과에서 선택할 타입은 실제 같은 자료에 존재한다.
        for typ in (107,108):self.put(TYPES+typ*500+0x1d4,feet[0]);self.put(TYPES+typ*500+0x1d8,feet[1]);self.put(TYPES+typ*500+0x114,len(codes)//4);self.put(TYPES+typ*500+0x124,CODES)
        self.put(PLAYERS+self.player*0xb4+0x50,value['flags'],1);self.put(PLAYERS+self.player*0xb4+0x78,value['region'])
        width,height=map_dimensions(value['mapDimensions']);mu.mem_write(CHUNKS,bytes(12+16*16*12));self.put(CHUNKS,1);self.put(CHUNKS+4,width);self.put(CHUNKS+8,height)
        # 실제 구조의 x바깥/y안쪽 물리 항목을 입력하고 검색 순서는 몸체가 정한다.
        for x in range(16):
            # 기본 영역127의 record도 stale문자가 있는 명시 값이다.
            for y in range(16):self.put(CHUNKS+12+(x*16+y)*12,127,1);mu.mem_write(CHUNKS+14+(x*16+y)*12,bytes((31,41,59,26,53,58)))
        # 일부 current record만 덮어써 첫 일치를 분리한다.
        for x,y,region,raw in map_entries(value):self.put(CHUNKS+12+(x*16+y)*12,region,1);mu.mem_write(CHUNKS+14+(x*16+y)*12,raw)
        directions=[0,4,0,6,2,7,3,5,1]
        if value['directions']:directions=[(item+2)&7 for item in directions]
        # 실제 전역 방향표의 현재 입력을 매 실행 초기화한다.
        for index,direction in enumerate(directions):self.put(0x53181c+index*4,direction)
        mu.mem_write(RECORD,record(value));entry=0x48f5c0;ecx=PLAYERS+self.player*0xb4;args=[OUTPUT];cleanup=4
        if value['op']=='R':entry=0x432430;ecx=CHUNKS;args=[OUTPUT,value['region']&0xffffffff];cleanup=8
        elif value['op']=='P':entry=0x432050;ecx=0;args=[OUTPUT,value['owner']&0xffffffff,*struct.unpack('<II',record(value)+bytes(2))];cleanup=0
        # 현재 원본 FPU/보존 레지스터를 초기화한다.
        for reg,sentinel in common.SAVED.items():mu.reg_write(reg,sentinel)
        mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EDX,0);mu.reg_write(UC_X86_REG_ECX,ecx);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
        mu.reg_write(UC_X86_REG_FPCW,0x027f);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_FPTAG,0xffff)
        mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.emu_start(entry,STOP,count=2000000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+cleanup:raise RuntimeError('Player범위정상반환/스택오류')
        if any(mu.reg_read(reg)!=sentinel for reg,sentinel in common.SAVED.items()):raise RuntimeError('Player범위보존레지스터오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=0x027f or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('Player범위x87정상반환오류')
        self.returns+=1
        current=[self.get(address,width) for address,width in [(0x594fbc,4),(0x594fa0,4),(0x594fc8,4),(0x52f7e6,1),(0x541204,4),(TYPES+107*500+0x1d4,4),(TYPES+107*500+0x1d8,4),(PLAYERS+self.player*0xb4+0x78,4)]]
        return [','.join(str(self.get(OUTPUT+index*4)) for index in range(4)),','.join(map(str,current)),','.join(self.trace) or '-']


def generate():
    """기대값을 재계산하지 않고 보호 PE의 정상 출력과 근거 해시를 보존한다."""
    oracle=Oracle();rows=[]
    # 각 입력의 실제 최상위 정상 반환을 한 번 기록한다.
    for value in cases():rows.append([*[value[key] for key in KEYS],*oracle.run(value)])
    FIXTURE.write_text('# PATCH Player bounds 실제PE 독립관찰. GUI/OS실행0.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),Path(common.__file__),common.BINARY,FIXTURE,oracle.table,oracle.table.with_name('creation.c'),ROOT/'tools/ghidra/player-geyser-bounds-functions.json']
    REPORT.write_text(json.dumps(dict(host=common.socket.gethostname(),date='2026-10-15',total=len(rows),returns=oracle.returns,native=dict(oracle.native),assertions=oracle.assertions,os_calls=0,
        files={path.relative_to(ROOT).as_posix():common.digest(path) for path in paths},limits=['PATCH의유효ChunkMap/정렬Player0..8을실행한다.CD/손상포인터/물리표밖은범위밖이다.',
            'Player/record/placement/실제CanonDecoder/TYPE발자국/CRT와변환은기계어다.assert보고만외부경계다.','Session의Confirm/empty84삭제/실제미션연결은미완료다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('Player범위실제PE',len(rows),'정상반환',dict(oracle.native),flush=True)


def verify():
    """저장된 파일 해시/입력 계획/정상 반환 수를 원본 재실행 없이 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];plan=cases()
    # 도구/PE/선택 export/fixture의 전체 해시를 확인한다.
    for path,sha in report['files'].items():
        if common.digest(ROOT/path)!=sha:raise RuntimeError('Player범위근거SHA불일치 '+path)
    if len(rows)!=len(plan) or report['returns']!=len(rows) or report['os_calls']:raise RuntimeError('Player범위입력/정상반환수불일치')
    # 원본 출력은 재계산하지 않고 입력 계획의 열만 확인한다.
    for row,value in zip(rows,plan):
        if len(row)!=20 or row[:17]!=[str(value[key]) for key in KEYS]:raise RuntimeError('Player범위입력계획불일치')
    print('Player범위독립근거감사통과',len(rows),flush=True)


def main():
    """기계어 관찰 생성 또는 기존 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate()


if __name__=='__main__':main()
