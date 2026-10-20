#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""전용 Help의 전체 생성/배치/갱신과 HTML 창의 실제 필드·호출 순서를 제품 변경 전에 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_quick_help_html import Oracle as BaseOracle, ROOT, POOL, SOURCE, OUTPUT, TYPES, TEXT, STOP, UC_X86_REG_ECX, UC_X86_REG_ESP, digest
from decomp_cursor_position import ranges

WINDOW,DECORATION,INNER,PICTURE_BG,BODY_BG,BODY,LEARN,BACK,CANCEL,INFO,PICTURE,SCROLL,BUFFER=(POOL+x for x in (0x40000,0x41000,0x42000,0x43000,0x44000,0x45000,0x4a000,0x4b000,0x4c000,0x4d000,0x4e000,0x4f000,0x50000))  # 각 노드는 서로 겹치지 않는 입력 영역이다.
VT=POOL+0x51000  # 명시 가상 경계의 표다.
PUSH,ATTACH=STOP+0xa00,STOP+0xb00  # 장치 없는 Push/Attach의 경계다.
NODES=(WINDOW,DECORATION,INNER,PICTURE_BG,BODY_BG,BODY,LEARN,BACK,CANCEL,INFO,PICTURE,SCROLL)  # 관측 순서를 고정한 노드다.
FIXTURE=ROOT/'cpppj/tests/fixtures/quick-help-window-x86.tsv'  # 기존 제어/HTML 원문과 구별한 전체 창 관측이다.
REPORT=ROOT/'cpppj/quick-help-window-native.json'  # 실제 몸체와 GUI/Config/파일 경계를 함께 보존한다.


