#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 10.78 TextEntry/Console 본체와 같은 0x28 버퍼를 격리 CPU에서 관측한다."""
import hashlib
import itertools
import json
import socket
import struct
from collections import Counter
from pathlib import Path
import decomp_text_entry_buffer as raw
from unicorn.x86_const import *

ROOT=raw.ROOT  # 동일 보호 PE와 새 관측자료의 루트다.
FIXTURE=ROOT/'cpppj/tests/fixtures/text-entry-gump-x86.tsv'  # 입력과 실제 본체 관측을 분리한 자료다.
REPORT=ROOT/'extracted/cpp-text-entry-gump-20261023-native.json'  # 명령/주소/장치 경계와 SHA를 기록한다.
NODE=raw.HEAP+0x9000  # 실제0x128 Console/TextEntry 노드다.
VERTICAL=raw.HEAP+0xa000  # 같은 세로 Slider 장치다.
HORIZONTAL=raw.HEAP+0xb000  # 같은 가로 Slider 장치다.
VTABLE=raw.HEAP+0xc000  # 기존 노드의 실제 가상 CALL 목적지만 장치에 연결한다.
DEVICE=raw.HEAP+0xd000  # 공통 Push/Redraw/Attach/자동완성/제출 장치다.
COMPLETION=raw.HEAP+0xe000  # 자동완성 반환 문자열의 실제 바이트다.
STRINGS=raw.HEAP+0x300000  # 서로 다른 String이 빌린 포인터를 덮지 않는 전용 장치 저장소다.
STRING_STRIDE=0x1000  # 각 객체의 NUL 포함 내용에 부여하는 고정 물리 간격이다.
STRING_CAPACITY=0x400000  # 원문 목록512개와 임시 문자열을 함께 수용하는 명시 한도다.
FIELDS=(0,4,0x20,0x2c,0x38,0x3c,0x40,0x44,0x48,0x4c,0x50,*range(0x80,0xa0,4),0xb4,*range(0xb8,0x128,4))  # 제품과 직접 비교하는 원문 필드다.
CHILD_FIELDS=(0,4,0x2c,0x38,0x3c,0x40,0x44,0x48,0x4c,0x50,0xa8)  # 자식의 실제 공유 필드다.
RANGES=(*raw.RANGES,(0x451b60,0x451df2),(0x451e00,0x452430),(0x452430,0x452723),(0x452730,0x4527aa),(0x4527b0,0x4528d3),(0x4528e0,0x452dc4),(0x43f820,0x43f887),(0x43f8a0,0x43fa23),(0x46ee50,0x46ee53),(0x4e49c0,0x4e4a36))  # allocator 외 본체·중첩 버퍼·실제 String getter·_ftol이다.
ENTRIES={'construct':0x452430,'consoleConstruct':0x43f820,'attach':0x4527b0,'scroll':0x451c90,'relative':0x4528b0,'handle':0x4528e0,'console':0x43f8a0,'draw':0x451f00,'idle':0x451e90,'setText':0x451e00,'append':0x451e20,'excluded':0x451b80,'horizontal':0x451c60,'destroy':0x452730}  # 정상 진입~RET를 관측한다.
INPUTS=('name','operation','node','buffer','blocks','arguments','text','excluded','clipboard','clock','completion','recipient','fill')  # 기대값을 계산하지 않는 raw 입력이다.
OUTPUTS=('nodeout','verticalout','horizontalout','bufferout','blocksout','clipboardout','trace','result','stackdelta','nativeinstructions')  # 뒤쪽은 실제 실행 출력이다.


