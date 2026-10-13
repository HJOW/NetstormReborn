#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""GunProcess의 두 PE 필드/상태/종속/notify/전송을 제한 x86으로 관찰한다. 게임/OS 실행은 없다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from decomp_owner_oracle import OwnerOracle, ROOT, TYPES, POOL, STACK, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 각 판본의 실제 함수/전역이다. 원본 바이너리에 쓰거나 게임을 띄우지 않는다.
SPEC={
 'originals':dict(entries=[0x46a1a0,0x46b830,0x46a290,0x46a960,0x4690f0,0x46a340,0x46bd20,0x468ed0,0x46a090,0x46a4b0],
  outer=0x50b668,inner=0x50b648,group=0x4ac950,valid=0x40ea40,anim=0x469070,fromform=0x4ae2d0,kill=0x41bf10,
  remove=0x4ae780,add=0x4b15f0,repop=0x41c010,range=0x4d5000,mode=0x4d4d00,find=0x46b6f0,
  random=0x455930,start=0x4689c0,attach=0x41bd20,stop=0x46a470,center=0x4adea0,shot=0x46b070,
  now=0x55b4d0,boss=0x540bc4,server=0x540bc0,fort=0x594fb8,fixed=0x5c85a4,disabled=0x5ca8ec,
  friendly=0x55a490,checking=0x5e4794,allies=0x595200,capacity=0x5c847c,
  wind=0x541288,thunder=0x541264,rain=0x54125c,process=0x5410f8,header=0x5409b4),
 'originalCD':dict(entries=[0x434090,0x4342f0,0x434720,0x434290,0x434cb0,0x434900,0x434790,0x433db0,0x436d90,0x434a70],
  outer=0x501118,inner=0x5010f8,group=0x4ac420,valid=None,anim=0x4349e0,fromform=0x4af4e0,kill=0x48f7b0,
  remove=0x4afa50,add=0x4afa10,repop=0x48f8d0,range=0x4433e0,mode=0x443110,find=0x435e10,
  random=0x48cc50,start=0x4a18f0,attach=0x48f590,stop=0x4349a0,center=0x4aef30,shot=0x434c30,
  now=0x5484a8,boss=0x540a2c,server=0x540a28,fort=0x540a1c,fixed=0x518904,disabled=0x511c8c,
  friendly=0x537ea0,checking=0x531454,allies=0x50f6e0,capacity=0x5395f4,
  wind=0x51cb74,thunder=0x51cb50,rain=0x51cb48,process=0x51c9e8,header=0x51a188),
}
# raw 객체/packet/난수 반환용 메모리다. 호스트 저장소와 무관하다.
OBJECT,PACKET,THUNK,VALUE=0x11318000,0x11319000,STOP+0x100,STOP+0x180
KEYS=('kind','stage','target','dirty','group','type','extra','parentstate','targetstate','boss','server','fort','fixed','disabled','friendly','allied','time','now','found','random','anim','exists','shoot','restart','event','value','force','include','parentarg')
FIXTURE=ROOT/'cpppj/tests/fixtures/gunprocess-x86.tsv'
REPORT=ROOT/'cpppj/recovery-gunprocess-evidence.json'


