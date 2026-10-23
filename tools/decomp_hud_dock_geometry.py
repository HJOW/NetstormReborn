#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78 HUD divider의 생성/등록/재배치와 viewport를 실제PE로 관측한다."""
import hashlib
import itertools
import json
import socket
import struct
from collections import Counter
from pathlib import Path
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW

ROOT=Path(__file__).resolve().parents[1]  # 보호입력과 새 산출물의 루트다.
PE_PATH=ROOT/'originals/Netstorm.exe'  # 읽기만 하는 실제10.78이다.
PE_SHA='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 원문식별자다.
FIXTURE=ROOT/'cpppj/tests/fixtures/hud-dock-geometry-x86.tsv'  # 독립현재메모리관측이다.
REPORT=ROOT/'extracted/cpp-hud-dock-geometry-20261023-native.json'  # 실제명령/경계와해시다.
BASE=0x400000  # PE로드기준이다.
HEAP=0x1000000  # 격리노드/가상표의영역이다.
NODES=HEAP+0x100  # 고정8개0x11cBYTE노드의시작이다.
STRIDE=0x200  # 노드간주소간격이다.
VT=HEAP+0x3000  # 명시가상표영역이다.
DETACH=HEAP+0x4000  # 가상Push명시경계다.
ATTACH=HEAP+0x4010  # 일반UI의가상Attach경계다.
RETURN=HEAP+0x5000  # 정상RET관측점이다.
STACK=0x2100000  # thiscall스택이다.
CONTROLS=(0x27f,0x37f)  # 실제x87두정밀도다.
RANGES={'register':(0x4c4790,0x4c47ec),'reflow':(0x4c47f0,0x4c49f0),'construct':(0x4c6c50,0x4c6cf3),'hud':(0x4c6530,0x4c6a36),'move':(0x464d10,0x464d56)}  # 정상RET까지의실제기계어다.
CRT=(0x4e49c0,0x4e4a35)  # 실제signed64절삭이다.
GLOBALS={'screenWidth':0x5c78f0,'screenHeight':0x5c78f4,'showHud':0x54db14,'mapLeft':0x5ca99c,'mapTop':0x5ca9a0,'mapWidth':0x5ca9b4,'mapHeight':0x5ca9b8,'floatLeft':0x5ca9bc,'floatTop':0x5ca9c0,'floatRight':0x5ca9c4,'floatBottom':0x5ca9c8,'rectLeft':0x5ca9cc,'rectTop':0x5ca9d0,'rectRight':0x5ca9d4,'rectBottom':0x5ca9d8,'bottomLeft':0x5ca9a4,'bottomTop':0x5ca9a8,'bottomRight':0x5ca9ac,'bottomBottom':0x5ca9b0,'fullRefresh':0x5949d4,'savedChatY':0x542674,'presentMask':HEAP+0x6000}  # 마지막mask는 순수serial조회 장치의 입력이다.
FIELDS={'flags':4,'parent':0x14,'type':0x20,'serial':0x2c,'scale':0x38,'baseWidth':0x3c,'baseHeight':0x40,'width':0x44,'height':0x48,'x':0x4c,'y':0x50,'axis':0xb8,'minX':0xbc,'maxX':0xc0,'minY':0xc4,'maxY':0xc8,'link0serial':0xcc,'link0flags':0xd0,'link1serial':0xd4,'link1flags':0xd8,'vertical':0xa0,'minimapDivider':0xa4,'chatDivider':0xa8,'minimap':0xac,'production':0xb0,'chat':0xb8,'knowledge':0xc0}  # raw필드mutation문법이다.
BOUNDARIES={0x466710:'closePopups',0x466750:'baseAttach',0x464e90:'bubble',0x464020:'resolve',DETACH:'push',ATTACH:'attach',0x4655d0:'background',0x4653c0:'activate',0x499370:'invalidate',0x4973c0:'finish',0x464990:'baseHandle',0x42f250:'chatVisible',0x42f270:'chatInput',0x4e0620:'assert'}  # 데이터장치/공통UI의명시외부경계다.
INPUTS=('name','operation','nodes','globals','arguments','mutation')  # 이6열만원문실행입력이다.
OUTPUTS=('completed','nodesout','globalsout','trace','result','stackdelta','nativeinstructions','crtinstructions')  # 뒤8열은실제CPU관측이다.


