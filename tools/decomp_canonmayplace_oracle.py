#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""전체 MayPlace를 실제 helper와 함께 정상 반환까지 실행한다. 임시 160바이트 메모리만 대체한다."""
import argparse
import csv
import itertools
import json
import struct
import zlib
from pathlib import Path
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,TYPES,POOL,PLAYERS,LISTS,STACK,STACK_BASE,STOP,digest
from decomp_canonplacement_oracle import PLACEMENT
from decomp_priestgeometry_oracle import GEOMETRY
from decomp_priestshape_oracle import SHAPE
from decomp_canonpermission_oracle import SPEC as PERMISSION
from decomp_canonterrain_oracle import SPEC as TERRAIN
from decomp_canonsurrounding_oracle import SPEC as SURROUNDING
from decomp_canonrelations_oracle import SPEC as RELATIONS
from decomp_priestcollision_oracle import SPEC as COLLISION
from decomp_playeranchor_oracle import SPEC as ANCHOR
from decomp_damageablepredestroy_oracle import CONTROLS
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 분석 전용 자료 주소와 실제 지도/풀 크기다.
GRID,FIXED,SPOTS,CODES,SHP,HEAP=0x13000000,0x13100000,0x13200000,0x13300000,0x13400000,0x13500000
CAPACITY=24000
SCALES=(1,2,4,16)
PATTERNS=(107,82,94,157,131,129,140,142)
FIXTURE=ROOT/'cpppj/tests/fixtures/canonmayplace-x86.tsv'
REPORT=ROOT/'cpppj/recovery-canonmayplace-evidence.json'
KEYS=('pattern','argument','direction','x','y','genus','flags','scene','owner','local','ready','editor','allies','relation','bypass','restricted','mode','force')