def bits(value):
    """double 값을 입력용 raw 비트로 바꾼다. 기대값은 계산하지 않는다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def inputs():
    """판단 결과를 모르는 독립 입력 격자로 원본의 각 분기를 방문한다."""
    base=dict(zip(KEYS,(1,0,60,0x20,1,128,0,0,0,1,1,0,0,0,0,0,bits(10),bits(10),0,0x3dcccccd,0,1,1,0,1,4,0,0,50)))
    result=[]
    # 11단계·표적/권한/시각/부모 차단의 대표 경계를 교차한다.
    for stage,target,boss,gate in itertools.product(range(11),(0,60),(0,1),range(5)):
        item=base.copy();item.update(stage=stage,target=target,boss=boss)
        if gate==1:item['time']=bits(10.25)
        if gate==2:item['extra']=0x20
        if gate==3:item['fixed']=1
        if gate==4:item['disabled']=1
        result.append(item)
    # 표적 무효/동맹·모드별 검색과 Archer/Cannon 회전 분기를 따로 방문한다.
    for targetstate,allied,friendly in itertools.product((0,1,2,3),(0,1),(0,1)):
        item=base.copy();item.update(stage=2,targetstate=targetstate,allied=allied,friendly=friendly);result.append(item)
    for group,typeid in ((0,71),(0,140),(0,148),(1,128),(1,129),(1,131)):
        item=base.copy();item.update(stage=2,group=group,type=typeid);result.append(item)
    for server,fort,friendly,found in itertools.product((0,1),(0,1),(0,1),(0,61)):
        item=base.copy();item.update(stage=1,target=0,boss=server,server=server,fort=fort,friendly=friendly,found=found);result.append(item)
    # Clear의 변경 전 전송 조건과 Set의 animation 현재 조회/Kernel 부재 분기를 방문한다.
    for kind,stage,target,restart,boss in itertools.product((2,),(0,1,3,6),(0,60),(0,1),(0,1)):
        item=base.copy();item.update(kind=kind,stage=stage,target=target,restart=restart,boss=boss);result.append(item)
    for target,anim,exists in itertools.product((0,60),(0,80),(0,1)):
        item=base.copy();item.update(kind=3,target=target,anim=anim,exists=exists);result.append(item)
    # 부착은 3/6만 바꾸며 notify는 표적0/삭제/부모 dead를 포함한다.
    for stage in range(11):
        item=base.copy();item.update(kind=4,stage=stage);result.append(item)
    for stage,event,value,target,boss,parentstate in itertools.product((1,3,6,9),(1,2,3),(4,7,10,60),(0,60),(0,1),(0,2)):
        # Animation 통지의 허용값 밖은 원본 assert 계약 위반이라 정상 관찰에 넣지 않는다.
        if event==1 and value==60:continue
        item=base.copy();item.update(kind=5,stage=stage,event=event,value=value,target=target,boss=boss,parentstate=parentstate);result.append(item)
    # 모든 dirty 하위 비트와 force/include/부모 여부를 방문해 stale packet도 기록한다.
    for kind,dirty,force,include,parentarg in itertools.product((7,8),range(32),(0,1),(0,1),(0,50)):
        item=base.copy();item.update(kind=kind,dirty=dirty,force=force,include=include,parentarg=parentarg);result.append(item)
    for group in (0,1):
        item=base.copy();item.update(kind=0,group=group,target=0);result.append(item)
    # 발사의 유효/무효 표적·실제 생성 경계 반환·차단·보스 권한·현재 시각을 방문한다.
    for kind,target,targetstate,shoot,boss,gate in itertools.product((6,),(0,60,128),(0,1,2,3),(0,1),(0,1),range(3)):
        item=base.copy();item.update(kind=kind,stage=7,target=target,targetstate=targetstate,shoot=shoot,boss=boss)
        if gate==1:item['extra']=0x20
        if gate==2:item['fixed']=1
        result.append(item)
    # 수신 observer는 parent 변경/dirty8/대기 단계가 모두 맞을 때만 현재 animation을 중지한다.
    for stage,dirty,parentarg,anim,exists in itertools.product(range(11),(0,8),(0,50),(0,80),(0,1)):
        item=base.copy();item.update(kind=9,stage=stage,dirty=dirty,parentarg=parentarg,anim=anim,exists=exists);result.append(item)
    # x87 비교 NaN 경계는 두 판본의 서로 다른 시간 조건을 그대로 관찰한다.
    for now,time in ((0x7ff8000000000000,bits(10)),(bits(10),0x7ff8000000000000),(bits(10),bits(-1))):
        item=base.copy();item.update(stage=4,boss=0,now=now,time=time);result.append(item)
    return result


class GunOracle(OwnerOracle):
    """실제 함수만 허용하고 아직 별도 body인 검색/애니메이션/감시/전송 경계를 관찰한다."""
    def __init__(self,edition):
        """독립 PE 매핑과 정확한 가상표를 확인한다."""
        super().__init__(edition);self.p=SPEC[edition];self.mu.mem_map(0,0x1000)
        # 기존 성공 export와 새 helper export를 각각 재사용한다.
        for name in ('gunprocess','gunprocess-helper'):
            self.exports.extend(ROOT/f'extracted/{name}/{edition}/{part}' for part in ('creation.c','functions.tsv'))
            with self.exports[-1].open(encoding='utf-8') as fp:
                # Ghidra가 내보낸 불연속 body만 허용한다.
                for row in csv.DictReader(fp,delimiter='\t'):
                    self.entries.add(int(row['entry'],16))
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        if self.read(self.p['inner']+0x18)!=self.p['entries'][1] or self.read(self.p['outer']+4)!=self.p['entries'][6]:raise RuntimeError('GunProcess 가상표 불일치')
        if edition=='originalCD':
            # CD Frame이 실제 호출하는 빈 가상 몸체의 단일 RET 바이트를 PE에서 확인한다.
            if self.read(0x4348f0,1)!=0xc3:raise RuntimeError('CD 빈 Frame hook 몸체 변화')
            self.allowed.append((0x4348f0,0x4348f1));self.entries.add(0x4348f0)
        self.mu.mem_write(THUNK,b'\xdd\x05'+struct.pack('<I',VALUE)+b'\xc3');self.allowed.append((THUNK,THUNK+7))
        self.stub_calls=collections.Counter();self.returns=0;self.events=[]

    def read(self,address,width=4):
        """가상 raw 값을 little endian으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def on_instruction(self,mu,address,size,data):
        """아직 상위 월드에 묶이지 않은 외부 효과를 명시 인자/시점으로 기록한다."""
        p=self.p;esp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX)
        args=[self.read(esp+4+4*i) for i in range(9)];ret=self.read(esp);purge=0;value=0
        if address==p['remove']:self.events.append(f'D:{args[0]}:{args[1]}')
        elif address==p['add']:self.events.append(f'A:{args[0]}')
        elif address==p['repop']:self.events.append('P')
        elif address==p['anim']:self.events.append('L:50');value=self.case['anim']
        elif address==p['fromform']:value=OBJECT+0x100 if self.case['exists'] else 0
        elif address==p['kill']:self.events.append(f'K:{self.case["anim"]}:{args[0]}');purge=4
        elif address==p['range']:self.events.append('R:50')
        elif address==p['mode']:self.events.append(f'M:{(this-POOL)//self.stride}:1');value=2
        elif address==p['center']:
            self.events.append(f'C:{(this-POOL)//self.stride}');mu.mem_write(args[0],struct.pack('<II',0x42500000,0x42540000));value=args[0];purge=4
        elif address==p['shot']:self.events.append('H:'+','.join(map(str,args)));value=self.case['shoot']
        elif address==p['find']:
            self.events.append(f'F:{args[0]}:{args[1]}:{args[2]}:2:1');value=self.case['found']
        elif address==p['random']:
            self.events.append('N:0:1045220557');mu.mem_write(VALUE,struct.pack('<d',struct.unpack('<f',struct.pack('<I',self.case['random']))[0]))
            mu.reg_write(UC_X86_REG_EIP,THUNK);self.stub_calls[f'{address:08x}']+=1;return
        elif address==p['start']:self.events.append(f'S:{args[0]}:{args[1]}:{args[2]}');value=81
        elif address==p['attach']:
            self.events.append(f'T:{args[0]}:{args[1]}:{args[2]}:{args[3]}');purge=16
            mu.mem_write(this+4,struct.pack('<II',70,args[1]))
        else:super().on_instruction(mu,address,size,data);return
        self.stub_calls[f'{address:08x}']+=1
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run_body(self,case):
        """입력 raw/padding/packet을 준비하고 독립 원본 결과 전체를 수집한다."""
        self.case=case;self.events=[];p=self.p;mu=self.mu;patch=self.edition=='originals'
        mu.mem_write(POOL,bytes(128*self.stride));parent=bytearray(self.stride);parent[10]=case['type'];parent[11]=case['parentstate']
        parent[34 if patch else 32]=1;parent[40 if patch else 35]=case['extra'];struct.pack_into('<H',parent,6,70)
        struct.pack_into('<II',parent,14,0x42480000,0x42500000);mu.mem_write(self.slot(50),bytes(parent))
        target=bytearray(self.stride);target[11]=case['targetstate'];target[34 if patch else 32]=2;mu.mem_write(self.slot(60),bytes(target))
        mu.mem_write(TYPES+case['type']*self.type_stride,bytes(self.type_stride));mu.mem_write(TYPES+case['type']*self.type_stride+0x9c,struct.pack('<I',case['group']))
        mu.mem_write(TYPES+case['type']*self.type_stride+0x4c,struct.pack('<f',2.25))
        fields=bytearray((i*13+7)&255 for i in range(0x38));struct.pack_into('<I',fields,0,p['outer']);fields[8]=case['dirty']
        struct.pack_into('<QH',fields,16,case['time'],case['target']);struct.pack_into('<II',fields,28,case['stage'],case['group']);struct.pack_into('<III',fields,40,p['inner'],70,50)
        mu.mem_write(OBJECT,bytes(fields));mu.mem_write(PACKET,bytes((i*7+3)&255 for i in range(64)))
        mu.mem_write(p['now'],struct.pack('<Q',case['now']))
        for name in ('boss','server','fort','fixed','disabled','friendly'):mu.mem_write(p[name],struct.pack('<I',case[name]))
        mu.mem_write(p['checking'],bytes(4));mu.mem_write(p['capacity'],struct.pack('<I',128));mu.mem_write(p['process'],struct.pack('<I',34))
        for name,value in (('wind',140),('thunder',131),('rain',129)):mu.mem_write(p[name],struct.pack('<I',value))
        mu.mem_write(p['allies'],bytes(9*9*4));mu.mem_write(p['allies']+(1*9+2)*4,struct.pack('<I',case['allied']))
        self.write_ranges=[(0,4),(OBJECT,OBJECT+0x200),(PACKET,PACKET+64),(VALUE,VALUE+8)]
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(register,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,0x27f);mu.reg_write(UC_X86_REG_FPSW,0)
        kind=case['kind'];inner=kind in (1,4,5,8);mu.reg_write(UC_X86_REG_ECX,OBJECT+(40 if inner else 0));mu.reg_write(UC_X86_REG_ESP,STACK)
        args={0:[50],1:[],2:[case['restart']],3:[61],4:[],5:[case['event'],0x12345678,case['value']],
              6:[],7:[PACKET,70,case['parentarg'],case['force']],8:[PACKET,case['include'],case['force']],9:[case['dirty'],case['parentarg'],0x12345678]}[kind]
        mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.mem_write(0,bytes(4))
        mu.emu_start(p['entries'][kind],STOP,count=40000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+len(args)*4:raise RuntimeError(f'GunProcess 반환/스택 오류: {self.edition} {case}')
        if mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('GunProcess x87 반환 스택 오류')
        self.returns+=1
        return [';'.join(self.events),bytes(mu.mem_read(OBJECT,0x38)).hex(),bytes(mu.mem_read(PACKET,64)).hex(),self.read(PACKET+2,2) if kind in (7,8) else 0]


def generate(smoke):
    """독립 관찰과 각 사용 파일의 SHA를 저장한다."""
    cases=inputs();cases=cases[::71] if smoke else cases;rows=[];editions={};files=[__file__,FIXTURE,ROOT/'tools/ghidra/gunprocess-functions.json',ROOT/'tools/ghidra/gunprocess-helper-functions.json']
    for edition in SPEC:
        oracle=GunOracle(edition)
        # 정상 반환을 별도 한 행으로 기록하고 같은 입력의 반복 실행을 새 관찰로 세지 않는다.
        for case in cases:
            try:output=oracle.run_body(case)
            except Exception as error:raise RuntimeError(f'{edition} case{case}: {error}') from error
            rows.append([edition,*[case[key] for key in KEYS],*output])
        editions[edition]=dict(cases=oracle.returns,returns=oracle.returns,native_calls=dict(oracle.native_calls),stub_calls=dict(oracle.stub_calls),assertions=oracle.assertions,instructions=oracle.instructions)
        files.extend(oracle.exports);files.append(ROOT/oracle.spec['binary'])
    FIXTURE.write_text('# 두 PE의 GunProcess 독립 관찰. 실제 검색·애니메이션·투사체 생성은 명시 외부 경계다.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8')
    REPORT.write_text(json.dumps(dict(schema=1,total=len(rows),host='VM-W11-CODEX',editions=editions,files={str(path.relative_to(ROOT)):digest(path) for path in map(__import__('pathlib').Path,files)},limitations=['실제 search/animation 생성/감시/전송/center/shot은 명시 외부 경계다.','명시 외부 효과는 관찰 사건으로 반환하며 원본 몸체 복원 완료로 세지 않는다.'],os_calls=0),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(total=len(rows),editions=editions),ensure_ascii=False))


def verify():
    """저장된 관찰 SHA·독립 입력·정상 반환을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    if len(rows)!=report['total']:raise RuntimeError('GunProcess 행 수 오류')
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError('GunProcess SHA 변화: '+name)
    expected=inputs()
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if item['assertions'] or item['returns']!=len(selected) or [[str(case[key]) for key in KEYS] for case in expected]!=[row[1:1+len(KEYS)] for row in selected]:raise RuntimeError('GunProcess 입력/반환 감사 오류')
    print('GunProcess SHA/입력/정상 반환 감사 통과:',len(rows))


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)
