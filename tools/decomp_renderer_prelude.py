#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 Renderer Draw 앞단의 옵션/override/구름/행 스캔 관문을 제품 작성 전에 격리 관측한다."""
import argparse,csv,itertools,json,platform
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,POOL,STACK
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG

ENTRY,END=0x4994b0,0x499758  # 실제 Draw 함수 진입부터 월드 좌표 계산 직전까지다.
SCAN=0x59a984  # index/16행/초기화/이전 화면 flags가 인접한19DWORD를 유지한다.
FIELDS=('mode','previous','force','override','width','height','initHeight','cameraY','left','top','right','bottom','index','initialized','hit','mutation','rendererFlag','dirtyCount','drawCounter')  # 기대값과 무관한 현재 전역/장치 입력이다.
GLOBALS=((0x5c78d4,'mode'),(0x59a8b0,'force'),(0x565dd8,'override'),(0x5c78f0,'width'),(0x5c78f4,'height'),(0x531864,'initHeight'),(0x59a920,'cameraY'),(0x5ca9cc,'left'),(0x5ca9d0,'top'),(0x5ca9d4,'right'),(0x5ca9d8,'bottom'))  # 원문이 각 효과 뒤 다시 읽는 실제 주소다.


def initial_rows():
    """행0을 포함한 기존16개 값을 명시 입력으로 만들며 초기화 결과를 예측하지 않는다."""
    return [index*7-11 for index in range(16)]  # 원문 초기화가 쓰지 않는0번 행의 기존값도 보존한다.


class Oracle(MemoryOracle):
    """상위 분기/행 초기화·진행·clip/MarkDirty/getter는 실제 PE 명령으로 실행한다."""
    def __init__(self):
        """현재 호스트의 보호 PE/정밀 함수표와 실제 하위 세 함수의 범위만 허용한다."""
        super().__init__()
        self.allowed=[(ENTRY,END)]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for entry in (0x470c80,0x463fe0,0x445290):  # dirty DWORD/override getter/정수 clip은 기존 몸체를 그대로 관측한다.
            for part in facts[entry]['ranges'].split(';'):  # 불연속 범위도 실제 정밀 내보내기를 따른다.
                first,last=(int(value,16) for value in part.split('-'))
                self.allowed.append((first,last+1))

    def effect(self,label):
        """명시 하위 효과가 바꾸는 현재 전역/인접 상태만 입력하며 상위 계산은 하지 않는다."""
        mutation=self.case['mutation']
        key=(mutation,label)
        if key in self.changed:
            return
        self.changed.add(key)
        if key==(1,'M'):
            self.put(0x5c78d4,0x38);self.put(0x5c78f0,801);self.put(0x59a8b0,1)
        elif key==(2,'F'):
            self.put(0x5c78d4,0x29);self.put(0x5c78f4,611)
        elif key==(3,'O'):
            self.put(0x5c78d4,0)
        elif key==(4,'U'):
            self.put(0x5c78d4,0x19)
        elif key==(5,'Q'):
            self.put(SCAN,15);self.put(0x5c78d4,0x19)
        elif key==(6,'A'):
            self.put(0x5c78d4,0);self.put(0x59a8b0,0)
        elif key==(7,'Q'):
            self.put(SCAN,2);self.put(SCAN+3*4,100);self.put(0x5c78f0,704);self.put(0x5ca9cc,17)
        elif key==(8,'B'):
            self.put(0x59a8b0,0);self.put(POOL+0x60,17)

    def instruction(self,mu,address,size,data):
        """실제 호출 순서/인자를 기록하고 미복원 외부 장치만 명시 반환으로 분리한다."""
        if address==END:
            self.completed=True;mu.emu_stop();return
        esp=mu.reg_read(UC_X86_REG_ESP)
        args=self.words(esp+4,4)
        if address==0x470c80:
            self.events.append('M');self.effect('M')
        elif address==0x463fe0:
            self.events.append('O');self.effect('O')
        elif address==0x4977f0:
            self.events.append('F:'+','.join(map(str,args)));self.effect('F');self.ret();return
        elif address==0x470c50:
            self.events.append('U');self.effect('U');self.ret();return
        elif address==0x470ce0:
            self.queries+=1;self.events.append('Q:'+','.join(map(str,args[:2])));self.effect('Q');self.ret(int(self.queries==self.case['hit']));return
        elif address==0x497580:
            rect=self.words(args[0],4);self.events.append('A:'+','.join(map(str,(*rect,args[1]))));self.effect('A');self.ret(purge=8);return
        elif address==0x497860:
            self.events.append('B');self.effect('B');self.ret();return
        if not any(first<=address and address+size<=last for first,last in self.allowed):
            raise RuntimeError(f'Renderer 앞단 관측 밖 명령 {address:08x}')

    def run(self,case,control):
        """독립 전역·스택·실제 Renderer 저장소를 준비하고 정상 후속 구간 도착만 수락한다."""
        self.case=case;self.events=[];self.changed=set();self.queries=0;self.completed=False  # 이전 효과나 스캔 요청을 다음 사례에 남기지 않는다.
        for address,key in GLOBALS: self.put(address,case[key])  # 매 실행의 현재 전역 입력이다.
        self.mu.mem_write(SCAN,bytes(19*4))
        self.put(SCAN,case['index'])
        for index,value in enumerate(initial_rows()): self.put(SCAN+(index+1)*4,value)  # 초기 행은 결과와 독립인 고정 입력이다.
        self.put(SCAN+17*4,case['initialized']);self.put(SCAN+18*4,case['previous'])
        self.mu.mem_write(POOL,bytes(0x800));self.put(POOL+0x60,case['rendererFlag']);self.put(POOL+0x64,case['dirtyCount'])
        self.put(0x59a8d4,case['drawCounter']);self.put(0x56b7fc,17);self.put(0x56b800,19)
        self.mu.mem_write(STACK-0x1000,bytes(0x1100))
        for register,value in ((UC_X86_REG_ESP,STACK),(UC_X86_REG_ECX,POOL),(UC_X86_REG_FPCW,control),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):  # thiscall 진입과 빈 x87은 실제 부모 계약이다.
            self.mu.reg_write(register,value)
        self.mu.emu_start(ENTRY,END+1,count=10000)
        assert self.completed and self.mu.reg_read(UC_X86_REG_ESP)==STACK-0xc0
        return [*self.words(SCAN,19),*(self.words(address,1)[0] for address,_ in GLOBALS),*self.words(POOL+0x60,2),self.words(0x59a8d4,1)[0],*self.words(0x56b7fc,2),';'.join(self.events) or '-']


