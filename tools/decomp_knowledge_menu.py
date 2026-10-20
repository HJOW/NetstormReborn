#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 지식 창의 필터·정렬·문자열·배치와 popup 표식을 제품 변경 전에 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_factory_menu import Oracle as BaseOracle, ROOT, POOL, TEXT, TYPES, STACK, STOP, digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

GUMPS,VTABLE=POOL+0x20000,POOL+0xe0000  # 기존 게임/타입 입력과 겹치지 않는 명시 GUI 저장소다.
PUSH,POP=STOP+0x500,STOP+0x600  # 장치 없는 가상 Push/Pop의 명시 경계다.
FIXTURE=ROOT/'cpppj/tests/fixtures/knowledge-menu-x86.tsv'  # 기존 Factory 원문과 구별되는 최초 지식 창 관측이다.
REPORT=ROOT/'cpppj/knowledge-menu-native.json'  # 실제 몸체/GUI 조회 경계를 함께 기록한다.
NAMES=('Unused','Wind Cannon','Long-name-here','Sun','Same','same','Wide Name Here','Priest','Temple','Zero Mana','Group10','BadTheme')  # 독립 번역 입력이며 제품 결과를 사용하지 않는다.
MANA=('', '', 'w', 'WW', 'w.r', 'r', 'ww', '', '', '', '', '')  # 원본 strlen이 읽는 독립 원문이다.
FLAGS=(0,0,0x10000,0,0x800,0x800,0x40000,0x200000,0,0,0,0)  # 우선 정렬/사제 제외 조건의 입력이다.


