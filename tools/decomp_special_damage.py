#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제10.78 신전 피해 분배와 윈드 타워 방어/중심/기하를 독립 관측한다."""
import argparse,csv,itertools,json,socket,struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,POOL,TYPES,TEXT,STACK,STOP,fbits,digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG

# 현재 피해자/공격자와 가상 효과 주소는 게임 이미지/기하 스택과 겹치지 않는다.
VICTIM,ATTACKER,REFRESH=15000,15001,STOP+0x100
ENTRIES=(0x4dba40,0x48ef70,0x4ac200,0x4ac550,0x49a840,0x4adea0,0x49a900,0x41ce90,0x4e49c0)
FIXTURE=ROOT/'cpppj/tests/fixtures/special-damage-x86.tsv' # 새 제품 전 실제 결과 동결 원문이다.
REPORT=ROOT/'cpppj/special-damage.json' # 계산/경계와 실제 호출 수를 구별한다.


def inputs():
    """신전 abstract/Form/피해와 방어의 방향·공격genus·같은중심·소수좌표를 교차한다."""
    vortex=[(0,extra,found,amount,owner,result) for extra,found,amount,owner,result in itertools.product((0,1,4,5),(0,15100),(0,123,0xffffffff),(0,2),(0,1))]
    defense=[(1,genus,typ,side,fbits(x),fbits(y),width,height) for genus,typ,side,(x,y,width,height) in itertools.product((0,0x100,0x100000,0x100100),(128,149),map(ord,'LMNOAX'),((50,40,1,1),(60,50,1,1),(50,60,3,3),(40,50,3,3),(50,50,2,2),(60.125,60.125,2,2),(40.25,39.875,4,3)))]
    return vortex+defense


class Oracle(MemoryOracle):
    """가상 일반피해/건설/표시만 경계로 두고 타워 방어 산술은 모두 실제 명령으로 실행한다."""
    def __init__(self):
        """현재 함수의 정확한 불연속 구간과 단일 풀/타입 표를 연결한다."""
        super().__init__();rows={int(r['entry'],16):r for r in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')};self.allowed=[];self.calls=Counter();self.returns=0;self.events=[]
        # 의도한 실제 함수만 실행하며 알 수 없는 진입을 넓은 코드 범위로 허용하지 않는다.
        for entry in ENTRIES:
            for part in rows[entry]['ranges'].split(';'):
                lo,hi=(int(n,16) for n in part.split('-'));self.allowed.append((lo,hi+1))
        self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES);self.put(0x5410e8,30);self.put(0x5412ac,149);self.put(TEXT+0x88,REFRESH)

    def instruction(self,mu,address,size,data):
        """일반피해/lookup/건설·표시의 실제 인수와 반환 순서만 기록한다."""
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address in (0x44c8e0,0x4b1a90,0x443dc0,REFRESH):
            args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,4);this=mu.reg_read(UC_X86_REG_ECX)
            if address==0x44c8e0:self.events.append('H:'+','.join(map(str,((this-POOL)//50,*args[:3]))));self.ret(self.case[-1],12)
            elif address==0x4b1a90:self.events.append('F:'+','.join(map(str,args)));self.ret(self.case[2])
            elif address==0x443dc0:self.events.append('C:'+','.join(map(str,args[:2])));self.ret()
            else:self.events.append('R:'+str((this-POOL)//50));self.ret()
            return
        if any(lo<=address and address+size<=hi for lo,hi in self.allowed):self.instructions+=1;return
        raise RuntimeError(f'특수 피해 허용 밖 실행 {address:08x}')

    def sample(self,case,precision):
        """명시 raw 입력과 경계 반환만 설치하고 실제 반환/현재 사건을 관찰한다."""
        self.case=case;self.events=[];mu=self.mu;victim=POOL+VICTIM*50;attacker=POOL+ATTACKER*50;mu.mem_write(victim,bytes(100));self.put(victim,TEXT);self.put(victim+10,142,1);self.put(attacker+10,128,1)
        self.put(victim+14,fbits(50));self.put(victim+18,fbits(50));self.put(TYPES+142*500+0x1d4,2);self.put(TYPES+142*500+0x1d8,2)
        if case[0]==0:
            _,extra,found,amount,owner,result=case;self.put(victim+40,extra,1);args=(amount,owner,ATTACKER);entry=0x4dba40;purge=12
        else:
            _,genus,typ,side,x,y,width,height=case;self.put(attacker+10,typ,1);self.put(attacker+14,x);self.put(attacker+18,y);self.put(TYPES+typ*500+0xec,genus);self.put(TYPES+typ*500+0x1d4,width);self.put(TYPES+typ*500+0x1d8,height);self.put(TYPES+142*500+0x124,TEXT+0x1000);self.put(TEXT+0x1000,side);args=(ATTACKER,);entry=0x48ef70;purge=4
        saved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x3456789a,UC_X86_REG_EBP:0x456789ab}
        # 각 호출의 비휘발/FPU/스택은 이전 호출 결과를 가져오지 않는다.
        for register,value in saved.items():mu.reg_write(register,value)
        for register,value in ((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,victim),(UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,precision),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):mu.reg_write(register,value)
        mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.emu_start(entry,STOP,count=100000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge and all(mu.reg_read(r)==v for r,v in saved.items()) and not mu.reg_read(UC_X86_REG_FPSW)&0x3800
        self.returns+=1;return str(mu.reg_read(UC_X86_REG_EAX))+'\t'+';'.join(self.events)


def main():
    """동결 생성과 읽기 전용 감사를 구분하며 기대 원문을 검사 중 다시 만들지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        # 현재 소스와 원본/fixture의 바이트 불변을 각각 확인한다.
        for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
        assert report['rows']==len(inputs())==sum(bool(line) and not line.startswith('#') for line in FIXTURE.read_text('utf-8').splitlines());print('특수 피해 원문 감사',report['rows']);return
    oracle=Oracle();rows=[]
    # 두 정밀도의 전체 결과가 같아야 하며 불일치 사례를 지우지 않는다.
    for case in (inputs()[::100] if args.smoke else inputs()):
        first=oracle.sample(case,0x27f);second=oracle.sample(case,0x37f);assert first==second,case;rows.append(','.join(map(str,case))+'\t'+first)
    if args.smoke:print('특수 피해 정상 반환',oracle.returns);return
    FIXTURE.write_text('# 입력 / 실제 반환 / 실제 효과 순서\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    files=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_special_damage.py',FIXTURE.relative_to(ROOT).as_posix()]
    report=dict(unit=186,host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',lastDecompileDate='2026-10-16',rows=len(rows),normalReturns=oracle.returns,instructions=oracle.instructions,precisionDifferences=0,nativeCalls=dict(oracle.calls),boundaries='실제4dba40 분기와48ef70/중심/TYPE getter/4방향/CRT 전체.4b1a90 lookup/443dc0 기존 건설 끝 시각/가상88 표시/44c8e0 기존 일반피해는 명시 반환·인수/순서 경계다. HP·죽음·미션 완주/다른 판본 증거가 아니다.',sha256={p:digest(ROOT/p) for p in files})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('특수 피해 독립 원본',len(rows),oracle.returns,oracle.instructions)


if __name__=='__main__':main()
