"""고정 채팅 Regexp/부모와 실제 PlayerFinder를 함께 실행하여 이름 경계의 최초 기대값을 만든다."""
import csv
import json
import socket
from pathlib import Path
from decomp_chat_recipients_regex import NativeRegexOracle
from decomp_chat_recipients import ROOT, TEXT, REGEX, digest
from decomp_priestmenu import PLAYERS
from unicorn.x86_const import UC_X86_REG_ECX

FIXTURE=ROOT/'cpppj/tests/fixtures/chat-recipient-identity-x86.tsv'  # 이전350행을 바꾸지 않는 실제 조회 접합 관측이다.
REPORT=ROOT/'cpppj/chat-recipient-identity-native-20261029.json'  # GUI/통신 없는 명시 기록표의 원문 관측이다.
SCHEMA=('scene','text','result','scratch','trace','instructions')  # 기대값은 실제 CPU 실행 뒤에만 기록한다.


def cases():
    """로컬·일반 전투·편집 조회를39/40/41BYTE와 실제40BYTE 이름을 가진 채팅 행에 교차한다."""
    texts=[b'',b'LocalUser',b'localuser',b'Alice',b'Bob',b'Alice Bob',b'ALLIES Alice',b'A'*39,b'A'*40,b'A'*41,b'B'*40,b'B'*40+b' ALLIES',b'B'*41+b' Unknown',b'\r'+b'A'*40+b'\nBob',b'Bob\v',b'ALLIES\0Unknown']
    for scene in range(3):  #3은 별도의 메타맵/네트워크가 아니라 원문 모드0/전투1/편집2다.
        for text in texts:yield scene,text.hex() or '-'


class JoinedOracle(NativeRegexOracle):
    """부모·정규식·실제4841c0 모두 실행하며 문자열 CRT/번역/할당만 외부 장치다."""
    def __init__(self):
        """기존 regex 생성과 분리된 실제 PlayerFinder 범위를 추가한다."""
        super().__init__();self.identity=[]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):  #단일 실제 Resolve의 명령만 더 허용한다.
                if int(row['entry'],16)==0x4841c0:self.identity.extend((int(a,16),int(b,16)+1) for a,b in (part.split('-') for part in row['ranges'].split(';')))

    def instruction(self,mu,address,size,data):
        """Identity 진입 이름만 관측하고 원문 본문을 대체하지 않는다."""
        if any(low<=address and address+size<=high for low,high in getattr(self,'identity',[])):
            if address==0x4841c0:self.trace.append('R:'+self.string(mu.reg_read(UC_X86_REG_ECX)+8).hex())
            self.instructions+=1;return
        super().instruction(mu,address,size,data)

    def joined(self,scene,text):
        """결과를 계산하지 않고 원문 Client/Player/Chat 표와 이름을 명시 입력으로 채운다."""
        self.put(0x59534c,PLAYERS);self.mu.mem_write(PLAYERS,bytes(9*180));self.mu.mem_write(0x558ed0,bytes(4800));self.put(0x540c70,1);self.put(0x540c74,100);self.put(0x5953fc,0);self.put(0x540ca8,TEXT+0xc000);self.mu.mem_write(TEXT+0xc000,b'LocalUser\0');self.put(0x594fbc,scene==1);self.put(0x5c85a4,scene==2);self.put(0x594fc8,0)
        for owner,name,subscriber in ((1,b'LocalUser',100),(2,b'Alice',200)):  #원문180BYTE Player 이름과Subscriber만 쓴다.
            self.mu.mem_write(PLAYERS+owner*180+12,name+b'\0');self.put(PLAYERS+owner*180+44,subscriber)
        for index,name,subscriber in ((0,b'Bob',300),(1,b'B'*40,400)):  #두 번째 행의40글자는flags0에NUL이 이어지는 실제 물리다.
            self.mu.mem_write(0x558ed0+index*48,name+b'\0');self.put(0x558ed0+index*48+44,subscriber)
        row=self.sample((1,text,0));assert row['status']=='normal';return dict(zip(SCHEMA,(scene,text,row['result'],row['scratch'],row['trace'],row['instructions'])))


def main():
    """최초 원문 fixture를 별도로 고정하고 이후 실행은 같은 바이트를 재검증한다."""
    oracle=JoinedOracle();rows=[oracle.joined(*case) for case in cases()]
    paths=(ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/NETSTORM.c',ROOT/'extracted/refined/originals/functions.tsv',ROOT/'tools/decomp_chat_recipients.py',ROOT/'tools/decomp_chat_recipients_regex.py',Path(__file__))
    report={'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','rows':len(rows),'instructions':sum(row['instructions'] for row in rows),'native':'채팅 두 부모·원본 Regexp 생성/검색/캡처·초기화/재초기화·실제004841c0','explicitDevices':['malloc','C locale CRT','번역'],'sources':{str(p.relative_to(ROOT)).replace('\\','/'):digest(p) for p in paths},'gameOrNetworkExecuted':False}
    if FIXTURE.exists() or REPORT.exists():
        with FIXTURE.open(encoding='utf-8',newline='') as stream:expected=list(csv.DictReader(stream,delimiter='\t'))
        assert [{key:str(value) for key,value in row.items()} for row in rows]==expected;report['fixtureSha256']=digest(FIXTURE);assert json.loads(REPORT.read_text(encoding='utf-8'))==report
    else:
        with FIXTURE.open('w',encoding='utf-8',newline='') as stream:writer=csv.DictWriter(stream,SCHEMA,delimiter='\t',lineterminator='\n');writer.writeheader();writer.writerows(rows)
        report['fixtureSha256']=digest(FIXTURE);REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'joinedNativeRows':len(rows),'differences':0}))


if __name__=='__main__':main()
