#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 일반 입력의 SaveGoMain·현재 저장 부모·next3 쓰기를 게임/OS 없이 독립 관측한다."""
import argparse
import csv
import hashlib
import io
import itertools
import json
from pathlib import Path
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]  # 보호 PE와 기존 정밀 내보내기의 읽기 전용 기준이다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
sys.path.insert(0,str(ROOT/'tools'))
import decomp_production_shortcuts as gate
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW

OWNER,BATTLE,FORT,HEADERS,NEXT=0x540cac,0x594fbc,0x594fb8,0x5c89f0,0x5c8878  # 같은 원문 현재 DWORD 주소다.
ENTRIES=(0x4bfb50,0x4b3540)  # 저장 선택 부모와 실제 next3 writer는 대체하지 않는다.
FIXTURE=ROOT/'cpppj/tests/fixtures/editor-save-go-main-x86.tsv'  # 기존 자료를 덮지 않는 새 관측이다.
REPORT=ROOT/'cpppj/editor-save-go-main-native-20261026.json'  # 실제 코드·관측·도구 식별자를 기록한다.
SCHEMA=('name','control','event','first','second','owner','battle','fort','headers','next','mutation','stop','completed','ownerOut','battleOut','fortOut','headersOut','nextOut','trace','stackDelta','instructions')  # 앞12열은 명시 입력이고 뒤9열은 CPU 관측이다.

class ExplicitStop(Exception):
    """선택된 실제 저장 하위에서의 명시 중단을 정상 복귀와 구별한다."""

