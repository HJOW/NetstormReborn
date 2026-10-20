#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""전용 Help HTML의 name/info·스트림·실제 기본 파서/링크·마나 그림 문자열을 제품 변경 전에 관측한다."""
import argparse
import itertools
import json
import re
import socket
import struct
from decomp_mission_abort import Oracle as BaseOracle, ROOT, POOL, TEXT, TYPES, STACK, STOP, digest
from decomp_cursor_position import ranges
from taff import TaffArchive
from cpp_menu_smoke import source
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

OBJECT,PANE,VT,HEAP=POOL+0x90000,POOL+0x94000,POOL+0x95000,POOL+0xe0000  # 격리 파서·실제 pane 입력·동적 서식 결과다.
SOURCE,OUTPUT,LINKS=0x13000000,0x13100000,0x13200000  # 큰 보호 도움말 원문과 출력/링크의 별도 저장소다.
TOPIC=TEXT+0x8000  # 생성자와 원본 비교가 읽는 주제다.
PUSH=STOP+0x900  # 전용 정보 pane의 가상 Push만 명시 경계다.
FIXTURE=ROOT/'cpppj/tests/fixtures/quick-help-html-x86.tsv'  # 기존 기본 파서/요청 원문을 바꾸지 않는 첫 파생 관측이다.
REPORT=ROOT/'cpppj/quick-help-html-native.json'  # 실제 몸체와 명시 경계를 구별한다.


