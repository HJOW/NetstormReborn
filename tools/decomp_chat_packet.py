"""단독 Chat 송신 부모와 송신 observer를 원본 CPU에서 관측한다. 게임/MP/소켓은 실행하지 않는다."""
import argparse
import csv
import json
import socket
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

MANAGER, RECIPIENTS, NAME = POOL+0x1000, POOL+0x4000, TEXT+0x3000  # 실제 manager/수신자와 현재 이름 포인터의 격리 물리다.
FIXTURE = ROOT/'cpppj/tests/fixtures/chat-packet-x86.tsv'  # 제품 작성 전에 고정하는 독립 원문 기대 출력이다.
REPORT = ROOT/'cpppj/chat-packet-native-20261030.json'  # 최초 관측의 원문·도구 해시와 실행 경계를 남긴다.
SCHEMA = ('kind','battle','direct','mask','count','own','source','name','mode','sending','status','result','subscriber','finalBattle','manager','recipients','packet','trace')  # 부모 입력과 전체 물리·반환·효과를 나눈다.


def cases():
    """단독 전투9와 채널 없는 로컬 fallback만 교차하며 MP1/11은 새로 검사하지 않는다."""
    for battle in (0,1):  # 동일 단독 세션의 전투 서식 유무를 구별한다.
        for direct in (0,1):  # 대상 표시가 전투 서식에0x100을 합치는지 본다.
            for mask in (0,8,32,64,128,232,0xffffffff):  # 세자리 패딩·복합 비트·signed 십진 변환이다.
                for own in (0,1):  # 자기 subscriber 중복 탐색과 누락 추가를 교차한다.
                    for source in (b'hello\0',b'xy\0tail',b'\0'):  # 비전투는 명시 길이 전체, 전투는 첫NUL 앞을 쓴다.
                        yield ('send',battle,direct,mask,2,own,source.hex(),b'Local'.hex(),'normal',1)
    for count in (0,1,31,32,255,256):  # count BYTE 절단과 실제 DWORD 배열 복사 폭을 구별한다.
        for battle in (0,1):  # 경계 count를 같은 단독 전투/비전투에 넣는다.
            for name in (b'',b'N\xe9%s'):  # 빈 이름 fallback과 원문 고위 BYTE/서식 문자를 비교한다.
                yield ('send',battle,0,32,count,int(count>0),b'body\0'.hex(),name.hex() or '-','normal',1)
    for battle in (0,1):  # 각 실제 외부효과에서 중단한 앞부분과 현재값 변경도 고정한다.
        for mode in ('addSwitch','stopAdd','stopSend','stopLog','stopReceive'):
            yield ('send',battle,0,32,2,0,b'body\0'.hex(),b'Local'.hex(),mode,1)
    for name_size in (1,0xffff,0x8000):  # observer의 WORD 입력과 실제 실행 가능한 두 assertion을 구별한다.
        for message_size in (0,5,0xffff):  # 비어 있는 필드와 최대 WORD의 합산을 구별한다.
            for count in (0,2,255):  # 최대 BYTE의 assertion도 실제 명령으로 관측한다.
                for sending in (0,1):  # 수신 observer는 모든 현재 바이트를 보존한다.
                    yield ('observe',0,name_size,message_size,count,0,'-','-','normal',sending)


