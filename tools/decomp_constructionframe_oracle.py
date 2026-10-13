#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""건설 표시 프레임·확장·추가·시각·팔레트·Draw의 전체 x86 몸체를 세 PE에서 독립 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_constructionvisual_oracle import (VisualOracle,DATA,OBJECT,PALETTE,SOURCE,OUTPUT,PARTICLES,PARENT,
    ROOT,BINARIES,CONTROLS,POOL,digest,fbits,UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)
from decomp_process_oracle import ProcessOracle,dbits

# 실제 전체 몸체와 외부 응답 경계의 판본별 주소다.
FRAME={'originals':dict(run=0x422f00,expand=0x422ea0,append=0x422e30,draw=0x423200,reset=0x418400,tick=0x418430,
    color=0x487e50,rect=0x487ba0,integer=0x4558c0,real=0x455930,maxhp=0x4adcb0,dirty=0x499310,
    blit=0x4a2020,kill=0x41bf10,rate=0x59a934,camera=(0x59a938,0x59a93c)),
    'originalCD':dict(run=0x4ea7e0,expand=0x4ea780,append=0x4ea700,draw=0x4eaa90,reset=0x45e280,tick=0x45e2b0,
    color=0x45d340,rect=0x45ce80,integer=0x48cbd0,real=0x48cc50,maxhp=0x4aed30,dirty=0x456d10,
    blit=0x423920,kill=0x48f7b0,rate=0x565c4c,camera=(0x565c50,0x565c54))}
FRAME['original1037']=dict(FRAME['originalCD'])
# 합성 색 DWORD 배열은 count 경계 바로 뒤의 명시한 한 DWORD도 포함한다.
COLORS=PALETTE+0x100
KEYS=('instant','pattern','random','capacity','total','active','selected','hp','maximum','first','now','last','elapsed',
    'rate','x','y','sx','sy','cx','cy','pixel','life','colors','mutate','free','parent','source','maxlife')
OFFSETS=(0xc,0x10,0x14,0x18,0x40,0x44,0x74,0x78,0x7c,0x80,0x24,0x28)
FIXTURE=ROOT/'cpppj/tests/fixtures/constructionframe-x86.tsv'
REPORT=ROOT/'cpppj/constructionframe.json'


def case(**changes):
    """유한 raw·버퍼·입자·시각 입력만 만들고 기대 갱신 값은 계산하지 않는다."""
    c=dict(instant=0,pattern=0,random=17,capacity=12,total=3,active=0,selected=3,hp=50,maximum=101,
        first=1,now=dbits(.025),last=dbits(0),elapsed=dbits(1000000.125),rate=fbits(.7),
        x=fbits(20.1),y=fbits(-3.2),sx=fbits(16),sy=fbits(11),cx=fbits(.75),cy=fbits(-.25),
        pixel=27,life=fbits(.25),colors=11,mutate=0,free=0,parent=PARENT,source=1,maxlife=fbits(2))
    c.update(changes);return c


