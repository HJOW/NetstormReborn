#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""생산 항목 우클릭 정보43d330의 실제 비용/허용/환불과 메뉴 호출을 별도 동결한다."""
import argparse,csv,itertools,json,socket,struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,POOL,TYPES,TEXT,STACK,STOP,digest,fbits
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG

NODE,MENU,HELP=TEXT+0x1000,TEXT+0x2000,TEXT+0x3000  # 실제 호출의 원문 노드/controlled 메뉴/도움말 문자열 경계다.
ENTRIES=(0x43d330,0x4af010,0x49aa30,0x43cb30,0x4e49c0)  # 메뉴 본체/현재 비용·SP 비교/환불/실제_ftol을 실행한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/inventory-about-x86.tsv'  # 새 C++ 전에 동결할 효과/문자열/반환 원문이다.
REPORT=ROOT/'cpppj/inventory-about.json'  # 실제 실행과 메뉴·조회 경계를 명시한다.


def inputs():
    """일반/회수 금지/전액/빈 종류·다이스와 정수 비용/NaN SP/NULL 도움말을 교차한다."""
    return list(itertools.product((82,122,128,154,164,165),(8,0x20,0x28),(0,1),(-1,0,400,33554431),(fbits(0),fbits(399),fbits(float('nan'))),(0,1,2)))