def bits(value):
    """명시 단정도 입력/GUI 필드를 DWORD로 보관한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def inputs():
    """정답 계산 없이 차단·보유/제외·표시 모드·화면/아이콘 치수와 동률을 교차한다."""
    rows=[]
    # 같은 필터 입력에서 내용물/제외와 이름 출력 모드가 달라지는 실제 분기를 선택한다.
    for known,excluded,mode in itertools.product((0,0x155,0xfff),(0,0x10),(0xffffffff,3)):
        rows.append((0,12,known,excluded,mode,640,480,65,60,0))
    # 빈 타입·큰 화면/홀수 치수·동률 이름도 전체 몸체의 정상 반환으로 관측한다.
    for count,view,profile in itertools.product((0,6,12),((1024,768,63,59),(800,600,66,61)),(0,1)):
        rows.append((0,count,0x5a,0x24,0xffffffff,*view,profile))
    rows.append((1,12,0xfff,0xfff,3,640,480,65,60,0));return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """전체492b60/행/CRT정렬·폭/높이·표식은 실제 명령이며 GUI/게임 조회만 명시한다."""
    def __init__(self):
        """기존 격리 PE와 새 몸체의 정확한 범위를 합치고 큰 지역 스택을 그대로 사용한다."""
        super().__init__();ranges(self,(0x492b60,0x49a840,0x475f20,0x475fb0,0x4c5340));self.menuCount=0;self.allReturns=0
        self.put(VTABLE+0x18,PUSH);self.put(VTABLE+0x1c,POP)

    def menuIndex(self,pointer):
        """GUI 호스트 주소를 입력과 무관한 생성 순번으로만 정규화한다."""
        assert GUMPS<=pointer<GUMPS+self.menuCount*0xc00 and (pointer-GUMPS)%0xc00==0
        return (pointer-GUMPS)//0xc00

    def instruction(self,mu,address,size,data):
        """게임/번역/GUI 조회를 명시하고 원본의 분기·정렬·x87 배치 계산을 대신하지 않는다."""
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,8);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==0x446e70:self.events.append('Cancel');self.ret();return
        if address==0x48fc60:self.ret(9);return
        if address==0x4acac0:
            assert ecx==POOL+9*50 and args[0]==4 and args[2]==0;self.ret(int(bool(self.known&(1<<args[1]))),12);return
        if address==0x477bb0:
            assert args[0]==0;pointer=GUMPS+self.menuCount*0xc00;self.mu.mem_write(pointer,bytes(0xc00));self.menuCount+=1
            self.put(pointer,VTABLE);self.put(pointer+4,0x4001);self.put(pointer+0x38,bits(1));self.put(pointer+0x3c,bits(60));self.put(pointer+0x40,bits(20));self.put(pointer+0x44,bits(60));self.put(pointer+0x48,bits(20));self.put(pointer+0x174,bits(52.5))
            self.events.append('V,'+str(self.menuCount-1));self.ret(pointer);return
        if address==0x477cb0:
            parameter='str:themeHelp' if args[2]==0x50f05c else str(args[2]);self.events.append('I,'+self.string(args[0]).hex()+','+str(args[1])+','+parameter+','+','.join(map(str,args[3:6])));self.ret();return
        if address==0x475ca0:
            pointer=GUMPS+(self.menuCount-1)*0xc00;self.put(pointer+0x4c,bits(struct.unpack('<i',struct.pack('<I',args[0]))[0]));self.put(pointer+0x50,bits(struct.unpack('<i',struct.pack('<I',args[1]))[0]))
            self.events.append('A,'+str(self.menuCount-1)+','+','.join(map(str,args[:2])));self.ret();return
        if address in (PUSH,POP):self.events.append(('P,' if address==PUSH else 'Q,')+str(self.menuIndex(ecx)));self.ret();return
        if address==0x464740:
            index=self.menuIndex(ecx);x,y=(struct.unpack('<f',struct.pack('<I',word))[0] for word in self.words(ecx+0x4c,2));self.put(args[0],int(x));self.put(args[0]+4,int(y));self.events.append('B,'+str(index));self.ret(purge=4);return
        super().instruction(mu,address,size,data)

    def sample(self,row):
        """같은 독립 TYPE/조회/GUI 입력에서 전체 출력 순서와 최종 노드를 수집한다."""
        blocked,count,self.known,excluded,mode,width,height,iconWidth,iconHeight,profile=row;self.excluded={index for index in range(12) if excluded&(1<<index)};self.production=set();self.menuCount=0;self.events=[];self.taken=0
        self.put(0x594fc8,blocked);self.put(0x541348,count);self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES);self.put(0x54126c,8);self.put(0x542384,mode);self.put(0x531860,width);self.put(0x531864,height);self.put(0x540cbc,iconWidth);self.put(0x540cc0,iconHeight);self.put(0x563e38,0);self.mu.mem_write(TYPES,bytes(188*500));self.mu.mem_write(0x5ca938,b'\xa5'*88)
        # 원본 필드의 실제 폭으로 입력하고 제외 타입의 데이터도 독립적으로 남긴다.
        for index in range(12):
            self.put(TYPES+index*500+0x98,4 if index==11 else index%4);self.put(TYPES+index*500+0x9c,10 if index in (0,10) else 0);self.put(TYPES+index*500+0xec,FLAGS[index]);self.mu.mem_write(TYPES+index*500+0xa0,MANA[index].encode()+b'\0')
            description=('Same' if index%2 else 'same') if profile else NAMES[index];self.mu.mem_write(TEXT+index*64,description.encode()+b'\0')
        self.invoke(0x492b60);self.allReturns+=1;nodes=[]
        # 실제 몸체가 남긴 표식/치수/목표폭·위치/배경 flags만 순번으로 정규화해 보존한다.
        for index in range(self.menuCount):
            pointer=GUMPS+index*0xc00;nodes.append(','.join(map(str,[self.words(pointer+offset,1)[0] for offset in (4,0x3c,0x40,0x44,0x48,0x4c,0x50,0x174,0x220)])))
        return '|'.join(('~'.join(nodes) or '-',str(self.words(0x5ca938,1)[0]),bytes(self.mu.mem_read(0x5ca940,80)).hex(),';'.join(self.events) or '-'))


def main():
    """첫 독립 관측을 동결하거나 보존 해시만 감사하며 게임/OS/제품 검사는 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text('utf-8'))
        # 최초 관측의 전체 입력 바이트를 확인한다.
        for name,expected in record['sha256'].items():assert digest(ROOT/name)==expected,name
        print('지식 창 최초 PE/정렬/문자열/배치 관측 보존 확인');return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('최초 관측 덮어쓰기 금지')
    oracle=Oracle();rows=inputs();lines=['# 전체 지식 창이며 게임/GUI 조회는 명시 경계, 실제 행/정렬/폭/높이/배치/표식을 관측한다.']
    # 기대값은 제품 코드가 생기기 전에 실제 PE의 정상 반환과 쓰기에서만 수집한다.
    for row in rows:lines.append(','.join(map(str,row))+'\t'+oracle.sample(row))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=['originals/Netstorm.exe','originalCD/NETSTORM.EXE','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_knowledge_menu.py','cpppj/tests/fixtures/knowledge-menu-x86.tsv']
    record={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-20','inputs':len(rows),'normalReturns':oracle.allReturns,'instructions':oracle.instructions,'scope':'실제492b60/492960/453f70/CRT정렬/ftol/475f20/475fb0/4c5340. TYPE/내용물/제외/번역/서식/GUI생성·부착/가상Push·Pop/위치·스레드는 명시 경계다. 원본 게임/OS/실제 장치 관측은 아니다.','gameExecuted':False,'osExecuted':False,'sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:record[key] for key in ('inputs','normalReturns','instructions')}))


if __name__=='__main__':
    main()
