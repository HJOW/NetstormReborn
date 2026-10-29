"""일반 Chat 본문 조합·호출 wrapper와 실제 대상 이름 getter를 격리 CPU에서 관측한다."""
import argparse
import csv
import json
import socket
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

FIRST, SECOND, RECIPIENTS = POOL+0x1000, POOL+0x4000, POOL+0x7000  # 출력/대상 manager와 수신자 배열의 명시 물리다.
NAME, NEXT_NAME, CURRENT, SUBSCRIBER = TEXT+0x1000, TEXT+0x2000, 0x548714, 0x540c74  # 실제 현재 이름 포인터와 두 전역을 사용한다.
WIDTH, OUTPUT = 2712, 0x298  # manager 전체와 그 안2048BYTE 본문 조합 영역이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/chat-message-x86.tsv'  # 제품 작성 전에 고정할 독립 원문 출력이다.
REPORT = ROOT/'cpppj/chat-message-native-20261030.json'  # 최초 관측의 해시와 실행·장치 경계를 기록한다.
SCHEMA = ('kind','spec','mask','direct','count','separate','source','name','mode','lookup','active','status','result','currentName','subscriber','managerA','managerB','input','local','trace')  # 입력과 실제 전체 물리·순서를 분리한다.
PATTERNS = (0x5048c4,0x50494c,0x50493c,0)  # 일반·행동·직접 대상·NULL 기본 서식의 원문 포인터다.


def cases():
    """채널 비트 조합·대상 개수·서식과 wrapper 관문/현재값 변경을 독립 입력으로 준다."""
    for mask in range(0,256,8):  # 실제 네 채널 비트와 무관한0x10 비트도 함께 교차한다.
        if mask&0x10:continue
        for direct in (0,1):  # 출력 manager의 직접 대상 표시를 두 값으로 실행한다.
            for count in (0,1,2):  # 빈 배열·상대 한 명·자기와 상대 두 명을 사용한다.
                for spec in range(4):  # 세 고정 서식과 NULL 기본 분기를 각각 관측한다.
                    yield ('format',spec,mask,direct,count,0,b'hello~0world'.hex(),b'Local'.hex(),'normal',1,1)
    sources=(b'',b' ',b'hello',b'!waves',b'!',b'~0only',b'\ttext',b'\xe9~1text',b'hello\0ignored')
    for kind in ('wrap','say'):  # 명시 this wrapper와 현재 manager를 쓰는 say를 구별한다.
        for source in sources:  # 빈/space/행동·제어문자·내부NUL 입력을 전달한다.
            for direct,count in ((0,0),(1,1),(1,2)):  # 자연 경로의 기본/직접 대상/여러 대상을 사용한다.
                for name in (b'Local',b''):  # 빈 현재 이름의 원문 Nameless fallback도 실행한다.
                    yield (kind,0,32,direct,count,0,source.hex() or '-',name.hex() or '-','normal',1,1)
    for active in (0,1):  # say의 현재 manager0과 선행 space/빈 입력 관문을 분리한다.
        for source in (b'',b' ',b'hello',b'!'):
            yield ('say',0,32,1,1,0,source.hex() or '-',b'Local'.hex(),'normal',1,active)
    for spec in (0,2):  # 직접 이름 getter와 출력 서식의 두 호출 위치를 교차한다.
        for mode in ('normal','resolveSwitch','renderSwitch','stopResolve','stopFormat','stopDeliver'):  # 현재값 변경과 각 선택 중단 위치를 별도 입력으로 준다.
            for lookup in (0,1):  # 원문 getter가 실패 문자열을 쓴 뒤 반환0일 때의 첫BYTE 재설정도 본다.
                yield ('format',spec,0xe8,1,2,1,b'body~2tail'.hex(),b'Lo%s'.hex(),mode,lookup,1)