class Oracle(MemoryOracle):
    """송신/observer·패킷 주소 getter·문자열 몸체는 실제 명령이며 외부 장치만 공급한다."""
    def __init__(self):
        """검토한 본체 범위 외 진입을 차단하고 보호 PE를 별도 CPU 메모리에 적재한다."""
        super().__init__();self.allowed=[];self.mu.mem_map(0,0x1000)  # 부모 SEH의 FS[0] 체인만 격리 메모리에 둔다.
        entries=(0x42d280,0x40a8d0,0x40a960,0x40a970,0x40a980,0x4df590,0x4e3180,0x4e30b0,0x4e30a0,0x4e3550,0x4e3200,0x4e33e0,0x4e2f40,0x4e3590,0x4e31a0,0x46ee50,0x4e4a40)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):  # 지정 원문 함수의 실제 분리 범위를 모두 허용한다.
                if int(row['entry'],16) in entries:self.allowed.extend((int(a,16),int(b,16)+1) for a,b in (part.split('-') for part in row['ranges'].split(';')))
        self.trace=[];self.packet=b'';self.next_heap=POOL+0x10000;self.mode='normal'  # 관측마다 새 장치 상태로 되돌리는 실제 저장소다.

    def capture_packet(self,address):
        """WORD 헤더가 아직4일 때도 caller가 실제로 복사한 전체 수신자 폭을 읽는다."""
        name=int.from_bytes(self.mu.mem_read(address+10,2),'little');body=int.from_bytes(self.mu.mem_read(address+12,2),'little')
        count=self.words(MANAGER,1)[0];self.packet=bytes(self.mu.mem_read(address,15+name+body+count*4))
        return self.packet.hex()

    def instruction(self,mu,address,size,data):
        """CRT 할당/서식과 추가·전달·진단·주소 장치만 대체하며 새 MP 진입은 거부한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,5)
        if address==STOP:mu.emu_stop();return
        if address==0x4e4391:
            token=self.next_heap;self.next_heap+=(args[0]+31)&~15;mu.mem_write(token,b'\xcc'*(args[0]+8));self.ret(token);return
        if address in (0x4e422a,0x4e4354,0x4e66e3):self.ret();return
        if address==0x4e58e9:
            assert self.string(args[1])==b'%d';value=args[2] if args[2]<0x80000000 else args[2]-0x100000000
            text=str(value).encode();mu.mem_write(args[0],text+b'\0');self.ret(len(text));return
        if address==0x4e51f0:
            text=self.string(args[1]);mu.mem_write(args[0],text[:args[2]].ljust(args[2],b'\0'));self.ret(args[0]);return
        if address==0x42d1b0:
            assert mu.reg_read(UC_X86_REG_ECX)==MANAGER;self.trace.append('add:'+str(args[0]))
            if self.mode=='stopAdd':raise InterruptedError('add')
            count=self.words(MANAGER,1)[0];self.put(RECIPIENTS+count*4,args[0]);self.put(MANAGER,count+1)
            if self.mode=='addSwitch':
                self.put(0x540c74,7);self.put(0x594fbc,1-self.words(0x594fbc,1)[0]);self.put(MANAGER+16,128);self.put(MANAGER+12,1);mu.mem_write(NAME,b'Modified\0')
            self.ret(0,4);return
        if address==0x4e2dc0:
            assert args[0]==9;self.trace.append('send:9:'+self.capture_packet(args[1]))
            if self.mode=='stopSend':raise InterruptedError('send')
            self.ret(0);return
        if address==0x4c2630:
            self.trace.append('log:'+str(args[0])+':'+self.string(args[1]).hex())
            if self.mode=='stopLog':raise InterruptedError('log')
            self.ret(0);return
        if address==0x4e0cd0:
            assert self.string(args[0])==b'0.0.0.0' and args[1]==0;self.trace.append('address:0.0.0.0:0');self.ret(mu.reg_read(UC_X86_REG_ECX),8);return
        if address==0x42f0e0:
            self.trace.append('receive:'+self.capture_packet(args[0]))
            if self.mode=='stopReceive':raise InterruptedError('receive')
            self.ret(0);return
        if address==0x4e0620:self.trace.append('assert:'+str(args[2]));self.ret(0);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError('미선언 원문 진입 '+hex(address))

    def sample(self,case):
        """독립 입력으로 전체 manager/배열과 실제 패킷·현재 전역·반환·순서를 관측한다."""
        kind,battle,direct,mask,count,own,source,name,self.mode,sending=case
        self.next_heap=POOL+0x10000;self.trace=[];self.packet=b''
        self.mu.mem_write(MANAGER,bytes((0xa5+index*17)&255 for index in range(2712)))  # 미작성 꼬리의 비영 입력도 보존한다.
        self.mu.mem_write(RECIPIENTS,b'\xa5'*1200)
        for offset,value in ((0,count),(4,300),(8,RECIPIENTS),(12,direct),(16,mask)):self.put(MANAGER+offset,value)  # 명시한 다섯 DWORD만 입력으로 바꾼다.
        for index in range(count):self.put(RECIPIENTS+index*4,100 if own and index==count//2 else 200+index)  # 원본 순회에서 자기 항목의 중간 위치도 읽는다.
        self.mu.mem_write(NAME,bytes.fromhex('' if name=='-' else name)+b'\0');self.mu.mem_write(TEXT,bytes.fromhex('' if source=='-' else source)+b'\0')
        for at,value in ((0x540ca8,NAME),(0x540c74,100),(0x594fbc,battle),(0x594fb8,0),(0x594fc8,0),(0x544c5c,0)):self.put(at,value)  # 단독9와 fallback 밖 채널은 모두0이다.
        self.mu.mem_write(STACK-0x6000,b'\xa5'*0x6200);self.put(STACK,STOP)
        if kind=='send':
            self.put(STACK+4,MANAGER);self.put(STACK+8,TEXT);self.put(STACK+12,len(bytes.fromhex(source)));entry=0x42d280;receiver=MANAGER;purge=12
        else:
            self.mu.mem_write(TEXT,b'\xd3'*32);self.put(TEXT+10,direct,2);self.put(TEXT+12,mask,2);self.put(TEXT+14,count,1);self.put(STACK+4,sending);entry=0x40a8d0;receiver=TEXT;purge=4
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0x11223344)  # 이전 관측의 레지스터를 물려주지 않는다.
        self.mu.reg_write(UC_X86_REG_ECX,receiver);self.mu.reg_write(UC_X86_REG_ESP,STACK);status='normal'
        try:self.mu.emu_start(entry,STOP,count=500000)
        except InterruptedError as error:status=str(error)
        if status=='normal':assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge
        if kind=='observe':self.packet=bytes(self.mu.mem_read(TEXT,32))
        output=(status,self.mu.reg_read(UC_X86_REG_EAX) if kind=='send' and status=='normal' else '-',self.words(0x540c74,1)[0],self.words(0x594fbc,1)[0],bytes(self.mu.mem_read(MANAGER,2712)).hex(),bytes(self.mu.mem_read(RECIPIENTS,1200)).hex(),self.packet.hex() or '-',';'.join(self.trace) or '-')
        return dict(zip(SCHEMA,(*case,*output)))


def main():
    """최초 출력만 신규 생성하며 verify는 보호 fixture를 수정하지 않고 재관측한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    assert socket.gethostname().lower()=='hjow-athlon'
    assert digest(ROOT/'originals/Netstorm.exe')=='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'
    oracle=Oracle();rows=[oracle.sample(case) for case in cases()]  # 기존 기대값을 읽기 전에 모든 원문 실행을 마친다.
    content='\t'.join(SCHEMA)+'\n'+''.join('\t'.join(str(row[key]) for key in SCHEMA)+'\n' for row in rows)
    if args.verify:assert FIXTURE.read_text(encoding='utf-8')==content
    else:
        with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:stream.write(content)
        report={'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','rows':len(rows),'sources':{p.relative_to(ROOT).as_posix():digest(p) for p in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/Netstorm.c',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__))},'fixtureSha256':digest(FIXTURE),'actualNative':['0042d280','0040a8d0','0040a960/70/80','004df590','004e3180/30b0/30a0/3550/3200/33e0/2f40/3590/31a0','0046ee50','004e4a40'],'explicitDevices':['CRT new/free/sprintf/strncpy','0042d1b0 수신자 추가/현재값 변경','004e2dc0 단독9 전달','진단/zero주소/로컬수신'],'gameOrNetworkExecuted':False,'limits':['MP1/11 분기는 새로 실행하지 않음','실제 수신0042ed30/전체 일반 Enter는 미연결','유효한 물리 범위의 부모와 observer만 관측']}
        with REPORT.open('x',encoding='utf-8',newline='\n') as stream:json.dump(report,stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps({'rows':len(rows),'verified':args.verify,'fixtureSha256':digest(FIXTURE)}))


if __name__=='__main__':main()  # import만으로 원본/fixture를 수정하지 않는다.
