#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""NSInfo/PlayerInfo/BattleInfo/특수버튼생성과실제일반Attach관문을독립관측한다."""
import hashlib
import itertools
import json
import socket
import struct
from collections import Counter
from pathlib import Path
import decomp_production_shortcuts as image
from unicorn.x86_const import *

ROOT=Path(__file__).resolve().parents[1]  # 보호원문과새산출물의루트다.
FIXTURE=ROOT/'cpppj/tests/fixtures/hud-nsinfo-create-x86.tsv'  # 앞10열입력과뒤8열실제관측이다.
REPORT=ROOT/'extracted/cpp-hud-nsinfo-create-20261023-native.json'  # 현재명시범위와해시다.
NODES,STRINGS,STRIDE=0x1100000,0x1200000,0x200  # 독립하위장치의실제32비트주소다.
TABLE,ATTACH,END=image.HEAP,image.HEAP+0x2000,image.HEAP+0x3000  # 하위가상Attach와정상최종RET다.
SERVER=0x594970  # 원문서버getter의같은정적80BYTE버퍼다.
ENTRIES={'hidden':0x47ca50,'ns':0x47c4b0,'player':0x479d40,'battle':0x47afa0,'button':0x4256d0}  # 실제함수진입점이다.
RANGES=((0x47ca50,0x47caa9),(0x47c4b0,0x47c8b6),(0x479d40,0x479f98),(0x47afa0,0x47b173),
        (0x4256d0,0x4256fa),(0x47c430,0x47c47c),(0x479fb0,0x479ff6),(0x47a8eb,0x47a8f6),
        (0x47b480,0x47b4c4),(0x47b8de,0x47b8e8))  # 생성본체/서버복사/일반Attach와정확한합류다.
GLOBALS=(0x540421,0x540440,0x540457,0x5ca920,0x5ca924,0x5467b8,0x5411c8,0x5bec3c,0x54db4c,0x540bac,0x594fc8,0x595344,0x593150)  # 현재종류/자산/zone/서버/관문/전역포인터다.
SCHEMA=('name','operation','globals','fill','typeResult','format','server','mutation','stop','fail',
        'completed','result','globalsout','nodesout','stringsout','trace','stackdelta','nativeinstructions')  # 결과를입력으로사용하지않는다.


def node(index):
    """독립장치의고정노드주소를반환한다."""
    return NODES+index*STRIDE


def pack(values):
    """원시DWORD를보존하는십진목록이다."""
    return ':'.join(str(value&0xffffffff) for value in values)


class Stop(Exception):
    """명시외부효과중단을정상원문RET와구별한다."""


