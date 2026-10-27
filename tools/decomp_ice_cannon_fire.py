#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78 PE의 아이스80BYTE 생성자/가상 단계/공통 Frame을 독립 x86으로 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_cannon_fire import Oracle as BaseOracle,letters
from decomp_crossbow_animation import ROOT,OBJECT,ALLOC,VTABLE,PARENT_QUERY,OUTPUT,TYPES,STACK,STOP,POOL
from decomp_owner_oracle import digest
from decomp_gunanimation_oracle import UNPOP,REPOP
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG)

KEYS=('kind','frame','due','targetstate','hp','mode','argument','active','edge','may','mutation')  # 원문 출력과 구별한 독립 입력 필드다.
FIXTURE=ROOT/'cpppj/tests/fixtures/ice-cannon-fire-x86.tsv'  # 기존 검사 자료는 수정하지 않는다.
REPORT=ROOT/'cpppj/ice-cannon-fire-native-20261027.json'  # 마지막 정밀 수행 PC와 실제 근거 해시다.
CONTEXT=tuple(range(201,209))  # 실제 문맥 생성 하위에 주는8DWORD 명시 외부 입력이다.

def sample(**values):
    """기대 결과를 계산하지 않고 독립 raw 입력의 기본값만 만든다."""
    row=dict(zip(KEYS,(0,0,1,0,1,0,0,1,2,1,0)));row.update(values);return row

def inputs():
    """전체 물리 프레임과 due/생존/냉각 경계/표적 결과 및 효과 재진입을 방문한다."""
    rows=[sample(frame=f,due=d,targetstate=t,mode=m,active=a,edge=e,may=v) for f,d,t,m,a,e,v in itertools.product(range(32),(0,1),(0,2),(0,3),(0,1),(0,1,2),(0,1))]
    rows += [sample(kind=2,frame=f,argument=n,mode=m,active=a,edge=e,may=v) for f,n,m,a,e,v in itertools.product((0,7,31),(-1,0,7,31),(0,3),(0,1,256),(0,1,2),(0,1))]
    rows += [sample(kind=1)]
    rows += [sample(kind=3,targetstate=d,hp=h) for d,h in itertools.product(range(4),(0,1))]
    rows += [sample(frame=f,mutation=m,edge=e) for f,m,e in itertools.product((0,6,7,31),range(1,10),(1,2))]
    return rows

