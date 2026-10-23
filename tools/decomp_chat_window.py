#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 ChatWindow 본체와 실제 Console/편집 버퍼를 격리 CPU에서 관측한다."""
import hashlib
import itertools
import json
import socket
import struct
from collections import Counter
from pathlib import Path
import decomp_text_entry_gump as entry
from unicorn.x86_const import *

ROOT=entry.ROOT  # 보호 PE와 새 고유 관측자료의 루트다.
FIXTURE=ROOT/'cpppj/tests/fixtures/chat-window-x86.tsv'  # 앞부분 입력과 실제 관측을 분리한다.
REPORT=ROOT/'extracted/cpp-chat-window-20261023-native.json'  # 실제 범위/명령/제외/SHA다.
BASE_FIXTURE=ROOT/'cpppj/tests/fixtures/chat-window-base-x86.tsv'  # 먼저마친83입력166관측을같은구버전스키마로보존한다.
CHAT=entry.raw.HEAP+0x3000  # 실제0x13c root다.
HELPER=entry.raw.HEAP+0x4000  # 실제 돌 버튼 장치다.
BACKGROUND=entry.raw.HEAP+0x5000  # 실제 평면 배경 장치다.
SCROLL=entry.raw.HEAP+0x6000  # 실제 ScrollingText 장치다.
SHARED=entry.raw.HEAP+0x7000  # Client 수명의 실제 공유 StyleTextBuffer 주소다.
PAYLOAD=entry.raw.HEAP+0x8000  # 현재 문자열 사건의 NUL 입력이다. raw STOP보다뒤에서별도설치한다.
LISTS=entry.raw.HEAP+0x180000  # 세 실제8BYTE String 배열의 명시 입력 저장소다.
VTABLE=entry.raw.HEAP+0x17f000  # root/helper/background/scroll의 실제 간접CALL 목적지다.
EXTRA=((0x431810,0x431bc8),(0x431bd0,0x431ed4),(0x431ee0,0x431f83),(0x42fab0,0x4300ed),(0x4300f0,0x43113a),(0x42ebc0,0x42ec91),(0x42e8d0,0x42e8e5),(0x42e8f0,0x42e976),(0x42e980,0x42e9dc),(0x42ea60,0x42ebb8))  # 목록 초기화를 포함한 정밀원문의 불연속 전체 범위다.
INPUTS=('name','operation','width','height','event','first','second','text','input','ready','battle','challenge','config','recipient','resolve','colors','backgrounds','icons','entryNode','entryBuffer','entryBlocks')  # 기대값을 계산하지 않는 독립 장치/입력이다.
OUTPUTS=('rootout','consoleout','scrollout','prefix','inputout','completions','trace','result','stackdelta','nativeinstructions')  # 실제 raw/장치 관측이다.
GLOBAL_EVENTS={0x542b24:0x2000201,0x5427a8:0x2000021,0x5429f0:0x20000bf,0x542914:0x2000202,0x5427d8:0x2000038,0x542794:0x2000203,0x542af8:0x2000204,0x54298c:0x2000205,0x542b08:0x2000206,0x542b0c:0x2000207,0x5428ac:0x2000051}  # 원문 분기를 선택하는 명시 현재 이름표 값이다.


def signed(value):
    """원문 signed DWORD를 그대로 읽는다."""
    return entry.raw.signed(value)


def hextext(value):
    """원시 문자열을 탭 없는16진으로만기록한다."""
    return value.hex() or '-'