def bits(value):
    """좌표 입력의 float32 비트값을 보존한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def inputs():
    """기대 결과를 계산하지 않고 정상/거부/주변/특수 지역/경계 입력을 교차한다."""
    rows=[]
    # 일반 자산과 단일/3×3 패턴을 같은 장면·권한·관계 조건에 넣는다.
    for i,(genus,scene,profile) in enumerate(itertools.product((0,4,0x200,0x4000,0x10000,0x20000,0x210000,0x2000000),(0,1,2,3,4,5),(0,1))):
        rows.append(dict(pattern=0,argument=0,direction=0,x=bits(20),y=bits(21),genus=genus,flags=0x802,scene=scene,
            owner=1,local=1 if profile==0 else 2,ready=int(i%5!=0),editor=int(i%7==0),allies=profile,
            relation=profile,bypass=int(i%11==0),restricted=int(i%3==0),mode=i%2,force=0))
    # 모든 정상 회전과 지도 절삭 경계를 실제 전체 호출에서 실행한다.
    for i,(local,direction,scene) in enumerate(itertools.product((1,2),(0,2,4,6),(0,2,3,5))):
        rows.append(dict(rows[1],pattern=1,argument=0,direction=direction,scene=scene,local=local,genus=0x10000,
            relation=1,ready=1,mode=0,restricted=0,x=bits(20.25),y=bits(21.5)))
    # 조기 허용/지도 여백과 로컬 미리보기의 판본별 지도 끝 반환도 대조한다.
    for i,(position,force) in enumerate(itertools.product(((-1,21),(0,21),(1,21),(20.0002,21.5),(253,253),(254,254),(255,255),(256,21)),(0,1))):
        rows.append(dict(rows[1],genus=4,ready=1,relation=1,x=bits(position[0]),y=bits(position[1]),force=force,mode=0))
    return rows


class MayPlaceOracle(OwnerOracle):
    """전체 함수와 실제 helper를 허용하고 CRT 임시 배열만 관찰 대체한다."""
    def __init__(self,edition):
        """새 내보내기/물리 지도/프레임/전체 용량 풀을 준비한다."""
        super().__init__(edition);self.p=PLACEMENT[edition];self.gm=GEOMETRY[edition];self.a=ANCHOR[edition]
        self.mu.mem_map(0,0x1000)
        old=(128*self.stride+4095)&~4095;full=(CAPACITY*self.stride+4095)&~4095
        self.mu.mem_map(POOL+old,full-old)
        # 모든 분석 자료를 서로 겹치지 않는 별도 가상 영역에 둔다.
        for address,size in ((GRID,0x30000),(FIXED,0x20000),(SPOTS,0x10000),(CODES,0x10000),(SHP,0x100000),(HEAP,0x1000)):
            self.mu.mem_map(address,size)
        self.bases=[];address=GRID
        # 원본 current surface와 일반 finder의 0단계는 같은 배열이다.
        for scale in SCALES:
            self.bases.append(address);address+=(256//scale)**2*2
        self.grid_size=address-GRID
        self.mu.mem_write(RELATIONS[edition]['map']-12,struct.pack('<7I',0,1,256,*self.bases))
        self.mu.mem_write(TERRAIN[edition]['map'],struct.pack('<I',FIXED))
        self.mu.mem_write(SURROUNDING[edition]['spots'],struct.pack('<I',SPOTS))
        self.mu.mem_write(SURROUNDING[edition]['board'],struct.pack('<I',256))
        self.mu.mem_write(RELATIONS[edition]['capacity'],struct.pack('<I',CAPACITY))
        self.exports=[ROOT/f'extracted/canonmayplace/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.entries=set();self.code=bytearray(0x300000);self.substitutions={'allocate':0,'free':0};self.returns=0
        with self.exports[1].open(encoding='utf-8') as fp:
            # 함수 몸체로 허용된 실제 불연속 명령만 빠르게 검사할 표를 만든다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 각 물리 범위는 포함 끝 주소까지다.
                for part in row['ranges'].split(';'):
                    lo,hi=(int(x,16) for x in part.split('-'));self.code[lo-self.base:hi+1-self.base]=b'\x01'*(hi+1-lo)

    def on_instruction(self,mu,address,size,data):
        """반환/호출을 관찰하되 배치/Player/탐색 몸체는 대체하지 않는다."""
        if address in (self.a['allocate'],self.a['free']):
            sp=mu.reg_read(UC_X86_REG_ESP)
            if address==self.a['allocate']:
                if self.live or struct.unpack('<I',mu.mem_read(sp+4,4))[0]!=160:raise RuntimeError('임시 메모리 요청 오류')
                self.live=True;self.substitutions['allocate']+=1;value=HEAP
            else:
                pointer=struct.unpack('<I',mu.mem_read(sp+4,4))[0] if self.edition=='originals' else struct.unpack('<I',mu.mem_read(mu.reg_read(UC_X86_REG_ECX),4))[0]
                if not self.live or pointer!=HEAP:raise RuntimeError('임시 메모리 반납 오류')
                self.live=False;self.substitutions['free']+=1;value=0
            mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_EIP,struct.unpack('<I',mu.mem_read(sp,4))[0]);mu.reg_write(UC_X86_REG_ESP,sp+4);return
        self.instructions+=1
        if address==self.spec['assert_report']:
            self.assertions+=1;raise RuntimeError('전체 배치 원본 assert')
        offset=address-self.base
        if offset<0 or offset+size>len(self.code) or not all(self.code[offset:offset+size]):raise RuntimeError(f'허용하지 않은 전체 배치 주소: {self.edition} {address:08x}')
        if address in self.entries:self.native_calls[f'{address:08x}']+=1

    def type(self,number,flags,genus,width,height,codes,default=0,group=0,hotspots=(0,0),metrics=None):
        """합성 타입/물리 프레임 자료만 공급하며 선택/범위는 원본 함수에 맡긴다."""
        typ=TYPES+number*self.type_stride;raw=bytearray(self.type_stride);shape=SHP+number*0x1000;code=CODES+number*0x100
        struct.pack_into('<I',raw,0xdc,shape);struct.pack_into('<II',raw,0xe8,flags,genus)
        struct.pack_into('<i',raw,0x9c,group);struct.pack_into('<Ii',raw,0x114,len(codes)//4,default);struct.pack_into('<I',raw,0x124,code)
        struct.pack_into('<2i',raw,SHAPE[self.edition]['hot'],*hotspots)
        struct.pack_into('<2i',raw,self.gm['foot'],width,height);self.mu.mem_write(typ,bytes(raw));self.mu.mem_write(code,codes)
        # 논리 프레임은 원본과 같은 SHP 주소 표/별도 signed WORD 헤더를 가진다.
        header_start=0x2000 if metrics is not None else 0x100
        if metrics is None:metrics=[(16,11,8,5)]*(len(codes)//4)
        # 실제 사제의 긴 프레임 주소 표와 물리 헤더가 겹치지 않게 분리한다.
        for frame,metric in enumerate(metrics):
            head=header_start+frame*0x40;self.mu.mem_write(shape+8+frame*8,struct.pack('<II',head+0x24,0))
            self.mu.mem_write(shape+head+0x18,struct.pack('<4h',*metric))

    def run(self,case,control,asset=None):
        """전체 thiscall 정상 반환/ABI/x87와 표시/지역/읽기 전용 자료를 관찰한다."""
        mu=self.mu;p=self.p;t=TERRAIN[self.edition];r=RELATIONS[self.edition];perm=PERMISSION[self.edition];a=self.a
        self.live=False;self.instructions=0;mu.mem_write(0,bytes(4));mu.mem_write(POOL,bytes(128*self.stride));mu.mem_write(GRID,bytes(self.grid_size));mu.mem_write(FIXED,bytes(0x20000));mu.mem_write(SPOTS,bytes([6])*0x10000)
        placing=157 if case['pattern'] else 80
        codes=b''.join(bytes((side,ord('P'),1,0)) for side in (range(ord('A'),ord('I')+1) if case['pattern'] else (ord('A'),)))
        surface,workshop,obstacle=(10,11,12) if asset else (83,84,86)
        if asset:
            placing=asset['number'];self.type(placing,case['flags'],case['genus'],*asset['foot'],asset['codes'],asset['default'],asset['group'],asset['hotspots'],asset['metrics'])
        else:self.type(placing,case['flags'],case['genus'],1,1,codes)
        self.type(surface,0x800,2,1,1,b'PP\x01\x00');self.type(workshop,0x800,6,1,1,b'AP\x01\x00');self.type(obstacle,2,0,1,1,b'AP\x01\x00')
        # 고정 섬/현재 해시의 같은 칸을 별도 자료로 등록하며 씬은 입력만 바꾼다.
        sid=50
        for y in range(19,25):
            # 6×6 지면이 회전된 패턴/소수 좌표도 포함한다.
            for x in range(18,24):
                raw=bytearray(self.stride);raw[10]=surface;raw[self.o['owner']]=2 if case['scene']==2 else 1;raw[30 if self.edition=='originals' else 28]=7;raw[self.o['extra']]=1
                struct.pack_into('<H',raw,8,9);struct.pack_into('<II',raw,14,bits(x),bits(y));mu.mem_write(self.slot(sid),bytes(raw))
                if case['scene']!=3 or (x,y)!=(20,21):
                    mu.mem_write(self.bases[0]+2*(y*256+x),struct.pack('<H',sid));mu.mem_write(FIXED+2*(y*256+x),struct.pack('<H',sid))
                sid+=1
        raw=bytearray(self.stride);raw[10]=workshop;raw[self.o['owner']]=1;raw[30 if self.edition=='originals' else 28]=7;struct.pack_into('<II',raw,14,bits(18),bits(20));mu.mem_write(self.slot(90),bytes(raw))
        # 추가 충돌 후보는 1단계 해시에 실제 등록한다.
        if case['scene']==4:
            raw=bytearray(self.stride);raw[10]=obstacle;struct.pack_into('<II',raw,14,bits(20),bits(21));mu.mem_write(self.slot(91),bytes(raw))
            mu.mem_write(self.bases[1]+2*((21//2)*128+20//2),struct.pack('<H',91))
        if case['scene']==5:mu.mem_write(GRID,bytes(self.grid_size));mu.mem_write(FIXED,bytes(0x20000))
        mu.mem_write(LISTS,struct.pack('<I',90));mu.mem_write(PLAYERS+case['owner']*self.player_stride,struct.pack('<III',LISTS,1,int(case['scene']!=1)))
        mu.mem_write(a['extras'],struct.pack('<I',LISTS+0x100));mu.mem_write(a['count'],bytes(4))
        # 81칸 방향 표 중 아군/외국의 요청 방향 두 칸만 채운다.
        table=[0]*81;table[9+case['owner']]=case['relation'];table[18+case['owner']]=case['relation'];mu.mem_write(perm['relations'],struct.pack('<81I',*table))
        for key,value in (('ready',case['ready']),('editor',case['editor']),('allies',case['allies']),('anyOwner',0)):mu.mem_write(perm[key],struct.pack('<I',value))
        for key,value in (('invalid',254),('ignore',0),('contained',6),('boss',1)):mu.mem_write(a[key],struct.pack('<I',value))
        if self.edition=='originals':
            for key in ('checking','fence','archer'):mu.mem_write(a[key],bytes(4))
            data=bytearray(4+128*32);struct.pack_into('<I',data,0,128)
            # 실제 특수 지역 레코드의 제한/존재 필드다.
            for i in range(128):struct.pack_into('<II',data,4+i*32,case['restricted'],1)
            mu.mem_write(r['islands'],bytes(data))
        mu.mem_write(r['bypass'],struct.pack('<I',case['bypass']));mu.mem_write(p['force'],struct.pack('<I',case['force']));mu.mem_write(p['local'],struct.pack('<i',case['local']))
        mu.mem_write(p['blocked'],struct.pack('<I',73));mu.mem_write(t['regions'],b'\x55'*144);mu.mem_write(t['sentinel'],struct.pack('<I',127));mu.mem_write(t['noIsland'],struct.pack('<I',83))
        preview=0x59a9f0 if self.edition=='originals' else 0x565998;mu.mem_write(preview,b'\x55'*144)
        mu.mem_write(SHAPE[self.edition]['scale'],struct.pack('<2f',16,11))
        # 캡처하는 자료는 실제 원본 패턴 번호와 입력별 client/무시 전역이다.
        for address,value in zip(self.gm['patterns'],asset['patterns'] if asset else PATTERNS,strict=True):mu.mem_write(address,struct.pack('<I',value))
        mu.mem_write(COLLISION[self.edition]['client'],bytes(4))
        mu.mem_write(COLLISION[self.edition]['globalMask'],bytes(4))
        self.write_ranges=[(0,4),(HEAP,HEAP+160),(preview,preview+144),(t['regions'],t['regions']+144),(p['blocked'],p['blocked']+4)]
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        # 호출자 보존 레지스터를 서로 식별할 수 있는 값으로 준비한다.
        for reg,value in preserved.items():mu.reg_write(reg,value)
        mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EDX,0);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_ECX,TYPES+placing*self.type_stride)
        mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        mu.mem_write(STACK,struct.pack('<7I',STOP,*[case[k] for k in ('argument','x','y','direction','owner','mode')]))
        mu.emu_start(p['entry'],STOP,count=300000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+28 or self.live:raise RuntimeError('전체 배치 정상 반환/메모리 균형 오류')
        if any(mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('전체 배치 보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800 or struct.unpack('<I',mu.mem_read(0,4))[0]:raise RuntimeError('전체 배치 x87/FS 오류')
        self.returns+=1
        return [mu.reg_read(UC_X86_REG_EAX),struct.unpack('<I',mu.mem_read(p['blocked'],4))[0],bytes(mu.mem_read(preview,144)).hex(),bytes(mu.mem_read(t['regions'],144)).hex(),
            zlib.adler32(mu.mem_read(POOL,128*self.stride)),zlib.adler32(mu.mem_read(GRID,self.grid_size)),zlib.adler32(mu.mem_read(FIXED,0x20000))]


def generate(smoke=False):
    """독립 실행 관찰만 저장하고 소규모 검사에서는 파일을 만들지 않는다."""
    rows=[];editions={};paths={Path(__file__),FIXTURE,ROOT/'tools/ghidra/canonmayplace-functions.json'}
    # 세 PE에서 같은 입력을 두 x87 정밀도로 실행한다.
    for edition in SPECS:
        oracle=MayPlaceOracle(edition);cases=inputs()[:4] if smoke else inputs()
        for case in cases:
            observed=[oracle.run(case,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError(f'전체 배치 x87 차이: {edition}/{case}')
            rows.append([edition,*[case[k] for k in KEYS],*observed[0]])
        editions[edition]=dict(cases=len(cases),returns=oracle.returns,native_calls=dict(oracle.native_calls),substitutions=oracle.substitutions,assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 전체 MayPlace {len(cases)}개 통과',flush=True)
    if smoke:return
    # 상속/주소/정밀도 근거 도구도 SHA에 포함한다.
    for name in ('owner','canonplacement','priestgeometry','priestshape','priestcollision','canonpermission','canonterrain','canonsurrounding','canonrelations','playeranchor','damageablepredestroy'):
        paths.add(ROOT/f'tools/decomp_{name}_oracle.py')
    FIXTURE.write_text('# 전체 MayPlace 정상 반환. 임시 160바이트 확보/반납만 대체.\n# edition '+' '.join(KEYS)+' allowed blocked preview regions rawAdler hashAdler fixedAdler\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',controls=list(CONTROLS),os_calls=0,editions=editions,
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},limits=['전체 MayPlace 진입부터 ret 24 정상 반환; 픽셀/decoder/finder/지형/Player/주변/최종 helper 실제 몸체',
        '임시 160바이트 확보/반납만 대체; 메모리 부족/예외 전파 미검증','타입/프레임/지도/raw/목록은 합성 입력; 실제 자산/미션/게임/GUI/OS 미검증']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """저장 SHA/입력/정상 반환/실제 진입/임시 메모리 균형을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('SHA 불일치: '+name)
    if len(rows)!=3*len(inputs()) or report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('전체 배치 근거 오류')
    # 원본 호출과 정상 반환·명시한 메모리 경계만 검증한다.
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition];calls=item['native_calls'];n=len(selected)
        if n!=len(inputs()) or item['returns']!=2*n or item['assertions'] or calls.get(f'{PLACEMENT[edition]["entry"]:08x}')!=2*n:raise RuntimeError('전체 배치 진입/반환 오류')
        if item['substitutions']['allocate']!=item['substitutions']['free'] or not item['substitutions']['allocate']:raise RuntimeError('전체 배치 임시 메모리 균형 오류')
        for row,case in zip(selected,inputs(),strict=True):
            if len(row)!=26 or [int(v) for v in row[1:19]]!=[case[k] for k in KEYS]:raise RuntimeError('전체 배치 입력/열 오류')
    print(f'canonmayplace 검증 통과: {len(rows)}개')


if __name__=='__main__':
    # 실제 게임 실행 없이 생성/소규모 실행/저장 감사만 수행한다.
    parser=argparse.ArgumentParser();parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)
