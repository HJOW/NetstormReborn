#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Test Battle 일반 입력과 실제 Guide Begin/Close/+64/next1을 OS 없이 독립 관측한다."""
import argparse
import csv
import hashlib
import io
import itertools
import json
from pathlib import Path
import socket
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]  # 보호 입력과 신규 증거의 저장소 기준이다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
sys.path.insert(0,str(ROOT/'tools'))
import decomp_production_shortcuts as gate
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW

CURRENT,MODE,NEXT=0x55b490,0x5cae84,0x5c8878  # 실제 현재 이름 포인터/선택 모드/다음 State다.
NAMES=(gate.HEAP+0x400,gate.HEAP+0x600)  # 각256BYTE 이름의 수명을 분리한다.
SECOND,NEW=gate.MISSION+0x200,gate.HEAP+0x3000  # 명시 해제 재진입 객체와 새 공장 객체다.
ENTRIES=(0x463e40,0x463b70,0x463b20,0x4639d0,0x4437f0,0x4b3560)  # 빈 현재의 Guide 입력 반환과 교체·해제·필드 쓰기·빈 종료·next1 부모다.
FIXTURE=ROOT/'cpppj/tests/fixtures/editor-test-battle-x86.tsv'  # 최초 결과를 덮지 않는 원문 관측이다.
REPORT=ROOT/'cpppj/editor-test-battle-native-20261027.json'  # 게임 실행과 격리 기계어 실행을 구별한다.
SCHEMA=('name','control','event','first','second','active','pointer','mode','next','guideResult','gumpResult','reentrant','mutation','stop','completed','activeOut','pointerOut','name0Out','name1Out','guideOut','pendingOut','newModeOut','newDemoOut','modeOut','nextOut','trace','stackDelta','instructions')  # 앞14열은 명시 입력이고 뒤14열만 CPU 관측이다.

class ExplicitStop(Exception):
    """명시 큰 하위의 부분 실패를 정상 반환과 구별한다."""

