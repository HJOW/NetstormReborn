"""채팅 고정 Regexp의 실제 컴파일러/검색까지 실행해350개 부모 관측을 다시 대조한다."""
import csv
import json
import socket
from pathlib import Path
from decomp_chat_recipients import RecipientOracle, cases, FIXTURE, ROOT, TEXT, STACK, STOP, REGEX, digest
from decomp_priestmenu import POOL
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

OUTPUT=ROOT/'cpppj/chat-recipients-regex-native-20261029.json'  # 최초 간이 검색 장치와 별개의 보강 근거다.


class NativeRegexOracle(RecipientOracle):
    """원문 regex 전역/컴파일/검색을 실제 실행하고 malloc/CRT 바이트 함수만 장치로 둔다."""
    def __init__(self):
        """같은 보호 이미지의 고정 regex를 실제 생성자에서 한 번 컴파일한다."""
        super().__init__();self.engine=[];self.heap=POOL+0x20000;self.allocations=[]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):  # 검토한 엔진 함수의 실제 명령 범위만 허용한다.
                if 0x495ae0<=int(row['entry'],16)<=0x496c90 or int(row['entry'],16)==0x4964e0:
                    self.engine.extend((int(a,16),int(b,16)+1) for a,b in (part.split('-') for part in row['ranges'].split(';')))
        self.instructions=0;self.put(STACK,STOP);self.put(STACK+4,0x5049e8);self.mu.reg_write(UC_X86_REG_ECX,REGEX);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.emu_start(0x496c90,STOP,count=1000000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+8
        assert self.words(REGEX,1)[0] and len(self.allocations)==1;self.compileInstructions=self.instructions

    def instruction(self,mu,address,size,data):
        """실제 엔진은 상위의 Python regex 대체를 통과하지 않게 직접 허용한다."""
        if any(low<=address and address+size<=high for low,high in self.engine):self.instructions+=1;return
        sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,3)
        if address==0x43fa80:
            width=args[0];allocated=self.heap;self.heap+=(width+15)&~15;assert self.heap<POOL+0x100000;self.allocations.append(width);self.ret(allocated);return
        if address==0x4e63e0:
            value=self.string(args[0])+b'\0';found=value.find(bytes([args[1]&255]));self.ret(0 if found<0 else args[0]+found);return
        if address==0x4e7054:
            byte=args[0]&255;self.ret(int(48<=byte<=57 or 65<=byte<=90 or 97<=byte<=122));return
        if address==0x4e7300:
            first=self.string(args[0])[:args[2]];second=self.string(args[1])[:args[2]];self.ret(int(first>second)-int(first<second));return
        super().instruction(mu,address,size,data)


def main():
    """이미 고정된350개 기대값을 수정하지 않고 실제regex 실행의 결과만 비교한다."""
    oracle=NativeRegexOracle();rows=[oracle.sample(case) for case in cases()]
    with FIXTURE.open(encoding='utf-8',newline='') as stream:expected=list(csv.DictReader(stream,delimiter='\t'))
    differences=[]
    for index,(actual,wanted) in enumerate(zip(rows,expected)):  # 실제 엔진 명령 증가를 제외한 입력/전체 출력/순서를 비교한다.
        if any(str(actual[key])!=wanted[key] for key in wanted if key!='instructions'):differences.append(index)
    assert len(rows)==len(expected)==350 and not differences, differences
    report={'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','rows':len(rows),'differences':differences,'compilerInstructions':oracle.compileInstructions,'runInstructions':sum(row['instructions'] for row in rows),'allocations':oracle.allocations,'native':'00496c90/00496b20 컴파일러와 실제004964e0 이하 엔진·상위 두 부모·캡처/초기화','explicitDevices':['malloc','C locale strchr/isalnum/strncmp/stricmp','Identity 응답/이름 변경','번역'],'sources':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/NETSTORM.c',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__),ROOT/'tools/decomp_chat_recipients.py',FIXTURE)},'gameOrNetworkExecuted':False}
    if OUTPUT.exists():assert json.loads(OUTPUT.read_text(encoding='utf-8'))==report
    else:OUTPUT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'verifiedWithNativeRegex':len(rows),'differences':len(differences)}))


if __name__=='__main__':main()
