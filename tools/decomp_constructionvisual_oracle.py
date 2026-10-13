#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""건설 표시 생성/버퍼/좌표/초기 입자 선택과 난수를 세 PE의 실제 전체 몸체로 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_process_oracle import (ProcessOracle,ROOT,BINARIES,SPECS,CONTROLS,POOL,ARENA,STACK,digest,fbits,
    UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP)

# 합성 객체와 각 버퍼는 에뮬레이터 안에서만 존재하며 호스트 포인터가 아니다.
OBJECT,PALETTE,SOURCE,OUTPUT,PARTICLES=ARENA,ARENA+0x200,ARENA+0x3000,ARENA+0x6000,ARENA+0xa000
PARENT=50
# 실제 생성·초기화·난수와 명시 경계의 주소다. CD/추가 판본은 코드 배치가 같다.
DATA={'originals':dict(create=(0x423900,0x423840),seed=0x423560,sync=0x423770,random=0x532710,
        palette=0x5453cc,type=0x5410b4,scales=(0x59a924,0x59a928),base=0x487fd0,
        capture=0x4a7db0,new=0x4e4391,free=0x4e4354,palette_ctor=0x487cb0,gradient=0x43ad20,palette_build=0x487d20,heap=0x4437f0),
    'originalCD':dict(create=(0x4eac30,0x4eab60),seed=0x4ea4a0,sync=0x4ea630,random=0x5308ec,
        palette=0x541848,type=0x51c9a4,scales=(0x565c3c,0x565c40),base=0x45c6c0,
        capture=0x45b100,new=0x4f1650,free=0x4f1a50,palette_ctor=0x45cfa0,gradient=0x459460,palette_build=0x45d020,heap=0x48ef00)}
DATA['original1037']=dict(DATA['originalCD'])
# 입력 열·의미 있는 객체 DWORD 위치·저장 관찰/근거 경로다.
KEYS=('instant','width','height','pattern','random','cold','mutate','z','x','y','scaleX','scaleY','selected','old')
OFFSETS=(0,8,0x20,0xc,0x10,0x14,0x18,0x48,0x54,0x58,0x5c,0x60,0x64,0x6c,0x70,0x74,0x78,0x7c,0x80,0x24,0x28)
FIXTURE=ROOT/'cpppj/tests/fixtures/constructionvisual-x86.tsv'
REPORT=ROOT/'cpppj/constructionvisual.json'


def case(**changes):
    """화면 좌표·시드·픽셀/경계 입력만 구성하고 표시 기대값은 계산하지 않는다."""
    result=dict(instant=0,width=7,height=9,pattern=0,random=0,cold=0,mutate=0,z=0x80,
        x=fbits(20.5),y=fbits(-3.25),scaleX=fbits(16.0),scaleY=fbits(11.0),selected=0,old=0)
    result.update(changes);return result


def plan():
    """두 생성 방식·팔레트 캐시·작은/큰 이미지·투명 패턴·난수·재초기화를 조합한다."""
    rows=[]
    # 큰 이미지의 불투명 수는 최소 500칸을 넘어서는 용량 경로도 실행한다.
    for instant,size,pattern,random,cold in itertools.product((0,1),((1,1),(2,3),(3,4),(4,5),(7,9),(61,60)),range(7),(0,1,0xffffffff),(0,1)):
        rows.append(('Create',case(instant=instant,width=size[0],height=size[1],pattern=pattern,random=random,cold=cold)))
    # 현재 입자 저장소 반납과 누적 selected/활성·첫 표시 필드 보존을 별도로 확인한다.
    for instant,size,pattern,old in itertools.product((0,1),((1,1),(7,9)),range(7),(0,1)):
        rows.append(('Seed',case(instant=instant,width=size[0],height=size[1],pattern=pattern,old=old,selected=0xfffffffe,random=17)))
    # signed zOrder와 현재 화면 배율은 동기화 때 다시 읽는다.
    for z,scale in itertools.product((0,127,128,255),(0.0,-2.5,3.25)):
        rows.append(('Sync',case(z=z,scaleX=fbits(scale),scaleY=fbits(scale),old=1,selected=31)))
    # 부착/캡처 경계가 공유 팔레트와 현재 raw/배율을 바꾼다.
    for instant,cold,mutate in itertools.product((0,1),(0,1),(1,2,3)):
        rows.append(('Create',case(instant=instant,cold=cold,mutate=mutate)))
    # 폭이 입자 용량을 넘을 때 확보한 칸까지만 선택하고 열 탐색/섞기 난수를 끝까지 실행한다.
    for instant,cold in itertools.product((0,1),(0,1)):
        rows.append(('Create',case(instant=instant,cold=cold,width=1001,height=3)))
    # 동일한 용량 한계를 기존 저장소/누적 선택 수가 있는 직접 초기화에서도 실행한다.
    for instant,old in itertools.product((0,1),(0,1)):
        rows.append(('Seed',case(instant=instant,old=old,width=1001,height=3,pattern=6,selected=0xfffffffe,random=17)))
    return rows


