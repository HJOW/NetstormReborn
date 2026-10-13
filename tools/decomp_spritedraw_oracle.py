#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""일반 sprite Draw 전체·선택/막대 보조를 세 실제 PE에서 격리 실행한다. VFX/가상 메서드/게임 조회만 경계다."""
import argparse
import collections
import csv
import json
import random
import struct
from decomp_drawsort_oracle import REGISTERS
from decomp_process_oracle import (ProcessOracle,ROOT,BINARIES,CONTROLS,STACK,STOP,digest,fbits,
    UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 격리 entry/raw/type/지도/가상 처리기/프로세스와 SHP 헤더 메모리다.
DATA,ENTRY,RAW,TYPES,TOPO,VTABLE,PROCS,AUX,SHAPES=0x16000000,0x16000100,0x16001000,0x16010000,0x16030000,0x16070000,0x16071000,0x16073000,0x17000000
VFRAME,VDRAW=0x13000500,0x13000520
FIXTURE=ROOT/'cpppj/tests/fixtures/spritedraw-x86.tsv'
REPORT=ROOT/'cpppj/spritedraw.json'
SEED=0x49822012
# 입력은 기대 좌표/막대를 계산하지 않고 raw 필드·전역·타입·현재 조회 결과만 담는다.
FIELDS=('tail','extra','frame','world_x','world_y','camera_x','camera_y','hot_x','hot_y','foot_x','foot_y','loaded','frame_count',
    'owner','wide_owner','local_player','owner_map','special','filter','flags1','flags2','unfiltered','vframe','gump_frame',
    'marker_enabled','marker_visible','surface','surface_excluded','selection_disabled','width','height','shape_hot_x','shape_hot_y',
    'hp','max_hp','quarter_hp','alias_type','alias_pinned','construction_active','construction_time','construction_left',
    'resource','capacity','resource_left','timer_match','timer_present','cost_match','cost','quantity','progress')
SPECS={
 'originals':dict(root=0x498220,stride=50,type_stride=500,table=0x59ab20,frame=36,extra=40,owner=34,hot=0x1dc,foot=0x1d4,
    camera=(0x59a91c,0x59a920),mode=0x59a8ac,filter=0x5424b4,wide=0x5412e8,unfiltered=0x5411a8,player=0x540c70,
    owner_maps=0x557940,owner_palette=0x531b08,colors=(0x556238,0x556438,0x557738),
    marker=(0x5319c0,0x594fbc,0x5411d8,0x5412cc),status=0x541230,selection=0x594fc8,selected=0x5cae80,
    selection_colors=0x5319c4,hp_colors=(0x5b5dc4,0x59afd8,0x5b5e58),background=0x5a4058,quarter=0x5ca8f0,
    alias=0x5412d0,pinned=0x5318ec,pool=0x5c8464,topology=0x542514,timers=(0x541280,0x541298,0x54117c),timer_type=0x5410ec,
    cost=0x541240,abstract_status=0x54122c,status_process=0x541154,kernel=0x56b808,now=0x55b4d0,
    boundary={0x4daa70:'plain',0x4daab0:'mapped',0x4dab90:'translated',0x4dabe0:'map',0x4dade0:'line',0x4a2060:'fill',
        0x491af0:'construction',0x4afb40:'resource',0x4ac9f0:'content',0x4b1a90:'contained',0x4ae2d0:'timer',VFRAME:'vframe',VDRAW:'vdraw'}),
 'originalCD':dict(root=0x455f60,stride=36,type_stride=468,table=0x51c960,frame=34,extra=35,owner=32,hot=0x1bc,foot=0x1b4,
    camera=(0x565c34,0x565c38),mode=0x520398,filter=0x5395c8,wide=0x51cbd4,unfiltered=0x51ca94,player=0x50f6c8,
    owner_maps=0x5365c8,owner_palette=0x5365a0,colors=(0x56c7e8,0x56e0e8,0x56c8e8),
    marker=(0x536454,0x540a20,0x51cac4,0x51cbb8),status=0x51cb1c,selection=0x540a24,selected=0x512868,
    selection_colors=0x536458,hp_colors=(0x564bcc,0x55bf50,0x55bac8),background=0x55baa8,quarter=0x511c90,
    alias=0x51cbbc,pool=0x5395dc,topology=0x5670cc,
    boundary={0x4d64e0:'plain',0x4d6510:'mapped',0x4d65e0:'translated',0x4d6630:'map',0x4d6850:'line',0x423960:'fill',VFRAME:'vframe',VDRAW:'vdraw'})}
SPECS['original1037']=dict(SPECS['originalCD'])

def plan():
    """전체 Draw의 분기를 나누는 완성 입력을 고정 순서로 만든다."""
    cases=[]
    def add(**values):
        """일관된 world/screen 입력과 현재 타입·전역·조회 결과를 준비한다."""
        c=dict(tail=0,extra=0,frame=3,world_x=fbits(20.25),world_y=fbits(17.75),camera_x=13,camera_y=-7,
            hot_x=5,hot_y=-4,foot_x=5,foot_y=7,loaded=1,frame_count=32,owner=2,wide_owner=0,local_player=2,owner_map=1,
            special=0,filter=8,flags1=0,flags2=0,unfiltered=0,vframe=9,gump_frame=7,marker_enabled=0,marker_visible=1,
            surface=1,surface_excluded=0,selection_disabled=0,width=23,height=19,shape_hot_x=8,shape_hot_y=11,
            hp=75,max_hp=100,quarter_hp=0,alias_type=90,alias_pinned=0,construction_active=0,construction_time=40,construction_left=17,
            resource=0,capacity=12,resource_left=37,timer_match=0,timer_present=1,cost_match=0,cost=fbits(49.75),quantity=13,progress=0)
        c.update(values);cases.append(c)
    # 본체/그림자·가상/abstract/dead·특수 필터·소유자 표의 혼합을 전부 관찰한다.
    for tail in (0,4,5,255):
        for extra in (0,1,2,3,0x10,0x11,0x12,0x13,0x20,0x22):
            for special in (0,1):
                for owner in (0,2,3): add(tail=tail,extra=extra,special=special,owner=owner)
    for frame in (0,31,32,63,255,256,0xffffffff): add(tail=4,frame=frame)
    for custom in (-1,0,255,256):
        for extra in (2,3,0x12,0x13):
            for special in (0,1): add(extra=extra,vframe=custom,special=special)
    for wide in (0,1):
        for unfiltered in (0,1):
            for flags in (0,8,0xffffffff): add(wide_owner=wide,unfiltered=unfiltered,flags2=flags,special=1,owner=300 if wide else 2)
    for loaded in (0,1):
        for pinned in (0,1):
            for dead in (0,1): add(tail=4,loaded=loaded,alias_pinned=pinned,extra=dead,special=1)
    # marker 조회는 entry의 float를 사용하고 raw screen은 저장된 월드 투영을 사용한다.
    for enabled in (0,1):
        for visible in (0,1):
            for owner in (0,2):
                for surface in (0,1):
                    for excluded in (0,1): add(flags1=0x400,marker_enabled=enabled,marker_visible=visible,owner=owner,surface=surface,surface_excluded=excluded)
    for world in (-2.75,-.5,-.25,0,.25,.5,.75,20.25):
        add(world_x=fbits(world),world_y=fbits(world),extra=0x20,hot_x=-11,hot_y=8)
    for hp in (-1,0,1,24,25,26,49,50,51,99,100,101,65535,0x7fffffff):
        add(extra=4,flags1=0x10,hp=hp)
    for maximum in (-1,0,1,3,4,100): add(extra=4,flags1=0x10,max_hp=maximum,hp=0)
    for flags2 in (0,0x400000):
        for extra in (4,5):
            for disabled in (0,1): add(extra=extra,flags1=0x10,flags2=flags2,selection_disabled=disabled)
    # 판본별 signed 헤더와 음수 /4의 0방향 절삭·선 호출 순서다.
    for width,height in ((0,0),(1,1),(3,4),(4,5),(-5,-7),(23,19),(101,77)):
        add(extra=4,width=width,height=height)
    for quarter in (0,1):
        for active in (0,1): add(extra=4,flags1=0x10,alias_type=70,quarter_hp=quarter,construction_active=active)
    for timer in (0,1,2,3,4):
        for present in (0,1): add(extra=4,flags1=0x10,timer_match=timer,timer_present=present,resource=1,cost_match=1)
    # 각 상태 flag, first/last Kernel 슬롯, 중복 멤버, 음수/최댓값/flag 1 우선순위를 독립 관찰한다.
    for extra in (0x14,0x24,0x34):
        for progress in (0,1,2): add(extra=extra,flags1=0x10,progress=progress,resource=1)
    for frame in (0,31,32,63): add(extra=4,frame=frame,flags1=0x440000)
    rng=random.Random(SEED)
    # 선택 외 장면에 고정 시드로 현재 상태와 음수 카메라를 혼합한다.
    for _ in range(60): add(tail=rng.randrange(8),extra=rng.choice((0,1,2,3,0x10,0x12,0x20,0x22)),
        special=rng.randrange(2),owner=rng.randrange(4),owner_map=rng.randrange(2),local_player=rng.randrange(4),
        vframe=rng.choice((-1,0,8,255)),hot_x=rng.randrange(-30,31),hot_y=rng.randrange(-30,31),
        camera_x=rng.randrange(-500,501),camera_y=rng.randrange(-500,501),world_x=fbits(rng.uniform(-20,20)),world_y=fbits(rng.uniform(-20,20)))
    # 특수 필터가 꺼진 WORD 소유자를 색 표 선택에 노출해 BYTE 절삭과 구분한다.
    for owner in (256,300): add(wide_owner=1,owner=owner,special=0)
    return cases

def encoded(c):
    """현재 입력 DWORD만 fixture로 직렬화한다."""
    return ','.join(str(c[key]&0xffffffff) for key in FIELDS)

class SpriteDrawOracle(ProcessOracle):
    """일반 Draw/헤더/체력/좌표/막대의 실제 몸체와 원본 출력 인수를 감사한다."""
    def __init__(self,edition):
        """검토한 함수 범위와 출력/게임 조회 경계를 준비한다."""
        super().__init__(edition);self.d=SPECS[edition];self.extra=[ROOT/f'extracted/spritedraw/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entry_names={};self.returns=collections.Counter();self.boundaries=collections.Counter()
        with self.extra[1].open(encoding='utf-8') as fp:
            # Ghidra의 불연속 함수 몸체만 실행한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entry_names[int(row['entry'],16)]=row['entry']
                for part in row['ranges'].split(';'):
                    a,b=(int(v,16) for v in part.split('-'));self.allowed.append((a,b+1))
        self.mu.mem_map(DATA,0x90000);self.mu.mem_map(SHAPES,0x100000)

    def on_instruction(self,mu,address,size,data):
        """픽셀/가상 메서드와 현재 게임 조회만 대체하며 원본 Draw 분기·좌표를 계산하지 않는다."""
        d,c=self.d,self.case;esp=mu.reg_read(UC_X86_REG_ESP)
        if address in d['boundary']:
            name=d['boundary'][address];self.calls[name]+=1;self.boundaries[name]+=1
            args=struct.unpack('<5I',mu.mem_read(esp+4,20));result=0;purge=0
            if name=='map':
                owner=c['owner']& (0xffff if c['wide_owner'] else 255)
                if args[0]!=DATA+0x80000+(owner%3+1)*256: raise RuntimeError('Sprite Draw 소유자 색 표 주소 오류')
                self.events.append('M,70,'+str(owner))
            elif name in ('plain','mapped','translated'):
                shape,frame,x,y=args[:4];typ=(shape-SHAPES)//0x1000
                mode=0 if name=='plain' else 1 if name=='mapped' else d['colors'].index(args[4])+2
                self.events.append(','.join(map(str,('S',typ,frame,x,y,mode))))
            elif name in ('line','fill'): self.events.append(','.join(map(str,('L' if name=='line' else 'R',*args))))
            elif name=='vframe': result=c['vframe'];self.events.append('F,5');
            elif name=='vdraw': self.events.append(f'V,5,{args[0]},{args[1]}');purge=8
            elif name=='construction': result=c['construction_active']
            elif name=='resource':
                if args[0]!=0x493: raise RuntimeError('Sprite Draw 자원 mask 오류')
                result=AUX if c['resource'] else 0;purge=4
            elif name=='content':
                if args[:3]!=(1,0,0): raise RuntimeError('Sprite Draw 내용물 조회 오류')
                result=6;purge=12
            elif name=='contained':
                if args[:4]!=(5,40,0,0): raise RuntimeError('Sprite Draw 타이머 contained 조회 오류')
                result=7 if c['timer_present'] else 0
            elif name=='timer': result=AUX+0x230
            mu.reg_write(UC_X86_REG_EAX,result&0xffffffff);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,self.u32(esp));return
        if not any(a<=address and address+size<=b for a,b in self.allowed): raise RuntimeError(f'Sprite Draw 허용 밖 실행: {self.binary} {address:08x}')
        if address in self.entry_names: self.hits[self.entry_names[address]]+=1;self.entries[self.entry_names[address]]+=1

    def run(self,c,control):
        """완성 raw/전역/형상/프로세스 입력에서 원본 root를 정상 반환시킨다."""
        mu,d=self.mu,self.d;self.case=c;mu.mem_write(DATA,bytes(0x90000));mu.mem_write(SHAPES,bytes(0x100000));self.events=[];self.entries=collections.Counter();self.calls=collections.Counter()
        base=RAW-5*d['stride'];self.put(d['table'],TYPES);self.put(d['pool'],base);self.put(d['topology'],TOPO)
        self.put(ENTRY+8,RAW);self.put(ENTRY+15,c['tail'],1);self.put(ENTRY,c['world_x']);self.put(ENTRY+4,c['world_y'])
        self.put(RAW+10,70,1);self.put(RAW+14,c['world_x']);self.put(RAW+18,c['world_y'])
        # 화면 입력은 oracle 기대 출력이 아니라 실제 root의 assert를 만족하는 준비된 Pop 투영 값이다.
        wx,wy=(struct.unpack('<f',struct.pack('<I',c[key]))[0] for key in ('world_x','world_y'))
        self.put(RAW+22,int(wx*16+.5)&0xffff,2);self.put(RAW+24,int(wy*11+.5)&0xffff,2)
        self.put(RAW+d['frame'],c['frame'],4 if self.patch else 1);self.put(RAW+d['extra'],c['extra'],1)
        self.put(RAW+d['owner'],c['owner'],1);self.put(RAW+12,c['owner'] if c['wide_owner'] else c['quantity'],2);self.put(RAW+26,c['hp'],4 if self.patch else 2)
        self.put(RAW,VTABLE);self.put(VTABLE+0x78,VFRAME);self.put(VTABLE+0xa4,VDRAW)
        self.put(RAW+42,74);self.put(RAW+46,c['construction_left']);self.put(base+6*d['stride']+18,74,2);self.put(base+8*d['stride']+10,73 if c['surface_excluded'] else 72,1)
        # 두 판본이 직접 읽는 SHP 전 36바이트 선택 헤더와 실제 frame table 형식이다.
        for typ in range(70,91):
            t=TYPES+typ*d['type_stride'];shape=SHAPES+typ*0x1000
            self.put(t+0xdc,shape);self.put(t+0x114,c['frame_count']);self.put(t+0x118,c['gump_frame'])
            for frame in range(64):
                header=shape+0x400+frame*0x40;self.put(shape+8+frame*8,header-shape+0x24)
                for offset,key in ((0x18,'width'),(0x1a,'height'),(0x1c,'shape_hot_x'),(0x1e,'shape_hot_y')): self.put(header+offset,c[key],2)
        t=TYPES+70*d['type_stride'];self.put(t+0xdc,SHAPES+70*0x1000 if c['loaded'] else 0);self.put(t,c['max_hp']);self.put(t+0xe8,c['flags1']);self.put(t+0xec,c['flags2']);self.put(t+0xc4,c['cost'])
        for offset,key in ((d['hot'],'hot_x'),(d['hot']+4,'hot_y'),(d['foot'],'foot_x'),(d['foot']+4,'foot_y')): self.put(t+offset,c[key])
        for address,key in ((d['camera'][0],'camera_x'),(d['camera'][1],'camera_y'),(d['mode'],'special'),(d['filter'],'filter'),(d['player'],'local_player'),
            (d['marker'][0],'marker_enabled'),(d['marker'][1],'marker_visible'),(d['selection'],'selection_disabled'),(d['quarter'],'quarter_hp'),(d['alias'],'alias_type')): self.put(address,c[key])
        self.put(d['wide'],70 if c['wide_owner'] else 90);self.put(d['unfiltered'],70 if c['unfiltered'] else 90);self.put(d['marker'][2],71);self.put(d['marker'][3],73);self.put(d['status'],75)
        self.put(d['selected'],0);self.put(d['owner_maps']+70*4,DATA+0x80000 if c['owner_map'] else 0)
        for owner in range(4): self.put(d['owner_palette']+owner*4,owner%3+1)
        self.put(d['owner_palette']+c['owner']*4,c['owner']%3+1)
        # 실제 게임은 플레이어가 제한되지만 WORD 소유자 폭은 해당 표 원소만 별도 준비해 관찰한다.
        for i in range(4): self.put(d['selection_colors']+i*4,180+i)
        for i,address in enumerate(d['hp_colors']): self.put(address,100+i)
        self.put(d['background'],9)
        bias=struct.unpack('<f',mu.mem_read(0x500edc if self.patch else 0x502244,4))[0];tx,ty=int(wx+bias),int(wy+bias)
        if 0<=tx<256 and 0<=ty<256: self.put(TOPO+(ty*256+tx)*2,8 if c['surface'] else 0,2)
        if self.patch:
            self.put(d['pinned'],c['alias_pinned']);self.put(0x5e4794,0);self.put(d['timer_type'],40);self.put(d['cost'],70 if c['cost_match'] else 90)
            for i,address in enumerate(d['timers']): self.put(address,70 if c['timer_match'] in (i+1,4) else 90)
            self.put(d['abstract_status'],76);self.put(d['status_process'],41)
            self.put(TYPES+74*500+0x1b8,c['capacity']);self.put(TYPES+74*500+0x1bc,c['construction_time']);self.put(TYPES+75*500+0x1cc,30);self.put(TYPES+76*500+0x1cc,20)
            self.put(AUX+0x20,c['resource_left']);mu.mem_write(d['now'],struct.pack('<d',100.25));mu.mem_write(d['kernel'],bytes(40000*4))
            for offset,end in ((0x248,119.99),(0x268,160.99),(0x228,151.01)): mu.mem_write(AUX+offset,struct.pack('<d',end))
            if c['progress']:
                # 슬롯 0은 큰 값이어도 건너뛴다. 마지막 슬롯/종류/멤버/flag 우선순위 모두 실제 메모리다.
                for i,(slot,typ,flags,end,members) in enumerate(((0,41,1,999,[5]),(1,41,3,111.9,[6,5,5]),(2,41,2,122.8,[5]),(3,41,1,99,[5]),(4,42,1,900,[5]),(5,41,2,900,[6]),(39999,41,1,120.6 if c['progress']==2 else 104,[5]))):
                    p=PROCS+i*0x100;self.put(d['kernel']+slot*4,p);self.put(p+4,100+i);self.put(base+(100+i)*50+10,typ,1)
                    self.put(p+0xc,p+0x50);self.put(p+0x14,len(members));self.put(p+0x20,flags);mu.mem_write(p+0x18,struct.pack('<d',end))
                    for n,member in enumerate(members): self.put(p+0x50+n*4,member)
        # raw/표/헤더/지도/프로세스 입력을 보호하고 root의 alias/selected 쓰기만 따로 허용한다.
        alias=TYPES+c['alias_type']*d['type_stride']+0xdc
        self.writes=[(STACK-0xf000,STACK+0x1000),(0,4),(d['selected'],d['selected']+4)]
        if self.patch: self.writes.append((alias,alias+4))
        before=bytes(mu.mem_read(DATA,0x90000));headers=bytes(mu.mem_read(SHAPES,0x100000))
        for reg,value in REGISTERS.items(): mu.reg_write(reg,value)
        self.put(0,0xffffffff);self.put(STACK,STOP);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_ECX,ENTRY);mu.reg_write(UC_X86_REG_EFLAGS,2)
        mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);mu.emu_start(d['root'],STOP,timeout=30000000,count=2000000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4 or any(mu.reg_read(r)!=v for r,v in REGISTERS.items()): raise RuntimeError('Sprite Draw ABI 정상 반환 오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800 or self.u32(0)!=0xffffffff: raise RuntimeError('Sprite Draw x87/SEH 복구 오류')
        after=bytearray(mu.mem_read(DATA,0x90000));after[alias-DATA:alias-DATA+4]=before[alias-DATA:alias-DATA+4]
        if bytes(after)!=before or bytes(mu.mem_read(SHAPES,0x100000))!=headers: raise RuntimeError('Sprite Draw 읽기 전용 입력 변경')
        self.returns[f'{d["root"]:08x}']+=1
        return [encoded(c),';'.join(self.events) or '-',str(self.u32(d['selected'])),str(self.u32(alias)),json.dumps(dict(self.entries),sort_keys=True,separators=(',',':')),json.dumps(dict(self.calls),sort_keys=True,separators=(',',':'))]

def generate(smoke):
    """두 x87의 관찰이 같은 행만 저장하며 C++ 코드는 읽거나 실행하지 않는다."""
    rows=[];editions={};files={ROOT/'tools/decomp_spritedraw_oracle.py',ROOT/'tools/decomp_process_oracle.py',ROOT/'tools/decomp_drawsort_oracle.py',ROOT/'tools/ghidra/spritedraw-functions.json',FIXTURE}
    for edition in BINARIES:
        oracle=SpriteDrawOracle(edition);indexed=list(enumerate(plan()))
        if smoke: indexed=indexed[::53]+indexed[-70:-55]+indexed[-3:]
        for index,c in indexed:
            try: observations=[oracle.run(c,control) for control in CONTROLS]
            except Exception as error: raise RuntimeError(f'{edition} case {index}: {c}') from error
            if observations[0]!=observations[1]: raise RuntimeError(f'Sprite Draw x87 차이: {edition} {index}')
            rows.append([edition,str(index),*observations[0]])
        editions[edition]=dict(cases=len(indexed),entries=dict(oracle.hits),returns=dict(oracle.returns),boundaries=dict(oracle.boundaries));files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(edition,len(indexed),'통과',flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 일반 sprite Draw 전체 관찰. VFX/가상 처리기/게임 조회만 경계.\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-12',controls=list(CONTROLS),editions=editions,os_calls=0,assertions=0,
        boundaries=sorted(set().union(*(set(d['boundary'].values()) for d in SPECS.values()))),files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},
        limits=['전체 일반 Draw/타입/선택 헤더/HP/막대/지도/Kernel 스캔/CRT 정상 반환','VFX 픽셀·가상 frame/Draw·건설 활성/자원/내용물/타이머 조회만 명시 경계',
            '준비된 finite raw/전역/타입/프로세스 입력, 실제 GUI/게임 규칙 공급은 후속']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')

def verify():
    """SHA/전체 입력/명령열 행 수와 진입/정상 반환/경계 합계를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha: raise RuntimeError('Sprite Draw SHA 오류: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls'] or report['assertions'] or set(report['editions'])!=set(BINARIES): raise RuntimeError('Sprite Draw 총계 오류')
    for edition,item in report['editions'].items():
        chosen=[r for r in rows if r[0]==edition];entries=collections.Counter();boundaries=collections.Counter()
        if len(chosen)!=len(plan()) or item['cases']!=len(chosen): raise RuntimeError('Sprite Draw 행 수 오류')
        for i,(row,c) in enumerate(zip(chosen,plan(),strict=True)):
            if len(row)!=8 or row[1]!=str(i) or row[2]!=encoded(c): raise RuntimeError('Sprite Draw 입력 오류')
            hits=json.loads(row[6]);calls=json.loads(row[7])
            if hits.get(f'{SPECS[edition]["root"]:08x}')!=1 or set(calls)-set(report['boundaries']): raise RuntimeError('Sprite Draw 관찰 경계 오류')
            for k,n in hits.items(): entries[k]+=n*2
            for k,n in calls.items(): boundaries[k]+=n*2
        if dict(entries)!=item['entries'] or dict(boundaries)!=item['boundaries'] or item['returns']!={f'{SPECS[edition]["root"]:08x}':len(chosen)*2}: raise RuntimeError('Sprite Draw 집계 오류')
    print('spritedraw 검증 통과:',len(rows),'개, 두 x87 정상 반환',len(rows)*2,'회')

def main():
    """무저장 표본·전체 생성·완성 근거 감사를 선택한다."""
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--smoke',action='store_true');p.add_argument('--verify',action='store_true');args=p.parse_args()
    verify() if args.verify else generate(args.smoke)

if __name__=='__main__': main()
