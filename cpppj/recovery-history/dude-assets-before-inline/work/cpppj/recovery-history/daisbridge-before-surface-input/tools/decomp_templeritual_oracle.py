#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 신전 의식/목표점/사제·룬 조회를 보호PE의 제한x86에서 독립 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_rawfinder_oracle import FinderOracle,ROOT,POOL,HEAP,MAP,VTABLE,bits,sha
from decomp_bridgedecay_oracle import TYPES,CODES,SPOTS,CAPACITY,FPU_CONTROLS,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 실제 하위 요청만 격리 관찰한다. count/좌표/HP/일반finder는원본이다.
SPEC={
 'originals':dict(main=0x447e40,stage=0x447890,count=0x4478f0,anchor=0x447aa0,point=0x447b30,rune=0x447c00,strong=0x4479c0,abstract=0x447dc0,spawn=0x447ca0,frame=0x4acee0,new=0x4e4391,shared=0x496f00,create=0x4af530,sound=0x4a9d70,boss=0x540bc4,runeType=0x5412f0,priestType=0x5412d0,scaleX=0x59a92c,scaleY=0x59a930,quarter=0x5ca8f0),
 'originalCD':dict(main=0x481fc0,stage=0x4812a0,count=0x481310,anchor=0x481c80,point=0x481d10,rune=0x481de0,strong=0x4817e0,abstract=0x481ee0,spawn=0x481e50,frame=0x4acbc0,new=0x4f1650,shared=0x48f1d0,create=0x4ab390,sound=0x438c00,boss=0x540a2c,runeType=0x51cbdc,priestType=0x51cbbc,scaleX=0x565c44,scaleY=0x565c48,quarter=0x511c90),
}
SHP=0x11400000 # 합성SHP의원본프레임오프셋/핫스폿입력저장소다.
RITUAL_SPOTS=0x11600000 # 네 해시 배열과 겹치지 않는 실제 지면 입력의 전용 주소다.
OUT=HEAP+0x8000 # 원본포인터반환의좌표2개저장소다.
OWNER,POP=HEAP+0xf000,HEAP+0xf010 # 명시가상owner/Pop관찰진입점이다.
MEMORY=HEAP+0x6000 # 원본new0x28이반환하는격리메모리다.
FIXTURE=ROOT/'cpppj/tests/fixtures/templeritual-x86.tsv'
REPORT=ROOT/'cpppj/recovery-templeritual-evidence.json'


