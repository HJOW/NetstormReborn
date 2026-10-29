"""원본 Chat 서식 명령과 실제 Regexp·스타일 슬롯 제거를 격리 CPU에서 관측한다."""
import argparse
import csv
import json
import socket
from pathlib import Path
from decomp_chat_recipients_regex import NativeRegexOracle
from decomp_priestmenu import ROOT, POOL, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

MANAGER_A, MANAGER_B = POOL+0x1000, POOL+0x4000  # 명시 두 manager의 실제 물리 토큰이다.
CURRENT, SUBSCRIBER, REGEX, CAPTURE = 0x548714, 0x540c74, 0x548724, 0x59a830  # 원문 현재 전역과 공통 캡처의 앞81BYTE다.
WIDTH, CAPTURE_BYTES = 2712, 81  # 원문 manager와 최대80BYTE 캡처+NUL의 관측 폭이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/chat-format-x86.tsv'  # 제품을 작성하기 전에 동결하는 독립 원문 출력이다.
REPORT = ROOT/'cpppj/chat-format-native-20261030.json'  # 실제 실행 범위·입력·장치 경계와 해시다.
SCHEMA = ('front','source','mode','status','current','subscriber','managerA','managerB','capture','trace')  # 입력과 실제 쓰기·장치 순서를 분리한다.


def cases():
    """스타일 검색·네 공백·80/60BYTE 경계와 현재 전역/공통 캡처의 변경을 입력으로 준다."""
    sources = [b'',b' ',b'plain',b'~',b'~~',b'~r',b'~rRed',b'junk~rRed suffix~gGreen',b'~rRed ~gGreen',
               b'~r\r~g',b'~r\n~g',b'~r\t~g',b'~r\v~g',b'~r\f~g',b'~r\xa0~g',b'~0',b'~0~1~2~3~4~5~6',
               b'~7~8~9',b'~r~0Red~6~gGreen',b'prefix~0~rRest',b'~r\0~gignored',b'~[C.12]~B',
               b'~r'+b'X'*57,b'~r'+b'X'*58,b'~r'+b'X'*59,b'~r'+b'X'*77,b'~r'+b'X'*78,b'~r'+b'X'*79,
               b'~r'+b'X'*78+b'~0',b'~0'+b'X'*100,b'~r'+b'X'*150]
    for front in (0,1):  # format/back와 front의 목적 필드·키를 각각 실행한다.
        for source in sources:  # 원문 고정 문자열과 길이 경계를 같은 순서로 교차한다.
            for mode in ('normal','copySwitch','stopCopy','stopConfig','assertRestore','assertStop'):  # 정상·현재값 변경·부분 중단을 별도 입력으로 준다.
                yield front,source.hex() or '-',mode