class TestBattleOracle(gate.Oracle):
    """실제 입력 관문과 작은 부모를 실행하며 저장/반납/공장/장치는 명시 공급한다."""
    def __init__(self):
        """functions.tsv의 실제 범위만 추가하며 큰 원문 함수를 임의로 허용하지 않는다."""
        super().__init__()
        self.extra=[]  # 작은 부모의 불연속 기계어 범위다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for entry in ENTRIES:  # 실제 함수 표의 끝 주소는 inclusive다.
            for part in facts[entry]['ranges'].split(';'):  # 원문 코드 밖 실행은 상위 guard가 거부한다.
                low,high=(int(value,16) for value in part.split('-'));self.extra.append((low,high+1))
        self.original.update({low:bytes(self.mu.mem_read(low,high-low)) for low,high in self.extra})

    def identifier(self,pointer):
        """격리된 현재 포인터만 번호로 기록하며 새로운 미명시 주소를 허용하지 않는다."""
        return (0,gate.MISSION,SECOND,NEW).index(pointer)

    def string(self,address):
        """실제 현재 주소의 첫 NUL 앞 바이트만 읽는다."""
        for length in range(2048):  # Guide의 물리 이름 범위를 넘는 원문 읽기를 거부한다.
            if not self.number(address+length,width=1):return bytes(self.mu.mem_read(address,length)).decode('ascii')
        raise RuntimeError('Test Battle 원문 이름 NUL 누락')

    def effect(self,name,text=None):
        """관측 순서를 기록하고 입력으로 지정한 변경/중단만 해당 외부 호출에 적용한다."""
        self.body.append(text if text is not None else name)
        if self.case['mutation']!='-':
            for item in self.case['mutation'].split(';'):  # 기대 관측을 변경 입력으로 사용하지 않는다.
                trigger,field,value=item.split('/')
                if trigger!=name:continue
                if field in ('name0','name1'):
                    self.mu.mem_write(NAMES[int(field[-1])],value.encode('ascii')+b'\0')
                elif field=='pointer':self.number(CURRENT,NAMES[int(value)])
                elif field=='mode':self.number(MODE,int(value))
                elif field=='next':self.number(NEXT,int(value))
                elif field=='active':self.number(gate.GUIDE+0x800,(0,gate.MISSION,SECOND,NEW)[int(value)])
                else:raise RuntimeError('미명시 Test Battle 변경 필드')
        if self.case['stop']==name:raise ExplicitStop()

    def guard(self,mu,access,address,size,value,data):
        """원문은 Guide 이름/현재, 새 미션+64와 두 예약 DWORD만 추가로 쓸 수 있다."""
        if gate.GUIDE<=address and address+size<=gate.GUIDE+0x804:return
        if address in (NEW+0x64,MODE,NEXT) and size==4:return
        return super().guard(mu,access,address,size,value,data)

    def instruction(self,mu,address,size,data):
        """큰 외부 하위만 대체하고 Begin의 늦은 이름 복사/Close 조건/+64는 실제 CPU에 맡긴다."""
        if address==0x4bfb50:self.effect('save');self.returned();return
        if address==0x4437f0:self.effect('finish','finish:'+str(self.identifier(mu.reg_read(UC_X86_REG_ECX))))
        if address==0x463ab0:
            active=self.identifier(self.number(gate.GUIDE+0x800));self.number(gate.GUIDE+0x800,SECOND if self.case['reentrant'] else 0);self.mu.mem_write(gate.GUIDE,b'\0')
            self.effect('release','release:'+str(active));self.returned();return
        if address==0x482fb0:
            name=self.string(self.number(mu.reg_read(UC_X86_REG_ESP)+4));self.number(NEW+0x64,0);self.number(NEW+0x68,0)
            self.effect('create','create:'+name);self.returned(NEW);return
        if address==0x460bb0:
            self.pending=self.string(self.number(mu.reg_read(UC_X86_REG_ESP)+4));self.effect('pending','pending:'+self.pending);self.returned();return
        if address==0x4b3560:self.effect('edit','edit:'+str(self.number(MODE)))
        if any(low<=address and address+size<=high for low,high in self.extra):
            self.instructions+=1;self.visited.add(address);return
        return super().instruction(mu,address,size,data)

    def sample(self,case,control):
        """사건/소비 결과/현재 이름과 부분 실패를 명시하고 입력의 실제 종료까지 실행한다."""
        self.case=case;self.body=[];self.trace=[];self.pending='old';self.instructions=0;self.completed=False
        self.mu.mem_write(gate.STACK-0x4000,bytes(0x8000));self.mu.mem_write(gate.STACK+0x14,struct.pack('<3I',case['event'],case['first'],case['second']))
        self.mu.mem_write(gate.GUIDE,b'OldGuide\0');self.number(gate.GUIDE+0x800,(0,gate.MISSION,SECOND,NEW)[case['active']])
        for pointer in (gate.MISSION,SECOND,NEW):self.number(pointer,gate.TABLE)  # 세 현재 객체는 같은 명시 가상 입력/빈 종료를 가진다.
        self.number(gate.TABLE+0x1c,0x4437f0);self.number(gate.TABLE+0x24,gate.VIRTUAL_INPUT)
        self.number(NEW+0x64,7);self.number(NEW+0x68,3)
        for pointer,text in zip(NAMES,('OriginalFort','AlternateFort')):self.mu.mem_write(pointer,text.encode('ascii')+b'\0')  # 매 입력 독립 원문 주소다.
        for address,value in ((CURRENT,NAMES[case['pointer']]),(MODE,case['mode']),(NEXT,case['next']),(0x594fbc,0),(gate.CURSOR,1),(gate.PAUSE,0),(gate.CONSOLE,0),(gate.READ,0),(gate.WRITE,0)):
            self.number(address,value)  # 실제 Battle14 하위는 기존 독립 fixture 범위이며 이번에는 Battle0 next1을 실행한다.
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)  # 잔여 레지스터를 사건 입력에 섞지 않는다.
        self.mu.reg_write(UC_X86_REG_ESP,gate.STACK);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        try:self.mu.emu_start(gate.ENTRY,0,count=300000,timeout=500000)
        except ExplicitStop:pass
        if not self.completed and case['stop']=='-':raise RuntimeError('Test Battle 실제 입력 끝 미도달')
        delta=self.mu.reg_read(UC_X86_REG_ESP)-gate.STACK
        if self.completed and delta:raise RuntimeError('Test Battle 정상 스택 복구 불일치')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW):raise RuntimeError('Test Battle 정수 경로 x87 변경')
        for low,raw in self.original.items():  # 모든 입력에서 실제 원문 코드 불변을 확인한다.
            if bytes(self.mu.mem_read(low,len(raw)))!=raw:raise RuntimeError('Test Battle 보호 원문 코드 변경')
        return dict(completed=int(self.completed),activeOut=self.identifier(self.number(gate.GUIDE+0x800)),pointerOut=NAMES.index(self.number(CURRENT)),name0Out=self.string(NAMES[0]),name1Out=self.string(NAMES[1]),guideOut=self.string(gate.GUIDE),pendingOut=self.pending,newModeOut=self.number(NEW+0x64),newDemoOut=self.number(NEW+0x68),modeOut=self.number(MODE),nextOut=self.number(NEXT),trace=';'.join(self.body) or '-',stackDelta=delta,instructions=self.instructions)

