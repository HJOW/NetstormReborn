#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보충 부모485e40과 실제 지역 검색·Canon·변환을 보호 PE에서 독립 관측한다."""
import argparse,collections,csv,itertools,json,socket,struct
import decomp_player_geyser_bounds as bounds
from decomp_particle_gradient import digest
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ROOT=bounds.ROOT # 보호 PE와 기존 정밀 export의 기준 경로다.
FIXTURE=ROOT/'cpppj/tests/fixtures/nugget-replenish-x86.tsv' # 이번에 독립 생성할 원문이다.
REPORT=ROOT/'cpppj/nugget-replenish.json' # 호출 수/경계/현재 PC와 파일 해시다.
KEYS=('boss','single','fort','editor','span','shape','rotation','foot','frames','layout','mutation') # 기대값 없는 명시 입력 열이다.

def cases():
    """모든 관문·패턴/회전·정수 감김·진단 후 현재 전역을 교차한다."""
    result=[]
    def add(**changes):
        """입력 필드만 구성하며 사각형은 계산하지 않는다."""
        value=dict(zip(KEYS,(1,0,0,0,1,0,0,0,0,0,0)));value.update(changes);result.append(tuple(value[k] for k in KEYS))
    # 네 현재 관문을 모두 분리한다.
    for boss,single,fort,editor in itertools.product((0,1),repeat=4):add(boss=boss,single=single,fort=fort,editor=editor)
    # 현재 지역의 전체 패턴/회전을 실제 Canon으로 읽는다.
    for shape,rotation in itertools.product(range(64),range(4)):add(shape=shape,rotation=rotation)
    # 큰 signed 확장 값과 감김에서 정수→float 순서를 검증한다.
    for span,shape in itertools.product((0,2,0xffffffff,0x7fffffff,0x80000000,0x7ffffff,0x8000000,0x10000000,0x10000001,0xefffffff),(0,14,17,63)):add(span=span,shape=shape)
    # 발자국과 첫 지역/부재·위치 바이트를 독립 입력한다.
    for foot,rotation,layout in itertools.product(range(4),range(4),range(3)):add(foot=foot,rotation=rotation,layout=layout,shape=14)
    # 권한 진단과 Canon 진단 뒤의 현재 값을 재독한다.
    for mutation in range(1,6):add(boss=0,mutation=mutation)
    for mutation,rotation in itertools.product((0,6,7),range(4)):add(frames=1,shape=17,rotation=rotation,mutation=mutation)
    return list(dict.fromkeys(result))

