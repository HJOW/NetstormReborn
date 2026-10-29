"""단독 Chat 수신의 필터·본문·문맥·UI 효과를 격리 CPU에서 관측한다. 게임/통신은 실행하지 않는다."""
import argparse
import csv
import json
import socket
from pathlib import Path
from decomp_chat_packet import Oracle as PacketOracle, ROOT, POOL, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

CHAT, PACKET = POOL+0x8000, TEXT+0x5000  # 실제 현재 Chat 포인터와 수신 패킷의 격리 저장소다.
KEYS = ('ShowBattle','ShowEveryone','ShowAllies','ShowObserver','ShowWisper','ShowMyAllies','ShowMyEmenys','ShowMyNeutrals','ShowChatWindow','PlaySoundMessage')  # 원문의 오타와 조회 순서도 유지한다.
FIXTURE = ROOT/'cpppj/tests/fixtures/chat-receive-x86.tsv'  # 제품보다 먼저 고정할 원문 기대값이다.
REPORT = ROOT/'cpppj/chat-receive-native-20261030.json'  # 최초 원문 관측과 명시 외부 장치의 근거다.
SCHEMA = ('kind','version','encoding','battle','mask','sender','flags','allied','neutral','ignored','mode','source','status','packet','news','subscriber','finalBattle','trace')  # 부모 입력과 수정된 전체 패킷/현재값/효과를 분리한다.


def cases():
    """실제 단독 수신의 채널·관계 옵션과 무시/버전/현재값 변경·중단을 교차한다."""
    def case(battle=1,mask=32,sender=200,flags=1023,allied=0,neutral=0,ignored=0,mode='normal',kind='direct',version=3,encoding=1):
        """본문 마지막 줄바꿈 또는NUL을 명시 길이 안에 넣어 수신의 강제NUL 쓰기를 관측한다."""
        source=b'032hello\n' if battle else b'hello\n\0'
        return (kind,version,encoding,battle,mask,sender,flags,allied,neutral,ignored,mode,source.hex())
    for battle in (0,1):  # 비전투는 옵션을 조회하지 않는 별도 부모다.
        for sender in (100,200):  # 자기 메시지의 소리 억제와 상대 메시지 알림을 비교한다.
            for flags in (0,1023):  # 전체 옵션 off/on을 같은 원문 본체에 준다.
                for mask in (0,8,32,64,128,256,232):yield case(battle,mask,sender,flags)  # 각각의 채널과 혼합/없음을 구별한다.
    for channel,mask in enumerate((8,32,64,128,256)):  # 채널 하나만 켜졌을 때 대응 옵션과 관계 필터를 분리한다.
        for relation_bits in range(8):  # ShowMyAllies/Emenys/Neutrals의 모든 조합이다.
            for allied in (0,1):  # 현재 관계 배열의0/비영을 구별한다.
                for neutral in (0,1):yield case(mask=mask,flags=(1<<channel)|(relation_bits<<5)|768,allied=allied,neutral=neutral)  # 관전자 표시BYTE도 별도 관문이다.
    for battle in (0,1):  # 선행 두 거부는 패킷NUL을 쓰기 전에 종료한다.
        yield case(battle,version=4);yield case(battle,encoding=2);yield case(battle,ignored=1)
        for mode in ('stopConfig','configBattleOff','wakeSwitch','stopWake','stopAppend','stopShow'):yield case(battle,mode=mode)  # 선택하지 않은 장치는 호출되지 않는다.
        yield case(battle,kind='wrap');yield case(battle,kind='noRoot')


