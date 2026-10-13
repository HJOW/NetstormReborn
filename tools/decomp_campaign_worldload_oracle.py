#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 미션 저장 부모의 prologue·Builder/Territory·mission 후처리 자연반환을 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_fortworldload_oracle import FortWorldOracle,ARENA,MANAGER,STREAM,DATA,OUTPUT,PLAN as FORT,territory
from decomp_islandbuilder_oracle import CHUNK_BYTES,ISLAND_BYTES,CODES,block_rle
from decomp_owner_oracle import ROOT,TYPES,STOP,STACK,digest
from decomp_priestgeometry_oracle import GEOMETRY
from decomp_canontype_oracle import frame_codes
from decomp_damageablepredestroy_oracle import CONTROLS
from fort import TypeCatalog,parse_fort,split_sections,SECTION_NAMES,FLAG1_BITS,FLAG2_BITS,derive_flags,type_hash
from shp import load_type_texts,TYPE_LOAD_ORDER
from typefile import parse_type
from taff import TaffArchive
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# Player9/현재handle/명시 번역결과는 기존 Fort 분석buffer 안의 독립 주소다.
PLAYERS,HANDLE,TRANSLATED=ARENA+0x2000,ARENA+0x3000,ARENA+0x3100
FIXTURE=ROOT/'cpppj/tests/fixtures/campaign-worldload-x86.tsv'
REPORT=ROOT/'cpppj/recovery-campaign-worldload-evidence.json'
KEYS=('owner','battle','editor','profile','position','end','depth','mutation','subscriber','handle')
# 실제미션 부모/선택후처리와 명시 하위경계를 판본별로 분리한다.
PLAN={
 'originals':dict(parent=0x4b5300,translate=0x4de9a0,config=0x4414d0,refresh=0x460730,player=0x490250,finish=0x4923e0,camera=0x4340d0,
   my=0x540c74,subscriber=0x54265c,battle=0x540c70,handle=0x540ca8,players=0x59534c,editor=0x5c85a4,mission=0x594fa4,stride=0xb4,region=0x78,you=0x51398c),
 'originalCD':dict(parent=0x4322e0,translate=0x4bcb20,config=0x4a9930,refresh=0x4ca4e0,player=0x407400,finish=0x40daf0,camera=0x488f50,
   my=0x50f6cc,subscriber=0x511c7c,battle=0x50f6c8,handle=0x50f6d0,players=0x50f834,editor=0x518904,mission=0x52e170,stride=0xac,region=0x70,you=0x519c18)}


def sample(**updates):
    """현재원본 전역/stream/좁은문자 입력만 준비하고 기대효과를 계산하지 않는다."""
    row=dict(zip(KEYS,(1,1,0,1,0,120,0,0,0x12345678,1)));row.update(updates);return row


def inputs():
    """두owner/editor/실제120BYTE와 효과뒤현재owner 재조회를 교차한다."""
    rows=[]
    # Template owner와battle owner가 같다는 전제를 넣지 않는다.
    for index,(owner,battle,editor,profile) in enumerate(itertools.product((0,1,8),(0,1,8),(0,1),(0,1,2))):
        rows.append(sample(owner=owner,battle=battle,editor=editor,profile=profile,position=(0,3,19)[index%3],end=(120,139,0x80000000)[index%3],depth=(0,0xffffffff)[index%2],handle=index%3))
    # 각명시경계의전역 변경은 다음 실제원본명령이현재값을 다시 읽는지만관찰한다.
    for mutation in range(1,11):rows.append(sample(editor=1,mutation=mutation,depth=3))
    # 보호 War Begins의실제Territory/172저장오브젝트는 부모의하위Object 요청경계와분리한다.
    rows.append(sample(profile=3,subscriber=0x82000004));return rows


class CampaignCatalog(TypeCatalog):
    """CD에없는패치타입을요청하지않고현재보호타입/실제TypeNames해시만해석한다."""
    def __init__(self,edition):
        """현재archive의실제typename 목록과 저장필드플래그만준비한다."""
        self.flags={};self.by_hash={}
        # 파일에실제로존재하는타입만등록하며부족한타입을대체정의하지않는다.
        texts=load_type_texts(str(ROOT/edition))
        for name in TYPE_LOAD_ORDER:
            if name.lower() not in texts:continue
            definition=parse_type(texts[name.lower()]);f1=f2=0
            # 같은기존저장파서의실제선택필드플래그로단어를해석한다.
            for word in definition.flags:
                f1|=next((value for key,value in FLAG1_BITS.items() if key.lower()==word.lower()),0)
                f2|=next((value for key,value in FLAG2_BITS.items() if key.lower()==word.lower()),0)
            self.flags[name]=derive_flags(f1,f2);self.by_hash[type_hash(name)]=name