def bits(value):
    """단정도 입력을 원본 DWORD로 보존한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def cases():
    """주제 prefix/case·태그·현재 TYPE·링크/스트림·실제 보호 본문을 독립 공급한다."""
    rows=[]
    # name은 첫 글자만 소문자여야 하며 비교는 주제 길이의 prefix다.
    texts=['<a name="themeHelp">First <b>bold</b><br>Line</a>\nSTOP','<a NAME="themeHelp">Miss</a>','<a name="themeHelper">Prefix</a>tail','<a name=themeHelp>Broken</a>','<a name="other">No</a><a name="THEMEHELP">Yes</a>\nSTOP','<a name="themeHelp"><a href="#sunCannonType">Cannon</a><a href="cmd:TryAgain">Retry</a></a>\nSTOP','<a name="themeHelp"><info><p>After</a>\nSTOP','<a name="themeHelp"><INFO>Caps<info extra>After</a>\nSTOP']
    for index,stream,links in itertools.product(range(len(texts)),(0,1),(0,1)):rows.append(('D',(texts[index],'themeHelp',1 if index>=6 else 0,stream,links,8)))
    for topic in ('','theme','ThemeHelp','missing'):rows.append(('D',(texts[0],topic,0,0,1,8)))
    # 값0/소수/음수·NaN·특수 피해·무적·무원소·마나/클래스·pane Push를 교차한다.
    for profile,pane in itertools.product(range(1,9),(0,8)):rows.append(('D',('<a name="unitType"><info></a>\nSTOP','unitType',profile,1,1,pane)))
    # 기본 도움말의 실제 링크/종료 a 태그를 전체 원문에서 관측한다.
    for topic,profile in (('themeHelp',0),('sunCannonType',1),('vesselPriestType',2)):
        rows.append(('F',(topic,profile,1,1,8)))
    for text,prefix in itertools.product(('', 'w', 'wrtsWRTS.?>','w..r.t','....','WR.TS'),(-1,0,2,99)):rows.append(('M',(text,prefix)))
    for previous in ('\n','\r','<','x'):rows.append(('B',(previous,7)))
    return rows


class Oracle(BaseOracle):
    """실제 파생/기본 HTML·링크·마나 변환과 CRT를 실행하며 UI/번역/서식 효과만 명시한다."""
    def __init__(self):
        """원본 PE와 검토한 불연속 범위만 설치한다. 원본 게임/OS는 실행하지 않는다."""
        super().__init__();self.allowed=[];self.mu.mem_map(SOURCE,0x100000);self.mu.mem_map(OUTPUT,0x100000);self.mu.mem_map(LINKS,0x100000)
        ranges(self,(0x46bea0,0x46bed0,0x46bf00,0x46bf10,0x46bf40,0x46bfc0,0x46c090,0x46c810,0x4437f0,0x40b380,0x4c4c00,0x4c4c60,0x4c4cc0,0x4e66ed,0x4ef510,0x4ef5a0,0x4e49c0,0x49fce0,0x472e60,0x473070,0x473090,0x4735c0))
        self.allowed.append((0x4c5240,0x4c5256));self.put(VT+0x18,PUSH);self.put(PANE,VT);self.normal=0;self.stream=bytearray();self.events=[]
        self.call(0x472e60,[]);self.frames=self.words(0x592b78,128);self.help=source(ROOT/'originals',TaffArchive(ROOT/'originals/netstorm.tarc'),'d/help.english')

    def guard(self,mu,access,address,size,value,data):
        """실제 쓰기를 파서/출력/링크/마나 초기화·본문 억제 전역 안으로 제한한다."""
        if any(low<=address and address+size<=high for low,high in ((SOURCE,SOURCE+0x100000),(OUTPUT,OUTPUT+0x100000),(LINKS,LINKS+0x100000),(0x592990,0x592f48),(0x565ec0,0x565ec8))):return
        super().guard(mu,access,address,size,value,data)

    def format(self,pattern,arguments):
        """고정 %s/%d/%u와 %%만 명시 ABI 경계에서 서식화한다. 분기/선택은 원본이다."""
        pattern=self.string(pattern).decode('latin1');values=[];index=0
        # 실제 서식이 참조한 인자만 C 문자열 또는32비트 정수로 읽는다.
        for match in re.finditer(r'%(?:%|s|d|u)',pattern):
            code=match[0]
            if code=='%%':continue
            value=arguments[index];index+=1;values.append(self.string(value).decode('latin1') if code=='%s' else struct.unpack('<i',struct.pack('<I',value))[0] if code=='%d' else value)
        return (pattern%tuple(values)).encode('latin1')

    def call(self,address,args,ecx=0,purge=0):
        """큰 보호 본문의 실제 순회도 끝까지 실행하고 스택/비휘발·x87를 확인한다."""
        saved={UC_X86_REG_EBX:0x123456,UC_X86_REG_ESI:0x234567,UC_X86_REG_EDI:0x345678,UC_X86_REG_EBP:0x456789}
        # 실제 호출 직전의 계약만 공급한다.
        for register,value in saved.items():self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_FPCW,0x027f);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,ecx)
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));self.mu.emu_start(address,STOP,count=10000000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge
        assert all(self.mu.reg_read(register)==value for register,value in saved.items()) and self.mu.reg_read(UC_X86_REG_FPCW)==0x027f
        return self.mu.reg_read(UC_X86_REG_EAX)

    def instruction(self,mu,address,size,data):
        """검토한 계산은 원본에 맡기고 실제 pane/파일·번역/Config·서식만 관측한다."""
        if any(low<=address and address+size<=high for low,high in self.allowed):self.instructions+=1;return
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,12);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address in (0x4e53dc,0x4e5f7b):
            value=struct.unpack('<i',struct.pack('<I',args[0]))[0];self.ret(value+32 if address==0x4e53dc and 65<=value<=90 else value-32 if address==0x4e5f7b and 97<=value<=122 else value);return
        if address==0x49a840:self.ret(TYPES+500);return
        if address==0x49af80:self.ret(TYPES+500+0x18);return
        if address==0x4de9a0:self.ret(args[0],4);return
        if address==0x4a8940:
            self.ret(self.words(0x542444+args[0]*4,1)[0]);return
        if address==0x441740:self.ret(args[0]);return
        if address==0x4410f0:self.ret(args[0]);return
        if address in (0x4a89c0,0x4a89e0):
            result=self.format(args[1],args[2:]);result=(self.string(args[0])+result) if address==0x4a89e0 else result;mu.mem_write(args[0],result+b'\0');self.ret(args[0]);return
        if address==0x4b9390:
            result=self.format(args[1],args[2:]);assert len(result)<args[0];mu.mem_write(HEAP,result+b'\0');self.ret(HEAP);return
        if address==0x4e58e9:
            result=self.format(args[1],args[2:]);mu.mem_write(args[0],result+b'\0');self.ret(len(result));return
        if address==0x41a910:self.stream.extend(bytes(mu.mem_read(args[0],args[1])));self.ret(args[1],12);return
        if address==0x4ba8f0:self.events.append('Pane,'+self.string(args[0]).hex());self.ret(purge=4);return
        if address==0x4ba360:self.events.append('Scroll,'+str(args[0]));self.ret(purge=4);return
        if address==PUSH:self.events.append('Push');self.ret();return
        if address==0x419df0:self.events.append('PaneAttach');self.ret();return
        super().instruction(mu,address,size,data)

    def sample(self,kind,row):
        """독립 입력만 설치하고 실제 출력/파서/링크/스트림과 pane 효과를 그대로 읽는다."""
        self.events=[];self.stream=bytearray();self.put(0x565ec0,0);self.put(0x565ec4,0);self.mu.mem_write(OBJECT,bytes(0x50));self.mu.mem_write(LINKS,bytes(0x10000))
        if kind=='M':
            text,prefix=row;self.mu.mem_write(TOPIC,text.encode()+b'\0');self.call(0x4735c0,[0,prefix&0xffffffff],TOPIC,8);result=self.string(TOPIC).hex()
        elif kind=='B':
            previous,value=row;self.mu.mem_write(SOURCE,previous.encode()+b'/\0');self.put(OBJECT+4,SOURCE+1);self.put(OBJECT+0x28,value);self.call(0x4c5240,[],OBJECT);result=str(self.words(OBJECT+0x28,1)[0])
        else:
            if kind=='F':topic,profile,stream,links,pane=row;raw=self.help
            else:text,topic,profile,stream,links,pane=row;raw=text.encode()
            self.mu.mem_write(SOURCE,raw+b'\0\0\0');self.mu.mem_write(TOPIC,topic.encode()+b'\0');self.mu.mem_write(TYPES+500,bytes(500));self.mu.mem_write(TYPES+500+0x18,b'Unit Label\0');self.mu.mem_write(TYPES+500+0xaa,b'Class Label\0');self.put(PANE+4,pane);self.put(0x59ab20,TYPES);self.put(0x5412d0,1);self.put(0x5ca8f0,int(profile==2));self.put(0x5e4794,0)
            # 합성 TYPE은 native 분기의 독립 입력이다. 실제 파일/월드 TYPE로 가장하지 않는다.
            self.put(TYPES+500,101 if profile!=6 else 0);self.put(TYPES+500+0x98,profile%6);self.put(TYPES+500+0xe8,0x10 if profile!=7 else 0);self.put(TYPES+500+0xec,0x100 if profile==3 else 0)
            for offset,value in ((0x44,23.75),(0x48,3.25),(0xc4,17.75)):
                self.put(TYPES+500+offset,bits(float('nan') if profile==8 else -value if profile==5 else value if profile not in (3,4) else 0))
            self.mu.mem_write(TYPES+500+0xa0,('w.r.t' if profile in (2,5) else '').encode()+b'\0')
            if profile==4:self.mu.mem_write(TYPES+500+0xaa,b'\0')
            self.call(0x4c4c00,[1 if profile else 0,TOPIC,PANE,1 if stream else 0],OBJECT,16);self.call(0x46bed0,[OUTPUT,8192,LINKS if links else 0,0x10000],OBJECT,16);self.call(0x46bf00,[SOURCE],OBJECT,4)
            self.call(0x46c810,[],OBJECT)
            end=self.words(OBJECT+8,1)[0]-OUTPUT;linkend=self.words(OBJECT+0x10,1)[0]-LINKS if links else 0
            result='|'.join((bytes(self.mu.mem_read(OUTPUT,end)).hex(),','.join(map(str,(self.words(OBJECT+4,1)[0]-SOURCE,*self.words(OBJECT+0x1c,5),self.words(0x565ec0,1)[0],self.words(0x565ec4,1)[0]))),bytes(self.mu.mem_read(LINKS,linkend)).hex(),self.stream.hex(),';'.join(self.events)))
        self.normal+=1;return result


def main():
    """최초 관측을 보존하거나 현재 해시만 읽으며 기존 기대값은 덮어쓰지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text('utf-8'))
        for name,expected in record['sha256'].items():assert digest(ROOT/name)==expected,name
        print('Quick Help HTML 최초 PE 관측 보존 확인');return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('최초 관측 덮어쓰기 금지')
    oracle=Oracle();lines=['# 실제 파생/기본 HTML·링크·마나 출력. 생성/번역/Config/서식·pane/stream은 명시 경계다.'];rows=cases()
    # 입력을 먼저 정한 뒤 모든 결과를 원본 VM에서 읽는다. C++는 읽거나 실행하지 않는다.
    for kind,row in rows:
        encoded=json.dumps(row,ensure_ascii=True,separators=(',',':'));lines.append('\t'.join((kind,encoded,oracle.sample(kind,row))))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n');names=['originals/Netstorm.exe','originalCD/NETSTORM.EXE','originals/netstorm.tarc','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_quick_help_html.py','cpppj/tests/fixtures/quick-help-html-x86.tsv']
    record={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-20','inputs':len(rows),'normalReturns':oracle.normal,'instructions':oracle.instructions,'gameExecuted':False,'osExecuted':False,'helpSha256':__import__('hashlib').sha256(oracle.help).hexdigest(),'scope':'실제 전용 name/info/stream·기본 HTML/링크·마나 초기화/문자열·최대HP/ftol·CRT. TYPE getter/설명·ASCII case/번역/Config/서식·pane/파일 stream은 명시 경계다. 합성 TYPE와 보호 help.english이며 원본 창/게임/OS 관측이 아니다.','sha256':{name:digest(ROOT/name) for name in names}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:record[key] for key in ('inputs','normalReturns','instructions')}))


if __name__=='__main__':main()