class Oracle(PacketOracle):
    """수신·wrapper·문맥 생성·문자열 몸체는 실제 명령이고 외부 UI/설정만 공급한다."""
    def __init__(self):
        """송신 관측의 순수CRT 장치를 재사용하되 수신 부모 범위를 추가로 허용한다."""
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):  # 수신/wrapper·실제 문맥과 문자열 지역 함수만 더한다.
                if int(row['entry'],16) in (0x42ed30,0x42f0e0,0x4248a0,0x4e3090,0x4e3160,0x4e31c0):self.allowed.extend((int(a,16),int(b,16)+1) for a,b in (part.split('-') for part in row['ranges'].split(';')))
        self.flags=0;self.allied=0;self.neutral=0;self.ignored=0  # 다음 독립 입력에서 덮어쓰는 외부 상태다.

    def instruction(self,mu,address,size,data):
        """선택된 외부효과의 인자/순서와 callback 뒤 현재 전역 재조회를 기록한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,6)
        if address==0x42f0e0:
            pass  # wrapper는 기존 송신 관측의 수신 stub을 쓰지 않고 실제 명령을 실행한다.
        elif address==0x42e300:
            self.trace.append('ignored:'+str(args[0]));self.ret(self.ignored,4);return
        elif address==0x441270:
            key=self.string(args[0]).decode();self.trace.append('config:'+key);self.put(args[1],int(bool(self.flags&(1<<KEYS.index(key)))))
            if self.mode=='stopConfig':raise InterruptedError('config')
            if self.mode=='configBattleOff' and key==KEYS[-1]:self.put(0x594fbc,0)
            self.ret(1);return
        elif address==0x484550:self.trace.append('owner:'+str(args[0]));self.ret(3);return
        elif address==0x419d50:
            self.trace.append('wake:'+str(args[0]))
            if self.mode=='wakeSwitch':self.put(0x540c74,self.words(PACKET+6,1)[0]);self.put(0x5b5dd8,99)
            if self.mode=='stopWake':raise InterruptedError('wake')
            self.ret(1);return
        elif address==0x4a9cb0:
            assert args[1:]==(0,0,0,0,0);self.trace.append('sound:'+self.string(args[0]).decode());self.ret(0);return
        elif address==0x42eca0:
            assert mu.reg_read(UC_X86_REG_ECX)==CHAT;self.trace.append('append:'+self.string(args[0]).hex()+':'+bytes(mu.mem_read(args[1],32)).hex())
            if self.mode=='stopAppend':raise InterruptedError('append')
            self.ret(0,8);return
        elif address==0x42e410:
            self.trace.append('show')
            if self.mode=='stopShow':raise InterruptedError('show')
            self.ret(0);return
        else:return super().instruction(mu,address,size,data)
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError('미선언 수신 원문 진입 '+hex(address))

    def sample(self,case):
        """전체 패킷/전역을 독립 비영 입력으로 되돌린 뒤 정상·중단 결과를 기록한다."""
        kind,version,encoding,battle,mask,sender,self.flags,self.allied,self.neutral,self.ignored,self.mode,source=case
        self.next_heap=POOL+0x10000;self.trace=[];self.mu.mem_write(PACKET,b'\xd3'*256);name=b'Sender\0';body=bytes.fromhex(source)
        self.put(PACKET,version,1);self.put(PACKET+1,0x41,1);self.put(PACKET+2,15+len(name)+len(body)+4,2);self.put(PACKET+5,encoding,1);self.put(PACKET+6,sender)
        self.put(PACKET+10,len(name),2);self.put(PACKET+12,len(body),2);self.put(PACKET+14,1,1);self.mu.mem_write(PACKET+15,name+body+(100).to_bytes(4,'little'))
        for at,value in ((0x540c74,100),(0x594fbc,battle),(0x594fc8,0),(0x548710,mask),(0x548748,7),(0x540c70,2),(0x5ca92c,0x1234),(0x5b5dd8,44),(0x548740,0 if kind=='noRoot' else CHAT),(0x595200+(2*9+3)*4,self.allied)):self.put(at,value)  # 한 단독 수신에 필요한 현재 슬롯만 쓴다.
        self.put(0x52f81e+3,self.neutral,1);self.put(0,0);self.mu.mem_write(STACK-0x6000,b'\xa5'*0x6200);self.put(STACK,STOP);self.put(STACK+4,PACKET);self.put(STACK+8,0)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0x11223344)  # 이전 관측 레지스터를 이어받지 않는다.
        self.mu.reg_write(UC_X86_REG_ECX,CHAT);self.mu.reg_write(UC_X86_REG_ESP,STACK);status='normal';entry=0x42ed30 if kind=='direct' else 0x42f0e0
        try:self.mu.emu_start(entry,STOP,count=500000)
        except InterruptedError as error:status=str(error)
        if status=='normal':assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+(12 if kind=='direct' else 4)
        output=(status,bytes(self.mu.mem_read(PACKET,256)).hex(),self.words(0x548748,1)[0],self.words(0x540c74,1)[0],self.words(0x594fbc,1)[0],';'.join(self.trace) or '-')
        return dict(zip(SCHEMA,(*case,*output)))


def main():
    """최초 fixture는 신규 생성만 하고 verify는 원문을 다시 실행해 같은 출력과 비교한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args();assert socket.gethostname().lower()=='hjow-athlon'
    assert digest(ROOT/'originals/Netstorm.exe')=='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'
    oracle=Oracle();rows=[oracle.sample(case) for case in cases()]  # 기대값을 읽기 전에 모든 원문 실행을 마친다.
    content='\t'.join(SCHEMA)+'\n'+''.join('\t'.join(str(row[key]) for key in SCHEMA)+'\n' for row in rows)
    if args.verify:assert FIXTURE.read_text(encoding='utf-8')==content
    else:
        with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:stream.write(content)
        report={'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','rows':len(rows),'sources':{p.relative_to(ROOT).as_posix():digest(p) for p in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/Netstorm.c',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__),ROOT/'tools/decomp_chat_packet.py')},'fixtureSha256':digest(FIXTURE),'actualNative':['0042ed30','0042f0e0','0040a960/70','004248a0','004e3090/3160/30b0/30a0/31c0/2f40/3590/3200/31a0','0046ee50'],'explicitDevices':['CRT new/free/strncpy','무시 목록 조회/설정/소유자 조회','UI 알림·소리·로그 append·표시'],'gameOrNetworkExecuted':False,'limits':['시스템 뉴스/MP 중계는 관측하지 않음','실제 Console 일반 Enter와 수신 UI 연결은 후속']}
        with REPORT.open('x',encoding='utf-8',newline='\n') as stream:json.dump(report,stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps({'rows':len(rows),'verified':args.verify,'fixtureSha256':digest(FIXTURE)}))


if __name__=='__main__':main()  # import만으로 원본이나 fixture에 쓰지 않는다.