class Oracle(image.Oracle):
    """기존보호이미지적재기와독립고정메모리장치를사용한다."""
    def __init__(self):
        """메모리내원문만실행하고게임/OS장치는만들지않는다."""
        super().__init__()
        self.mu.mem_map(NODES,0x10000)
        self.mu.mem_map(STRINGS,0x10000)
        self.mu.mem_map(0,0x1000)
        self.number(TABLE+0x20,ATTACH)
        self.original={low:bytes(self.mu.mem_read(low,high-low)) for low,high in RANGES}

    def string(self,address):
        """현재NUL앞원시문자열을명시장치에서만읽는다."""
        result=bytearray()  # 실제바이트이다.
        for offset in range(512):  # 명시문자열의허용물리폭이다.
            value=self.number(address+offset,width=1)
            if not value:
                return bytes(result)
            result.append(value)
        raise RuntimeError('NSInfo문자열NUL한도')

    def event(self,name,values=()):
        """실제인자기록뒤같은주소의명시변경을입력순서대로적용한다."""
        self.boundaries[name]+=1
        self.trace.append(':'.join((name,*(str(v) for v in values))))
        trigger=name+str(self.calls[name]+1)
        self.calls[name]+=1
        for effect in self.effects:  # 현재전역/노드/문자열은같은메모리를변경한다.
            target,address,value=effect.split('/')
            if target==trigger:
                address=int(address)
                self.number(address,int(value),1 if address in GLOBALS[:3] else 4)
        if trigger in self.stops:
            raise Stop()

    def guard(self,mu,access,address,size,value,data):
        """실제CPU쓰기를생성노드/실제서버버퍼/프레임/현재전역/SEH에제한한다."""
        if image.STACK-0x8000<=address and address+size<=image.STACK+0x1000:
            category='stack'
        elif NODES<=address and address+size<=NODES+0x10000:
            category='nodes'
        elif SERVER<=address and address+size<=SERVER+0x100:
            category='server'
        elif address==0 and size==4:
            category='seh'
        elif address==0x593150 and size==4:
            category='current'
        else:
            raise RuntimeError(f'미명시NSInfo쓰기 {address:08x}/{size}')
        self.writes[category]+=1

    def base_device(self,token,flags=1):
        """하위공통생성장치의명시초기필드만같은원시주소에쓴다."""
        self.number(token,TABLE)
        self.number(token+4,flags)
        self.number(token+0x20,0)

    def instruction(self,mu,address,size,data):
        """네부모와실제서버복사/Attach관문을실행하고기존하위처리기만대체한다."""
        if address==END:
            self.completed=True
            mu.emu_stop()
            return
        if any(low<=address<high for low,high in RANGES):
            self.instructions+=1
            self.visited.add(address)
            return
        esp,token=mu.reg_read(UC_X86_REG_ESP),mu.reg_read(UC_X86_REG_ECX)  # 실제호출스택과this다.
        if address in (0x479ff6,0x47b4c4):
            kind='player' if address==0x479ff6 else 'battle'
            self.event('meta',(kind,token))
            mu.reg_write(UC_X86_REG_EIP,0x47a8eb if kind=='player' else 0x47b8de)
            return
        if address==0x4e4391:
            requested=self.number(esp+4)
            self.alloc_calls+=1
            if self.alloc_calls in self.failures:
                result=0
            else:
                result=node(len(self.nodes))
                self.nodes.append((result,requested))
            self.event('alloc',(requested,result))
            self.returned(result)
            return
        if address==0x4642c0:
            args=[self.number(esp+i*4) for i in range(1,5)]
            self.base_device(token,args[3])
            self.event('base',(token,*args))
            self.returned(token,16)
            return
        constructors={0x4655d0:('background',6),0x4bc060:('scroll',4),0x4bb170:('text',4),0x4253e0:('buttonBase',3),0x4254b0:('stone',5)}  # 실제기존생성자의인자폭이다.
        if address in constructors:
            name,count=constructors[address]
            args=[self.number(esp+i*4) for i in range(1,count+1)]
            self.base_device(token)
            if name=='scroll':
                self.number(token+0xd0,0x100)
            if name in ('stone','buttonBase'):
                self.number(token+0xac,0x100)
            self.event(name,(token,*args))
            self.returned(token,count*4)
            return
        setters={0x4ba230:'scrollable',0x4ba1b0:'slider',0x4ba210:'wrap',0x4ba8f0:'parseScroll',0x4bc800:'parseText'}  # 기존하위갱신/설정의명시경계다.
        if address in setters:
            name,value=setters[address],self.number(esp+4)
            if name in ('scrollable','slider'):
                bit=4 if name=='scrollable' else 2
                self.number(token+0xd0,(self.number(token+0xd0)&~bit)|((value&1)*(bit)))
            values=(token,self.string(value).hex()) if name.startswith('parse') else (token,value)
            self.event(name,values)
            self.returned(0,4)
            return
        if address in (0x4bb310,0x424f90):
            name='fitText' if address==0x4bb310 else 'fitStone'
            count=2 if name=='fitText' else 0
            self.event(name,(token,*(self.number(esp+i*4) for i in range(1,count+1))))
            self.returned(0,count*4)
            return
        if address in (ATTACH,0x466750):
            self.event('attach' if address==ATTACH else 'baseAttach',(token,*(self.number(esp+i*4) for i in range(1,5))))
            self.returned(0,16)
            return
        if address==0x4de9a0:
            source=self.string(self.number(esp+4))
            value=bytes.fromhex(self.case['format']) if source==b'~w~E%s\nPlayers in Zone %d\n' else b'T:'+source
            result=STRINGS+len(self.strings)*0x200
            self.mu.mem_write(result,value+b'\0')
            self.strings.append(result)
            self.event('translate',(source.hex(),result))
            self.returned(result,4)
            return
        if address==0x458f90:
            args=[self.number(esp+i*4) for i in range(1,7)]
            self.mu.mem_write(args[3],bytes.fromhex(self.case['server'])+b'\0')
            self.event('remote',args)
            self.returned()
            return
        if address==0x4a89c0:
            destination,fmt,server,zone=(self.number(esp+i*4) for i in range(1,5))
            source,name=self.string(fmt),self.string(server)
            signed=struct.unpack('<i',struct.pack('<I',zone))[0]
            value=source%(name,signed)
            self.mu.mem_write(destination,value+b'\0')
            self.formatted=value
            self.event('format',(source.hex(),name.hex(),zone))
            self.returned(destination)
            return
        if address==0x49a840:
            self.event('type',(self.number(esp+4),self.case['typeResult']))
            self.returned(self.case['typeResult'])
            return
        if address==0x4e0620:
            self.event('assert',(self.number(esp+12),))
            self.returned()
            return
        raise RuntimeError(f'미명시NSInfo실행 {address:08x}')

    def run(self,case,control):
        """독립입력만설치하고실제부모의전체현재메모리와순서를관측한다."""
        self.case,self.trace,self.calls,self.nodes,self.strings=case,[],Counter(),[],[]
        self.formatted,self.instructions,self.completed,self.alloc_calls=b'',0,False,0
        self.effects=[] if case['mutation']=='-' else case['mutation'].split(';')
        self.stops=set() if case['stop']=='-' else set(case['stop'].split(';'))
        self.failures=set() if case['fail']=='-' else set(map(int,case['fail'].split(':')))
        self.mu.mem_write(NODES,bytes([case['fill']])*0x10000)
        self.mu.mem_write(STRINGS,bytes([case['fill']])*0x10000)
        self.mu.mem_write(SERVER,bytes([case['fill']])*0x100)
        for index,(address,value) in enumerate(zip(GLOBALS,map(int,case['globals'].split(':')))):  # 실제현재입력순서를보존한다.
            self.number(address,value,1 if index<3 else 4)
        operation=case['operation']
        if operation!='hidden':
            self.nodes.append((node(0),{'ns':0xd8,'player':0xbc,'battle':0x16c,'button':0xc8}[operation]))
        self.mu.mem_write(image.STACK-0x4000,bytes(0x8000))
        self.number(image.STACK,END)
        if operation=='button':
            self.number(image.STACK+4,16)
            self.number(image.STACK+8,15)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 보존레지스터와초기값이다.
            self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ECX,node(0))
        self.mu.reg_write(UC_X86_REG_ESP,image.STACK)
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.number(0,0)
        try:
            self.mu.emu_start(ENTRIES[operation],0,count=300000)
        except Stop:
            pass
        if not self.completed and not self.stops:
            raise RuntimeError('NSInfo원문RET미도달')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW):
            raise RuntimeError('NSInfox87상태변경')
        result=dict(completed=int(self.completed),result=self.mu.reg_read(UC_X86_REG_EAX),globalsout=pack(self.number(a,width=1 if i<3 else 4) for i,a in enumerate(GLOBALS)),
                    nodesout=';'.join(f'{address}:{requested}:'+bytes(self.mu.mem_read(address,0x180)).hex() for address,requested in self.nodes) or '-',
                    stringsout=';'.join(f'{address}:'+bytes(self.mu.mem_read(address,0x200)).hex() for address in self.strings)+'|'+bytes(self.mu.mem_read(SERVER,0x100)).hex()+'|'+self.formatted.hex(),
                    trace=';'.join(self.trace) or '-',stackdelta=self.mu.reg_read(UC_X86_REG_ESP)-image.STACK,nativeinstructions=self.instructions)
        for low,expected in self.original.items():  # 실제실행본체는모든입력뒤원문과같아야한다.
            if bytes(self.mu.mem_read(low,len(expected)))!=expected:
                raise RuntimeError('보호NSInfo본체변경')
        total=self.totals.setdefault(f'{control:03x}',dict(runs=0,instructions=0))
        total['runs']+=1
        total['instructions']+=self.instructions
        return result


