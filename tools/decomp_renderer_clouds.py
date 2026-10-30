#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본의 구름 dirty4칸 저장/배경 갱신과 dirty별 배경 선택을 격리 x86에서 관측한다."""
import argparse,csv,gc,itertools,json,platform
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,POOL,STACK,STOP
from unicorn import UcError,UC_ERR_EXCEPTION
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG

APPEND,REFRESH,BACKDROP,END=0x4977f0,0x497860,0x499bfd,0x499cfe  # 두 실제 함수와 Draw의 배경 선택 구간이다.
COUNT,RECTS=0x59a8d4,0x59a940  # 일반 Renderer dirty 표와 다른4개 구름 사각형/카운터다.
FIELDS=('kind','count','checking','mode','hidden','dirty','covered','tint','black','cameraX','cameraY','denominator','left','top','right','bottom','mutation')  # 원문 계산과 별개인 명시 입력 열이다.
GLOBALS=((0x5c78d4,'mode'),(0x59a8b4,'hidden'),(POOL,'dirty'),(STACK+0x6c,'covered'),(0x540cdc,'tint'),(0x5b5e58,'black'),(0x59a91c,'cameraX'),(0x59a920,'cameraY'),(0x540ce4,'denominator'))  # 호출 효과 뒤 다시 읽는 같은 원문 저장소다.


class Oracle(MemoryOracle):
    """계산/쓰기는 실제 명령으로 실행하고 진단/화면/구름 픽셀 장치만 분리한다."""
    def __init__(self):
        """현재 호스트의 정밀 함수 범위와 보호 PE를 읽는다."""
        super().__init__();self.allowed=[(BACKDROP,END)]  # 허용 구간 밖 실행을 즉시 거부한다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for entry in (APPEND,REFRESH):  # 실제 두 함수의 모든 조각만 추가한다.
            for part in facts[entry]['ranges'].split(';'):
                first,last=(int(value,16) for value in part.split('-'));self.allowed.append((first,last+1))

    def effect(self,label):
        """첫 하위 효과에서만 현재 카운터·입력·전역을 바꾸어 재조회 위치를 검증한다."""
        if label in self.changed: return
        self.changed.add(label)
        mutation=self.case['mutation']
        if label=='A' and mutation in (1,2): self.put(COUNT,1 if mutation==1 else 0x7fffffff)
        elif label=='P' and mutation==3: self.put(COUNT,1)
        elif label=='P' and mutation in (4,5):
            self.put(COUNT,3);self.put(0x540ce4,7 if mutation==4 else 0);self.put(0x59a91c,-17);self.put(0x59a920,23)
            for index,value in enumerate((101,103,107,109)): self.put(RECTS+16+index*4,value)  # 다음 사각형의 실제 현재 입력이다.
        elif label=='P' and mutation==10: self.put(COUNT,-1)
        elif label=='F' and mutation in (6,7,9):
            self.put(0x5c78d4,0 if mutation==6 else 24);self.put(0x540cdc,247);self.put(POOL,4 if mutation==9 else 0);self.put(STACK+0x6c,1 if mutation==9 else 0)
            self.put(0x540ce4,3);self.put(0x59a91c,-17);self.put(0x59a920,23)
            for index,value in enumerate((10,20,30,40)): self.put(STACK+0x34+index*4,value)  # 이미 보관한 right/bottom과 재조회 left/top을 구별한다.

    def instruction(self,mu,address,size,data):
        """실제 장치 인자·진단 행과 정상 종료 또는 나눗셈 fault를 구분한다."""
        if address==STOP or (self.case['kind']==2 and address==END): self.completed=True;mu.emu_stop();return
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,6)
        if address==0x4e0620:
            self.events.append('A:'+str(args[2]));self.effect('A');self.ret();return
        if address in (0x4711d0,0x471410):
            label='P' if address==0x471410 else 'C';self.events.append(label+':'+','.join(map(str,args)));self.effect(label);self.ret();return
        if address==0x4a2060:
            self.events.append('F:'+','.join(map(str,args[:5])));self.effect('F');self.ret();return
        if not any(first<=address and address+size<=last for first,last in self.allowed): raise RuntimeError(f'구름 Renderer 관측 밖 명령 {address:08x}')

    def run(self,case,control):
        """독립4칸/전역/지역 입력으로 실제 함수와 배경 구간을 실행한다."""
        self.case=case;self.events=[];self.changed=set();self.completed=False;fault=0  # 각 관측의 부분 효과를 다른 사례와 공유하지 않는다.
        self.mu.mem_write(STACK-0x1000,bytes(0x1100));self.put(STACK,STOP);self.put(COUNT,case['count']);self.put(0x5e4794,case['checking'])
        for index in range(16): self.put(RECTS+index*4,index*17-3)  # 기존 사각형을 0으로 지우는 잘못된 동작도 검출한다.
        for address,key in GLOBALS: self.put(address,case[key])  # 현재 전역을 원문 저장소에 직접 공급한다.
        self.put(STACK+0x20,POOL)
        for index,key in enumerate(('left','top','right','bottom')):
            self.put(STACK+0x34+index*4,case[key]);self.put(STACK+4+index*4,case[key])  # 함수 인자와 Draw 지역 사각형 입력이다.
        for register,value in ((UC_X86_REG_ESP,STACK),(UC_X86_REG_FPCW,control),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):
            self.mu.reg_write(register,value)  # 배경 선택은 x87 계산을 추가하지 않는다.
        try: self.mu.emu_start((APPEND,REFRESH,BACKDROP)[case['kind']],STOP+1,count=10000)
        except UcError as error:
            if error.errno!=UC_ERR_EXCEPTION: raise
            assert self.mu.reg_read(UC_X86_REG_EIP) in (0x49787e,0x497887,0x499c7a,0x499c83),hex(self.mu.reg_read(UC_X86_REG_EIP))
            fault=1
        assert self.completed or fault,(case,hex(self.mu.reg_read(UC_X86_REG_EIP)),self.events[:5])
        assert fault or self.mu.reg_read(UC_X86_REG_ESP)==STACK+(0 if case['kind']==2 else 4)
        return [self.words(COUNT,1)[0],*self.words(RECTS,16),*(self.words(address,1)[0] for address,_ in GLOBALS),*self.words(STACK+0x34,4),fault,';'.join(self.events) or '-']


