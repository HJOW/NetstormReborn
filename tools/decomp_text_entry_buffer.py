#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78의0x28 편집 버퍼 본체를 실제CPU로 실행해 독립자료를 만든다."""
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

ROOT=Path(__file__).resolve().parents[1]  # 보호입력과 새자료의루트다.
PE_PATH=ROOT/'originals/Netstorm.exe'  # 읽기만하는실제10.78이다.
PE_SHA='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 고정원문식별자다.
FIXTURE=ROOT/'cpppj/tests/fixtures/text-entry-buffer-x86.tsv'  # 독립실제기계어관측이다.
REPORT=ROOT/'extracted/cpp-text-entry-buffer-20261023-native.json'  # 호스트/실제명령/경계/해시다.
BASE=0x400000  # 실제PE의로드기준이다.
HEAP=0x1000000  # 격리원문주소공간이다.
STATE=HEAP+0x1000  # 실제40BYTE현재버퍼다.
SOURCE=HEAP+0x2000  # 고정NUL삽입문자열이다.
EXCLUDED=HEAP+0x6000  # 고정clipboard제외문자열이다.
OUTPUT=HEAP+0x7000  # 출력문자열2048BYTE와선택쌍이다.
PAIR=HEAP+0x7800  # 줄선택출력의별도8BYTE다.
RETURN=HEAP+0x8000  # 정상RET관측종료점이다.
BLOCKS=HEAP+0x10000  # alloc장치가반환하는고정주소범위다.
STRIDE=0x10000  # 할당블록사이의가상주소간격이다.
STACK=0x2100000  # 완전한진입스택이다.
CONTROLS=(0x27f,0x37f)  # 정수본체도두x87제어어에서독립확인한다.
RANGES=((0x451550,0x451640),(0x451680,0x451b60))  # allocator가상wrapper를제외한실제부모본문이다.
ENTRIES={'construct':0x451a80,'destroy':0x451680,'select':0x4516c0,'insert':0x451710,'reset':0x451930,'lines':0x451550,'lineCount':0x451950,'lineLength':0x451960,'copyLine':0x451990,'copyAll':0x4518f0,'copy':0x4518c0,'cut':0x451b10,'paste':0x451800}  # 실제개별진입주소다.
FIELDS={'vtable':0,'offsets':4,'offsetCapacity':8,'lineCount':12,'dirty':16,'selectionBegin':20,'selectionEnd':24,'length':28,'capacity':32,'data':36}  # 원문10DWORD다.
BOUNDARIES={0x43fa80:'allocate',0x451640:'allocate',0x43fa90:'release',0x451650:'release',0x451660:'resize',0x4e63e0:'strchr',0x4e5b40:'memmove',0x4dd810:'clipboardRead',0x4dd870:'clipboardWrite'}  # allocator/표준C메모리/clipboard만명시한다.
INPUTS=('name','operation','state','blocks','arguments','text','excluded','clipboard','failures','mutation','fill')  # 앞11열만원문실행입력이다.
OUTPUTS=('completed','stateout','blocksout','output','selectionout','clipboardout','trace','result','stackdelta','nativeinstructions','crtinstructions')  # 뒤11열은실제관측이다.


def uint(value):
    """DWORD폭으로입력을정규화한다."""
    return value&0xffffffff


def signed(value):
    """원시DWORD를signed32로읽는다."""
    return struct.unpack('<i',struct.pack('<I',uint(value)))[0]


def pack(values):
    """원시DWORD목록을정확한little-endian으로만든다."""
    return struct.pack('<'+'I'*len(values),*map(uint,values))