class Oracle(NativeRegexOracle):
    """두 명령·슬롯 제거·실제 검색/캡처만 실행하고 CRT/설정/진단은 명시 장치로 둔다."""
    def __init__(self):
        """보호 PE의 동일 Regexp 생성자로00504a00 패턴을 별도 원문 객체에 컴파일한다."""
        self.mode='normal'  # 컴파일 시에는 장치 변경이 없는 기본 관측 모드를 사용한다.
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):  # 새로 허용한 세 본체의 실제 범위만 추가한다.
                if int(row['entry'],16) in (0x42ddf0,0x42def0,0x42d160):
                    self.allowed.extend((int(a,16),int(b,16)+1) for a,b in (part.split('-') for part in row['ranges'].split(';')))
        assert self.string(0x504a00)==b'(~[^~\r\t\n ]+)+'
        self.put(STACK,STOP);self.put(STACK+4,0x504a00)
        self.mu.reg_write(UC_X86_REG_ECX,REGEX);self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.emu_start(0x496c90,STOP,count=1000000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP

    def alias(self,value):
        """두 유효 manager와0만 주소 독립 토큰으로 표현한다."""
        if value==0:return 0
        if value==MANAGER_A:return 1
        if value==MANAGER_B:return 2
        raise RuntimeError('예상 밖 manager '+hex(value))

    def instruction(self,mu,address,size,data):
        """원문 분기/복사를 실행하고 선택한 CRT/설정 경계의 부분효과와 순서만 제공한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,5)
        if address==0x42d160:
            assert mu.reg_read(UC_X86_REG_EAX)==CAPTURE
            self.trace.append('strip:'+bytes(mu.mem_read(CAPTURE,CAPTURE_BYTES)).hex())
        if address==0x4e0620:
            self.trace.append('assert:'+str(args[2]))
            if self.mode=='assertStop':raise InterruptedError('assertion')
            assert self.mode=='assertRestore'
            self.put(CURRENT,MANAGER_A);self.ret(0);return
        if address==0x4e51f0:
            target,source,count=args[:3]
            assert source==CAPTURE and count in (60,500)
            base=MANAGER_A if MANAGER_A<=target<MANAGER_A+WIDTH else MANAGER_B
            self.trace.append('copy:'+str(self.alias(base))+':'+str(target-base)+':'+str(count))
            text=self.string(source);mu.mem_write(target,text[:count].ljust(count,b'\0'))
            if self.mode=='copySwitch':
                self.put(CURRENT,MANAGER_B);self.put(SUBSCRIBER,0xffffffff);mu.mem_write(CAPTURE,b'~gNEXT\0')
            if self.mode=='stopCopy':raise InterruptedError('copy')
            self.ret(target);return
        if address==0x4e58e9:
            target,pattern,subscriber,suffix=args[:4]
            assert self.string(pattern)==b'%d%s'
            signed=subscriber if subscriber<0x80000000 else subscriber-0x100000000
            text=str(signed).encode('ascii')+self.string(suffix)
            mu.mem_write(target,text+b'\0');self.trace.append('key:'+text.hex());self.ret(len(text));return
        if address==0x4414d0:
            self.trace.append('config:'+self.string(args[0]).hex()+':'+self.string(args[1]).hex())
            if self.mode=='stopConfig':raise InterruptedError('config')
            self.ret(0);return
        if address==0x4c2630:
            assert args[0]==29 and self.string(args[1])==b'Illegal format "%s"\n'
            self.trace.append('error:'+self.string(args[2]).hex());self.ret(0);return
        super().instruction(mu,address,size,data)

    def sample(self,case):
        """두 manager/공통 캡처/스택을 독립 입력으로 되돌리고 정상/선택 중단을 구별한다."""
        front,source,self.mode=case
        for number,token in enumerate((MANAGER_A,MANAGER_B)):  # 두 전체 물리를 서로 다른 비영 패턴으로 구성한다.
            self.mu.mem_write(token,bytes((0xa5+number*11+index*17)&255 for index in range(WIDTH)))
        self.mu.mem_write(CAPTURE,bytes([0xd3])*CAPTURE_BYTES)
        self.mu.mem_write(TEXT,bytes.fromhex('' if source=='-' else source)+b'\0')
        self.put(CURRENT,0 if self.mode.startswith('assert') else MANAGER_A);self.put(SUBSCRIBER,100)
        self.mu.mem_write(STACK-0x1000,bytes([0xa5])*0x1100);self.put(STACK,STOP);self.put(STACK+4,TEXT)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):
            self.mu.reg_write(register,0x11223344)  # 호출 사이의 보존/임시 레지스터를 새 입력으로 되돌린다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.trace=[];status='normal'
        try:self.mu.emu_start(0x42def0 if front else 0x42ddf0,STOP,count=1000000)
        except InterruptedError as error:status=str(error)
        if status=='normal':
            assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
            assert self.mu.reg_read(UC_X86_REG_EAX)==0
        return dict(zip(SCHEMA,(*case,status,self.alias(self.words(CURRENT,1)[0]),self.words(SUBSCRIBER,1)[0],
                               bytes(self.mu.mem_read(MANAGER_A,WIDTH)).hex(),bytes(self.mu.mem_read(MANAGER_B,WIDTH)).hex(),
                               bytes(self.mu.mem_read(CAPTURE,CAPTURE_BYTES)).hex(),';'.join(self.trace) or '-')))


def main():
    """최초 결과는 새 파일에만 쓰고 verify는 같은 원문 실행으로 기존 전체 출력을 대조한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    assert socket.gethostname().lower()=='hjow-athlon'
    assert digest(ROOT/'originals/Netstorm.exe')=='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'
    assert digest(ROOT/'extracted/refined/originals/Netstorm.c')=='af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646'
    oracle=Oracle();rows=[oracle.sample(case) for case in cases()]
    content='\t'.join(SCHEMA)+'\n'+''.join('\t'.join(str(row[key]) for key in SCHEMA)+'\n' for row in rows)
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8')==content
    else:
        with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:stream.write(content)
        report={'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','rows':len(rows),
                'sources':{path.relative_to(ROOT).as_posix():digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/Netstorm.c',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__))},
                'fixtureSha256':digest(FIXTURE),'actualNative':['0042ddf0','0042def0','0042d160','00495a10','00496c90/004964e0 이하 엔진'],
                'explicitDevices':['malloc/C locale CRT','strncpy/sprintf','Config.Set/진단','현재 manager/subscriber·공통 캡처 변경·선택 중단'],
                'gameOrNetworkExecuted':False,'limits':['두 서식 명령의 원문 부분효과이며 전체 채팅 송신/게임/GUI 검증이 아님']}
        with REPORT.open('x',encoding='utf-8',newline='\n') as stream:json.dump(report,stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps({'rows':len(rows),'verified':args.verify,'fixtureSha256':digest(FIXTURE)}))


if __name__=='__main__':main()  # import는 fixture와 보호 파일에 쓰지 않는다.
