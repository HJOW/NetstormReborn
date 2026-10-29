"""채팅 대상 이름 검사 부모를 보호 PE에서 격리 관측한다. 게임·통신·OS는 실행하지 않는다."""
import argparse
import csv
import json
import re
import socket
from collections import Counter
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

FIXTURE=ROOT/'cpppj/tests/fixtures/chat-recipients-x86.tsv'  # 최초 부모 관측을 제품과 독립해서 보존한다.
REPORT=ROOT/'cpppj/chat-recipients-native-20261029.json'  # 실제 CPU와 명시 하위 장치의 범위를 기록한다.
PATTERN=b'^[\r\n\t ]*([^\r\n\t ]+)(.*)'  # PE 005049e8은 실제 CR/LF/TAB 바이트를 담는다. 범용 Regexp 구현을 주장하지 않는다.
REGEX=0x548730; CAPTURE=TEXT+0x8000; TRANSLATED=TEXT+0x9000  # 원문 전역과 격리 캡처/번역 저장소다.
SCHEMA=('manager','text','mode','status','result','scratch','trace','instructions')  # 입력과 실제 원시 출력을 분리한다.


def cases():
    """NULL/빈 입력·정확한 네 구분자·40BYTE 복사·현재 번역/부분중단을 독립 교차한다."""
    texts=[None,b'',b' \t\r\n',b'Alice',b'ALICE',b'Unknown',b'EVERYONE',b'everyone',b'ALLIES',b'BATTLE',b'OBSERVER',b'Alice EVERYONE',b'Alice Unknown',b'Unknown Alice',b'\t Alice\r\nALLIES \n',b'ALLIES\v',b'ALLIES\f',b'A'*39,b'A'*40,b'A'*41,b'Alice\0Unknown',b'Tous',b'Inconnus Tous',b'\xa0',b'Alice\rUnknown']
    for manager in (0,1):  # manager0에서 입력/하위 자체가 선택되지 않는 관문이다.
        for text in texts:
            for mode in range(7):
                yield manager,'~' if text is None else text.hex() or '-',mode


