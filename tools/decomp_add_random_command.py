#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""AddRandom 일반 입력과 실제0040dfd0 부모를 제품 변경 전에 독립 관측한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_game_menu_actions import Oracle as MenuOracle, cases
from decomp_oracle import Oracle as BaseOracle, ROOT, STOP, STACK
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW


class InputOracle(MenuOracle):
    """Guide/Gump/일반 입력은 실행하고 이름 조회와 배치 부모만 명시 경계로 둔다."""
    def instruction(self,mu,address,size,data):
        """signed255 비교·모드·뗌과 실제 두 인자의 전달을 관측한다."""
        if address in (0x49a860,0x40dfd0):
            self.body_event=True
            sp=mu.reg_read(UC_X86_REG_ESP)
            count=1 if address==0x49a860 else 2
            self.event('lookup' if count==1 else 'add',tuple(self.number(sp+4*(i+1)) for i in range(count)))
            self.returned(self.lookup if count==1 else 0)
            return
        super().instruction(mu,address,size,data)


def double_words(value):
    """기대값 계산 없이 입력 double의 원시 두DWORD를 기록한다."""
    return struct.unpack('<II',struct.pack('<d',value))


class ParentOracle(BaseOracle):
    """기존 실제 seed/CRT와 고정 범위·섬 검색을 실행하고 마지막 배치만 관측한다."""
    def __init__(self):
        """같은 정밀 디컴파일에서 네 부모/순수 보조의 정확 범위를 추가한다."""
        super().__init__('originals',[dict(name='AddRandom',originals='0040dfd0')],[0x4bcab0,0x431fc0,0x40d900,0x46f380,0x4e49c0])
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def read(self,address):
        """가상 메모리의 현재 DWORD를 원문 폭으로 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def put(self,address,value):
        """명시 입력/장치 효과만 해당DWORD에 설치한다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))

    def args(self,count):
        """현재 cdecl/thiscall 호출 스택의 실제 인자를 읽는다."""
        return struct.unpack('<'+'I'*count,self.mu.mem_read(self.mu.reg_read(UC_X86_REG_ESP)+4,4*count))

    def event(self,name,*values):
        """명시 경계의 실제 호출 인자를 순서대로 보존한다."""
        self.events.append(':'.join(map(str,(name,*values))))

    def ret(self,value):
        """명시 cdecl 장치만 반환하며 순수 게임 계산은 원문 명령에 맡긴다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX,value)
        self.mu.reg_write(UC_X86_REG_EIP,self.read(sp))
        self.mu.reg_write(UC_X86_REG_ESP,sp+4)

    def write(self,mu,access,address,size,value,data):
        """실제 부모의 쓰기는 지역 스택으로만 제한한다."""
        if not 0x30000000<=address<=address+size<=0x30010000:
            raise RuntimeError(f'AddRandom 부모 미허용 쓰기 {address:08x}+{size}')

    def on_instruction(self,mu,address,size,data):
        """벽시각 효과 뒤 현재 owner/fort 재조회와 최종 seed/사각형을 기록한다."""
        if address==0x40ded0:
            args=self.args(9)
            self.event('B',self.read(args[0]),*args[1:])
            self.ret(7)
            return
        if address==0x46f380:
            self.event('R',*self.args(2))
        if address==0x460d70:
            output,=self.args(1)
            self.event('T',*double_words(self.time))
            mu.mem_write(output,struct.pack('<d',self.time))
            if self.mutation:
                self.put(0x540c70,2)
                self.put(0x594fb8,1)
            self.ret(output)
            return
        super().on_instruction(mu,address,size,data)

    def observe(self,typ,quantity,time,owner,fort,mutation,control):
        """세 실제 섬 기록과 호출 입력을 설치하고 정상 반환/스택/지역 난수를 읽는다."""
        self.time=time  # 명시 벽시각 조회의 현재 double 입력이다.
        self.mutation=mutation  # 벽시각 장치 효과에서 owner/fort를 바꾸는 입력이다.
        self.events=[]  # 이번 원문 호출에서 관측한 효과 순서다.
        self.put(0x540c70,owner)
        self.put(0x594fb8,fort)
        raw=bytearray(3204)
        struct.pack_into('<I',raw,0,3)
        for index,(current,original) in enumerate(((2,1),(1,1),(0,9))):  # 일반/fort의 서로 다른 첫 일치를 만든다.
            at=4+index*32
            struct.pack_into('<II',raw,at+4,1,current)
            raw[at+12],raw[at+13]=0,original
        self.mu.mem_write(0x568b60,bytes(raw))
        for reg,value in ((UC_X86_REG_EAX,0),(UC_X86_REG_EBX,0),(UC_X86_REG_ECX,0),(UC_X86_REG_EDX,0),(UC_X86_REG_ESI,0),(UC_X86_REG_EDI,0),(UC_X86_REG_EBP,0),(UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,control),(UC_X86_REG_FPSW,0)):
            self.mu.reg_write(reg,value)  # 호출마다 이전 레지스터/정밀도 입력을 분리한다.
        self.mu.mem_write(STACK,struct.pack('<III',STOP,typ,quantity))
        self.mu.emu_start(0x40dfd0,STOP,count=20000)
        delta=self.mu.reg_read(UC_X86_REG_ESP)-STACK
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and delta==4
        return ';'.join(self.events),delta


def main():
    """두 원문 관측을 새 디렉터리에 동결하고 세 정밀도/반복 결과를 비교한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    options=parser.parse_args()
    out=options.output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    input_oracle=InputOracle()
    seed=cases()[0]
    input_rows=[]
    for index,(parameter,flags,mode,lookup) in enumerate(itertools.product((0,100,134,255,0x1000000,0xffffffff),(0,0x80000000,0x38000000,0xb8000000),range(4),(0,134,0xffffffff))):
        values=list(map(int,seed['globals'].split(':')))
        values[0],values[3]=mode&1,(mode>>1)&1
        values[8]=values[9]=0
        event=0x02000098|flags
        case=seed.copy()
        case.update(name=f'add-{index}',event=f'{event}:305419896:{parameter}',globals=':'.join(map(str,values)),guideResult=0,gumpResult=0,read=0,filled=0,mutation='-',stop='-')
        input_oracle.lookup=lookup  # 실제 이름 조회 경계가 반환할 signed TYPE 비트다.
        results=[input_oracle.run(case,control) for control in (0x27f,0x37f,0x27f)]
        assert results[0]==results[1]==results[2]
        r=results[0]
        input_rows.append((event,parameter,mode,lookup,r['bodytrace'],r['completed'],r['stackdelta']))
    parent=ParentOracle()
    parent_rows=[]
    for typ,quantity,time,owner,fort,mutation in itertools.product((0,89,134,255,0xffffffff),(0,1,3),(0.0,2.5,-30.25,123.456),(1,2,9),(0,1),(0,1)):
        results=[parent.observe(typ,quantity,time,owner,fort,mutation,control) for control in (0x27f,0x37f,0x27f)]  # 부모 계산을 세 번 실제 실행한다.
        assert results[0]==results[1]==results[2]
        parent_rows.append((typ,quantity,*double_words(time),owner,fort,mutation,*results[0]))
    for name,schema,rows in (('add-random-input-x86.tsv',('event','parameter','mode','lookup','trace','completed','stackdelta'),input_rows),('add-random-parent-x86.tsv',('type','quantity','timeLow','timeHigh','owner','fort','mutation','trace','stackdelta'),parent_rows)):
        (out/name).write_text('\t'.join(schema)+'\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows),encoding='utf-8',newline='\n')  # 제품 출력 없이 관측만 기록한다.
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_add_random_command.py','tools/decomp_oracle.py','tools/decomp_game_menu_actions.py','tools/decomp_production_shortcuts.py')
    report=dict(unit=689,date='2026-11-01',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),inputCases=len(input_rows),parentCases=len(parent_rows),observations=3*(len(input_rows)+len(parent_rows)),differences=0,boundaries='일반 입력4d6e61..4da477의 이름조회/40dfd0은 명시 경계. 별도 실제40dfd0/4bcab0/431fc0/40d900/46f380/CRT는 실행하고 벽시각과 마지막40ded0만 명시 경계다. 마지막 개별 객체 생성/공격/공중3종/게임/OS/통신은 실행하지 않는다. 기존 PlaceRandom719관측과 별도다.',fixtures={name:hashlib.sha256((out/name).read_bytes()).hexdigest() for name in ('add-random-input-x86.tsv','add-random-parent-x86.tsv')},sha256={path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in paths})
    (out/'native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(input=len(input_rows),parent=len(parent_rows),observations=report['observations'],differences=0)))


if __name__=='__main__':
    main()