def cases():
    """전체생성과독립하위/부호/원시보존/현재조회/부분중단을기대값없이분해한다."""
    rows=[]  # 명시장치입력목록이다.
    defaults=(6,37,60,4,5,3,92,7,0xffffffff,0x50c894,0,1,0x31415926)  # 실제전역열순서다.

    def add(name,operation='hidden',changes=None,**extra):
        """변경한입력필드만등록하고관측결과를계산하지않는다."""
        values=list(defaults)
        for index,value in (changes or {}).items():  # 현재전역입력만수정한다.
            values[index]=value
        case=dict(name=name,operation=operation,globals=pack(values),fill=0xa5,typeResult=0x12345678,
                  format=b'~w~E%s\nPlayers in Zone %d\n'.hex(),server=b'Remote Server'.hex(),mutation='-',stop='-',fail='-')
        case.update(extra)
        rows.append(case)

    for operation,fill in itertools.product(ENTRIES,(0,0xa5,0xff)):
        add(f'base-{operation}-{fill}',operation,fill=fill)  # 전체와각실제부모의미작성바이트를구별한다.
    for operation,kind in itertools.product(('ns','player','battle'),(0,127,128,255)):
        add(f'kind-{operation}-{kind}',operation,changes={0 if operation=='ns' else 1 if operation=='player' else 2:kind})
    for zone,server in itertools.product((0,1,0x7fffffff,0x80000000,0xffffffff),(0xffffffff,0,17)):
        add(f'zone-{zone}-{server}',changes={5:zone,8:server})
    for meta,known in itertools.product((0,1),(0,1)):
        add(f'mode-{meta}-{known}',changes={10:meta,11:known})
    add('root-allocation-failure',fail='1')
    add('missing-button-type','player',typeResult=0)
    mutations=(('zone-after-translate','hidden',f'translate1/{0x5467b8}/91'),
               ('zone-after-server','hidden',f'translate2/{0x5467b8}/92'),
               ('background-current','hidden',f'alloc2/{0x5ca920}/44;alloc5/{0x5ca924}/19'),
               ('title-current','hidden',f'format1/{node(0)+0xc8}/{node(2)}'),
               ('kind-current','hidden',f'base2/{0x540457}/128;base3/{0x540440}/255'),
               ('button-flags','player',f'buttonBase1/{node(2)+0xac}/128'),
               ('master-current','player',f'translate1/{node(0)+0xa4}/{node(1)}'),
               ('button-type-after-assert','player',f'assert1/{node(0)+0xb4}/9'),
               ('format-borrowed-bytes','hidden',f'translate2/{STRINGS}/{int.from_bytes(b"HEAD","little")}'),
               ('server-borrowed-bytes','hidden',f'translate2/{STRINGS+0x200}/{int.from_bytes(b"LAN!","little")}'),
               ('wrapper-current-overwrite','hidden',f'alloc50/{0x593150}/123'),
               ('mode-after-base-attach','hidden',f'baseAttach1/{0x594fc8}/1;baseAttach1/{0x595344}/1'))  # 고정this와현재자식/전역재조회를구별한다.
    for name,operation,mutation in mutations:
        add(name,operation,mutation=mutation,typeResult=0 if name=='button-type-after-assert' else 0x12345678)
    for name,operation in (('alloc','hidden'),('base','hidden'),('scroll','hidden'),('translate','hidden'),('format','hidden'),('parseScroll','hidden'),('attach','hidden'),('baseAttach','hidden'),('type','player'),('buttonBase','player'),('parseText','player'),('fitText','battle'),('fitStone','player'),('assert','player')):
        add('stop-'+name,operation,stop=name+'1',typeResult=0 if name=='assert' else 0x12345678)
    for allocation in (2,3,5,7,10,11,15,20,30,40,50):
        add(f'stop-allocation-{allocation}',stop='alloc'+str(allocation))  # 이미만든모든자식을부분관측으로보존한다.
    return rows


