#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Quick Help 요청004c5c00과 실제 C 로케일 CRT를 격리 실행해 선택·이력·부착 순서를 동결한다."""
import argparse,csv,itertools,json,socket,struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,TEXT,STACK,STOP,digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG

NODE,TOPIC,VT,TLS,LOCALE=TEXT+0x1000,TEXT+0x2000,TEXT+0x4000,TEXT+0x8000,TEXT+0x9000 # 실제 노드/주제/가상 표와 C 로케일의 명시 외부 입력이다.
PUSH,ATTACH=STOP+0x100,STOP+0x200 # 가상 Push/Attach의 명시 장치 경계다.
FIXTURE=ROOT/'cpppj/tests/fixtures/quick-help-request-x86.tsv' # 새 C++ 전에 동결하는 원문이다.
REPORT=ROOT/'cpppj/quick-help-request.json' # 원본 실행 범위/명시 경계를 보관한다.
ENTRIES=(0x4c5c00,0x4e65ed,0x4ef510) # 실제 요청과 stricmp의 C 로케일 계산이다.


def inputs():
    """접미 제거 순서·대소문자·마침표 보존·최대79바이트·세 lookup과 직접 TYPE/현재 가상 부착을 교차한다."""
    names=(b'sunCannon',b'cannonHelp',b'CANNONhElPtYpE',b'cannonTypeHelp',b'rainBatteryType',b'researchHelp',b'help',b'helptype',b'typehelp',b'sunCannon.type',b'windArcherHelp',b'a'*75+b'Help',b'cannonHelp\0ignored',b'\xffcannonHelp')
    results=((-1,-1,-1),(0,99,123),(128,-1,-1),(-1,133,999),(-1,-1,140),(-2,-3,-4),(-1,0,-1))
    return list(itertools.product(names,results,(0,128),range(5)))+[(b'',(-1,-1,-1),128,mode) for mode in range(5)]


