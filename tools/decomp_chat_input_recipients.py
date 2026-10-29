"""현재 채팅 입력의 대상 포함 검사와Subscriber wrapper를 실제 원문 부모/Regexp/Identity로 관측한다."""
import csv
import json
import socket
from pathlib import Path
from decomp_chat_recipient_identity import JoinedOracle
from decomp_chat_recipients import ROOT, TEXT, STACK, STOP, digest
from decomp_priestmenu import POOL
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

FIXTURE=ROOT/'cpppj/tests/fixtures/chat-input-recipients-x86.tsv'  # 독립 최초 기대값이며 이전 fixture를 덮어쓰지 않는다.
REPORT=ROOT/'cpppj/chat-input-recipients-native-20261029.json'  # 실제 CPU와UI 문자열 장치의 경계를 구별한다.
CHAT=POOL+0x90000;CONSOLE=CHAT+0x400;TARGET=TEXT+0xd000  # 실제 원문 객체 배치를 빌리는 격리 물리다.
SCHEMA=('operation','scene','manager','input','target','subscriber','result','trace','rawEax','instructions')  # 사용되는하위BYTE와원시EAX를분리한다.


def cases():
    """정상/미지/부분이름·대소문자·빈이름·40BYTE·manager0 관문을 독립 교차한다."""
    inputs=(b'',b'Alice:hi',b'ALICE:hi',b'Alice Bob:hi',b'ALLIES Alice:hi',b'Unknown Alice:hi',b'Alice hi',b':hi',b'LocalUser Bob:hi',b'A'*40+b':hi',b'B'*40+b':hi',b'Alice\0Bob:hi')
    for scene in range(3):  # 로컬/일반전투/편집만 사용하며 메타맵이나통신은실행하지않는다.
        for manager in (0,1):
            for text in inputs:
                for target in (b'',b'Alice',b'lice',b'alice',b'Bob',b'B'*40):yield 'contains',scene,manager,text,target,0
                for subscriber in (100,200,300,999):yield 'subscriber',scene,manager,text,b'',subscriber


class InputOracle(JoinedOracle):
    """실제42e9e0/42f640과현재입력복사장치를기존원문검사/검색에연결한다."""
    def __init__(self):
        """두 부모의 정확한 범위와 실제 Chat+0xa0 Console 포인터를 준비한다."""
        super().__init__();self.inputParents=[];self.currentInput=b''
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):  # 원문 두 부모만추가허용한다.
                if int(row['entry'],16) in (0x42e9e0,0x42f640):self.inputParents.extend((int(a,16),int(b,16)+1) for a,b in (part.split('-') for part in row['ranges'].split(';')))

    def instruction(self,mu,address,size,data):
        """입력은실제CopyText호출위치에서만제공하고strstr도같은바이트규칙으로처리한다."""
        if any(low<=address and address+size<=high for low,high in getattr(self,'inputParents',[])):self.instructions+=1;return
        if address==0x451e80:
            assert mu.reg_read(UC_X86_REG_ECX)==CONSOLE;destination,width=self.words(mu.reg_read(UC_X86_REG_ESP)+4,2);assert width==128;value=self.currentInput.split(b'\0',1)[0];assert len(value)<width;self.trace.append('C');mu.mem_write(destination,value+b'\0');self.ret(len(value),8);return
        if address==0x4e6770:
            source,target=self.words(mu.reg_read(UC_X86_REG_ESP)+4,2);found=self.string(source).find(self.string(target));self.ret(0 if found<0 else source+found);return
        super().instruction(mu,address,size,data)

    def runInput(self,case):
        """현재표입력을먼저준비하고지정한부모의정상반환/순서를관측한다."""
        operation,scene,manager,self.currentInput,target,subscriber=case
        self.joined(scene,'-');self.trace=[];self.instructions=0;self.put(0x548714,manager);self.put(0x548740,CHAT);self.put(CHAT+0xa0,CONSOLE);self.mu.mem_write(TARGET,target+b'\0');self.mu.mem_write(STACK-0x2000,bytes([0xa5])*0x2100);self.put(STACK,STOP);self.put(STACK+4,subscriber if operation=='subscriber' else TARGET);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,CHAT)
        self.mu.emu_start(0x42f640 if operation=='subscriber' else 0x42e9e0,STOP,count=1000000);assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+(4 if operation=='subscriber' else 8);eax=self.mu.reg_read(UC_X86_REG_EAX)
        return dict(zip(SCHEMA,(operation,scene,manager,self.currentInput.hex() or '-',target.hex() or '-',subscriber,eax&255,';'.join(self.trace) or '-',eax,self.instructions)))


def main():
    """최초 기대값을 고정하고 재실행은 기존 행/해시를 바꾸지 않고 대조한다."""
    oracle=InputOracle();rows=[oracle.runInput(case) for case in cases()]
    paths=(ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/NETSTORM.c',ROOT/'extracted/refined/originals/functions.tsv',ROOT/'tools/decomp_chat_recipients.py',ROOT/'tools/decomp_chat_recipients_regex.py',ROOT/'tools/decomp_chat_recipient_identity.py',Path(__file__))
    report={'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','rows':len(rows),'instructions':sum(row['instructions'] for row in rows),'native':'42e9e0/42f640+실제채팅대상부모/Regexp/PlayerFinder','explicitDevices':['현재Console의CopyText128 결과','malloc','C locale CRT/strstr','영어번역'],'sources':{str(p.relative_to(ROOT)).replace('\\','/'):digest(p) for p in paths},'gameOrNetworkExecuted':False}
    if FIXTURE.exists() or REPORT.exists():
        with FIXTURE.open(encoding='utf-8',newline='') as stream:expected=list(csv.DictReader(stream,delimiter='\t'))
        assert [{key:str(value) for key,value in row.items()} for row in rows]==expected;report['fixtureSha256']=digest(FIXTURE);assert json.loads(REPORT.read_text(encoding='utf-8'))==report
    else:
        with FIXTURE.open('w',encoding='utf-8',newline='') as stream:writer=csv.DictWriter(stream,SCHEMA,delimiter='\t',lineterminator='\n');writer.writeheader();writer.writerows(rows)
        report['fixtureSha256']=digest(FIXTURE);REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'inputNativeRows':len(rows),'differences':0}))


if __name__=='__main__':main()
