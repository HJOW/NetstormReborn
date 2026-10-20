#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""도움말 창 재사용·이력·실제 입력 분배를 제품 변경 전에 격리 PE로 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_mission_abort import Oracle as BaseOracle, ROOT, POOL, TEXT, STACK, STOP, digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

NODE,ALT,FOREIGN,VT,BODY,BUFFER,BUTTONS=POOL+0x88000,POOL+0x89000,POOL+0x8a000,POOL+0x8b000,POOL+0x8c000,POOL+0x8d000,POOL+0x8e000  # 서로 겹치지 않는 GUI 입력이다.
TOPIC,OTHER,HEAP=TEXT+0x6000,TEXT+0x7000,POOL+0xc0000  # 원문과 이력 할당의 격리 저장소다.
ATTACH,HANDLE=STOP+0x700,STOP+0x800  # 실제 가상 부착/입력의 명시 경계다.
FIXTURE=ROOT/'cpppj/tests/fixtures/quick-help-control-x86.tsv'  # 기존 요청985행과 구별한 최초 관측이다.
REPORT=ROOT/'cpppj/quick-help-control-native.json'  # 전체 몸체와 외부 창/장치 경계를 구별한다.


def bits(value):
    """단정도 입력을 원본 DWORD 비트로 고정한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def cases():
    """창 재사용·다른 모달·TYPE/주제·스크롤·Back/버튼·현재 값 변경을 독립 공급한다."""
    rows=[]
    # 창 유무와 현재 모달을 교차하며0/1 요청 모드도 구별한다.
    for existing,active,mode in itertools.product((0,1),range(4),(0,1)):rows.append(('O',(existing,active,mode)))
    # 연속 중복만 생략하는 이력과 마지막 노드까지의 pop을 관측한다.
    for names,pops in itertools.product(('Alpha','Alpha;alpha;Beta;ALPHA','themeHelp;sunCannonType;themeHelp','Alpha\0ignored;ALPHA;Beta',''),(0,1,2,5)):rows.append(('H',(names,pops)))
    # 열은 사건/첫인자/둘째인자/로컬x,y/페이지높이/이력개수/이전팝업/삭제후변경이다.
    for code,parameter in itertools.product((0x2000017,0x2000057,0x82000017),(0,1,71,255,256,OTHER,0xffffffff)):
        rows.append(('E',(code,TOPIC,parameter,bits(1),bits(1),bits(51.75),2,0x20000a0,0)))
    for code,height in itertools.product((0x210000,0x220000,0x260000,0x280000,0x80260000,0x20000),(bits(51.75),0x4f800000,0x7fc01234)):
        rows.append(('E',(code,0,0,bits(1),bits(1),height,0,0,0)))
    for x,y,release in itertools.product((bits(-1),bits(0),bits(449),bits(450),0x7fc01234),(bits(0),bits(350)),(0,0x80000000)):
        rows.append(('E',(0x10000|release,0,0,x,y,bits(51.75),0,0,0)))
    for first,count,previous,mutation in itertools.product((81,82,83,99),(0,1,2),(0,0x20000a0),(0,1)):
        rows.append(('E',(0x200002c,first,0,bits(1),bits(1),bits(51.75),count,previous,mutation)))
    rows.extend(('E',(code,TOPIC,OTHER,bits(1),bits(1),bits(51.75),0,0,0)) for code in (0x2000025,0x200008b))
    rows.append(('Z',(0x200002c,82,0,bits(1),bits(1),bits(51.75),0,0,0)))  # 학습 TYPE0도 창을 닫는다.
    rows.extend(('P',(present,text)) for present,text in ((0,''),(1,''),(1,'short'),(1,'Alpha\0ignored')))  # 마지막 표식의 null/빈문자열과 잔여 바이트다.
    return rows


class Oracle(BaseOracle):
    """실제 열기/이력 생성·삭제·입력·CRT/정수 변환만 실행하고 GUI/연결은 명시한다."""
    def __init__(self):
        """현재 정밀 함수 범위를 격리 로더에 더하며 실제 게임은 시작하지 않는다."""
        super().__init__();self.mu.mem_map(0,0x1000);ranges(self,(0x4c62a0,0x4c5260,0x4c52c0,0x4c5340,0x4c6d00,0x4c6e00,0x4c70a7,0x4c8e90,0x4652b0,0x4e49c0));self.heap=0;self.returnCount=0
        self.put(VT+0x20,ATTACH);self.put(VT+0x28,HANDLE)

    def guard(self,mu,access,address,size,value,data):
        """원본 쓰기는 이력/팝업 전역·SEH 및 기존 격리 노드/스택만 허용한다."""
        if address==0 and size==4:return
        if 0x5ca934<=address and address+size<=0x5ca9b8:return
        super().guard(mu,access,address,size,value,data)

    def instruction(self,mu,address,size,data):
        """실제 분배/이력은 계속 실행하고 창 생성·모달·스크롤·요청/링크·예약만 관측한다."""
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,6);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==0x4e4391:
            pointer=NODE if self.operation=='O' else HEAP+self.heap*128;self.heap+=1;self.ret(pointer);return
        if address==0x4e4354:self.ret();return
        if address==0x466710:self.events.append('ClosePopups');self.ret();return
        if address==0x464020:self.ret(NODE if args[0]==13 and self.existing else ALT if args[0]==14 else 0);return
        if address==0x4c5380:self.events.append('Construct,'+str(args[0]));self.ret(NODE,4);return
        if address==ATTACH:self.events.append('Attach,'+','.join(map(str,args[:4])));self.ret(purge=16);return
        if address==HANDLE:self.events.append('Handle,'+','.join(map(str,args[:2]))+','+self.string(args[2]).hex());self.ret(1,12);return
        if address==0x466600:self.events.append('Modal,'+str(args[0]));self.put(0x565dd4,NODE);self.ret(purge=4);return
        if address==0x4650a0:self.put(args[0],self.row[3]);self.put(args[0]+4,self.row[4]);self.events.append('Local');self.ret(purge=16);return
        if address==0x4c5c00:self.events.append('Request,'+str(args[0])+','+self.string(args[1]).hex());self.ret(purge=8);return
        if address==0x49a840:self.mu.mem_write(OTHER,('Unit'+str(args[0])).encode()+b'\0');self.ret(OTHER-4);return
        if address==0x4de9a0:self.ret(args[0],4);return
        if address==0x4ba380:self.events.append('Scroll,'+str(args[0]));self.ret(purge=4);return
        if address==0x466480:
            self.events.append('Kill')
            if self.operation=='E' and self.row[8]:self.mu.mem_write(0x5ca940,b'changed\0');self.put(0x5ca938,0x2000abc)
            self.ret();return
        if address==0x4a00c0:self.events.append('Link,'+self.string(args[0]).hex());self.ret();return
        if address==0x452f90:self.events.append('Queue,'+str(args[0])+','+self.string(args[1]).hex()+','+self.string(args[2]).hex());self.ret();return
        if address==0x492a30:self.events.append('Learn,'+str(args[0]));self.ret();return
        if address==0x464990:self.events.append('Base');self.ret(7,12);return
        super().instruction(mu,address,size,data)

    def history(self):
        """실제 연결 이력의 현재 문자열을 머리부터 읽는다. 기대값을 계산하지 않는다."""
        values=[];pointer=self.words(0x5ca934,1)[0]
        # 실제 next를 따라가며 잘못된 순환은 거부한다.
        while pointer:
            assert pointer not in values;values.append(pointer);pointer=self.words(pointer+4,1)[0]
        return '/'.join('s'+self.string(self.words(pointer,1)[0]).hex() for pointer in values) or '-'

    def prepare(self):
        """이전 입력과 독립인 실제 노드/전역·문자열을 공급한다."""
        self.operation='X';self.events=[];self.heap=0;self.put(0x5ca934,0);self.mu.mem_write(NODE,bytes(0xd0));self.put(NODE,VT);self.put(NODE+0x30,120);self.put(NODE+0x3c,bits(450));self.put(NODE+0x44,bits(450));self.put(NODE+0x48,bits(350));self.put(NODE+0xac,BODY);self.put(BODY+0xa0,BUFFER);self.put(NODE+0xcc,71)
        # 세 버튼은 실제 Gump 등록 ID로 비교하므로 다른 ID도 함께 남긴다.
        for index in range(3):self.put(NODE+0xb0+index*4,BUTTONS+index*0x100);self.put(BUTTONS+index*0x100+0x2c,81+index)
        self.mu.mem_write(TOPIC,b'themeHelp\0');self.mu.mem_write(OTHER,b'otherHelp\0');self.mu.mem_write(256,b'lowHelp\0');self.put(0x5ca990,13);self.put(0x5cad28,14);self.put(0x5ca99c,16);self.put(0x5ca9b4,800);self.put(0x563e38,0);self.put(0x5ca938,0);self.mu.mem_write(0x5ca940,b'before\0'+bytes(73));self.put(0x565dd4,0)

    def sample(self,case):
        """각 실제 함수가 정상 반환한 후 현재 이력/순서/필드를 보존한다."""
        self.prepare();kind,row=case
        if kind=='H':
            names,pops=row
            # 중간 중복/첫NUL과 pop 횟수는 입력이며 결과 목록은 native에서 읽는다.
            for name in names.split(';'):
                self.mu.mem_write(TOPIC,name.encode()+b'\0');self.call(0x4c52c0,[TOPIC])
            results=[]
            for index in range(pops):
                pointer=self.call(0x4c6d00,[]);results.append('s'+self.string(pointer).hex() if pointer else 'n')
            output=self.history()+'|'+','.join(results)
        elif kind=='P':
            present,topic=row;self.mu.mem_write(TOPIC,topic.encode()+b'\0');self.call(0x4c5340,[0x20000a0,TOPIC if present else 0]);output=str(self.words(0x5ca938,1)[0])+'|'+bytes(self.mu.mem_read(0x5ca940,80)).hex()
        elif kind=='O':
            self.operation='O';self.existing,active,mode=row;self.put(0x565dd4,(0,NODE,ALT,FOREIGN)[active]);result=self.call(0x4c62a0,[TOPIC,mode,71]);output=str(int(result==NODE))+'|'+str(self.words(NODE+0x30,1)[0])+'|'+(';'.join(self.events) or '-')
        else:
            self.row=row;code,first,parameter,x,y,height,count,previous,mutation=row
            if kind=='Z':self.put(NODE+0xcc,0)
            # 실제 이력 몸체로 Back의 현재 스택을 준비한다.
            for name in ('firstHelp','secondHelp')[:count]:self.mu.mem_write(OTHER,name.encode()+b'\0');self.call(0x4c52c0,[OTHER])
            self.mu.mem_write(OTHER,b'otherHelp\0');self.events=[];self.operation='E';self.put(BUFFER+0x40,height);self.put(0x5ca938,previous);result=self.call(0x4c6e00,[code,first,parameter],NODE,12);output=str(result)+'|'+self.history()+'|'+(';'.join(self.events) or '-')
        self.returnCount+=1;return output


def main():
    """ 최초 독립 원문을 저장하거나 현재 보호 해시만 확인한다. 게임/OS/C++는 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text('utf-8'))
        # 첫 관측의 모든 입력 바이트를 그대로 검사한다.
        for name,expected in record['sha256'].items():assert digest(ROOT/name)==expected,name
        print('Quick Help 제어/이력 최초 PE 관측 보존 확인');return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('최초 관측 덮어쓰기 금지')
    oracle=Oracle();rows=cases();lines=['# 실제 Help 재사용/이력/입력이며 전용 창·게임/장치 효과는 명시 경계다.']
    # 이력 문자열은 hex로 기록하고 나머지 원본 DWORD는 비트대로 보존한다.
    for kind,row in rows:
        encoded=(row[0].encode().hex()+','+str(row[1])) if kind=='H' else (str(row[0])+','+row[1].encode().hex()) if kind=='P' else ','.join(map(str,row));lines.append('\t'.join((kind,encoded,oracle.sample((kind,row)))))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n');paths=['originals/Netstorm.exe','originalCD/NETSTORM.EXE','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_quick_help_control.py','cpppj/tests/fixtures/quick-help-control-x86.tsv']
    record={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-20','inputs':len(rows),'normalReturns':oracle.returnCount,'instructions':oracle.instructions,'scope':'실제4c62a0/4c5260/4c52c0/4c6d00/4c6e00/4c8e90/4652b0·CRT. 생성/등록조회·모달/좌표/스크롤/요청/링크/예약/학습/기본 입력·할당/반납은 명시 경계다. 전용 본문/창·게임/OS 관측이 아니다.','gameExecuted':False,'osExecuted':False,'sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:record[key] for key in ('inputs','normalReturns','instructions')}))


if __name__=='__main__':main()