def cases():
    """옵션/이전 flags/force/override와 모든16행·경계/효과 재조회 입력을 준비한다."""
    base=dict(zip(FIELDS,(40,40,0,0,1024,768,768,500,84,0,1024,768,0,0,1,0,7,9,11)))
    rows=[]
    for mode,changed,force,override in itertools.product((*range(64),138,139,171,235),(0,1),(0,1),(0,37)):  # 화면 옵션 여섯 비트와 현재 호환 모드의 교차다.
        rows.append(dict(base,mode=mode,previous=mode if not changed else mode^1,force=force,override=override))
    for index,initialized,hit in itertools.product((*range(16),-1,16,17),(0,1),(-1,1,4,30)):  # 인접 index/초기화/이전 flags alias도 실제 메모리대로 관측한다.
        rows.append(dict(base,index=index,initialized=initialized,hit=hit))
    for key,values in (('initHeight',(-2147483648,-1,0,29,30,2147483647)),('width',(0,-1,1280)),('height',(0,-1,800)),('cameraY',(-2147483648,2147483647)),('initialized',(0x80000000,0xffffffff)),('mutation',tuple(range(1,9)))):
        for value in values: rows.append(dict(base,**{key:value}))  # 한 입력만 바꾸어 잘못된 조기 생략을 검출한다.
    rows.append(dict(base,mode=25,previous=25,mutation=8))
    return [dict(zip(FIELDS,row)) for row in dict.fromkeys(tuple(case[key] for key in FIELDS) for case in rows)]  # 동일 입력만 제거하고 최초 순서는 유지한다.


def main():
    """최초/재관측/다른 x87 제어어 결과가 같을 때 새 경로에 원시와 fixture를 기록한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    inputs=cases();oracle=Oracle()
    first=[oracle.run(case,0x27f) for case in inputs]
    replay=[oracle.run(case,0x27f) for case in inputs]
    extended=[oracle.run(case,0x37f) for case in inputs]
    assert first==replay==extended
    for name,values in (('inputs.json',inputs),('first.json',first),('replay.json',replay),('extended.json',extended)):  # 최초 원시 출력과 입력을 같은 경로에 보존한다.
        (out/name).write_text(json.dumps(values,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    with (out/'renderer-prelude-x86.tsv').open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# 입력19열 / 인접상태19DWORD / 현재전역11DWORD / Renderer2·counter·cloud2 / 사건순서\n')
        for case,result in zip(inputs,first): stream.write('\t'.join(map(str,(*(case[key] for key in FIELDS),*result)))+'\n')  # 기대값은 원문 관측에서만 가져온다.
    report={'unit':618,'host':platform.node(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','entry':hex(ENTRY),'end':hex(END),'cases':len(inputs),'observations':len(inputs)*3,'differences':0,
        'scope':'실제 상위 앞단·MarkDirty/getter/정수clip 명령의 격리 x86 관측이다. Update/RowDirty/dirty목록/배경장치는 명시 경계다. 게임GUI/원본실행/공격/포획/희생/승리/신규MP 검사가 아니다.'}
    (out/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':
    main()
