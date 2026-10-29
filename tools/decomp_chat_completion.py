#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""채팅 이름 자동완성의 실제 원문/strncpy/ASCII 비교와 연속 상태를 관측한다."""
import csv
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,TEXT,STACK,STOP,digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP

STATE,SIZE,RECORDS=0x548810,0x4090,0x558ed0  # 실제 인덱스/길이/접두/개수/128후보와100×48BYTE 원본 저장소다.
FIXTURE=ROOT/'cpppj/tests/fixtures/chat-completion-x86.tsv'  # 최초 원문 상태 관측이며 재실행은 덮어쓰지 않는다.
SCHEMA=('name','reset','records','mode','query','length','backward','result','state','trace','instructions')  # 앞7열은 명시 입력이다.


def encoded(data):
    """빈 문자열과 NULL을 구별하는 원시 BYTE 표현이다."""
    return data.hex() or '-'


class Oracle(MemoryOracle):
    """후보 수집/순환/복사와 실제 CRT 몸체를 CPU로 실행한다."""
    def __init__(self):
        """허용한 세 몸체와 명시 번역/진단 경계만 준비한다."""
        super().__init__();self.allowed=[]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as file:
            for row in csv.DictReader(file,delimiter='\t'):  # C locale 비교와 strncpy까지 실제 원문이다.
                if int(row['entry'],16) in (0x42e540,0x4e51f0,0x4ef5a0):self.allowed.extend((int(a,16),int(b,16)+1) for a,b in (part.split('-') for part in row['ranges'].split(';')))
        self.original={a:bytes(self.mu.mem_read(a,b-a)) for a,b in self.allowed};self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self,mu,access,address,size,value,data):
        """CPU는 실제 전역 상태와 격리 스택에만 쓸 수 있다."""
        if STATE<=address and address+size<=STATE+SIZE:return
        if STACK-0x1000<=address and address+size<=STACK+32:return
        raise RuntimeError(f'미명시 자동완성 쓰기 {address:08x}/{size}')

    def instruction(self,mu,address,size,data):
        """번역 장치와 C locale 선택 외 계산은 원문만 실행한다."""
        if address==STOP:mu.emu_stop();return
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,3)
        if address==0x4de9a0:
            key=self.string(args[0]);value=key
            if self.mode==1:value={b'BATTLE':b'Area',b'EVERYONE':b'Tous',b'ALLIES':b'Friends',b'OBSERVER':b'Eyes'}[key]
            elif self.mode==2:value=b'dup'
            elif self.mode==3 and key==b'ALLIES':value=b''
            elif self.mode in (4,5) and key==b'BATTLE':value=b'Z'*(127 if self.mode==4 else 128)
            self.trace.append('T:'+key.hex());mu.mem_write(TEXT+0x4000,value+b'\0');self.ret(TEXT+0x4000,4);return
        if address==0x4e66ed:mu.reg_write(UC_X86_REG_EIP,0x4ef5a0);return  #TLS/locale 생성 대신 실제 C locale ASCII 몸체로 진입한다.
        if address==0x4e0620:self.trace.append('A:'+str(args[2]));self.ret(0);return
        if not any(a<=address<b for a,b in self.allowed):raise RuntimeError(f'미명시 자동완성 실행 {address:08x}')
        self.instructions+=1

    def run(self,row):
        """새 query와 NULL 순환 사이에 전역 상태를 그대로 보존한다."""
        if row['reset']:self.mu.mem_write(STATE,bytes(SIZE))
        records=bytes.fromhex(row['records']) if row['records']!='-' else b'';self.mu.mem_write(RECORDS,records+bytes(4800-len(records)))
        self.mode=row['mode'];self.trace=[];self.instructions=0;query=0
        if row['query']!='~':
            source=bytes.fromhex(row['query']) if row['query']!='-' else b'';query=TEXT;self.mu.mem_write(query,source+b'\0')
        self.mu.mem_write(STACK-0x1000,bytes(0x1020));self.mu.mem_write(STACK,struct.pack('<4I',STOP,query,row['length'],row['backward']));self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.emu_start(0x42e540,STOP,count=1000000);assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        pointer=self.mu.reg_read(UC_X86_REG_EAX);result='~' if not pointer else encoded(self.string(pointer))
        for address,expected in self.original.items():assert bytes(self.mu.mem_read(address,len(expected)))==expected  #자료 쓰기가 원문을 바꾸지 않았다.
        return dict(result=result,state=encoded(bytes(self.mu.mem_read(STATE,SIZE)).rstrip(b'\0')),trace=';'.join(self.trace) or '-',instructions=self.instructions)


