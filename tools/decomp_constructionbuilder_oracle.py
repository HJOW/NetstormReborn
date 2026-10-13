#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""건설 사제/운반 객체 생성·이동 전체와 기하/공급원 소비를 실제 PE의 제한 x86으로 대조한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,TYPES,POOL,STACK,STOP,digest
from decomp_returnpiece_oracle import ReturnOracle,VTABLE,ALLOCATION
from decomp_damageablepredestroy_oracle import CONTROLS,bits
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX

# 가상 경로/공급원/반환 주소와 경계 함수다. 실제 호스트 메모리/OS와는 무관하다.
PATH,SOURCES,OUT=0x14700000,0x14800000,0x14900000
OWNER,POP,SPEED=STOP+0x400,STOP+0x500,STOP+0x600
# 실제 루트/기하/자원 효과와 전역이다.
SPEC={
 'originals':dict(entry=0x443000,next=0x473570,anchor=0x48fdb0,create=0x4af530,approach=0x41db70,move=0x489080,
    choose=0x4734d0,sort=0x474970,allocate=0x4e4391,timer=0x496e80,notify=0x4214a0,arrival=0x9c50,
    globals=dict(builder=0x541270,carrier=0x5411fc,bridge=0x5411a0,mask=0x5424b8,event=0x532750,now=0x55b4d0,list=0x592f4c,count=0x592f54)),
 'originalCD':dict(entry=0x4d1120,next=0x48aaf0,anchor=0x406f80,create=0x4ab390,approach=0x440400,move=0x47d150,
    choose=0x48aa40,sort=0x48ab40,allocate=0x4f1650,timer=0x48efd0,notify=0x448c10,arrival=0x12d0,
    globals=dict(builder=0x51cb5c,carrier=0x51cae8,bridge=0x51ca8c,mask=0x5395cc,event=0x52da30,now=0x5484a8,list=0x567658,count=0x567660)),
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 고정 입력 열 순서다. 시각은 double 비트를 두 DWORD로 보존한다.
KEYS=('player','anchor','owner','mask','genus','x','y','footX','footY','timeLow','timeHigh','nowLow','nowHigh',
    'path','speed','sources','allocation','frame','mutate','bridge','count')
FIXTURE=ROOT/'cpppj/tests/fixtures/constructionbuilder-x86.tsv'
REPORT=ROOT/'cpppj/constructionbuilder.json'


def double_words(value):
    """입력 double만 DWORD 둘로 분리한다. 기대 반환 시각은 계산하지 않는다."""
    return struct.unpack('<2I',struct.pack('<d',value))


def case(**changes):
    """합성 전역/타입/좌표/경계 응답의 기본 입력을 만든다."""
    value=dict(player=3,anchor=60,owner=3,mask=4,genus=4,x=bits(20.5),y=bits(21.25),footX=3,footY=2,
        timeLow=0,timeHigh=0,nowLow=0,nowHigh=0,path=1,speed=bits(1.5),sources=0,allocation=1,frame=0x12345678,mutate=0,bridge=82,count=4)
    value.update(changes);return value


def inputs():
    """실패 이전 효과·도착 시각/속도·여러 공급원·프레임 폭·현재 자료 변동을 교차한다."""
    result=[]
    # 기준점/첫 경로 실패가 자원 단계로 넘어가지 않는지 비교한다.
    for anchor,path,player in itertools.product((0,60),(0,1),(0,3,259,0xffffffff)):
        result.append(('Move',case(anchor=anchor,path=path,player=player,sources=2)))
    # 유효/지도 밖 좌표·홀수/짝수 발자국의 위 가운데 반올림을 실제 helper로 실행한다.
    for (x,y),foot,sources in itertools.product(((20.5,21.25),(20.00001,21.99999),(1.25,1.5),(-3.5,300.25),(255.9,254.5)),
            ((1,1),(3,2),(4,4),(10,6)),(0,1,2)):
        result.append(('Move',case(x=bits(x),y=bits(y),footX=foot[0],footY=foot[1],sources=sources)))
    # 요청 0/-0의 우회와 현재/과거/나중 시각, 현재 경로 단계 변경 뒤 최종 반환을 대조한다.
    for time,now,speed,mutate in itertools.product((0.0,-0.0,5.0,15.0,30.0),(0.0,2.5),(-1.0,0.0,1.5,64.0),(0,1,2)):
        low,high=double_words(time);nl,nh=double_words(now)
        result.append(('Move',case(timeLow=low,timeHigh=high,nowLow=nl,nowHigh=nh,speed=bits(speed),mutate=mutate,sources=2)))
    # 공급원 목록은 바깥 자원 선택 결과다. 실제 소비/단어/메타/좌표/할당/이동은 루트에서 관찰한다.
    for sources,allocation,frame,owner,bridge in itertools.product((0,1,2,3),(0,1),(0,255,256,0xffffffff),(1,8),(82,105)):
        result.append(('Move',case(sources=sources,allocation=allocation,frame=frame,owner=owner,bridge=bridge)))
    # 경계 뒤 현재 목표/기준점 소유자/새 객체 타입/이벤트/메타를 다시 읽는다.
    for mutate in range(3,9):result.append(('Move',case(mutate=mutate,sources=2,frame=0x12345678)))
    # genus와 접근 mask의 일치 여부·최상위 비트도 원본 DWORD 연산으로 검사한다.
    for mask,genus in itertools.product((0,4,0x80000004),(0,2,4,0xffffffff)):
        result.append(('Move',case(mask=mask,genus=genus,sources=1)))
    # 소비 helper의 signed 개수·구멍·중복·비활성 꼬리를 직접 비교한다.
    for profile,count in itertools.product(range(4),(0,1,2,3,4,0x80000000,0xffffffff)):
        result.append(('Take',case(sources=profile,count=count)))
    return result


def source_entries(profile):
    """입력 목록만 준비한다. 실제 반환/최종 목록은 기계어가 결정한다."""
    return ((0,0,0,0),(70,0,0,0),(0,70,71,70),(71,0,70,0))[profile]


class BuilderOracle(ReturnOracle):
    """원본 전체/기하/소비를 실행하고 가상 공간·경로·자원 정책의 인자만 관찰한다."""
    def __init__(self,edition):
        """공통 로더 뒤 이번 내보내기 범위와 경로/목록/결과 메모리를 준비한다."""
        super().__init__(edition);self.r=SPEC[edition];self.patch=edition=='originals';self.returns=collections.Counter();self.stub_calls=collections.Counter()
        self.exports=[ROOT/f'extracted/constructionbuilder/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 실제 함수별 불연속 범위 밖 실행은 거부한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 포함 끝 주소를 반열린 구간으로 바꾼다.
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.mu.mem_map(PATH,0x10000);self.mu.mem_map(SOURCES,0x1000);self.mu.mem_map(OUT,0x1000)
        self.mu.mem_write(VTABLE+0x74,struct.pack('<I',OWNER));self.mu.mem_write(VTABLE+0x90,struct.pack('<I',POP))
        self.mu.mem_write(VTABLE+0x30,struct.pack('<I',SPEED))
        self.stubs={self.r[key]:(letter,key) for key,letter in (('anchor','A'),('create','C'),('approach','P'),('move','M'),
            ('choose','Q'),('sort','S'),('allocate','L'),('timer','T'),('notify','N'))}
        self.stubs.update({OWNER:('O','owner'),POP:('B','pop'),SPEED:('V','speed')})

    def event(self,letter,*values):
        """모든 원본 인자를 비트 그대로 저장한다."""
        self.events.append(letter+(':'+':'.join(map(str,values)) if values else ''))

    def sid(self,address):
        """합성 raw 포인터가 슬롯 시작인지 검사하고 번호를 얻는다."""
        if address<POOL or (address-POOL)%self.stride:raise RuntimeError('건설 사제 this 정렬 오류')
        return (address-POOL)//self.stride

    def put(self,key,value):
        """경계 입력으로 현재 전역 DWORD를 바꾼다."""
        self.mu.mem_write(self.r['globals'][key],struct.pack('<I',value&0xffffffff))

    def stub(self,letter,key):
        """몸체 밖의 효과만 응답한다. 기하/반올림/소비/속도 계산은 대체하지 않는다."""
        mu=self.mu;c=self.case;result=0;purge=0
        if key=='anchor':self.event(letter,*self.args(4));result=c['anchor']
        elif key=='create':
            typ,flags=self.args(2);sid=100+self.born;self.born+=1
            raw=bytearray((i*37+11)&255 for i in range(self.stride));struct.pack_into('<I',raw,0,VTABLE);raw[10]=typ;raw[11]=4
            if c['mutate']==6 and self.born>1:raw[10]=106
            mu.mem_write(self.slot(sid),bytes(raw));self.event(letter,typ,flags,sid);result=self.slot(sid)
        elif key=='owner':
            sid=self.sid(mu.reg_read(UC_X86_REG_ECX));owner,=self.args(1);self.event(letter,sid,owner);purge=4
            mu.mem_write(self.slot(sid)+self.o['owner'],bytes([owner&255]))
            if c['mutate']==3 and sid==100:mu.mem_write(self.slot(50)+14,struct.pack('<2f',22.5,24.5))
        elif key=='approach':
            out,x,y,flags=self.args(4);self.event(letter,x,y,flags,*struct.unpack('<4I',mu.mem_read(mu.reg_read(UC_X86_REG_ECX),16)))
            mu.mem_write(out,struct.pack('<2f',10.25,11.5));result=out;purge=16
        elif key=='pop':
            sid=self.sid(mu.reg_read(UC_X86_REG_ECX));self.event(letter,sid,*self.args(3));purge=12
        elif key=='move':
            values=self.args(8 if self.patch else 7);self.event(letter,*values)
            if values[0]==100:result=PATH if c['path'] else 0
            if c['mutate']==2 and values[0]!=100:mu.mem_write(PATH+0x10,struct.pack('<I',1))
        elif key=='speed':
            self.event(letter,*self.args(2));purge=8
            if c['mutate']==1:mu.mem_write(PATH+0x10,struct.pack('<I',1))
        elif key=='choose':
            self.event(letter,*self.args(4));purge=16
            mu.mem_write(SOURCES,struct.pack('<4I',*source_entries(c['sources'])))
            if c['mutate']==4:
                mu.mem_write(self.slot(50)+14,struct.pack('<2f',25.5,26.5));mu.mem_write(self.slot(60)+self.o['owner'],bytes([8]))
        elif key=='sort':
            self.event(letter,*self.args(2));purge=8
            if c['mutate']==7:
                mu.mem_write(SOURCES,struct.pack('<4I',71,70,0,0));self.put('carrier',106)
            if c['mutate']==8:mu.mem_write(self.slot(70)+14,struct.pack('<2f',32.75,33.25))
        elif key=='allocate':
            self.event(letter,*self.args(1));result=ALLOCATION if c['allocation'] else 0
            if c['mutate']==5:self.put('event',0x12345678)
        elif key=='timer':self.event(letter,*self.args(3));purge=12;result=ALLOCATION
        elif key=='notify':self.event(letter,*self.args(1))
        else:raise RuntimeError('알 수 없는 건설 사제 경계')
        self.ret(result,purge)

    def on_instruction(self,mu,address,size,data):
        """경계의 실제 몸체는 실행하지 않고 나머지는 허용 명령만 실행한다."""
        found=self.stubs.get(address)
        if found is None:OwnerOracle.on_instruction(self,mu,address,size,data);return
        letter,key=found;self.stub(letter,key);self.stub_calls[letter]+=1

    def run_builder(self,operation,c,control):
        """입력 전역/raw를 만들고 SEH/ABI/x87 정상 반환과 출력/목록/생성 슬롯을 기록한다."""
        self.case=c;self.events=[];self.born=0;mu=self.mu;self.write_ranges=[]
        # 타입 메타/발자국을 매 행에서 초기화해 이전 경계 변경이 새 입력에 남지 않게 한다.
        for typ in (100,83,84,85,134,105,106):
            record=TYPES+typ*self.type_stride;mu.mem_write(record,bytes(self.type_stride))
            foot=(c['footX'],c['footY']) if typ==100 else (3,2) if typ in (83,84,85) else (1,1)
            mu.mem_write(record+(0x1d4 if self.patch else 0x1b4),struct.pack('<2i',*foot))
            mu.mem_write(record+0x98,struct.pack('<I',1 if typ==84 else 2 if typ==85 else 0))
            mu.mem_write(record+0xe0,struct.pack('<I',c['speed']));mu.mem_write(record+0xec,struct.pack('<I',c['genus']))
            mu.mem_write(record+0x130,struct.pack('<16I',*[c['frame']+i+(256 if typ==106 else 0)&0xffffffff for i in range(16)]))
        # 목표/기준점/두 공급원은 raw와 실제 기하 helper를 공유한다.
        for sid,typ,x,y in ((50,100,struct.unpack('<f',struct.pack('<I',c['x']))[0],struct.unpack('<f',struct.pack('<I',c['y']))[0]),
                (60,83,10.25,11.5),(70,84,30.25,31.5),(71,85,40.5,41.75)):
            raw=bytearray(self.stride);raw[10]=typ;raw[self.o['owner']]=c['owner'];struct.pack_into('<2f',raw,14,x,y)
            mu.mem_write(self.slot(sid),bytes(raw))
        # 모든 효과 단계가 읽는 현재 전역과 첫 경로의 두 단계 시각이다.
        for key,value in [('builder',134),('carrier',105),('bridge',c['bridge']),('mask',c['mask']),('event',0x25b),('list',SOURCES),('count',c['count'])]:self.put(key,value)
        mu.mem_write(self.r['globals']['now'],struct.pack('<2I',c['nowLow'],c['nowHigh']))
        mu.mem_write(PATH,struct.pack('<I',VTABLE));mu.mem_write(PATH+0x10,bytes(4));mu.mem_write(PATH+self.r['arrival'],struct.pack('<2d',10.0,12.5))
        mu.mem_write(SOURCES,struct.pack('<4I',*source_entries(c['sources'])));mu.mem_write(OUT,bytes(8));mu.mem_write(0,struct.pack('<I',0x11223344))
        if not self.patch:mu.mem_write(0x546778,bytes(4))
        if self.patch:mu.mem_write(0x5e4794,bytes(4))
        self.write_ranges=[(0,4),(OUT,OUT+8),(SOURCES,SOURCES+16)]
        # native 루트가 쓰는 새 운반 객체 WORD/프레임만 허용한다. factory/가상 경계의 입력 쓰기는 별도다.
        for sid in range(101,106):
            self.write_ranges.extend([(self.slot(sid)+12,self.slot(sid)+14),
                (self.slot(sid)+(36 if self.patch else 34),self.slot(sid)+(40 if self.patch else 35))])
        self.prepare_registers(control)
        entry=self.r['entry' if operation=='Move' else 'next'];args=[OUT,c['player'],50,c['timeLow'],c['timeHigh']] if operation=='Move' else [OUT]
        mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.emu_start(entry,STOP,count=100000)
        self.check(control,STACK+4+(4 if operation=='Take' else 0));self.returns[f'{entry:08x}']+=1
        if bytes(mu.mem_read(0,4))!=struct.pack('<I',0x11223344):raise RuntimeError('건설 사제 SEH 머리 복구 오류')
        out=bytes(mu.mem_read(OUT,8)).hex() if operation=='Move' else str(mu.reg_read(UC_X86_REG_EAX))+':'+str(struct.unpack('<I',mu.mem_read(OUT,4))[0])
        raws=';'.join(bytes(mu.mem_read(self.slot(100+i),self.stride)).hex() for i in range(self.born)) or '-'
        return [';'.join(self.events) or '-',out,raws,','.join(map(str,struct.unpack('<4I',mu.mem_read(SOURCES,16))))]


def generate(smoke=False):
    """세 PE를 각각 두 정밀도로 실행해 출력이 같을 때만 독립 fixture/근거를 저장한다."""
    rows=[];editions={};paths={Path(__file__),FIXTURE,ROOT/'tools/ghidra/constructionbuilder-functions.json',
        *[ROOT/f'tools/decomp_{name}_oracle.py' for name in ('owner','priestgeometry','returnpiece','damageablepredestroy')]}
    # 각 실제 PE를 별도 가상 메모리로 실행한다.
    for edition in SPECS:
        oracle=BuilderOracle(edition);plan=inputs()[::29] if smoke else inputs();counts=collections.Counter()
        # 함수 진입/정상 반환과 명시 경계 인자를 두 실행에서 함께 검사한다.
        for op,c in plan:
            observed=[oracle.run_builder(op,c,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError(f'건설 사제 x87 관찰 차이 {edition} {op} {c}')
            rows.append([edition,op,','.join(str(c[key]) for key in KEYS),*observed[0]]);counts[op]+=1
        editions[edition]=dict(cases=dict(counts),returns=dict(oracle.returns),native_calls=dict(oracle.native_calls),substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary'])
        # 공통 로더가 실제로 읽는 이전 내보내기도 SHA 근거에 포함한다.
        for group in ('owner','priestgeometry','returnpiece'):
            paths.update(ROOT/f'extracted/{group}/{edition}/{name}' for name in ('creation.c','functions.tsv'))
        print(f'{edition}: constructionbuilder {len(plan)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 건설 사제/운반 생성·이동 전체/기하/목록 소비. 경로/자원 정책/가상 효과는 명시 경계.\n'
        '# 판본 연산 입력('+ ' '.join(KEYS)+') 사건 반환 생성raw 최종sources\n'+
        '\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='VM-W11-CODEX',date='2026-10-13',controls=list(CONTROLS),os_calls=0,editions=editions,
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},limits=[
            '실제 전체 루트·발자국/위 가운데/중심·반올림/CRT·genus/타입·단어 쓰기·공급원 소비',
            '기준점/factory/가상 소유자/공간/경로/속도 적용·자원 선택/정렬·할당/Regular/표면 알림은 명시 경계; Regular 생성자의 실제 세 인자는 event,parent,payload이며 첫0/마지막00532750의 의미를 이전 문서에서 반대로 해석했던 것을 교정',
            '유한 합성 좌표/작은 발자국/시각·타입 100/83/84/85/134/105/106·0..3 공급원; CD Pentium FDIV 우회 전역 0',
            '원본 assert/게임/OS/예외 처리기 미실행, 정상 SEH/스택/보존 레지스터/x87·허용 실행/쓰기·SHA 감사']),
        ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'total: {len(rows)}',flush=True)


def verify():
    """기대 동작을 재계산하지 않고 SHA·입력·반환 폭·진입/반환/경계 집계를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 모든 실행/입력 근거 파일이 생성 시점 그대로인지 검사한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError('SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('건설 사제 메타 합계 오류')
    # 원본 출력의 결과 의미를 재생성하지 않고 계획 입력/행 폭/효과 수만 대조한다.
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition];plan=inputs();counts=collections.Counter();events=[]
        if len(chosen)!=len(plan) or item['assertions']:raise RuntimeError('건설 사제 행/assert 오류')
        # 생성 raw의 길이와 반환 비트 폭은 판본별로 검사한다.
        for row,(op,c) in zip(chosen,plan,strict=True):
            if len(row)!=7 or row[1]!=op or row[2]!=','.join(str(c[k]) for k in KEYS):raise RuntimeError('건설 사제 입력 순서 오류')
            if op=='Move' and len(bytes.fromhex(row[4]))!=8:raise RuntimeError('건설 사제 double 반환 폭 오류')
            if op=='Take' and (len(row[4].split(':'))!=2 or int(row[4].split(':')[0]) not in (0,1) or not 0<=int(row[4].split(':')[1])<=0xffffffff):raise RuntimeError('건설 공급원 반환 폭 오류')
            if row[5]!='-' and any(len(bytes.fromhex(raw))!=SPECS[edition]['stride'] for raw in row[5].split(';')):raise RuntimeError('건설 사제 raw 폭 오류')
            if len(row[6].split(','))!=4:raise RuntimeError('건설 사제 목록 폭 오류')
            counts[op]+=1;events.extend(row[3].split(';'))
        if dict(counts)!=item['cases']:raise RuntimeError('건설 사제 연산 합계 오류')
        # 두 정밀도의 루트 진입/정상 반환을 각각 대조한다.
        for op,key in (('Move','entry'),('Take','next')):
            addr=f'{SPEC[edition][key]:08x}';expected=2*counts[op]
            if item['returns'].get(addr,0)!=expected or (key=='entry' and item['native_calls'].get(addr,0)!=expected):raise RuntimeError('건설 사제 정상 반환 오류')
        # 실제 루트 안에서 소비 helper를 추가 호출하므로 직접 진입 수와는 구분한다.
        if item['native_calls'].get(f"{SPEC[edition]['next']:08x}",0)<2*counts['Take']:raise RuntimeError('건설 공급원 실제 진입 누락')
        for letter in 'ACPMQSLTNOBV':
            if item['substitutions'].get(letter,0)!=2*sum(e==letter or e.startswith(letter+':') for e in events):raise RuntimeError('건설 사제 경계 합계 오류: '+letter)
        if any(item['native_calls'].get(f'{SPEC[edition][key]:08x}',0) for key in ('anchor','create','approach','move','choose','sort','allocate','timer','notify')):raise RuntimeError('건설 사제 경계 몸체 실행')
    print(f'constructionbuilder 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 근거 감사 중 선택한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)


if __name__=='__main__':main()