def plan(edition):
    """용량·HP·첫 시각·만료·선형 행 경계·버전별 읽기/반올림을 조합한다."""
    rows=[]
    # 같은 저장소에서 활성화/만료/가득 찬 용량을 함께 관찰한다.
    for first,hp,layout,now in itertools.product((0,1),(0,49,100,-1,65537,0x12340032),range(4),(0,.025,.125,-.025)):
        capacity,total,active=((12,3,0),(12,3,2),(3,3,1),(12,0,0))[layout]
        rows.append(('Run',case(first=first,hp=hp,capacity=capacity,total=total,active=active,now=dbits(now),
            instant=layout%2,maxlife=fbits(9 if layout%2 else 2))))
    # 경계 중 HP·좌표·시각·카메라·abstract가 바뀌는 읽기 순서를 확인한다.
    for mutate,hp in itertools.product(range(1,6),(0,100)):
        rows.append(('Run',case(mutate=mutate,hp=hp)))
    # 슬롯 경계·이웃 순서·투명 후보·남은 용량을 독립 진입으로 확인한다.
    for pixel,capacity,pattern in itertools.product((0,8,9,15,16,55,56,63),(3,4,5,12),(0,2)):
        rows.append(('Expand',case(pixel=pixel,capacity=capacity,pattern=pattern)))
    for pixel,capacity in itertools.product((0,8,27,63),(3,4)):
        rows.append(('Append',case(pixel=pixel,capacity=capacity)))
    if edition!='originals':
        # CD는 잘못된 픽셀에서도 total만 먼저 증가하는 정상 반환 경로가 있다.
        rows.extend(('Append',case(pixel=pixel)) for pixel in (-1,64))
        rows.extend(('Run',case(**change)) for change in ({'free':1},{'parent':0},{'source':0},{'parent':32768}))
    # 색 경계 바로 뒤의 DWORD는 명시 입력이며 임의 인접 메모리를 읽지 않는다.
    for maximum,count,life in itertools.product((2,9,1.3),(0,3,11),(-1,0,.125,.25,.5,1,1.3,2,9,10)):
        rows.append(('Color',case(maxlife=fbits(maximum),colors=count,life=fbits(life))))
    # float 저장 전에 rate를 곱하는 순서와 double 누적을 드러내는 시각 입력이다.
    for first,now,elapsed,rate in itertools.product((0,1),(0,.010000001,.123456789,-.010000001),
            (0,1000000.125,16777216.125),(.7,1.1234567)):
        rows.append(('Clock',case(first=first,now=dbits(now),elapsed=dbits(elapsed),rate=fbits(rate))))
    # 시간 차이가 float 1로 저장돼도 반올림 전 rate 곱은 다음 float 색인 쪽으로 넘어간다.
    rows.append(('Clock',case(now=dbits(.1000000059),rate=fbits(.7))))
    for capacity,x,camera in itertools.product((0,12),(20.1,-20.1,1048576.125),(0,.75,-100.25)):
        rows.append(('Draw',case(capacity=capacity,total=0,x=fbits(x),y=fbits(x),cx=fbits(camera),cy=fbits(camera))))
    for instant,seed in itertools.product((0,1),(0,0xffffffff)):
        rows.append(('Sequence',case(instant=instant,maxlife=fbits(9 if instant else 2),random=seed,first=0)))
    return rows


def image(c):
    """입력 전용 8×8 캡처를 만들며 0xff만 투명으로 취급한다."""
    return bytes(255 if c['pattern']==2 and i%3==0 else (i*13+17)%255 for i in range(64))