class SaveGoMainOracle(gate.Oracle):
    """기존 Guide/Gump/일반 입력 관문과 두 작은 부모를 실제 PE 기계어로 실행한다."""

    def __init__(self):
        """저장 큰 하위 두 개만 명시 장치로 대체하고 나머지는 기존 허용 코드로 제한한다."""
        super().__init__()
        self.body=[]  # 현재 저장 부모의 실제 조회·선택·next 쓰기 순서다.
        self.recording=False  # 004bfb50 안의 직접 조회만 별도 본문 trace로 분리한다.
        self.stops=set()  # 입력으로 지정한 저장 하위 중단 지점이다.
        self.mutations=[]  # 입력으로 지정한 하위 callback 복귀 전 현재 DWORD 변경이다.
        self.calls={}  # 같은 하위의1기반 호출 번호다.
        self.extra=[]  # functions.tsv가 정한 두 부모의 실제 불연속 범위다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as source:
            facts={int(row['entry'],16):row for row in csv.DictReader(source,delimiter='\t')}
        for address in ENTRIES:  # 원문 두 부모의 코드 범위를 추정 확장하지 않는다.
            for part in facts[address]['ranges'].split(';'):  # 끝 주소는 inclusive다.
                low,high=(int(value,16) for value in part.split('-'));self.extra.append((low,high+1))
        self.original.update({low:bytes(self.mu.mem_read(low,high-low)) for low,high in self.extra})
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def read(self,mu,access,address,size,value,data):
        """분기 계산을 만들지 않고 실제 CPU가 읽은 현재 DWORD와 슬롯을 기록한다."""
        if not self.recording or size!=4: return
        names={OWNER:'owner',BATTLE:'battle',FORT:'fort'}  # 세 실제 전역의 관측 이름이다.
        if address in names: self.body.append(f'{names[address]}:{self.number(address)}')
        elif HEADERS<=address<HEADERS+180 and (address-HEADERS)%20==0:
            self.body.append(f'manager:{(address-HEADERS)//20}:{self.number(address)}')

    def guard(self,mu,access,address,size,value,data):
        """CPU 쓰기는 기존 스택/큐와 정확한 next DWORD 하나만 허용한다."""
        if address==NEXT and size==4:
            self.writes['next']+=1;self.body.append(f'next:{value}');return
        return super().guard(mu,access,address,size,value,data)

    def instruction(self,mu,address,size,data):
        """전체 입력 관문·두 작은 부모는 그대로 실행하고 큰 snapshot 하위만 기록한다."""
        if address==0x4bfb50: self.recording=True
        if address==0x4d816c: self.recording=False
        if address in (0x4bf880,0x4bf9a0):
            kind='battle' if address==0x4bf880 else 'fort'
            pointer=mu.reg_read(UC_X86_REG_ECX)  # 원문이 재조회한 실제 this다.
            if not HEADERS<=pointer<HEADERS+180 or (pointer-HEADERS)%20: raise RuntimeError('원문 snapshot this 범위')
            self.body.append(f'{kind}Save:{(pointer-HEADERS)//20}')
            self.calls[kind]=self.calls.get(kind,0)+1;trigger=kind+str(self.calls[kind])
            for effect,target,value in self.mutations:  # 정상 원문 복귀 전에 입력의 현재 메모리 변경만 수행한다.
                if effect==trigger: self.number(target,value)
            if trigger in self.stops: raise ExplicitStop()
            self.returned();return
        if any(low<=address and address+size<=high for low,high in self.extra):
            self.instructions+=1;self.visited.add(address);return
        return super().instruction(mu,address,size,data)

    def sample(self,case,control):
        """명시 사건/현재 Template9/모드/next를 설치하고 원문 관문부터 정상 끝까지 실행한다."""
        self.case={'guideResult':0,'gumpResult':0};self.trace=[];self.instructions=0;self.completed=False
        self.body=[];self.recording=False;self.calls={};self.stops=set() if case['stop']=='-' else set(case['stop'].split(';'))
        self.mutations=[] if case['mutation']=='-' else [(effect,int(target),int(value)) for effect,target,value in (entry.split('/') for entry in case['mutation'].split(';'))]
        self.number(gate.GUIDE+0x800,gate.MISSION);self.number(gate.MISSION,gate.TABLE);self.number(gate.TABLE+0x24,gate.VIRTUAL_INPUT)
        self.number(gate.CURSOR,1);self.number(gate.PAUSE,0);self.number(gate.CONSOLE,0);self.number(gate.READ,0);self.number(gate.WRITE,0)
        for address,key in ((OWNER,'owner'),(BATTLE,'battle'),(FORT,'fort'),(NEXT,'next')): self.number(address,case[key])  # 현재 명시 입력 DWORD만 쓴다.
        self.mu.mem_write(HEADERS,bytes.fromhex(case['headers']));self.mu.mem_write(gate.STACK-0x4000,bytes(0x8000))
        self.mu.mem_write(gate.STACK+0x14,struct.pack('<3I',case['event'],case['first'],case['second']))
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP): self.mu.reg_write(register,0)  # 이전 호출의 레지스터를 입력으로 섞지 않는다.
        self.mu.reg_write(UC_X86_REG_ESP,gate.STACK);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        try: self.mu.emu_start(gate.ENTRY,0,count=300000,timeout=500000)
        except ExplicitStop: pass
        if not self.completed and not self.stops: raise RuntimeError('SaveGoMain 원문 입력 끝 미도달')
        delta=self.mu.reg_read(UC_X86_REG_ESP)-gate.STACK  # 정상 분기만 기존 입력 프레임의0균형을 요구한다.
        if self.completed and delta!=0: raise RuntimeError('SaveGoMain 원문 정상 스택 복구 불일치')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW): raise RuntimeError('정수 원문 분기의 x87 상태 변경')
        for low,expected in self.original.items():  # 원문 코드 바이트가 실행 중 변하지 않았는지 매 입력 확인한다.
            if bytes(self.mu.mem_read(low,len(expected)))!=expected: raise RuntimeError('SaveGoMain 원문 코드 변경')
        return dict(completed=int(self.completed),ownerOut=self.number(OWNER),battleOut=self.number(BATTLE),fortOut=self.number(FORT),headersOut=bytes(self.mu.mem_read(HEADERS,180)).hex(),nextOut=self.number(NEXT),trace=';'.join(self.body) or '-',stackDelta=delta,instructions=self.instructions)