class CampaignWorldOracle(FortWorldOracle):
    """이미완료Builder/Territory를 같은inline저장소로 재사용하고 실제미션부모만 추가한다."""
    def __init__(self,edition):
        """부모 최초export2/2와별도후처리export1/1을덮지않고불연속범위를허용한다."""
        super().__init__(edition);self.cp=PLAN[edition];self.parent_boundaries=collections.Counter()
        # 최초부모와후속helper의원문/주소범위를각각전체출처로보존한다.
        for name in ('campaign-world-load','campaign-world-post'):
            paths=[ROOT/f'extracted/{name}/{edition}/{part}' for part in ('creation.c','functions.tsv')];self.exports.extend(paths)
            with paths[1].open(encoding='utf-8-sig') as stream:
                for row in csv.DictReader(stream,delimiter='\t'):
                    if int(row['entry'],16) not in (self.cp['parent'],self.cp['finish']):continue
                    self.entries.add(int(row['entry'],16))
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'));self.allowed[low-0x400000:high+1-0x400000]=b'\1'*(high+1-low)
        self.archive=ROOT/edition/'netstorm.tarc';archive=TaffArchive(str(self.archive));entry=next(item for item in archive.entries if item.name.lower()=='\\d\\thewarbegins.fort')
        self.fortBytes=archive.read(entry);sections,_=split_sections(self.fortBytes);named=dict(zip(SECTION_NAMES,sections));self.missionTerritory=named['Territory'];assert len(self.missionTerritory)==120
        self.fortSummary=parse_fort(self.fortBytes,CampaignCatalog(edition));self.storedObjects=sum(self.fortSummary['objects'].values());assert self.storedObjects==172

    def write(self,mu,access,address,size,value,data):
        """최초subscriber/현재Template+8·handle/Player이름의실제쓰기만추가로허용한다."""
        if not hasattr(self,'cp'):return super().write(mu,access,address,size,value,data)
        if any(low<=address and address+size<=low+length for low,length in ((self.cp['my'],4),(self.p['templates'],180))):return
        super().write(mu,access,address,size,value,data)

    def return_boundary(self,sp,value=0,purge=0):
        """명시 외부콜의 실제ABI 복귀만 공급하며 하위게임몸체 완료로 세지 않는다."""
        self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,self.read(sp))

    def instruction(self,mu,address,size,data):
        """원본prologue/tail과같은Builder를실행하며 번역/config/spec/Player/Terrain/Object/UI는경계다."""
        if not hasattr(self,'cp'):return super().instruction(mu,address,size,data)
        cp=self.cp;c=self.input;sp=mu.reg_read(UC_X86_REG_ESP);args=[self.read(sp+4+index*4) for index in range(2)]
        if address in (self.s['constructor'],self.s['attach'],self.s['all']):
            self.events.append('C' if address==self.s['constructor'] else 'A' if address==self.s['attach'] else 'P:0:20')
            if address==self.s['all'] and c['mutation']==6:self.put(self.p['owner'],6)
        if address==cp['finish']:self.events.append('F:1');assert self.read(cp['mission'])==1
        if address==self.p['generate'] and c['mutation']==8:self.put(self.p['owner'],8)
        if address==self.p['objects'] and c['mutation']==9:self.put(cp['battle'],8)
        targets={cp[key] for key in ('translate','config','refresh','player','camera')}
        if address not in targets:return super().instruction(mu,address,size,data)
        self.parent_boundaries[f'{address:08x}']+=1
        if address==cp['translate']:
            assert args[0]==cp['you'] and bytes(mu.mem_read(cp['you'],4))==b'You\0';self.events.append('Y');self.return_boundary(sp,TRANSLATED,4)
        elif address==cp['config']:
            assert bytes(mu.mem_read(args[0],17))==b'fort.mySubHandle\0' and args[1]==HANDLE;self.events.append('S');self.return_boundary(sp)
            if c['mutation']==1:self.put(self.p['owner'],2)
        elif address==cp['refresh']:
            self.events.append('V');self.return_boundary(sp)
            if c['mutation']==2:self.put(cp['battle'],2)
        elif address==cp['player']:
            self.events.append('L:'+str(args[0]));self.return_boundary(sp)
            if c['mutation']==3:self.put(cp['battle'],3)
            if c['mutation']==4:self.put(cp['editor'],0)
            if c['mutation']==5:self.put(self.p['owner'],5)
        else:
            self.events.append('K');self.return_boundary(sp)
            if c['mutation']==7:self.put(cp['battle'],7)
            if c['mutation']==10:mu.mem_write(HANDLE,b'camera-name\0')

    def run_campaign(self,case,control):
        """동일현재입력에서실제부모/후처리의정상반환과전체지도·섬·Player·문자열을관찰한다."""
        self.input=case;self.case=dict(case,kind=0,template=1,count=0,exists=0,mutation=0);self.kind='Campaign';self.values=case;self.events=[];self.assertions=0;p=self.p;cp=self.cp;mu=self.mu
        mu.mem_write(0,bytes(4));mu.mem_write(ARENA,b'\xa5'*0x4000);self.put(MANAGER+8,STREAM)
        mu.mem_write(self.s['world'],struct.pack('<III',1,16,16)+(bytes((127,ord('.')))+b'\xa5'*6+struct.pack('<I',0x12345678))*256)
        mu.mem_write(self.s['islands'],b'\xa5'*ISLAND_BYTES);self.put(self.s['islands'],0)
        # 원본전역 포인터/현재9기록을 명시입력으로 공급한다. Player 하위경계는 기록에 가짜SID를 쓰지 않는다.
        mu.mem_write(PLAYERS,b'\xa5'*(9*cp['stride']));self.put(cp['players'],PLAYERS);self.put(cp['handle'],HANDLE);self.put(cp['battle'],case['battle']);self.put(cp['editor'],case['editor']);self.put(cp['mission'],1)
        self.put(cp['my'],0xabcdef01);self.put(cp['subscriber'],case['subscriber']);self.put(p['owner'],case['owner']);self.put(p['noRegion'],127);self.put(p['depth'],case['depth'])
        mu.mem_write(p['templates'],b'\xa5'*180)
        # 실제9개Template의manager pointer는현재저장소를모두직접가리킨다.
        for owner in range(9):self.put(p['templates']+owner*20,MANAGER)
        name=(b'\0',b'You\0',b'new-handle\0')[case['handle']];mu.mem_write(TRANSLATED,name);mu.mem_write(HANDLE,b'\x7e'*64)
        records=self.missionTerritory if case['profile']==3 else territory(case['profile']);mu.mem_write(DATA,bytes(case['position'])+records);self.put(STREAM,DATA);self.put(STREAM+4,case['position']);self.put(STREAM+8,case['end'])
        self.put(self.spec['globals']['types'],TYPES)
        # 같은기존CanonDecoder에최초불변pattern/type프레임 입력만공급한다.
        for address,value in zip(GEOMETRY[self.edition]['patterns'],(107,82,94,157,131,129,140,142),strict=True):self.put(address,value)
        codes=frame_codes(0);typ=TYPES+107*self.type_stride;mu.mem_write(typ,bytes(self.type_stride));self.put(typ+0x114,len(codes)//4);self.put(typ+0x124,CODES);mu.mem_write(CODES,codes)
        if self.edition=='originals':self.put(0x5e4794,0)
        self.output=OUTPUT;self.prepare_registers(control);mu.mem_write(STACK,struct.pack('<I',STOP));mu.emu_start(cp['parent'],STOP,count=1000000);self.check(control,STACK+4);self.returns+=1
        assert self.read(0)==0
        return [str(self.read(cp['my'])),bytes(mu.mem_read(p['templates'],180)).hex(),bytes(mu.mem_read(HANDLE,64)).hex(),bytes(mu.mem_read(PLAYERS,9*cp['stride'])).hex(),
            str(self.read(p['owner'])),str(self.read(cp['battle'])),str(self.read(cp['editor'])),str(self.read(STREAM+4)),str(self.read(STREAM+8)),str(self.read(p['depth'])),
            block_rle(bytes(mu.mem_read(self.s['world']+12,256*12)),12),str(self.read(self.s['islands']))+','+block_rle(bytes(mu.mem_read(self.s['islands']+4,ISLAND_BYTES-4)),32),';'.join(self.events) or '-',str(self.assertions)]


def generate(smoke):
    """최초두x87관찰을비교하고 새부모고유입력/진단·미션하위경계를구분한다."""
    rows=[];editions={};files={Path(__file__).resolve(),ROOT/'tools/ghidra/campaign-world-load-functions.json',ROOT/'tools/ghidra/campaign-world-post-functions.json',ROOT/'cpppj/src/o/RawCampaignWorldLoad.h',ROOT/'cpppj/src/o/RawCampaignWorldLoad.cpp'}
    # 각 실제PE는 자기보호WarBegins와같은원본inline자료를 사용한다.
    for edition in PLAN:
        oracle=CampaignWorldOracle(edition);cases=inputs()
        if smoke:cases=[cases[0],cases[1],cases[53],*cases[-11:]]
        normal=diagnostic=0
        # 반복정밀도/기존Builder 몸체의 내부콜을 새고유수에 더하지 않는다.
        for case in cases:
            observed=[oracle.run_campaign(case,control) for control in CONTROLS];assert observed[0]==observed[1],(edition,case);normal+=observed[0][-1]=='0';diagnostic+=observed[0][-1]!='0';rows.append([edition,','.join(str(case[key]) for key in KEYS),*observed[0]])
        editions[edition]=dict(unique=len(cases),normal_unique=normal,assert_unique=diagnostic,returns=oracle.returns,actual_body_calls=dict(oracle.native),child_boundaries=dict(oracle.boundaries),prologue_ui_player_boundaries=dict(oracle.parent_boundaries),stored_objects=172,parent_input_target=173)
        files.update(oracle.exports);files.add(oracle.archive);files.add(ROOT/oracle.spec['binary']);print(edition,editions[edition],flush=True)
    if smoke:return
    # 독립원문/재사용도구의전체출처를보존하며새Player/전체GameWorld완료로확대하지않는다.
    for name in ('decomp_fortworldload_oracle','decomp_islandbuilder_oracle','decomp_owner_oracle','decomp_priestgeometry_oracle','decomp_canontype_oracle','decomp_damageablepredestroy_oracle','fort','taff','typefile','shp'):files.add(ROOT/f'tools/{name}.py')
    FIXTURE.write_text('# edition\t'+','.join(KEYS)+'\tmySubscriber\tTemplate180\tHandle64\tPlayer9\tTemplateOwner\tBattleOwner\tEditor\tPosition\tEnd\tDepth\tChunkRle\tIslandRle\tEvents\tAsserts\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');files.add(FIXTURE)
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',date='2026-10-14',total=len(rows),normal_total=sum(item['normal_unique'] for item in editions.values()),assert_total=sum(item['assert_unique'] for item in editions.values()),controls=list(CONTROLS),editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},game_runs=0,os_calls=0,
        limitations=['실제부모 prologue/tail·기존Builder/Territory/Runtime/ChunkMap·mission1후처리의자연반환을원본명령으로실행한다. genericFort534와분리한새고유입력이다.',
            '번역/config/spec/Player생성/지형/ObjectLoad/Graph/Camera의하위호출은명시경계다. 실제173전체설치/Ready/login/AI/GUI완료근거가아니다.',
            '보호WarBegins의172저장자료를확인하고해당Territory120입력을선택한행을분리한다.173은입력대상이며최종Pool수가아니다.',
            '원본Money/Badges/Technology호출0을임의추가하지않으며미션SP/myTech는후속stage3별도범위다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """부모/후처리원문·보호PE/archive·고유수·normal/assert와전체SHA를감사한다."""
    proof=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 실제현재각출처를전체바이트로확인한다.
    for name,value in proof['files'].items():assert digest(ROOT/name)==value,name
    assert len(rows)==proof['total']==len({(row[0],row[1]) for row in rows}) and sum(int(row[-1])==0 for row in rows)==proof['normal_total']
    print('ActualCampaignWorld 부모 전체 출처/고유수/정상·assert 감사 통과')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
