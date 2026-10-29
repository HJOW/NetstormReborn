"""원문0042dc90의 단독 자기 메시지/로컬 중계만 관측한다. 원격 MP 분기는 실행하지 않는다."""
import argparse
import csv
import json
import socket
from pathlib import Path
from decomp_chat_packet import Oracle as PacketOracle, ROOT, POOL, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

PACKET=TEXT+0x5000  # 중계가 같은 물리에 마지막NUL과 수신자 count/첫 항목을 쓰는 실제 패킷이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/chat-relay-x86.tsv'  # 제품보다 먼저 고정할 원문 독립 출력이다.
REPORT=ROOT/'cpppj/chat-relay-native-20261030.json'  # 최초 관측의 해시/실행 경계를 남긴다.
SCHEMA=('battle','server','count','source','mode','status','mask','packet','trace')  # 입력과 현재 마스크/전체 패킷/효과를 나눈다.


def cases():
    """단독 자기 수신과 미접속 로컬 경로에서 실제 접두 atoi/수신자 count 변화를 교차한다."""
    for battle in (0,1):  # 전투에서만 마지막NUL과 세자리 마스크를 갱신한다.
        for server in (0,1):  # 로컬 수신 전용과 단독 서버의 자기 분배를 구별한다.
            for count in (0,1,3):  # 빈 수신자와 자기 중복만 사용해 원격 Player 분기를 실행하지 않는다.
                for source in (b'032hello\n',b'008~rword\n',b'0-1hi\n',b'-12hi\n',b'256hi\n',b'999long\n',b' 32hi\n',b'\t64hi\n',b'x12hi\n',b'32\0tail'):
                    yield (battle,server,count,source.hex(),'normal')
            for mode in ('stopAddress','addressBattleOff','stopReceive'):yield (battle,server,1,b'032body\n'.hex(),mode)  # 실제 선택 장치의 부분효과도 독립 관측한다.


class Oracle(PacketOracle):
    """중계·문자열/substring은 원문이며 CRT atol·주소 준비·로컬 수신은 명시 장치다."""
    def __init__(self):
        """공통 CRT 장치에 중계/substring/atoi의 검토한 실제 범위만 더한다."""
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):  # 분리된 실제 범위도 빠뜨리지 않는다.
                if int(row['entry'],16) in (0x42dc90,0x4e31c0,0x4e66de):self.allowed.extend((int(a,16),int(b,16)+1) for a,b in (part.split('-') for part in row['ranges'].split(';')))
        self.address=0  # 현재 관측의 스택20BYTE 주소를 기록하는 토큰이다.

    def instruction(self,mu,address,size,data):
        """원격 Player/송신 진입은 선언하지 않으며 로컬 주소·수신 장치의 순서만 기록한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,4)
        if address==0x4e2a50:
            self.address=args[0];self.trace.append('address:'+str(args[1]));mu.mem_write(args[0],bytes(range(20)))
            if self.mode=='stopAddress':raise InterruptedError('address')
            if self.mode=='addressBattleOff':self.put(0x594fbc,0)
            self.ret(args[0]);return
        if address==0x42f0e0:
            self.trace.append('receive:'+bytes(mu.mem_read(args[0],256)).hex()+':'+bytes(mu.mem_read(args[1],20)).hex())
            if self.mode=='stopReceive':raise InterruptedError('receive')
            self.ret(0);return
        if address==0x4e66de:
            text=self.string(args[0]).lstrip(b' \r\n\t\v\f');sign=1
            if text[:1] in (b'+',b'-'):
                if text[:1]==b'-':sign=-1
                text=text[1:]
            value=0
            for byte in text:  # 좁은 CRT 십진수의 첫 비숫자에서 종료한다.
                if byte<48 or byte>57:break
                value=value*10+byte-48
            self.ret(sign*value);return
        return super().instruction(mu,address,size,data)

    def sample(self,case):
        """같은256BYTE 패킷과 현재 전역을 되돌린 뒤 실제 원문을 실행한다."""
        battle,server,count,source,self.mode=case;body=bytes.fromhex(source);self.next_heap=POOL+0x10000;self.trace=[];self.address=0
        self.mu.mem_write(PACKET,b'\xd3'*256);self.put(PACKET,3,1);self.put(PACKET+1,0x41,1);self.put(PACKET+2,22+len(body)+count*4,2);self.put(PACKET+5,1,1);self.put(PACKET+6,100);self.put(PACKET+10,7,2);self.put(PACKET+12,len(body),2);self.put(PACKET+14,count,1);self.mu.mem_write(PACKET+15,b'Local\0\0'+body)
        for index in range(count):self.put(PACKET+22+len(body)+index*4,100)  # 단독 자기 subscriber만 배열에 넣는다.
        for at,value in ((0x540c74,100),(0x594fbc,battle),(0x540bc0,server),(0x548710,0xaabbccdd)):self.put(at,value)  # 비전투는 이전 마스크를 그대로 유지해야 한다.
        self.put(0,0);self.mu.mem_write(STACK-0x6000,b'\xa5'*0x6200);self.put(STACK,STOP);self.put(STACK+4,9);self.put(STACK+8,0);self.put(STACK+12,PACKET)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0x11223344)  # 이전 관측의 임시 레지스터를 물려주지 않는다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK);status='normal'
        try:self.mu.emu_start(0x42dc90,STOP,count=500000)
        except InterruptedError as error:status=str(error)
        if status=='normal':assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        return dict(zip(SCHEMA,(*case,status,self.words(0x548710,1)[0],bytes(self.mu.mem_read(PACKET,256)).hex(),';'.join(self.trace) or '-')))


def main():
    """기대값은 신규 생성만 하며 verify는 원문을 다시 실행하고 기존 파일과 대조한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args();assert socket.gethostname().lower()=='hjow-athlon'
    assert digest(ROOT/'originals/Netstorm.exe')=='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'
    oracle=Oracle();rows=[oracle.sample(case) for case in cases()]  # 기대 파일을 읽기 전에 모든 원문 관측을 끝낸다.
    content='\t'.join(SCHEMA)+'\n'+''.join('\t'.join(str(row[key]) for key in SCHEMA)+'\n' for row in rows)
    if args.verify:assert FIXTURE.read_text(encoding='utf-8')==content
    else:
        with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:stream.write(content)
        report={'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','rows':len(rows),'sources':{p.relative_to(ROOT).as_posix():digest(p) for p in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/Netstorm.c',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__),ROOT/'tools/decomp_chat_packet.py')},'fixtureSha256':digest(FIXTURE),'actualNative':['0042dc90','0040a970/80','004e3180/31c0/2f40/30b0/31a0','0046ee50'],'explicitDevices':['CRT new/free/strncpy/atol','004e2a50 주소','0042f0e0 로컬 수신'],'gameOrNetworkExecuted':False,'limits':['원격 Player/다른 채널 송신 분기는 새로 검사하지 않음','실제 전송/UI 접합은 별도 검사']}
        with REPORT.open('x',encoding='utf-8',newline='\n') as stream:json.dump(report,stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps({'rows':len(rows),'verified':args.verify,'fixtureSha256':digest(FIXTURE)}))


if __name__=='__main__':main()  # import로 원본이나 fixture에 쓰지 않는다.
