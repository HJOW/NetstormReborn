#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""구름 표시/dirty/갱신 여섯 부모의 실제 PE 명령을 제품 변경 전에 격리 관측한다."""
import argparse,csv,itertools,json,socket,struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,POOL,STACK,STOP,digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_FPCW

STATE,SIZE,CLIP,SPEED=0x5697f0,2054,0x5c7a44,0x540a80  # 같은 연속 전역/clip/현재 속도의 보호 PE 주소다.
CACHE,ALL,PART,DIRTY=0x569ffc,0x56b7fc,0x56b800,0x56a000  # 부모가 다시 읽는 실제 DWORD다.
ENTRIES=dict(update=0x470c50,mark=0x470c80,clean=0x470ca0,query=0x470ce0,draw=0x4711d0,alternate=0x471410)  # 여섯 실제 함수 진입점이다.
TOKENS={0:0,11:POOL+0x1000,22:POOL+0x51000}  # 생성/복사 장치의 명시 반환 주소이며 부모 계산이 아니다.
FIXTURE=ROOT/'cpppj/tests/fixtures/kencloud-render-x86.tsv'  # 최초 관측을 덮어쓰지 않는 공통 회귀 입력이다.
REPORT=ROOT/'cpppj/kencloud-render-native.json'  # 원문 실행과 명시 장치의 경계/SHA를 기록한다.


def fnv(data):
    """전체 원시 상태 바이트를 64비트 FNV1a로 식별한다."""
    result=14695981039346656037
    for value in data: result=((result^value)*1099511628211)&0xffffffffffffffff  # 원시 순서를 보존한다.
    return f'{result:016x}'


def cases():
    """사각형/행/감김·캐시·하위 변경과 중단을 독립 입력한다."""
    rows=[]
    # kind,cache,speed,pattern,all,partial,left,top,right,bottom,offsetX,offsetY,mutation 순서다.
    for kind,rect,offset,pattern in itertools.product(('draw','alternate'),((0,0,640,480),(13,17,534,539),(5,9,6,10),(1,2,1,9),(1,2,9,2),(0,0,65536,65536)),((0,0),(3,7),(-1,-513),(511,512),(-512,-1),(2147483647,-2147483648)),(0,1,2)):
        rows.append((kind,11,3,pattern,1 if pattern==2 else 0,19,*rect,*offset,'none'))
    for kind,cache,speed in itertools.product(('update',),(0,11),(0,1,3)):
        rows.append((kind,cache,speed,1,17,19,0,0,0,0,0,0,'none'))
    for kind,pattern,all_dirty,partial in itertools.product(('clean','mark'),range(3),(0,17),(0,19)):
        rows.append((kind,11,3,pattern,all_dirty,partial,0,0,0,0,0,0,'none'))
    for first,second,pattern,all_dirty in itertools.product((0,511,512,-1,-512,-513,2147483647,-2147483648),(0,1,-1,512),range(3),(0,17)):
        rows.append(('query',11,3,pattern,all_dirty,19,first,second,0,0,0,0,'none'))
    for kind in ('draw','alternate'):  # 실제 경계 뒤 현재 cache/dirty/clip와 중단을 다시 관측한다.
        for mutation in ('build-speed2','build-stop','clip1-cache22','clip1-all0','clip1-stop','lock-all1','lock-all0','lock-stop','blit1-cache22','blit1-all0','blit1-stop','blit2-stop','unlock-stop','clip2-stop'):
            rows.append((kind,0 if mutation.startswith('build') else 11,3,1,0,19,13,17,534,539,3,7,mutation))
    for mutation in ('build-speed2','build-speed0','build-stop','step1-speed0','step1-stop','step2-stop'):
        rows.append(('update',0 if mutation.startswith('build') else 11,3,1,17,19,0,0,0,0,0,0,mutation))
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """분기/루프/dirty 쓰기는 PE이며 생성/step/clip/잠금/복사는 명시 동기 장치다."""
    def __init__(self):
        """현재 호스트의 전체 정밀 함수표에서 불연속 몸체 범위를 읽는다."""
        super().__init__();self.total=0;self.completions=Counter();self.ranges=[]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for entry in ENTRIES.values():  # 의미로 추정한 범위 대신 실제 함수표의 모든 조각을 허용한다.
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.ranges.append((low,high+1))

    def current(self,address):
        """실제 같은 현재 DWORD를 읽는다."""
        return self.words(address,1)[0]

    def mutate(self,label):
        """선택한 동기 하위의 명시 입력 변경 또는 중단만 수행한다."""
        mutation=self.case[-1]
        if mutation==label+'-stop': self.cause=label;self.mu.emu_stop()
        elif mutation==label+'-speed2': self.put(SPEED,2)
        elif mutation==label+'-speed0': self.put(SPEED,0)
        elif mutation==label+'-cache22': self.put(CACHE,TOKENS[22])
        elif mutation==label+'-all0': self.put(ALL,0)
        elif mutation==label+'-all1': self.put(ALL,1)

    def instruction(self,mu,address,size,data):
        """원문 몸체만 실행하며 하위 인자/호출 순서·현재 효과를 보존한다."""
        self.instructions+=1;self.total+=1
        if self.instructions>300000: raise RuntimeError('구름 표시 독립 명령 상한 '+str(self.case))
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,8);label=None
        if address==0x471040:
            self.events.append('BUILD');self.put(CACHE,TOKENS[11]);label='build'  # 생성 결과만 명시하며 기존 생성246관측을 대신 계산하지 않는다.
        elif address==0x470b20:
            self.counts['step']+=1;self.events.append('STEP');label='step'+str(self.counts['step'])
        elif address==0x4a1580:
            self.counts['clip']+=1;self.events.append('CLIP:'+','.join(map(str,args[:4])))
            for index,value in enumerate(args[:4]): self.put(CLIP+index*4,value)  # 현재 clip 장치의 명시 저장 효과다.
            label='clip'+str(self.counts['clip'])
        elif address in (0x4a14e0,0x4a15d0): self.events.append('LOCK:'+str(int(address==0x4a15d0)));label='lock'
        elif address in (0x4a1550,0x4a1650): self.events.append('UNLOCK:'+str(int(address==0x4a1650)));label='unlock'
        elif address==0x4a1fe0:
            self.counts['blit']+=1;self.events.append('BLIT:'+','.join(map(str,args)));label='blit'+str(self.counts['blit'])
        if label is not None:
            self.mutate(label)
            if not self.cause:self.ret()
            return
        if not any(low<=address<high for low,high in self.ranges):raise RuntimeError('구름 표시 원문 범위 밖 '+hex(address))

    def sample(self,case,control):
        """같은2054DWORD 전체·현재 clip/속도·반환·trace를 독립 초기화한다."""
        self.case=case;self.events=[];self.counts=Counter();self.cause='';self.instructions=0
        kind,cache,speed,pattern,all_dirty,partial,left,top,right,bottom,x,y,mutation=case
        state=[0x81000000+index for index in range(SIZE)]
        for index in range(512):  # 선택한 이전 표시/현재 dirty 행만 명시 입력으로 만든다.
            state[index]=int(pattern==2 or (pattern==1 and index in (0,8,511)))
            state[516+index]=int(pattern==2 or (pattern==1 and index in (1,8,511)))
        state[512:516]=[TOKENS[cache]+262144,TOKENS[cache]+261632,TOKENS[cache]+512,TOKENS[cache]]
        state[2051:2054]=[all_dirty,partial,9]
        self.mu.mem_write(STATE,struct.pack('<'+'I'*SIZE,*[v&0xffffffff for v in state]));self.put(SPEED,speed)
        for index,value in enumerate((-9,-7,900,700)):self.put(CLIP+index*4,value)  # 저장/복구가 필요한 기존 clip다.
        self.mu.mem_write(STACK-0x1000,b'\xcd'*0x1100);self.put(STACK,STOP)
        for index,value in enumerate((left,top,right,bottom,x,y)):self.put(STACK+4+index*4,value)  # 인자는 같은 원시 DWORD다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.emu_start(ENTRIES[kind],STOP);cause=self.cause or 'return';self.completions[cause]+=1
        result=self.mu.reg_read(UC_X86_REG_EAX) if kind=='query' and not self.cause else 0
        return '\t'.join((str(control),','.join(map(str,case)),cause,str(result),fnv(bytes(self.mu.mem_read(STATE,SIZE*4))),','.join(map(str,self.words(CLIP,4))),str(self.current(SPEED)),';'.join(self.events)))