class Oracle(bounds.Oracle):
    """기존 전체 Canon 관측 메모리를 재사용하되 새 부모/원점 함수를 실제 실행한다."""
    def __init__(self):
        """원본 명령 범위만 추가하며 OS나 원본 프로세스를 실행하지 않는다."""
        assert digest(bounds.common.BINARY)==bounds.common.SHA
        pe=bounds.common.pefile.PE(str(bounds.common.BINARY));self.data=pe.get_memory_mapped_image();self.base=pe.OPTIONAL_HEADER.ImageBase
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);self.mu.mem_map(self.base,(len(self.data)+4095)&~4095);self.mu.mem_write(self.base,self.data)
        # 이 PC에 있는 정밀 export를 사용한다. 없는 옛 선택 export를 새로 만들지 않는다.
        for address,size in ((bounds.TYPES,0x20000),(bounds.PLAYERS,0x1000),(bounds.CODES,0x1000),(bounds.OUTPUT,0x1000),(bounds.STOP,0x1000),(bounds.common.STACK_BASE,0x10000)):self.mu.mem_map(address,size)
        self.preparing=True;self.instructions=0;self.parents=0;self.preparations=0;self.calls=collections.Counter();self.native=collections.Counter();self.returns=0;self.assertions=0
        self.refined=ROOT/'extracted/refined/originals/functions.tsv';self.table=self.refined;self.allowed=[];self.entries={}
        rows={int(r['entry'],16):r for r in csv.DictReader(self.refined.open(encoding='utf-8'),delimiter='\t')}
        selected=json.loads((ROOT/'tools/ghidra/player-geyser-bounds-functions.json').read_text('utf-8'))['editions']['originals']
        # 두 새 몸체와 이미 관측한 하위 범위를 구별한다.
        for entry in [0x485e40,0x431ff0,*[int(s,16) for s in selected]]:
            self.entries[entry]=f'{entry:08x}'
            # 분리 구간도 실제 export대로 허용한다.
            for part in rows[entry]['ranges'].split(';'):
                lo,hi=(int(v,16) for v in part.split('-'));self.allowed.append((lo,hi+1))
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def back(self):
        """cdecl 효과 경계에서 caller 소유 인자를 그대로 두고 복귀한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EIP,self.get(sp));self.mu.reg_write(UC_X86_REG_ESP,sp+4);self.mu.reg_write(UC_X86_REG_EAX,0)

    def instruction(self,mu,address,size,data):
        """실제 네 몸체 진입과 진단·큰 배치의 전체 인자만 관측한다."""
        if self.preparing:return super().instruction(mu,address,size,data)
        self.instructions+=1
        if address in self.entries:self.calls[self.entries[address]]+=1
        if address in (0x432430,0x431ff0,0x40d900):self.events.append({0x432430:'R',0x431ff0:'O',0x40d900:'C'}[address])
        if address==0x4e0620:
            sp=mu.reg_read(UC_X86_REG_ESP);line=self.get(sp+12);assert line in (0x246,299);self.events.append('A:'+str(line))
            mutation=self.value['mutation']
            if line==0x246 and mutation:
                self.put(0x540bc4,1)
                if mutation==2:self.put(0x594fa4,1)
                elif mutation==3:self.put(0x594fb8,1)
                elif mutation==4:self.put(0x5c85a4,1)
                elif mutation==5:self.put(0x542654,0x7fffffff)
            if line==299 and not self.changed:
                self.changed=True
                if mutation==6:self.put(0x542654,0x8000000)
                elif mutation==7:self.put(bounds.TYPES+107*500+0x1d4,2);self.put(bounds.TYPES+107*500+0x1d8,3);self.put(0x541204,108)
            self.back();return
        if address==0x40e050:
            sp=mu.reg_read(UC_X86_REG_ESP);args=[self.get(sp+4+i*4) for i in range(5)];self.events.append('P:'+':'.join(map(str,args)));self.back();return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'보충 범위밖 명령 {address:x}')

    def sample(self,row,precision):
        """기존 원본 R 준비 뒤 같은 raw 지도에서 새 부모를 독립 실행한다."""
        self.value=dict(zip(KEYS,row));v=self.value;self.preparing=True
        bounds.Oracle.run(self,bounds.case(op='R',shape=v['shape'],rotation=v['rotation'],foot=v['foot'],frames=v['frames'],layout=2))
        self.preparations+=1;self.preparing=False;self.events=[];self.changed=False;mu=self.mu
        # 127 기본 지도에서 지역0 첫 기록/중복/부재만 설정한다.
        record=bytes((v['shape']|(v['rotation']<<6),0xa5,0x32,17,29,41))
        if v['layout']!=2:self.put(bounds.CHUNKS+12+(2*16+3)*12,0,1);mu.mem_write(bounds.CHUNKS+14+(2*16+3)*12,record)
        if v['layout']==1:self.put(bounds.CHUNKS+12+(3*16)*12,0,1);mu.mem_write(bounds.CHUNKS+14+(3*16)*12,bytes((((v['shape']+7)&63)|(v['rotation']<<6),0xa5,0xff,17,29,41)))
        # 현재 글로벌을 실제 주소/폭으로 제공한다.
        for addr,key in ((0x540bc4,'boss'),(0x594fa4,'single'),(0x594fb8,'fort'),(0x5c85a4,'editor'),(0x542654,'span')):self.put(addr,v[key])
        for reg,value in bounds.common.SAVED.items():mu.reg_write(reg,value)
        mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ECX,0);mu.reg_write(UC_X86_REG_EDX,0);mu.reg_write(UC_X86_REG_ESP,bounds.STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
        mu.reg_write(UC_X86_REG_FPCW,precision);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_FPTAG,0xffff)
        mu.mem_write(bounds.STACK,struct.pack('<II',bounds.STOP,0xdec0dec0));mu.emu_start(0x485e40,bounds.STOP,count=2000000)
        assert mu.reg_read(UC_X86_REG_EIP)==bounds.STOP and mu.reg_read(UC_X86_REG_ESP)==bounds.STACK+4 and self.get(bounds.STACK+4)==0xdec0dec0
        assert all(mu.reg_read(reg)==value for reg,value in bounds.common.SAVED.items())
        assert mu.reg_read(UC_X86_REG_FPCW)==precision and mu.reg_read(UC_X86_REG_FPSW)&0x3800==0 and mu.reg_read(UC_X86_REG_FPTAG)&0xffff==0xffff
        self.parents+=1
        current=[self.get(a) for a in (0x540bc4,0x594fa4,0x594fb8,0x5c85a4,0x542654,0x541204,bounds.TYPES+107*500+0x1d4,bounds.TYPES+107*500+0x1d8)]
        return ';'.join(self.events) or '-',','.join(map(str,current))

def main():
    """원문 생성과 저장 해시 감사만 제공하며 C++ 출력은 읽지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        r=json.loads(REPORT.read_text('utf-8'));rows=[s.split('\t') for s in FIXTURE.read_text('utf-8').splitlines() if s and not s.startswith('#')]
        for p,h in r['sha256'].items():assert digest(ROOT/p)==h,p
        assert len(rows)==r['uniqueInputs'] and r['normalParentReturns']==len(rows)*2 and [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()] and all(r[1:3]==r[3:5] for r in rows)
        print('보충 부모 독립근거 감사',len(rows));return
    oracle=Oracle();rows=[];different=[]
    # 53/64비트 정밀도 기대값 모두 보호 PE에서만 읽는다.
    for index,c in enumerate(cases()):
        result=[oracle.sample(c,p) for p in (0x27f,0x37f)]
        if result[0]!=result[1]:different.append(index)
        rows.append((','.join(map(str,c)),*result[0],*result[1]))
    FIXTURE.write_text('# input effects53 globals53 effects64 globals64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=['originals/Netstorm.exe','tools/decomp_nugget_replenish.py','tools/decomp_player_geyser_bounds.py','tools/decomp_missionframe_oracle.py','tools/ghidra/player-geyser-bounds-functions.json',oracle.refined.relative_to(ROOT).as_posix(),FIXTURE.relative_to(ROOT).as_posix()]
    report=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalParentReturns=oracle.parents,preparationRegionReturns=oracle.preparations,instructions=oracle.instructions,native=dict(oracle.calls),precisionDifferentInputs=different,boundaries='485e40/431ff0/432430/40d900와 기존 실제 Canon/TYPE 하위를 PE로 실행했다. assertion 보고와 큰40e050 배치만 명시 경계다. 준비용R은 부모 관측과 별도 집계했다. 보존GP/ret/canary/x87/CW/TOP/빈tag/실행·쓰기범위를 검사했다. 원본게임/창/OS 실행과 실제 비미션 배치 검증은 아니다.',sha256={p:digest(ROOT/p) for p in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(report,ensure_ascii=False))

if __name__=='__main__':main()