def inputs(command):
    """기대 결과를 계산하지 않고 분기·인자 불일치·현재 조회 변경·부분 실패 입력만 만든다."""
    rows=[];headers=bytearray((index*7+3)&255 for index in range(180))  # manager 외 주변16BYTE도 보존을 확인할 입력이다.
    for owner in range(9): struct.pack_into('<I',headers,owner*20,100+owner)  # 실제 첫 DWORD는 서로 다르다.
    def add(name,**changes):
        """기본 입력에서 명시한 필드만 바꾼다."""
        case=dict(name=name,event=command,first=0,second=1,owner=1,battle=0,fort=1,headers=headers.hex(),next=77,mutation='-',stop='-');case.update(changes);rows.append(case)
    for first,second,mode,flag in itertools.product((0,1),(0,1,0xffffffff),((0,0),(0,1),(1,0),(1,1)),(0,0x80000000,0x38000000)):
        add(f'plain-{first}-{second}-{mode[0]}-{mode[1]}-{flag}',first=first,second=second,battle=mode[0],fort=mode[1],event=command|flag)
    for owner in (0,2,8): add(f'owner-{owner}',owner=owner)
    blank=bytearray(headers);struct.pack_into('<I',blank,20,0);add('manager-zero',headers=blank.hex(),battle=1,fort=1)
    add('battle-changes-owner',battle=1,mutation=f'battle1/{OWNER}/2')
    add('battle-enables-fort',battle=1,fort=0,mutation=f'battle1/{FORT}/1')
    add('battle-disables-fort',battle=1,fort=1,mutation=f'battle1/{FORT}/0')
    add('battle-clears-next-then-fort',battle=1,mutation=f'battle1/{NEXT}/0;fort1/{NEXT}/88')
    add('battle-clears-current-manager',battle=1,mutation=f'battle1/{HEADERS+20}/0')
    add('battle-stop',battle=1,stop='battle1')
    add('fort-stop',battle=1,stop='fort1')
    return rows

def main():
    """신규 fixture만 생성하고 --verify에서는 기존 바이트를 쓰지 않고 재현 대조한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');options=parser.parse_args()
    if FIXTURE.exists()!=options.verify: raise RuntimeError('생성/재현 모드와 기존 fixture 존재가 맞지 않습니다')
    oracle=SaveGoMainOracle();command=oracle.number(0x542a20);rows=[]
    for case in inputs(command):  # 원문이 결과를 만들며 Python은 기대 분기를 만들지 않는다.
        for control in gate.CONTROLS: rows.append(dict(case,control=control,**oracle.sample(case,control)))  # 두 x87 정밀도에서 같은 정수 분기를 독립 관측한다.
    output=io.StringIO(newline='');writer=csv.DictWriter(output,SCHEMA,delimiter='\t',lineterminator='\n');writer.writeheader();writer.writerows(rows);encoded=output.getvalue().encode('utf-8')
    if options.verify:
        if FIXTURE.read_bytes()!=encoded: raise RuntimeError('SaveGoMain 기존 fixture 재현 바이트 불일치')
    else: FIXTURE.write_bytes(encoded)
    evidence=[dict(first=f'{low:08x}',lastExclusive=f'{low+len(raw):08x}',sha256=hashlib.sha256(raw).hexdigest(),bytes=len(raw)) for low,raw in sorted(oracle.original.items())]  # 실제 실행 허용 범위의 최초 PE 코드 식별자다.
    report=dict(host='HJOW-Athlon',lastFullDecompilation='HJOW-Athlon / 2026-10-23',originalGameExecuted=False,command=command,rows=len(rows),normalReturns=sum(row['completed'] for row in rows),partialStops=sum(not row['completed'] for row in rows),binarySha256=hashlib.sha256(gate.PE_PATH.read_bytes()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),gateToolSha256=hashlib.sha256(Path(gate.__file__).read_bytes()).hexdigest(),fixtureSha256=hashlib.sha256(encoded).hexdigest(),codeEvidence=evidence,codeImageUnchanged=True,replaced=['Guide virtual input result','Gump input result','004bf880 Battle snapshot child','004bf9a0 Fort snapshot child'],actualSmallParents=['004d815b SaveGoMain branch','004bfb50 current snapshot selection','004b3540 next3 store'],verifiedExistingFixture=options.verify)
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False))

if __name__=='__main__': main()  # 보호 PE의 허용된 정수 몸체만 격리 실행한다.