def number(value):
    """입력DWORD를음수도보존해직렬화한다."""
    return value&0xffffffff


def bits(value):
    """float32의원시DWORD다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def signed(value):
    """DWORD를signed32로읽는다."""
    return struct.unpack('<i',struct.pack('<I',number(value)))[0]


def node(index):
    """고정노드번호의실제격리주소다."""
    return NODES+index*STRIDE


class Oracle:
    """외부공통Gump처리만명시하고실제분기/필드/CRT를실행한다."""
    def __init__(self):
        """원문과격리메모리/훅을마련한다."""
        pe=pefile.PE(str(PE_PATH));self.mu=Uc(UC_ARCH_X86,UC_MODE_32)
        self.mu.mem_map(BASE,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);self.mu.mem_write(BASE,pe.get_memory_mapped_image())
        self.mu.mem_map(0,4096);self.mu.mem_map(HEAP,0x10000);self.mu.mem_map(STACK-0x10000,0x20000)
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.memory_write)
        self.original={a:bytes(self.mu.mem_read(a,b-a)) for a,b in (*RANGES.values(),CRT)}  # 원문코드불변검사다.
        self.totals=Counter();self.calls_total=Counter();self.writes=Counter()  # 실제누적실행관측이다.

    def dword(self,address,value=None):
        """현재메모리의DWORD를조회/입력한다."""
        if value is not None:self.mu.mem_write(address,struct.pack('<I',number(value)))
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def token(self,address):
        """로그의실제노드주소만1기반토큰으로바꾼다."""
        if not address:return 0
        if NODES<=address<NODES+8*STRIDE and (address-NODES)%STRIDE==0:return (address-NODES)//STRIDE+1
        raise RuntimeError('미명시노드주소 '+hex(address))

    def returned(self,value=0,cleanup=0):
        """실제CALL복귀주소와thiscall인자를소비한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EIP,self.dword(esp));self.mu.reg_write(UC_X86_REG_ESP,esp+4+cleanup);self.mu.reg_write(UC_X86_REG_EAX,number(value))

    def event(self,name,values=()):
        """현재호출과기본장치효과뒤명시한mutation을순서대로적용한다."""
        self.trace.append(name+(':'+':'.join(str(number(x)) for x in values) if values else ''));self.calls[name]+=1;self.calls_total[name]+=1
        trigger=name+str(self.calls[name])  # 같은경계의1기반호출번호다.
        for when,target,field,value in self.effects:  # 한경계의여러효과는입력순서대로적용한다.
            if when!=trigger:continue
            if target.startswith('node'):self.dword(node(int(target[4:]))+FIELDS[field],int(value))
            elif target=='global':self.dword(GLOBALS[field],int(value))
            elif target=='control' and field=='stop' and value=='1':self.stopped=True;self.mu.emu_stop()
            else:raise RuntimeError('미명시callback효과')

    def memory_write(self,mu,access,address,size,value,data):
        """실제본체쓰기를8노드/현재전역/스택/SEH로제한한다."""
        if STACK-0x4000<=address and address+size<=STACK+40:key='stack'
        elif any(node(i)<=address and address+size<=node(i)+0x11c for i in range(8)):key='nodes'
        elif any(a<=address and address+size<=a+4 for a in GLOBALS.values()):key='globals'
        elif address==0 and size==4:key='seh'
        else:raise RuntimeError('미명시실제쓰기 %08x/%d'%(address,size))
        self.writes[key]+=1

    def instruction(self,mu,address,size,data):
        """선택본체와중첩Move/reflow/HUD및CRT는실제명령으로진행한다."""
        if address==RETURN:self.completed=True;mu.emu_stop();return
        if any(a<=address<b for a,b in (*RANGES.values(),CRT)):
            self.instructions+=1
            if CRT[0]<=address<CRT[1]:self.crt+=1
            return
        if address not in BOUNDARIES:raise RuntimeError('미명시코드 %08x'%address)
        name=BOUNDARIES[address];esp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX)
        args=lambda n:[self.dword(esp+4+4*i) for i in range(n)]  # 실제CALL스택의현재인자다.
        if name=='closePopups':self.event(name);cleanup=0;result=0
        elif name in ('baseAttach','attach'):
            a=args(4);self.dword(this+4,self.dword(this+4)&~8);self.dword(this+0x4c,a[0]);self.dword(this+0x50,a[1])
            if a[2]:self.dword(this+0x14,a[2])
            self.event(name,[self.token(this),a[0],a[1],self.token(a[2]),a[3]]);cleanup=16;result=0
        elif name=='push':self.dword(this+4,self.dword(this+4)|8);self.event(name,[self.token(this)]);cleanup=0;result=0
        elif name=='resolve':
            serial=args(1)[0];result=next((node(i) for i in range(8) if self.dword(GLOBALS['presentMask'])&(1<<i) and self.dword(node(i)+0x2c)==serial),0)
            self.event(name,[serial,self.token(result)]);cleanup=0
        elif name=='bubble':
            a=args(3);self.event(name,[self.token(this),*a])
            if not self.stopped and self.kind=='hud':mu.reg_write(UC_X86_REG_ECX,node(0));mu.reg_write(UC_X86_REG_EIP,RANGES['hud'][0]);return
            cleanup=12;result=0
        elif name=='background':
            a=args(6);self.dword(this+4,a[3]|8);self.dword(this+0x38,bits(1));self.dword(this+0x3c,a[0]);self.dword(this+0x40,a[1]);self.dword(this+0x44,a[0]);self.dword(this+0x48,a[1]);self.dword(this+0xac,a[4]);self.dword(this+0xb0,a[5]);self.dword(this+0xb4,a[2]);self.event(name,[self.token(this),*a]);cleanup=24;result=this
        elif name=='activate':self.event(name,[self.token(this),*args(1)]);cleanup=4;result=0
        elif name=='invalidate':self.event(name,args(5));cleanup=0;result=0
        elif name=='finish':self.event(name);cleanup=0;result=0
        elif name=='baseHandle':self.event(name,args(3));cleanup=12;result=73
        elif name in ('chatVisible','chatInput'):self.event(name,args(1));cleanup=0;result=0
        else:self.event(name,[args(3)[2]]);cleanup=0;result=0
        if not self.stopped:self.returned(result,cleanup)

    def run(self,case,control):
        """독립입력에서실제RET나명시중단까지실행하고모든노드/전역을관측한다."""
        self.kind=case['operation'];self.trace=[];self.calls=Counter();self.completed=False;self.stopped=False;self.instructions=0;self.crt=0
        self.effects=[] if case['mutation']=='-' else [x.split('/') for x in case['mutation'].split(';')]
        self.mu.mem_write(STACK-0x4000,bytes([0xcc])*0x4040);self.dword(0,0)
        raw=bytes.fromhex(case['nodes'])
        for i in range(8):  # 실제raw를쓰고가상표주소만명시경계표로치환한다.
            self.mu.mem_write(node(i),raw[i*0x11c:(i+1)*0x11c]);self.dword(node(i),VT+i*0x100)
            self.dword(VT+i*0x100+0x18,DETACH);self.dword(VT+i*0x100+0x10,RANGES['move'][0]);self.dword(VT+i*0x100+0x20,RANGES['reflow'][0] if self.kind=='hud' and i in (1,2,3) else ATTACH)
        for key,value in zip(GLOBALS,map(int,case['globals'].split(':'))):self.dword(GLOBALS[key],value)  # 현재전역을입력한다.
        arguments=list(map(int,case['arguments'].split(':'))) if case['arguments']!='-' else []
        if self.kind=='register':arguments[0]=node(arguments[0]-1) if arguments[0] else 0
        if self.kind=='reflow':arguments[2]=node(arguments[2]-1) if arguments[2] else 0
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(arguments)+1),RETURN,*map(number,arguments)))
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,node(0));self.mu.reg_write(UC_X86_REG_EAX,0x12345678);self.mu.reg_write(UC_X86_REG_EDX,0x23456789)
        for register,value in ((UC_X86_REG_EBX,0x33557799),(UC_X86_REG_ESI,0x446688aa),(UC_X86_REG_EDI,0x557799bb),(UC_X86_REG_EBP,0x6688aacc)):self.mu.reg_write(register,value)  # callee 보존 레지스터 검사의 번이다.
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.emu_start(RANGES[self.kind][0],RETURN+1,count=500000)
        if not self.completed and not self.stopped:raise RuntimeError('실행한도')
        delta=self.mu.reg_read(UC_X86_REG_ESP)-STACK
        if self.completed:
            if delta!=4+len(arguments)*4:raise RuntimeError('stack delta')
            for register,value in ((UC_X86_REG_EBX,0x33557799),(UC_X86_REG_ESI,0x446688aa),(UC_X86_REG_EDI,0x557799bb),(UC_X86_REG_EBP,0x6688aacc)):  # 정상RET의보존레지스터다.
                if self.mu.reg_read(register)!=value:raise RuntimeError('callee보존')
            if self.dword(0):raise RuntimeError('SEH불일치')
        output=[]
        for i in range(8):  #가상표는명시장치이므로앞4BYTE만0정규화한다.
            current=bytearray(self.mu.mem_read(node(i),0x11c));current[:4]=bytes(4);output.append(current.hex())
        result=self.mu.reg_read(UC_X86_REG_EAX)
        if self.kind=='construct' and self.completed:result=self.token(result)
        for a,original in self.original.items():  # 실행뒤원문명령이그대로인지검사한다.
            if bytes(self.mu.mem_read(a,len(original)))!=original:raise RuntimeError('원문변경')
        self.totals[str(control)]+=self.instructions
        return [int(self.completed),''.join(output),':'.join(str(self.dword(a)) for a in GLOBALS.values()),';'.join(self.trace) or '-',result,delta,self.instructions,self.crt]


