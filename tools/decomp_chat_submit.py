"""채팅 제출/수신자 추가와 실제 Regexp를 격리 PE에서 관측한다. 게임·통신은 실행하지 않는다."""
import argparse
import csv
import json
import socket
from pathlib import Path
from decomp_chat_recipients_regex import NativeRegexOracle
from decomp_priestmenu import ROOT, POOL, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

MANAGER, BUFFER, NEW = POOL+0x10000, POOL+0x1000, POOL+0x70000  # 실제 할당 대신 쓸 명시 물리 주소/식별자다.
SUBMIT_REGEX = 0x548718  # 원문 대상/본문 분리의 별도 Regexp 객체다.
WIDTH = 2712  # 원문 manager의 전체 실제 크기다.
FIXTURE = ROOT/'cpppj/tests/fixtures/chat-submit-x86.tsv'  # 제품 작성 전에 고정하는 원문 관측이다.
REPORT = ROOT/'cpppj/chat-submit-native-20261030.json'  # 실행 범위/명시 장치/최초 해시다.
SCHEMA = ('kind','source','scene','mode','count','capacity','subscriber','status','manager','oldBuffer','newBuffers','initialIdentity','identity','initialText','textWork','trace')  # 입력과 실제 전체 출력을 분리한다.


def cases():
    """콜론/네 공백/이름/채널/2048 복사 경계와 실제32슬롯 확장을 입력으로 지정한다."""
    texts = [b'',b' ',b'\t\r\n',b'hello',b' hello',b':hello',b':',b'Alice:hi',b'Alice Bob:hi',b'Alice Alice:hi',b'Unknown:hi',b'Alice Unknown:hi',b'Unknown Alice:hi',b'EVERYONE:hi',b'ALLIES BATTLE:hi',b'Alice EVERYONE:hi',b'EVERYONE Alice:hi',b'OBSERVER:hi',b'\tAlice\rBob :\n\thi:again',b'Alice:',b'EVERYONE: \t\n',b'Alice\v:hi',b'Alice\f:hi',b'\xa0:hi',b'A'*40+b':hi',b'A'*41+b':hi',b'hello\0Alice:later',b'\t .clear',b'EVERYONE:.clear']
    for text in texts:  # 고정 입력의 교차만 지정하고 기대 결과는 실제 원문에서 얻는다.
        for scene in range(3):
            for mode in range(4):
                yield ('submit',text.hex() or '-',scene,mode,7,32,0)
    for text in (b'Tous:bonjour',b'Alice Bob:hi',b'EVERYONE:hi'):
        for mode in (4,5,6,7):  # 번역/선택 중단/할당 실패/free 중단의 입력이다.
            yield ('submit',text.hex(),2,mode,7,32,0)
    for size in (2047,2048,2049):  # 본문과 접두 버퍼의 인접 NUL까지 실제 기계어로 관측한다.
        for prefix in (b'',b'Alice:',b'Unknown:'):
            yield ('submit',(prefix+b'x'*size).hex(),1,0,7,32,0)
    for count,capacity in ((0,0),(0,32),(31,32),(32,32),(33,64),(64,64)):
        for subscriber in (100,0xdeadbeef):  # 중복과 새 추가를 같은 물리 입력에서 나누어 지정한다.
            yield ('add','-',0,0,count,capacity,subscriber)
    for mode in (6,7):
        yield ('add','-',0,mode,32,32,0xdeadbeef)


def records(scene):
    """현재 Chat100행의 이름/flags/subscriber 입력만 구성한다."""
    result = bytearray(4800)
    for index in range(100):  # scene0은빈행,1은두사람/중복/고비트,2는100수신자로확장을요구한다.
        if scene == 0:
            continue
        if scene == 1 and index >= 6:
            continue
        value = index+11 if scene == 2 else (11,22,11,0,33,44)[index]
        flag = 0x20 if scene == 2 else (0x20,0x40,0x28,0xff,0x80,0xa0)[index]
        result[index*48+40] = flag
        result[index*48+44:index*48+48] = value.to_bytes(4,'little')
    return result


