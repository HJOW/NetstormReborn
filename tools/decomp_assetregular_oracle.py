#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 거주 구역·간헐천·신전 Handle/공통 피해 사건/실제 finder·Advance·난수를 격리 실행한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_rawfinder_oracle import FinderOracle, ROOT, POOL, HEAP, MAP, STACK_BASE, VTABLE, bits, sha
from decomp_bridgedecay_oracle import TYPES, CODES, STACK, STOP, FPU_CONTROLS
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 실제 Handle·외부 몸체·현재 모드/목록 전역 주소다. 합성 표에서 같은 인자를 사용하는 효과 경계만 대체한다.
SPEC = {
 'originals':dict(entries=(0x49a4e0,0x485640,0x4dc220,0x44b900), tables=(0x50f540,0x50da88,0x5196f0),
    frame=0x4acee0,advance=0x4afc90,notify=0x4214a0,surface=0x40eaf0,owner=0x4adf00,residence2=0x450a90,
    ascend=0x4db8e0,playername=0x484480,notice=0x4c2630,islandcount=0x46f480,island=0x46f530,
    packet=0x40aa20,stamp=0x4e2e30,request=0x4dc050,region=0x46f650,remove=0x452fe0,
    ground=0x470ac0,attached=0x4b1a90,ghost=0x443fe0,theme=0x4544a0,
    random=0x532710,fort=0x594fb8,boss=0x540bc4,suppress=0x594fa0,checking=0x5e4794,
    decoration=0x5411f0,maxscan=0x59a9e4,temples=0x5e189c,generation=0x54db4c,animation=0x542744,
    damageevent=0x5326c0,ghostevent=0x5326c4,ghosttype=0x5410e8),
 'originalCD':dict(entries=(0x44a710,0x416070,0x44efd0,0x461a40), tables=(0x501b90,0x500760,0x501d28),
    frame=0x4acbc0,advance=0x4acce0,notify=0x448c10,surface=None,owner=0x4aefa0,residence2=0x4e8cb0,
    ascend=0x44ee30,playername=0x401920,notice=0x48ca70,islandcount=0x4bdba0,island=0x4bdc50,
    packet=0x40a890,stamp=0x4c0420,request=0x44ea00,region=0x4bdcf0,remove=0x4a6aa0,
    ground=0x455b40,attached=0x4eb310,ghost=0x4d06a0,theme=None,
    random=0x5308ec,fort=0x540a1c,boss=0x540a2c,suppress=0x52e16c,checking=0x531454,
    decoration=0x51cadc,maxscan=0x51f12c,temples=0x565af0,generation=0x52e8f0,animation=0x5128cc,
    damageevent=0x520fdc,ghostevent=0x520fe0,ghosttype=0x51c9d8)
}
# 합성 caller는 원본 x87 반환값을 float로 저장할 뿐 기대값을 계산하지 않는다.
TEMP, CALLER, UPDATE, DAMAGE = HEAP+0xf000, STOP+0x300, STOP+0x320, STOP+0x330
FIXTURE=ROOT/'cpppj/tests/fixtures/assetregular-x86.tsv'
REPORT=ROOT/'cpppj/assetregular.json'
KEYS=('kind','event','count','payload','word','frame','cost','rng','fort','boss','suppress','checking',
      'owner','theme','element','support','maxscan','scene','ascend_active','dead','hit','genus','extra','attached','mutation')


