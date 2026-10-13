#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""미리보기 Draw의 실제 분기·좌표·격자·글자 배치를 세 PE에서 실행한다. 픽셀/글꼴/번역만 경계다."""
import argparse
import collections
import csv
import json
import random
import struct
from decomp_drawsort_oracle import REGISTERS
from decomp_process_oracle import (ProcessOracle,ROOT,BINARIES,CONTROLS,STACK,STOP,digest,fbits,
    UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 격리 작업 항목·raw 슬롯·타입 표·반환 문자열 주소다.
DATA,ENTRY,RAW,TYPES,STRINGS=0x16000000,0x16000100,0x16000200,0x16001000,0x16030000
FIXTURE=ROOT/'cpppj/tests/fixtures/previewdraw-x86.tsv'
REPORT=ROOT/'cpppj/previewdraw.json'
SEED=0x44627032
# 첫 29개 상태는 두 판본의 실제 전역 입력이다. 이하 타입/슬롯/격자 입력은 별도다.
FIELDS=('parent','legal','type','destroy_type','no_outline','overlay_trigger','overlay_type','label_type','owner','camera_x','camera_y',
    'grid_option','show_letters','spacing','label_offset_x','label_offset_y','label_width','label_height','flags','message_override',
    'blocked','powered','reachable','price_width','price_icon_offset','price_icon_x','price_icon_y','price_type','has_owner_map',
    'foot_x','foot_y','hot_x','hot_y','type_flags','tail','frame','screen_x','screen_y','world_x','world_y')
SPECS={
 'originals':dict(root=0x446270,stride=50,type_stride=500,table=0x59ab20,foot=0x1d4,hot=0x1dc,frame=36,
    globals=(0x55a278,0x55a290,0x55a284,0x5411a0,0x5412c4,0x5411e4,0x541244,0x5411f4,0x540c70,0x59a91c,0x59a920,
        0x5c78d4,0x55a2d0,0x5325e0,0x55a2b0,0x55a2b4,0x55a2b8,0x55a2bc,0x55a2e0,0x55a29c,0x55a2dc,0x55a294,0x55a298,
        0x55a270,0x55a26c,0x55a19c,0x55a1a0,0x55a2d4,0x557940),
    colors=(0x555f38,0x556038,0x556338),owner_maps=0x557940,labels=0x55a308,label_frames=0x592b78,grid=0x59a9f0,
    widths=0x55a254,heights=0x55a1a4,messages=0x5325cc,price=0x55a1bc,
    good=0x5bec2c,bad=0x5b5e58,
    boundary={0x4a24b0:'outline',0x4a2180:'fill',0x4daa70:'plain',0x4daab0:'mapped',0x4dab90:'translated',
        0x4dabe0:'map',0x4de9a0:'message',0x4a3f70:'text'}),
 'originalCD':dict(root=0x43dbd0,stride=36,type_stride=468,table=0x51c960,foot=0x1b4,hot=0x1bc,frame=34,
    globals=(0x51b624,0x51b63c,0x51b630,0x51ca8c,0x51cbb0,0x51cad0,0x51cb30,0x51cae0,0x50f6c8,0x565c34,0x565c38,
        0x516af0,0x51b680,0x51b6ac,0x51b660,0x51b664,0x51b668,0x51b66c,0x51b6a8,0x51b648,0x51b6a4,0x51b640,0x51b644,
        0x565230,0x5652e8,0x5652e0,0x5652e4,0x51b69c,0x5365c8),
    colors=(0x56dee8,0x56dfe8,0x56dde8),owner_maps=0x5365c8,labels=0x565328,label_frames=0x530460,grid=0x565998,
    widths=0x565250,heights=0x565238,messages=0x51b688,price=0x5652f0,
    good=0x55bad0,bad=0x55bac8,
    boundary={0x423cf0:'outline',0x423a80:'fill',0x4d64e0:'plain',0x4d6510:'mapped',0x4d65e0:'translated',
        0x4d6630:'map',0x4bcb20:'message',0x4260f0:'text'})}
SPECS['original1037']=dict(SPECS['originalCD'])

def plan():
    """원본 기대 그리기를 계산하지 않고 완성 상태·슬롯·표 입력만 만든다."""
    cases=[]
    def add(labels='ABaz',grid='sparse',**values):
        """현재 상태/타입/문구 측정 입력과 희소 또는 전체 격자를 모은다."""
        c=dict(parent=0,legal=1,type=70,destroy_type=90,no_outline=90,overlay_trigger=90,overlay_type=71,label_type=72,
            owner=2,camera_x=13,camera_y=-7,grid_option=0,show_letters=0,spacing=9,label_offset_x=10,label_offset_y=6,
            label_width=101,label_height=35,flags=0,message_override=0,blocked=0,powered=1,reachable=1,price_width=45,
            price_icon_offset=27,price_icon_x=-3,price_icon_y=2,price_type=73,has_owner_map=1,
            foot_x=2,foot_y=3,hot_x=5,hot_y=-4,type_flags=0,tail=0,frame=17,screen_x=322,screen_y=187,
            world_x=fbits(20.25),world_y=fbits(17.75))
        c.update(values);c['labels']=labels.encode('latin-1').hex() or '-'
        c['grid']=bytes((1 if grid=='all' or (grid=='sparse' and i%5==0) else 0) for i in range(144)).hex();cases.append(c)
    # 본체는 부모/플래그/특수 타입의 윤곽 조건과 합법/소유자/파괴 색 표를 구분한다.
    for tail in (0,1,2,3,4,255):
        for parent in (0,1):
            for flags in (0,4,0x100000,0x100004):
                for legal in (0,1): add(tail=tail,parent=parent,type_flags=flags,legal=legal)
    for owner in (-1,0,1,8):
        for mapped in (0,1):
            for legal in (0,1):
                for destroy in (70,90): add(owner=owner,has_owner_map=mapped,legal=legal,destroy_type=destroy)
    for special in (70,90):
        for legal in (0,1): add(no_outline=special,overlay_trigger=special,legal=legal)
    # 패치 DWORD와 CD BYTE 프레임 폭을 구분한다. 출력 경계만 관찰하므로 큰 인덱스도 입력으로 가능하다.
    for frame in (0,255,256,65535,0xffffffff): add(frame=frame)
    # 실패 사유 선택과 억제, 비용 표시, 글자 필터를 서로 독립적으로 바꾼다.
    for flags in (0,0x20,0x40,0x60,0x120,0x140):
        for legal in (0,1):
            for blocked in (0,1):
                for powered in (0,1):
                    for reachable in (0,1): add(tail=2,flags=flags,legal=legal,blocked=blocked,powered=powered,reachable=reachable)
    for override in (0,1):
        for letters in (0,1):
            for labels in ('','ABaz','aBzA','\x7f\x80\xff'):
                add(tail=2,flags=0x60,message_override=override,show_letters=letters,labels=labels)
    # floor(+0.5)·분수 경계·음수 좌표와 표의 열 우선/독립 clamp/행 순서를 관찰한다.
    for x in (-2.75,-.5,-.25,0,.25,.4999999701976776,.5,.75,20.25):
        for y in (-1.75,.25,.5,.75):
            for grid in ('none','sparse','all'): add(tail=2,legal=0,grid_option=1,world_x=fbits(x),world_y=fbits(y),grid=grid)
    for fx,fy in ((0,0),(1,1),(5,3),(10,10),(-1,-1)):
        add(tail=2,legal=0,grid_option=1,foot_x=fx,foot_y=fy,grid='all')
    # 월드와 화면 입력을 함께 음수로 옮겨 floor 차이가 독립 clamp 뒤에도 관찰되게 한다.
    for world,screen in ((-2.75,-43),(-.5,-7),(-.25,-3),(.25,4),(.75,12),(20.25,324)):
        for grid in ('none','sparse','all'):
            add(tail=2,legal=0,grid_option=1,world_x=fbits(world),screen_x=screen,world_y=fbits(17.75),screen_y=195,grid=grid)
    for value in (-32768,-101,-1,0,1,101,32767):
        add(tail=2,flags=0x40,screen_x=value,screen_y=-value if value!=-32768 else 32767,label_width=value,price_width=value)
    rng=random.Random(SEED)
    # 고정 시드 장면은 현재 좌표/문구 크기/색/깃발의 혼합을 넓힌다.
    for _ in range(90):
        add(tail=rng.randrange(8),parent=rng.randrange(2),legal=rng.randrange(2),owner=rng.randrange(-1,10),
            show_letters=rng.randrange(2),grid_option=rng.randrange(2),flags=rng.choice((0,32,64,96)),
            powered=rng.randrange(2),blocked=rng.randrange(2),reachable=rng.randrange(2),message_override=rng.randrange(2),
            foot_x=rng.randrange(0,6),foot_y=rng.randrange(0,6),hot_x=rng.randrange(-20,21),hot_y=rng.randrange(-20,21),
            camera_x=rng.randrange(-500,501),camera_y=rng.randrange(-500,501),spacing=rng.randrange(-10,20),
            world_x=fbits(rng.uniform(-20,20)),world_y=fbits(rng.uniform(-20,20)),grid=rng.choice(('none','sparse','all')))
    return cases

def encoded(case):
    """호스트 재생용 상태 DWORD/글자/격자만 직렬화한다."""
    return ','.join(str(case[f]&0xffffffff) for f in FIELDS),case['labels'],case['grid']

class PreviewDrawOracle(ProcessOracle):
    """실제 Draw/타입/좌표/CRT와 명시 출력 경계의 호출 인수·상태 보존을 감사한다."""
    def __init__(self,edition):
        """불연속 허용 함수와 출력 전용 경계를 준비한다."""
        super().__init__(edition);self.d=SPECS[edition];self.extra=[ROOT/f'extracted/previewdraw/{edition}/{n}' for n in ('creation.c','functions.tsv')]
        self.allowed=[];self.entry_names={};self.returns=collections.Counter();self.boundaries=collections.Counter()
        with self.extra[1].open(encoding='utf-8') as fp:
            # 내보낸 구간 사이의 코드를 실행하지 않는다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entry_names[int(row['entry'],16)]=row['entry']
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.mu.mem_map(DATA,0x40000)

    def on_instruction(self,mu,address,size,data):
        """출력/번역 경계는 호출 인수만 기록하며 기하나 분기 결과를 대신 만들지 않는다."""
        d=self.d;esp=mu.reg_read(UC_X86_REG_ESP)
        if address in d['boundary']:
            name=d['boundary'][address];self.boundaries[name]+=1;self.calls[name]+=1;result=0
            args=list(struct.unpack('<5I',mu.mem_read(esp+4,20)))
            # map은 팔레트 전역 선택, shape는 그 당시 원본이 결정한 좌표·frame·색 모드다.
            if name=='map':
                pointer=args[0];mode=d['colors'].index(pointer)+2 if pointer in d['colors'] else 1
                self.events.append(f'M,{mode}');self.color=mode
            elif name in ('plain','mapped','translated'):
                shape,frame,x,y=args[:4];typ=(shape-0x17000000)//0x1000
                mode=0 if name=='plain' else self.color if name=='mapped' else 3
                self.events.append(','.join(map(str,('S',typ,frame,x,y,mode))))
                if name=='translated' and args[4]!=d['colors'][1]: raise RuntimeError('Preview Draw 명시 변환 표 오류')
            elif name in ('outline','fill'): self.events.append(','.join(map(str,('R',int(name=='fill'),*args))))
            elif name=='message':
                index=(args[0]-STRINGS)//16;self.message=index;result=STRINGS+0x100+index*16
            elif name=='text':
                text,x,y=args[:3];index=self.message if text!=d['price'] else 5
                self.events.append(f'T,{index},{x},{y}')
                if args[4]!=0xffffffff: raise RuntimeError('Preview Draw 텍스트 길이 오류')
            mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,esp+4+(4 if name=='message' else 0));mu.reg_write(UC_X86_REG_EIP,self.u32(esp));return
        if not any(a<=address and address+size<=b for a,b in self.allowed): raise RuntimeError(f'Preview Draw 허용 밖 실행: {self.binary} {address:08x}')
        if address in self.entry_names: self.hits[self.entry_names[address]]+=1;self.entries[self.entry_names[address]]+=1

    def run(self,c,control):
        """완성 입력에서 원본 Draw 전체를 정상 반환시키고 전체 명령열과 입력 불변을 기록한다."""
        mu,d=self.mu,self.d;mu.mem_write(DATA,b'\0'*0x40000);self.events=[];self.color=0;self.message=-1;self.entries=collections.Counter();self.calls=collections.Counter()
        self.put(d['table'],TYPES)
        # raw 화면 좌표는 Pop에서 이미 만든 signed WORD이며 entry의 float 좌표는 쓰지 않는다.
        self.put(ENTRY+8,RAW);self.put(ENTRY+15,c['tail'],1);self.put(RAW+10,70,1)
        self.put(RAW+22,c['screen_x']&0xffff,2);self.put(RAW+24,c['screen_y']&0xffff,2)
        self.put(RAW+14,c['world_x']);self.put(RAW+18,c['world_y']);self.put(RAW+d['frame'],c['frame'],4 if self.patch else 1)
        # 실제 타입 포인터 조회와 보조 이미지 선택이 같은 전체 타입 표를 사용한다.
        for typ in range(70,74): self.put(TYPES+typ*d['type_stride']+0xdc,0x17000000+typ*0x1000)
        type_address=TYPES+70*d['type_stride'];self.put(type_address+0xec,c['type_flags'])
        for offset,key in ((d['foot'],'foot_x'),(d['foot']+4,'foot_y'),(d['hot'],'hot_x'),(d['hot']+4,'hot_y')): self.put(type_address+offset,c[key])
        for key,address in zip(FIELDS[:29],d['globals'],strict=True):
            if key=='has_owner_map': self.put(address+70*4,0x16038000 if c[key] else 0)
            elif key=='price_type': self.put(address,0x17000000+c[key]*0x1000)
            else: self.put(address,c[key])
        self.put(d['good'],231);self.put(d['bad'],189)
        labels=bytes.fromhex(c['labels']) if c['labels']!='-' else b'';mu.mem_write(d['labels'],labels+b'\0')
        # signed char 조회를 위한 앞 128칸까지 표 입력으로 준비한다.
        for letter in range(-128,128): self.put(d['label_frames']+letter*4,letter&0xffffffff)
        mu.mem_write(d['grid'],bytes.fromhex(c['grid']))
        for i in range(5): self.put(d['widths']+i*4,21+i*13);self.put(d['heights']+i*4,9+i*7);self.put(d['messages']+i*4,STRINGS+i*16)
        # 패치 CRT floor는 기본 x87 경로를 관찰한다. SSE 분기는 별도 범위다.
        if self.patch: self.put(0x5e5340,0)
        before=[(a,n,bytes(mu.mem_read(a,n))) for a,n in ((ENTRY,16),(RAW,d['stride']),(TYPES,0x25000),(d['grid'],144),(d['labels'],len(labels)+1))]
        self.writes=[(0,4),(STACK-0xf000,STACK+0x1000)]
        for reg,value in REGISTERS.items(): mu.reg_write(reg,value)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EDX,UC_X86_REG_ECX): mu.reg_write(reg,0)
        mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        self.put(0,0xffffffff);self.put(STACK,STOP);self.put(STACK+4,ENTRY)
        mu.emu_start(d['root'],STOP,timeout=10000000,count=1000000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4 or any(mu.reg_read(r)!=v for r,v in REGISTERS.items()): raise RuntimeError('Preview Draw ABI 정상 반환 오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800 or self.u32(0)!=0xffffffff: raise RuntimeError('Preview Draw x87/SEH 복구 오류')
        for a,n,b in before:
            if bytes(mu.mem_read(a,n))!=b: raise RuntimeError('Preview Draw 읽기 전용 입력 변경')
        self.returns[f'{d["root"]:08x}']+=1
        return [*encoded(c),';'.join(self.events) or '-',json.dumps(dict(self.entries),sort_keys=True,separators=(',',':')),json.dumps(dict(self.calls),sort_keys=True,separators=(',',':'))]

def generate(smoke):
    """두 x87 관찰이 같은 전체 계획만 저장한다. C++ 구현을 가져오거나 실행하지 않는다."""
    rows=[];editions={};files={ROOT/'tools/decomp_previewdraw_oracle.py',ROOT/'tools/decomp_process_oracle.py',ROOT/'tools/decomp_drawsort_oracle.py',ROOT/'tools/ghidra/previewdraw-functions.json',FIXTURE}
    for edition in BINARIES:
        oracle=PreviewDrawOracle(edition);indexed=list(enumerate(plan()))
        if smoke: indexed=indexed[::47]+indexed[-3:]
        for index,c in indexed:
            observed=[oracle.run(c,control) for control in CONTROLS]
            if observed[0]!=observed[1]: raise RuntimeError(f'Preview Draw x87 차이: {edition} {index}')
            rows.append([edition,str(index),*observed[0]])
        editions[edition]=dict(cases=len(indexed),entries=dict(oracle.hits),returns=dict(oracle.returns),boundaries=dict(oracle.boundaries))
        files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(edition,len(indexed),'통과',flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 미리보기 Draw 전체 관찰. 픽셀/글꼴/번역만 경계.\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-11',controls=list(CONTROLS),editions=editions,os_calls=0,assertions=0,
        boundaries=['outline','fill','plain','mapped','translated','map','message','text'],files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},
        limits=['전체 미리보기 Draw/타입 조회/좌표/교차/CRT 정상 반환','픽셀 합성·색 표 선택·번역·글꼴만 인수 관찰 경계',
            '준비한 화면 WORD·원본 격자 12×12·finite 좌표/x87 floor 경로, 실제 GUI/커서 입력은 후속']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')

def verify():
    """SHA·입력 계획·진입/정상 반환·출력 경계 합계를 완성 관찰에서 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[s.split('\t') for s in FIXTURE.read_text(encoding='utf-8').splitlines() if s and not s.startswith('#')]
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha: raise RuntimeError('Preview Draw SHA 오류: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls'] or report['assertions'] or set(report['editions'])!=set(BINARIES): raise RuntimeError('Preview Draw 총계 오류')
    for edition,item in report['editions'].items():
        chosen=[r for r in rows if r[0]==edition];entries=collections.Counter();boundaries=collections.Counter()
        if len(chosen)!=len(plan()) or item['cases']!=len(chosen): raise RuntimeError('Preview Draw 행 수 오류')
        for i,(r,c) in enumerate(zip(chosen,plan(),strict=True)):
            if len(r)!=8 or r[1]!=str(i) or tuple(r[2:5])!=encoded(c): raise RuntimeError('Preview Draw 입력 오류')
            counts=json.loads(r[6]);calls=json.loads(r[7])
            if counts.get(f'{SPECS[edition]["root"]:08x}')!=1 or set(calls)-set(report['boundaries']): raise RuntimeError('Preview Draw 관찰 경계 오류')
            for k,n in counts.items(): entries[k]+=n*2
            for k,n in calls.items(): boundaries[k]+=n*2
        if dict(entries)!=item['entries'] or dict(boundaries)!=item['boundaries'] or item['returns']!={f'{SPECS[edition]["root"]:08x}':len(chosen)*2}: raise RuntimeError('Preview Draw 집계 오류')
    print('previewdraw 검증 통과:',len(rows),'개, 두 x87 정상 반환',len(rows)*2,'회')

def main():
    """전체 생성·무저장 표본·완성 기록 감사를 선택한다."""
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--smoke',action='store_true');p.add_argument('--verify',action='store_true');args=p.parse_args()
    verify() if args.verify else generate(args.smoke)

if __name__=='__main__': main()