class Oracle(MemoryOracle):
    """게임·창·OS는 실행하지 않고 검토한 요청/CRT만 실제 기계어로 실행한다."""
    def __init__(self):
        """실제 PE와 분리된 노드/가상 효과 경계를 준비한다."""
        super().__init__();rows={int(r['entry'],16):r for r in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')};self.allowed=[];self.returns=0;self.calls=Counter()
        # 함수의 불연속 구간을 각각 허용한다.
        for entry in ENTRIES:
            for part in rows[entry]['ranges'].split(';'):
                lo,hi=(int(n,16) for n in part.split('-'));self.allowed.append((lo,hi+1))

    def event(self,*parts):
        """실제 외부 호출과 현재 원문 바이트를 순서대로 보존한다."""
        self.events.append(','.join(map(str,parts)))

    def instruction(self,mu,address,size,data):
        """원본 계산은 계속 실행하고 조회·순수 서식·창/이력·가상 효과만 명시 경계에서 반환한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,6);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if any(lo<=address and address+size<=hi for lo,hi in self.allowed):self.instructions+=1;return
        if address==0x4ea61d:self.ret(TLS)
        elif address==0x4a89c0:
            target,pattern,value=args[:3];pattern=self.string(pattern);value=self.string(value);self.event('Format',pattern.hex(),value.hex());mu.mem_write(target,pattern.replace(b'%s',value)+b'\0');self.ret(target)
        elif address==0x49a860:
            result=self.results[self.lookup];self.lookup+=1;self.event('Lookup',self.string(args[0]).hex(),result&0xffffffff);self.ret(result)
        elif address==0x4c52c0:self.event('History',self.string(args[0]).hex());self.ret()
        elif address==0x4c5970:
            assert ecx==NODE;self.event('Show',args[0],self.string(args[1]).hex())
            if self.mode==2:self.put(NODE+4,8)
            elif self.mode==3:self.put(NODE+4,0)
            self.ret(purge=8)
        elif address==PUSH:
            assert ecx==NODE;self.event('Push')
            if self.mode==4:self.put(NODE+4,0x800);self.put(NODE+0x4c,0x80000000);self.put(NODE+0x50,0x7fc12345)
            self.ret()
        elif address==ATTACH:assert ecx==NODE;self.event('Attach',*args[:4]);self.ret(purge=16)
        else:raise RuntimeError(f'도움말 요청 범위 밖 실행 {address:08x}')

    def sample(self,values,precision):
        """입력만 설치하고 정상 반환/비휘발/FPU/현재 노드/원문 보존을 확인한다."""
        name,self.results,kind,self.mode=values;mu=self.mu;mu.mem_write(NODE,bytes(0xc0));mu.mem_write(TOPIC,name+b'\0');self.put(NODE,VT);self.put(VT+0x18,PUSH);self.put(VT+0x20,ATTACH);self.put(NODE+4,8 if self.mode in (1,3) else 0);self.put(NODE+0x4c,0x41880000);self.put(NODE+0x50,0xc1100000);self.put(TLS+0x64,LOCALE);self.put(0x543efc,LOCALE);self.put(LOCALE+0x14,0);self.events=[];self.lookup=0
        saved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x3456789a,UC_X86_REG_EBP:0x456789ab}
        # 독립 호출의 비휘발 레지스터와 x87 상태를 명시한다.
        for register,value in saved.items():mu.reg_write(register,value)
        for register,value in ((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,NODE),(UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,precision),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):mu.reg_write(register,value)
        mu.mem_write(STACK,struct.pack('<3I',STOP,kind,TOPIC));mu.emu_start(0x4c5c00,STOP,count=100000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+12
        assert all(mu.reg_read(r)==v for r,v in saved.items()) and mu.reg_read(UC_X86_REG_FPCW)==precision and not mu.reg_read(UC_X86_REG_FPSW)&0x3800
        assert bytes(mu.mem_read(TOPIC,len(name)+1))==name+b'\0';self.returns+=1
        return ';'.join(self.events)+'|'+','.join(map(str,(self.words(NODE+4,1)[0],*self.words(NODE+0x4c,2))))


def main():
    """새 원문 생성/짧은 실행/기존 SHA 검사를 분리하며 기존 기대값을 검증 중 덮어쓰지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        # 현재 근거와 모든 동결 행을 확인한다.
        for name,wanted in report['sha256'].items():assert digest(ROOT/name)==wanted,name
        rows=[r for r in FIXTURE.read_text('utf-8').splitlines() if r and not r.startswith('#')];assert len(rows)==report['rows']==len(inputs());print('Quick Help 요청 SHA/원문 감사',len(rows));return
    oracle=Oracle();rows=[]
    # 두 정밀도에서 동일 입력의 원본 호출/필드를 비교한다.
    for values in (inputs()[:1] if args.smoke else inputs()):
        first=oracle.sample(values,0x27f);second=oracle.sample(values,0x37f);assert first==second,values
        name,results,kind,mode=values;rows.append(name.hex()+'\t'+','.join(map(str,(*results,kind,mode)))+'\t'+first)
    if args.smoke:print('Quick Help 요청/실제 CRT 정상 반환',oracle.returns);return
    FIXTURE.write_text('# topic hex / lookup0,1,2 kind mode / ordered effects|flags,xBits,yBits\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    names=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_quick_help_request.py',FIXTURE.relative_to(ROOT).as_posix())
    report=dict(unit=184,host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',lastDecompileDate='2026-10-16',rows=len(rows),normalReturns=oracle.returns,instructions=oracle.instructions,precisionDifferences=0,nativeCalls=dict(oracle.calls),sha256={name:digest(ROOT/name) for name in names},limits=['실제4c5c00/4e65ed/4ef510의 분기/문자열 비교를 실행한다. 타입 조회·순수%s서식·4c52c0이력·4c5970창·가상Push/Attach와 C로케일 thread getter는 명시 외부 경계다.','전체 본문/창/Client 연결·CD10.72·게임 동일성의 증거가 아니다. 원본 스택 밖을 읽는 짧은 접미 입력 및80바이트 초과 입력은 동결 대상으로 만들지 않았다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Quick Help 요청 독립 원본',len(rows),oracle.returns,oracle.instructions)


if __name__=='__main__':main()
