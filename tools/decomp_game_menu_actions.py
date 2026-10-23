#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""기존 현재사건 관문부터 메뉴 일반명령과 실제 큐/종료 RET를 독립 관측한다."""
import hashlib
import itertools
import json
import socket
import struct
from collections import Counter
from pathlib import Path
import decomp_production_shortcuts as gate
from unicorn.x86_const import *

ROOT=Path(__file__).resolve().parents[1]  # 새 산출물의 작업 루트다.
FIXTURE=ROOT/'cpppj/tests/fixtures/game-menu-actions-x86.tsv'  # 입력과 실제 관측이다.
REPORT=ROOT/'extracted/cpp-game-menu-actions-20261023-native.json'  # 명시 경계와 실행 근거다.
ARENA=0x1010000  # 명시 Player/Form/두 SID 목록의 같은 원시 저장소다.
ARENA_SIZE=0x800  # 전체 장치 메모리의 관측 폭이다.
PLAYERS,POOL,FIRST,SECOND=ARENA,ARENA+0x400,ARENA+0x600,ARENA+0x640  # 실제 전역이 가리키는 장치 주소다.
FRAME,QUIT_RETURN=gate.STACK+0x3000,gate.HEAP+0x3100  # 원본 종료 epilogue가 복구하는 상위 프레임이다.
GLOBALS=(0x5c85a4,0x594fb8,0x594fc8,0x594fa4,0x595344,0x540c70,0x59a8ac,0x59a8b0,
         gate.PAUSE,gate.CONSOLE,0x55a714,0x5e18a4,0x59534c,0x5c8464,0x55a70c,0x5e189c)  # 현재 읽기/쓰기 순서를 가진 실제 전역이다.
COMMANDS=(0x542700,0x542944,0x542a8c,0x542a90,0x542a30,0x5429ac,0x5429d4,0x542ab8,0x542b54)  # 이 단위의 불변 명령 이름표다.
EXTRA=((0x4da4be,0x4da507),)  # QuitApp만의 실제 SEH 복구/RET다.
SCHEMA=('name','event','guideResult','gumpResult','globals','devices','read','filled','mutation','stop',
        'completed','quit','globalsout','devicesout','readout','writeout','physicalqueue','bodytrace','envelopetrace','stackdelta','nativeinstructions')  # 앞10열만 입력이다.


def packed(values):
    """원시 DWORD 순서를 바꾸지 않는 십진 입력 표기다."""
    return ':'.join(str(value&0xffffffff) for value in values)


class ExplicitStop(Exception):
    """명시 효과에서 중단한 부분 관측을 정상 원문 반환과 구별한다."""


