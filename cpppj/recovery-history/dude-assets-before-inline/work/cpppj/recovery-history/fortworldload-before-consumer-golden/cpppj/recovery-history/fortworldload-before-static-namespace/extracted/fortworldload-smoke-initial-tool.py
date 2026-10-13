#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Fort 저장 부모·Territory 읽기와 기존 같은 지도 Builder의 전체 현재바이트를 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_islandbuilder_oracle import IslandBuilderOracle,CHUNK_BYTES,ISLAND_BYTES,CODES,block_rle
from decomp_owner_oracle import ROOT,TYPES,STOP,STACK,digest
from decomp_priestgeometry_oracle import GEOMETRY
from decomp_canontype_oracle import frame_codes
from decomp_damageablepredestroy_oracle import CONTROLS
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 포인터/스트림/원본120BYTE/단독getter출력을보관하는분석메모리다.
ARENA,MANAGER,STREAM,DATA,OUTPUT=0x11a00000,0x11a00100,0x11a00200,0x11a01000,0x11a02000
FIXTURE=ROOT/'cpppj/tests/fixtures/fortworldload-x86.tsv'
REPORT=ROOT/'cpppj/recovery-fortworldload-evidence.json'
# 부모/실제Territory getter와현재Template/원본전역·명시child주소다.
PLAN={
 'originals':dict(parent=0x4b5f70,territory=0x4bd660,lookup=0x44ceb0,generate=0x46da70,objects=0x4be610,
  money=0x4bf340,badges=0x4be8e0,technology=0x4beee0,populate=0x46e630,graph=0x463110,
  owner=0x540cac,templates=0x5c89f0,noRegion=0x50c038,depth=0x5c89b8,exists=0x46ee30,missing=0x22e),
 'originalCD':dict(parent=0x4330c0,territory=0x427c00,lookup=0x44c4e0,generate=0x490530,objects=0x428d50,
  money=0x4297f0,badges=0x428fc0,technology=0x4293d0,populate=0x4906c0,graph=0x45bd60,
  owner=0x50f6d4,templates=0x564bd0,noRegion=0x505eb0,depth=0x5178d4,exists=0x4bd4c0,missing=0x226)}
KEYS=('kind','alternate','owner','count','exists','profile','position','end','depth','mutation','template') # 고정입력순서이며기대결과를포함하지않는다.


def sample(**changes):
    """유효현재입력을준비하며owner/지도/호출순서의기대값을계산하지않는다."""
    result=dict(zip(KEYS,(0,0,1,0,0,1,0,120,0,0,1)));result.update(changes);return result


def inputs():
    """현재owner/활성Territory/기존island0/대체분기와후속효과뒤재조회를교차한다."""
    rows=[]
    # 두부모모드/원본localOwner/현재Exists와실제활성레코드를방문한다.
    for index,values in enumerate(itertools.product((0,1,0xffffffff),(0,1,8),(0,1),(0,1),(0,1,2))):
        alternate,owner,count,exists,profile=values
        rows.append(sample(alternate=alternate,owner=owner,count=count,exists=exists,profile=profile,position=(0,3,19)[index%3],
           end=(120,139,0x80000000)[index%3],depth=(0,0xffffffff)[index%2]))
    # 각child효과는다음child의현재owner/지역/Exists만명시변경한다.
    for alternate,mutation in itertools.product((0,1),range(1,10)):
        rows.append(sample(alternate=alternate,mutation=mutation,count=1,exists=0,depth=3,profile=1))
    # 직접Territory는현재pos를유지하고끝signed비교/즉시CD depth감김을실행한다.
    for position,end,depth,owner in itertools.product((0,3,19),(0,119,120,139,0x80000000),(0,1,0xffffffff),(0,1,8)):
        rows.append(sample(kind=1,position=position,end=end,depth=depth,owner=owner))
    # missingTemplate진단경계가pointer를복구한후에만원본복사가정상반환한다.
    for kind,owner in itertools.product((0,1),(0,1,8)):rows.append(sample(kind=kind,owner=owner,template=0))
    return list({tuple(row[key] for key in KEYS):row for row in rows}.values())


def territory(profile):
    """원본6BYTE기록의명시입력이며기존PlaceAll이선택여부와현재등록을결정한다."""
    data=bytearray(120)
    if profile:data[:6]=bytes((3,1,0x11,7,0xaa,0xbb))
    if profile==2:data[18:24]=bytes((3,1,0x55,31,0xcc,0xdd))
    return bytes(data)


