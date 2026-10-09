#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 모양 기반 표면 finder 전체와 MayPlace 주변 권한 구간을 실행한다.

Player locator만 명시 대체한다. 게임/OS/전체 MayPlace/최종 관계는 실행하지 않는다.
"""
import argparse
import csv
import itertools
import json
import struct
import zlib
from pathlib import Path
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,TYPES,POOL,STACK,STOP,digest
from decomp_damageablepredestroy_oracle import CONTROLS
from decomp_canonpermission_oracle import SPEC as PERMISSION
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 실제 생성/순회/주변 구간 주소와 그 구간이 사용하는 스택 지역 자료다.
SPEC={
 'originals':dict(ctor=0x4b2590,next=0x4b1e70,begin=0x49bd1d,end=0x49bdc7,
    decoder=0xd4,finder=0x17c,permission=0x10,owner=0x290,map=0x5c84bc,spots=0x5c7c44,board=0x531928,foot=0x1d4),
 'originalCD':dict(ctor=0x4ebce0,next=0x4ec030,begin=0x4459d4,end=0x445a82,
    decoder=0xd8,finder=0x12c,permission=0x20,owner=0x10,map=0x52d590,spots=0x52fe48,board=0x52e9a8,foot=0x1b4),
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 분석 전용 지도/spot/프레임과 두 객체의 저장소다. 호스트/OS 주소가 아니다.
HEAP=0x13000000
MAP,SPOTS,CODES,FINDER,DECODER=HEAP,HEAP+0x20000,HEAP+0x30000,HEAP+0x31000,HEAP+0x32000
FIXTURE=ROOT/'cpppj/tests/fixtures/canonsurrounding-x86.tsv'
REPORT=ROOT/'cpppj/recovery-canonsurrounding-evidence.json'
KEYS=('kind','width','height','x','y','sourceFrame','sourceFlags','otherFlags','surface','ready','editor','allies','anyOwner','owner','relation','selected','initial','mutation','nodes','cells','spots')


def bits(value):
    """입력 좌표를 단정도 비트값으로 보존한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def packed(rows):
    """순서가 있는 입력/관찰 목록을 한 필드로 기록한다."""
    return ';'.join(','.join(map(str,row)) for row in rows) or '-'


def inputs():
    """모서리/중복/프레임/소수 spot와 방향 관계·권한 누적의 합성 입력만 만든다."""
    cases=[]
    # 실제 기대값은 아래 조합에서 실행한 기계어가 결정한다.
    for index,values in enumerate(itertools.product(((1,1),(3,2),(12,12)),(0,1,2),(0,1,2,3,4,5),(0,1))):
        (w,h),frame,mode,fraction=values;x=20.25 if fraction else 20.0;y=21.5 if fraction else 21.0
        left=int(x-w+1);top=int(y-h+1)
        nodes=[(50,1,bits(x),bits(top-1),0,0,0),(51,2,bits(left-1),bits(y),1,0,0),
            (52,0,bits(x+1),bits(y),2,2 if mode==1 else 0,1),
            (53,1,bits(x),bits(y+1),0,0,8 if mode==2 else 0),
            (54,1,bits(x),bits(y),0,0,0),(55,1,bits(x+1),bits(y+1),0,0,0)]
        cells=[(int(x),top-1,50),(left-1,int(y),51),(int(x)+1,int(y),52),
            (int(x),int(y)+1,53),(int(x),int(y),54),(int(x)+1,int(y)+1,55)]
        if w>1:cells.append((int(x)-1,top-1,50))
        spots=[]
        if mode==3:
            # 전체 내부 발자국은 원본 Begin의 조기 반환을 통과한다.
            for cy in range(max(1,top),int(y)+1):
                # 각 행의 spot 8 입력을 채운다.
                for cx in range(max(1,left),int(x)+1):spots.append((cx,cy,8))
        elif mode==4:spots=[(int(x),top-1,8),(int(x)+1,top-1,8)]
        cases.append(dict(kind='W',width=w,height=h,x=bits(x),y=bits(y),sourceFrame=frame,sourceFlags=4,
            otherFlags=2 if mode!=5 else 4,surface=0 if mode==5 else 0x800,ready=1,editor=0,allies=0,
            anyOwner=0,owner=1,relation=0,selected=50,initial=0,mutation=index%5,nodes=packed(nodes),cells=packed(cells),spots=packed(spots)))
    # 지도 끝과 모서리 clamp가 있는 현재 모양도 실제 생성/순회로 관찰한다.
    for w,h,x,y in itertools.product((1,3),(1,2),(1.0,1.25,254.0,255.0),(1.0,255.0)):
        case=dict(cases[0]);case.update(width=w,height=h,x=bits(x),y=bits(y),sourceFrame=1,mutation=0)
        left=max(1,int(x-w+1));top=max(1,int(y-h+1));nodes=[];cells=[]
        # 네 변의 정상 지도 좌표만 자료로 넣고 확장된 배열 밖 칸은 원본이 건너뛰게 한다.
        for sid,(cx,cy) in enumerate(((int(x),top-1),(left-1,int(y)),(int(x)+1,int(y)),(int(x),int(y)+1)),50):
            if cx<0 or cy<0 or cx>255 or cy>255:continue
            nodes.append((sid,1,bits(cx),bits(cy),0,0,0));cells.append((cx,cy,sid))
        case.update(nodes=packed(nodes),cells=packed(cells),spots='-');cases.append(case)
    walk_cases=list(cases)
    # 같은 지도 입력으로 바깥 관계/실제 후보 helper/초기 true 생략을 독립 조합한다.
    for index,values in enumerate(itertools.product((0,1),(0,1),(0,1),(0,1),(0,1,2),(0,1),(0,50),(0,1))):
        ready,editor,allies,any_owner,owner,relation,selected,initial=values
        case=dict(walk_cases[index%len(walk_cases)]);case.update(kind='P',sourceFrame=1,sourceFlags=4,otherFlags=2,surface=0x800,
            ready=ready,editor=editor,allies=allies,anyOwner=any_owner,owner=owner,relation=relation,selected=selected,initial=initial,mutation=index%5)
        cases.append(case)
    return cases