class Oracle(MemoryOracle):
    """조합·say/wrapper·첫 비자기 수신자·이름 getter·슬롯 제거는 실제 기계어다."""
    def __init__(self):
        """보호 PE와 검토한 여섯 함수 범위만 새 CPU에 적재한다."""
        super().__init__();self.allowed=[]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):  # 지정한 본체 외 원문 진입은 허용하지 않는다.
                if int(row['entry'],16) in (0x42f1e0,0x42dc30,0x42d640,0x42d250,0x42d160,0x484410):
                    self.allowed.extend((int(a,16),int(b,16)+1) for a,b in (part.split('-') for part in row['ranges'].split(';')))
        self.trace=[];self.local=0;self.kind='format';self.mode='normal';self.lookup=1;self.receiver=FIRST  # 현재 한 관측의 입력과 실제 임시 출력 포인터다.

    def instruction(self,mu,address,size,data):
        """순수 CRT와 명시 Identity 응답/전달 경계만 장치로 제공하며 패킷·OS는 실행하지 않는다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,5)
        if address==STOP:mu.emu_stop();return
        if address==0x484410:self.local=args[0]
        if address==0x4841c0:
            record=mu.reg_read(UC_X86_REG_ECX);subscriber=self.words(record+4,1)[0];self.trace.append('lookup:'+str(subscriber))
            if self.mode=='stopResolve':raise InterruptedError('resolve')
            if self.mode=='resolveSwitch':
                self.put(0x540ca8,NEXT_NAME);self.put(SUBSCRIBER,7);self.put(self.receiver+12,0)
                mu.mem_write(FIRST+0x14,b'~pLateBack\0');mu.mem_write(FIRST+0x50,b'~uLateFront\0');mu.mem_write(TEXT,b'new~0body\0')
            if self.lookup:mu.mem_write(record+8,b'Ta%s\0')
            self.ret(self.lookup);return
        if address==0x4e58e9:
            target,pattern,name,other=args[:4];self.local=other;formatting=self.string(pattern)
            assert formatting in (b'%s> ',b'%s ',b'%s (to %s)> ')
            parts=formatting.split(b'%s');values=(self.string(name),self.string(other));text=parts[0]
            for index,part in enumerate(parts[1:]):  # 삽입한 이름 안의 %s를 다시 서식으로 해석하지 않는다.
                text+=values[index]+part
            self.trace.append('format:'+formatting.hex()+':'+values[0].hex()+':'+values[1].hex()+':'+str(target-FIRST))
            mu.mem_write(target,text+b'\0')
            if self.mode=='renderSwitch':
                self.put(self.receiver+12,0);self.put(FIRST+16,0x20);mu.mem_write(FIRST+0x50,b'~lRender\0');mu.mem_write(TEXT,b'render~1changed\0')
            if self.mode=='stopFormat':raise InterruptedError('format')
            self.ret(len(text));return
        if address==0x42d160:self.trace.append('strip')
        if address==0x42d280:
            recipient,body,length=args[:3]
            assert mu.reg_read(UC_X86_REG_ECX)==FIRST and body==FIRST+OUTPUT
            self.trace.append('deliver:'+('2' if recipient==SECOND else '1')+':'+str(length)+':'+bytes(mu.mem_read(body,length)).hex())
            if self.mode=='stopDeliver':raise InterruptedError('deliver')
            self.ret(0x55aacc33,12);return
        if address==0x42f270:self.trace.append('visible:'+str(args[0]));self.ret(0);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError('미선언 원문 진입 '+hex(address))

    def sample(self,case):
        """전체 manager/입력·스택을 독립 패턴으로 되돌린 뒤 실제 반환/부분효과를 읽는다."""
        self.kind,spec,mask,direct,count,separate,source,name,self.mode,self.lookup,active=case
        self.receiver=SECOND if separate else FIRST
        for number,token in enumerate((FIRST,SECOND)):  # 두 manager의 전체 실제 물리를 서로 다른 비영 패턴으로 구성한다.
            self.mu.mem_write(token,bytes((0xa5+number*11+index*17)&255 for index in range(WIDTH)))
            for offset,value in ((0,count),(4,32),(8,RECIPIENTS),(12,direct),(16,mask)):
                self.put(token+offset,value)  # 실제 count/capacity/배열/직접 표시/채널을 입력으로만 쓴다.
            self.mu.mem_write(token+0x14,b'~rBACK\0');self.mu.mem_write(token+0x50,b'~gFRONT\0')
        subscribers=() if count==0 else (200,) if count==1 else (100,200)
        for index,value in enumerate(subscribers):self.put(RECIPIENTS+index*4,value)  # 실제 선택 순서의 수신자 DWORD다.
        self.mu.mem_write(TEXT,bytes([0xd3])*256);self.mu.mem_write(TEXT,bytes.fromhex('' if source=='-' else source)+b'\0')
        self.mu.mem_write(NAME,bytes.fromhex('' if name=='-' else name)+b'\0');self.mu.mem_write(NEXT_NAME,b'Changed\0')
        self.put(0x540ca8,NAME);self.put(SUBSCRIBER,100);self.put(CURRENT,FIRST if active else 0)
        self.mu.mem_write(STACK-0x2000,bytes([0xa5])*0x2200);self.put(STACK,STOP)
        if self.kind=='format':
            self.put(STACK+4,self.receiver);self.put(STACK+8,TEXT);self.put(STACK+12,PATTERNS[spec]);entry=0x42d640
        else:self.put(STACK+4,TEXT);entry=0x42dc30 if self.kind=='wrap' else 0x42f1e0
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):
            self.mu.reg_write(register,0x11223344)  # 관측 사이 임시/보존 레지스터를 이어받지 않는다.
        self.mu.reg_write(UC_X86_REG_ECX,FIRST);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.trace=[];self.local=0;status='normal'
        try:self.mu.emu_start(entry,STOP,count=200000)
        except InterruptedError as error:status=str(error)
        if status=='normal':
            purge=12 if self.kind=='format' else 4 if self.kind=='wrap' else 0
            assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge
        output=(status,self.mu.reg_read(UC_X86_REG_EAX) if status=='normal' else '-',2 if self.words(0x540ca8,1)[0]==NEXT_NAME else 1,
                self.words(SUBSCRIBER,1)[0],bytes(self.mu.mem_read(FIRST,WIDTH)).hex(),bytes(self.mu.mem_read(SECOND,WIDTH)).hex(),
                bytes(self.mu.mem_read(TEXT,256)).hex(),bytes(self.mu.mem_read(self.local,256)).hex() if self.local else '-', ';'.join(self.trace) or '-')
        return dict(zip(SCHEMA,(*case,*output)))


def main():
    """최초 출력은 새 파일에만 저장하며 verify는 같은 전체 원문 결과를 다시 비교한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    assert socket.gethostname().lower()=='hjow-athlon'
    assert digest(ROOT/'originals/Netstorm.exe')=='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'
    assert digest(ROOT/'extracted/refined/originals/Netstorm.c')=='af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646'
    oracle=Oracle();rows=[oracle.sample(case) for case in cases()]  # 기대 fixture를 읽기 전에 모든 입력을 실제 원문에서 실행한다.
    content='\t'.join(SCHEMA)+'\n'+''.join('\t'.join(str(row[key]) for key in SCHEMA)+'\n' for row in rows)
    if args.verify:assert FIXTURE.read_text(encoding='utf-8')==content
    else:
        with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:stream.write(content)
        report={'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','rows':len(rows),
                'sources':{path.relative_to(ROOT).as_posix():digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/Netstorm.c',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__))},
                'fixtureSha256':digest(FIXTURE),'actualNative':['0042f1e0','0042dc30','0042d640','0042d250','00484410','0042d160'],
                'explicitDevices':['CRT sprintf','Identity 응답/현재 전역·본문 변경','0042d280 전달 반환/중단','0042f270 입력 표시'],
                'gameOrNetworkExecuted':False,'limits':['패킷 생성/전송0042d280과수신0042ed30은실행하지않음','정상 물리 범위의 일반 본문 조합 부모만관측']}
        with REPORT.open('x',encoding='utf-8',newline='\n') as stream:json.dump(report,stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps({'rows':len(rows),'verified':args.verify,'fixtureSha256':digest(FIXTURE)}))


if __name__=='__main__':main()  # import는 원본과 fixture에 쓰지 않는다.