def main():
    """고정보호PE/호스트에서두제어어를실행하고새자료만기록한다."""
    if socket.gethostname().lower()!='vm-w11-codex' or image.digest(image.PE_PATH)!=image.PE_SHA:
        raise RuntimeError('보호원문/현재호스트불일치')
    oracle,inputs,rows=Oracle(),cases(),[]
    for index,case in enumerate(inputs):  # 관측결과만두제어어에서비교한다.
        previous=None
        for control in image.CONTROLS:  # 53/64비트x87제어어를각각설치한다.
            try:
                output=oracle.run(case,control)
            except Exception as error:
                raise RuntimeError(case['name']+': '+str(error)) from error
            # 미완료EAX는반환값이아니므로제어어차이판정에서도제외한다.
            comparison={key:value for key,value in output.items() if key!='result' or output['completed']}
            if previous is not None and previous!=comparison:
                raise RuntimeError('NSInfo정밀도차이 '+case['name'])
            previous=comparison
            values=dict(case,name=case['name']+f'-cw{control:03x}',**output)
            rows.append('\t'.join(str(values[field]) for field in SCHEMA))
        if index%20==0:
            print('observed',index,flush=True)
    FIXTURE.write_text('# '+'\t'.join(SCHEMA)+'\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    record=dict(host=socket.gethostname(),lastDecompileHost='VM-W11-Codex / 2026-10-23',inputs=len(inputs),observations=len(rows),schema=SCHEMA,
                globals=[hex(address) for address in GLOBALS],nodeBase=NODES,nodeStride=STRIDE,nodeBytes=0x180,stringBase=STRINGS,stringBytes=0x200,serverAddress=SERVER,
                precisions=oracle.totals,precisionDifferences=0,boundaries=dict(oracle.boundaries),cpuWrites=dict(oracle.writes),
                nativeRanges=[[hex(a),hex(b)] for a,b in RANGES],
                limits=['실제47ca50/47c4b0/479d40/47afa0/4256d0전체생성본체와47c430서버getter를실행한다.',
                        '실제479fb0/47b480의공통Attach→현재meta/playerKnown관문을실행한다.관문이후전체메타맵배치는meta명시경계다.',
                        'new/공통Gump/배경/Scrolling/Text/ButtonBase/Stone/번역/TYPE/서버원격/형식화/Parse/Fit/가상Attach/진단은명시장치다.실제하위Slider와Stone내부자식은기존생성기연결의별도범위다.',
                        '부모단계의정상최소노드는50개다.일반raw0x180와명시번역버퍼전체512BYTE/서버256BYTE를보존한다.형식화출력의스택주소는문자열내용으로관측한다.',
                        '최상위new실패만정상분기다.자식null역참조/실제EHunwind/GUI/게임/전체메타맵/소멸은이자료가검증하지않는다.'],
                gameExecuted=False,guiExecuted=False,
                sha256={str(path.relative_to(ROOT)).replace('\\','/'):image.digest(path) for path in (image.PE_PATH,ROOT/'extracted/refined/originals/Netstorm.c',Path(image.__file__),Path(__file__),FIXTURE)})
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({key:record[key] for key in ('inputs','observations','precisionDifferences')}),flush=True)


if __name__=='__main__':
    main()