def inputs():
    """분기/경계 입력만 만들며 반환값·프레임·사건의 기대값은 계산하지 않는다."""
    result=[]
    def case(**values):
        base=dict(kind=0,event=0,count=0,payload=bits(0.08),word=0,frame=0,cost=bits(2000),rng=0,
            fort=0,boss=1,suppress=0,checking=0,owner=1,theme=1,element=2,support=1,maxscan=0,
            scene=0,ascend_active=1,dead=0,hit=0,genus=0,extra=0,attached=0,mutation=0)
        base.update(values);result.append(tuple(base[name] for name in KEYS))
    # 주민 low nibble·signed DWORD frame/CD BYTE·공유 난수 상태를 교차한다.
    for word,frame,rng in itertools.product((0,1,15,0xabc0,0xabcf),(0,1,2,47,255,0xffffffff,0x80000000),(0,1,0xffffffff,0x12345678)):
        case(word=word,frame=frame,rng=rng)
    # 실제 주변 finder의 공집합/first match/뒤 후보·코드 플래그와 최대 조사 개수다.
    for scene,rng,maxscan in itertools.product(range(5),(0,1,0x12345678),(0,3,100)):
        case(event=1,scene=scene,rng=rng,maxscan=maxscan)
    # 현재 표면 owner/첫 같은 owner 신전·theammode·원소의 실제 프레임 선택이다.
    for word,element,support,theme in itertools.product((0,1,15),(0,1,2,3),(0,1,2),(0,1)):
        case(event=3,word=word,element=element,support=support,theme=theme,frame=17)
    case(event=2,payload=bits(-1))
    case(event=99,payload=0x7fc12345)
    # 결정량 비율의 두 임계점/동일점/완전 충전·빈/음수/zero 비용·각 frame reset 경계다.
    for word,frame,cost in itertools.product((0,999,1000,1001,1499,1500,1501,1999,2000,2001,65535),
        (0,15,16,17,31,32,33,47,48,255,0xffffffff),(bits(2000),bits(-2000),bits(0),0x7fc12345)):
        case(kind=1,word=word,frame=frame,cost=cost)
    for event in (1,99):case(kind=1,event=event,payload=bits(9.25))
    # 신전 Ascend 정리·편집 역방향·count31 정지·난수 연속 프레임·모든 이탈/통지 게이트다.
    for ascend_active,payload in itertools.product((0,1),(bits(0.08),bits(-1),0x7fc12345)):
        case(kind=2,event=0,ascend_active=ascend_active,payload=payload)
    for fort,frame in itertools.product((0,1),(0,1,7,8,15,16,47)):
        case(kind=2,event=1,fort=fort,frame=frame)
    for count,frame,rng in itertools.product((0,1,30,31,32,63,0xffffffff),(0,1,7,15),(0,1,0xffffffff)):
        case(kind=2,event=2,count=count,frame=frame,rng=rng)
    for count,word,boss,suppress,dead in itertools.product((0,1,2,3,22,23,0xffffffff),(0,2),(0,1),(0,1),(0,1)):
        case(kind=2,event=3,count=count,word=word,boss=boss,suppress=suppress,dead=dead)
    for count,checking in itertools.product((0,1,4,5,0x7fffffff,0x80000000,0xffffffff),(0,1)):
        case(kind=2,event=4,count=count,checking=checking)
    # Damageable 원본 분배는 간헐천과 신전 default 경유 양쪽에서 실행한다.
    for kind,hit,genus,count in itertools.product((1,2,3),(0,1),(0,0x200000),(0,0xffff,0x80000005,0x12345678)):
        case(kind=kind,event=6699,hit=hit,genus=genus,count=count)
    for kind,extra,attached in itertools.product((1,2,3),(0,1,8,9),(0,1)):
        case(kind=kind,event=6700,extra=extra,attached=attached)
    # 외부 가상 변경 뒤 현재 frame/type/owner/죽음 상태를 다시 읽는 대표 입력이다.
    for kind,event,mutation in ((0,0,1),(0,3,2),(1,0,3),(2,2,4),(2,3,5),(2,4,6),(3,6699,7)):
        case(kind=kind,event=event,mutation=mutation,count=5 if event in (3,4) else 0)
    return list(dict.fromkeys(result))