class Oracle(NativeRegexOracle):
    """제출550BYTE·추가153BYTE·채널조회·Identity초기화·Regexp엔진은 실제 CPU다."""
    def __init__(self):
        """이미 컴파일한 토큰 패턴과 별도로 원문 대상/본문 패턴을 컴파일한다."""
        super().__init__(); self.allowed.extend(((0x42da00,0x42dc26),(0x42d1b0,0x42d249),(0x42d120,0x42d156),(0x4e4a40,0x4e4a7d)))
        self.put(STACK,STOP);self.put(STACK+4,0x5049c8);self.mu.reg_write(UC_X86_REG_ECX,SUBMIT_REGEX);self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.emu_start(0x496c90,STOP,count=1000000);assert self.mu.reg_read(UC_X86_REG_EIP)==STOP

    def instruction(self,mu,address,size,data):
        """외부 조회/할당/실행 장치만 응답하고 선택 순서와 전체 쓰기는 원문에서 읽는다."""
        sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,5)
        if address==0x495a10 and mu.reg_read(UC_X86_REG_ECX)==SUBMIT_REGEX and args[0]==3:
            self.body=args[1];self.initialText=bytes(mu.mem_read(self.body,4097)).hex();self.initialIdentity=bytes(mu.mem_read(self.body-60,60)).hex()
        if address==0x4841c0:
            record=mu.reg_read(UC_X86_REG_ECX);name=self.string(record+8);self.trace.append('R:'+name.hex());self.resolves+=1
            if self.mode==5 and self.resolves==2:raise InterruptedError('resolve')
            if self.mode==3 and name==b'Unknown':mu.mem_write(record+8,b'ALLIES\0')
            accepted=self.mode==2 or (self.mode!=1 and name.lower() in (b'alice',b'bob'))
            if accepted:self.put(record+4,22 if name.lower()==b'bob' else 11)
            self.ret(int(accepted));return
        if address==0x4de9a0:
            key=self.string(args[0]);self.trace.append('T:'+key.hex());value=b'Tous' if self.mode==4 and key==b'EVERYONE' else key
            mu.mem_write(TEXT+0x9000,value+b'\0');self.ret(TEXT+0x9000,4);return
        if address==0x4e4391:
            width=args[0];self.trace.append('N:'+str(width))
            if self.mode==6:self.ret(0);return
            token=self.next;self.next+=(width+15)&~15;mu.mem_write(token,bytes([0xd3])*width);self.buffers.append((token,width));self.ret(token);return
        if address==0x4e4354:
            self.trace.append('F:'+str(args[0]));
            if self.mode==7:raise InterruptedError('free')
            self.ret(0);return
        if address==0x4e0620:
            self.trace.append('A:'+str(args[2]));raise InterruptedError('assertion')
        if address==0x4e51f0:
            destination,source,count=args[:3];raw=self.string(source);mu.mem_write(destination,(raw[:count]+bytes(max(0,count-len(raw))))[:count]);self.ret(destination);return
        if address==0x42f270:self.trace.append('V:'+str(args[0]));self.ret(0);return
        if address==0x43f6f0:self.trace.append('D:'+self.string(args[0]).hex());self.ret(0);return
        super().instruction(mu,address,size,data)

    def sample(self,case,control):
        """서로 독립인 입력 표/스택/할당을 만들고 정상/선택중단을 구별한다."""
        kind,source,scene,self.mode,count,capacity,subscriber=case
        manager=bytearray((165+index*17)&255 for index in range(WIDTH))
        for at,value in ((0,count),(4,capacity),(8,BUFFER),(12,7),(16,0xaabbccdd)):
            manager[at:at+4]=value.to_bytes(4,'little')
        self.mu.mem_write(MANAGER,bytes(manager))
        if capacity:self.mu.mem_write(BUFFER,b''.join((100+index).to_bytes(4,'little') for index in range(capacity)))
        self.mu.mem_write(0x558ed0,bytes(records(scene)));self.mu.mem_write(TEXT,bytes.fromhex('' if source=='-' else source)+b'\0')
        self.mu.mem_write(STACK-0x10000,bytes([0xa5])*0x10100);self.put(STACK,STOP);self.put(STACK+4,TEXT if kind=='submit' else subscriber)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0x11223344)  # 관측 사이에 일반 레지스터 입력을 이어받지 않는다.
        self.mu.reg_write(UC_X86_REG_ECX,MANAGER);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.next=NEW;self.buffers=[];self.trace=[];self.resolves=0;self.body=0;self.initialText='-';self.initialIdentity='-';status='normal'
        try:self.mu.emu_start(0x42da00 if kind=='submit' else 0x42d1b0,STOP,count=2000000)
        except InterruptedError as error:status=str(error)
        if status=='normal':assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+8
        return dict(zip(SCHEMA,(*case,status,bytes(self.mu.mem_read(MANAGER,WIDTH)).hex(),bytes(self.mu.mem_read(BUFFER,capacity*4)).hex() if capacity else '-', '|'.join(str(token)+':'+bytes(self.mu.mem_read(token,width)).hex() for token,width in self.buffers) or '-', self.initialIdentity,bytes(self.mu.mem_read(self.body-60,60)).hex() if self.body else '-',self.initialText,bytes(self.mu.mem_read(self.body,4097)).hex() if self.body else '-', ';'.join(self.trace) or '-')))


def main():
    """두 정밀도를 먼저 대조하고 최초 관측은 새 파일에만 저장한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args();oracle=Oracle();rows=[]
    for case in cases():  # 부모의 실제 쓰기/외부 순서가 두 독립 실행에서 같은지 확인한다.
        pair=[oracle.sample(case,control) for control in (0x27f,0x37f)];assert pair[0]==pair[1],case;rows.append(pair[0])
    sources={path.relative_to(ROOT).as_posix():digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/NETSTORM.c',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__),ROOT/'tools/decomp_chat_recipients_regex.py',ROOT/'tools/decomp_chat_recipients.py',ROOT/'tools/decomp_priestmenu.py')}
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'));assert report['sources']==sources and report['fixtureSha256']==digest(FIXTURE)
        with FIXTURE.open(encoding='utf-8',newline='') as stream:assert list(csv.DictReader(stream,delimiter='\t'))==[{key:str(value) for key,value in row.items()} for row in rows]
        print(json.dumps({'verified':len(rows),'differences':0}));return
    if FIXTURE.exists() or REPORT.exists():raise FileExistsError('최초 관측 덮어쓰기 금지')
    with FIXTURE.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.DictWriter(stream,SCHEMA,delimiter='\t',lineterminator='\n');writer.writeheader();writer.writerows(rows)
    report=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-23',rows=len(rows),observations=len(rows)*2,sources=sources,fixtureSha256=digest(FIXTURE),native=['0042da00','0042d1b0','0042d120','00484190/1b0','00496c90/004964e0 및 실제 엔진/캡처','004e4a40'],explicitDevices=['Identity 응답/이름 변경','번역/CRT','할당/free/선택 중단','입력줄/로컬 명령 분배 진입'],gameOrNetworkExecuted=False)
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps({'rows':len(rows),'observations':len(rows)*2}))


if __name__=='__main__':main()