def defaults():
    """기본8노드와HUD의현재전역을독립장치입력으로만든다."""
    raw=bytearray(8*0x11c)
    def put(i,offset,value):
        """입력raw의한DWORD를기록한다."""
        struct.pack_into('<I',raw,i*0x11c+offset,number(value))
    for i in range(8):  # 모든기본노드를살아있는1배율자료로만든다.
        for off,v in ((4,1),(0x2c,100+i),(0x38,bits(1)),(0x3c,bits(80)),(0x40,bits(75)),(0x44,bits(80)),(0x48,bits(75)),(0x4c,bits(10)),(0x50,bits(20))):put(i,off,v)
    g=[1024,768,1,85,0,939,672,*[bits(v) for v in (85,0,1024,672)],85,0,1024,672,0,688,80,768,7,bits(400),255]
    return raw,g,put


def case(name,kind,arguments,edits=(),mutation='-',hud=False,globals_edit=()):
    """후보기하와콜백효과만작성하고원문출력은추정하지않는다."""
    raw,g,put=defaults()
    if hud:
        for off,i in ((0xa0,1),(0xa4,2),(0xa8,3),(0xac,4),(0xb0,5),(0xb8,7),(0xc0,6)):put(0,off,node(i))  # 원본HUD직계포인터다.
        for i,x,y,w,h,axis in ((1,80,0,5,768,4),(2,0,688,80,5,8),(3,85,672,939,5,0x1a),(4,0,693,80,75,0),(5,0,0,80,688,0),(6,0,0,80,688,0),(7,85,677,939,91,0)):
            for off,v in ((0x4c,x),(0x50,y),(0x3c,w),(0x40,h),(0x44,w),(0x48,h)):put(i,off,bits(v))  # 정상실제배치직전입력이다.
            put(i,0xb8,axis)
        for master,links in ((1,[(2,1),(3,4),(4,1),(5,1),(6,1),(7,4)]),(2,[(4,8),(5,2),(6,2)]),(3,[(7,8)])):
            for k,(slave,flags) in enumerate(links):put(master,0xcc+k*8,100+slave);put(master,0xd0+k*8,flags)  # 원문등록순서다.
    for i,off,value in edits:put(i,off,value)  #경계별현재raw변경입력이다.
    for key,value in globals_edit:g[list(GLOBALS).index(key)]=number(value)  #화면/장치입력을바꾼다.
    return dict(zip(INPUTS,(name,kind,raw.hex(),':'.join(map(str,g)),':'.join(str(number(v)) for v in arguments) or '-',mutation)))