def inputs():
    """각단계/두count구간/현재방향/후속·권한/룬·사제후보/실패·효과변이입력만교차한다."""
    result=[]
    # 프레임은각count구간단계경계전후이며방향N/M/L과일반A의실제우선순위를분리한다.
    for index,row in enumerate(itertools.product((0,1),(3,13,14,24,25,35,36,46,47,57,58,64,119,120,164),('A','N','M','L'),range(7),(0,1),(0,1))):
        result.append((*row,index%4,index%3,(index//3)%4))
    return result


class TempleOracle(FinderOracle):
    """기존PE로더/일반finder만재사용하며새현재정밀범위와명시효과만허용한다."""
    def __init__(self,edition):
        """새상위/하위body와기존cleanup공통helper의동일출처를연결한다."""
        super().__init__(edition);self.patch=edition=='originals';self.p=SPEC[edition];self.stubs={};self.allowed[:]=bytes(len(self.allowed));self.entries={};self.paths=[];self.hits=collections.Counter();self.returns=0;self.mu.mem_map(SHP,0x20000);self.mu.mem_map(RITUAL_SPOTS,0x10000);self.put(self.g['spots'],RITUAL_SPOTS)
        # 기존완료helper출처는그대로읽으며원본실행허용에공통finder만추가한다.
        for name in ('templeritual','pathcleanup'):
            manifest=ROOT/f'tools/ghidra/{name}-functions.json';table=ROOT/f'extracted/{name}/{edition}/functions.tsv';source=table.with_name('creation.c');self.paths.extend((manifest,table,source));selected={int(e,16) for e in json.loads(manifest.read_text(encoding='utf-8'))['editions'][edition]}
            with table.open(encoding='utf-8') as stream:
                # 불연속실제함수범위외에는PE코드실행을허용하지않는다.
                for row in csv.DictReader(stream,delimiter='\t'):
                    if int(row['entry'],16) not in selected:continue
                    self.entries[int(row['entry'],16)]=row['entry']
                    for part in row['ranges'].split(';'):
                        low,high=(int(v,16) for v in part.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\1'*(high+1-low)
        self.spec=dict(self.spec,entries={name:self.p[name] for name in ('main','stage','count','anchor','point','rune','strong','abstract','spawn')});self.boundaries={self.p[name]:name for name in ('frame','new','shared','create','sound')};self.boundaries.update({OWNER:'owner',POP:'pop'})

    def read(self,address,width=4):
        """현재littleendian필드의명시폭만읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self,address,value,width=4):
        """명시입력과효과변이만원본폭대로쓴다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def on_write(self,mu,access,address,size,value,data):
        """상위원본의Rune8HP/프레임/추상bit쓰기만추가허용한다."""
        if self.slot(8)<=address and address+size<=self.slot(8)+self.stride:return
        return super().on_write(mu,access,address,size,value,data)

    def on_instruction(self,mu,address,size,data):
        """분기·단계·SHP·조회는원본이고외부수명/음향요청만현재인자로기록한다."""
        if address in getattr(self,'entries',{}):self.hits[self.entries[address]]+=1
        boundary=getattr(self,'boundaries',{}).get(address)
        if not boundary:return super().on_instruction(mu,address,size,data)
        self.stub_calls[boundary]+=1;sp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(sp);this=mu.reg_read(UC_X86_REG_ECX);a=[self.read(sp+4*i) for i in range(1,5)];purge=0;result=0
        if boundary=='frame':
            sid=self.sid_of(this);purge=8;self.events.append(f'F:{sid},{a[0]},{a[1]}');self.put(this+self.o['frame'],a[0],self.o['frame_size'])
            if self.case[-1]==1:self.put(self.p['boss'],1-self.read(self.p['boss']))
            if self.case[-1]==2:self.put(self.slot(5)+self.o['frame'],14,self.o['frame_size'])
        elif boundary=='new':
            assert a[0]==40;self.events.append('N:40');result=MEMORY if self.case[5] else 0
            if self.case[-1]==3:self.put(self.p['boss'],1-self.read(self.p['boss']))
        elif boundary=='shared':
            assert this==MEMORY;purge=12;result=MEMORY;self.events.append('S:'+','.join(map(str,a[:3])))
        elif boundary=='create':
            assert a[:2]==[166,0];self.events.append('C:166,0');result=self.slot(8);self.mu.mem_write(result,bytes([0xa5])*self.stride);self.put(result,VTABLE);self.put(result+10,166,1);self.put(result+11,4,1)
        elif boundary=='sound':
            end=bytes(mu.mem_read(a[2],64)).split(b'\0')[0].decode('ascii');self.events.append('A:'+str(a[0])+','+str(a[1])+','+end+','+str(a[3]))
        elif boundary=='owner':
            assert this==self.slot(8);purge=4;self.events.append('O:8,'+str(a[0]));self.put(this+(34 if self.patch else 32),a[0],1)
        elif boundary=='pop':
            assert this==self.slot(8);purge=12;self.events.append('P:8,'+','.join(map(str,a[:3])));self.put(this+14,a[0]);self.put(this+18,a[1]);self.put(this+11,self.read(this+11,1)&~4,1)
        mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def prepare(self,case,control):
        """원본목표점으로부모입력좌표를준비한뒤그좌표비트를fixture입력에직접기록한다."""
        self.case=case;mode,frame,side,profile,boss,reserve,spot,position,mutation=case;self.mu.mem_write(POOL,bytes(12*self.stride));self.mu.mem_write(TYPES,bytes(0x40000));self.mu.mem_write(CODES,bytes(0x20000));self.mu.mem_write(RITUAL_SPOTS,bytes([spot])*65536)
        self.setup([(5,165,0,0,bits(40.75),bits(41.25),3,3,0,1),(6,158,0,0,bits(40.75),bits(41.25),1,1,0,1)])
        # 타입/SHP/프레임은같은고정합성자산입력이며선택결과는모든경로에서원본이읽는다.
        for typ,count in ((165,180),(166,6),(158,32)):
            table=TYPES+typ*self.type_stride;self.put(table,200);self.put(table+0xe8,16);self.put(table+0xdc,SHP+(typ-158)*0x3000);self.put(table+0x114,count);self.put(table+0x124,CODES+(typ-158)*0x1000)
            for index in range(count):
                self.put(CODES+(typ-158)*0x1000+index*4,ord(side) if typ==165 else ord('A'),1);self.put(SHP+(typ-158)*0x3000+8+index*8,0x800+index*12)
                self.put(SHP+(typ-158)*0x3000+0x800+index*12-4,(index%5)*16,2);self.put(SHP+(typ-158)*0x3000+0x800+index*12-2,(index%5)*11,2)
            # 사제K..P현재first-table은기존RitualActions대조와같은입력이다.
            for stage in range(6):self.put(table+0x158+stage*4,2+stage)
        self.put(TYPES+158*self.type_stride+0xec,0x210000);self.put(self.slot(5)+(34 if self.patch else 32),1,1);self.put(self.slot(6)+(34 if self.patch else 32),1,1);self.put(self.slot(5)+self.o['frame'],frame,self.o['frame_size']);self.put(self.slot(6)+26,100,4 if self.patch else 2)
        self.put(VTABLE+0x80,self.p['count']);self.put(VTABLE+0x74,OWNER);self.put(VTABLE+0x90,POP)
        for name,value in [('boss',boss),('runeType',166),('priestType',158),('quarter',0),('scaleX',bits(16)),('scaleY',bits(11))]:self.put(self.p[name],value)
        if not self.patch:self.put(0x546778,0)
        stage=self.call('stage',[-1],self.slot(5),4,control);target_stage=min(stage,4) if position==0 else min(stage+1,4)
        self.call('point',[OUT,target_stage],self.slot(5),8,control);xy=bytes(self.mu.mem_read(OUT,8))
        if position==2:xy=struct.pack('<ff',80.75,81.25)
        # 후보유형은원본실제버킷에추가한다.강약/owner/룬매장분기를기대함수로계산하지않는다.
        x,y=struct.unpack('<ff',xy);objects=[(5,165,0,0,bits(40.75),bits(41.25),3,3,0,1),(6,158,0,0,bits(x),bits(y),1,1,0,1)]
        if profile in (3,4,5,6):
            rx,ry=(40.75,41.25) if profile==5 else (x-(min(stage,4)%5),y-(min(stage,4)%5));objects.append((7,166,2 if profile==4 else 0,8 if profile==6 else 1 if profile==5 else 0,bits(rx),bits(ry),1,1,0,1))
        self.setup(objects);self.put(self.slot(5)+self.o['frame'],frame,self.o['frame_size']);self.put(self.slot(5)+(34 if self.patch else 32),1,1);self.put(self.slot(6)+(34 if self.patch else 32),2 if profile==2 else 1,1);self.put(self.slot(6)+26,99 if profile==1 else 100,4 if self.patch else 2)
        return [*struct.unpack('<II',xy),bits(rx) if profile>=3 else 0,bits(ry) if profile>=3 else 0]

    def run(self,case,control):
        """좌표/조회 helper6개와전체Ritual출력/현재raw/권한을별도원본호출로읽는다."""
        xy=self.prepare(case,control);queries={};stage=self.call('stage',[-1],self.slot(5),4,control);queries['count']=self.call('count',[],self.slot(5),0,control);queries['stage']=stage
        self.call('anchor',[OUT,min(stage,4)],self.slot(5),8,control);queries['anchor']=list(struct.unpack('<II',self.mu.mem_read(OUT,8)));self.call('point',[OUT,min(stage,4)],self.slot(5),8,control);queries['point']=list(struct.unpack('<II',self.mu.mem_read(OUT,8)));queries['rune']=self.call('rune',xy[:2],self.slot(5),8,control);queries['strong']=self.call('strong',[],self.slot(5),0,control);queries['abstract']=self.call('abstract',[],self.slot(5),0,control)
        self.call('main',[6,case[0]],self.slot(5),8,control);self.returns+=1
        return dict(input=list(case),position=xy,queries=queries,effects=';'.join(self.events) or '-',raw=[bytes(self.mu.mem_read(self.slot(sid),self.stride)).hex() for sid in (5,6,7,8)],boss=self.read(self.p['boss']))


def generate(smoke=False):
    """두정밀도에서같은고유입력만저장하며신규조회/전체반환과실행범위SHA를남긴다."""
    cases=inputs();cases=cases[::227] if smoke else cases;rows=[];editions={};files={Path(__file__),FIXTURE}
    # 두보호PE에같은입력각각독립실행하고반복은새관찰에합산하지않는다.
    for edition in SPEC:
        oracle=TempleOracle(edition)
        for index,case in enumerate(cases):
            try:
                result=oracle.run(case,FPU_CONTROLS[0]);assert oracle.run(case,FPU_CONTROLS[1])==result
            except Exception as error:raise RuntimeError(f'{edition} #{index} {case}: {error}') from error
            rows.append(dict(edition=edition,**result))
        editions[edition]=dict(cases=len(cases),returns=oracle.returns,entries=dict(oracle.hits),boundaries=dict(oracle.stub_calls),asserts=oracle.assertions);files.update(oracle.paths);files.add(ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'))
    columns=[]
    # C++은기존탭fixture읽기와동일한명시입력/원본출력만사용한다.
    for row in rows:
        q=row['queries'];columns.append([row['edition'],','.join(map(str,row['input'])),','.join(map(str,row['position'])),','.join(map(str,[q['count'],q['stage'],*q['anchor'],*q['point'],q['rune'],q['strong'],q['abstract']])),row['effects'],';'.join(row['raw']),str(row['boss'])])
    FIXTURE.write_text('# edition mode,frame,side,profile,boss,reserve,spot,position,mutation priestXY,runeXY_bits queries effects slots5,6,7,8 boss\n'+'\n'.join('\t'.join(row) for row in columns)+'\n',encoding='utf-8',newline='\n');files.update(ROOT/name for name in ('cpppj/src/o/RawTempleRitual.h','cpppj/src/o/RawTempleRitual.cpp','cpppj/src/o/RawSquidHitPoints.h','cpppj/src/o/RawSquidHitPoints.cpp','cpppj/tests/TempleRitualTests.cpp','cpppj/recovery-squidhitpoints-lineage-evidence.json','tools/decomp_rawfinder_oracle.py','tools/decomp_bridgeeffects_oracle.py','tools/decomp_bridgedecay_oracle.py'))
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',total=len(rows),editions=editions,files={p.relative_to(ROOT).as_posix():sha(p) for p in sorted(files)},game_runs=0,smoke=smoke,limits=['실제Ritual/단계/count/SHP/좌표/사제·룬finder/공통HPsetter는원본몸체다.','frame setter/new/Shared/create/sound/가상owner/Pop은명시관찰경계다.','Rune 실제postPop304/건설시작,신전305/306/307전체consumer·GameWorld/UI는후속이다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('신전 의식 native 고유 입력:',len(rows))


def verify():
    """현재출처SHA와고유입력/전체원본반환/명시경계를감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==report['total'] and report['game_runs']==0
    for name,value in report['files'].items():assert sha(ROOT/name)==value,name
    for edition,item in report['editions'].items():assert item['returns']==2*item['cases'] and item['asserts']==0 and item['entries'][f'{SPEC[edition]["main"]:08x}']==item['returns']
    print('신전 의식 SHA/원본반환 감사:',len(rows))


# 명시호출에서만새격리관찰을생성하거나현재근거를감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