class Oracle(gate.Oracle):
    """Guide/Gump 관문과 메뉴 명령의 실제 본체를 같은 메모리에서 실행한다."""

    def __init__(self):
        """기존 생산 관측기의 원문 적재/실제 Guide/큐를 재사용한다."""
        super().__init__()
        self.mu.mem_map(ARENA,0x1000)
        self.mu.mem_map(0,0x1000)
        self.original.update({low:bytes(self.mu.mem_read(low,high-low)) for low,high in EXTRA})

    def cstring(self,address):
        """실제로 전달한 NUL 앞 문자열만 외부 장치에서 읽는다."""
        output=bytearray()  # 실제 문자 바이트다.
        for offset in range(512):  # 허용 장치 문자열의 최대 폭이다.
            byte=self.number(address+offset,width=1)
            if not byte:
                return output.decode('ascii')
            output.append(byte)
        raise RuntimeError('메뉴 장치 문자열 한도')

    def event(self,name,values=()):
        """명시 경계의 실제 인자 기록 뒤 지정한 현재 메모리 변경을 적용한다."""
        self.boundaries[name]+=1
        entry=':'.join((name,*(str(value) for value in values)))  # 실제 장치 인자다.
        (self.body if self.body_event else self.envelope).append(entry)
        trigger=name+str(self.calls[name]+1)
        self.calls[name]+=1
        for effect in self.effects:  # 같은 경계의 입력 변경을 입력 순서대로 적용한다.
            target,address,value=effect.split('/')
            if target==trigger:
                self.number(int(address),int(value))
        if trigger in self.stops:
            raise ExplicitStop()

    def guard(self,mu,access,address,size,value,data):
        """원문 CPU 쓰기를 실제 프레임/큐/Hide 전역/SEH에만 허용한다."""
        if gate.STACK-0x8000<=address and address+size<=gate.STACK+0x3100:
            category='stack'
        elif gate.READ<=address and address+size<=gate.QUEUE+40*12:
            category='queue'
        elif 0x59a8ac<=address and address+size<=0x59a8b4:
            category='visibility'
        elif address==0 and size==4:
            category='seh'
        else:
            raise RuntimeError(f'미명시 메뉴 쓰기 {address:08x}/{size}')
        self.writes[category]+=1

    def instruction(self,mu,address,size,data):
        """명령 분기와 실제 큐/Guide를 실행하고 선택한 하위 효과만 대체한다."""
        if address==QUIT_RETURN:
            self.completed,self.quit=True,self.mu.reg_read(UC_X86_REG_EAX)
            mu.emu_stop()
            return
        if address==gate.END:
            self.completed=True
            mu.emu_stop()
            return
        esp=mu.reg_read(UC_X86_REG_ESP)  # 현재 실제 경계 스택이다.
        self.body_event=address==0x452fe0 or address==0x460de0 and self.number(esp)==0x4d827a
        if any(low<=address<high for low,high in EXTRA):
            self.instructions+=1
            self.visited.add(address)
            return
        if address==0x4a89c0:
            destination,format_address,value=(self.number(esp+offset) for offset in (4,8,12))
            signed=struct.unpack('<i',struct.pack('<I',value))[0]  # 실제 %d 인자폭이다.
            self.mu.mem_write(destination,(self.cstring(format_address)%signed).encode('ascii')+b'\0')
            self.boundaries['formatter']+=1
            self.returned(destination)
            return
        if address==0x4d53f0:
            self.body_event=False
            self.event('shortcut')
            self.returned()
            return
        boundaries={0x4cf960:('tell',1),0x441580:('config',1),0x4a9cb0:('sound',6),
                    0x4d3090:('status',5),0x446e70:('cancel',0),0x460df0:('pause',0),0x460e30:('resume',0),
                    0x4aa900:('mute',0),0x4aa970:('unmute',0),0x433ba0:('center',1),
                    0x4950d0:('priest',1),0x455380:('reload',3),0x4e0620:('assert',3)}  # 실제 외부 호출의 이름/인자폭이다.
        if address in boundaries:
            name,count=boundaries[address]
            values=[self.number(esp+offset*4) for offset in range(1,count+1)]  # 실제 push 순서다.
            if name in ('tell','config','sound'):
                values[0]=self.cstring(values[0])
            if name=='status':
                values[4]=self.cstring(values[4])
            if name=='assert':
                values=[values[2]]
            if name=='pause':
                self.number(gate.PAUSE,self.number(gate.PAUSE)+1)
            elif name=='resume' and self.number(gate.PAUSE):
                self.number(gate.PAUSE,self.number(gate.PAUSE)-1)
            self.body_event=True
            self.event(name,values)
            self.returned()
            return
        super().instruction(mu,address,size,data)

    def run(self,case,control):
        """원문 관문에서 시작하여 전체 현재 자료/큐/효과/부분 중단을 관측한다."""
        self.case,self.body,self.envelope,self.calls=case,[],[],Counter()
        self.trace,self.instructions,self.completed,self.quit=[],0,False,0
        self.body_event=False
        self.effects=[] if case['mutation']=='-' else case['mutation'].split(';')
        self.stops=set() if case['stop']=='-' else set(case['stop'].split(';'))
        self.mu.mem_write(ARENA,bytes.fromhex(case['devices']))
        for address,value in zip(GLOBALS,map(int,case['globals'].split(':'))):  # 현재 원시 전역을 입력 그대로 설치한다.
            self.number(address,value)
        self.number(gate.GUIDE+0x800,gate.MISSION)
        self.number(gate.MISSION,gate.TABLE)
        self.number(gate.TABLE+0x24,gate.VIRTUAL_INPUT)
        self.number(gate.CURSOR,1)
        self.number(gate.READ,case['read'])
        self.number(gate.WRITE,(case['read']+case['filled'])%40)
        for index in range(40):  # 비활성 큐 슬롯도 고유 입력으로 보존한다.
            self.mu.mem_write(gate.QUEUE+index*12,struct.pack('<3I',0x2000300+index,100+index,200+index))
        self.mu.mem_write(gate.STACK-0x4000,bytes(0x8000))
        self.mu.mem_write(gate.STACK+0x14,struct.pack('<3I',*map(int,case['event'].split(':'))))
        self.number(gate.STACK+8,FRAME)
        self.number(FRAME,0)
        self.number(FRAME+4,QUIT_RETURN)
        self.number(gate.STACK+0x26c4,0)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 원문 사건 프레임의 EDI=0을 포함한다.
            self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,gate.STACK)
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        try:
            self.mu.emu_start(gate.ENTRY,0,count=300000)
        except ExplicitStop:
            pass
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW):
            raise RuntimeError('메뉴 정수 분기의 x87 상태 변경')
        if not self.completed and not self.stops:
            raise RuntimeError('메뉴 현재 사건 종료 미도달')
        output=dict(completed=int(self.completed),quit=self.quit,globalsout=packed(self.number(address) for address in GLOBALS),
                    devicesout=bytes(self.mu.mem_read(ARENA,ARENA_SIZE)).hex(),readout=self.number(gate.READ),writeout=self.number(gate.WRITE),
                    physicalqueue=bytes(self.mu.mem_read(gate.QUEUE,480)).hex(),bodytrace=';'.join(self.body) or '-',
                    envelopetrace=';'.join(self.envelope) or '-',stackdelta=self.mu.reg_read(UC_X86_REG_ESP)-gate.STACK,nativeinstructions=self.instructions)
        for low,expected in self.original.items():  # 모든 입력 뒤 실제 실행 범위의 불변을 확인한다.
            if bytes(self.mu.mem_read(low,len(expected)))!=expected:
                raise RuntimeError('보호 본체 변경')
        total=self.totals.setdefault(f'{control:03x}',dict(runs=0,instructions=0))
        total['runs']+=1
        total['instructions']+=self.instructions
        return output