class FortWorldOracle(IslandBuilderOracle):
    """기존완료Builder를실제재사용하고새부모/Territory만추가하며나머지child는명시경계다."""
    def __init__(self,edition):
        """기존현재PC불연속범위와새부모4/7범위를결합한다."""
        super().__init__(edition);self.p=PLAN[edition];self.mu.mem_map(0,4096);self.mu.mem_map(ARENA,0x4000)
        extra=[ROOT/f'extracted/fortworldload/{edition}/{name}' for name in ('creation.c','functions.tsv')];self.exports.extend(extra)
        ranges=[]
        with extra[1].open(encoding='utf-8-sig') as stream:ranges.extend(csv.DictReader(stream,delimiter='\t'))
        # 기존완료Exists helper만추가해현재3204BYTE를직접읽는다.
        path=ROOT/f'extracted/island-runtime/{edition}/functions.tsv';self.exports.extend([path.parent/'creation.c',path])
        with path.open(encoding='utf-8-sig') as stream:ranges.extend(row for row in csv.DictReader(stream,delimiter='\t') if int(row['entry'],16)==self.p['exists'])
        # 새/선택기존의실제몸체주소외에는실행을허용하지않는다.
        for row in ranges:
            self.entries.add(int(row['entry'],16))
            for part in row['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed[low-0x400000:high+1-0x400000]=b'\1'*(high+1-low)
        self.boundaries=collections.Counter()

    def write(self,mu,access,address,size,value,data):
        """새FS/depth/stream/출력외의쓰기에는기존Builder전체저장소제한을적용한다."""
        if address==self.p['depth'] and size==4:self.events.append(f'L:{value&0xffffffff}')
        if any(low<=address and address+size<=low+length for low,length in ((0,4),(ARENA,0x4000),(self.p['depth'],4))):return
        super().write(mu,access,address,size,value,data)

    def instruction(self,mu,address,size,data):
        """부모의currentTemplate/단일지역인자를관찰하고현재owner/Exists변경을명시공급한다."""
        p=self.p;c=self.case;targets={p[key] for key in ('lookup','generate','objects','money','badges','technology','populate','graph')}
        sp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX)
        if address==p['territory']:
            self.output=self.read(sp+4)
            if not c['kind']:assert bytes(mu.mem_read(self.output,120))==bytes(120)
        if address==self.s['all'] and c['mutation']==1:self.put(p['owner'],2)
        if address==self.s['all'] and c['mutation']==8:self.put(p['noRegion'],22)
        if address==self.s['assert_']:
            line=self.read(sp+12)
            if line in (p['missing'],0x2e):
                self.events.append('A:T' if line==p['missing'] else 'A:E');self.assertions+=1
                if line==p['missing']:self.put(this,MANAGER)
                mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,self.read(sp));return
        if address not in targets:
            return super().instruction(mu,address,size,data)
        self.boundaries[f'{address:08x}']+=1;args=[self.read(sp+4+index*4) for index in range(3)];purge=value=0
        if address==p['lookup']:
            assert bytes(mu.mem_read(args[0],10))==b'Territory\0' and args[1]==0
            if c['mutation']==9:self.put(p['owner'],5)
            value=STREAM;purge=8
        elif address==p['generate']:
            assert args[0]==self.read(p['noRegion']);self.events.append(f'G:{args[0]}');purge=4
            if c['mutation']==2:self.put(p['owner'],3)
            if c['mutation']==7:self.put(self.s['islands']+8,1)
        elif address==p['populate']:
            assert args[0]==0;self.events.append('I:0');purge=4
        elif address==p['graph']:
            assert args[0]==0;self.events.append('R:0')
        else:
            owner=((this-p['templates'])&0xffffffff)//20;assert owner<9
            token='O' if address==p['objects'] else 'M' if address==p['money'] else 'B' if address==p['badges'] else 'T'
            if token=='O':assert args[:2]==[0,20];purge=8
            self.events.append(f'{token}:{owner}')
            if c['mutation']==3 and token=='O':self.put(p['owner'],4)
            if c['mutation']==4 and token=='M':self.put(p['owner'],5)
            if c['mutation']==5 and token=='B':self.put(p['owner'],6)
            if c['mutation']==6 and token=='T':self.put(p['owner'],7)
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,self.read(sp))

    def run_parent(self,case,control):
        """단독getter또는전체부모를실제실행해모든청크/섬/스트림/현재전역/child순서를관찰한다."""
        self.case=case;self.kind='FortParent';self.values=case;self.events=[];self.assertions=0;p=self.p;s=self.s;mu=self.mu
        mu.mem_write(0,bytes(4));mu.mem_write(ARENA,bytes(0x4000));self.put(MANAGER+8,STREAM)
        mu.mem_write(s['world'],struct.pack('<III',1,16,16)+(bytes((127,ord('.')))+b'\xa5'*6+struct.pack('<I',0x12345678))*256)
        mu.mem_write(s['islands'],b'\xa5'*ISLAND_BYTES);self.put(s['islands'],case['count']);self.put(s['islands']+8,case['exists'])
        self.put(p['owner'],case['owner']);self.put(p['noRegion'],127);self.put(p['depth'],case['depth'])
        # 각실제Template첫pointer는명시입력이며null은diagnostic경계가복구한다.
        for owner in range(9):self.put(p['templates']+owner*20,MANAGER if case['template'] else 0)
        raw=bytes(case['position'])+territory(case['profile']);mu.mem_write(DATA,raw);self.put(STREAM,DATA);self.put(STREAM+4,case['position']);self.put(STREAM+8,case['end'])
        self.put(self.spec['globals']['types'],TYPES)
        # 기존실제CanonDecoder에현재원본패턴전역/불변프레임입력만준비한다.
        for address,value in zip(GEOMETRY[self.edition]['patterns'],(107,82,94,157,131,129,140,142),strict=True):self.put(address,value)
        codes=frame_codes(0);typ=TYPES+107*self.type_stride;mu.mem_write(typ,bytes(self.type_stride));self.put(typ+0x114,len(codes)//4);self.put(typ+0x124,CODES);mu.mem_write(CODES,codes)
        if self.edition=='originals':self.put(0x5e4794,0)
        self.prepare_registers(control);self.output=OUTPUT
        if case['kind']:
            mu.mem_write(OUTPUT,b'\xa5'*120);mu.reg_write(UC_X86_REG_ECX,p['templates']+case['owner']*20)
            mu.mem_write(STACK,struct.pack('<II',STOP,OUTPUT));mu.emu_start(p['territory'],STOP,count=600000);self.check(control,STACK+8)
        else:
            mu.mem_write(STACK,struct.pack('<II',STOP,case['alternate']));mu.emu_start(p['parent'],STOP,count=600000);self.check(control,STACK+4)
        self.returns+=1;islands=bytes(mu.mem_read(s['islands'],ISLAND_BYTES))
        return [str(self.read(STREAM+4)),str(self.read(STREAM+8)),str(self.read(p['depth'])),str(self.read(p['owner'])),str(self.read(p['noRegion'])),
          bytes(mu.mem_read(self.output,120)).hex(),block_rle(bytes(mu.mem_read(s['world']+12,256*12)),12),str(self.read(s['islands']))+','+block_rle(islands[4:],32),';'.join(self.events) or '-',str(self.assertions)]


def generate(smoke):
    """양판본의두x87 동일관찰만기록하고기존Builder/Exists 진입은새입력으로중복합산하지않는다."""
    rows=[];editions={};paths={Path(__file__).resolve(),ROOT/'tools/decomp_islandbuilder_oracle.py',ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_priestgeometry_oracle.py',
      ROOT/'tools/decomp_canontype_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/fortworldload-functions.json',ROOT/'cpppj/src/o/RawFortWorldLoad.h',ROOT/'cpppj/src/o/RawFortWorldLoad.cpp'}
    # 각판본은자기원본과current inline world 저장소를사용한다.
    for edition in PLAN:
        oracle=FortWorldOracle(edition);cases=inputs()
        if smoke:cases=cases[:6]+cases[108:126]+cases[-6:]
        normal=assertions=0
        # x87 반복관찰은독립입력수에다시더하지않는다.
        for case in cases:
            observed=[oracle.run_parent(case,control) for control in CONTROLS];assert observed[0]==observed[1],(edition,case)
            result=observed[0];normal+=result[-1]=='0';assertions+=result[-1]!='0';rows.append([edition,','.join(str(case[key]) for key in KEYS),*result])
        editions[edition]=dict(unique=len(cases),normal_unique=normal,assert_unique=assertions,returns=oracle.returns,native_calls=dict(oracle.native),boundaries=dict(oracle.boundaries),reused_ranges=oracle.reused_ranges)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: FortWorld {len(cases)} 正상{normal}/assert{assertions}',flush=True)
    if smoke:return
    FIXTURE.write_text('# edition\t'+','.join(KEYS)+'\tposition\tend\tdepth\towner\tnoRegion\tTerritory120\tfullChunkRle\tfullIslandRle\tevents\tasserts\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');paths.add(FIXTURE)
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',date='2026-10-14',total=len(rows),normal_total=sum(item['normal_unique'] for item in editions.values()),assert_total=sum(item['assert_unique'] for item in editions.values()),controls=list(CONTROLS),game_runs=0,os_calls=0,existing_body_observations_recounted=0,
      editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},limitations=['새부모/Territory getter/zero120과기존Builder/Runtime/CanonDecoder가같은current 저장소에서실제실행된다.',
      'Generate/Object/Money/Badges/Technology/Populate/Graph의하위몸체는명시관찰경계다. 전체게임이나하위생성완료로세지않는다.',
      'missingTemplate입력은명시진단callback이원래Template첫pointer를복구한후에만정상복사한다. 원래null역참조메모리완료를주장하지않는다.',
      '기존완료몸체의진입/같은입력x87반복을이번고유관찰에중복합산하지않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """전체현재SHA/고유입력/normal-assert분리와두정밀도ABI복귀를감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 모든보호PE/원본범위/도구/생산소스/fixture를전체바이트로연결한다.
    for name,value in report['files'].items():assert digest(ROOT/name)==value,name
    assert len(rows)==report['total']==len({(row[0],row[1]) for row in rows})
    # 행마지막진단횟수만으로정상/진단반환을분리한다.
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];assert len(selected)==item['unique']
        assert sum(row[-1]=='0' for row in selected)==item['normal_unique'] and sum(row[-1]!='0' for row in selected)==item['assert_unique'] and item['returns']==len(selected)*2
    print(f'FortWorld 현재출처감사: {len(rows)}')


# --smoke는무저장표본이고전체실행만원본기록을생성한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)
