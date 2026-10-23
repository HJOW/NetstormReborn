#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 cf570의 실제 문자열 조립/serial 재사용 부모만 격리 관측한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, TEXT, STACK, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG,UC_X86_REG_EFLAGS)

# 실제 포인터 대신 쓸 격리 입력/임시/번역/두 HTML 부모 주소다. 원본 이미지와 겹치지 않는다.
SOURCE,BUFFER,CANCEL=TEXT,TEXT+0x1000,TEXT+0x3000
FIRST,SECOND=0x12200000,0x12204000
TRANSIENT,CREATED=0x5cab34,0x5cab38
FIXTURE=ROOT/'cpppj/tests/fixtures/mission-loading-message-x86.tsv'  # 제품을 읽지 않는 독립 관측이다.
REPORT=ROOT/'extracted/cpp-mission-loading-message-20261023-native.json'  # 실제 실행 범위·해시·한계를 보존한다.


def inputs():
    """기대 본문 없이 LF/대소문자/버튼/스타일 prefix와 현재 부모 선택을 교차한다."""
    messages=(b'',b'Starting Mission...',b'Starting Mission...\n',b'Body\r\n',b'Body\r',b'~.Body',b' ~.Body',
        b'Body$Button=OK,DoNothing,0',b'Body$ButtonInstant=OK,DoNothing,0',b'Body$button=lower',
        b'Body$Buttonish',b'Body\n\n',b'Body\0ignored',b'<b>Body</b>',b'\x80\xff',b'%d already formatted')
    values=list(itertools.product(messages,(b'Cancel',b'Annuler',b'A,B$C'),range(5),(0,1,0xffffffff),(0,1),(0,)))
    values.extend(itertools.product((b'Starting Mission...',),(b'Cancel',b''),(0,1,4),(0,0xffffffff),(0,1),(1,2,3,4)))
    values.extend((message,b'Cancel',profile,0,0,0) for message in (b'~.'+b'A'*1020+b'\n',b'$Button'+b'A'*1015+b'\n') for profile in (0,1))
    return values