class FrameOracle(VisualOracle):
    """기존 제한 PE/ABI 로더에 새 몸체와 최대 HP/dirty/장치/Kill 응답만 더한다."""
    def __init__(self,edition):
        """읽기 전용 내보내기의 불연속 범위와 필요한 전역 쓰기를 등록한다."""
        super().__init__(edition);self.f=FRAME[edition]
        self.extra.extend(ROOT/f'extracted/constructionframe/{edition}/{name}' for name in ('creation.c','functions.tsv'))
        with self.extra[-1].open(encoding='utf-8') as fp:
            # 함수 사이의 코드는 허용하지 않는다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entry_names[int(row['entry'],16)]=row['entry']
                # 실제 명령 범위만 추가한다.
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.writes.extend((a,a+width) for a,width in ((self.s['now'],8),(self.f['rate'],4),
            *((a,4) for a in self.f['camera']),*((a,4) for a in self.d['scales'])))

    def on_instruction(self,mu,address,size,data):
        """실제 알고리즘은 대체하지 않고 네 외부 경계의 인자와 순서를 검사한다."""
        f=self.f;c=getattr(self,'case',None)
        if address in (f['maxhp'],f['dirty'],f['blit'],f['kill']):
            key=next(k for k in ('maxhp','dirty','blit','kill') if f[k]==address);self.boundaries[key]+=1
            if key=='maxhp':
                if mu.reg_read(UC_X86_REG_ECX)!=POOL+PARENT*self.s['stride']:raise RuntimeError('최대 HP this 오류')
                self.events.append('H:50')
                if c['mutate']==1:self.put(POOL+PARENT*self.s['stride']+26,1)
                if c['mutate']==2:self.put(POOL+PARENT*self.s['stride']+26,100)
                if c['mutate']==3:
                    self.put(POOL+PARENT*self.s['stride']+14,fbits(-7.75));self.put(self.s['now'],dbits(30),8)
                self.ret(c['maximum']);return
            if key=='dirty':
                values=self.args(5)
                if values[-1]:raise RuntimeError('dirty flags 오류')
                signed=struct.unpack('<4i',struct.pack('<4I',*values[:4]));self.events.append('I:'+':'.join(map(str,signed)))
                if self.dirty_count==0 and c['mutate']==4:self.put(f['camera'][0],fbits(100.25))
                if self.dirty_count==0 and c['mutate']==5:self.put(POOL+PARENT*self.s['stride']+(40 if self.patch else 35),0,1)
                self.dirty_count+=1;self.ret();return
            if key=='kill':
                if self.args(1)!=(0,) or mu.reg_read(UC_X86_REG_ECX)!=OBJECT:raise RuntimeError('Kill 인자 오류')
                self.events.append('K:0');self.ret(0,4);return
            args=self.args(8)
            if args[2]!=OUTPUT or args[3:]!=(8,0,0,8,8):raise RuntimeError('Draw 버퍼/영역 오류')
            xy=struct.unpack('<2i',struct.pack('<2I',*args[:2]));self.events.append('D:'+':'.join(map(str,(*xy,*args[3:]))));self.ret();return
        ProcessOracle.on_instruction(self,mu,address,size,data)

    def invoke(self,key,args=(),this=OBJECT,purge=0):
        """정상 반환·스택/x87/SEH와 보존 레지스터를 확인하고 실제 루트 반환 수를 기록한다."""
        result=self.call(self.f[key],args,this=this,purge=purge)
        if any(self.mu.reg_read(reg) for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)):
            raise RuntimeError('건설 프레임 보존 레지스터 오류')
        self.returns[f'{self.f[key]:08x}']+=1;return result

    def snapshot(self):
        """관찰한 필드·전체 용량 입자·두 버퍼·난수만 직렬화한다. 기대 동작은 계산하지 않는다."""
        fields=':'.join(str(self.u32(OBJECT+off)) for off in OFFSETS)
        fields+=':'+':'.join(str(struct.unpack('<Q',self.mu.mem_read(OBJECT+off,8))[0]) for off in (0x30,0x38))
        particles=[]
        # 사용을 마친 칸의 잔여 값도 두 번 복사하는 만료 처리의 근거다.
        for i in range(self.u32(OBJECT+0x24)):
            active,color,padding,life,pointer=struct.unpack('<BBHII',self.mu.mem_read(PARTICLES+12*i,12))
            if padding:raise RuntimeError('입자 패딩 변경')
            particles.append(':'.join(map(str,(active,color,life,pointer-OUTPUT if pointer else 0))))
        return [fields,bytes(self.mu.mem_read(SOURCE,64)).hex(),bytes(self.mu.mem_read(OUTPUT,64)).hex(),
            ';'.join(particles) or '-',str(self.u32(self.d['random']))]

    def run_frame(self,operation,c,control):
        """원본 whole body를 실행해 최종 상태와 연속 프레임의 모든 중간 상태를 관찰한다."""
        self.begin(dict(server=1,boss=1,rotation=0,now=c['now']),control);self.case=c;self.dirty_count=0;mu=self.mu;d=self.d;f=self.f
        self.put(d['random'],c['random']);self.put(f['rate'],c['rate'])
        if not self.patch:self.put(0x546778,0)
        # 현재 표시 배율/카메라 입력을 각각 원본 전역에 쓴다.
        for address,value in zip((*d['scales'],*f['camera']),(c['sx'],c['sy'],c['cx'],c['cy'])):self.put(address,value)
        raw=bytearray(self.s['stride']);struct.pack_into('<I',raw,0,self.s['base_vtable']);raw[10]=100;raw[11]=c['free']
        raw[40 if self.patch else 35]=1;struct.pack_into('<3I',raw,14,c['x'],c['y'],0);struct.pack_into('<I',raw,26,c['hp']&0xffffffff)
        mu.mem_write(POOL+PARENT*self.s['stride'],bytes(raw));mu.mem_write(OBJECT,bytes(136))
        self.put(PALETTE,c['colors']);self.put(PALETTE+4,COLORS)
        # 조회 가능한 끝 경계 DWORD까지 명시 입력 색을 채운다.
        for i in range(c['colors']+1):self.put(COLORS+4*i,0x81000000+i*257+7)
        values={0:(0x503680 if c['instant'] else 0x5036ac) if self.patch else (0x507410 if c['instant'] else 0x507440),
            8:c['parent'],0xc:fbits(1.1),0x10:fbits(-2.1),0x14:fbits(3.1),0x18:fbits(4.1),
            0x24:c['capacity'],0x28:c['total'],0x2c:PARTICLES,0x40:fbits(13),0x44:fbits(17),0x48:PALETTE,
            0x4c:SOURCE if c['source'] else 0,0x50:OUTPUT,0x54:8,0x58:8,0x5c:-3,0x60:2,0x64:56,
            0x6c:fbits(2) if c['instant'] else 0x3fa66666,0x70:c['maxlife'],0x74:c['selected'],0x78:64,
            0x7c:c['first'],0x80:c['active']}
        # 실행 중 변경 가능한 필드의 입력만 준비한다.
        for off,value in values.items():self.put(OBJECT+off,value)
        self.put(OBJECT+0x30,c['last'],8);self.put(OBJECT+0x38,c['elapsed'],8)
        source=bytearray(image(c));mu.mem_write(OUTPUT,b'\xff'*64)
        # 원본에 공급하는 세 초기 입자다. 선택 후 소스는 이미 투명하다.
        for i in range(c['total']):
            pixel=(27,35,43)[i];life=struct.unpack('<f',struct.pack('<I',c['life']))[0]*(i+1)
            mu.mem_write(PARTICLES+i*12,struct.pack('<BBHII',int(i<c['active']),source[pixel],0,fbits(life),OUTPUT+pixel));source[pixel]=255
        mu.mem_write(SOURCE,bytes(source));result=0;history=[]
        if operation=='Run':self.invoke('run')
        elif operation=='Expand':self.invoke('expand',[OUTPUT+c['pixel']],purge=4)
        elif operation=='Append':
            address=self.invoke('append',[c['pixel']],purge=4);result=0 if not address else (address-PARTICLES)//12+1
        elif operation=='Color':result=self.invoke('color',[c['life'],c['maxlife']],this=PALETTE,purge=8)
        elif operation=='Clock':
            if not c['first']:self.invoke('reset',this=OBJECT+0x30);self.put(OBJECT+0x7c,1)
            self.invoke('tick',this=OBJECT+0x30)
        elif operation=='Draw':self.invoke('draw',[37,41],purge=8)
        else:
            # 중간 상태도 저장하며 매 프레임 HP와 현재 시각만 외부 입력으로 변경한다.
            for step in range(5):
                self.put(self.s['now'],dbits(step*.0625),8);self.put(POOL+PARENT*self.s['stride']+26,step*25)
                if step==4:self.put(POOL+PARENT*self.s['stride']+(40 if self.patch else 35),0,1)
                self.invoke('run');self.invoke('draw',[37,41],purge=8);history.append('|'.join(self.snapshot()))
        return [';'.join(self.events) or '-',*self.snapshot(),str(result),'~'.join(history) or '-']