def cases():
    """정상4칸·signed count/IDIV·화면 플래그와 하위 효과만 교차한다."""
    base=dict(zip(FIELDS,(0,0,0,138,0,0,0,167,0,101,203,15,13,17,534,539,0)));rows=[]
    for count,checking,mutation in itertools.product((0,1,2,3,4,5,0x7fffffff),(0,1),(0,1,2)):
        rows.append(dict(base,count=count,checking=checking,mutation=mutation))  # 음수 append는 앞선 물리 영역 쓰기이므로4칸 계약 밖으로 구별한다.
    for count,denominator,coordinates in itertools.product((0,1,2,4,-1,-2147483648),(0,1,15,-1),((101,203),(-17,23),(-2147483648,0),(0,-2147483648))):
        rows.append(dict(base,kind=1,count=count,denominator=denominator,cameraX=coordinates[0],cameraY=coordinates[1]))
    for mode,hidden,dirty,covered in itertools.product((*range(64),138,139,171,235),(0,1),(0,4),(0,1)):
        rows.append(dict(base,kind=2,mode=mode,hidden=hidden,dirty=dirty,covered=covered))  # clear와cloud 선택은 원문 출력으로만 얻는다.
    for mutation in (3,4,5,10): rows.append(dict(base,kind=1,count=2,mutation=mutation))
    for mutation in (6,7,9): rows.append(dict(base,kind=2,mode=24,hidden=1,mutation=mutation))
    for denominator,coordinates in itertools.product((0,-1),((-2147483648,0),(0,-2147483648),(101,203))):
        rows.append(dict(base,kind=2,mode=24,denominator=denominator,cameraX=coordinates[0],cameraY=coordinates[1]))
    return [dict(zip(FIELDS,row)) for row in dict.fromkeys(tuple(case[key] for key in FIELDS) for case in rows)]  # 입력 중복만 제거한다.


def observe(inputs,control):
    """IDIV fault 뒤에는 새 CPU로 이어가 Unicorn의 잔류 예외 상태를 다음 입력에 섞지 않는다."""
    oracle=Oracle();result=[];faults=[]
    for case in inputs:  # 정상 실행은 같은 격리 장치를 재사용하고 fault 뒤만 장치를 반납한다.
        value=oracle.run(case,control);result.append(value)
        if value[-2]:
            faults.append([len(result)-1,hex(oracle.mu.reg_read(UC_X86_REG_EIP))])
            oracle=None;gc.collect();oracle=Oracle()
    return result,faults


def main():
    """최초/재관측/다른 제어어가 같은 새 원시/fixture만 생성한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    inputs=cases();first,faults=observe(inputs,0x27f);replay,replay_faults=observe(inputs,0x27f);extended,extended_faults=observe(inputs,0x37f)
    assert first==replay==extended and faults==replay_faults==extended_faults
    for name,value in (('inputs.json',inputs),('first.json',first),('replay.json',replay),('extended.json',extended),('faults.json',faults)):
        (out/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')  # 최초 입력/출력을 그대로 보존한다.
    with (out/'renderer-clouds-x86.tsv').open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# 입력17 / count·rect16·globals9·localRect4·fault / 사건순서\n')
        for case,result in zip(inputs,first): stream.write('\t'.join(map(str,(*(case[key] for key in FIELDS),*result)))+'\n')  # 원문이 반환한 기대값만 쓴다.
    report={'unit':619,'host':platform.node(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','cases':len(inputs),'observations':len(inputs)*3,'differences':0,'faults':sum(row[-2] for row in first),'allFaultsAreActualIdiv':True,
        'scope':'실제 구름4칸 저장/반복·정수 나눗셈/배경 선택의 격리 x86이다. 진단과 실제 픽셀/구름 장치는 명시 경계이며 게임GUI/MP/공격/포획/희생/승리 실행은 없다.'}
    (out/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':
    main()