def cases():
    """모드/반환/부호/큐/경계 재진입을 기대값 계산 없이 입력으로만 생성한다."""
    rows=[]  # 중복을 제거할 필요 없는 명시 입력 행이다.
    arena=bytearray(ARENA_SIZE)  # Player/Form/SID 목록의 실제 입력이다.
    for owner in range(3):  # 세 실제 Player의 두 Home SID를 구별한다.
        struct.pack_into('<I',arena,owner*0xb4+0x7c,10+owner)
        struct.pack_into('<I',arena,owner*0xb4+0x84,20+owner)
    for sid in range(8):  # 원문 Form의 비정렬 float 두 DWORD를 고유하게 둔다.
        struct.pack_into('<ff',arena,0x400+sid*50+0xe,sid*3.25,-sid*5.5)
    struct.pack_into('<4I',arena,0x600,1,2,3,4)
    struct.pack_into('<4I',arena,0x640,4,5,6,7)
    defaults=[0,0,0,1,1,1,0,7,0,0,2,2,PLAYERS,POOL,FIRST,SECOND]  # 글로벌 입력의 고정 순서다.
    image=gate.pefile.PE(str(gate.PE_PATH)).get_memory_mapped_image()  # ID를 Python 기대 매핑 없이 원문에서 읽는다.
    ids=[int.from_bytes(image[address-gate.BASE:address-gate.BASE+4],'little') for address in COMMANDS]

    def add(name,command,second=99,changes=None,**options):
        """결과가 없는 현재 전역/원시 바이트/외부 효과만 등록한다."""
        globals_=defaults.copy()
        for index,value in (changes or {}).items():  # 지정한 현재 입력 필드만 변경한다.
            globals_[index]=value
        case=dict(name=name,event=packed((command,23,second)),guideResult=0,gumpResult=0,globals=packed(globals_),devices=arena.hex(),read=0,filled=0,mutation='-',stop='-')
        case.update(options)
        rows.append(case)

    for command in ids:  # 모든 명령이 실제 Guide/Gump/release 관문을 통과하는지 확인한다.
        for guide,gump,flag in itertools.product((0,1,0xffffffff),(0,1),(0,0x80000000,0x38000000)):
            add(f'gate-{command}-{guide}-{gump}-{flag}',command|flag,guideResult=guide,gumpResult=gump)
    for value in (0,1,98,99,100,0xffffffff):  # QuitApp 진단 복귀/부분 중단을 구별한다.
        add(f'quit-{value}',ids[0],value)
        if value!=99:
            add(f'quitstop-{value}',ids[0],value,stop='assert1')
    for editor,fort,meta,read,filled in itertools.product((0,1),(0,1),(0,1),(0,39),(0,39)):
        add(f'request-{editor}-{fort}-{meta}-{read}-{filled}',ids[1],changes={0:editor,1:fort,2:meta},read=read,filled=filled)
    for index,value in itertools.product((2,3),(0,1,50,100,101,0x7fffffff,0x80000000,0xffffffff)):
        add(f'volume-{index}-{value}',ids[index],value)
    for mission,editor,depth in itertools.product((0,1),(0,1),(0,1,2,0xffffffff,0x80000000)):
        add(f'pause-{mission}-{editor}-{depth}',ids[4],changes={0:editor,3:mission,8:depth})
    for known,meta,owner in itertools.product((0,1),(0,1),(0,1,2)):
        add(f'home-{known}-{meta}-{owner}',ids[5],changes={2:meta,4:known,5:owner})
    for owner in (0,1,2,0xffffffff):  # Priest의 무조건 전달 폭을 관측한다.
        add(f'priest-{owner}',ids[6],owner)
    for hide in (0,1,2,0xffffffff):  # Hide의 논리 반전과 redraw 쓰기를 구별한다.
        add(f'hide-{hide}',ids[7],changes={6:hide})
    for meta,first,second in itertools.product((0,1),(-1,0,1,2),(-1,0,2)):
        add(f'reload-{meta}-{first}-{second}',ids[8],changes={2:meta,10:first,11:second})
    mutations=[('home-owner',ids[5],{5:0},f'assert1/{0x540c70}/2'),
               ('reload-count',ids[8],{},f'reload1/{0x55a714}/1;reload1/{0x5e18a4}/0'),
               ('reload-list',ids[8],{},f'reload1/{FIRST+4}/7'),
               ('reload-pool',ids[8],{},f'reload1/{0x5c8464}/{POOL+50}'),
               ('reload-coordinates',ids[8],{},f'reload1/{POOL+2*50+0xe}/1065353216'),
               ('pause-depth',ids[4],{},f'status1/{gate.PAUSE}/5'),
               ('request-mode',ids[1],{},f'mission1/{0x5c85a4}/1'),
               ('gate-hide',ids[7],{},f'gump1/{0x59a8ac}/9')]
    for name,command,changes,mutation in mutations:  # 경계 뒤의 현재값 재조회를 관측한다.
        add(name,command,changes=changes,mutation=mutation)
    for name,index,stop in (('quit-sound',0,'sound1'),('tell',1,'tell1'),('config',2,'config1'),('pause-status',4,'status1'),('pause-cancel',4,'cancel1'),('pause-clock',4,'pause1'),('reload-first',8,'reload1'),('reload-second',8,'reload3')):
        add('stop-'+name,ids[index],stop=stop)
    add('home-fallback',ids[5],devices=(arena[:0xb4+0x7c]+bytes(4)+arena[0xb4+0x80:]).hex())
    add('home-empty',ids[5],devices=(arena[:0xb4+0x7c]+bytes(12)+arena[0xb4+0x88:]).hex())
    add('menujustclosed',0x20000be)
    add('unknown',0x20003fe)
    return rows