class AssetRegularOracle(FinderOracle):
    """actual finder/Handle/Advance/난수는 실행하고 선언한 표시·Ascend·이탈·피해 하위 효과만 관찰한다."""
    def __init__(self,edition):
        """최신 정밀 함수와 기존 독립 진행 함수의 범위를 추가한다."""
        super().__init__(edition);self.p=SPEC[edition];self.entries=set();self.returns=0
        for name in ('assetregular','frameadvance'):
            paths=[ROOT/f'extracted/{name}/{edition}/{suffix}' for suffix in ('creation.c','functions.tsv')]
            self.body_paths.extend(paths)
            with paths[1].open(encoding='utf-8') as stream:
                # Ghidra의 불연속 몸체 범위만 실행 허용한다.
                for row in csv.DictReader(stream,delimiter='\t'):
                    self.entries.add(int(row['entry'],16))
                    for span in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in span.split('-'))
                        self.allowed[low-self.base:high+1-self.base]=b'\x01'*(high+1-low)
        # theammode의 원소→프레임 분기는 현재 호스트의 정밀 범위에서 실제 몸체로 실행한다.
        if edition=='originals':
            path=ROOT/'extracted/refined/originals/functions.tsv';self.body_paths.append(path)
            with path.open(encoding='utf-8') as stream:
                rows=[row for row in csv.DictReader(stream,delimiter='\t') if int(row['entry'],16)==self.p['theme']]
            if len(rows)!=1:raise RuntimeError('최신 거주 구역 테마 helper 범위 누락')
            self.entries.add(self.p['theme'])
            for span in rows[0]['ranges'].split(';'):
                low,high=(int(value,16) for value in span.split('-'))
                self.allowed[low-self.base:high+1-self.base]=b'\x01'*(high+1-low)
        for table,entry in zip(self.p['tables'],self.p['entries']):
            if self.read(table+0x5c)!=entry:raise RuntimeError('Handle 실제 가상 표 불일치')
        self.stubs={}
        self.mu.mem_write(CALLER,b'\xd9\x1d'+struct.pack('<I',TEMP)+b'\xe9'+struct.pack('<i',STOP-(CALLER+11)))
        self.mu.mem_write(VTABLE+0x88,struct.pack('<I',UPDATE));self.mu.mem_write(VTABLE+0x98,struct.pack('<I',DAMAGE))
        self.mu.mem_write(VTABLE+0x74,struct.pack('<I',self.p['owner']))

    def read(self,address,width=4):
        """분석 raw 메모리를 지정 폭으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self,address,value,width=4):
        """입력/명시 경계의 실제 폭만 저장한다."""
        self.mu.mem_write(address,int(value).to_bytes(width,'little'))

    def frame(self,sid):
        """현재 판본의 frame을 읽는다."""
        return self.read(self.slot(sid)+self.o['frame'],self.o['frame_size'])

    def update(self,sid,flags):
        """표시 갱신 시점의 현재 프레임을 관찰한다."""
        self.events.append(f'U:{sid}:{flags}:{self.frame(sid)}')

    def on_write(self,mu,access,address,size,value,data):
        """실제 Handle의 쓰기를 스택/FS·대상 상태/프레임·난수/최대 조사 전역으로 제한한다."""
        if STACK_BASE<=address and address+size<=STACK_BASE+0x40000:return
        if address==0 and size==4:return
        if TEMP<=address and address+size<=TEMP+4:return
        if self.p['random']<=address and address+size<=self.p['random']+4:return
        if self.p['maxscan']<=address and address+size<=self.p['maxscan']+4:return
        if POOL<=address and address+size<=POOL+128*self.stride:
            field=(address-POOL)%self.stride
            if (12<=field and field+size<=14) or (self.o['frame']<=field and field+size<=self.o['frame']+self.o['frame_size']):return
        raise RuntimeError(f'Handle 허용 밖 쓰기 {self.edition} {address:08x}+{size}')

    def on_instruction(self,mu,address,size,data):
        """실제 조회/난수/finder/Advance를 그대로 두고 외부 효과만 ABI대로 돌려준다."""
        if address in (CALLER,CALLER+6):return
        if address in self.entries:self.native_calls[f'{address:08x}']+=1
        p=self.p;boundary={p[name] for name in ('frame','notify','surface','owner','residence2','ascend','playername','notice',
            'islandcount','island','packet','stamp','request','region','remove','ground','attached','ghost')}|{UPDATE,DAMAGE,0x441270 if self.stride==50 else None}
        if address not in boundary:return super().on_instruction(mu,address,size,data)
        sp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(sp);this=mu.reg_read(UC_X86_REG_ECX)
        args=[self.read(sp+4+4*i) for i in range(6)];purge=0;value=0;c=self.case
        if address==p['frame']:
            sid=self.sid_of(this);self.update(sid,args[1]);self.put(this+self.o['frame'],args[0]&((1<<(8*self.o['frame_size']))-1),self.o['frame_size'])
            self.update(sid,args[1]);purge=8
            if c['mutation']==4:self.put(self.slot(50)+self.o['frame'],7,self.o['frame_size'])
        elif address==UPDATE:
            self.update(self.sid_of(this),0)
            if c['mutation']==3:self.put(this+12,1000,2)
        elif address==p['notify']:
            self.events.append(f'N:{args[0]}')
            if c['mutation']==1:self.put(self.slot(50)+self.o['frame'],23,self.o['frame_size'])
        elif address==p['surface']:value=60 if c['support'] else 0
        elif address==p['owner']:
            self.events.append(f'O:{self.sid_of(this)}:{args[0]}');purge=4;self.put(this+(34 if self.stride==50 else 32),args[0]&255,1)
            if c['mutation']==2:self.put(this+42,7)
        elif address==p['residence2']:self.events.append(f'E:{args[0]}')
        elif address==p['ascend']:self.events.append(f'P:{self.sid_of(this)}:{args[0]}');purge=4;value=c['ascend_active']
        elif address==p['playername']:self.events.append(f'L:{args[1]}')
        elif address==p['notice']:pass
        elif address==p['islandcount']:self.events.append(f'R:{args[0]}');purge=4
        elif address==p['island']:self.events.append('I');purge=8;value=9
        elif address in (p['packet'],p['stamp']):
            pass
        elif address==p['request']:
            packet=bytes(self.mu.mem_read(args[2],8));self.events.append(f'Q:{args[0]}:{args[1]}:{packet[4]}:{int.from_bytes(packet[5:7],"little")}:{packet[7]}')
            if c['dead'] or c['mutation']==5:self.put(self.slot(50)+11,1,1)
        elif address==p['region']:self.events.append('Z');purge=8;value=11
        elif address==p['remove']:
            self.events.append(f'A:{args[0]}:{args[1]}:{args[2]}')
            if c['mutation']==6:self.put(self.slot(50)+12,99,2)
        elif address==p['ground']:self.events.append(f'G:{args[0]}:{args[1]}')
        elif address==DAMAGE:
            self.events.append(f'H:{self.sid_of(this)}:{args[0]}:{args[1]}:{args[2]}');purge=12;value=c['hit']
            if c['mutation']==7:self.put(TYPES+self.read(this+10,1)*self.type_stride+0xec,0x200000)
        elif address==p['attached']:self.events.append(f'F:{args[0]}:{args[1]}:{args[2]}:{args[3]}');value=c['attached']
        elif address==p['ghost']:self.events.append(f'X:{args[0]}:{args[1]}')
        elif address==0x441270:
            self.put(args[1],c['theme']);value=1
        self.stub_calls[f'{address:08x}']+=1
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,values,control):
        """현재 입력에서 원본 정상 반환·x87·raw·난수·최대 탐색·모든 효과 순서를 수집한다."""
        self.case=dict(zip(KEYS,values));c=self.case;p=self.p;patch=self.stride==50;owner=34 if patch else 32
        self.events=[]
        x,y=(50.75,52.5) if c['scene']!=4 else (5.25,4.75)
        objects=[(50,89,0,c['extra'],bits(x),bits(y),1,1,0,1)]
        # 전체 둘레에서 실제 finder가 반환할 후보가 존재하게 입력 버킷을 준비한다. 판단은 원본만 한다.
        for sid,pos,number in ((60,49.75,91),(61,50.25,91),(62,51.75,90)):
            objects.append((sid,number,0,8 if c['scene']==3 else 0,bits(pos),bits(50.25),1,1,0,1))
        self.setup(list(reversed(objects)))
        number=89 if c['kind']==0 else 122 if c['kind']==1 else 124
        self.put(self.slot(50)+10,number,1);self.put(self.slot(50)+owner,c['owner'],1)
        self.put(self.slot(50)+12,c['word'],2);self.put(self.slot(50)+self.o['frame'],c['frame']&((1<<(8*self.o['frame_size']))-1),self.o['frame_size'])
        self.put(self.slot(50)+(33 if patch else 31),1,1)
        if patch:self.put(self.slot(50)+42,5)
        self.put(self.slot(60)+owner,c['support'],1);self.put(self.slot(62)+owner,c['owner'],1)
        for typ in (89,90,91,122,124):
            base=TYPES+typ*self.type_stride;self.put(base+0xc4,c['cost']);self.put(base+0xec,c['genus']);self.put(base+0x98,c['element'])
            codes=CODES+typ*256;pattern='A'*8+'B'*8+'C'*8+'D'*8+'E'*8+'F'*8
            flags=8 if c['scene']==1 else 16 if c['scene']==2 else 0
            self.mu.mem_write(codes,b''.join(bytes((ord(letter),65,index,flags)) for index,letter in enumerate(pattern)))
            self.put(base+0x124,codes)
            self.mu.mem_write(base+0x130,struct.pack('<16i',0,8,16,24,32,40,*([-1]*10)))
            self.mu.mem_write(base+0x170,struct.pack('<16i',8,8,8,8,8,8,*([0]*10)))
        # 초기 프레임 밖 입력은 패치 간헐천의 직접 쓰기/거주 구역 event0에만 도달하며 native 코드 조회에 쓰지 않는다.
        self.put(p['random'],c['rng']);self.put(p['fort'],c['fort']);self.put(p['boss'],c['boss']);self.put(p['suppress'],c['suppress']);self.put(p['checking'],c['checking'])
        self.put(p['decoration'],91);self.put(p['maxscan'],c['maxscan']);self.put(p['generation'],3);self.put(p['animation'],17)
        self.put(p['damageevent'],6699);self.put(p['ghostevent'],6700);self.put(p['ghosttype'],30)
        self.mu.mem_write(p['temples'],struct.pack('<III',HEAP+0xd000,2,1));self.put(HEAP+0xd000,62)
        if not patch:self.put(0x546778,0)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0)
        self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,self.slot(50))
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.mem_write(STACK,struct.pack('<IIII',CALLER,c['event'],c['count'],c['payload']))
        self.mu.emu_start(p['entries'][c['kind']],STOP,count=1000000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+16:raise RuntimeError('Handle 반환/스택 오류')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('Handle x87 복구 오류')
        self.returns+=1
        output=bytearray(self.mu.mem_read(self.slot(50),self.stride))
        struct.pack_into('<I',output,0,p['tables'][min(c['kind'],2)])
        return [self.read(TEMP),self.read(p['random']),self.read(p['maxscan']),';'.join(self.events) or '-',output.hex(),
            ','.join(str(self.frame(sid)) for sid in (60,61,62))]


def generate(smoke=False):
    """고유 입력을 두 정밀도에서 실행하고 실제 PE 출처·정상 반환·명시 외부 경계를 기록한다."""
    cases=inputs();cases=cases[::101] if smoke else cases;rows=[];editions={}
    files=[Path(__file__),ROOT/'tools/ghidra/assetregular-functions.json',FIXTURE,
        ROOT/'cpppj/src/o/RawAssetRegular.h',ROOT/'cpppj/src/o/RawAssetRegular.cpp',ROOT/'cpppj/tests/AssetRegularTests.cpp']
    for edition in SPEC:
        oracle=AssetRegularOracle(edition)
        for values in cases:
            if edition=='originalCD' and values[0]==1 and values[1]==0 and values[5]&255>=48:continue
            output=oracle.run(values,FPU_CONTROLS[0]);check=oracle.run(values,FPU_CONTROLS[1])
            if output!=check:raise RuntimeError(f'Handle 정밀도 차이 {edition} {values}')
            rows.append([edition,','.join(map(str,values)),*output])
        count=sum(row[0]==edition for row in rows)
        editions[edition]=dict(cases=count,returns=oracle.returns,assertions=oracle.assertions,
            entries=dict(oracle.native_calls),external=dict(oracle.stub_calls))
        files.extend(oracle.body_paths+[ROOT/oracle.spec['binary']])
        print(f'{edition}: {count}개 정상 반환',flush=True)
    if smoke:return
    FIXTURE.write_text('# edition '+','.join(KEYS)+' return rng maxscan events raw neighbors\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files += [ROOT/'tools/decomp_rawfinder_oracle.py',ROOT/'tools/decomp_bridgeeffects_oracle.py',ROOT/'tools/decomp_bridgedecay_oracle.py']
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',editions=editions,os_calls=0,
        files={path.relative_to(ROOT).as_posix():sha(path) for path in dict.fromkeys(files)},limits=[
            'Handle/Damageable/finder/FrameAdvance/raw getter/공유 난수는 실제 PE 기계어다.',
            '표시 갱신/프레임 지정/소유자/Ascend/event2/다른 소유자 섬 조회/송신/애니메이션/피해/건설 시작은 명시 관찰 경계다.',
            'C++은 실제 Frame/Finder/Advance를 같은 풀에서 사용한다. 하위 효과를 미연결이면 거부한다.',
            '초기 postPop 최초 시각 예약은 기존 ProcessHost 몸체이며 이번 원본 관찰로 다시 합산하지 않는다.',
            '미션 초기 월드/실제 Ascend·생산/최종 게임 완주 또는 GUI 동등성을 입증하지 않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'total':len(rows),'editions':{key:value['cases'] for key,value in editions.items()}}))


def verify():
    """현재 SHA·고유 입력·명시 외부 효과와 실제 두 정밀도 정상 반환을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in report['files'].items():
        if sha(ROOT/name)!=value:raise RuntimeError('SHA 불일치: '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows)!=report['total'] or report['os_calls']!=0:raise RuntimeError('관찰 수/OS 경계 불일치')
    for edition,record in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition]
        if len(chosen)!=record['cases'] or record['returns']!=len(chosen)*2 or record['assertions']:raise RuntimeError('정상 반환/원본 assert 실패')
        if len({row[1] for row in chosen})!=len(chosen):raise RuntimeError('고유 입력 중복')
    print(f'자산 Handle {len(rows)}개 SHA/반환/입력 감사 통과')


# 명시 명령에서만 보호 PE를 읽어 새 fixture를 생성하거나 기존 근거를 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate(args.smoke)