class Oracle(BaseOracle):
    """검토한 아이스 함수 명령만 추가하고 월드/표시/문맥 하위는 명시 입력으로 둔다."""
    def __init__(self,control):
        """기존 실제 공통 Fire와 새 생성자·미소유 Step·가상 getter 범위를 허용한다."""
        super().__init__(control)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for part in facts[0x4685e0]['ranges'].split(';'):  # 정밀 소유 범위의 실제 ctor만 실행한다.
            low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.allowed.extend(((0x468630,0x468637),(0x468650,0x468656),(0x468660,0x468791))) # 마지막 RET4의3BYTE까지 포함하며 뒤 INT3는 제외한다.
        assert self.read(0x50b3f0+0x18)==0x467330 and self.read(0x50b3f0+0x48)==0x468660
        assert struct.unpack('<f',self.mu.mem_read(0x507828,4))[0]==struct.unpack('<f',struct.pack('<f',.08))[0]

    def mutate(self,where):
        """외부 효과 이후의 현재 필드/시각/가상 getter 변경만 공급한다."""
        super().mutate(where)
        if where=='Point' and self.case['mutation']==5:self.mu.mem_write(0x541260,struct.pack('<I',133))

    def on_instruction(self,mu,address,size,data):
        """냉각 표시·현재 문맥·MayTarget만 외부 경계이며 분기는 실제 CPU가 수행한다."""
        if address in (0x4a8770,0x45ae60,0x46af80):
            esp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(esp)
            if address==0x4a8770:
                self.events.append('E:'+str(self.read(esp+4)))
                if self.case['mutation']==7:mu.mem_write(self.p['now'],struct.pack('<d',100));mu.mem_write(OBJECT+8,struct.pack('<I',51))
                advance=4
            elif address==0x45ae60:
                self.events.append('X:'+str(self.read(esp+4)));destination=mu.reg_read(UC_X86_REG_ECX);mu.mem_write(destination,struct.pack('<8I',*CONTEXT));mu.reg_write(UC_X86_REG_EAX,destination)
                if self.case['mutation']==8:mu.mem_write(OBJECT+8,struct.pack('<I',51));mu.mem_write(OBJECT+44,struct.pack('<H',61))
                advance=8
            else:
                self.events.append('A:'+','.join(str(self.read(esp+4+i*4)) for i in range(9)));mu.reg_write(UC_X86_REG_EAX,self.case['may'])
                if self.case['mutation']==9:mu.mem_write(OBJECT+64,bytes(4))
                advance=4
            mu.reg_write(UC_X86_REG_ESP,esp+advance);mu.reg_write(UC_X86_REG_EIP,ret);return
        super().on_instruction(mu,address,size,data)

    def run_body(self,case):
        """실제 반환·80BYTE 전체/네 슬롯/효과/ABI와 쓰기 guard를 관찰한다."""
        self.case=dict(case,direction=0,growl=5,roar=1);case=self.case;self.events=[];mu=self.mu;kind=case['kind'];self.point=(60,50)
        fields=bytearray((i*13+7)&255 for i in range(80));struct.pack_into('<III',fields,0,0x50b3f0,81,50);struct.pack_into('<ffff',fields,12,60,60,59.5,59.5);struct.pack_into('<I',fields,32,0x50b3ec);fields[40]=0x20;struct.pack_into('<HH',fields,42,70,60);struct.pack_into('<dI',fields,48,10 if case['due'] else 10.25,0);struct.pack_into('<I',fields,64,case['active']);struct.pack_into('<d',fields,72,(9,10,10.25)[case['edge']]);mu.mem_write(OBJECT,bytes(fields))
        for sid,t in ((50,129),(51,129),(60,158),(61,158),(70,34)):  # 50/51 두 부모와60/61 두 표적을 현재 입력으로 둔다.
            raw=bytearray(50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=t;raw[11]=case['targetstate'] if sid==60 else 0;raw[12]=0xa5;raw[34]=1;struct.pack_into('<ff',raw,14,50.5 if sid<60 else 60,50.5 if sid<60 else 60);struct.pack_into('<I',raw,36,case['frame']);mu.mem_write(self.slot(sid),bytes(raw))
        mu.mem_write(VTABLE,struct.pack('<I',PARENT_QUERY));mu.mem_write(VTABLE+0x48,struct.pack('<I',UNPOP));mu.mem_write(VTABLE+0x90,struct.pack('<I',REPOP));mu.mem_write(OBJECT+0x100,struct.pack('<I',VTABLE+0x200));mu.mem_write(VTABLE+0x21c,struct.pack('<I',STOP+0x100))
        source=TYPES+129*self.type_stride;mu.mem_write(source,bytes(self.type_stride));codes=letters(case['mode']);codeaddr=OUTPUT+0x100;mu.mem_write(codeaddr,b''.join(bytes((letter,0,0,0)) for letter in codes));mu.mem_write(source+0x124,struct.pack('<I',codeaddr))
        for letter in range(65,81):  # 원문 TYPE의 문자 구간을 명시 입력으로 공급한다.
            indices=[i for i,value in enumerate(codes) if value==letter];mu.mem_write(source+0x130+(letter-65)*4,struct.pack('<i',indices[0] if indices else -1));mu.mem_write(source+0x170+(letter-65)*4,struct.pack('<i',len(indices)))
        for address,value in ((self.p['gun'],34),(self.p['special'],148),(0x5410fc,35),(0x541198,80),(0x541260,130),(0x541264,131)):  # 실제 getter의 현재 원본 전역이다.
            mu.mem_write(address,struct.pack('<I',value))
        mu.mem_write(self.p['now'],struct.pack('<d',10));mu.mem_write(0x501a54,struct.pack('<f',20));mu.mem_write(0,bytes(4));self.write_ranges=[(0,4),(OBJECT,OBJECT+0x100),(ALLOC,ALLOC+0x200)]+[(self.slot(sid),self.slot(sid)+50) for sid in (50,51,60,61)]
        saved={reg:0x12340000+i for i,reg in enumerate((UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP))}
        for reg,value in saved.items():mu.reg_write(reg,value)  # 원문 비휘발 레지스터 보존을 확인한다.
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EDX):mu.reg_write(reg,0)  # 앞선 실행의 임시값을 입력으로 쓰지 않는다.
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,self.control);mu.reg_write(UC_X86_REG_FPTAG,0xffff);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_ECX,OBJECT);mu.reg_write(UC_X86_REG_ESP,STACK)
        args=[70,60] if kind==1 else [case['argument']&0xffffffff] if kind==2 else [];mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.emu_start((0x467330,0x4685e0,0x468660,0x466f50)[kind],STOP,count=100000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+4+len(args)*4,(case,'return')
        assert all(mu.reg_read(reg)==value for reg,value in saved.items()),(case,'nonvolatile')
        assert not (mu.reg_read(UC_X86_REG_FPSW)&0x3800) and mu.reg_read(UC_X86_REG_FPCW)==self.control and self.read(0)==0,(case,'x87/SEH')
        slots=b''.join(bytes(mu.mem_read(self.slot(sid),50)) for sid in (50,51,60,61)).hex()
        return [str(mu.reg_read(UC_X86_REG_EAX)) if kind in (2,3) else 'return',';'.join(self.events),bytes(mu.mem_read(OBJECT,80)).hex(),slots]

def generate(smoke):
    """기존 자료를 덮어쓰지 않고 두 x87 정밀도의 같은 원문 결과를 저장한다."""
    cases=[sample(),sample(edge=1),sample(kind=1),sample(kind=2),sample(kind=3)] if smoke else inputs();assert len({tuple(c[k] for k in KEYS) for c in cases})==len(cases)
    oracles=[Oracle(0x027f),Oracle(0x037f)];lines=[]
    for index,case in enumerate(cases):  # 기대값은 C++를 부르지 않고 보호PE 명령에서만 얻는다.
        output=[oracle.run_body(dict(case)) for oracle in oracles];assert output[0]==output[1],(index,'precision');lines.append('\t'.join([str(case[k]) for k in KEYS]+output[0]))
    if smoke:print('아이스 원문 smoke',len(lines));return
    with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:stream.write('# 보호10.78 아이스80BYTE 발사. 냉각 표시/문맥/표적/Missile 생성은 명시 외부 경계다.\n'+'\n'.join(lines)+'\n')
    paths=set(oracles[0].exports+[ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__).resolve(),ROOT/'tools/decomp_cannon_fire.py',ROOT/'tools/decomp_crossbow_animation.py',ROOT/'tools/decomp_gunarcher_oracle.py',ROOT/'tools/decomp_gunanimation_oracle.py',ROOT/'tools/decomp_owner_oracle.py',FIXTURE])
    report=dict(host=socket.gethostname(),lastFullDecompilation='HJOW-Athlon / 2026-10-23',date='2026-10-27',uniqueInputs=len(cases),normalReturns=len(cases)*2,precisionDifferences=0,limits='공통Frame/아이스Ctor/Step/가상getter/Launch를 실제 실행한다. 냉각 표시004a8770·문맥0045ae60·MayTarget0046af80·공간/중심/Host/Missile ctor는 명시 외부 입력이다. 전체80BYTE/네 슬롯200BYTE/효과/반환/ABI/쓰기guard를 대조하며 전체 투사체·냉각 표시·게임 완주 근거는 아니다.',sha256={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('아이스 원문 입력',len(cases),'정상 반환',len(cases)*2)

def verify():
    """보존한 원문/자료/도구 해시와 고유 입력 수만 대조한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['sha256'].items():assert digest(ROOT/name)==expected,name  # 기대값을 C++ 결과로 재작성하지 않는다.
    rows=[r for r in FIXTURE.read_text('utf-8').splitlines() if r and not r.startswith('#')];assert len(rows)==report['uniqueInputs'];print('아이스 원문 SHA 감사',len(rows))

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)
