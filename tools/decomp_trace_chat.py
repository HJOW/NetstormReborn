#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문 진단→Chat 부모의 스타일·현재 자식·스크롤·부분효과를 격리 x86에서 관측한다."""
import argparse
import csv
import ctypes
import hashlib
import json
import socket
import struct
from pathlib import Path
from decomp_local_mission_transport import Oracle as Base, ROOT, TEXT, STACK, STOP
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

CHAT, FIRST, SECOND, STYLE = TEXT+0x8000, TEXT+0x9000, TEXT+0xa000, TEXT+0xb000  # 원본 주소와 구별한 격리 노드다.
FIXTURE=ROOT/'cpppj/tests/fixtures/trace-chat-x86.tsv'  # 제품 예상값으로 작성하지 않는 새 원시 관측이다.
REPORT=ROOT/'cpppj/trace-chat-native-20261029.json'  # 최초 원시/PE/도구 해시와 명시 경계를 기록한다.


def cases():
    """분기 결과가 아니라 관문/NULL/문맥 alias/콜백 변경·중단 입력을 선택한다."""
    rows=[]
    for style in ('none','supplied','root','child'):  # NULL과 세 실제 스타일 주소를 구별한다.
        for bottom in (0,1,0xffffffff): rows.append(('append',1,9,1,style,bottom,'none',b'~rLog\n'))
    for mutation in ('at-child','parse-child','parse-context','bottom-context','stop-at','stop-parse','stop-bottom'):
        rows.append(('append',1,9,1,'supplied',1,mutation,b'A\0tail'))  # 현재 포인터/스타일의 재조회 위치를 드러낸다.
    for enabled in (0,1,0x80000000):
        for current in (0,1):
            for text in (b'normal\n',b'100%%\n',b'\xe9\x00ignored'):
                rows.append(('trace',enabled,0x800000d7,current,'none',1,'none',text))
    return rows