class Oracle(MemoryOracle):
    """게임 계산은 실제 PE이며 메뉴/번역·정보 작성/위치·음향/현재 SP만 외부 입력이다."""
    def __init__(self):
        """검토 함수와 명시 노드/현재 비용·SP 주소를 준비한다."""
        super().__init__();self.allowed=[];self.calls=Counter();self.returns=0
        rows={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        # 실제 불연속 함수 구간을 각각 허용한다.
        for entry in ENTRIES:
            for part in rows[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def event(self,*values):
        """실제 효과와 현재 바이트 문자열을 순서대로 보존한다."""
        self.events.append(','.join(map(str,values)))

    def instruction(self,mu,address,size,data):
        """실제 정수/부동소수 계산은 계속 실행하며 명시 메뉴 경계만 반환한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,6);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if any(low<=address and address+size<=high for low,high in self.allowed):self.instructions+=1;return
        if address==0x477ae0:self.event('Begin');self.ret(MENU)
        elif address==0x465330:self.event('Idle',args[0]);assert ecx==MENU;self.ret(purge=4)
        elif address==0x4644e0:self.event('Object',args[0]);assert ecx==MENU;self.ret(purge=4)
        elif address==0x4ad780:self.event('Describe',*args[:4]);self.ret()
        elif address==0x4de9a0:self.event('Translate',self.string(args[0]).hex());self.ret(args[0],4)
        elif address==0x4a89c0:
            pattern=self.string(args[1]).decode('ascii');number=struct.unpack('<i',struct.pack('<I',args[2]))[0];assert pattern.count('%d')==1;mu.mem_write(args[0],(pattern%number).encode()+b'\0');self.ret(args[0])
        elif address==0x478020:self.event('Header',self.string(args[0]).hex());self.ret()
        elif address==0x49a840:self.ret(TYPES+args[0]*500)
        elif address==0x40ec50:self.event('Money',args[0]);self.put(args[1],self.money);self.ret()
        elif address==0x477cb0:self.event('Entry',self.string(args[0]).hex() if args[0] else '-',*args[1:6]);self.ret()
        elif address==0x475ca0:self.event('Attach',*args[:3]);self.ret()
        elif address==0x4a9cb0:self.event('Sound',self.string(args[0]).hex(),*args[1:6]);self.ret()
        else:raise RuntimeError(f'항목 정보 범위 밖 실행 {address:08x}')

    def sample(self,values,precision):
        """명시 입력만 설치하고 전체 정상 반환/비휘발/x87 균형과 raw 보존을 확인한다."""
        kind,flags,editor,cost,self.money,helpKind=values;mu=self.mu;mu.mem_write(NODE,bytes(0xc0));mu.mem_write(POOL+50*50,bytes(50));self.put(NODE+0xa0,50);self.put(NODE+0x24,314159);self.put(POOL+50*50+18,kind,2);self.put(POOL+50*50+20,flags,2)
        self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES);self.put(0x59ab50+kind*4,cost+kind*23);self.put(0x5c85a4,editor);self.put(0x540c70,1);self.put(0x540cac,2);self.put(0x541240,122);self.put(0x5412e8,164);self.put(0x5412ec,165);self.put(0x5412c0,154)
        mu.mem_write(HELP,(b'Help' if helpKind==2 else b'')+b'\0');self.put(0x54db38,HELP if helpKind else 0);self.put(0x542754,0x02000017);self.put(0x54286c,0x0200005d);self.put(0x563e38,MENU);self.events=[]
        before=bytes(mu.mem_read(NODE,0xc0))+bytes(mu.mem_read(POOL+50*50,50));registers={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x3456789a,UC_X86_REG_EBP:0x456789ab}
        # 각 호출을 독립 레지스터/FPU 입력으로 시작한다.
        for register,value in registers.items():mu.reg_write(register,value)
        for register,value in ((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,NODE),(UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,precision),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):mu.reg_write(register,value)
        mu.mem_write(STACK,struct.pack('<3I',STOP,17,0xfffffff7));mu.emu_start(0x43d330,STOP,count=100000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+12 and mu.reg_read(UC_X86_REG_EAX)==1
        assert all(mu.reg_read(register)==value for register,value in registers.items()) and mu.reg_read(UC_X86_REG_FPCW)==precision and not mu.reg_read(UC_X86_REG_FPSW)&0x3800
        assert before==bytes(mu.mem_read(NODE,0xc0))+bytes(mu.mem_read(POOL+50*50,50));self.returns+=1;return ';'.join(self.events)


def main():
    """새 원문 생성과 기존 SHA 감사를 구별한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:
        r=json.loads(REPORT.read_text('utf-8'))
        # 과거 기대값을 새 계산으로 덮어쓰지 않는다.
        for name,wanted in r['sha256'].items():assert digest(ROOT/name)==wanted,name
        rows=[row for row in FIXTURE.read_text('utf-8').splitlines() if row and not row.startswith('#')];assert len(rows)==r['rows']==len(inputs());print('항목 정보 SHA/입력 감사',len(rows));return
    oracle=Oracle();rows=[]
    # 두 정밀도의 실제 원문이 다르면 입력을 제거하지 않고 멈춘다.
    for values in (inputs()[:1] if args.smoke else inputs()):
        first=oracle.sample(values,0x27f);second=oracle.sample(values,0x37f);assert first==second,values;rows.append((','.join(str(value&0xffffffff) for value in values),first))
    if args.smoke:print('항목 우클릭 실제 정보/비용 smoke 정상 반환');return
    FIXTURE.write_text('# kind flags editor cost money helpKind / actual ordered effects\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_inventory_about.py',FIXTURE.relative_to(ROOT).as_posix())
    report=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',unit=181,rows=len(rows),normalReturns=oracle.returns,instructions=oracle.instructions,precisionDifferences=0,nativeCalls=dict(oracle.calls),sha256={name:digest(ROOT/name) for name in paths},limits=['실제43d330/4af010/49aa30/43cb30/_ftol의 비용·허용·환불과 조건/현재 필드 읽기를 실행한다. 메뉴/번역·정보 작성/순수 서식/현재 SP/위치·음향은 명시 경계다.','NULL/빈/Help와6종류·회수flags·편집/정수 큰 비용·NaN SP를 교차한다. 실제 메뉴/Session/GUI 연결은 별도 검사이며 CD/전체 게임 동일성의 증거는 아니다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('항목 정보 독립 원본',len(rows),oracle.returns,oracle.instructions)


if __name__=='__main__':main()