def records(kind):
    """이름·Subscriber를 실제4800BYTE 입력으로 보존한다."""
    output=bytearray(4800)
    if kind=='many':
        for index in range(100):  #전체 물리 행의 마지막 슬롯까지 후보 순서를 확인한다.
            name=f'user{index:03}'.encode();output[index*48:index*48+len(name)]=name;struct.pack_into('<I',output,index*48+44,1)
    elif kind=='sparse':
        for index,name,mode in ((0,b'Alice',1),(1,b'Ghost',0),(3,b'Alfred',2),(5,b'Bob',1),(40,b'',1),(70,b'aLICE',0xffffffff),(99,b'alpha',1)):
            output[index*48:index*48+len(name)]=name;struct.pack_into('<I',output,index*48+44,mode)  #빈이름도비영Subscriber면후보가된다.
    elif kind=='boundary':
        output[0:129]=b'X'*128+b'\0'  #첫 행 이름128은 제외되고 다음 행의80BYTE 이름은 그대로 포함된다.
    elif kind=='forty':
        output[0:40]=b'A'*40;struct.pack_into('<I',output,44,1)  #이름40BYTE와 바로 뒤 주소의NUL을 구별한다.
    return encoded(output.rstrip(b'\0'))


def cases():
    """상태 초기화와 여러 NULL 순환·번역·현재 기록 변경의 입력만 만든다."""
    rows=[]
    for kind in ('empty','sparse','many','boundary','forty'):  #모든 종류에서 새 query/정방향/역방향 감김을 순서대로 실행한다.
        for query,length in ((b'',0),(b'a',1),(b'AL',2),(b'Unknown',7),(b'X'*127,127)):
            sequence=((None,0,0),(query,length,0),(None,0,0),(None,0,1),(None,0,1),(None,0,0))
            for step,(word,size,backward) in enumerate(sequence):
                rows.append(dict(name=f'{kind}-{length}-{step}',reset=int(step==0),records=records(kind if step<3 else 'empty'),mode=0,query='~' if word is None else encoded(word),length=size,backward=backward))  #NULL순환은기록변경후에도기존후보를쓴다.
    for mode in range(1,6):  #별칭·중복·번역빈값·127/128BYTE 번역 복사를 확인한다.
        for step,(word,size,backward) in enumerate(((b'',0,1),(None,0,1),(None,0,0),(b'd',1,0),(None,0,0),(b'Z'*128,128,0),(None,0,1))):
            rows.append(dict(name=f'translate-{mode}-{step}',reset=int(step==0),records=records('sparse'),mode=mode,query='~' if word is None else encoded(word),length=size,backward=backward))
    for step,word in enumerate((b'ABCDEFGH',b'AB',b'',b'\xc0')):  #짧게다시입력할때strncpy뒤의미작성접두꼬리를보존한다.
        rows.append(dict(name=f'tail-{step}',reset=int(step==0),records=records('empty'),mode=0,query=encoded(word),length=len(word),backward=0))
    return rows


def main():
    """같은초기입력열을두번관측하고최초fixture와후속재관측을구별한다."""
    assert digest(ROOT/'originals/Netstorm.exe')=='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236';inputs=cases();runs=[]
    for repeat in range(2):  #두관측은별도CPU/별도BSS에서시작한다.
        oracle=Oracle();runs.append([{**row,**oracle.run(row)} for row in inputs])
    assert runs[0]==runs[1];data=('\t'.join(SCHEMA)+'\n'+'\n'.join('\t'.join(str(row[key]) for key in SCHEMA) for row in runs[0])+'\n').encode('utf-8')
    if FIXTURE.exists():assert FIXTURE.read_bytes()==data
    else:FIXTURE.write_bytes(data)
    report={'host':socket.gethostname(),'rows':len(inputs),'independentObservations':len(inputs)*2,'differences':0,'fixtureSha256':digest(FIXTURE),'nativeBodies':['0042e540','004e51f0 strncpy','004ef5a0 ascii_strnicmp'],'externalDevices':['Translate: declared input mapping','__strnicmp wrapper: C locale selection','assertion: record and continue'],'scope':'실제전역상태/후보수집/순환/실제CRT복사·비교.게임/OS/통신/멀티플레이실행없음.'}
    target=ROOT/'cpppj/chat-completion-native-20261029.json';text=json.dumps(report,ensure_ascii=False,indent=2)+'\n'
    if target.exists():assert target.read_text(encoding='utf-8')==text
    else:target.write_bytes(text.encode('utf-8'))
    print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':main()