class RecipientOracle(MemoryOracle):
    """부모·초기화/재초기화·캡처/오프셋은 실제 CPU이며 검색/Identity/번역/CRT만 명시 장치다."""
    def __init__(self):
        """같은 호스트 정밀 함수표에서 허용 몸체의 정확한 범위만 읽는다."""
        super().__init__();self.allowed=[];self.scratch=0;self.trace=[];self.counts=Counter();self.mode=0
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):  # 실제 선택한 여섯 몸체 외 진입은 거부한다.
                if int(row['entry'],16) in (0x42dc70,0x42d910,0x484190,0x4841b0,0x495a10,0x495a80):
                    self.allowed.extend((int(a,16),int(b,16)+1) for a,b in (part.split('-') for part in row['ranges'].split(';')))

    def instruction(self,mu,address,size,data):
        """하위 응답은 입력으로만 주고 반환·복사·비교 순서·부분효과는 원문에서 읽는다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX);args=self.words(sp+4,5)
        if address==STOP:mu.emu_stop();return
        if address==0x484190:self.scratch=this
        if address==0x4964e0:
            source=args[0];match=re.match(PATTERN,self.string(source),re.DOTALL);mu.mem_write(CAPTURE,bytes(80));self.put(REGEX,CAPTURE);self.put(REGEX+4,int(bool(match)));self.put(REGEX+8,source)
            if match:
                for group in range(3):  # 실제 capture helper가 소비하는 포인터만 제공한다.
                    self.put(CAPTURE+4*group,source+match.start(group));self.put(CAPTURE+40+4*group,source+match.end(group))
            self.ret(int(bool(match)),4);return
        if address==0x4841c0:
            self.counts['resolve']+=1;name=self.string(this+8);self.trace.append('R:'+name.hex());self.counts['calls']+=1
            if self.mode==5 and self.counts['resolve']==2:raise InterruptedError('resolve')
            if self.mode==4 and name==b'Unknown':mu.mem_write(this+8,b'EVERYONE\0')
            accepted=self.mode==2 or (self.mode!=1 and name.lower()==b'alice')
            self.ret(int(accepted));return
        if address==0x4de9a0:
            key=self.string(args[0]);self.trace.append('T:'+key.hex())
            if self.mode==6:raise InterruptedError('translate')
            value=b'Tous' if self.mode==3 and key==b'EVERYONE' else key;mu.mem_write(TRANSLATED,value+b'\0');self.ret(TRANSLATED,4);return
        if address==0x4e65ed:
            first,second=self.string(args[0]).lower(),self.string(args[1]).lower();self.ret(int(first>second)-int(first<second));return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'미선언 원문 명령 {address:08x}')

    def sample(self,case):
        """명시 stack잔여·비영 보존 레지스터로 실행하고 정상/선택중단을 구별한다."""
        manager,text,self.mode=case;self.trace=[];self.counts=Counter();self.scratch=0;self.instructions=0;self.mu.mem_write(STACK-0x1000,bytes([0xa5])*0x1100)
        self.put(0x548714,manager);self.mu.mem_write(TEXT,(b'' if text=='~' else bytes.fromhex('' if text=='-' else text))+b'\0');self.put(STACK,STOP);self.put(STACK+4,0 if text=='~' else TEXT)
        saved={UC_X86_REG_EBX:0x11223344,UC_X86_REG_ESI:0x22334455,UC_X86_REG_EDI:0x33445566,UC_X86_REG_EBP:0x44556677}
        for register,value in saved.items():self.mu.reg_write(register,value)  # 실제 부모의 보존 계약을 별도로 검사한다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK);status='normal'
        try:self.mu.emu_start(0x42dc70,STOP,count=100000)
        except InterruptedError as error:status=str(error)
        if status=='normal':
            assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
            assert all(self.mu.reg_read(register)==value for register,value in saved.items())
        return dict(zip(SCHEMA,(*case,status,self.mu.reg_read(UC_X86_REG_EAX) if status=='normal' else '-',bytes(self.mu.mem_read(self.scratch,60)).hex() if self.scratch else '-', ';'.join(self.trace) or '-',self.instructions)))


def main():
    """최초 관측은 새 파일에만 쓰며 verify는 기존 입력/출력/원문 해시를 다시 대조한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args();oracle=RecipientOracle();rows=[oracle.sample(case) for case in cases()]
    assert oracle.string(0x5049e8)==PATTERN
    keys=[oracle.words(0x531768+4*index,1)[0] for index in range(9)];assert [oracle.string(p).decode('ascii') if p else None for p in keys]==['','','','BATTLE','','EVERYONE','ALLIES','OBSERVER',None]
    protected={str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/NETSTORM.c',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__))}
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'));assert protected==report['sources'];assert digest(FIXTURE)==report['fixtureSha256']
        with FIXTURE.open(encoding='utf-8',newline='') as stream:expected=list(csv.DictReader(stream,delimiter='\t'))
        assert [{key:str(value) for key,value in row.items()} for row in rows]==expected
        print(json.dumps({'verified':len(rows),'differences':0}));return
    if FIXTURE.exists() or REPORT.exists():raise FileExistsError('최초 관측 파일은 덮어쓰지 않는다')
    with FIXTURE.open('w',encoding='utf-8',newline='') as stream:writer=csv.DictWriter(stream,SCHEMA,delimiter='\t',lineterminator='\n');writer.writeheader();writer.writerows(rows)
    REPORT.write_text(json.dumps({'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','rows':len(rows),'status':dict(Counter(row['status'] for row in rows)),'instructions':sum(row['instructions'] for row in rows),'sources':protected,'fixtureSha256':digest(FIXTURE),'native':['42dc70','42d910','484190','4841b0','495a10','495a80'],'explicitDevices':['fixed regex search (DOTALL)','Identity result/name mutation','translation','C-locale stricmp'],'gameOrNetworkExecuted':False},ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps({'observed':len(rows)}))


if __name__=='__main__':main()
