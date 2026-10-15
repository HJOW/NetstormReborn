#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PATCH Early Missions3~6 전체 분기와 실제 Gate/TotalMade/Genus를 격리 실행한다."""
import collections
import csv
import itertools
import json
import struct
import sys
import decomp_missionframe_oracle as common
from decomp_missionframe_oracle import ROOT, bits, digest
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 별도 raw 입력 주소와 원본 타입 전역/실제 개수 표다.
POOL,TYPES,PLAYERS=0x12000000,0x13000000,0x13200000
TYPE_GLOBALS={0x541174:71,0x541178:72,0x541184:75,0x541188:76,0x54118c:77,0x541190:78,0x541244:123,0x541258:128,0x541264:131,0x541268:132,0x54126c:133,0x541288:140,0x54128c:141,0x5412ec:165}
COMMANDS={0x5429c4:0x1230001,0x5429b4:0x1230002,0x5429b8:0x1230003,0x5429c8:0x1230004,0x542a50:0x1230005,0x542a70:0x1230006}
KEYS=['tutorial','stage','event','arg1','arg2','production','global','local','vortex','sourceType','sourceByte','sourceGenus','destType','ownPriest','enemyPriest','ownGraph','enemyGraph','captured','region','home','nearby','selected','tell','c0','c4','timer','now','depth','lock','mutation']
FIXTURE=ROOT/'cpppj/tests/fixtures/tutorial-later-x86.tsv'
REPORT=ROOT/'cpppj/tutorial-later.json'


def cases():
    """모든 실제 단계와 조건/서명DWORD/예약/정지/상위modifier를 서로 다른 명시 입력으로 방문한다."""
    result=[]
    for tutorial,stage,scene in itertools.product(range(3,7),'@ABCDEFGHI',range(10)):
        c=dict(zip(KEYS,[tutorial,ord(stage),0x1230001,1,2,0xffffffff,1,0,1,71,0,0x200000,133,10,11,1,1,1,2,1,0,1,1,0,0,bits(0.),bits(10.),0,0,0]))
        if scene==1:c.update(production=0,global_=0,local=1,vortex=0,enemyPriest=0);c['global']=c.pop('global_')
        elif scene==2:c.update(event=0x1230003,sourceGenus=0x200000,timer=bits(10.));c['global']=0xffffffff
        elif scene==3:c.update(event=0x1230002,enemyGraph=2,selected=0,local=1);c['global']=0x80000000
        elif scene==4:c.update(event=0x1230004,arg1=132,arg2=78,sourceType=72,sourceByte=1,region=1,nearby=1,vortex=0,local=0xffffffff);c['global']=0x7fffffff
        elif scene==5:c.update(event=0x1230003,c0=1,c4=1,tell=0,timer=bits(11.),sourceGenus=0x100)
        elif scene==6:c.update(event=0x1230006,sourceType=72,sourceGenus=0x100,now=bits(16777216.25));c['global']=0;c['local']=0
        elif scene==7:c.update(event=0x1230005,sourceType=133,timer=bits(9.),selected=0,local=2);c['global']=2
        elif scene==8:c.update(event=0x1230004,arg1=132,arg2=78)
        elif scene==9:c.update(event=0xf1230001,production=1<<7,captured=0)
        result.append(c)
    # Advance가 거부되어도 옵션/허용/선택/알림 후속효과는 남는지 확인한다.
    for tutorial,stage,depth,lock in itertools.product(range(3,7),'ABCDEFGH',range(2),(0,1)):
        c=dict(next(value for value in result if value['tutorial']==tutorial and value['stage']==ord(stage)))
        c.update(depth=depth*2,lock=lock,event=0x1230003,timer=bits(10.));result.append(c)
    # 외부 선택/생산/안내가 현재 입력을 바꾸는 효과의 읽기 순서다.
    for tutorial,stage,mutation in ((3,'B',1),(3,'D',2),(5,'C',3),(5,'C',4),(5,'D',5)):
        c=dict(next(value for value in result if value['tutorial']==tutorial and value['stage']==ord(stage)));c['mutation']=mutation
        if mutation==4:c.update(vortex=0,c0=0)
        if mutation==5:c.update(production=0,c4=0)
        result.append(c)
    unique={}
    for c in result:unique.setdefault(tuple(c[key] for key in KEYS),c)
    return list(unique.values())