class Oracle(Base):
    """분기/문맥 memcpy/포인터 재읽기는 원문으로, formatter/UI는 명시 장치로 실행한다."""
    def __init__(self):
        """같은 PC 정밀 함수표의 실제 세 함수 범위만 실행 허용한다."""
        super().__init__();self.active=False;self.crt=ctypes.CDLL('msvcrt')
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        self.allowed=[]
        for address in (0x434f40,0x42f190,0x42eca0):
            for part in facts[address]['ranges'].split(';'):  # 범위 사이 빈틈이나 임의 하위를 허용하지 않는다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def context(self,address):
        """대상 노드의 현재32BYTE를 그대로 기록한다."""
        return bytes(self.mu.mem_read(address+0x80,32)).hex()

    def read(self,mu,access,address,size,value,data):
        """진단 관문 뒤의 실제 색/Chat 포인터 읽기 위치를 기록한다."""
        if self.active and address in (0x5acd24,0x548740): self.events.append('color' if address==0x5acd24 else 'current')

    def token(self,address):
        """격리 노드 주소를 검사에서도 쓰는 의미 토큰으로 표시한다."""
        return '1' if address==FIRST else '2' if address==SECOND else (_ for _ in ()).throw(RuntimeError('알 수 없는 Chat 자식'))

    def instruction(self,mu,address,size,data):
        """현재 콜백의 실제 인자/문맥을 기록하고 명시한 한 효과만 변경한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,3);target=mu.reg_read(UC_X86_REG_ECX)
        if address==0x4de490:
            #434f40은 추가인자를 주지 않는다. 정의된 일반문자/%%만 실제 호스트 CRT 장치로 공급한다.
            format_bytes=self.string(args[1]);buffer=ctypes.create_string_buffer(1024)
            self.crt.sprintf(buffer,ctypes.c_char_p(format_bytes));mu.mem_write(args[0],buffer.value+b'\0');self.ret();return
        if address in (0x4ba400,0x4ba8f0,0x4ba3b0):
            kind={0x4ba400:'at',0x4ba8f0:'parse',0x4ba3b0:'bottom'}[address]
            self.events.append(kind+':'+self.token(target)+':'+self.context(CHAT)+':'+self.context(target)+((':'+self.string(args[0]).hex()) if kind=='parse' else ''))
            if self.mutation=='stop-'+kind: self.status='stop';mu.emu_stop();return
            if self.mutation==kind+'-child': self.put(CHAT+0xa4,SECOND)
            if self.mutation==kind+'-context':
                mu.mem_write(CHAT+0x80,struct.pack('<8I',*range(700,708)));mu.mem_write(FIRST+0x80,struct.pack('<8I',*range(800,808)))
            self.ret(self.bottom if kind=='at' else 0,purge=4 if kind=='parse' else 0);return
        if not any(low<=address<high for low,high in self.allowed): raise RuntimeError(f'Chat 실행 범위 밖 {address:08x}')

    def sample(self,case):
        """매 관측마다 노드의 서로 다른 문맥과 스택/입력을 초기화한다."""
        action,enabled,color,current,style,bottom,mutation,text=case
        self.events=[];self.status='normal';self.bottom=bottom;self.mutation=mutation
        for address,first in ((CHAT,100),(FIRST,200),(SECOND,300),(STYLE-0x80,400)):
            self.mu.mem_write(address,b'\xa5'*0x13c);self.mu.mem_write(address+0x80,struct.pack('<8I',*range(first,first+8)))  # 부모/두 자식/입력32BYTE가 다르다.
        self.put(CHAT+0xa4,FIRST);self.put(0x531854,enabled);self.put(0x5acd24,color);self.put(0x548740,CHAT if current else 0)
        self.mu.mem_write(TEXT,text+b'\0')
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP): self.mu.reg_write(register,0)  # 앞선 실행 레지스터를 재사용하지 않는다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.put(STACK,STOP);self.put(STACK+4,TEXT)
        if action=='append':
            self.mu.reg_write(UC_X86_REG_ECX,CHAT);self.put(STACK+8,{'none':0,'supplied':STYLE,'root':CHAT+0x80,'child':FIRST+0x80}[style])
        self.active=True;self.mu.emu_start(0x434f40 if action=='trace' else 0x42eca0,STOP,count=100000);self.active=False
        if self.status=='normal' and (self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+(4 if action=='trace' else 12)): raise RuntimeError('Chat 정상 ret/스택 불일치')
        return '\t'.join((action,str(enabled),str(color),str(current),style,str(bottom),mutation,text.hex(),self.status,self.context(CHAT),self.context(FIRST),self.context(SECOND),self.token(self.words(CHAT+0xa4)[0]),';'.join(self.events) or '-'))


def main():
    """최초 파일을 보존하고 verify는 CPU/CRT 관측 전체를 다시 대조한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify and (FIXTURE.exists() or REPORT.exists()): raise RuntimeError('최초 Chat 관측 덮어쓰기 금지')
    oracle=Oracle();rows=[oracle.sample(case) for case in cases()]
    content='# action / enabled / color / current / style / bottom / mutation / input hex / status / root / first / second context / final child / trace\n'+'\n'.join(rows)+'\n'
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_local_mission_transport.py','tools/decomp_trace_chat.py')
    hashes={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in paths}
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        if hashes!=report['sha256'] or content!=FIXTURE.read_text(encoding='utf-8'): raise RuntimeError('Chat 독립 재관측 차이')
    else:
        FIXTURE.write_text(content,encoding='utf-8',newline='\n')
        report={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','observations':len(rows),'sha256':hashes,'fixtureSha256':hashlib.sha256(content.encode()).hexdigest(),'scope':'434f40/42f190/42eca0 전체 원문. formatter는 정의된 인자 없는 서식의 호스트 CRT 장치이며 UI atBottom/parse/requestBottom은 명시 관측/변경/중단 장치다. 원본 게임/GUI/네트워크 실행 없음.'}
        REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'observations':len(rows),'verified':args.verify,'differences':0}))


if __name__=='__main__':
    main()
