#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 생산 실패 안내의 생성/두 wrapper/4초 Idle/Draw/삭제를 실제 PE에서 관찰한다."""
import argparse,csv,itertools,json,socket,struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,STACK,STOP,TEXT,digest,fbits
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG

NODE,OWNED,SOURCE,REFERENCE=TEXT+0x1000,TEXT+0x3000,TEXT+0x4000,TEXT+0x5000  # 서로 겹치지 않는 현재 입력/소유 문자열이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/production-hint-x86.tsv'  # 새 C++ 전 동결하는 실제 필드/효과 원문이다.
REPORT=ROOT/'cpppj/production-hint.json'  # 원본 범위와 명시 TextGump/장치 경계를 보존한다.
ENTRIES=(0x4728d0,0x4729d0,0x472a00,0x472a50,0x472a70,0x472be0,0x472d20,0x465260)  # 실제 파생 몸체와 입력 사각형 setter다.
STRINGS=(b'',b'~0~w~ENot enough Storm Power to build this',b'~0~w~ENone in inventory')  # 명시 번역 입력도 첫 NUL 바이트까지 유지한다.
POINTS=((0,0),(17,-9),(-25,1024),(16777217,-16777217),(2147483647,2147483647),(-2147483648,-2147483648))  # signed 덧셈 감김/float 저장 경계다.