def cases(command):
    """분기·인자 불일치·Guide/Gump 소비·늦은 주소/내용 변경·부분 중단 입력만 생성한다."""
    rows=[]  # 기대 출력은 생성하지 않는다.
    def add(name,**changes):
        """독립 기본 상태에 명시한 입력만 덮어쓴다."""
        row=dict(name=name,event=command,first=91,second=0,active=1,pointer=0,mode=77,next=88,guideResult=0,gumpResult=0,reentrant=0,mutation='-',stop='-');row.update(changes);rows.append(row)
    for second,active,flag,reentrant in itertools.product((0,1,2,0xffffffff),(0,1),(0,0x3c000000,0x40000000,0x80000000),(0,1)):
        add(f'plain-{second}-{active}-{flag}-{reentrant}',second=second,active=active,event=command|flag,reentrant=reentrant,first=0 if second else 1)
    for source in ('guide','gump'):
        for second in (0,1):add(f'consumed-{source}-{second}',second=second,**{source+'Result':1})
    for trigger in ('save','finish','release','create','pending','edit'):
        second=1 if trigger in ('pending','edit') else 0
        add('stop-'+trigger,second=second,stop=trigger,mutation=trigger+'/next/99')
    for trigger in ('save','finish','release'):
        for text in ('X','ChangedMuchLongerFortName'):
            add(f'borrowed-{trigger}-{text}',mutation=f'{trigger}/name0/{text}')
    add('save-redirect',mutation='save/pointer/1');add('save-activates',active=0,mutation='save/active/2')
    add('finish-redirect-keeps-borrow',mutation='finish/pointer/1;release/name0/ChangedOriginal')
    add('release-redirect-keeps-borrow',mutation='release/pointer/1;release/name0/X')
    add('close-redirect',second=2,mutation='release/pointer/1')
    add('pending-changes-current',second=1,mutation='pending/name0/AfterPending;pending/mode/99')
    add('edit-overwrites-next',second=1,mutation='edit/next/99;edit/mode/33')
    return rows

def main():
    """최초 fixture만 생성하거나 모든 관측을 재실행해 기존 바이트와 대조한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');options=parser.parse_args()
    if FIXTURE.exists()!=options.verify:raise RuntimeError('기존 fixture 덮어쓰기 또는 재검증 입력 누락')
    oracle=TestBattleOracle();command=oracle.number(0x5429fc);inputs=cases(command);rows=[]
    for case in inputs:  # 최초 결과는 실제 CPU만 만든다.
        for control in gate.CONTROLS:rows.append(dict(case,control=control,**oracle.sample(case,control)))  # 두 x87 제어어도 별도 실행한다.
    stream=io.StringIO(newline='');writer=csv.DictWriter(stream,SCHEMA,delimiter='\t',lineterminator='\n');writer.writeheader();writer.writerows(rows);raw=stream.getvalue().encode('utf-8')
    if options.verify:
        if FIXTURE.read_bytes()!=raw:raise RuntimeError('Test Battle 최초 원문 관측 재현 불일치')
    else:FIXTURE.write_bytes(raw)
    report=dict(host=socket.gethostname(),lastFullDecompilation='HJOW-Athlon / 2026-10-23',gameExecuted=False,osExecuted=False,command=command,inputs=len(inputs),rows=len(rows),normalReturns=sum(row['completed'] for row in rows),partialStops=sum(not row['completed'] for row in rows),binarySha256=hashlib.sha256(gate.PE_PATH.read_bytes()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixtureSha256=hashlib.sha256(raw).hexdigest(),gateToolSha256=hashlib.sha256(Path(gate.__file__).read_bytes()).hexdigest(),codeImageUnchanged=True,codeEvidence=[dict(first=f'{low:08x}',bytes=len(value),sha256=hashlib.sha256(value).hexdigest()) for low,value in sorted(oracle.original.items())],replaced=['Guide virtual input result','Gump input result','004bfb50 snapshot','00463ab0 release and explicit reentry','00482fb0 mission factory','00460bb0 pending copy'],actual=['general input c2 branch','00463b70 Begin and late NUL copy','00463b20 conditional Close without finish','004639d0 current Mission +64 store','004437f0 empty finish','004b3560 next1/Battle0 branch'],limits=['actual Runtime/State7 and working-file namespace not connected by this fixture','Battle14 preservation remains covered by existing FortSelection fixture'],verifiedExistingFixture=options.verify)
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps({key:report[key] for key in ('inputs','rows','normalReturns','partialStops','fixtureSha256')}))

if __name__=='__main__':main()  # 실제 게임/창/네트워크를 생성하지 않는다.