class Oracle(common.Oracle):
    """게임 계산은 원본 바이트이며 생산/UI/Graph/지역/포획 조회만 명시 외부 경계다."""
    def __init__(self):
        """현재 전체 정밀 함수 범위만 허용하고 raw 입력을 분리한다."""
        super().__init__();self.allowed=[];self.calls=collections.Counter();self.active=False
        for address,size in ((POOL,0x10000),(TYPES,0x20000),(PLAYERS,0x10000)):self.mu.mem_map(address,size)
        entries=common.TUTORIALS[2:]+[0x4c33f0,0x4c2a40,0x4c2480,0x4c2560,0x4c23e0,0x4ac200,0x49a840]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):
                if int(row['entry'],16) in entries:
                    for part in row['ranges'].split(';'):
                        low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.entries=set(entries);self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def read(self,mu,access,address,size,value,data):
        """실제 TYPE 전역 읽기마다 현재 바이트 재조회를 기록한다."""
        if self.active and address in TYPE_GLOBALS:
            if size!=4:raise RuntimeError('Tutorial 타입 전역 읽기 폭 오류')
            self.trace.append('Y'+str(address))

    def effect(self,kind):
        """첫 외부 효과의 명시 입력 변화만 적용한다. 원본 조건을 계산하지 않는다."""
        self.trace.append(kind)
        if kind in self.changed:return
        self.changed.add(kind);mutation=self.case['mutation']
        if mutation==1 and kind.startswith('F'):self.put(0x540c70,2)
        elif mutation==2 and kind.startswith('I'):self.case['production']=0
        elif mutation==3 and kind=='S':self.case['selected']=2
        elif mutation==4 and kind.startswith('T'):self.put(common.GLOBALS['stage'],ord('D'))
        elif mutation==5 and kind=='K':self.depth=3;self.put(common.MISSION+0xb8,7)

    def instruction(self,mu,address,size,data):
        """원본 Tutorial/Advance/Pause/제작/부류는 실행하고 선택된 큰 하위만 인자와 함께 대체한다."""
        if address==common.STOP:return
        if self.first_fields is not None:
            c0,c4,arg1,arg2=self.first_fields;self.first_fields=None
            self.put(common.MISSION+0xc0,c0);self.put(common.MISSION+0xc4,c4);self.put(common.EVENT+4,arg1);self.put(common.EVENT+8,arg2)
        if address in self.entries:self.calls[f'{address:08x}']+=1
        if address in common.TUTORIALS[2:]:self.put(common.MISSION,common.STOP+0x200);self.put(common.STOP+0x228,0x4c2a40)
        esp=mu.reg_read(UC_X86_REG_ESP);arg=self.get(esp+4);cleanup=0;result=0;kind=None
        if address==0x4921f0:kind='F'+str(arg);result=self.case['ownPriest' if arg==1 else 'enemyPriest']
        elif address==0x462bc0:kind='G'+str(arg);result=self.case['ownGraph' if arg==self.case['ownPriest'] else 'enemyGraph']
        elif address==0x44f970:kind='C'+str(arg);result=int(bool(self.case['production']&(1<<(arg%32))))
        elif address==0x4929e0:kind='I'+str(arg)
        elif address==0x46f530:kind='R';result=self.case['region'];cleanup=8
        elif address==0x4b2070:kind='N'+str(self.get(esp+12));result=self.case['nearby']
        elif address==0x493c00:kind='H'+str(arg);result=self.case['captured']
        elif address==0x4d5430:kind='S';result=self.case['selected']
        elif address==0x482de0:kind='T'+bytes(mu.mem_read(arg,32)).split(b'\0')[0].decode('ascii');result=self.case['tell'];cleanup=4
        elif address==0x466710:kind='K'
        elif address==0x4d5c90:
            if any(self.get(esp+o) for o in (4,8,12)):raise RuntimeError('선택 초기화0/0/0 오류')
            kind='Z';self.case['selected']=0
        elif address==0x4d5c60:kind='X';self.case['selected']=0
        if kind is not None:
            self.boundaries[kind]+=1;self.effect(kind);target=self.get(esp);mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,esp+4+cleanup);mu.reg_write(UC_X86_REG_EIP,target);return
        if any(low<=address and address+size<=high for low,high in self.allowed):return
        super().instruction(mu,address,size,data)

    def write(self,mu,access,address,size,value,data):
        """옵션BYTE2와 현재 다이스 허용DWORD 외의 raw 입력 쓰기는 거부한다."""
        if address==0x52f7e2 and size==1:return
        if address==0x5ca0d0+165*4 and size==4:return
        super().write(mu,access,address,size,value,data)

    def run(self,c):
        """각 독립 입력의 전체 공유 상태/옵션/제한/효과와 정상 ABI 반환을 관찰한다."""
        self.active=False;mu=self.mu
        mu.mem_write(POOL,bytes(0x10000));mu.mem_write(TYPES,bytes(0x20000));mu.mem_write(PLAYERS,bytes(1620))
        self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES);self.put(0x541348,188);self.put(0x59534c,PLAYERS);self.put(0x540c70,1)
        for address,value in TYPE_GLOBALS.items():self.put(address,value)
        for address,value in COMMANDS.items():self.put(address,value)
        self.put(POOL+50+10,c['sourceType'],1);self.put(POOL+50+0x22,c['sourceByte'],1);self.put(POOL+50+14,bits(12.,4));self.put(POOL+50+18,bits(14.,4));self.put(POOL+100+10,c['destType'],1)
        self.put(TYPES+c['sourceType']*500+0xec,c['sourceGenus']);self.put(PLAYERS+180+0x78,c['home'])
        mu.mem_write(0x5c94d0,struct.pack('<256I',*([c['local']]*256)));mu.mem_write(0x5c9cd0,struct.pack('<256I',*([c['global']]*256)));self.put(0x5c9cd0+123*4,c['vortex'])
        self.put(0x5ca0d0+165*4,9);mu.mem_write(0x52f7e0,bytes.fromhex('030405060708090a'))
        base=common.base_case();base.update(c);base['state'].update(stage=c['stage'],timer=c['timer'],lock=c['lock'])
        self.put(common.MISSION+0xc0,c['c0']);self.put(common.MISSION+0xc4,c['c4'])
        # 공통 실행기가 객체를0초기화한 뒤 첫 명령에서 입력 두DWORD/추가 사건을 설치한다.
        # 공통 실행기가 비운 두 필드는 첫 실제 명령 직전에 명시 입력으로 설치한다.
        self.first_fields=(c['c0'],c['c4'],c['arg1'],c['arg2']);common.ENTRIES['Frame']=common.TUTORIALS[c['tutorial']-1]
        self.active=True;result=super().run(base);self.active=False
        return [*result,bytes(mu.mem_read(0x52f7e0,8)).hex(),self.get(0x5ca0d0+165*4),self.case['selected'],self.get(common.MISSION+0xc0),self.get(common.MISSION+0xc4),self.get(0x540c70)]