class Oracle(entry.Oracle):
    """공통 자식 생성/메뉴 장치 외 부모·Console·버퍼는 실제 코드다."""
    def __init__(self):
        """기존 Console 실제 실행기에 Chat 부모의 보호범위를추가한다."""
        super().__init__();self.ranges=(*entry.RANGES,*EXTRA);self.original.update({a:bytes(self.mu.mem_read(a,b-a)) for a,b in EXTRA});self.mu.mem_map(entry.raw.HEAP+0x200000,0x10000)

    def window(self,address,count):
        """새 root/자식/문자열배열과 기존 Console/버퍼만허용한다."""
        if CHAT<=address and address+count<=SHARED+0x400:return
        if VTABLE<=address and address+count<=entry.raw.HEAP+0x200000:return
        return super().window(address,count)

    def memory_write(self,mu,access,address,size,value,data):
        """부모/현재전역쓰기를기존 실제편집버퍼쓰기와구별한다."""
        if address==0x54878c and size==4:self.event('appendCallback',[0x548758,value]);self.write_total['callback']+=1;return
        if CHAT<=address and address+size<=CHAT+0x13c:self.write_total['chat']+=1;return
        if address in (0x548740,0x548744,0x54da91):self.write_total['globals']+=1;return
        return super().memory_write(mu,access,address,size,value,data)

    def instruction(self,mu,address,size,data):
        """부모 actual명령과 간접가상호출을 실행하고 명시 공통장치만관측한다."""
        if address==entry.raw.HEAP+0x200000:return
        if any(a<=address<b for a,b in EXTRA):self.instructions+=1;return
        esp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX);arg=lambda i:self.dword(esp+4+i*4)  # 실제ABI다.
        if address==0x4e4391:
            caller=self.dword(esp)
            devices={0x431879:HELPER,0x431939:BACKGROUND,0x4319a5:entry.NODE,0x431a12:SCROLL,0x431f38:CHAT}
            if caller in devices:self.event('new',[arg(0),devices[caller]]);self.returned(devices[caller]);return
        if address==0x4642c0:
            self.dword(this,VTABLE);self.dword(this+4,arg(3));self.dword(this+0x38,entry.bits(1));self.dword(this+0x3c,arg(1));self.dword(this+0x40,arg(2));self.dword(this+0x44,arg(1));self.dword(this+0x48,arg(2));self.event('constructBase',[this,*[arg(i) for i in range(4)]]);self.returned(this,16);return
        if address==0x4254b0:
            self.dword(this,VTABLE);self.dword(this+4,1);self.dword(this+0x38,entry.bits(1));self.event('constructHelper',[this,hextext(self.cstring(arg(2))),arg(3)]);self.returned(this,20);return
        if address==0x424f90:
            self.dword(this+0x3c,entry.bits(9));self.dword(this+0x40,entry.bits(14));self.event('fitHelper',[this]);self.returned();return
        if address in (0x464ed0,0x464f80):self.event('title' if address==0x464ed0 else 'help',[arg(0),hextext(self.cstring(arg(1)))]);self.returned();return
        if address==0x4de9a0:self.event('translate',[hextext(self.cstring(arg(0)))]);self.returned(arg(0),4);return
        if address==0x465020 and this==CHAT:
            self.dword(arg(1),6);self.dword(arg(2),14);self.event('measureChat',[this,hextext(self.cstring(arg(0)))]);self.returned(0,20);return
        if address==0x4655d0 and this==BACKGROUND:
            self.dword(this,VTABLE);self.dword(this+4,1);self.dword(this+0x38,entry.bits(1));self.dword(this+0x3c,arg(0));self.dword(this+0x40,arg(1));self.dword(this+0xb4,arg(2));self.event('constructChatBackground',[this,arg(0),arg(1),arg(2)]);self.returned(this,24);return
        if address==0x4bc060:
            self.dword(this,VTABLE);self.dword(this+4,1);self.dword(this+0x38,entry.bits(1));self.dword(this+0x3c,entry.bits(signed(arg(0))));self.dword(this+0x40,entry.bits(signed(arg(1))));self.dword(this+0xa4,SHARED+0x100);self.dword(this+0xd0,0x10);self.event('constructScrolling',[this,*[arg(i) for i in range(4)]]);self.returned(this,16);return
        if address==0x4bc3b0:self.dword(this+0xa4,arg(0));self.dword(this+0xa8,arg(1));self.event('replaceBuffer',[this,arg(0),arg(1)]);self.returned(0,8);return
        if address in (0x4ba1b0,0x4ba230):self.event('setSlider' if address==0x4ba1b0 else 'setScrollable',[this,arg(0)]);self.returned(0,4);return
        if address==entry.DEVICE+0x20 and this in (CHAT,HELPER,BACKGROUND,SCROLL):
            self.event('attach',[this,*[arg(i) for i in range(4)]]);self.returned(0,16);return
        if address in (0x4ba360,0x4ba380,0x4ba3b0):self.event({0x4ba360:'requestScroll',0x4ba380:'relativeScroll',0x4ba3b0:'requestBottom'}[address],[this,*([arg(0)] if address!=0x4ba3b0 else [])]);self.returned(0,0 if address==0x4ba3b0 else 4);return
        if address==0x464990:self.event('baseHandle',[this,arg(0),arg(1),arg(2)]);self.returned(77,12);return
        if address==0x42da00:self.event('sendChat',[this,hextext(self.cstring(arg(0)))]);self.returned(0,4);return
        if address==0x484190:
            self.mu.mem_write(this,bytes(60));self.dword(this+0x34,arg(0));self.returned(0,4);return
        if address==0x4841c0:
            self.event('resolve',[self.dword(this+4)]);result=self.chat_case['resolve'];self.mu.mem_write(this+8,(bytes.fromhex(result) if result!='~' else b'')+b'\0');self.returned(int(result!='~'));return
        if address==0x4e6770:
            source,needle=arg(0),self.cstring(arg(1));offset=self.cstring(source).find(needle);self.returned(source+offset if offset>=0 else 0);return
        if address==0x431580:
            end=self.dword(this+8);self.string_value(end,self.strings[arg(2)]);self.dword(this+8,end+8);self.dword(this+12,end+8);self.returned(0,12);return
        if address==0x49a840:
            token=LISTS+0xc000+arg(0)*0x200;self.dword(token+0x114,7)
            if self.dword(esp)==0x431d6f:self.event('frameCount',[arg(0),7])
            self.returned(token);return
        if address==0x419850:
            typ=(this-LISTS-0xc000)//0x200;frame=arg(1);height=(-1,7,8,9,16,17,32767)[frame];token=LISTS+0xd000+frame*0x40;self.mu.mem_write(token,bytes(36));self.mu.mem_write(token+0x1a,struct.pack('<h',height));self.event('frameHeight',[typ,frame,entry.raw.uint(height)]);self.returned(token,8);return
        if address==0x4c2630:self.event('log',[arg(0),hextext(self.cstring(arg(1))),arg(2)]);self.returned();return
        if address==0x43f660:self.event('register',[hextext(self.cstring(arg(0))),arg(1),arg(2)]);self.returned();return
        if address in (0x477bb0,0x477ee0,0x477cb0,0x477dd0,0x475c00,0x478160,0x475c40,0x475ca0,0x441090):
            if address==0x477bb0:self.event('menuBegin');result=0
            elif address==0x477ee0:self.event('menuTitle',[hextext(self.cstring(arg(0)))]);result=0
            elif address==0x477cb0:self.event('menuItem',[hextext(self.cstring(arg(0))),arg(1),hextext(self.cstring(arg(2))) if arg(2) else '~']);result=0
            elif address==0x477dd0:self.event('menuChecked',[arg(0),hextext(self.cstring(arg(1))),arg(2),hextext(self.cstring(arg(3)))]);result=0
            elif address==0x475c00:self.menu_number+=1;self.event('menuTake',[self.menu_number]);result=self.menu_number
            elif address==0x478160:self.event('menuSubmenu',[hextext(self.cstring(arg(0))),arg(1)]);result=0
            elif address==0x475c40:self.event('menuFinalize');result=0
            elif address==0x475ca0:self.event('menuOpen',[arg(0),arg(1),arg(2)]);result=0
            else:self.event('config',[hextext(self.cstring(arg(0)))]);result=int(self.chat_case['config'])
            self.returned(result);return
        if address in (0x4e3180,0x4e3590,0x4e3550):
            value=str(signed(arg(0))).encode() if address==0x4e3550 else self.cstring(arg(0));self.string_value(this,(b'' if address==0x4e3180 else self.strings.get(this,b''))+value);self.returned(this,4);return
        return super().instruction(mu,address,size,data)

    def run(self,case,control):
        """앞부분 입력으로 현재 Console/네 노드를설치하고 전체부모RET까지실행한다."""
        prep=entry.make_case('handle',text=bytes.fromhex(case['input']),selection=(0,0));prep['operation']='prepare';prep['arguments']='-';entry.ENTRIES['prepare']=entry.raw.HEAP+0x200000;self.mu.mem_write(entry.ENTRIES['prepare'],b'\xc3');super().run(prep,control)
        self.case['operation']='construct' if case['operation'] in ('construct','create') else 'handle';self.chat_case=case;self.trace=[];self.instructions=0;self.completed=False;self.menu_number=0
        for node in (CHAT,HELPER,BACKGROUND,SCROLL):self.mu.mem_write(node,bytes([0])*0x180);self.dword(node,VTABLE);self.dword(node+4,1);self.dword(node+0x38,entry.bits(1))  # 현재독립장치노드다.
        self.dword(VTABLE+8,entry.DEVICE);self.dword(VTABLE+0x18,entry.DEVICE+0x10);self.dword(VTABLE+0x20,entry.DEVICE+0x20)
        for offset,token in ((0xa0,entry.NODE),(0xa4,SCROLL),(0xa8,HELPER),(0xac,BACKGROUND)):self.dword(CHAT+offset,token)  # 원문네자식순서다.
        self.dword(CHAT+0x3c,entry.bits(signed(int(case['width']))));self.dword(CHAT+0x40,entry.bits(signed(int(case['height']))));self.dword(entry.NODE+0x2c,21);self.dword(HELPER+0x2c,23);self.dword(SCROLL+0xa4,SHARED);self.mu.mem_write(PAYLOAD,bytes.fromhex(case['text'])+b'\0')
        for address,value in {**GLOBAL_EVENTS,0x548740:CHAT,0x548744:int(case['ready']),0x54da91:0 if case['colors']=='~' else 1,0x541348:2,0x594fbc:int(case['battle']),0x594fc8:int(case['challenge']),0x548714:0x11223344,0x5b5dc0:32,0x5b5e60:30,0x5b5dd4:31,0x5b5dd8:33,0x5cae94:11,0x5cae98:22,0x563e38:33}.items():self.dword(address,value)  # 원문전역의실제현재입력이다.
        self.mu.mem_write(0x540424,b'\x09');self.case['recipient']=case['recipient']
        for index,(name,base,begin,end) in enumerate((('colors',LISTS,0x54de8c,0x54de90),('backgrounds',LISTS+0x4000,0x54deac,0x54deb0),('icons',LISTS+0x8000,0x54de9c,0x54dea0))):
            values=[] if case[name]=='~' else [bytes.fromhex(value) for value in case[name].split(':')];self.dword(begin,base);self.dword(end,base+len(values)*8);self.dword(end+4,base+len(values)*8)
            for item,value in enumerate(values):self.string_value(base+item*8,value)  # 실제 getter가 읽는 객체별 독립 포인터 입력이다.
        kind=case['operation'];arguments=[int(case['width']),int(case['height'])] if kind=='construct' else [] if kind in ('create','destroy','normalize') else [int(case['event']),int(case['first']),PAYLOAD if case['second']=='~' else int(case['second'])]
        entry_point={'construct':0x431810,'create':0x431ee0,'destroy':0x42e8d0,'normalize':0x42ebc0,'handle':0x42fab0}[kind];self.mu.mem_write(entry.raw.STACK,entry.raw.pack([entry.raw.RETURN,*arguments]));self.mu.reg_write(UC_X86_REG_ESP,entry.raw.STACK);self.mu.reg_write(UC_X86_REG_ECX,CHAT);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0);self.mu.emu_start(entry_point,entry.raw.RETURN+1,count=3000000)
        if not self.completed:raise RuntimeError('ChatWindow 정상RET없음')
        delta=self.mu.reg_read(UC_X86_REG_ESP)-entry.raw.STACK
        if delta!=4+len(arguments)*4:raise RuntimeError('ChatWindow 스택불일치')
        root=self.values(CHAT,(0,4,0x20,0x3c,0x40,0x44,0x48,0xa0,0xa4,0xa8,0xac,0xb0,0xb4))+':'+':'.join(str(self.dword(address)) for address in (0x548740,0x548744,0x54da91));scroll=self.values(SCROLL,(0xa4,0xa8,0xd0,0xd4));prefix=self.cstring(CHAT+0xb8);input_bytes=self.cstring(self.dword(entry.raw.STATE+36));result=self.mu.reg_read(UC_X86_REG_EAX) if kind=='handle' else 0
        for address,value in self.original.items():
            if bytes(self.mu.mem_read(address,len(value)))!=value:raise RuntimeError('보호본문변경')
        completions=[]
        for begin,end in ((0x54de8c,0x54de90),(0x54deac,0x54deb0),(0x54de9c,0x54dea0)):completions.append(':'.join(self.strings[index].hex() for index in range(self.dword(begin),self.dword(end),8)) or '~')  # 실제생성결과만관측한다.
        return [root,self.values(entry.NODE,entry.FIELDS),scroll,hextext(prefix),hextext(input_bytes),';'.join(completions),';'.join(self.trace) or '-',result,delta,self.instructions]