def cases():
    """분기폭/float저장/현재조회/진단중단과정상HUD를대조할입력목록이다."""
    output=[]
    for axis,w,h in itertools.product((0,4,8,0x12,0x12345678),(-1,0,5,16777217),(5,768)):output.append(case('ctor%03d'%len(output),'construct',(axis,w,h,0x3400000,0,7),edits=((0,0xbc,991),(0,0xc0,992),(0,0xc4,993),(0,0xc8,994),(0,0xcc,900))))  # signed폭과미초기화제한보존이다.
    for count,child,serial in itertools.product((0,1,9,10),(0,2),(0,101)):
        edits=[(0,0xcc+8*i,200+i) for i in range(count)]+[(1,0x2c,serial)]  #현재빈슬롯과serial0이다.
        output.append(case('register%03d'%len(output),'register',(child,0xfedcba98),edits=edits))
    output.append(case('registerStop','register',(2,1),edits=[(0,0xcc+8*i,200+i) for i in range(10)],mutation='assert1/control/stop/1'))
    for flags,scale in itertools.product(range(256),(0.5,1.0,1.25)):
        edits=((0,0xcc,101),(0,0xd0,flags),(0,0x4c,bits(-2.25)),(0,0x50,bits(7.75)),(0,0x3c,bits(5.125)),(0,0x40,bits(3.75)),(1,0x38,bits(scale)),(1,0x4c,bits(-9.875)),(1,0x50,bits(13.125)))
        output.append(case('reflow%04d'%len(output),'reflow',(bits(80.25),bits(688.125),0,0),edits=edits))  # relation 하위BYTE256개와 재부착 모든 분기다.
    for axis,visible in itertools.product((0,0x10,0xffffffff),(0,1)):output.append(case('body%04d'%len(output),'reflow',(bits(80.125),bits(-1.25),0,0),edits=((0,0xb8,axis),(0,4,visible),(0,0x38,bits(1.25)),(0,0xcc,101),(0,0xd0,15))))  # 현재 폭과 표시 관문이다.
    for mutation in ('closePopups1/node0/y/'+str(bits(12.5)),'baseAttach1/node0/flags/0','baseAttach1/node0/x/'+str(bits(99.25)),'bubble1/node0/link0flags/8','resolve1/node0/link0flags/10','push1/node0/x/'+str(bits(91.75)),'push1/node1/y/'+str(bits(33.25)),'push1/node0/link0flags/4','attach1/node0/link0flags/8','attach1/node0/link1serial/102','attach1/global/screenHeight/801','push1/control/stop/1'):
        output.append(case('mutate%04d'%len(output),'reflow',(bits(80.25),bits(688.125),0,0),edits=((0,0xcc,101),(0,0xd0,15),(0,0xd8,8)),mutation=mutation))  # 각callback 뒤 현재값 재조회다.
    for width,height,show,mode,hidden in itertools.product((640,1024,1280),(480,768),(0,1),(0,1,3,4),(0,1)):
        output.append(case('mode%04d'%len(output),'hud',(0x0200003d,mode,0),hud=True,edits=((3,0x50,bits(height if hidden else height-96)),),globals_edit=(('screenWidth',width),('screenHeight',height),('showHud',show))))  # 정상HUD와 숨긴chat의 실제배치다.
    for source,x,y,w in itertools.product((100,101,102,103,999),(-5.25,80.75),(672.875,768),(5.125,16777216)):
        output.append(case('view%04d'%len(output),'hud',(0x0200003a,source,0),hud=True,edits=((1,0x4c,bits(x)),(1,0x3c,bits(w)),(3,0x50,bits(y)))))  # 현재source와ftol및 floatLTRB다.
    for event,first,second,y in itertools.product((0x02000088,0x02000005,0x8200003a,0x02000046,0x123),(0,101,103),(0,1,2),(672,768)):
        output.append(case('event%04d'%len(output),'hud',(event,first,second),hud=True,edits=((3,0x50,bits(y)),)))  # chat표시/크기와release마스크/기본전달이다.
    for mutation in ('invalidate1/global/mapHeight/555','invalidate1/global/screenHeight/900','bubble1/global/showHud/0','push1/node0/vertical/'+str(node(2)),'baseAttach1/node1/baseWidth/'+str(bits(7.25)),'invalidate1/control/stop/1'):
        output.append(case('hudMutation%04d'%len(output),'hud',(0x0200003d,3,0),hud=True,mutation=mutation))  # 중첩HUD알림 뒤 현재필드변경이다.
    return output