class Oracle:
    """버퍼기계어와표준장치계약의실제호출순서를관측한다."""
    def __init__(self):
        """보호PE/스택/고정할당공간을준비한다."""
        pe=pefile.PE(str(PE_PATH));self.mu=Uc(UC_ARCH_X86,UC_MODE_32)
        self.mu.mem_map(BASE,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);self.mu.mem_write(BASE,pe.get_memory_mapped_image())
        self.mu.mem_map(HEAP,0x200000);self.mu.mem_map(STACK-0x10000,0x20000)
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.memory_write)
        self.original={a:bytes(self.mu.mem_read(a,b-a)) for a,b in RANGES};self.calls_total=Counter();self.write_total=Counter()  # 실제불변/누적관측이다.

    def dword(self,address,value=None):
        """현재DWORD를입력하거나조회한다."""
        if value is not None:self.mu.mem_write(address,pack([value]))
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def state(self):
        """현재실제10DWORD를원시순서로관측한다."""
        return ':'.join(str(x) for x in struct.unpack('<10I',self.mu.mem_read(STATE,40)))

    def window(self,address,count):
        """명시할당/고정입력/스택안의현재물리폭만허용한다."""
        if count<0:raise RuntimeError('음수메모리폭')
        if STACK-0x1000<=address and address+count<=STACK+32:return
        if SOURCE<=address and address+count<=SOURCE+0x4000:return
        if EXCLUDED<=address and address+count<=EXCLUDED+0x1000:return
        if OUTPUT<=address and address+count<=OUTPUT+0x810:return
        if any(base<=address and address+count<=base+len(block['bytes']) for base,block in self.blocks.items()):return
        raise RuntimeError('미명시메모리 %08x/%d'%(address,count))

    def memory_write(self,mu,access,address,size,value,data):
        """실제본문쓰기를현재상태/할당/출력/스택으로분류한다."""
        if STATE<=address and address+size<=STATE+40:key='state'
        elif STACK-0x1000<=address and address+size<=STACK+32:key='stack'
        elif OUTPUT<=address and address+size<=OUTPUT+0x810:key='output'
        else:self.window(address,size);key='blocks'
        self.write_total[key]+=1

    def returned(self,value=0,cleanup=0):
        """실제CALL복귀주소와규약의인자만소비한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EIP,self.dword(esp));self.mu.reg_write(UC_X86_REG_ESP,esp+4+cleanup);self.mu.reg_write(UC_X86_REG_EAX,uint(value))

    def event(self,name,values=()):
        """실제외부호출을기록하고장치기본효과뒤현재state변경을공급한다."""
        self.calls[name]+=1;self.calls_total[name]+=1;trigger=name+str(self.calls[name]);self.trace.append(name+(':'+':'.join(map(str,values)) if values else ''))
        for when,target,field,value in self.effects:  # 한경계의변경들은입력순서를유지한다.
            if when!=trigger:continue
            if target=='state':self.dword(STATE+FIELDS[field],int(value))
            elif target=='control' and field=='stop' and value=='1':self.stopped=True;self.mu.emu_stop()
            else:raise RuntimeError('미명시편집버퍼효과')

    def allocated(self,size,fail=False):
        """가드16BYTE를포함한실제소유블록을공급한다.원문capacity는변경하지않는다."""
        if fail:return 0
        if not 0<=size<=16384:raise RuntimeError('명시할당한도')
        base=BLOCKS+self.next_block*STRIDE;self.next_block+=1;payload=bytes([self.fill])*(size+16);self.mu.mem_write(base,payload);self.blocks[base]={'nominal':size,'live':1,'bytes':payload};return base

    def cstring(self,address):
        """표준strchr장치의NUL앞범위를순수하게읽는다."""
        result=bytearray()
        for index in range(32768):  # 명시한최대저장소안에서만탐색한다.
            self.window(address+index,1);value=self.mu.mem_read(address+index,1)[0]
            if not value:return bytes(result)
            result.append(value)
        raise RuntimeError('NUL검색한도')

    def instruction(self,mu,address,size,data):
        """부모기계어는실행하고명시외부함수만동작계약으로대체한다."""
        if address==RETURN:self.completed=True;mu.emu_stop();return
        if any(a<=address<b for a,b in RANGES):self.instructions+=1;return
        if address not in BOUNDARIES:raise RuntimeError('미명시코드 %08x'%address)
        name=BOUNDARIES[address];esp=mu.reg_read(UC_X86_REG_ESP);arg=lambda i:self.dword(esp+4+4*i)  # 실제CALL스택이다.
        if name=='strchr':
            source=arg(0);needle=arg(1)&255;string=self.cstring(source);offset=string.find(bytes([needle]));self.returned(source+offset if offset>=0 else source+len(string) if needle==0 else 0);return
        if name=='memmove':
            destination,source,count=arg(0),arg(1),arg(2);self.window(source,count);self.window(destination,count);payload=bytes(mu.mem_read(source,count));mu.mem_write(destination,payload);self.returned(destination);return
        if name=='allocate':
            size=arg(0);failed='allocate'+str(self.calls[name]+1) in self.failures;result=self.allocated(size,failed);self.event(name,[size,result]);cleanup=4 if address==0x451640 else 0
        elif name=='release':
            token=arg(0)
            if token:
                if token not in self.blocks:raise RuntimeError('미명시반납주소')
                self.blocks[token]['live']=0
            self.event(name,[token]);result=0;cleanup=4 if address==0x451650 else 0
        elif name=='resize':
            old,size=arg(0),arg(1);failed='resize'+str(self.calls[name]+1) in self.failures;result=self.allocated(size,failed)
            if result:
                if old:
                    original=self.blocks[old];count=min(original['nominal'],size);mu.mem_write(result,bytes(mu.mem_read(old,count)));original['live']=0
            self.event(name,[old,size,result]);cleanup=8
        elif name=='clipboardRead':
            destination,count=arg(0),arg(1);payload=bytes.fromhex(self.case['clipboard']);self.window(destination,count);mu.mem_write(destination,(payload+bytes(count))[:count]);self.event(name,[count]);result=1;cleanup=0
        else:
            source,count=arg(0),arg(1);self.window(source,count);self.clipboard=bytes(mu.mem_read(source,count));self.event(name,[self.clipboard.hex() or '-']);result=1;cleanup=0
        if not self.stopped:self.returned(result,cleanup)

    def run(self,case,control):
        """한진입을정상RET또는명시중단까지실행하고메모리전체를관측한다."""
        self.case=case;self.calls=Counter();self.trace=[];self.blocks={};self.completed=False;self.stopped=False;self.instructions=0;self.fill=int(case['fill']);self.clipboard=b''
        self.failures=set() if case['failures']=='-' else set(case['failures'].split(','));self.effects=[] if case['mutation']=='-' else [x.split('/') for x in case['mutation'].split(';')]
        self.mu.mem_write(STACK-0x1000,bytes([0xcc])*0x1040);self.mu.mem_write(OUTPUT,bytes([0x6d])*0x810);self.mu.mem_write(PAIR,pack([0x11223344,0x55667788]))
        self.mu.mem_write(STATE,pack(list(map(int,case['state'].split(':')))));self.mu.mem_write(SOURCE,bytes.fromhex(case['text'])+bytes(1));self.mu.mem_write(EXCLUDED,bytes.fromhex(case['excluded'])+bytes(1))
        if case['blocks']!='-':
            for item in case['blocks'].split(';'):  # 독립입력의기존할당을설치한다.
                base,nominal,live,payload=item.split(':');base=int(base);payload=bytes.fromhex(payload);self.blocks[base]={'nominal':int(nominal),'live':int(live),'bytes':payload};self.mu.mem_write(base,payload)
        self.next_block=len(self.blocks);arguments=list(map(int,case['arguments'].split(':'))) if case['arguments']!='-' else []
        kind=case['operation']
        if kind in ('insert','reset'):arguments=[SOURCE if arguments[0] else 0]
        elif kind=='paste':arguments=[EXCLUDED if arguments[0] else 0]
        elif kind=='copyAll':arguments=[OUTPUT,arguments[0]]
        elif kind=='copyLine':arguments=[arguments[0],OUTPUT,arguments[1],PAIR]
        self.mu.mem_write(STACK,pack([RETURN,*arguments]));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,STATE);self.mu.reg_write(UC_X86_REG_EAX,0x12345678);self.mu.reg_write(UC_X86_REG_EDX,0x23456789)
        for register,value in ((UC_X86_REG_EBX,0x33557799),(UC_X86_REG_ESI,0x446688aa),(UC_X86_REG_EDI,0x557799bb),(UC_X86_REG_EBP,0x6688aacc)):self.mu.reg_write(register,value)  # 실제보존레지스터검사다.
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0);self.mu.emu_start(ENTRIES[kind],RETURN+1,count=1000000)
        if not self.completed and not self.stopped:raise RuntimeError('실행한도')
        delta=mu_delta=self.mu.reg_read(UC_X86_REG_ESP)-STACK
        if self.completed:
            if delta!=4+len(arguments)*4:raise RuntimeError('스택불일치 %s/%d'%(case['name'],mu_delta))
            for register,value in ((UC_X86_REG_EBX,0x33557799),(UC_X86_REG_ESI,0x446688aa),(UC_X86_REG_EDI,0x557799bb),(UC_X86_REG_EBP,0x6688aacc)):  # 정상RET의callee보존이다.
                if self.mu.reg_read(register)!=value:raise RuntimeError('레지스터불일치')
        output=[]
        for base,block in sorted(self.blocks.items()):output.append(':'.join((str(base),str(block['nominal']),str(block['live']),bytes(self.mu.mem_read(base,len(block['bytes']))).hex())))  # 해제한블록/가드도관측한다.
        for a,original in self.original.items():  # 원문코드가바뀌지않았는지확인한다.
            if bytes(self.mu.mem_read(a,len(original)))!=original:raise RuntimeError('원문변경')
        result=self.mu.reg_read(UC_X86_REG_EAX)
        if kind=='construct' and self.completed:result=1
        return [int(self.completed),self.state(),';'.join(output) or '-',bytes(self.mu.mem_read(OUTPUT,2048)).hex(),':'.join(str(x) for x in struct.unpack('<2I',self.mu.mem_read(PAIR,8))),self.clipboard.hex() or '-',';'.join(self.trace) or '-',result,delta,self.instructions,0]


def make_case(name,kind,text=b'ab\ncd',selection=(1,3),argument=(),insert=b'Z',capacity=None,offset_capacity=128,dirty=1,failures='-',mutation='-',clipboard=b'A\tB\nC',excluded=b'',null=False):
    """입력바이트/장치만만들며원문출력은계산하지않는다."""
    if capacity is None:capacity=max(8,len(text)+1)
    data=BLOCKS;offsets=BLOCKS+STRIDE;payload=(text+b'\0'+bytes([0xa5])*(max(capacity,len(text)+1)+16-len(text)-1));table=bytes([0xa5])*(offset_capacity*4+16)
    state=[0x508da8,offsets,offset_capacity,7,dirty,*selection,len(text),capacity,data]
    blocks=';'.join(':'.join((str(base),str(nominal),'1',body.hex())) for base,nominal,body in ((data,capacity,payload),(offsets,offset_capacity*4,table)))  # 실제기존물리메모리입력이다.
    if kind=='construct':state=[0xa5a5a5a5]*10;blocks='-';argument=argument or (0,)
    if kind in ('insert','reset'):argument=(0 if null else 1,)
    if kind=='paste':argument=(0 if null else 1,)
    return dict(zip(INPUTS,(name,kind,':'.join(str(uint(x)) for x in state),blocks,':'.join(str(uint(x)) for x in argument) or '-',insert.hex(),excluded.hex(),clipboard.hex(),failures,mutation,165)))


def cases():
    """정상문자/경계선택/성장·실패/줄표/clipboard의독립입력이다."""
    rows=[]
    for size,fail in itertools.product((0,1,2048,3072),('-', 'allocate1','allocate2')):rows.append(make_case('ctor%04d'%len(rows),'construct',argument=(size,),failures=fail))  # 실제두할당과고정capacity필드다.
    for begin,end in itertools.product((-9,0,1,4,5,99),repeat=2):rows.append(make_case('select%04d'%len(rows),'select',argument=(begin,end)))  # signed정렬/clamp의양쪽경계다.
    for source,selection,capacity in itertools.product((b'',b'Q',b'xy',b'abcdef',b'A\0ignored',b'\x80\xff'),((0,0),(0,5),(1,3),(5,5)),(5,16)):
        rows.append(make_case('insert%04d'%len(rows),'insert',selection=selection,insert=source,capacity=capacity))  # 선택폭보다짧음/같음/김과끝NUL이다.
    for kind in ('insert','reset'):
        for fail in ('-','resize1'):rows.append(make_case('grow%04d'%len(rows),kind,insert=b'x'*1050,capacity=8,failures=fail))  # 1024단위증가와실패부분상태다.
    for text,line,selection in itertools.product((b'',b'one',b'a\nb',b'\n',b'x\ny\n'),(-1,0,1,2,8),((0,0),(0,99),(1,1),(2,4))):
        begin=min(selection[0],len(text));end=min(selection[1],len(text))
        rows.append(make_case('line%04d'%len(rows),'copyLine',text=text,selection=(begin,end),argument=(line,32)))  # LF포함행길이/끝빈줄/선택교차다.
    for kind,text in itertools.product(('lines','lineCount','lineLength'),(b'',b'a\nb',b'\n'*127,b'\n'*130)):
        for fail in ('-','allocate1'):rows.append(make_case('lines%04d'%len(rows),kind,text=text,selection=(0,0),argument=(1,) if kind=='lineLength' else (),failures=fail))  # 실제128칸경계의부분실패다.
    for kind,selection in itertools.product(('copy','cut'),((0,0),(1,3),(5,5))):rows.append(make_case('clip%04d'%len(rows),kind,selection=selection))  # 빈선택의전체복사/실제선택변경차이다.
    for source,excluded,null in itertools.product((b'',b'ABCD',b'A\tB\nC',b'xxxyxx'),(b'',b'\t\n',b'x'),(False,True)):rows.append(make_case('paste%04d'%len(rows),'paste',clipboard=source,excluded=excluded,null=null))  # 제외반복삭제와null필터차이다.
    for kind,capacity in itertools.product(('copyAll','copyLine'),(0,1,3,5,6,12)):rows.append(make_case('output%04d'%len(rows),kind,argument=(0,capacity) if kind=='copyLine' else (capacity,)))  # 실패출력보존과NUL크기경계다.
    for kind in ('destroy','reset'):rows.append(make_case('simple%04d'%len(rows),kind,null=True))  # 실제소멸과null초기화다.
    for kind in ('lines','lineCount'):rows.append(make_case('cached%04d'%len(rows),kind,dirty=0))  # dirty0은기존줄표/현재count를그대로보존한다.
    null_data=make_case('destroyNullData','destroy');fields=null_data['state'].split(':');fields[-1]='0';null_data['state']=':'.join(fields);rows.append(null_data)  # data0이면offset표도반납하지않는원문조건이다.
    for kind in ('copy','cut'):rows.append(make_case('emptyClipboard%04d'%len(rows),kind,text=b'',selection=(0,0)))  # 0길이복사억제와빈Cut의dirty쓰기를구별한다.
    mutations=(('insert','resize1/state/selectionBegin/0;resize1/state/selectionEnd/1'),('lines','allocate1/state/offsetCapacity/64'),('lines','release1/state/offsetCapacity/100'),('construct','allocate1/state/dirty/9'),('destroy','release1/state/offsets/0'),('paste','clipboardRead1/state/selectionBegin/0;clipboardRead1/state/selectionEnd/5'),('cut','clipboardWrite1/state/selectionBegin/0;clipboardWrite1/state/selectionEnd/1'),('insert','resize1/control/stop/1'),('lines','allocate1/control/stop/1'),('lines','release1/control/stop/1'),('construct','allocate2/control/stop/1'),('destroy','release1/control/stop/1'))  # 현재재조회와명시중단입력이다.
    for kind,effect in mutations:
        rows.append(make_case('mutation%04d'%len(rows),kind,text=b'\n'*130 if kind=='lines' else b'ab\ncd',insert=b'x'*1050,selection=(1,3),capacity=8 if kind!='lines' else 132,mutation=effect))  # 기대출력을읽지않는장치효과다.
    return rows


def main():
    """호스트/원문을고정하고두정밀도의전체현재메모리를새파일에쓴다."""
    if socket.gethostname().lower()!='vm-w11-codex':raise RuntimeError('현재/최종디컴파일호스트불일치')
    if hashlib.sha256(PE_PATH.read_bytes()).hexdigest()!=PE_SHA:raise RuntimeError('원문SHA불일치')
    oracle=Oracle();rows=[];inputs=cases();differences=0
    for case in inputs:  # 각입력의두실제CPU진입을독립실행한다.
        observed=[]
        for control in CONTROLS:
            output=oracle.run(case,control);observed.append(output);rows.append([case[key] for key in INPUTS]+output)  # 입력과실제관측을분리한다.
        if observed[0][:-2]!=observed[1][:-2]:differences+=1
    FIXTURE.write_text('\t'.join(INPUTS+OUTPUTS)+'\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8')
    report={'host':socket.gethostname(),'decompileHost':'VM-W11-Codex','decompileDate':'2026-10-23','peSha256':PE_SHA,'schema':list(INPUTS+OUTPUTS),'fields':FIELDS,'entryPoints':ENTRIES,'stateAddress':STATE,'blockBase':BLOCKS,'blockStride':STRIDE,'cases':len(inputs),'rows':len(rows),'completed':sum(row[11] for row in rows),'precisionDifferences':differences,'nativeInstructions':sum(row[-2] for row in rows),'calls':dict(oracle.calls_total),'writes':dict(oracle.write_total),'fixtureSha256':hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),'toolSha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'boundaries':{hex(k):v for k,v in BOUNDARIES.items()},'scope':['실제0x28버퍼13진입과중첩줄재생성/삽입본문실행','malloc/free/realloc·표준strchr/memmove·clipboard만명시경계','표준strchr/memmove는순수C메모리계약이며장치trace없음','alloc마다nominal+16가드포함전체메모리관측;원문exact-capacity NUL쓰기도기록','생성자는NUL미작성·요청nonzero에도capacity2048·data0소멸조건보존','메모리한도16384/최대16가상블록;기존입력문자열과재할당으로무효가되는자기참조는별도미검증','void/미완료EAX는의미반환값아님;constructor정상this는토큰1정규화','게임/GUI실행없음']}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps({key:report[key] for key in ('cases','rows','completed','precisionDifferences','nativeInstructions','fixtureSha256')},ensure_ascii=False))


if __name__=='__main__':main()