def bits(value):
    """단정도 입력/결과를 반올림 없는 DWORD로 보존한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def cases():
    """창 생성·캐시·스크립트 표식·자식 유무와 실제 배치/본문 전이를 교차한다."""
    rows=[]
    # 생성자의 실제 전체 순서와 캐시/스크립트 분기를 독립 공급한다.
    for teach,cached,script,scale in itertools.product((0,71),(0,1),(0,1),(bits(1),bits(1.25))):rows.append(('C',(teach,cached,script,scale)))
    # 필수 Learn 외 Cancel/Back은 선택 가능하며 그림/정보의 유무도 교차한다.
    for picture,info,buttons,profile in itertools.product((0,1),(0,1),range(4),range(3)):rows.append(('L',(picture,info,buttons,profile)))
    # clear와 plain HTML child의 소유/Push·기존 위치 재조회를 보존한다.
    for old,void in itertools.product((0,1),(0,1)):rows.append(('H',(old,void)))
    for kind,topic,frame,old in itertools.product((0,1),('', 'unitType'),(-1,3),(0,1)):rows.append(('S',(kind,topic,frame,old)))
    return rows


class Oracle(BaseOracle):
    """실제 창/HTML 부모 몸체와 기존 실제 파생 HTML·ftol·CRT를 실행한다."""
    def __init__(self):
        """실제 게임/창을 시작하지 않고 허용 함수 범위와 격리 노드를 추가한다."""
        self.operation='X';self.actualPush=set();super().__init__();self.mu.mem_map(0,0x1000)
        ranges(self,(0x4c5380,0x4c58a0,0x4c5970,0x4c5d60,0x46c9c0,0x46ca50,0x46ca70,0x46cac0,0x46cbc0,0x46cc30,0x4e4a40))
        self.put(VT+0x18,PUSH);self.put(VT+0x20,ATTACH);self.actualPush={self.words(table+0x18,1)[0] for table in (0x514ec4,0x50bd38)}

    def guard(self,mu,access,address,size,value,data):
        """원본 쓰기를 노드/부모 버퍼/SEH·도움말 전역에 한정한다."""
        if address==0 and size==4:return
        if 0x5ca934<=address and address+size<=0x5ca9b8:return
        super().guard(mu,access,address,size,value,data)

    def event(self,*values):
        """명시 GUI/파일 효과의 대상/인자와 순서를 그대로 기록한다."""
        self.events.append(','.join(map(str,values)))

    def base(self,token,width=0,height=0,flags=1):
        """실제 기본 생성자는 별도 검증 대상이며 이 관측에는 독립된 현재 필드를 공급한다."""
        self.mu.mem_write(token,bytes(0xa0));self.put(token,VT);self.put(token+4,flags|8);self.put(token+0x2c,100+NODES.index(token));self.put(token+0x38,self.scale);self.put(token+0x3c,width);self.put(token+0x40,height);self.put(token+0x44,bits(struct.unpack('<f',struct.pack('<I',width))[0]*self.scaleValue));self.put(token+0x48,bits(struct.unpack('<f',struct.pack('<I',height))[0]*self.scaleValue))

    def attach(self,token,args):
        """같은 노드의 현재 좌표/부모/void를 바꾸는 명시 기본/가상 부착이다."""
        self.event('Attach',token,*args[:4]);self.put(token+0x4c,args[0]);self.put(token+0x50,args[1]);self.put(token+4,self.words(token+4,1)[0]&~8)
        if args[2]:self.put(token+0x14,args[2])

    def instruction(self,mu,address,size,data):
        """실제 창 계산/재조회는 계속 실행하고 기존 GUI 생성/입력·파일/Config만 공급한다."""
        if any(low<=address and address+size<=high for low,high in self.allowed):self.instructions+=1;return
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,10);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==0x4e4391:
            size=args[0];index=self.allocated.get(size,0);self.allocated[size]=index+1
            token=({0xc4:(DECORATION,),0xb8:(INNER,PICTURE_BG,BODY_BG),0x40a4:(BODY,),0x170:(LEARN,BACK,CANCEL),0xd8:(INFO,SCROLL),0xb4:(PICTURE,)})[size][index]
            self.mu.mem_write(token,b'\xa5'*size);self.event('New',size,token);self.ret(token);return
        if address==0x4642c0:self.event('Base',ecx,*args[:4]);self.base(ecx,args[1],args[2],args[3]);self.ret(ecx,16);return
        if address==0x465d10:self.event('Decoration',ecx,*args[:2]);self.base(ecx,args[0],args[1]);self.put(ecx+8,0x20);self.put(ecx+0x34,0x123);self.ret(ecx,8);return
        if address==0x4655d0:self.event('Background',ecx,*args[:6]);self.base(ecx,args[0],args[1],args[3]);self.ret(ecx,24);return
        if address==0x4254b0:
            self.event('Stone',ecx,*args[:2],self.string(args[2]).hex(),*args[3:5]);self.base(ecx,bits(60),bits(20),0);self.ret(ecx,20);return
        if address==0x424f90:self.event('Fit',ecx);self.ret();return
        if address==0x4a2820:self.event('Color',*args[:3]);self.ret(35);return
        if address==0x4c79a0:self.event('Script');self.ret(self.script);return
        if address==0x441300:self.event('Config',self.string(args[0]).hex());mu.mem_write(args[1],b'd/help.english\0');self.ret(1);return
        if address==0x41baa0:self.event('Load',self.string(args[0]).hex(),args[1],args[3]);self.put(args[2],len(self.helpText));self.ret(SOURCE);return
        if address==0x466750:self.attach(ecx,args);self.ret(purge=16);return
        if address==ATTACH:self.attach(ecx,args);self.ret(purge=16);return
        if address==PUSH or address in self.actualPush:self.event('Push',ecx);self.put(ecx+4,self.words(ecx+4,1)[0]|8);self.ret();return
        if address==0x466480:self.event('Kill',ecx);self.put(ecx+4,self.words(ecx+4,1)[0]|0x10000);self.ret();return
        if address==0x4bc060:self.event('Text',ecx,*args[:4]);self.base(ecx);self.put(ecx+0xa4,BUFFER);self.put(BUFFER+4,1);self.ret(ecx,16);return
        if address==0x4ba8f0:self.event('Styled',ecx,self.string(args[0]).hex());self.ret(purge=4);return
        if address in (0x4ba360,0x4ba1b0,0x4ba230):self.event({0x4ba360:'Scroll',0x4ba1b0:'Slider',0x4ba230:'Scrollable'}[address],ecx,args[0]);self.ret(purge=4);return
        if address==0x419df0:self.event('Reattach',ecx);self.put(ecx+4,self.words(ecx+4,1)[0]&~8);self.ret();return
        if address==0x441660:
            self.event('Substitute',self.string(args[1]).hex(),args[2]);mu.mem_write(args[0],self.string(args[1])+b'\0');self.ret();return
        if address==0x419fc0:self.event('Picture',ecx,*args[:3]);self.base(ecx,bits(99),bits(88));self.ret(ecx,12);return
        super().instruction(mu,address,size,data)

    def prepare(self):
        """이전 관측의 노드/전역/버퍼/할당 순서를 격리한다."""
        self.operation='X';self.events=[];self.stream=bytearray();self.allocated={};self.scale=bits(1);self.scaleValue=1;self.script=0;self.helpText=b'<a name="unitType"><info><p>BODY</a>\nEND';self.mu.mem_write(SOURCE,self.helpText+b'\0\0');self.mu.mem_write(TEXT+0xa000,b'unitType\0')
        # 같은 생성/현재 노드의 독립 기본 필드를 설치한다.
        for token in NODES:self.mu.mem_write(token,bytes(0xd0));self.base(token,bits(450),bits(350));self.put(token+4,1)
        self.put(WINDOW,0x514ec4);self.put(WINDOW+0x20,19);self.put(WINDOW+0xa0,0);self.put(WINDOW+0xa4,DECORATION);self.put(WINDOW+0xa8,INFO);self.put(WINDOW+0xac,BODY);self.put(WINDOW+0xb0,CANCEL);self.put(WINDOW+0xb4,LEARN);self.put(WINDOW+0xb8,BACK);self.put(WINDOW+0xbc,0);self.put(WINDOW+0xc0,INNER);self.put(WINDOW+0xc4,PICTURE_BG);self.put(WINDOW+0xc8,BODY_BG);self.put(WINDOW+0xcc,0)
        self.put(BODY+0xa0,0);self.put(INFO+0xa4,BUFFER);self.put(BUFFER+4,1);self.put(0x531860,1024);self.put(0x531864,768);self.put(0x5b5dc8,47);self.put(0x5acd24,0);self.put(0x5ca938,0x20000a0);self.mu.mem_write(0x5ca940,b'before\0'+bytes(73));self.put(0x5ca990,0);self.put(0x5ca994,SOURCE);self.mu.mem_write(0x5cab40,b'mission\0');self.put(0x594fbc,0);self.put(0x594fc8,0);self.put(0x565ec0,0);self.put(0x565ec4,0);self.put(0x54120c,2);self.put(0x5412dc,3)
        self.mu.mem_write(TYPES+500,bytes(500));self.mu.mem_write(TYPES+500+0x18,b'Unit Label\0');self.put(TYPES+500+0x98,3);self.put(TYPES+500+0x120,3)

    def output(self):
        """결과는 원본 노드의 전체 기본/파생 필드·현재 표식과 호출 순서에서만 읽는다."""
        nodes=';'.join(str(token)+':'+bytes(self.mu.mem_read(token,0xd0 if token==WINDOW else 0xa0)).hex() for token in NODES)
        return '|'.join((nodes,str(self.words(0x5ca990,1)[0])+','+str(self.words(0x5ca994,1)[0])+','+str(self.words(0x5ca938,1)[0]),self.string(0x5ca940).hex(),';'.join(self.events)))

    def sample(self,kind,row):
        """실제 전체 함수의 정상 반환 뒤 결과만 읽는다. C++와 GUI/OS는 실행하지 않는다."""
        self.prepare();self.operation=kind
        if kind=='C':
            teach,cached,self.script,self.scale=row;self.scaleValue=struct.unpack('<f',struct.pack('<I',self.scale))[0];self.put(0x5ca994,SOURCE if cached else 0);self.call(0x4c5380,[teach],WINDOW,4)
        elif kind=='L':
            picture,info,buttons,profile=row;self.put(WINDOW+0xa0,PICTURE if picture else 0);self.put(WINDOW+0xa8,INFO if info else 0);self.put(WINDOW+0xb0,CANCEL if buttons&1 else 0);self.put(WINDOW+0xb8,BACK if buttons&2 else 0)
            self.put(WINDOW+0x3c,bits((450,641,320)[profile]));self.put(WINDOW+0x40,bits((350,481,240)[profile]))
            # 합성 배율/버튼 치수도 실제 max/ftol/중앙 배치를 검증하는 독립 입력이다.
            for index,token in enumerate(NODES[1:]):self.put(token+0x38,bits((1,1.25,0.75)[profile]));self.put(token+0x3c,bits(30.75+index*7));self.put(token+0x40,bits(20.75+index))
            self.put(BUFFER+4,int(profile!=2));self.call(0x4c5d60,[bits(13),bits(21),0,0],WINDOW,16)
        elif kind=='H':
            old,void=row;self.put(BODY,0x50bd38);self.put(BODY+0xa0,SCROLL if old else 0);self.put(BODY+4,9 if void else 1);self.put(BODY+0x4c,bits(17));self.put(BODY+0x50,bits(19));self.mu.mem_write(TEXT+0xa000,b'~Nstyled\0');self.allocated[0xd8]=1;self.call(0x46cac0,[TEXT+0xa000],BODY,4)
        else:
            type,topic,frame,old=row;self.put(WINDOW+0xa0,PICTURE if old else 0);self.put(WINDOW+0xa8,INFO if old else 0);self.put(BODY,0x50bd38);self.put(BODY+0xa0,SCROLL if old else 0);self.mu.mem_write(TEXT+0xa000,topic.encode()+b'\0');self.put(TYPES+500+0x120,frame);self.call(0x4c5970,[type,TEXT+0xa000],WINDOW,8)
        self.normal+=1;return self.output()


def main():
    """최초 원문을 보존하거나 현재 SHA만 읽고 기존 기대값을 덮어쓰지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text('utf-8'))
        for name,expected in record['sha256'].items():assert digest(ROOT/name)==expected,name
        print('Quick Help 전체 창 최초 PE 관측 보존 확인');return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('최초 관측 덮어쓰기 금지')
    oracle=Oracle();rows=cases();lines=['# 실제 전체 창/HTML 부모 몸체. 기본/자식 생성·부착/Push·파일/Config·pane는 명시 경계다.']
    # 입력을 먼저 정한 뒤 기대값은 실제 VM 결과에서만 읽는다.
    for kind,row in rows:lines.append('\t'.join((kind,json.dumps(row,separators=(',',':')),oracle.sample(kind,row))))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n');names=['originals/Netstorm.exe','originalCD/NETSTORM.EXE','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_quick_help_window.py','cpppj/tests/fixtures/quick-help-window-x86.tsv']
    record={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-20','inputs':len(rows),'normalReturns':oracle.normal,'instructions':oracle.instructions,'gameExecuted':False,'osExecuted':False,'scope':'실제4c5380/58a0/5970/5d60·46c9c0/ca50/ca70/cac0/cbc0/cc30와 실제 파생/기본 HTML/링크·ftol/CRT. 기본/자식 GUI 생성·부착/Push/텍스트/스크롤/그림·번역/Config/파일/색 조회는 명시 경계다. 원본 게임/OS/장치 관측이 아니다.','sha256':{name:digest(ROOT/name) for name in names}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:record[key] for key in ('inputs','normalReturns','instructions')}))


if __name__=='__main__':main()