def main():
    """호스트/보호 SHA를 고정하고 두 제어어의 독립 관측만 새 파일에 저장한다."""
    if socket.gethostname().lower()!='vm-w11-codex' or gate.digest(gate.PE_PATH)!=gate.PE_SHA:
        raise RuntimeError('허용 호스트/보호 PE 불일치')
    oracle,inputs,rows=Oracle(),cases(),[]  # 동일 원문에서 독립 실행할 입력들이다.
    for index,case in enumerate(inputs):  # 각 입력의 두 정밀도에서 모든 관측을 비교한다.
        previous=None
        for control in gate.CONTROLS:  # x87 53/64비트 제어어를 각각 설치한다.
            try:
                output=oracle.run(case,control)
            except Exception as error:
                raise RuntimeError(case['name']+': '+str(error)) from error
            if previous is not None and previous!=output:
                raise RuntimeError('정밀도 차이 '+case['name'])
            previous=output
            values=dict(case,name=case['name']+f'-cw{control:03x}',**output)
            rows.append('\t'.join(str(values[field]) for field in SCHEMA))
        if index%100==0:
            print('observed',index,flush=True)
    FIXTURE.write_text('# '+'\t'.join(SCHEMA)+'\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    record=dict(host=socket.gethostname(),lastDecompileHost='VM-W11-Codex / 2026-10-23',inputs=len(inputs),observations=len(rows),schema=SCHEMA,
                precisions=oracle.totals,precisionDifferences=0,boundaries=dict(oracle.boundaries),cpuWrites=dict(oracle.writes),globalAddresses=[hex(a) for a in GLOBALS],
                commandAddresses=[hex(a) for a in COMMANDS],nativeRanges=[[hex(a),hex(b)] for a,b in gate.RANGES+EXTRA],
                scope='생산 단축키 관측기의 실제004d6e61 현재 사건 Guide 준비에서 시작한다. 실제Guide/release/일반명령/실제큐452fe0/일반tail을 실행한다. QuitApp은실제SEH복구와RET도실행한다.',
                limits=['전체004d62b0함수 진입/OS폴링/큐Pop은 실행하지 않는다. 이미 확보한현재사건지역프레임/EDI0/상위프레임은 입력 경계다.',
                        'Guide가상/Gump/일반입력/형식화/Config/Tell/오디오/Clock변경/커서취소/카메라/Reload/진단은 명시 외부 장치다. 형식화와 주소 getter는순수경계라bodytrace에추가하지않는다.',
                        'Clock경계는명시pauseDepth++/비영--장치를먼저적용한다. 실제시각누적과오디오재생/실제GUI효과를완성했다고주장하지않는다.',
                        '명령전역9개는보호PE의서로다른불변ID다. 01000000은대상Form가상분배를뜻하므로이번메뉴관측에서제외한다.',
                        '미완료부분행의EAX는반환값이아니다. 정상일반행은현재사건종료라벨,QuitApp만실제RET의EAX1을관측한다.'],
                gameExecuted=False,guiExecuted=False,
                sha256={str(path.relative_to(ROOT)).replace('\\','/'):gate.digest(path) for path in (gate.PE_PATH,ROOT/'extracted/refined/originals/Netstorm.c',Path(__file__),FIXTURE)})
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({key:record[key] for key in ('inputs','observations','precisionDifferences')}),flush=True)


if __name__=='__main__':
    main()
