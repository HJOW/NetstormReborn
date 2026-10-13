#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 실제 Gun finder/MayTarget/MayShoot/MayFire/사정선·CRT 각도 계산을 격리 x86에서 실행한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_rawfinder_oracle import FinderOracle, ROOT, POOL, HEAP, MAP, STACK_BASE, bits, sha
from decomp_bridgedecay_oracle import TYPES, FPU_CONTROLS, UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 실제 게임/OS를 실행하지 않는 별도 자료와 입력 스키마다.
FIXTURE=ROOT/'cpppj/tests/fixtures/guntarget-x86.tsv'
REPORT=ROOT/'cpppj/guntarget.json'
KEYS=('kind','type','direction','theme','range','tx','ty','extra','flags1','flags2','quantity','owner','allied','friendly','frame','scene','show','sid')
# 판본별 최상위 몸체·전역과 출력 주소다. CRT fmod도 기존 정밀 내보내기로 실제 실행한다.
SPEC={
 'originals':dict(entries=(0x46b6f0,0x46af80,0x46ac00,0x46b070,0x469190,0x4ade70),draw=0x41d670,
  friendly=0x55a490,show=0x54daa4,players=0x59534c,player_stride=0xb4,wind=0x541288,sun=0x541258,fence=0x541278,thunder=0x5412a8,
  half=0x5409c0,scales=(0x59a92c,0x59a930),colors=(0x5bec2c,0x59afd8),quantity_width=4),
 'originalCD':dict(entries=(0x435e10,0x434b20,0x435580,0x434c30,0x434d60,0x4aeec0),draw=0x440920,
  friendly=0x537ea0,show=0x52e830,players=0x50f834,player_stride=0xac,wind=0x51cb74,sun=0x51cb44,fence=0x51cb64,thunder=0x51cb94,
  half=0x51a198,scales=(0x565c44,0x565c48),colors=(0x55bad0,0x55bf50),quantity_width=2),
}
# fixture의 고정 발사자 위치와 격리 출력/Player/SHP 공간이다.
SOURCE_X,SOURCE_Y=50.75,52.5
OUTPUT,PLAYERS,SHAPES=HEAP+0x10000,HEAP+0x11000,HEAP+0x14000


