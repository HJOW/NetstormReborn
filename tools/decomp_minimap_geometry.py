#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 10.78 미니맵의 기하 본체와 CRT를 실제 실행해 새 독립 관측을 남긴다."""
import hashlib
import itertools
import json
import socket
import struct
from collections import Counter
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

ROOT=Path(__file__).resolve().parents[1]  # 보호 원문과 새 산출물의 공통 루트다.
PE_PATH=ROOT/'originals/Netstorm.exe'  # 변경하지 않는 실제 10.78 입력이다.
PE_SHA='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 분석 원문을 고정하는 식별자다.
FIXTURE=ROOT/'cpppj/tests/fixtures/minimap-geometry-x86.tsv'  # 제품 결과와 독립적인 새 실행 자료다.
REPORT=ROOT/'extracted/cpp-minimap-geometry-20261023-native.json'  # 명령/경계/해시의 새 기록이다.
BASE=0x400000  # 원본 PE의 로드 기준이다.
HEAP=0x1000000  # 명시 장치와 raw 미니맵의 격리 주소다.
NODE=HEAP+0x100  # 실제0xacBYTE 미니맵 객체다.
VTABLE=HEAP+0x1000  # Dirty 가상 호출만 공급하는 표다.
DIRTY=HEAP+0x1800  # 명시 가상+8 진입점이다.
RETURN=HEAP+0x2000  # 정상 RET 관측 종료점이다.
BITMAP=HEAP+0x4000  # Draw에 주소만 전달하는128x128 원시 비트맵이다.
STACK=0x2100000  # 온전한 thiscall 진입 스택이다.
CONTROLS=(0x27F,0x37F)  # x87 double/extended 두 제어어다.
RANGES={'rebuild':(0x480270,0x4803E5),'move':(0x4803F0,0x480439),'idle':(0x480440,0x4805D6),'attach':(0x4805E0,0x480654),'draw':(0x480660,0x48080E)}  # 실제 본체의 RET까지다.
CRT=(0x4E49C0,0x4E4A35)  # 실제 signed64 절삭 CRT다.
GLOBALS={'cameraXBits':0x59A900,'cameraYBits':0x59A904,'pixelsX':0x59A908,'pixelsY':0x59A90C,'left':0x5CA99C,'top':0x5CA9A0,'viewWidth':0x5CA9B4,'viewHeight':0x5CA9B8}  # 본체가 현재 읽는 실제 전역이다.
STATE={'widthBits':0x3C,'heightBits':0x40,'cropX':0xA0,'cropY':0xA4,'scrolling':0xA8}  # 실제 미니맵 필드다.
BOUNDARIES={DIRTY:'dirty',0x452E00:'poll',0x4650A0:'local',0x4648E0:'bounds',0x464200:'clip',0x464290:'clipEnd',0x4A1FE0:'blit',0x4A24B0:'frame',0x497F20:'center',0x466750:'baseAttach',0x4E0620:'assert'}  # CRT 외의 명시 외부 경계다.
INPUTS=('name','operation','widthBits','heightBits','cropX','cropY','scrolling','cameraXBits','cameraYBits','left','top','right','bottom','pixelsX','pixelsY','mouseXBits','mouseYBits','destX','destY','mutation')  # 앞20열만 독립 실행 입력이다.
OUTPUTS=('completed','stateout','globalsout','trace','stackdelta','nativeinstructions','crtinstructions')  # 뒤7열은 실제 CPU/장치 관측이다.


def sha(path):
    """원문이나 새 산출물의 바이트 식별자를 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bits(value):
    """float32 입력의 실제 DWORD 비트를 만든다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def signed(value):
    """DWORD 비트를 변경하지 않고 signed32로 읽는다."""
    return struct.unpack('<i',struct.pack('<I',value&0xFFFFFFFF))[0]