def generate(smoke):
    """두 x87 제어 워드의 동일한 실제 관찰만 fixture와 SHA 근거로 저장한다."""
    rows=[];editions={};files={Path(__file__),ROOT/'tools/decomp_process_oracle.py',ROOT/'tools/decomp_constructionvisual_oracle.py',
        ROOT/'tools/ghidra/constructionvisual-functions.json',ROOT/'tools/ghidra/constructionframe-functions.json',FIXTURE}
    # 각 PE를 독립 메모리에 읽으며 원본 파일/게임/장치는 실행하거나 수정하지 않는다.
    for edition in BINARIES:
        oracle=FrameOracle(edition);cases=plan(edition);cases=cases[::29]+cases[-4:] if smoke else cases
        # C++ 구현을 호출하지 않고 두 기계어 실행 관찰을 직접 비교한다.
        for index,(operation,c) in enumerate(cases,1):
            observed=[oracle.run_frame(operation,c,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError(f'프레임 x87 관찰 차이: {edition} {operation} {c}')
            rows.append([edition,operation,','.join(str(c[key]) for key in KEYS),*observed[0]])
            if index%100==0:print(f'{edition}: {index}/{len(cases)}개 대조',flush=True)
        editions[edition]=dict(cases=dict(collections.Counter(op for op,_ in cases)),entries=dict(oracle.hits),
            returns=dict(oracle.returns),boundaries=dict(oracle.boundaries),assertions=oracle.asserts)
        files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(f'{edition}: constructionframe {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 건설 프레임/확장/추가/팔레트/시각/Draw. 최대 HP·dirty 큐·장치 blit·Kill만 응답 경계.\n'+
        '\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),controls=list(CONTROLS),host='HJOW-Athlon',date='2026-10-11',os_calls=0,
        editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},limits=[
            'RunFrame/가상 확장/Append/Clock/팔레트/사각형/Draw·실제 정수/실수 난수/CRT 전체 몸체',
            '최대 HP·dirty 큐·장치 blit·BaseProcess Kill만 명시 응답 경계; 장치 출력/삭제/등록은 후속',
            '유한 합성 8x8 이미지/시간/배율/카메라, 사용·미사용 전체 용량 입자/두 버퍼/시드/연속 중간 상태 관찰',
            '팔레트 count 경계 뒤 한 DWORD는 명시 입력, CD Pentium FDIV 우회 전역=0, 원본 assert/잘못된 패치 포인터 실행 없음',
            'ABI/보존 레지스터/x87/SEH·허용 실행/쓰기·SHA/입력/폭/진입/반환/외부 경계 감사']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """저장 SHA·입력 순서·행/버퍼/입자 폭·실제 진입/정상 반환·외부 사건 집계만 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[l.split('\t') for l in FIXTURE.read_text(encoding='utf-8').splitlines() if l and not l.startswith('#')]
    # 입력/내보내기/로더/관찰의 바이트 변경을 모두 검출한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError('건설 프레임 SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls'] or set(report['editions'])!=set(BINARIES):raise RuntimeError('프레임 총계 오류')
    # 판본별 계획/정상 반환/경계 사건을 관찰만으로 집계한다.
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition];cases=plan(edition);returns=collections.Counter();boundaries=collections.Counter();f=FRAME[edition]
        if len(chosen)!=len(cases) or item['assertions']:raise RuntimeError('프레임 행/assert 오류')
        # 기대 갱신 계산 없이 각 저장 열과 실제 입력만 검사한다.
        for row,(op,c) in zip(chosen,cases,strict=True):
            if len(row)!=11 or row[1]!=op or row[2]!=','.join(str(c[k]) for k in KEYS):raise RuntimeError('프레임 입력/열 오류')
            snapshots=[row[4:9]] if row[10]=='-' else [s.split('|') for s in row[10].split('~')]
            if op=='Sequence' and (len(snapshots)!=5 or snapshots[-1]!=row[4:9]):raise RuntimeError('연속 중간 관찰 오류')
            # 전체 저장소의 폭을 확인하고 원래 입자 선택/만료는 재계산하지 않는다.
            for snap in snapshots:
                values=[int(v) for v in snap[0].split(':')];particles=[] if snap[3]=='-' else snap[3].split(';')
                if len(values)!=14 or any(not 0<=v<=0xffffffff for v in values[:12]) or any(not 0<=v<=0xffffffffffffffff for v in values[12:]):raise RuntimeError('프레임 필드 폭 오류')
                if len(bytes.fromhex(snap[1]))!=64 or len(bytes.fromhex(snap[2]))!=64 or len(particles)!=c['capacity'] or not 0<=int(snap[4])<=0xffffffff:raise RuntimeError('프레임 버퍼/입자 폭 오류')
                # 미사용 칸도 실제 잔여 DWORD 범위를 검사한다.
                for particle in particles:
                    active,color,life,pixel=(int(v) for v in particle.split(':'))
                    if active not in (0,1) or not 0<=color<=255 or not 0<=life<=0xffffffff or not 0<=pixel<64:raise RuntimeError('입자 범위 오류')
            keys=['run']*5+['draw']*5 if op=='Sequence' else (['reset','tick'] if not c['first'] else ['tick']) if op=='Clock' else [op.lower()]
            # 명시적으로 실행한 루트의 정상 반환을 집계한다.
            for key in keys:returns[f'{f[key]:08x}']+=2
            # 실제 경계 사건만 집계하며 좌표/인자를 재계산하지 않는다.
            for event in row[3].split(';'):
                if event!='-':boundaries[{'H':'maxhp','I':'dirty','D':'blit','K':'kill'}[event[0]]]+=2
            if not 0<=int(row[9])<=0xffffffff:raise RuntimeError('루트 반환 폭 오류')
        if dict(returns)!=item['returns'] or dict(boundaries)!=item['boundaries'] or item['cases']!=dict(collections.Counter(op for op,_ in cases)):raise RuntimeError('프레임 호출 집계 오류')
        # 모든 정상 반환 루트와 내부 helper가 실제 진입했는지 확인한다.
        for address,count in returns.items():
            if item['entries'].get(address,0)<count:raise RuntimeError('프레임 루트 진입 누락')
        for key in ('run','expand','append','draw','reset','tick','color','rect','integer','real'):
            if not item['entries'].get(f'{f[key]:08x}',0):raise RuntimeError('프레임 내부 진입 누락: '+key)
    print(f"constructionframe 근거 감사 통과: {len(rows)}개, OS/게임 호출 0")


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate(args.smoke)