class SurroundingOracle(OwnerOracle):
    """원본 finder와 주변 구간에서 쓰기/호출/ABI를 제한하고 관찰한다."""
    def __init__(self,edition):
        """전체 원본 함수 범위와 별도 지도 자료를 준비한다."""
        super().__init__(edition);self.s=SPEC[edition];self.p=PERMISSION[edition]
        self.mu.mem_map(HEAP,0x40000);self.mu.mem_map(0,0x1000)
        self.exports=[ROOT/f'extracted/canonsurrounding/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set();self.substitutions=0;self.fragments=0;self.calls=0
        with self.exports[1].open(encoding='utf-8') as fp:
            # 새 내보내기의 실제 명령 범위만 실행한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 불연속 몸체 범위도 각각 허용한다.
                for part in row['ranges'].split(';'):
                    lo,hi=(int(x,16) for x in part.split('-'));self.allowed.append((lo,hi+1))
        self.mu.mem_write(self.s['map'],struct.pack('<I',MAP));self.mu.mem_write(self.s['spots'],struct.pack('<I',SPOTS))
        self.mu.mem_write(self.s['board'],struct.pack('<I',256))

    def on_instruction(self,mu,address,size,data):
        """Player 진입 인자만 관찰 후 지정 결과와 아직 읽지 않은 후보/지도 변화를 대체한다."""
        if address==self.p['anchor']:
            sp=mu.reg_read(UC_X86_REG_ESP);self.observed.append(struct.unpack('<4I',mu.mem_read(sp+4,16)))
            if len(self.observed)==1:self.mutate()
            self.substitutions+=1;mu.reg_write(UC_X86_REG_EAX,self.case['selected'])
            mu.reg_write(UC_X86_REG_EIP,struct.unpack('<I',mu.mem_read(sp,4))[0]);mu.reg_write(UC_X86_REG_ESP,sp+4);return
        super().on_instruction(mu,address,size,data)

    def mutate(self):
        """반환/조회 뒤 미래 raw 또는 지도 칸을 바꾸며 현재 결과 자체는 재계산하지 않는다."""
        mode=self.case['mutation']
        if mode==1:self.mu.mem_write(self.slot(53)+11,b'\x02')
        elif mode==2:self.mu.mem_write(MAP+(22*256+20)*2,b'\x00\x00')
        elif mode==3:self.mu.mem_write(self.slot(53)+self.o['owner'],b'\x02')
        elif mode==4:self.mu.mem_write(CODES+4,b'PP\x02\x00')

    def call(self,address,args,this,control):
        """전체 finder 호출의 정상 ret N/보존 레지스터와 x87 스택을 검사한다."""
        mu=self.mu;preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        # 보존 레지스터에 식별할 값들을 넣는다.
        for reg,value in preserved.items():mu.reg_write(reg,value)
        mu.reg_write(UC_X86_REG_ECX,this);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
        mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);self.instructions=0
        mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.emu_start(address,STOP,count=50000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+4*len(args):raise RuntimeError('표면 finder ABI 오류')
        if any(mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('표면 finder 보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('표면 finder x87 오류')
        self.calls+=1

    def snapshot(self):
        """실제 finder의 결과/정수 커서/반환 캐시를 그대로 기록한다."""
        mu=self.mu;n=struct.unpack('<I',mu.mem_read(FINDER+0x64,4))[0]
        return [struct.unpack('<I',mu.mem_read(FINDER+0x34,4))[0],*struct.unpack('<6i',mu.mem_read(FINDER+0x68,24)),n,
            *struct.unpack('<'+'H'*n,mu.mem_read(FINDER+0x80,2*n))]

    def run(self,case,control):
        """원본 생성/순회 또는 주변 구간을 실행하고 raw/지도 불변성과 실제 조회 인자를 관찰한다."""
        self.case=case;self.observed=[];self.instructions=0;s=self.s;p=self.p;mu=self.mu
        mu.mem_write(POOL,bytes(128*self.stride));mu.mem_write(MAP,bytes(0x20000));mu.mem_write(SPOTS,bytes(0x10000));mu.mem_write(FINDER,bytes(0x100))
        codes=b'PP\x01\x00AB\x02\x00JP\x03\x00';mu.mem_write(CODES,codes)
        # 타입과 실제 프레임 코드 표 입력만 구성한다.
        for typ,w,h,flags1,flags2 in ((82,case['width'],case['height'],0x800,case['sourceFlags']),(83,1,1,case['surface'],case['otherFlags'])):
            raw=bytearray(self.type_stride);struct.pack_into('<II',raw,0xe8,flags1,flags2);struct.pack_into('<II',raw,s['foot'],w,h)
            struct.pack_into('<I',raw,0x124,CODES);mu.mem_write(TYPES+typ*self.type_stride,bytes(raw))
        # 원본 슬롯은 frame/owner/extra의 판본 오프셋을 사용한다.
        for entry in case['nodes'].split(';'):
            sid,owner,x,y,frame,state,extra=map(int,entry.split(','));raw=bytearray(self.stride);raw[10]=83;raw[11]=state;raw[self.o['owner']]=owner;raw[self.o['extra']]=extra
            struct.pack_into('<II',raw,14,x,y)
            if self.edition=='originals':struct.pack_into('<I',raw,36,frame)
            else:raw[34]=frame
            mu.mem_write(self.slot(sid),bytes(raw))
        # 지도에는 번호만 입력하고 원본이 다음 반환을 결정하게 한다.
        for entry in case['cells'].split(';'):
            x,y,sid=map(int,entry.split(','));mu.mem_write(MAP+2*(y*256+x),struct.pack('<H',sid))
        if case['spots']!='-':
            # spot 조건을 물리 BYTE 지도에 넣는다.
            for entry in case['spots'].split(';'):
                x,y,value=map(int,entry.split(','));mu.mem_write(SPOTS+y*256+x,bytes([value]))
        decoder=bytearray(0x54);struct.pack_into('<I',decoder,0,82);struct.pack_into('<II',decoder,16,case['sourceFrame'],1);struct.pack_into('<II',decoder,32,case['x'],case['y'])
        mu.mem_write(DECODER,bytes(decoder));table=[case['relation']]*81;mu.mem_write(p['relations'],struct.pack('<81I',*table))
        # 후보 helper가 읽는 현재 전역이다.
        for key in ('ready','editor','allies','anyOwner'):mu.mem_write(p[key],struct.pack('<I',case[key]))
        self.write_ranges=[(0,4),(FINDER,FINDER+0x100)]
        snapshots=[];result=0
        if case['kind']=='W':
            self.call(s['ctor'],(DECODER,8),FINDER,control);snapshots.append(self.snapshot())
            # 반환 뒤의 변경은 아직 읽지 않은 후보/지도에만 영향을 준다.
            for index in range(64):
                if not snapshots[-1][0]:break
                if index==0:self.mutate()
                self.call(s['next'],(),FINDER,control);snapshots.append(self.snapshot())
            else:raise RuntimeError('표면 finder 반환 횟수 초과')
        else:
            mu.mem_write(STACK,bytes(0x400));mu.mem_write(STACK+s['decoder'],bytes(decoder));mu.mem_write(STACK+s['permission'],struct.pack('<I',case['initial']))
            mu.mem_write(STACK+s['owner'],struct.pack('<I',case['owner']));mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EBX,0)
            mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
            mu.emu_start(s['begin'],s['end'],count=100000)
            if mu.reg_read(UC_X86_REG_EIP)!=s['end'] or mu.reg_read(UC_X86_REG_ESP)!=STACK:raise RuntimeError('주변 구간 종료/스택 오류')
            if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('주변 구간 x87 오류')
            result=struct.unpack('<I',mu.mem_read(STACK+s['permission'],4))[0];self.fragments+=1
        return [result,packed(snapshots),packed(self.observed),zlib.adler32(mu.mem_read(POOL,128*self.stride)),zlib.adler32(mu.mem_read(MAP,0x20000)),zlib.adler32(mu.mem_read(SPOTS,0x10000))]


def generate(smoke=False):
    """두 x87 정밀도의 원본 관찰이 같을 때만 독립 fixture/근거를 기록한다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_canonpermission_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/canonsurrounding-functions.json',FIXTURE}
    # 세 PE는 독립 실행기/코드 허용 범위로 대조한다.
    for edition in SPECS:
        oracle=SurroundingOracle(edition);cases=inputs()
        if smoke:cases=cases[::max(1,len(cases)//40)]
        # 입력 하나를 두 제어 워드로 실제 실행한다.
        for case in cases:
            values=[oracle.run(case,control) for control in CONTROLS]
            if values[0]!=values[1]:raise RuntimeError(f'x87 차이: {edition} {case}')
            rows.append([edition,*[case[key] for key in KEYS],*values[0]])
        editions[edition]=dict(cases=len(cases),calls=oracle.calls,fragments=oracle.fragments,substitutions=oracle.substitutions,native_calls=dict(oracle.native_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 주변 표면 {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 모양 표면 finder 전체/MayPlace 주변 구간 실제 x86. Player locator만 대체.\n# edition '+' '.join(KEYS)+' result snapshots anchors rawAdler mapAdler spotAdler\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',controls=list(CONTROLS),os_calls=0,editions=editions,
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},limits=['flag 8 모양 finder 생성/순회/가상 필터/기하/접합/CRT의 실제 몸체와 정상 반환 ABI',
            'MayPlace 주변 구간만 실행, 실제 방향 관계/후보 helper와 원본 루프 실행',
            'Player locator 진입 인자/횟수 관찰 후 결과/미래 후보/지도 변경만 명시 대체',
            '정상 유한 지도 좌표/양수 발자국/64개 이하 signed WORD 캐시 입력, 전체 MayPlace/최종 관계/게임/OS는 검증 아님']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def verify():
    """저장된 입력/PE/내보내기의 SHA와 원본 호출/구간/대체 횟수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    # 모든 근거 자료의 바이트를 기록 당시와 비교한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    if len(rows)!=report['total'] or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('주변 근거 오류')
    # 실제 함수 진입과 두 정밀도 구간/대체 수를 관찰 행에서 독립 집계한다.
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition];s=SPEC[edition]
        if len(selected)!=item['cases'] or item['assertions']:raise RuntimeError('주변 행/assert 오류')
        if item['fragments']!=2*sum(r[1]=='P' for r in selected):raise RuntimeError('주변 구간 누락')
        if item['substitutions']!=2*sum(0 if r[24]=='-' else len(r[24].split(';')) for r in selected):raise RuntimeError('Player 대체 횟수 오류')
        if not item['native_calls'].get(f'{s["ctor"]:08x}',0) or not item['native_calls'].get(f'{s["next"]:08x}',0):raise RuntimeError('실제 finder 누락')
        if not item['substitutions'] or not item['native_calls'].get(f'{PERMISSION[edition]["entry"]:08x}',0):raise RuntimeError('실제 후보 권한/Player 경계 누락')
        if item['native_calls'].get(f'{PERMISSION[edition]["anchor"]:08x}',0):raise RuntimeError('Player 몸체가 예기치 않게 실행됨')
    print(f'canonsurrounding 검증 통과: {len(rows)}개')


if __name__=='__main__':
    # 생성/소규모 실행/저장 근거 감사를 분리한다.
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)