class Oracle:
    """현재 raw 상태와 입력 장치만 주고 실제 PE의 기하 계산을 관측한다."""
    def __init__(self):
        """원문 PE와 격리 스택/FS0/객체를 준비한다."""
        pe=pefile.PE(str(PE_PATH))  # 읽기 전용 보호 입력의 파서다.
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32)  # 실제32BIT CPU다.
        self.mu.mem_map(BASE,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);self.mu.mem_write(BASE,pe.get_memory_mapped_image())
        self.mu.mem_map(0,4096);self.mu.mem_map(HEAP,0x10000);self.mu.mem_map(STACK-0x10000,0x20000)
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.memory_write)
        self.original={first:bytes(self.mu.mem_read(first,last-first)) for first,last in (*RANGES.values(),CRT)}  # 실제 본체 불변 확인용이다.
        self.totals={};self.calls_total=Counter();self.writes=Counter()  # 정밀도별 실제 명령/외부 호출/원문 쓰기다.

    def number(self,address,value=None):
        """현재 DWORD를 읽거나 입력/명시 경계 효과를 기록한다."""
        if value is not None:self.mu.mem_write(address,struct.pack('<I',value&0xFFFFFFFF))
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def returned(self,value=0,cleanup=0):
        """실제 CALL의 복귀주소와 thiscall 인자만 소비한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP)  # 경계의 실제 진입 스택이다.
        self.mu.reg_write(UC_X86_REG_EIP,self.number(esp));self.mu.reg_write(UC_X86_REG_ESP,esp+4+cleanup);self.mu.reg_write(UC_X86_REG_EAX,value&0xFFFFFFFF)

    def event(self,name,text):
        """실제 호출 순서를 적고 현재 경계의 명시 효과만 공급한다."""
        self.trace.append(text);self.calls[name]+=1;self.calls_total[name]+=1
        trigger=name+str(self.calls[name])  # 같은 종류의1기반 호출 번호다.
        for event,target,field,value in self.effects:  # 명시한 변경을 원문의 호출 시점에서 입력 순서대로 적용한다.
            if event!=trigger:continue
            if target=='state':self.number(NODE+STATE[field],int(value))
            elif target=='global':self.number(GLOBALS[field],int(value))
            elif target=='control' and field=='stop' and value=='1':self.stopped=True;self.mu.emu_stop()
            else:raise RuntimeError('미명시 미니맵 callback 효과')

    def memory_write(self,mu,access,address,size,value,data):
        """실제 본체 쓰기를 raw 객체/스택/SEH 슬롯으로 제한한다."""
        if STACK-0x2000<=address and address+size<=STACK+40:key='stack'
        elif NODE<=address and address+size<=NODE+0xAC:key='state'
        elif address==0 and size==4:key='seh'
        else:raise RuntimeError(f'미명시 본체 쓰기 {address:08x}/{size}')
        self.writes[key]+=1

    def instruction(self,mu,address,size,data):
        """선택 본체/CRT는 실행하고 명시 외부 함수만 대체한다."""
        if address==RETURN:self.completed=True;mu.emu_stop();return
        if any(first<=address<last for first,last in (RANGES[self.kind],CRT)):
            self.instructions+=1
            if CRT[0]<=address<CRT[1]:self.crt+=1
            return
        if address not in BOUNDARIES:raise RuntimeError(f'미명시 코드 {address:08x}')
        name=BOUNDARIES[address];esp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX)  # 원문이 전달한 실제 인자다.
        if name=='dirty':
            if this!=NODE:raise RuntimeError('Dirty의 실제 this 불일치')
            self.event(name,'dirty:'+str(self.number(esp+4)))
            if not self.stopped:self.returned(cleanup=4)
        elif name=='poll':
            output=self.number(esp+4);self.mu.mem_write(output,struct.pack('<III',0xFFFF,0,0));self.event(name,'poll:'+str(self.number(esp+8)))
            if not self.stopped:self.returned()
        elif name=='local':
            output=self.number(esp+4);self.mu.mem_write(output,struct.pack('<II',self.case['mouseXBits'],self.case['mouseYBits']));self.event(name,'local:'+str(self.number(esp+8))+':'+str(self.number(esp+12))+':'+str(self.number(esp+16)))
            if not self.stopped:self.returned(output,16)
        elif name=='bounds':
            x=self.case['destX'];y=self.case['destY'];w=struct.unpack('<f',struct.pack('<I',self.number(NODE+0x3C)))[0];h=struct.unpack('<f',struct.pack('<I',self.number(NODE+0x40)))[0]  # 명시 Bounds 장치의 정수 반환이다.
            rect=(x,y,x+int(w),y+int(h));self.mu.mem_write(self.number(esp+4),struct.pack('<4i',*rect));self.event(name,'bounds:'+':'.join(map(str,rect)))
            if not self.stopped:self.returned(cleanup=4)
        elif name=='clip':
            values=[signed(self.number(esp+offset)) for offset in (4,8,12,16,20)];self.event(name,'clip:'+':'.join(map(str,values)))
            if not self.stopped:self.returned(this,20)
        elif name=='clipEnd':
            self.event(name,'clipEnd')
            if not self.stopped:self.returned()
        elif name=='blit':
            values=[self.number(esp+offset) for offset in range(4,36,4)];values[2]=signed(values[2]-BITMAP);values=[signed(value) for value in values];self.event(name,'blit:'+':'.join(map(str,values)))
            if not self.stopped:self.returned()
        elif name=='frame':
            values=[signed(self.number(esp+offset)) for offset in (4,8,12,16,20)];self.event(name,'frame:'+':'.join(map(str,values)))
            if not self.stopped:self.returned()
        elif name=='center':
            self.event(name,'center:'+str(self.number(esp+4))+':'+str(self.number(esp+8)))
            if not self.stopped:self.returned(cleanup=8)
        elif name=='baseAttach':
            self.event(name,'baseAttach:'+':'.join(str(self.number(esp+offset)) for offset in (4,8,12,16)))
            if not self.stopped:self.returned(cleanup=16)
        else:
            self.event(name,'assert:'+str(self.number(esp+12)))
            if not self.stopped:self.returned()

    def run(self,case,control):
        """독립 입력을 실제 진입에서 정상 RET 또는 명시 중단까지 실행한다."""
        self.case=case;self.kind=case['operation'];self.trace=[];self.calls=Counter();self.completed=False;self.stopped=False;self.instructions=0;self.crt=0  # 이번 원문 관측 상태다.
        self.effects=[] if case['mutation']=='-' else [entry.split('/') for entry in case['mutation'].split(';')]  # 입력에만 있는 효과다.
        self.mu.mem_write(STACK-0x2000,bytes([0xCC])*0x2040);self.mu.mem_write(NODE,bytes(0xAC));self.number(0,0)
        self.number(NODE,VTABLE);self.number(VTABLE+8,DIRTY);self.number(0x5949CC,128);self.number(0x5949C4,BITMAP);self.number(0x59A8BC,HEAP+0x3000);self.number(0x5BEC2C,155)
        for name,offset in STATE.items():self.number(NODE+offset,case[name])  # 실제 raw 필드의 입력을 설치한다.
        for name,address in GLOBALS.items():self.number(address,(case['right']-case['left']) if name=='viewWidth' else (case['bottom']-case['top']) if name=='viewHeight' else case[name])  # 화면 사각형은 원문 origin/크기로 배치한다.
        arguments=[RETURN]
        if self.kind=='move':arguments.extend((case['mouseXBits'],case['mouseYBits']))
        if self.kind=='attach':arguments.extend((bits(case['destX']),bits(case['destY']),HEAP+0x3000,0))
        self.mu.mem_write(STACK,struct.pack('<'+'I'*len(arguments),*arguments))
        registers={UC_X86_REG_EAX:0x10111213,UC_X86_REG_EBX:0x20212223,UC_X86_REG_ECX:NODE,UC_X86_REG_EDX:0x40414243,UC_X86_REG_ESI:0x50515253,UC_X86_REG_EDI:0x60616263,UC_X86_REG_EBP:0x70717273,UC_X86_REG_ESP:STACK,UC_X86_REG_FPCW:control,UC_X86_REG_FPSW:0}  # 독립 thiscall 진입 레지스터다.
        for register,value in registers.items():self.mu.reg_write(register,value)  # 이전 실행의 잔여를 제거한다.
        self.mu.emu_start(RANGES[self.kind][0],RETURN+1,count=20000)
        cleanup=8 if self.kind=='move' else 16 if self.kind=='attach' else 0  # 원본 thiscall RET가 소비한 인자 수다.
        delta=self.mu.reg_read(UC_X86_REG_ESP)-STACK-4-cleanup
        if not self.completed and not self.stopped:raise RuntimeError('정상 종료/명시 중단 없는 관측')
        if self.completed:
            if delta or self.number(0):raise RuntimeError('정상 RET의 스택/SEH 차이')
            for register in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 실제 callee-save를 확인한다.
                if self.mu.reg_read(register)!=registers[register]:raise RuntimeError('callee-save 변경')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('x87 제어어/스택 차이')
        for first,raw in self.original.items():  # 실행한 원문을 변경하지 않았는지 확인한다.
            if bytes(self.mu.mem_read(first,len(raw)))!=raw:raise RuntimeError('실제 본체 바이트 변경')
        state=':'.join(str(self.number(NODE+offset)) for offset in STATE.values());left=self.number(GLOBALS['left']);top=self.number(GLOBALS['top'])  # 실제 현재 상태의 원시 출력이다.
        globalsout=':'.join(map(str,(self.number(GLOBALS['cameraXBits']),self.number(GLOBALS['cameraYBits']),left,top,(left+self.number(GLOBALS['viewWidth']))&0xFFFFFFFF,(top+self.number(GLOBALS['viewHeight']))&0xFFFFFFFF,self.number(GLOBALS['pixelsX']),self.number(GLOBALS['pixelsY']))))
        output=dict(completed=int(self.completed),stateout=state,globalsout=globalsout,trace=';'.join(self.trace),stackdelta=delta,nativeinstructions=self.instructions,crtinstructions=self.crt)
        total=self.totals.setdefault(self.kind+f'-{control:x}',Counter());total.update(runs=1,nativeinstructions=self.instructions,crtinstructions=self.crt)
        return output


def cases():
    """정상 HUD와 crop/edge/callback 기하 경계를 독립적으로 교차한다."""
    base=dict(widthBits=bits(80),heightBits=bits(75),cropX=0,cropY=0,scrolling=0,cameraXBits=bits(0),cameraYBits=bits(0),left=85,top=0,right=1024,bottom=672,pixelsX=16,pixelsY=11,mouseXBits=bits(40),mouseYBits=bits(37),destX=0,destY=693,mutation='-')  # 해상도1024x768 기본 분할의 crop와 별도 viewport다.
    rows=[]  # 기대값 없이 만든 독립 입력 목록이다.
    for operation,dimensions,camera,crop in itertools.product(('rebuild','draw','attach'),((80,75),(128,128),(200,160),(0,0),(-5,-3),(79.5,74.25)),(-31.9,0,20.5,128.9,255.9),(-4,0,48,100)):  # 정수/소수/음수/큰 창과 현재 카메라를 교차한다.
        rows.append(dict(base,name=f'{operation}-{len(rows):04d}',operation=operation,widthBits=bits(dimensions[0]),heightBits=bits(dimensions[1]),cameraXBits=bits(camera),cameraYBits=bits(camera+7.25),cropX=crop,cropY=crop))
    for x,y,crop in itertools.product((-1,0,79.9,80,81),(-1,0,73.9,74,75,76),(0,48,100)):  # 좌/상단 <=0, 우측 >=width와 하단 >=height-1의 끝점 포함 관문이다.
        rows.append(dict(base,name=f'idle-{len(rows):04d}',operation='idle',scrolling=1,cropX=crop,cropY=crop,mouseXBits=bits(x),mouseYBits=bits(y)))
    for scrolling in (0,1):  # 재생성 억제와 Idle 진단의 별도 의미를 관측한다.
        for operation in ('rebuild','idle','move'):rows.append(dict(base,name=f'scrolling-{operation}-{scrolling}',operation=operation,scrolling=scrolling,cropX=23,cropY=17))  # Move는 scrolling과 무관하게 같은 합산을 수행한다.
    for left,top,right,bottom,pixelX,pixelY in ((0,0,640,480,16,11),(85,25,800,504,16,11),(-85,-25,1024,768,16,11),(85,0,1680,954,24,17),(85,0,1280,672,8,5)):  # 같은 원문 origin/폭 관계와 signed 나눗셈을 확인한다.
        for operation in ('rebuild','draw'):rows.append(dict(base,name=f'viewport-{len(rows):04d}',operation=operation,left=left,top=top,right=right,bottom=bottom,pixelsX=pixelX,pixelsY=pixelY,cameraXBits=bits(-7.9),cameraYBits=bits(99.9)))
    effects=(('rebuild','dirty1/state/cropX/17'),('idle','poll1/state/cropX/20'),('idle','local1/state/widthBits/'+str(bits(40))),('idle','center1/state/cropY/9'),('idle','assert1/state/scrolling/1'),('idle','assert1/control/stop/1'),('draw','bounds1/state/cropX/23'),('draw','clip1/state/cropY/19'),('draw','blit1/state/cropX/31'),('draw','blit1/global/cameraXBits/'+str(bits(70.5))),('draw','frame1/state/cropY/7'),('draw','blit1/control/stop/1'),('move','center1/state/cropX/7'))  # 명시 callback 뒤의 현재 raw/global 재조회를 구별한다.
    for operation,effect in effects:rows.append(dict(base,name=f'effect-{len(rows):04d}',operation=operation,mutation=effect))  # 효과는 원문 계산이 아닌 명시 외부 장치다.
    for crop,local in itertools.product((16777217,-16777217,2147483647,-2147483648),(1.0,1.5,-1.5,0.0001)):  # FILD를 먼저float32로 좁히거나 덧셈을 조기 저장하는 차이를 드러낸다.
        for operation in ('move','idle','attach'):rows.append(dict(base,name=f'wide-crop-{len(rows):04d}',operation=operation,cropX=crop,cropY=crop,scrolling=1,mouseXBits=bits(local),mouseYBits=bits(local)))  # 물리 그림 접근 없이 실제 정수/실수 기하만 관측한다.
    return rows


def main():
    """허용한 새 도구/TSV/보고서만 만들고 원문 식별자와 정밀도 차이를 확인한다."""
    if socket.gethostname().lower()!='vm-w11-codex' or sha(PE_PATH)!=PE_SHA:raise RuntimeError('호스트/원본 PE 식별자 불일치')
    oracle=Oracle();rows=[];inputs=cases();differences=[]  # 실제 실행과 현재 입력만 소유한다.
    for case in inputs:  # 모든 입력을 두 실제 제어어로 별도 실행한다.
        previous=None
        for control in CONTROLS:  # 합성 기대값이나 제품 코드를 사용하지 않는다.
            try:output=oracle.run(case,control)
            except Exception as error:raise RuntimeError(case['name']) from error
            if previous is not None and previous!=output:differences.append(case['name'])
            previous=output;values=dict(case,name=case['name']+f'-cw{control:x}',**output);rows.append('\t'.join(str(values[key]) for key in (*INPUTS,*OUTPUTS)))
    FIXTURE.write_text('# 실제 10.78 미니맵/CRT 독립 관측. 앞20열 입력, 뒤7열 실행 결과.\n#'+'\t'.join((*INPUTS,*OUTPUTS))+'\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    report=dict(host=socket.gethostname(),date='2026-10-23',inputs=len(inputs),observations=len(rows),schema=[*INPUTS,*OUTPUTS],precision_differences=differences,native_totals=oracle.totals,boundaries={f'{address:08x}':name for address,name in BOUNDARIES.items()},boundary_calls=oracle.calls_total,native_writes=oracle.writes,native_ranges=[dict(first=f'{first:08x}',end_exclusive=f'{last:08x}',sha256=hashlib.sha256(oracle.original[first]).hexdigest()) for first,last in (*RANGES.values(),CRT)],sha256={str(path.relative_to(ROOT)).replace('\\','/'):sha(path) for path in (PE_PATH,Path(__file__),FIXTURE)},scope='생성/토글/전체dock/셀래스터/GUI는 정적 근거 또는 별도 작업이다. bounds는 입력dest와현재width/height의절삭을 반환하는 명시 장치다. clip/blit/frame/center/기본Attach/Dirty는 효과 경계이며 실제 픽셀을 그리지 않는다.',mutation='trigger/target/field/decimalValue. 종류별1기반 경계에서 state/global만 변경하며 control/stop/1은 부분자료를 남기고 중단한다.',new_game_runs=0,new_decompilations=0)
    report['instruction_counting']='nativeinstructions는선택PE본문과실제CRT명령의합계이며crtinstructions는그부분집합이다. 정상RET는callee-save/ESP/FS0/x87제어어/TOP0을검증한다.'
    report['device_contract']='bitmap폭128/카메라outline색155를명시장치입력으로둔다. outline색155가원본고정색이라는뜻이아니다. Poll은(ffff,0,0),local은입력mouse floatBits를반환한다. Bounds는현재width/height절삭과dest를반환하고clip은순수관측장치다. callback은경계인자를먼저캡처한뒤효과를적용하며다음본문읽기는현재자료를쓴다.'
    report['static_findings']={'constructor':'004801b0/Netstorm.c103754:flags301,gt=movsx BYTE[00540438](보호초기29),VT0050d51c,crop0/0/scrolling0,title Sky Overview/help minimapGump,serial005949dc. 공유bitmap소유권은없다.','lifetime':'VT+0=004d4c40→thunk00464420→flags&1이면free. 토글480810은Kill464480후global null, 재생성시root100/100 Attach후fullRefresh++. 정상HUD는4815b0→4c7160의별도생성이다.','normal_hud':'Netstorm.c158491~158502/158532/158536,156660~156705,157952~157982:기본명목128^2를세로divider x80/폭5와왼쪽아래divider y=H-80/높이5의slave flag1/8로붙이면crop창(0,H-75),80x75다. 전체dockdrag는실행하지않은정적경계다.','viewport':'PE004c689c~004c6927:mapLeft=ftol(vertical.x+vertical.width),mapWidth=W-mapLeft,mapHeight=ftol(chatDivider.y). chatDivider는왼쪽아래미니맵divider와다르며초기y=H-96이다. 현재top005ca9a0과합쳐LTRB를구성한다.','frame':'Draw는8bitbitmap crop을현재Gump화면원점으로복사한뒤viewport카메라outline을그린다. 4a24b0인자는exclusive right/bottom이며그함수가각각1을뺀뒤네선을그린다.'}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    if sha(PE_PATH)!=PE_SHA:raise RuntimeError('보호 원문 변경')
    print(json.dumps(dict(inputs=len(inputs),observations=len(rows),precision_differences=differences)))


if __name__=='__main__':main()