def cases():
    """원본출력을계산하지않고키/모드/명령/문자열/실제목록입력만교차한다."""
    rows=[];lists=['7e5b432e305d:7e5b432e315d','7e5b422e305d:7e5b422e315d','7e5b49312e305d:7e5b49312e315d']  # 최소실제문자열장치입력이다.
    def add(kind,**values):
        """앞18열의raw입력만준비한다."""
        case=dict(zip(INPUTS,('',kind,200,100,0,0,0,'',b'Bob:hello'.hex(),1,0,0,0,1,'~',*lists)));case.update(values);case['name']='chat%04d'%len(rows);prep=entry.make_case('handle',text=bytes.fromhex(case['input']),selection=(0,0));case.update(entryNode=prep['node'],entryBuffer=prep['buffer'],entryBlocks=prep['blocks']);rows.append(case)
    for width,height in itertools.product((80,200,939),(0,100,500)):add('construct',width=width,height=height,input='')  # 전체유효생성진입이다.
    for operation in ('construct','create'):add(operation,input='',colors='~',backgrounds='~',icons='~',ready=0)  # 색512문자열과현재2TYPE의8..16높이아이콘초기화다.
    for ready in (0,1):add('create',ready=ready,input='')  # 명령한번등록및현재root설치다.
    for key,modifier in itertools.product((0,9,13,27,0x10000,0x20000,0x210000,0x220000,0x230000,0x240000,0x260000,0x280000),(0,0x20000000,0x80000000)):add('handle',event=key|modifier)  # 일반키/release관문이다.
    for text in (b'copy',b'paste',b'~B',b'',b'COPY'):add('handle',event=GLOBAL_EVENTS[0x542b24],text=text.hex(),second='~')  # 문자열사건은대소문자구별한다.
    for event in (0x5429f0,0x5427d8,0x54298c,0x542794,0x542af8):
        for first in (21,99):add('handle',event=GLOBAL_EVENTS[event],first=first,second=404,resolve=b'Alice'.hex() if event==0x542794 else '~')  # 실제serial/identity/실패흐름이다.
    for source,recipient in itertools.product((b'',b'hello',b'Bob:hello',b'Bob:Alice:hello'),(0,1)):add('handle',event=GLOBAL_EVENTS[0x5427d8],first=21,input=source.hex(),recipient=recipient)  # Enter후접두와본문의두조회다.
    for battle,challenge,config in itertools.product((0,1),repeat=3):add('handle',event=GLOBAL_EVENTS[0x5427a8],first=23,battle=battle,challenge=challenge,config=config)  # 실제helper메뉴전체분기다.
    for source in (b'',b'hello',b'Bob:hello',b'Bob:Alice:hello'):add('normalize',input=source.hex())  # 원문보조본문전체다.
    add('destroy')
    for recipient,event in itertools.product((0,1),(0x542794,0x542af8)):add('handle',event=GLOBAL_EVENTS[event],first=21,second=404,input=b'Bob Alice:hello'.hex(),resolve=b'Alice'.hex(),recipient=recipient)  # 수신자중복추가/실제제거와유효성분기다.
    strings=lambda count:':'.join(('v%d'%index).encode().hex() for index in range(count))  # 그룹경계용독립String배열입력이다.
    add('handle',event=GLOBAL_EVENTS[0x5427a8],first=23,battle=1,colors=strings(45),backgrounds=strings(45),icons=strings(25));return rows