def bits(value):
    """float 저장 비트를 원문 DWORD로 직렬화한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


class Oracle(raw.Oracle):
    """버퍼 실제 본체를 재사용하고 Gump 외부 장치 경계만 추가한다."""
    def __init__(self):
        """보호 PE를 그대로 로드하고 SEH의 FS:0 한 DWORD만 추가한다."""
        super().__init__();self.mu.mem_map(0,4096);self.mu.mem_map(STRINGS,STRING_CAPACITY)
        self.original={a:bytes(self.mu.mem_read(a,b-a)) for a,b in RANGES}

    def window(self,address,count):
        """원문 노드/장치 문자열과 기존 버퍼의 물리 메모리만 허용한다."""
        if NODE<=address and address+count<=raw.HEAP+0x10000:return
        if STRINGS<=address and address+count<=STRINGS+STRING_CAPACITY:return
        if 0x400000<=address and address+count<=0x600000:return
        return super().window(address,count)

    def memory_write(self,mu,access,address,size,value,data):
        """실제 본체의 현재 노드·SEH·버퍼 쓰기를 각각 센다."""
        if address==0 and size==4:key='seh'
        elif NODE<=address and address+size<=NODE+0x128:key='node'
        elif VERTICAL<=address and address+size<=HORIZONTAL+0xd8:key='children'
        else:return super().memory_write(mu,access,address,size,value,data)
        self.write_total[key]+=1

    def values(self,address,fields):
        """같은 현재 노드의 raw 필드만 순서대로 관측한다."""
        return ':'.join(str(self.dword(address+offset)) for offset in fields)

    def string_value(self,address,value):
        """각 String의 실제 첫 DWORD에 독립 내용 포인터를 설치하고 기존 빌림을 보존한다."""
        if address not in self.string_addresses:
            slot=len(self.string_addresses)*STRING_STRIDE  # 같은 객체의 후속 getter는 같은 주소를 읽는다.
            if slot+STRING_STRIDE>STRING_CAPACITY:raise RuntimeError('명시 String 객체 수 한도')
            self.string_addresses[address]=STRINGS+slot
        if len(value)>=STRING_STRIDE:raise RuntimeError('명시 String 내용 한도')
        pointer=self.string_addresses[address]  # 다른 객체가 조회되어도 현재 내용은 덮이지 않는다.
        self.strings[address]=value;self.dword(address,pointer);self.mu.mem_write(pointer,value+b'\0')

    def instruction(self,mu,address,size,data):
        """실제 분기/선택/버퍼 호출은 실행하며 글꼴·트리·Slider 장치만 바꾼다."""
        if address==raw.RETURN:self.completed=True;mu.emu_stop();return
        if any(a<=address<b for a,b in RANGES):self.instructions+=1;return
        esp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX);arg=lambda index:self.dword(esp+4+index*4)  # 실제 call 스택과 this다.
        if address in raw.BOUNDARIES:return super().instruction(mu,address,size,data)
        if address in (DEVICE,0x464a50):self.event('redraw',[this,arg(0)]);self.returned(0,4);return
        if address in (DEVICE+0x10,0x464560):self.event('push',[this]);self.returned();return
        if address in (DEVICE+0x20,0x466750):
            values=[this,*[arg(i) for i in range(4)]];name='baseAttach' if address==0x466750 else 'attach';self.dword(this+0x4c,arg(0));self.dword(this+0x50,arg(1));self.event(name,values);self.returned(0,16);return
        if address==0x464e90:self.event('bubble',[this,arg(0),arg(1),arg(2)]);self.returned(0,12);return
        if address in (0x4a6350,0x4a6320,0x4a6050):
            count=2 if address==0x4a6350 else 1;name={0x4a6350:'range',0x4a6320:'page',0x4a6050:'position'}[address]
            self.event(name,[this,*[arg(i) for i in range(count)]])
            if name=='position':self.dword(this+0xa8,arg(0))
            self.returned(0,count*4);return
        if address==0x465020:
            pointer,length=arg(0),raw.signed(arg(4));payload=self.cstring(pointer) if length<0 else bytes(mu.mem_read(pointer,length));width=sum(4+(value%5) for value in payload);height=14
            self.dword(arg(1),width);self.dword(arg(2),height);self.event('measure',[payload.hex() or '-',raw.uint(length),width,height]);self.returned(0,20);return
        if address==0x465050:
            context=arg(3);color=self.dword(context+8) if context else self.dword(this+0x88);self.event('text',[self.cstring(arg(0)).hex() or '-',arg(1),arg(2),int(bool(context)),color]);self.returned(0,16);return
        if address==0x464740:self.dword(arg(0),17);self.dword(arg(0)+4,23);self.event('screen',[this]);self.returned(arg(0),4);return
        if address==0x4a2060:self.event('fill',[arg(i) for i in range(5)]);self.returned();return
        if address==0x465c60:self.event('background',[this]);self.returned();return
        if address==0x42dc70:self.event('recipient',[self.cstring(arg(0)).hex() or '-']);self.returned(int(self.case['recipient']));return
        if address==0x460d70:
            times=self.case['clock'].split(':');value=float(times[min(self.calls['now'],len(times)-1)]);mu.mem_write(arg(0),struct.pack('<d',value));self.event('now',[struct.unpack('<Q',struct.pack('<d',value))[0]]);self.returned(arg(0));return
        if address==0x465330:self.event('idle',[this,arg(0)]);self.returned(0,4);return
        if address==0x4655d0:
            self.dword(this+4,1);self.dword(this+0x38,bits(1));self.dword(this+0x3c,0);self.dword(this+0x40,0);self.dword(this+0xb4,arg(2));self.event('constructBackground',[this,arg(2)]);self.returned(this,24);return
        if address==0x4e4391:
            amount=arg(0)
            if self.case['operation'] in ('construct','consoleConstruct') and amount in (40,216):
                token=raw.STATE if amount==40 else (VERTICAL if not self.slider_allocated else HORIZONTAL);self.slider_allocated+=amount==216
            else:token=self.allocated(amount)
            self.event('new',[amount,token]);self.returned(token);return
        if address==0x4a64b0:
            self.dword(this,VTABLE);self.dword(this+4,1);self.dword(this+0x38,bits(1));self.dword(this+0x3c,bits(raw.signed(arg(0))));self.dword(this+0x40,bits(raw.signed(arg(1))));self.dword(this+0x44,bits(raw.signed(arg(0))));self.dword(this+0x48,bits(raw.signed(arg(1))));self.dword(this+0xa8,0);self.event('constructSlider',[this,*[arg(i) for i in range(4)]]);self.returned(this,16);return
        if address==0x4e4354:
            token=arg(0)
            if token in self.blocks:self.blocks[token]['live']=0
            self.event('delete',[token]);self.returned();return
        if address==0x464420:self.event('destructBase',[this]);self.returned();return
        if address==0x4e0620:self.event('assert',[arg(2)]);self.returned();return
        if address==0x4e701a:
            value=arg(0)&255;self.returned(int(value==32 or 9<=value<=13));return
        if address==DEVICE+0x30:
            source=arg(0);self.event('complete',[self.cstring(source).hex() if source else '~',arg(1),arg(2)]);self.returned(COMPLETION if self.case['completion']!='~' else 0);return
        if address==DEVICE+0x40:self.event('submit',[self.cstring(arg(0)).hex() or '-']);self.returned();return
        if address==0x4e3090:self.string_value(this,b'');self.returned(this);return
        if address==0x4e3160:self.string_value(this,self.cstring(arg(0)));self.returned(this,4);return
        if address==0x4e32b0:
            value=self.strings[this];begin,count=arg(0),arg(1)
            if begin+count<=len(value):value=value[:begin]+value[begin+count:]
            self.string_value(this,value);self.returned(this,8);return
        if address==0x4e31a0:self.strings.pop(this,None);self.returned();return
        raise RuntimeError('미명시 TextEntry 코드 %08x'%address)

    def run(self,case,control):
        """실제 진입부터 정상 RET까지 실행하고 노드·전체 버퍼·장치 호출을 저장한다."""
        self.case=case;self.calls=Counter();self.trace=[];self.blocks={};self.completed=False;self.stopped=False;self.instructions=0;self.fill=int(case['fill']);self.clipboard=b'';self.failures=set();self.effects=[];self.strings={};self.string_addresses={};self.slider_allocated=0
        self.mu.mem_write(0,raw.pack([0]));self.mu.mem_write(raw.STACK-0x1000,bytes([0xcc])*0x1040);self.mu.mem_write(NODE,bytes([0xa5])*0x128)
        for offset,value in zip(FIELDS,map(int,case['node'].split(':'))):self.dword(NODE+offset,value)  # 독립 raw 노드 입력만 설치한다.
        for child,serial,width,height in ((VERTICAL,31,10,100),(HORIZONTAL,32,100,10)):  # 실제 장치 노드의 같은 기본 저장소다.
            self.mu.mem_write(child,bytes([0])*0xd8)
            for offset,value in ((0,VTABLE),(4,1),(0x2c,serial),(0x38,bits(1)),(0x3c,bits(width)),(0x40,bits(height)),(0x44,bits(width)),(0x48,bits(height)),(0xa8,3)):self.dword(child+offset,value)
        for slot,address in ((8,DEVICE),(0x18,DEVICE+0x10),(0x20,DEVICE+0x20)):self.dword(VTABLE+slot,address)  # 객체 본체가 실제로 간접 CALL한다.
        self.mu.mem_write(raw.STATE,raw.pack(list(map(int,case['buffer'].split(':')))));self.mu.mem_write(raw.SOURCE,bytes.fromhex(case['text'])+b'\0');self.mu.mem_write(raw.EXCLUDED,bytes.fromhex(case['excluded'])+b'\0');self.mu.mem_write(COMPLETION,(bytes.fromhex(case['completion']) if case['completion']!='~' else b'')+b'\0')
        if case['blocks']!='-':
            for item in case['blocks'].split(';'):  # 원시 입력의 실제 기존 두 할당을 설치한다.
                base,nominal,live,payload=item.split(':');base=int(base);payload=bytes.fromhex(payload);self.blocks[base]={'nominal':int(nominal),'live':int(live),'bytes':payload};self.mu.mem_write(base,payload)
        self.next_block=len(self.blocks);arguments=[] if case['arguments']=='-' else list(map(int,case['arguments'].split(':')))
        self.mu.mem_write(raw.STACK,raw.pack([raw.RETURN,*arguments]));self.mu.reg_write(UC_X86_REG_ESP,raw.STACK);self.mu.reg_write(UC_X86_REG_ECX,NODE);self.mu.reg_write(UC_X86_REG_EAX,0x12345678)
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        for address,value in ((0x59afd8,5),(0x5acd24,9),(0x5a4058,0),(0x5427d8,0x2000038),(0x5429f0,0x20000bf),(0x5427d0,0x2000036),(0x5427b0,0x200002e)):self.dword(address,value)  # 원본 현재 전역 입력이다.
        self.mu.mem_write(0x54042a,b'\x0f');self.mu.mem_write(0x540425,b'\x0a');self.mu.emu_start(ENTRIES[case['operation']],raw.RETURN+1,count=1000000)
        if not self.completed:raise RuntimeError('원문 정상 RET 없음')
        delta=self.mu.reg_read(UC_X86_REG_ESP)-raw.STACK
        if delta!=4+len(arguments)*4:raise RuntimeError('스택 불일치 %s/%d'%(case['name'],delta))
        for address,payload in self.original.items():  # 매 관측 후 보호 본체의 불변을 확인한다.
            if bytes(self.mu.mem_read(address,len(payload)))!=payload:raise RuntimeError('원문 코드 변경')
        blocks=[]
        for base,block in sorted(self.blocks.items()):blocks.append(':'.join((str(base),str(block['nominal']),str(block['live']),bytes(self.mu.mem_read(base,len(block['bytes']))).hex())))  # 가드/반납 블록도 포함한다.
        result=self.mu.reg_read(UC_X86_REG_EAX) if case['operation'] in ('handle','console') else 0
        return [self.values(NODE,FIELDS),self.values(VERTICAL,CHILD_FIELDS),self.values(HORIZONTAL,CHILD_FIELDS),self.state(),';'.join(blocks) or '-',self.clipboard.hex() or '-',';'.join(self.trace) or '-',result,delta,self.instructions]


def make_case(operation,text=b'ab\ncd',selection=(1,1),arguments=(),changes=None,excluded=b'',completion=b'world',recipient=0,initial=True):
    """입력 메모리/장치만 만들며 선택/키/Draw의 기대값은 계산하지 않는다."""
    base=raw.make_case('','construct' if operation in ('construct','consoleConstruct') else 'insert',text=text,selection=selection,capacity=max(16,len(text)+1));fields={offset:0xa5a5a5a5 for offset in FIELDS}
    fields.update({0:VTABLE,4:1,0x20:15,0x2c:17,0x38:bits(1),0x3c:bits(80),0x40:bits(28),0x44:bits(80),0x48:bits(28),0x4c:bits(3),0x50:bits(4),0xb4:0x4000067,0xb8:VERTICAL,0xbc:HORIZONTAL,0xc0:1,0xc8:0,0xcc:0x3fe00000,0xd0:1,0xd4:0,0xd8:1,0xdc:1,0xe0:0,0xe4:0,0xe8:0,0xec:0,0xf0:1,0xf4:1,0xf8:1,0xfc:0,0x100:0,0x104:raw.EXCLUDED if excluded else 0,0x108:raw.STATE,0x110:DEVICE+0x30,0x114:DEVICE+0x40,0x118:1,0x11c:0,0x120:2})
    fields.update(dict(zip(range(0x80,0xa0,4),(0,5,31,0,0,0,1,1))));fields.update(changes or {})
    if operation in ('construct','consoleConstruct'):arguments=(80,28,raw.SOURCE if initial else 0,5,0x4000067,31)
    elif operation in ('setText','excluded'):arguments=(raw.SOURCE if initial else 0,)
    elif operation=='append':arguments=(raw.SOURCE if initial else 0,*arguments)
    return dict(zip(INPUTS,('',operation,':'.join(str(raw.uint(fields[offset])) for offset in FIELDS),base['state'],base['blocks'],':'.join(str(raw.uint(value)) for value in arguments) or '-',text.hex(),excluded.hex(),b'A\tB\nC'.hex(),'0.5:0.75','~' if completion is None else completion.hex(),recipient,165)))


def cases():
    """주요 문자·수정키·읽기전용·줄/선택/크기·콜백 관문을 교차한다."""
    rows=[]
    for operation in ('construct','consoleConstruct'):
        for text,initial in itertools.product((b'',b'hi',b'a\nb'),(False,True)):rows.append(make_case(operation,text=text,initial=initial))  # null/빈/다중줄 생성이다.
    keys=(0,8,9,13,27,31,32,65,127,128,255,256,0x10000,0x20000,0x100000,0x230000,0x240000,0x250000,0x260000,0x270000,0x280000,0x2d0000,0x2e0000,0x2000036,0x200002e)
    for key,modifier,readonly in itertools.product(keys,(value<<28 for value in range(16)),(0,1)):
        rows.append(make_case('handle',arguments=(key|modifier,31 if key==0x2000036 else 32,3),changes={0xd4:readonly},excluded=b'A\n'))  # 실제 jump표/모든 modifier와 release다.
    for selection,text,maximum in itertools.product(((0,0),(0,3),(3,3)),(b'abc',b'a\nb'),(0,4,5)):rows.append(make_case('handle',text=text,selection=selection,arguments=(65,11,22),changes={0x100:maximum}))  # 선택길이를 빼지 않는 최대길이 관문이다.
    for multiline,notice,forward in itertools.product((0,1),repeat=3):rows.append(make_case('handle',arguments=(13,11,22),changes={0xf4:multiline,0xf8:notice,0xf0:forward}))  # Enter 통지와 LF 전달 및 변경 통지를 따로 교차한다.
    for event,serial,second in itertools.product((0x2000036,0x200002e),(31,32,999),(0,1,2,3,4,5)):rows.append(make_case('handle',arguments=(event,serial,second)))  # 두 Slider/잘못된 serial/네 방향과 무시되는 step이다.
    for line,column,old in itertools.product((-2,0,1,3,99),(-2,0,1,4,99),(0,1)):rows.append(make_case('scroll',text=b'abcd\nz\n',arguments=(line,column),changes={0xe0:old}))  # 쓰기 전 줄 길이를 읽는 원문 clamp다.
    for show,operation in itertools.product((0,1,2),('attach','horizontal')):rows.append(make_case(operation,arguments=(bits(5.25),bits(6.5),99,3) if operation=='attach' else (show,),changes={0xe8:show,0xec:show}))  # 자식 Push/크기/표시와 부모 부착 순서다.
    for operation in ('setText','append','excluded'):
        for initial in (False,True):rows.append(make_case(operation,text=b'X\nY\n',initial=initial,arguments=(1,) if operation=='append' else ()))  # 현재 문자열과 별도 제외문자 LF 제거다.
    for text in (b'',b'\n',b'\n\n',b'A\nB\nC',b'\r\n\t',b'\x80\xff'):rows.append(make_case('excluded',text=text,excluded=b'old'))  # 이전 포인터 반납과 LF 반복삭제의 물리 바이트다.
    for event,text,selection,reset,completion,readonly in itertools.product((9,0x20000009,13,0x100000,65), (b'one:wo',),((6,6),(0,3)),(0,1),(None,b'',b'WORLD'),(0,1)):
        rows.append(make_case('console',text=text,selection=selection,arguments=(event,11,22),changes={0x118:reset,0xd4:readonly},completion=completion))  # Tab callback은 readonly를 별도 검사하지 않는다.
    for text,selection,column,highlight in itertools.product((b'',b'one\ntwo',b'Bob:yes'),((0,0),(0,2),(2,2)),(0,1,20),(0,1)):
        selection=tuple(min(value,len(text)) for value in selection);rows.append(make_case('draw',text=text,selection=selection,changes={0xe4:column,0xfc:highlight},recipient=1))  # LF/선택/caret/수평삭제/수신대상 색이다.
    for height,readonly,phase in itertools.product((0.5,14.0,14.5,28.0),(0,1),(0,1)):rows.append(make_case('draw',changes={0x40:bits(height),0xd4:readonly,0xd0:phase}))  # 정수 나머지와 float높이의 실제 표시줄 수다.
    for flags,readonly,time in itertools.product((1,0x10001),(0,1),(0.25,0.5,0.75)):
        row=make_case('idle',changes={4:flags,0xd4:readonly});row['clock']=str(time)+':1.0';rows.append(row)  # 실제 두 시계 조회·동률·미도달과 dead 관문이다.
    for owns in (0,1):rows.append(make_case('destroy',changes={0xc0:owns}))  # 버퍼 공유와 소유 반납을 구별한다.
    for index,row in enumerate(rows):row['name']='entry%04d'%index  # 입력에 안정된 순번만 붙인다.
    return rows


def main():
    """현재/최종 디컴파일 호스트와 SHA를 고정하여 두 x87 관측을 저장한다."""
    if socket.gethostname().lower()!='vm-w11-codex':raise RuntimeError('호스트 불일치')
    if hashlib.sha256(raw.PE_PATH.read_bytes()).hexdigest()!=raw.PE_SHA:raise RuntimeError('원문 SHA 불일치')
    oracle=Oracle();rows=[];differences=0;inputs=cases()
    for case in inputs:  # 동일 raw 입력을 두 독립 x87 제어어에서 정상 RET까지 실행한다.
        observed=[]
        for control in raw.CONTROLS:
            try:output=oracle.run(case,control)
            except Exception as error:raise RuntimeError('%s/%s: %s'%(case['name'],case['operation'],error)) from error
            observed.append(output);rows.append([case[key] for key in INPUTS]+output)
        if observed[0][:-1]!=observed[1][:-1]:differences+=1
    FIXTURE.write_text('\t'.join(INPUTS+OUTPUTS)+'\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    report={'host':socket.gethostname(),'decompileHost':'VM-W11-Codex','decompileDate':'2026-10-23','peSha256':raw.PE_SHA,'schema':list(INPUTS+OUTPUTS),'fields':FIELDS,'childFields':CHILD_FIELDS,'entryPoints':ENTRIES,'ranges':RANGES,'cases':len(inputs),'rows':len(rows),'precisionDifferences':differences,'nativeInstructions':sum(row[-1] for row in rows),'calls':dict(oracle.calls_total),'writes':dict(oracle.write_total),'fixtureSha256':hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),'toolSha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'bufferToolSha256':hashlib.sha256(Path(raw.__file__).read_bytes()).hexdigest(),'stringGetter':{'address':'0046ee50','bytes':'8b01c3','actualBody':True,'objectDataPointer':True,'storageStride':STRING_STRIDE,'storageCapacity':STRING_CAPACITY},'scope':['정상 진입~RET의 TextEntry/Console 본체·중첩 실제 버퍼·_ftol 실행','글꼴 측정/출력·팔레트/recipient·공통 트리/가상·Slider·시계·메모리/clipboard 장치만 명시 경계','Slider 경계는 raw position을 그대로 저장하며 자체 clamp/하위 자식은 별도 기존 검사 범위','글꼴 장치는 원시 BYTE당 4+BYTE%5폭/14높이이며 실제 자산 픽셀 검증 아님','Draw의 임시 String/ASCII isspace는 명시 순수 계약이며 실제 분기/복사/선택 본체는 원문','선택 임시버퍼128BYTE를 넘는 원본 stack overflow와 잘못된 줄/메모리 입력은 미검증','void EAX는 제품 contract가 아니므로 result0으로 정규화','게임/GUI 실행·원본 변경·새 디컴파일 없음']}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:report[key] for key in ('cases','rows','precisionDifferences','nativeInstructions','fixtureSha256')},ensure_ascii=False))


if __name__=='__main__':main()