def pixels(c):
    """기대 입자 대신 입력 캡처 픽셀만 만든다. 0xff만 투명이다."""
    result=bytearray()
    # 서로 다른 하단/구멍/마지막 픽셀을 가진 일곱 패턴을 만든다.
    for i in range(c['width']*c['height']):
        pattern=c['pattern'];color=(i*13+17)%255
        if pattern==1:color=255
        if pattern==2 and i%3==0:color=255
        if pattern==3 and i//c['width']>c['height']//3:color=255
        if pattern==4 and i%c['width']!=c['width']//2:color=255
        if pattern==5 and i!=c['width']*c['height']-1:color=255
        if pattern==6:color=0
        result.append(color)
    return bytes(result)


class VisualOracle(ProcessOracle):
    """앞 PE/ABI 로더에 표시 생성/초기화 몸체와 제한된 외부 응답 경계를 더한다."""
    def __init__(self,edition):
        """실제 함수 범위와 원본 난수/팔레트/유효성 진단 전역 쓰기를 허용한다."""
        super().__init__(edition);self.d=DATA[edition];self.edition=edition;self.returns=collections.Counter();self.boundaries=collections.Counter()
        self.extra=[ROOT/f'extracted/constructionvisual/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with self.extra[1].open(encoding='utf-8') as fp:
            # 진입점과 불연속 몸체 범위를 그대로 사용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entry_names[int(row['entry'],16)]=row['entry']
                # 함수 사이의 코드까지 허용하지 않는다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.writes.extend((a,a+4) for a in (self.d['random'],self.d['palette'],0x5c8450,0x5c7c48))

    def ret(self,value=0,purge=0):
        """응답 경계에서만 원본 호출 규약으로 반환한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,value)
        self.mu.reg_write(UC_X86_REG_EIP,self.u32(esp));self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge)

    def args(self,count):
        """현재 경계의 스택 DWORD 인자만 읽는다."""
        return struct.unpack('<'+'I'*count,self.mu.mem_read(self.mu.reg_read(UC_X86_REG_ESP)+4,count*4))

    def on_instruction(self,mu,address,size,data):
        """부착/SHP 캡처/팔레트 생성/확보만 응답하며 실제 선택·좌표·난수·CRT는 대체하지 않는다."""
        d=self.d;c=getattr(self,'case',None)
        if address in (d['new'],d['free'],d['base'],d['capture'],d['palette_ctor'],d['gradient'],d['palette_build'],d['heap']):
            name=next(key for key in ('new','free','base','capture','palette_ctor','gradient','palette_build','heap') if d[key]==address)
            self.boundaries[name]+=1
            if name=='new':
                amount=self.args(1)[0];self.events.append(f'A:{amount}')
                if amount==136 and not self.object_allocated:self.object_allocated=True;result=OBJECT
                elif amount==8:result=PALETTE
                elif amount==c['width']*c['height'] and not self.output_allocated:self.output_allocated=True;result=OUTPUT
                elif amount>=6000 and amount%12==0:result=PARTICLES
                else:raise RuntimeError(f'표시 확보 크기 오류: {amount}')
                self.ret(result);return
            if name=='free':
                if self.args(1)!=(PARTICLES,):raise RuntimeError('표시 입자 반납 인자 오류')
                self.events.append('F');self.ret();return
            if name=='base':
                if self.args(2)!=(PARENT,17) or mu.reg_read(UC_X86_REG_ECX)!=OBJECT:raise RuntimeError('표시 부착 경계 인자 오류')
                mu.mem_write(OBJECT,bytes(36));self.put(OBJECT+8,PARENT);self.events.append('B:50:17')
                if c['mutate']==1:self.put(d['palette'],PALETTE)
                if c['mutate']==2:self.put(POOL+PARENT*self.s['stride']+14,fbits(7.25))
                self.ret(OBJECT,8);return
            if name=='capture':
                parent,w,h,x,y=self.args(5)
                if (parent,w,h,x,y)!=(PARENT,OBJECT+0x54,OBJECT+0x58,OBJECT+0x5c,OBJECT+0x60):raise RuntimeError('표시 캡처 인자 오류')
                # 캡처 출력 인자 네 개만 채우며 입자 알고리즘은 실행 몸체에 맡긴다.
                for pointer,value in ((w,c['width']),(h,c['height']),(x,-3),(y,2)):self.put(pointer,value)
                self.events.append('C:50')
                if c['mutate']==3:self.put(d['scales'][0],fbits(-4.5));self.put(POOL+PARENT*self.s['stride']+14,fbits(-1.25))
                self.ret(SOURCE);return
            if name=='palette_ctor':
                if mu.reg_read(UC_X86_REG_ECX)!=PALETTE:raise RuntimeError('공유 색 표 this 오류')
                self.events.append('P');self.ret(PALETTE);return
            if name=='gradient':
                values=struct.unpack('<6dId',mu.mem_read(mu.reg_read(UC_X86_REG_ESP)+4,60));kind=values[6]
                expected=(0.0,0.0,1.0,1.0,1.0,1.0,8,2.4) if kind==8 else (1.0,1.0,1.0,128/255,128/255,128/255,3,2.4)
                if values!=expected:raise RuntimeError(f'공유 색 띠 인자 오류: {self.edition} {values!r}')
                pointer=mu.reg_read(UC_X86_REG_ECX)
                if not STACK-0xf000<=pointer<STACK:raise RuntimeError('색 띠 지역 객체 범위 오류')
                self.gradient_pointers.append(pointer);self.events.append(f'G:{kind}');self.ret(pointer,60);return
            if name=='palette_build':
                if self.args(3)!=(*self.gradient_pointers,0) or mu.reg_read(UC_X86_REG_ECX)!=PALETTE:raise RuntimeError('공유 색 표 합치기 인자 오류')
                self.events.append('J');self.ret(0,12);return
            self.ret();return
        ProcessOracle.on_instruction(self,mu,address,size,data)

    def run(self,operation,c,control):
        """정상 반환·ABI/레지스터/x87/SEH와 실제 입자/픽셀·난수 최종 상태를 관찰한다."""
        self.begin(dict(server=1,boss=1,rotation=0,now=0),control);self.case=c;self.gradient_pointers=[];d=self.d
        self.object_allocated=operation!='Create';self.output_allocated=operation!='Create'
        mu=self.mu;self.put(d['random'],c['random']);self.put(d['palette'],0 if c['cold'] else PALETTE);self.put(d['type'],17)
        # 실행 전 입력 화면 배율을 원본 전역에 쓴다.
        for a,v in zip(d['scales'],(c['scaleX'],c['scaleY'])):self.put(a,v)
        raw=bytearray(self.s['stride']);struct.pack_into('<I',raw,0,self.s['base_vtable']);raw[10]=100;raw[11]=0
        raw[34 if self.patch else 32]=1;raw[35 if self.patch else 33]=c['z'];struct.pack_into('<2I',raw,14,c['x'],c['y'])
        mu.mem_write(POOL+PARENT*self.s['stride'],bytes(raw));mu.mem_write(OBJECT,b'\xa5'*136)
        mu.mem_write(SOURCE,pixels(c));mu.mem_write(OUTPUT,b'\xff'*(c['width']*c['height']))
        if operation!='Create':
            table=(0x503680 if c['instant'] else 0x5036ac) if self.patch else (0x507410 if c['instant'] else 0x507440)
            values={0:table,8:PARENT,0x20:123,0xc:fbits(1),0x10:fbits(2),0x14:fbits(3),0x18:fbits(4),0x48:PALETTE,
                0x4c:SOURCE,0x50:OUTPUT,0x54:c['width'],0x58:c['height'],0x5c:-3,0x60:2,0x64:(c['height']-1)*c['width'],
                0x6c:fbits(2.0) if c['instant'] else 0x3fa66666,0x70:fbits(9.0 if c['instant'] else 2.0),
                0x74:c['selected'],0x78:0,0x7c:1,0x80:3,0x24:9 if c['old'] else 0,0x28:0,0x2c:PARTICLES if c['old'] else 0}
            # 직접 재초기화 루트가 읽을 기존 필드만 설정한다.
            for offset,value in values.items():self.put(OBJECT+offset,value)
        entry=d['create'][c['instant']] if operation=='Create' else d['seed'] if operation=='Seed' else d['sync']
        args=[PARENT] if operation=='Create' else [37,41] if operation=='Seed' else []
        self.call(entry,args,this=0 if operation=='Create' else OBJECT,purge=8 if operation=='Seed' else 0)
        if operation=='Create' and mu.reg_read(UC_X86_REG_EAX)!=OBJECT:raise RuntimeError('생성 객체 반환 포인터 오류')
        if any(mu.reg_read(reg) for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)):raise RuntimeError('표시 보존 레지스터 오류')
        self.returns[f'{entry:08x}']+=1;fields=':'.join(str(self.u32(OBJECT+off)) for off in OFFSETS)
        capacity,total=self.u32(OBJECT+0x24),self.u32(OBJECT+0x28);particles=[]
        # 사용한 입자의 목적지 포인터만 버퍼 오프셋으로 정규화한다.
        for i in range(total):
            active,color,padding,life,pointer=struct.unpack('<BBHII',mu.mem_read(PARTICLES+i*12,12))
            if padding:raise RuntimeError('입자 패딩 변경')
            particles.append(f'{active}:{color}:{life}:{pointer-OUTPUT}')
        if any(mu.mem_read(PARTICLES+total*12,(capacity-total)*12)):raise RuntimeError('미사용 입자가 0이 아님')
        size=c['width']*c['height']
        return [';'.join(self.events),fields,bytes(mu.mem_read(SOURCE,size)).hex(),bytes(mu.mem_read(OUTPUT,size)).hex(),
            ';'.join(particles) or '-',str(self.u32(d['random'])),str(self.u32(d['palette']))]


def generate(smoke):
    """두 정밀도의 원본 출력이 일치할 때만 관찰과 SHA/실제 진입/반환/경계를 저장한다."""
    rows=[];editions={};files={Path(__file__),ROOT/'tools/decomp_process_oracle.py',ROOT/'tools/ghidra/constructionvisual-functions.json',FIXTURE}
    # 각 PE마다 독립 가상 메모리로 실행한다.
    for edition in BINARIES:
        oracle=VisualOracle(edition);cases=plan()[::23]+plan()[-8:] if smoke else plan()
        # 표시 알고리즘의 C++ 기대식을 만들지 않고 실제 관찰 두 개를 비교한다.
        for index,(operation,c) in enumerate(cases,1):
            observed=[oracle.run(operation,c,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError('건설 표시 x87 정밀도 차이')
            rows.append([edition,operation,','.join(str(c[key]) for key in KEYS),*observed[0]])
            if not smoke and index%100==0:print(f'{edition}: {index}/{len(cases)}개 대조',flush=True)
        editions[edition]=dict(cases=dict(collections.Counter(op for op,_ in cases)),entries=dict(oracle.hits),returns=dict(oracle.returns),boundaries=dict(oracle.boundaries),assertions=oracle.asserts)
        files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(f'{edition}: constructionvisual {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 건설 표시 생성/버퍼/좌표/초기 입자 선택·난수. 부착/캡처/팔레트/확보만 명시 경계.\n'+
        '\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),controls=list(CONTROLS),host='HJOW-Athlon',date='2026-10-11',os_calls=0,
        editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},limits=[
            '두 생성 진입/공통 생성자(차가운/재사용 색 표 분기)/버퍼/좌표/초기 입자·실제 정수/실수 난수·CRT/stack probe',
            'RenderProcess 등록/부모 부착·SHP 캡처·색 띠/표 생성·확보/반납만 응답 경계',
            '유한 합성 시각/이미지/배율·최소/초과 입자 용량/가득 찬 저장소·투명 패턴·난수/재초기화·경계 중 raw/전역 변경',
            'RunFrame/팔레트 갱신·화면 출력/실제 SHP 캡처/게임/OS는 후속, new 실패/null/assert 실행 없음',
            'ABI/보존 레지스터/x87/SEH·허용 실행/쓰기·SHA/입력/폭/진입/반환/외부 경계·미사용 입자 0 감사']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """기대 입자를 재계산하지 않고 저장 SHA·행/폭·정상 반환/실제 진입·사건 집계를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 생성에 사용한 모든 원본/로더/목록/내보내기/fixture가 그대로인지 확인한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError('건설 표시 SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls'] or set(report['editions'])!=set(BINARIES):raise RuntimeError('건설 표시 총계 오류')
    # 판본별 입력 순서와 실제 루트의 정상 반환을 확인한다.
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition];cases=plan();counts=collections.Counter();d=DATA[edition];boundaries=collections.Counter();float_calls=0;int_calls=0
        if len(chosen)!=len(cases) or item['assertions']:raise RuntimeError('표시 행/assert 오류')
        # 출력 값의 범위/폭과 사건 집계만 확인하며 입자 선택을 다시 구현하지 않는다.
        for row,(operation,c) in zip(chosen,cases,strict=True):
            if len(row)!=10 or row[1]!=operation or row[2]!=','.join(str(c[key]) for key in KEYS):raise RuntimeError('표시 입력 순서 오류')
            values=[int(v) for v in row[4].split(':')];size=c['width']*c['height']
            if len(values)!=len(OFFSETS) or len(bytes.fromhex(row[5]))!=size or len(bytes.fromhex(row[6]))!=size:raise RuntimeError('표시 필드/버퍼 폭 오류')
            particles=[] if row[7]=='-' else row[7].split(';')
            if len(particles)!=values[-1]:raise RuntimeError('표시 입자 수 오류')
            # 입자 필드의 폭과 목적지 경계만 검사한다.
            for particle in particles:
                active,color,life,pixel=(int(v) for v in particle.split(':'))
                if active not in (0,1) or not 0<=color<=255 or not 0<=life<=0xffffffff or not 0<=pixel<size:raise RuntimeError('입자 폭 오류')
            float_calls+=2*len(particles);int_calls+=4*c['width'] if particles else 0
            # 저장된 외부 사건에서 경계 호출 수만 집계한다.
            for event in row[3].split(';'):
                key={'A':'new','B':'base','C':'capture','P':'palette_ctor','G':'gradient','J':'palette_build','F':'free'}[event[0]];boundaries[key]+=2
            entry=d['create'][c['instant']] if operation=='Create' else d['seed'] if operation=='Seed' else d['sync'];counts[f'{entry:08x}']+=2
        if dict(counts)!=item['returns'] or item['cases']!=dict(collections.Counter(op for op,_ in cases)):raise RuntimeError('표시 루트 반환 집계 오류')
        # 루트별 정상 반환과 실제 함수 진입의 대응을 확인한다.
        for entry,count in counts.items():
            if item['entries'].get(entry,0)<count:raise RuntimeError('표시 실제 진입 부족')
        float_entry='00455930' if edition=='originals' else '0048cc50';int_entry='004558c0' if edition=='originals' else '0048cbd0'
        if item['entries'].get(float_entry,0)!=float_calls or item['entries'].get(int_entry,0)!=int_calls:raise RuntimeError('표시 실제 난수 진입 집계 오류')
        boundaries['heap']=boundaries['palette_ctor']
        # 공개 사건과 힙 검사 경계 외에 숨은 호출이 없는지 확인한다.
        if dict(boundaries)!=item['boundaries']:raise RuntimeError('표시 외부 경계 집계 오류')
        # 각 외부 경계의 두 정밀도 합계를 확인한다.
        for boundary,count in boundaries.items():
            if item['boundaries'].get(boundary)!=count:raise RuntimeError('표시 외부 경계 집계 오류')
    print(f'constructionvisual 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)


if __name__=='__main__':main()