def main():
    """같은호스트/SHA로두x87관측을새파일에만저장한다."""
    if socket.gethostname().lower()!='vm-w11-codex':raise RuntimeError('호스트불일치')
    if hashlib.sha256(entry.raw.PE_PATH.read_bytes()).hexdigest()!=entry.raw.PE_SHA:raise RuntimeError('PE SHA불일치')
    oracle=Oracle();rows=[];different=[];inputs=cases()
    for case in inputs:
        outputs=[]
        for control in entry.raw.CONTROLS:
            try:output=oracle.run(case,control)
            except Exception as error:raise RuntimeError('%s/%s: %s'%(case['name'],case['operation'],error)) from error
            outputs.append(output);rows.append([case[key] for key in INPUTS]+output)
        if outputs[0][:-1]!=outputs[1][:-1]:different.append(case['name'])
    FIXTURE.write_text('\t'.join(INPUTS+OUTPUTS)+'\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    retained=[]
    for row in rows:  # 초기화추가전의83입력을동일순서/기존열로따로보존한다.
        if row[15]=='~' or len(retained)>=166:continue
        saved=row[:len(INPUTS)+5]+row[len(INPUTS)+6:];saved[0]='chat%04d'%(len(retained)//2);retained.append(saved)
    base_schema=INPUTS+tuple(value for value in OUTPUTS if value!='completions');BASE_FIXTURE.write_text('\t'.join(base_schema)+'\n'+'\n'.join('\t'.join(map(str,row)) for row in retained)+'\n',encoding='utf-8',newline='\n')
    report=dict(host=socket.gethostname(),decompileHost='VM-W11-Codex',decompileDate='2026-10-23',peSha=entry.raw.PE_SHA,inputs=len(inputs),rows=len(rows),different=different,instructions=sum(row[-1] for row in rows),ranges=oracle.ranges,calls=dict(oracle.calls_total),writes=dict(oracle.write_total),schema=INPUTS+OUTPUTS,stringGetter={'address':'0046ee50','bytes':'8b01c3','actualBody':True,'objectDataPointer':True},entryToolSha=hashlib.sha256(Path(entry.__file__).read_bytes()).hexdigest(),toolSha=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixtureSha=hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),limitations=['ChatWindow 생성/공장/Handle/수신자추가제거/접두/소멸과중첩TextEntry/버퍼/_ftol은실제본문','한번색/아이콘목록초기화루프도actual;String/벡터append/TYPE·프레임헤더는명시장치','공통Root/Helper/Background/ScrollingText생성·교체버퍼/메뉴/Identity/Chatmanager는명시장치','Normalize42ebc0및42ea60/42e8f0/42e980은실제본문이며strstr만순수C계약','String은독립순수계약이며실제글꼴/픽셀/GUI/통신완주증거가아님','실제네자식어댑터조립과전체공통트리링크/공유버퍼수명은후속통합검증'])
    report.update(baseRows=len(retained),baseFixtureSha=hashlib.sha256(BASE_FIXTURE.read_bytes()).hexdigest());REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:report[key] for key in ('inputs','rows','different','instructions','fixtureSha','baseFixtureSha')},ensure_ascii=False))


if __name__=='__main__':main()
