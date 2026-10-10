#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 전체 돌 배경/질감 반복 및 명암 표 생성 몸체를 제한 x86으로 관찰한다. 게임 실행 없음."""
import argparse
import collections
import csv
import json
import random
import struct
from pathlib import Path
from decomp_owner_oracle import ROOT, SPECS, OwnerOracle, TYPES, STACK, STOP, digest
from decomp_gumpvisual_oracle import CONTROLS
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EBP,
    UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 명암 표/생성자/전체 배경·반복 함수 및 자산 조회/장치 경계의 원본 주소다.
SPEC={
 'originals':dict(sort=0x439de0,maps=0x43c010,background=0x465880,tile=0x465690,
    table=0x555d88,bright=0x556238,dark=0x556438,clip=0x5c7a44,
    setclip=0x4a1580,setmap=0x4dabe0,raw=0x4daa70,shaded=0x4daab0,
    typeget=0x49a840,frameget=0x419850,
    outputs=[0x555e38,0x555f38,0x556038,0x556138,0x556238,0x556338,0x556438,0x557738,0x557838]),
 'originalCD':dict(sort=0x458a90,maps=0x496530,background=0x494a20,tile=0x494720,
    table=0x565cb0,bright=0x56c7e8,dark=0x56e0e8,clip=0x552880,
    setclip=0x422e10,setmap=0x4d6630,raw=0x4d64e0,shaded=0x4d6510,
    outputs=[0x56dde8,0x56dee8,0x56dfe8,0x56c6e8,0x56c7e8,0x56c8e8,0x56e0e8,0x56d2e8,0x56dce8])
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 에뮬레이터의 RGB 입력, ColorSort, SHP·메타데이터 저장소다. 호스트 주소와 관계없다.
LOGICAL,SORT,SHAPE,METADATA=TYPES+0x10000,TYPES+0x11000,TYPES+0x18000,TYPES+0x18100
# 재현 가능한 입력 시드와 새 근거/fixture 경로다.
SEED=0x465880
FIXTURE=ROOT/'cpppj/tests/fixtures/gumpbackground-x86.tsv'
REPORT=ROOT/'cpppj/recovery-gumpbackground-evidence.json'


def palette_inputs():
    """실제 팔레트와 중복·제외 범위·RGB 혼합 입력만 만든다. 출력 번호는 원본이 정한다."""
    data=(ROOT/'originals/d/GIFCLOUD.COL').read_bytes()
    actual=[int.from_bytes(data[8+i*3:11+i*3]+b'\0','little') for i in range(256)]
    actual[0]=0;actual[255]=0xffffff
    rng=random.Random(SEED)
    mixed=[rng.getrandbits(32) for _ in range(256)]
    grey=[i*0x010101 for i in range(256)]
    grey[17]=grey[18]=grey[229]=0x646464;grey[245]=0xffffff
    return (actual,mixed,grey)


def draw_inputs():
    """눌림·명암 우선순위·테두리 선택·음수/작은 영역과 클립 입력을 만든다."""
    result=[];rng=random.Random(SEED)
    # 네 테두리 조합과 눌림을 독립 관찰한다.
    for edges in range(16):
        flags=0x2000000|(edges<<21)
        result.append((10,20,85,37,0,0,100,100,flags,33,33))
        result.append((-7,-11,40,19,-5,-3,35,17,flags|0x40000,9,6))
    # 들어오는 두 명암 비트가 함께 켜질 때 밝은 표가 우선인지도 관찰한다.
    for flags in (0x2000000,0x3e80000,0x3f00000,0x3f80000):
        result.append((1,1,74,18,4,3,65,16,flags,33,33))
    # 잘못된 타일 크기는 원본의 조기 반환 경로다. 빈 영역도 호출/클립 복구를 유지한다.
    for width,height in ((1,33),(33,1),(0,0),(-1,33),(33,33)):
        result.append((2,2,2,2,0,0,100,100,0x3e00000,width,height))
    # 화면 밖 및 다양한 원점/주기를 섞되 원본 루프가 끝나는 작은 좌표를 사용한다.
    for _ in range(15):
        x,y=rng.randint(-40,100),rng.randint(-40,100)
        result.append((x,y,x+rng.randint(1,80),y+rng.randint(1,35),0,0,100,100,
            0x2000000|(rng.randrange(16)<<21)|rng.choice((0,0x40000,0x80000,0x100000)),
            rng.randint(3,35),rng.randint(3,35)))
    return result


class BackgroundOracle(OwnerOracle):
    """전체 원본 몸체를 실행하고 자산 조회/clip/map/shape 장치 경계만 기록한다."""
    def __init__(self,edition):
        """읽기 전용 PE·전체 내보낸 범위·허용 쓰기와 합성 자산 배치를 준비한다."""
        super().__init__(edition);self.b=SPEC[edition]
        self.exports=[ROOT/f'extracted/gumpbackground/{edition}/{n}' for n in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # Ghidra가 확정한 전체 몸체의 불연속 범위만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    a,z=(int(v,16) for v in part.split('-'));self.allowed.append((a,z+1))
        # 수백만 명령의 범위 감사는 바이트 집합으로 하되 모든 명령의 양 끝을 계속 검사한다.
        self.allowed_bytes={a for low,high in self.allowed for a in range(low,high)}
        self.stubs={self.b[n]:n for n in ('setclip','setmap','raw','shaded')}
        if edition=='originals':self.stubs.update({self.b[n]:n for n in ('typeget','frameget')})
        self.write_ranges=[(SORT,SORT+256*56),(self.b['clip'],self.b['clip']+16)]+[(a,a+256) for a in self.b['outputs']]
        self.boundaries=collections.Counter();self.returns=0;self.events=[]
        self.mu.mem_write(self.b['table'],struct.pack('<I',SORT))
        self.mu.mem_write(TYPES+82*self.type_stride+0xdc,struct.pack('<I',SHAPE))
        self.mu.mem_write(SHAPE+8,struct.pack('<I',METADATA+36-SHAPE))
        if edition!='originals':self.mu.mem_write(0x546778,bytes(4))

    def read(self,address,format):
        """원본 인자·전역을 정확한 바이트 배치로 읽는다."""
        return struct.unpack(format,self.mu.mem_read(address,struct.calcsize(format)))

    def ret(self,value=0,purge=0):
        """cdecl 자산/장치 경계의 반환만 수행한다. 내부 분기는 원본 명령이 실행한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);target,=self.read(sp,'<I')
        self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,target)

    def on_instruction(self,mu,address,size,data):
        """전체 tile 진입과 장치 출력 순서를 관찰하고 허용 몸체 밖 실행을 거부한다."""
        name=self.stubs.get(address)
        if address==self.b['tile']:
            sp=mu.reg_read(UC_X86_REG_ESP)
            self.events.append('p,'+','.join(map(str,self.read(sp+4,'<4iI'))))
        if not name:
            self.instructions+=1
            if address not in self.allowed_bytes or address+size-1 not in self.allowed_bytes:raise RuntimeError(f'허용하지 않은 배경 실행: {address:08x}')
            if address in self.entries:self.native_calls[f'{address:08x}']+=1
            return
        sp=mu.reg_read(UC_X86_REG_ESP)
        self.boundaries[name]+=1
        if name=='typeget':
            if self.read(sp+4,'<I')!=(82,):raise RuntimeError('타입 조회 입력 오류')
            self.ret(TYPES+82*self.type_stride)
        elif name=='frameget':
            if mu.reg_read(UC_X86_REG_ECX)!=TYPES+82*self.type_stride or self.read(sp+4,'<2I')!=(0,0):raise RuntimeError('프레임 조회 입력 오류')
            self.ret(METADATA,8)
        elif name=='setclip':
            rect=self.read(sp+4,'<4i');self.events.append('c,'+','.join(map(str,rect)))
            mu.mem_write(self.b['clip'],struct.pack('<4i',*rect));self.ret()
        elif name=='setmap':
            pointer,=self.read(sp+4,'<I')
            if pointer not in (self.b['bright'],self.b['dark']):raise RuntimeError('명암 표 선택 오류')
            self.shade=1 if pointer==self.b['bright'] else 2;self.events.append(f'm,{self.shade}');self.ret()
        else:
            shape,frame,x,y=self.read(sp+4,'<IIii')
            if (shape,frame)!=(SHAPE,0):raise RuntimeError('질감 그림 인자 오류')
            self.events.append(f'd,{x},{y},{self.shade if name=="shaded" else 0}');self.ret()

    def call(self,entry,args,control,purge=0,this=0):
        """정상 반환·스택·보존 레지스터·x87 제어/스택 균형을 검사한다."""
        mu=self.mu;mu.mem_write(STACK-0x800,bytes(0x800));mu.mem_write(STACK,struct.pack('<I',STOP)+args)
        saved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_EBP:0x23456789,UC_X86_REG_ESI:0x34567890,UC_X86_REG_EDI:0x45678901}
        # 호출마다 새 ABI 입력을 공급해 장치 경계의 누락도 검출한다.
        for register,value in saved.items():mu.reg_write(register,value)
        mu.reg_write(UC_X86_REG_ECX,this);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
        mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);self.instructions=0
        mu.emu_start(entry,STOP,count=50000000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+purge:raise RuntimeError('배경 반환/스택 오류')
        if any(mu.reg_read(r)!=v for r,v in saved.items()):raise RuntimeError('배경 보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('배경 x87 균형 오류')
        self.returns+=1

    def maps(self,logical,control):
        """실제 ColorSort 생성자가 만든 RGB/HLS를 전체 명암 생성 몸체에 공급한다."""
        raw=struct.pack('<256I',*logical);self.mu.mem_write(LOGICAL,raw);self.mu.mem_write(SORT,bytes(256*56))
        self.call(self.b['sort'],struct.pack('<I',LOGICAL),control,4,SORT)
        if self.mu.reg_read(UC_X86_REG_EAX)!=SORT:raise RuntimeError('ColorSort 생성 결과 오류')
        source=bytes(self.mu.mem_read(SORT,256*56));self.call(self.b['maps'],b'',control)
        if bytes(self.mu.mem_read(LOGICAL,len(raw)))!=raw or bytes(self.mu.mem_read(SORT,len(source)))!=source:raise RuntimeError('명암 생성 입력 변경')
        return bytes(self.mu.mem_read(self.b['bright'],256)).hex(),bytes(self.mu.mem_read(self.b['dark'],256)).hex()

    def draw(self,case,control):
        """전체 배경/반복/클립 몸체를 실행해 pass·clip·map·draw 경계 순서를 반환한다."""
        self.events=[];self.shade=0;rect,clip,flags,width,height=case[:4],case[4:8],case[8],case[9],case[10]
        self.mu.mem_write(self.b['clip'],struct.pack('<4i',*clip));self.mu.mem_write(METADATA+24,struct.pack('<hh',width,height))
        self.call(self.b['background'],struct.pack('<4i3I',*rect,flags,82,0),control)
        if self.read(self.b['clip'],'<4i')!=tuple(clip):raise RuntimeError('질감 클립 복구 누락')
        return '|'.join(self.events) or '-'


def generate(smoke=False):
    """두 x87 정밀도의 독립 관찰이 일치할 때만 새 fixture와 SHA 근거를 저장한다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_gumpvisual_oracle.py',
        ROOT/'tools/ghidra/gumpbackground-functions.json',ROOT/'originals/d/GIFCLOUD.COL',FIXTURE}
    palettes=palette_inputs()[:1] if smoke else palette_inputs();draws=draw_inputs()[:2] if smoke else draw_inputs()
    # 세 PE를 순서대로 읽기 전용 실행한다.
    for edition in SPECS:
        oracle=BackgroundOracle(edition)
        # 전체 표 생성을 각 정밀도에서 실행하고 결과를 비교한다.
        for number,logical in enumerate(palettes):
            first=oracle.maps(logical,CONTROLS[0]);second=oracle.maps(logical,CONTROLS[1])
            if first!=second:raise RuntimeError(f'명암 x87 차이: {edition} {number}')
            rows.append([edition,'maps',number,struct.pack('<256I',*logical).hex(),*first]);print(f'{edition}: 명암 표 {number} 통과',flush=True)
        # 배경 출력 명령도 같은 입력에서 두 번 관찰한다.
        for number,case in enumerate(draws):
            first=oracle.draw(case,CONTROLS[0]);second=oracle.draw(case,CONTROLS[1])
            if first!=second:raise RuntimeError(f'배경 x87 차이: {edition} {number}')
            rows.append([edition,'draw',number,*case,first])
        editions[edition]=dict(cases=len(palettes)+len(draws),returns=oracle.returns,native_calls=dict(oracle.native_calls),
            boundaries=dict(oracle.boundaries),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 배경 {len(draws)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 전체 원본 ColorSort/명암 생성 및 배경/반복/clip 몸체의 독립 x86 관찰.\n'+
        '\n'.join('\t'.join(map(str,r)) for r in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-10',controls=list(CONTROLS),os_calls=0,
        editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},
        limits=['질감 경로만 대조; 체크/단색 fill·flat line·debug 출력 제외',
            'Patch 타입/프레임 조회 및 clip/map/shape 출력은 명시 경계; 전체 배경/타일/클립 분기는 실제 원본 실행',
            '명암 생성은 전체 몸체 실행, 복원/대조 출력은 밝음/어두움 두 표; 나머지 일곱 표는 후속',
            '파일 캐시 !color.dat·ColorSort 재정렬·paletteDirty 소비·전체 자식 Gump 수명은 후속']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA·입력 순서·전체 진입/반환·경계 계수와 fixture 배치를 다시 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    # 원본/몸체/도구/fixture는 생성 시점 바이트와 같아야 한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError(f'SHA 불일치: {name}')
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls'] or set(report['editions'])!=set(SPECS):raise RuntimeError('배경 근거 계수 오류')
    # 판본별 입력과 native 전체 진입·정상 반환을 확인한다.
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition];palettes=palette_inputs();draws=draw_inputs();b=SPEC[edition]
        if len(selected)!=len(palettes)+len(draws) or item['cases']!=len(selected) or item['assertions']:raise RuntimeError('배경 표본 계수 오류')
        if item['returns']!=4*len(palettes)+2*len(draws):raise RuntimeError('배경 정상 반환 계수 오류')
        for name,count in (('sort',len(palettes)*2),('maps',len(palettes)*2),('background',len(draws)*2)):
            if item['native_calls'].get(f'{b[name]:08x}')!=count:raise RuntimeError('배경 전체 진입 오류')
        # fixture 입력 바이트/순서와 출력 이벤트에서 관찰 계수를 재구성한다.
        observed=collections.Counter();passes=0
        for number,logical in enumerate(palettes):
            row=selected[number]
            if row[:4]!=[edition,'maps',str(number),struct.pack('<256I',*logical).hex()] or len(row)!=6 or any(len(bytes.fromhex(v))!=256 for v in row[4:]):raise RuntimeError('명암 fixture 입력 오류')
        for number,case in enumerate(draws):
            row=selected[len(palettes)+number]
            if row[:14]!=[edition,'draw',str(number),*map(str,case)] or len(row)!=15:raise RuntimeError('배경 fixture 입력 오류')
            for event in ([] if row[-1]=='-' else row[-1].split('|')):
                kind,*args=event.split(',')
                if kind=='p':passes+=2
                if kind=='c':observed['setclip']+=2
                if kind=='m':observed['setmap']+=2
                if kind=='d':observed['raw' if args[-1]=='0' else 'shaded']+=2
        if edition=='originals':observed['typeget']=observed['frameget']=passes
        if dict(observed)!=item['boundaries'] or item['native_calls'].get(f'{b["tile"]:08x}')!=passes:raise RuntimeError('배경 경계/타일 계수 오류')
    print(f"돌 배경 감사 통과: {report['total']}개")


def main():
    """소량 직접 실행·전체 생성·저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