def main():
    """최초 독립 자료만 만들며 재실행은 원시 SHA를 검증한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        for name,value in report['sha256'].items():assert digest(ROOT/name)==value,name  # 최초 원문/도구/fixture를 수정하지 않는다.
        print(json.dumps({'verified':report['observations']}));return
    assert not FIXTURE.exists() and not REPORT.exists(),'최초 구름 표시 자료 덮어쓰기 금지'
    oracle=Oracle();inputs=cases();lines=['# control / kind,cache,speed,pattern,all,part,l,t,r,b,x,y,mutation / completion / query / stateFNV / clip / speed / trace']
    for case in inputs:  # 양 x87 모드에서 실제 정수 몸체의 모든 효과가 같은지 확인한다.
        first=oracle.sample(case,0x27f);second=oracle.sample(case,0x37f);assert first.split('\t')[1:]==second.split('\t')[1:],case;lines.extend((first,second))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    names=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_kencloud_render.py','cpppj/tests/fixtures/kencloud-render-x86.tsv')
    report=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',lastDecompileDate='2026-10-23',entries=ENTRIES,uniqueInputs=len(inputs),observations=len(inputs)*2,precisionDifferences=0,instructions=oracle.total,completions=dict(oracle.completions),gameExecuted=False,guiExecuted=False,scope='실제 여섯 부모의 분기/감김/반복/dirty/clip 복구. 생성/step/clip/잠금/복사는 명시 장치이며 실제 픽셀/전체994b0/자연 Frame 증거와 구별한다.',sha256={name:digest(ROOT/name) for name in names})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('uniqueInputs','observations','completions','instructions')}))


if __name__=='__main__':main()