def main():
    """기대값은 실제 PE에서만 생성하며 저장 후 현재 소스 SHA를 감사한다."""
    if sys.argv[1:]==['--verify']:
        r=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,sha in r['sha256'].items():
            if digest(ROOT/path)!=sha:raise RuntimeError('Tutorial3~6 SHA 불일치 '+path)
        with FIXTURE.open(encoding='utf-8') as stream:rows=list(csv.reader(stream,delimiter='\t'))[1:]
        if len(rows)!=r['native_unique_inputs'] or len({row[0] for row in rows})!=len(rows):raise RuntimeError('Tutorial3~6 고유입력 오류')
        print('Tutorial3~6 native/SHA verified:',len(rows));return
    if sys.argv[1:]:raise SystemExit('사용법: decomp_tutorial_later.py [--verify]')
    oracle=Oracle();rows=[]
    for c in cases():rows.append([','.join(str(c[k]&0xffffffffffffffff) for k in KEYS),*oracle.run(c)])
    with FIXTURE.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['input','gate','depth','trace','result','options','daisAllowed','selected','c0','c4','owner']);writer.writerows(rows)
    sources=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_tutorial_later.py','tools/decomp_missionframe_oracle.py','cpppj/src/o/RawTutorialLater.h','cpppj/src/o/RawTutorialLater.cpp','cpppj/tests/TutorialLaterTests.cpp','cpppj/tests/fixtures/tutorial-later-x86.tsv']
    r={'host':common.socket.gethostname(),'date':'2026-10-15','status':'native-observed-product-unbuilt','native_unique_inputs':len(rows),'normal_returns':oracle.returns,'native_entries':dict(oracle.calls),'boundaries':dict(oracle.boundaries),'os_gui_runs':0,'limits':['실제 Tutorial3~6/Gate Advance/Pause/TotalMade/Genus 몸체를 실행했다. 생산/Graph/지역/포획/선택/Tell은 명시 외부 경계다.','실제 게임/건설/AI전략/자연완주/CD는 이번 범위가 아니다.'],'sha256':{p:digest(ROOT/p) for p in sources if (ROOT/p).exists()}}
    REPORT.write_text(json.dumps(r,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('Tutorial3~6 native:',len(rows),flush=True)


if __name__=='__main__':main()