def inputs():
    """분기·동맹·서버 SID·현재 수량·네 방향·사거리 경계의 입력만 만든다. 결과는 원본이 정한다."""
    base=dict(zip(KEYS,(1,71,0,0,bits(10.5),bits(55.75),bits(52.5),0,0x10,0,2,2,0,0,1,0,0,15050)))
    cases=[]
    # 세 판단 몸체를 여섯 group/특수 타입과4방향·9위치로 교차한다.
    for kind,typ,direction,delta in itertools.product((1,2,3),(71,128,129,140,148,83),(0,2,4,6),((-10,0),(0,-10),(10,0),(0,10),(0,0),(3.75,0),(20,0),(-1,-1),(0.25,0.25))):
        item=base.copy();item.update(kind=kind,type=typ,direction=direction,tx=bits(SOURCE_X+delta[0]),ty=bits(SOURCE_Y+delta[1]));cases.append(item)
    # MayTarget의 조기 반환과 사제 signed 수량/선택 상태/동일 위치를 독립 관찰한다.
    for sid,extra,flags1,flags2,quantity,allied,friendly in itertools.product((60,5999,6000,14999,15000,15050),(0,1,16,32),(0,0x10),(0,0x200000,0x400000),(0,1,2,0xffffffff),(0,1),(0,1)):
        # 같은 정상 branch를 반복하는 거대한 교차 대신 결정적인 독립 표본을 유지한다.
        if (sid+extra+flags1+flags2+quantity+allied+friendly)%43:continue
        item=base.copy();item.update(sid=sid,extra=extra,flags1=flags1,flags2=flags2,quantity=quantity,allied=allied,friendly=friendly);cases.append(item)
    for quantity in (0,1,2,32767,32768,65535,65536,0x7fffffff,0x80000000,0xffffffff):
        item=base.copy();item.update(flags2=0x200000,quantity=quantity);cases.append(item)
    # Cannon ray의 핫스폿·긴/짧은 사거리·Rain/Thunder 테마·짝수/홀수 방향 및 진단 출력을 관찰한다.
    for typ,direction,theme,length,scene in itertools.product((128,129),(0,1,2,4,6),(0,1,2),(0.5,2.0,10.5),(0,1)):
        item=base.copy();item.update(kind=4,type=typ,direction=direction,theme=theme,range=bits(length),scene=scene,show=scene);cases.append(item)
    # 썬 캐논의28프레임 방향 표 자체를 현재 표적4위치로 검사한다.
    for frame,direction in itertools.product(range(28),(0,2,4,6)):
        delta=((0,-4),(4,0),(0,4),(-4,0))[direction//2]
        item=base.copy();item.update(kind=3,type=128,frame=frame,tx=bits(SOURCE_X+delta[0]),ty=bits(SOURCE_Y+delta[1]));cases.append(item)
    # 실제 일반 finder의 strict score·ground1000·후보/확장 발자국과 표시 순서를 관찰한다.
    for typ,direction,scene,allied,friendly,show in itertools.product((71,128,129,140,148),(0,2,4,6),(0,1,2,3),(0,1),(0,1),(0,1)):
        item=base.copy();item.update(kind=0,type=typ,direction=direction,scene=scene,allied=allied,friendly=friendly,show=show);cases.append(item)
    # 범위 안/경계/범위 밖의 실제 rect 최소 거리와 float 저장 지점을 확인한다.
    for typ,length,dx,dy in itertools.product((71,140),(5.0,10.0,10.5),(0,4,5,10,10.000001),(0,3,5)):
        item=base.copy();item.update(kind=2,type=typ,range=bits(length),tx=bits(50.25+dx),ty=bits(52.0+dy));cases.append(item)
    # 확장 발자국의 직접 getter는 소수 좌표 및 SHP와 무관한 발자국 차이를 관찰한다.
    for scene,tx,ty in itertools.product((0,1,2),(1.25,50.75,255.0),(1.25,52.5,255.0)):
        item=base.copy();item.update(kind=5,scene=scene,tx=bits(tx),ty=bits(ty));cases.append(item)
    return list({tuple(item[key] for key in KEYS):item for item in cases}.values())


class GunTargetOracle(FinderOracle):
    """실제 finder/플래그/사거리/각도/CRT는 원본을 실행하고 진단 출력만 인자로 관찰한다."""
    def __init__(self,edition):
        """기존 격리 메모리와 precise body를 재사용하되 효과 대체표를 비운다."""
        super().__init__(edition);self.gun=SPEC[edition];self.stubs={};self.gun_entries=set();self.returns=0
        self.mu.mem_map(HEAP+0x10000,0x10000)
        # 네 고유 Gun 내보내기와 이미 검증된 CRT fmod 하위 몸체를 허용한다.
        for name in ('guntarget','guntarget-helper','guntarget-geometry','guntarget-distance-angle','pathmath'):
            paths=[ROOT/f'extracted/{name}/{edition}/{part}' for part in ('creation.c','functions.tsv')];self.body_paths.extend(paths)
            with paths[1].open(encoding='utf-8') as stream:
                # Ghidra가 기록한 실제 불연속 body만 실행한다.
                for row in csv.DictReader(stream,delimiter='\t'):
                    self.gun_entries.add(int(row['entry'],16))
                    for part in row['ranges'].split(';'):
                        low,high=(int(v,16) for v in part.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\x01'*(high+1-low)

    def read(self,address,width=4):
        """little endian raw 값을 원본 필드 폭으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self,address,value,width=4):
        """입력 필드만 지정한 폭으로 준비한다."""
        self.mu.mem_write(address,int(value).to_bytes(width,'little'))

    def on_write(self,mu,access,address,size,value,data):
        """표적 판단은 풀·타입·해시에 쓰지 않으며 스택·지역 출력만 변경한다."""
        if STACK_BASE<=address and address+size<=STACK_BASE+0x40000:return
        if address==0 and size==4:return
        if OUTPUT<=address and address+size<=OUTPUT+16:return
        raise RuntimeError(f'GunTarget 읽기 전용 영역 쓰기: {self.edition} {address:08x}+{size}')

    def on_instruction(self,mu,address,size,data):
        """계산 몸체는 대체하지 않고 진단 그리기 요청만 현재 사각형/색으로 기록한다."""
        if address in self.gun_entries:self.native_calls[f'{address:08x}']+=1
        if address!=self.gun['draw']:return super().on_instruction(mu,address,size,data)
        sp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX);color=self.read(sp+4)
        self.events.append('D:'+str(int(color==1002))+':'+','.join(str(self.read(this+i*4)) for i in range(4)))
        self.stub_calls['drawRange']+=1;mu.reg_write(UC_X86_REG_ESP,sp+8);mu.reg_write(UC_X86_REG_EIP,self.read(sp))

    def run_target(self,case,control=FPU_CONTROLS[0]):
        """상위 결과를 계산하지 않고 같은 타입/슬롯/네 해시 입력의 native 반환·출력을 기록한다."""
        p=self.gun;typ=case['type'];sid=case['sid'];scene=case['scene'];extra=self.o['extra'];owner_offset=34 if self.edition=='originals' else 32
        width,height,extension=(1,1,0) if scene==0 else (3,2,3 if scene==2 else 0)
        objects=[(sid,86,0,case['extra'],case['tx'],case['ty'],width,height,extension,1)]
        if case['kind']==0:
            # 같은 거리·지상/공중·큰 발자국·finder 순서에 영향을 주는 독립 후보들이다.
            objects.extend([(15060,87,0,0,bits(53.75 if scene!=3 else 55.75),bits(52.5),1,1,0,1),
                            (15070,88,0,0,bits(50.75),bits(56.5),2,2,1,2)])
            if scene==3:objects.reverse()
        self.mu.mem_write(POOL,bytes(24000*self.stride));self.mu.mem_write(TYPES,bytes(256*self.type_stride));self.setup(objects)
        # 현재 source 타입의 meta/SHP와 후보 공통 플래그/owner/수량을 준비한다.
        for number in (71,128,129,140,148,83):
            start=TYPES+number*self.type_stride;self.put(start+0x9c,1 if number in (128,129) else 3 if number==83 else 0)
            self.put(start+0x44,case['range']);self.put(start+0x98,case['theme']);self.put(start+self.foot_offset,1);self.put(start+self.foot_offset+4,1)
            self.put(start+self.foot_offset+8,2 if scene else 0);self.put(start+self.foot_offset+12,(-3 if scene else 0)&0xffffffff)
            self.mu.mem_write(start+0x15c,struct.pack('<4i',0,1,2,3));shape=SHAPES+(number%8)*0x400
            self.put(start+0xdc,shape);self.put(start+0x114,4)
            # 실제 sprite lookup은 offset+0x24로 되돌아온 헤더의 signed short 핫스폿을 읽는다.
            for index in range(4):
                header=shape+0x80+index*0x40;self.put(shape+8+index*8,header-shape+0x24)
                self.mu.mem_write(header+0x20,struct.pack('<hh',3*index-4 if scene else 0,index-2 if scene else 0))
        for target in (sid,15060,15070):
            start=self.slot(target);number=self.read(start+10,1)
            self.put(start+owner_offset,case['owner'],1);self.put(start+26,case['quantity']&((1<<(p['quantity_width']*8))-1),p['quantity_width'])
            flags2=case['flags2'] if target==sid else 0x100000 if target==15060 else 0
            self.put(TYPES+number*self.type_stride+0xe8,case['flags1']);self.put(TYPES+number*self.type_stride+0xec,flags2)
        self.put(p['players'],PLAYERS);self.mu.mem_write(PLAYERS,bytes(9*p['player_stride']))
        self.put(PLAYERS+p['player_stride']+0x54,(1<<1)|((1<<case['owner']) if case['allied'] else 0))
        self.put(p['friendly'],case['friendly']);self.put(p['show'],case['show']);self.put(self.g.get('asserts',0x531454),0)
        self.put(p['sun'],128);self.put(p['fence'],136);self.put(p['wind'],140);self.put(p['thunder'],148);self.put(p['half'],bits(30))
        self.put(p['scales'][0],bits(16));self.put(p['scales'][1],bits(11));self.put(p['colors'][0],1001);self.put(p['colors'][1],1002)
        if self.edition=='originalCD':self.put(0x546778,0)
        query=[bits(SOURCE_X),bits(SOURCE_Y),bits(50.25),bits(52),typ,case['direction'],1,case['frame']]
        kind=case['kind'];this=0;purge=0
        if kind==0:args=[query[0],query[1],typ,case['direction'],1]
        elif kind in (1,2,3):args=[*query,sid]
        elif kind==4:args=[OUTPUT,*query,case['direction']]
        else:args=[OUTPUT,1];this=self.slot(sid);purge=8
        self.mu.mem_write(OUTPUT,bytes(16));self.spec=dict(self.spec,entries=dict(self.spec['entries'],Gun=p['entries'][kind]));self.call('Gun',args,this,purge,control)
        self.returns+=1
        result=self.mu.reg_read(UC_X86_REG_EAX) if kind<4 else 0
        return [result,','.join(str(self.read(OUTPUT+4*i)) for i in range(4)),';'.join(self.events) or '-']


def generate(smoke=False):
    """두 정밀도 반복이 같은 결과일 때만 고유 입력을 한 번 기록하고 검증 SHA를 보존한다."""
    cases=inputs();cases=cases[::109] if smoke else cases;rows=[];editions={};files=[Path(__file__),FIXTURE]
    for edition in SPEC:
        oracle=GunTargetOracle(edition)
        for index,case in enumerate(cases):
            try:
                result=oracle.run_target(case);again=oracle.run_target(case,FPU_CONTROLS[1])
                if result!=again:raise RuntimeError(f'x87 정밀도 차이: {result} != {again}')
            except Exception as error:raise RuntimeError(f'{edition} #{index} {case}: {error}') from error
            rows.append([edition,','.join(str(case[key]) for key in KEYS),*result])
        editions[edition]=dict(cases=len(cases),returns=oracle.returns,assertions=oracle.assertions,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.stub_calls),instructions=oracle.instructions)
        files.extend(oracle.body_paths);files.append(ROOT/oracle.spec['binary'])
    FIXTURE.write_text('# 실제 Gun 두 PE의 고유 입력 및 정상 반환/사정선/진단 요청. 정밀도 반복은 새 관찰로 세지 않는다.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8')
    files.extend(ROOT/path for path in ('cpppj/src/o/RawGunTarget.h','cpppj/src/o/RawGunTarget.cpp','cpppj/tests/GunTargetTests.cpp',
        'cpppj/src/o/RawGunProcess.h','cpppj/src/o/RawGunProcess.cpp','cpppj/src/o/RawSquidFinder.h','cpppj/src/o/RawSquidFinder.cpp',
        'cpppj/src/o/SidPool.h','cpppj/src/o/SidPool.cpp','cpppj/src/o/SquidFactory.h','cpppj/src/o/SquidFactory.cpp','cpppj/src/o/SquidHash.h','cpppj/src/o/SquidHash.cpp',
        'cpppj/tests/RawSceneSupport.h','cpppj/tests/TestSupport.h','tools/decomp_rawfinder_oracle.py','tools/decomp_bridgeeffects_oracle.py','tools/decomp_bridgedecay_oracle.py'))
    files.extend(ROOT/f'tools/ghidra/{name}-functions.json' for name in ('guntarget','guntarget-helper','guntarget-geometry','guntarget-distance-angle','pathmath'))
    REPORT.write_text(json.dumps(dict(schema=1,host='VM-W11-CODEX',total=len(rows),editions=editions,files={path.relative_to(ROOT).as_posix():sha(path) for path in set(files) if path.exists()},limitations=['공간 등록 입력은 직접 만든 네 실제 해시/next 상태이며 미션 전체 Pop을 증명하지 않는다.','진단 drawRange만 실제 사각형 인자로 관찰하며 게임/GUI/투사체/소리/애니메이션은 실행하지 않는다.','동일성을 관찰한 입력의 부동소수 정밀도 반복은 고유 관찰 수에서 제외한다.'],game_runs=0),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(total=len(rows),editions=editions),ensure_ascii=False))


def verify():
    """고유 입력·모든 출처 SHA·정상 반환·미대체 판단·두 정밀도 확인을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    if len(rows)!=report['total']:raise RuntimeError('GunTarget 행 수 불일치')
    for name,value in report['files'].items():
        if sha(ROOT/name)!=value:raise RuntimeError('GunTarget SHA 변화: '+name)
    expected=[','.join(str(case[key]) for key in KEYS) for case in inputs()]
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if [row[1] for row in selected]!=expected or item['assertions'] or item['returns']!=2*len(selected) or set(item['boundaries'])-{'drawRange'}:
            raise RuntimeError('GunTarget 입력/실제 판단 감사 오류')
    print('GunTarget SHA/고유 입력/실제 계산/두 정밀도 정상 반환 감사:',len(rows))


if __name__=='__main__':
    # 저장 생성 또는 감사만 실행하며 원본 게임 프로세스를 만들지 않는다.
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)