def dbits(value):
    """실제 double 입력의64비트를 저장한다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def inputs():
    """기대 필드를 계산하지 않고 생성/수명/출력/소유권의 명시 입력을 교차한다."""
    rows=[]
    # 기본/번역 문자열과 현재 팔레트/종류 BYTE를 독립 입력으로 둔다.
    for now,text,(white,black,kind) in itertools.product((0.,3.25,1e9),range(3),((7,0,25),(0x81234567,0x12345678,0x80))):rows.append(('C',(dbits(now),text,white,black,kind)))
    # deadline 전/같음/후와 NaN의 unordered 비교를 그대로 관찰한다.
    for until,now in itertools.product((0.,4.,float('nan')),(0.,3.9999999999999996,4.,5.)):rows.append(('I',(dbits(until),dbits(now))))
    # 실제 Draw는 head0에서도 Bounds/Position/StyleDraw 호출을 유지한다.
    for head,(x,y) in itertools.product((0,1),POINTS):rows.append(('D',(head,x&0xffffffff,y&0xffffffff)))
    # 소유 문자열과 scalar 하위 BYTE bit0만 별도로 관찰한다.
    for owned,flags in itertools.product((0,1),(0,1,2,0x100,0x101,0xffffffff)):rows.append(('X',(owned,flags)))
    # 두 안내 wrapper의 위치/기준 사각형/현재 벽시각과 원본 번역을 실행한다.
    for kind,(x,y),now,box in itertools.product((0,1),POINTS,(0.,3.25,1e9),(0,1)):rows.append(('W',(kind,x&0xffffffff,y&0xffffffff,dbits(now),box)))
    return rows


class Oracle(MemoryOracle):
    """TextGump/문자열 heap/시계/Draw/부착만 경계이며 안내 파생의 실제 명령은 실행한다."""
    def __init__(self):
        """기존 격리 PE에 SEH와 실제 파생 함수의 불연속 범위를 준비한다."""
        super().__init__();self.mu.mem_map(0,0x1000);self.calls=Counter();self.returns=0;self.events=[]
        rows={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')};self.allowed=[]
        # 함수 사이의 바이트나 OS 코드는 실행하지 않는다.
        for entry in ENTRIES:
            for part in rows[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def write(self,mu,access,address,size,value,data):
        """객체/소유 문자열/출력 포인터/현재 마지막 일련번호/SEH·스택만 변경할 수 있다."""
        regions=((NODE,NODE+0xb8),(OWNED,OWNED+512),(0x592988,0x59298c),(0,4),(STACK-0x1f000,STACK+0x1000))
        if not any(low<=address and address+size<=high for low,high in regions):raise RuntimeError(f'안내 범위 밖 쓰기 {address:08x}+{size}')

    def event(self,*values):
        """관찰한 효과 인자와 문자열만 원문 순서대로 저장한다."""
        self.events.append(','.join(map(str,values)))

    def instruction(self,mu,address,size,data):
        """실제 안내 명령을 실행하고 각 명시 경계의 호출 순서/인자만 기록한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,6);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if any(low<=address and address+size<=high for low,high in self.allowed):self.instructions+=1;return
        if address==0x4bb170:
            assert args[:4]==(0,0,6,0);self.event('Text');self.put(NODE,0x5144d4);self.put(NODE+4,0x400001);self.put(NODE+0x2c,41);self.put(NODE+0xa0,0);self.ret(NODE,16)
        elif address==0x4e78a5:
            text=self.string(args[0]);self.event('Dup',text.hex());mu.mem_write(OWNED,text+b'\0');self.ret(OWNED)
        elif address==0x4bc800:self.event('Style',self.string(args[0]).hex());self.put(NODE+0xa0,0x12408000);self.ret(purge=4)
        elif address==0x4bb310:
            self.event('Fit',*args[:2]);self.put(NODE+0x3c,fbits(51.25));self.put(NODE+0x40,fbits(13.5));self.put(NODE+0x44,fbits(51.25));self.put(NODE+0x48,fbits(13.5));self.ret(purge=8)
        elif address==0x465330:self.event('Idle',args[0]);self.put(NODE+4,self.words(NODE+4,1)[0]|0x80);self.ret(purge=4)
        elif address==0x460d70:mu.mem_write(args[0],struct.pack('<Q',self.now));self.event('Now');self.ret(args[0])
        elif address==0x466480:self.event('Kill');self.ret()
        elif address==0x4648e0:mu.mem_write(args[0],struct.pack('<4i',*self.box));self.event('Bounds',int(ecx==REFERENCE));self.ret(args[0],4)
        elif address==0x464740:self.put(args[0],self.x);self.put(args[0]+4,self.y);self.event('Position');self.ret(args[0],4)
        elif address==0x4755e0:self.event('Draw',int(args[0]!=0),args[1],args[2],args[3]);self.ret()
        elif address==0x4e4354:self.event('Free',1 if args[0]==OWNED else 2);assert args[0] in (OWNED,NODE);self.ret()
        elif address==0x4bc790:self.event('TextDelete');self.put(NODE,0x50ac70);self.ret()
        elif address==0x4e4391:assert args[0]==0xb8;self.event('New',args[0]);self.ret(NODE)
        elif address==0x4de9a0:self.event('Translate',self.string(args[0]).hex());self.ret(args[0],4)
        elif address==0x466750:
            self.event('Attach',*args[:4]);self.put(NODE+0x4c,args[0]);self.put(NODE+0x50,args[1]);self.ret(purge=16)
        else:raise RuntimeError(f'안내 범위 밖 실행 {address:08x}')

    def snapshot(self):
        """실제 파생이 쓰는 필드와 미작성a4를 기록하고 head/문자열 포인터만 의미 토큰으로 바꾼다."""
        raw=bytes(self.mu.mem_read(NODE,0xb8));parts=[raw[o:o+4] for o in (0,4,0x20,0x2c,0x30,0x3c,0x40,0x44,0x48,0x4c,0x50)]
        parts.extend((raw[0x5c:0x6c],raw[0x80:0xa0],struct.pack('<I',int(self.words(NODE+0xa0,1)[0]!=0)),raw[0xa4:0xb0],struct.pack('<I',int(self.words(NODE+0xb0,1)[0]!=0)),raw[0xb4:0xb8]));return b''.join(parts).hex()

    def sample(self,row,precision):
        """명시 입력만 설치하고 원본의 정상 반환/SEH/스택/비휘발 레지스터를 확인한다."""
        op,v=row;mu=self.mu;mu.mem_write(NODE,b'\xa5'*0xb8);self.put(0,0xffffffff);self.put(0x563e38,REFERENCE);self.put(0x592988,0x76543210);self.put(0x5a4058,7);self.put(0x5bec2c,0);self.put(0x540434,25,1)
        mu.mem_write(SOURCE,STRINGS[1]+b'\0');self.now=dbits(3.25);self.box=(10,20,40,56);self.x=17;self.y=-9;args=();entry=0x4729d0;purge=0;self.events=[]
        if op=='C':
            self.now,text,white,black,kind=v;mu.mem_write(SOURCE,STRINGS[text]+b'\0');self.put(0x5a4058,white);self.put(0x5bec2c,black);self.put(0x540434,kind,1);entry=0x4728d0;args=(SOURCE,);purge=4
        elif op=='I':mu.mem_write(NODE+0xa8,struct.pack('<Q',v[0]));self.now=v[1]
        elif op=='D':self.put(NODE+0xa0,v[0]);self.x,self.y=v[1:];entry=0x472a00
        elif op=='X':self.put(NODE+0xb0,OWNED if v[0] else 0);entry=0x472a50;args=(v[1],);purge=4
        else:
            kind,x,y,self.now,box=v;self.box=(0,0,36,36) if not box else (-17,20,2147483647,-2147483648);entry=0x472be0 if not kind else 0x472d20;args=(x,y,REFERENCE)
        registers={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x3456789a,UC_X86_REG_EBP:0x456789ab}
        # 독립 sentinel과 x87 스택은 매 호출 다시 설치한다.
        for register,value in registers.items():mu.reg_write(register,value)
        for register,value in ((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,NODE),(UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,precision),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):mu.reg_write(register,value)
        mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.emu_start(entry,STOP,count=100000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge and self.words(0,1)==(0xffffffff,)
        assert all(mu.reg_read(register)==value for register,value in registers.items()) and mu.reg_read(UC_X86_REG_FPCW)==precision and not mu.reg_read(UC_X86_REG_FPSW)&0x3800
        self.returns+=1;return self.snapshot(),str(self.words(0x592988,1)[0]),';'.join(self.events) or '-'


def main():
    """새 원문 생성과 기존 SHA/입력 감사 중 하나만 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:
        r=json.loads(REPORT.read_text('utf-8'))
        # 기대 출력은 다시 계산하지 않는다.
        for name,wanted in r['sha256'].items():assert digest(ROOT/name)==wanted,name
        rows=[row for row in FIXTURE.read_text('utf-8').splitlines() if row and not row.startswith('#')];assert len(rows)==r['rows']==len(inputs());print('생산 실패 안내 SHA/입력 감사',len(rows));return
    oracle=Oracle();rows=[]
    # 두 정밀도의 차이가 있으면 입력을 제거하지 않고 먼저 멈춘다.
    for row in (inputs()[:1] if args.smoke else inputs()):
        first=oracle.sample(row,0x27f);second=oracle.sample(row,0x37f);assert first==second,row;rows.append((row[0],','.join(map(str,row[1])),*first))
    if args.smoke:print('생산 실패 안내 실제 생성 smoke 정상 반환');return
    FIXTURE.write_text('# op inputs fields116 lastSerial effects\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_production_hint.py',FIXTURE.relative_to(ROOT).as_posix())
    report=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',unit=181,rows=len(rows),normalReturns=oracle.returns,instructions=oracle.instructions,counts=dict(Counter(row[0] for row in rows)),precisionDifferences=0,nativeCalls=dict(oracle.calls),sha256={name:digest(ROOT/name) for name in paths},limits=['실제 안내 생성/4초 Idle/Draw/소멸과 두 안내 wrapper/입력 사각형 setter다. TextGump/문자열 heap·번역/시계/측정/그리기/가상 부착·삭제는 명시 경계다.','TextGump 경계는 기존 실제 모듈로 연결하며 새 원문은 파생 필드/효과의 순서와 원본 덧셈·단정도 저장을 비교한다. 포인터는 head/소유 문자열의 비영 의미만 정규화한다.','10.78만 관측하며 원본 게임/창/OS 실행·보호 파일 변경·새 디컴파일은 없다. 전체 생산 패널/미션 자연 완주 증명은 아니다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('생산 실패 안내 독립 원본',len(rows),oracle.returns,oracle.instructions)


if __name__=='__main__':main()