class Oracle(MemoryOracle):
    """전체 cf570/string CRT는 실제 명령이며 할당/확장printf/번역/HTML 하위는 명시 경계다."""
    def __init__(self):
        """원문cf570과strstr 범위만 기존 보호PE 위에 허용한다."""
        super().__init__();self.allowed=[];self.trace=[];self.normal=0;self.buffer_calls=0;self.format_calls=0
        facts={int(r['entry'],16):r for r in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        for entry in (0x4cf570,0x4e6770):
            for part in facts[entry]['ranges'].split(';'):  # 실제 불연속 문자열 함수 범위도 그대로 허용한다.
                low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))

    def hex(self,address):
        """관측 C문자열의 첫NUL 앞 바이트만 보존한다."""
        return self.string(address).hex() or '-'

    def mutate(self,token,where):
        """메타/본문 콜백이 노드의 현재 depth/serial 또는 전역을 바꾸는 명시 입력이다."""
        mutation=self.case[5]
        if mutation==1 and where=='M':self.put(token+0x21bc,100);self.put(token+0x2c,99)
        elif mutation==2 and where=='B':self.put(token+0x21bc,0);self.put(token+0x2c,77)
        elif mutation==3 and where=='M':self.put(TRANSIENT,888);self.put(CREATED,666)
        elif mutation==4 and where=='M':self.mu.mem_write(BUFFER,b'AfterMetadata\n\0')

    def instruction(self,mu,address,size,data):
        """실제 부모가 호출하는 외부 경계의 입력과 콜백 후 현재 필드를 기록한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,4);token=mu.reg_read(UC_X86_REG_ECX)
        if address==0x4b9320:
            assert args[0]==1024;self.buffer_calls+=1;self.ret(BUFFER);return
        if address==0x4de490:
            assert args[0]==BUFFER;self.format_calls+=1;mu.mem_write(BUFFER,self.string(args[1])+b'\0');self.ret();return
        if address==0x4de9a0:
            assert self.string(args[0])==b'Cancel';self.trace.append('T,Cancel');self.ret(CANCEL,4);return
        if address==0x464020:
            profile=self.case[2];serial=args[0]
            found=FIRST if profile==0 and serial==17 else SECOND if profile in (1,4) and serial==(41 if profile==1 else 57) else 0
            self.trace.append('L,'+str(serial)+','+str(found));self.ret(found);return
        if address in (0x4c83b0,0x4c7aa0):
            kind='M' if address==0x4c83b0 else 'B'
            self.trace.append(kind+','+str(token)+','+str(self.words(token+0x21bc,1)[0])+','+self.hex(args[0]));self.mutate(token,kind);self.ret(1,4);return
        if address==0x4cf270:
            assert tuple(args[1:])==(0,400,30)
            self.trace.append('C,0,400,30,'+self.hex(args[0]));profile=self.case[2]
            self.put(CREATED,41 if profile==1 else 57 if profile==4 else 0);self.ret(0 if profile==2 else 41);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'미등록 진행 안내 명령 {address:08x}')
        self.instructions+=1

    def sample(self,case,precision):
        """외부 포맷 경계 뒤 같은 원문 바이트와 현재 serial 표를 공급하고 전체cf570 반환을 확인한다."""
        self.case=case;self.trace=[];message,cancel,profile,depth,alias,_=case
        self.mu.mem_write(SOURCE,message+b'\0');self.mu.mem_write(BUFFER-1,b'\xd5'+bytes(0x1001));self.put(BUFFER+1024,0xd5,1);self.mu.mem_write(CANCEL,cancel+b'\0')
        if alias:self.mu.mem_write(BUFFER,message+b'\0')
        self.put(TRANSIENT,17);self.put(CREATED,73)
        for node,serial in ((FIRST,17),(SECOND,41 if profile==1 else 57)):  # 실제 외부 조회가 반환할 노드의 명시 초기 필드다.
            self.mu.mem_write(node,bytes(0x21e8));self.put(node+0x2c,serial);self.put(node+0x21bc,depth)
        saved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x3456789a,UC_X86_REG_EBP:0x456789ab}
        for register,value in saved.items():self.mu.reg_write(register,value)
        for register,value in ((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,0),(UC_X86_REG_EDX,0),(UC_X86_REG_ESP,STACK),
            (UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,precision),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):
            self.mu.reg_write(register,value)  # ABI/x87 상태는 매 관측 새로 넣는다.
        self.mu.mem_write(STACK,struct.pack('<III',STOP,BUFFER if alias else SOURCE,0))
        self.mu.emu_start(0x4cf570,STOP,count=200000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        assert all(self.mu.reg_read(register)==value for register,value in saved.items())
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision and not self.mu.reg_read(UC_X86_REG_FPSW)&0x3800
        assert len(self.string(BUFFER))<1024 and self.mu.mem_read(BUFFER+1024,1)==b'\xd5'
        self.normal+=1
        return (self.hex(BUFFER),str(self.mu.reg_read(UC_X86_REG_EAX)),str(self.words(TRANSIENT,1)[0]),str(self.words(CREATED,1)[0]),
            str(self.words(FIRST+0x21bc,1)[0]),str(self.words(FIRST+0x2c,1)[0]),str(self.words(SECOND+0x21bc,1)[0]),
            str(self.words(SECOND+0x2c,1)[0]),';'.join(self.trace))


def main():
    """제품과 독립인 새 fixture를 생성하거나 기존 관측 해시만 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,wanted in report['sha256'].items():assert digest(ROOT/name)==wanted,name
        assert report['rows']==len(inputs());print('진행 안내 원문 해시 확인',report['rows']);return
    oracle,rows=Oracle(),[]
    for case in inputs():  # 실제 두 정밀도의 본문/필드/호출 순서를 비교한 뒤 원문을 동결한다.
        first=oracle.sample(case,0x27f);assert first==oracle.sample(case,0x37f),case
        message,cancel,profile,depth,alias,mutation=case
        rows.append((message.hex() or '-',cancel.hex() or '-',str(profile),str(depth),str(alias),str(mutation),*first))
    FIXTURE.write_text('# source cancel profile depth alias mutation body return transient created firstDepth firstSerial secondDepth secondSerial effects\n'+
        '\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv',
        'tools/decomp_priestmenu.py','tools/decomp_mission_loading_message.py',FIXTURE.relative_to(ROOT).as_posix())
    report=dict(host=socket.gethostname(),lastDecompileHost='VM-W11-Codex',lastDecompileDate='2026-10-23',rows=len(rows),
        normalReturns=oracle.normal,nativeInstructions=oracle.instructions,bufferCalls=oracle.buffer_calls,formatCalls=oracle.format_calls,
        precisionDifferences=0,sha256={name:digest(ROOT/name) for name in paths},limits=[
            'cf570전체/strstr실제명령을실행한다. b9320임시버퍼/4de490확장printf결과/Cancel번역/64020검색/cf270생성/c83b0메타/c7aa0본문은명시외부경계다.',
            '제품입력은이미printf된C문자열이다. alias입력은원문의format생략도검증하지만제품에확장printf/임시고리를새로복원한것은아니다.',
            '1024BYTE정상임시버퍼내결과만실행하며원본버퍼밖쓰기/실제HTML생성/Fit/화면/GDI는증명하지않는다.',
            '보호파일/기존제품/CMake수정과게임/GUI/빌드/커밋은없다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('진행 안내 실제 명령 관측',len(rows),oracle.normal,oracle.instructions)


if __name__=='__main__':main()