def main():
    """원문/호스트를고정하고두정밀도관측과해시를새파일로쓴다."""
    if socket.gethostname().lower()!='vm-w11-codex':raise RuntimeError('현재/최종디컴파일호스트불일치')
    if hashlib.sha256(PE_PATH.read_bytes()).hexdigest()!=PE_SHA:raise RuntimeError('원문SHA불일치')
    oracle=Oracle();inputs=cases();rows=[];differences=0
    for entry in inputs:  #두실제x87정밀도를독립진입한다.
        observed=[]
        for control in CONTROLS:
            out=oracle.run(entry,control);rows.append([entry[k] for k in INPUTS]+out);observed.append(out)
        if observed[0][:-2]!=observed[1][:-2]:differences+=1
    FIXTURE.write_text('\t'.join(INPUTS+OUTPUTS)+'\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    report={'host':socket.gethostname(),'decompileHost':'VM-W11-Codex','decompileDate':'2026-10-23','peSha256':PE_SHA,'schema':list(INPUTS+OUTPUTS),'globals':GLOBALS,'nodeFields':FIELDS,'nodeAddress':NODES,'nodeStride':STRIDE,'nodeBytes':0x11c,'controls':list(CONTROLS),'cases':len(inputs),'rows':len(rows),'precisionDifferences':differences,'completedRows':sum(r[6] for r in rows),'nativeInstructions':sum(r[-2] for r in rows),'crtInstructions':sum(r[-1] for r in rows),'calls':dict(oracle.calls_total),'writes':dict(oracle.writes),'fixtureSha256':hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),'toolSha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'boundaries':{hex(k):v for k,v in BOUNDARIES.items()},'scope':['004c6c50/004c4790/004c47f0/004c6530및중첩00464d10실제기계어','실제CRT004e49c0','가상표첫DWORD만0정규화;다른raw와전역전체비교','background/baseAttach/attach/push기본효과뒤mutation;resolve반환은effect전현재serial로고정','hud bubble은부모route경계만명시하고같은스택으로실제004c6530재진입','construct완료result는1기반this토큰;register/reflow와미완료EAX는의미반환으로비교하지않음','창유형/shape/트리링크·dirty그림자체는명시외부경계;GUI/게임실행없음']}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('cases','rows','completedRows','precisionDifferences','nativeInstructions','crtInstructions','fixtureSha256')},ensure_ascii=False))


if __name__=='__main__':main()
