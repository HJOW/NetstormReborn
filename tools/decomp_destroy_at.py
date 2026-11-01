#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""DestroyAt의 실제 일반 입력·문자열 해석·삭제 부모를 제품 변경 전에 독립 관측한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_oracle import Oracle as BaseOracle, ROOT, SCRATCH, STACK, STOP
from decomp_game_menu_actions import Oracle as MenuOracle, cases
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

TEXT,OUTPUT,THREAD=SCRATCH+0x1000,SCRATCH+0x2000,SCRATCH+0x10000  # 원문 문자열/출력/TLS 장치의 별도 메모리다.
POOL,TABLE,DELETE=0x12000000,0x12001000,STOP+0x100  # 세 물리 슬롯과 명시 가상 삭제 경계다.


class MemoryOracle(BaseOracle):
    """완료된 전체 정밀 범위만 읽으며 작은 메모리/명시 호출 도구를 제공한다."""
    def word(self,address,value=None):
        """같은 비정렬DWORD를 읽거나 명시 장치 입력으로 쓴다."""
        if value is not None:
            self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def args(self,count):
        """호출 스택에서 원문 인자 개수만 읽는다."""
        return struct.unpack('<'+'I'*count,self.mu.mem_read(self.mu.reg_read(UC_X86_REG_ESP)+4,count*4))

    def returned(self,value=0,purge=0):
        """명시 경계만 해당 호출 규약으로 반환한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX,value&0xffffffff)
        self.mu.reg_write(UC_X86_REG_EIP,self.word(sp))
        self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge)

    def event(self,name,*values):
        """실제 경계 호출의 순서/값만 관측 문자열로 남긴다."""
        self.trace.append(':'.join(map(str,(name,*values))))


class InputOracle(MenuOracle):
    """일반 입력은 실행하고44c790의 현재 문자열 포인터 전달만 관측한다."""
    def instruction(self,mu,address,size,data):
        """키 뗌/Edit/미션 관문과 실제 y 인자를 원문에서 읽는다."""
        if address==0x44c790:
            self.body_event=True
            self.event('destroyAt',(self.number(mu.reg_read(UC_X86_REG_ESP)+4),))
            self.returned()
            return
        super().instruction(mu,address,size,data)


class ParserOracle(MemoryOracle):
    """실제 토큰/atol을 실행하고 타입 이름 검색과 CRT TLS만 명시 장치로 둔다."""
    def __init__(self):
        """순수 parser/CRT의 정확 범위와 C로케일의 기존 타입 표 포인터를 준비한다."""
        super().__init__('originals',[dict(name='Location',originals='0044c540')],[0x487710,0x487620,0x4e66de,0x4e6fa1,0x4e6656,0x4e65ed,0x4ef510,0x4e63e0])
        self.word(THREAD+0x64,self.word(0x543efc))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def write(self,mu,access,address,size,value,data):
        """parser의 출력/지역/TLS 외 원문 상태 쓰기를 거부한다."""
        if 0x30000000<=address<=address+size<=0x30010000 or OUTPUT<=address<=address+size<=OUTPUT+20 or THREAD<=address<=address+size<=THREAD+0x100:
            return
        raise RuntimeError(f'DestroyAt parser 미명시 쓰기 {address:08x}+{size}')

    def on_instruction(self,mu,address,size,data):
        """타입 문자열의 실제 바이트를 읽으며 토큰/숫자 계산은 원문에 맡긴다."""
        if address==0x4ea61d:
            self.returned(THREAD)
            return
        if address==0x49a860:
            name=self.read_string(self.args(1)[0])
            self.event('type',name.hex() or '-')
            self.returned({b'':0,b'suncannon':128,b'buildmana':134}.get(name.lower(),0xffffffff))
            return
        super().on_instruction(mu,address,size,data)

    def observe(self,text):
        """문자열 하나에서 다섯 실제 정수 출력과 호출/스택 복구를 읽는다."""
        self.trace=[]  # 이번 parser가 호출한 타입 이름 조회 기록이다.
        self.mu.mem_write(TEXT,text+b'\0')
        self.mu.mem_write(OUTPUT,bytes(20))
        self.call('Location',[TEXT,OUTPUT,OUTPUT+4,OUTPUT+8,OUTPUT+12,OUTPUT+16])
        return struct.unpack('<5I',self.mu.mem_read(OUTPUT,20)),';'.join(self.trace) or '-'


class ParentOracle(MemoryOracle):
    """실제44c790/점 정수화와 필터 루프를 실행하고 parser/finder/삭제만 경계로 둔다."""
    def __init__(self):
        """세 슬롯의 실제 타입/owner와 가상 표를 독립 메모리에 준비한다."""
        super().__init__('originals',[dict(name='DestroyAt',originals='0044c790')],[0x41d770,0x4e49c0])
        self.mu.mem_map(POOL,0x2000)
        self.word(TABLE+0x10,DELETE)
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def write(self,mu,access,address,size,value,data):
        """실제 원문은 스택만 쓰며 슬롯 변화는 명시 삭제/조회 효과에서만 허용한다."""
        if not 0x30000000<=address<=address+size<=0x30010000:
            raise RuntimeError(f'DestroyAt 부모 미명시 쓰기 {address:08x}+{size}')

    def on_instruction(self,mu,address,size,data):
        """파생 필터/owner의 늦은 읽기·0/경계 종료·삭제 뒤 Next 순서를 관측한다."""
        if address==0x4e0620:
            self.event('assert',self.args(3)[2])
            if self.boss==2:
                self.word(0x540bc4,1)
            self.returned()
            return
        if address==0x44c540:
            args=self.args(6)
            self.event('parse')
            for pointer,value in zip(args[1:],self.location):  # parser 경계가 지정 다섯 정수를 같은 출력 포인터에 쓴다.
                self.word(pointer,value)
            self.returned()
            return
        if address==0x4b16d0:
            self.event('begin',*self.args(4))
            self.cursor=0
            self.word(mu.reg_read(UC_X86_REG_ECX)+0x34,self.order[0])
            self.returned(0,16)
            return
        if address==0x4b1810:
            self.cursor+=1
            value=self.order[self.cursor]
            self.event('next',value)
            self.word(mu.reg_read(UC_X86_REG_ECX)+0x34,value)
            self.returned()
            return
        if address in (0x4ac200,DELETE):
            sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50
            if address==DELETE:
                self.event('delete',sid,*self.args(1))
                self.mu.mem_write(POOL+sid*50+11,b'\x01')
                if self.mutation==1:
                    self.mu.mem_write(POOL+2*50+10,b'\x64')
                    self.mu.mem_write(POOL+2*50+34,b'\x01')
                if self.mutation==3:
                    self.word(0x5c847c,2)
                self.returned(0,4)
            else:
                typ=self.mu.mem_read(POOL+sid*50+10,1)[0]
                genus={100:0x100,101:0x2000,102:0x80}[typ]
                self.event('genus',sid,genus)
                if self.mutation==2:
                    self.mu.mem_write(POOL+sid*50+34,b'\x02')
                self.returned(genus)
            return
        super().on_instruction(mu,address,size,data)

    def observe(self,x,y,typ,radius,owner,boss,profile,mutation,control):
        """입력만 설치하고 전체 호출/물리 슬롯과 정상 반환을 읽는다."""
        self.location=(x&0xffffffff,y&0xffffffff,typ,radius&0xffffffff,owner)  # parser 장치의 원시 출력이다.
        self.boss=boss  #0은 진단 뒤 종료,1은 정상,2는 진단 장치가 권한을 바꾼다.
        self.mutation=mutation  # 명시 조회/삭제 효과가 후속 읽기 상태를 바꾼다.
        self.order=((1,2,3,0),(0,),(4,1,0),(1,4,2,0))[profile]  # finder 장치가 반환할 현재 SID 순서다.
        self.cursor=0  # 같은 명시 finder의 현재 항목 위치다.
        self.trace=[]  # 이번 실제 부모의 경계 호출 기록이다.
        raw=bytearray(200)
        for sid,typ0,owner0 in ((1,100,1),(2,101,2),(3,102,1)):  # 서로 다른 필터 통과/실패 슬롯이다.
            struct.pack_into('<I',raw,sid*50,TABLE)
            raw[sid*50+10],raw[sid*50+11],raw[sid*50+34]=typ0,sid,owner0
        self.mu.mem_write(POOL,bytes(raw))
        self.word(0x5c8464,POOL)
        self.word(0x5c847c,4)
        self.word(0x540bc4,1 if boss==1 else 0)
        from unicorn.x86_const import UC_X86_REG_FPCW, UC_X86_REG_FPSW
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.call('DestroyAt',[TEXT])
        return ';'.join(self.trace) or '-',bytes(self.mu.mem_read(POOL,200)).hex()


def main():
    """세 계층의 원문 기대값을 새 디렉터리에만 기록하며 C++을 호출하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    out=args.output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    source=ParserOracle()
    texts=[b'',b'(205x186)',b'205x186',b' (205x186)',b'(1x2)sunCannon/25#2',b'(1x2)buildMana/-5#1',b'(1x2)missing/5#9',b'(-1x+2)/0#0',b'(12.5x3.9)',b'(1x2)"sunCannon"/999#2\nignored',b'(1x2)#2',b'(1x2)/25#2',b'(2147483647x-2147483648)',b'(1xx2))sunCannon//25##2',b'(1x2)'+b'a'*90+b'/5#2',b'(1x2)sunCannon/25#2\0trailing']
    parsed=[]
    for text in texts:  # 입력 문자열은 기대 정수/필터와 독립적으로 정한다.
        values,trace=source.observe(text)
        parsed.append((text.hex() or '-',*values,trace))
    inp=InputOracle()
    seed=cases()[0]
    inputs=[]
    for index,(parameter,flags,mode) in enumerate(itertools.product((0,0x1000000,0xffffffff),(0,0x80000000,0x38000000,0xb8000000),range(4))):
        values=list(map(int,seed['globals'].split(':')))
        values[0],values[3]=mode&1,(mode>>1)&1
        values[8]=values[9]=0
        event=0x020000e1|flags
        case=seed.copy()
        case.update(name=f'destroy-at-{index}',event=f'{event}:305419896:{parameter}',globals=':'.join(map(str,values)),guideResult=0,gumpResult=0,read=0,filled=0,mutation='-',stop='-')
        results=[inp.run(case,control) for control in (0x27f,0x37f,0x27f)]
        assert results[0]==results[1]==results[2]
        r=results[0]
        inputs.append((event,parameter,mode,r['bodytrace'],r['completed'],r['stackdelta']))
    parent=ParentOracle()
    parents=[]
    selected=[(205,186,typ,25,owner,boss,profile,mutation) for typ,owner,boss,profile,mutation in itertools.product((0,100,101,0xffffffff),(0,1,2,9),(0,1,2),range(4),range(4))]
    selected.extend((x,y,typ,radius,owner,1,0,0) for (x,y),typ,radius,owner in itertools.product(((0,0),(-1,2),(16777217,2147483647)),(0,100),(0,25,-5),(0,1)))
    for case in selected:  # x87 두 정밀도와 첫 정밀도 반복의 원문 결과를 동결한다.
        results=[parent.observe(*case,control) for control in (0x27f,0x37f,0x27f)]
        assert results[0]==results[1]==results[2]
        parents.append((*case,*results[0]))
    groups=(('destroy-at-parse-x86.tsv',('text','x','y','type','radius','owner','trace'),parsed),('destroy-at-input-x86.tsv',('event','parameter','mode','trace','completed','stackdelta'),inputs),('destroy-at-parent-x86.tsv',('x','y','type','radius','owner','boss','profile','mutation','trace','pool'),parents))
    for name,schema,rows in groups:
        (out/name).write_text('\t'.join(schema)+'\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows),encoding='utf-8',newline='\n')  # 값은 원문/명시 경계 관측만 사용한다.
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_destroy_at.py','tools/decomp_oracle.py','tools/decomp_game_menu_actions.py','tools/decomp_production_shortcuts.py')
    report=dict(unit=690,date='2026-11-01',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),parserCases=len(parsed),inputCases=len(inputs),parentCases=len(parents),observations=len(parsed)+3*(len(inputs)+len(parents)),differences=0,boundaries='실제44c540 토큰/atol과44c790 점 정수화/권한·필터·순회는 원문. parser의타입조회/TLS, 부모의parser/finder/분류getter/가상삭제는 명시 경계. 범위 출력은44c790에서사용하지않는다. 실제 게임/OS/통신/공격/승리는 실행하지 않는다.',fixtures={name:hashlib.sha256((out/name).read_bytes()).hexdigest() for name,_,_ in groups},sha256={path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in paths})
    (out/'native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(parser=len(parsed),input=len(inputs),parent=len(parents),observations=report['observations'])))


if __name__=='__main__':
    main()
