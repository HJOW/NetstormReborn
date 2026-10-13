#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""공통 postPop의 비용·공급/작업장 목록·타입 통계를 활성 실제 x86으로 대조한다."""
import argparse
import collections
import csv
import hashlib
import json
import struct
import zlib
from pathlib import Path
from decomp_display_oracle import DisplayOracle, packed
from decomp_pop_oracle import SHAPE, HEADS, bits
from decomp_creation_oracle import TYPES
from decomp_sid_oracle import POOL
from decomp_oracle import ROOT, STACK
from unicorn.x86_const import UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_FPCW,UC_X86_REG_FPSW

# 후처리 전용 격리 Player/목록 메모리다. 기존 지도/풀과 겹치지 않는다.
ARENA=0x12000000
PROVIDERS=ARENA+0x1000
FACTORIES=ARENA+0x2000
OWNERS=ARENA+0x3000
# 각 판본의 실제 전역 주소/표/소유자 구조체 폭이다.
POST={
 'originals':dict(cost=0x59508c,cost_table=0x59ab50,counts=(0x5c94d0,0x5c98d0,0x5c9cd0),
    local=0x540c70,players=0x59534c,stride=0xb4,graphs=0x540414,invalid=0x540410,
    providers=(0x592f84,0x592f88,0x592f8c),factories=(0x55a4d4,0x55a4d8,0x55a4dc),
    dirty=0x55a4a4,pending=0x5caea4,provider_fn=0x472f70,factory_fn=0x44f770,owner_fn=0x490050),
 'originalCD':dict(cost=0x512114,counts=(0x5109a0,0x510da0,0x5111a0),
    local=0x50f6c8,players=0x50f834,stride=0xac,graphs=0x5207f8,invalid=0x5207f4,
    providers=(0x567648,0x56764c,0x567650),factories=(0x566fe0,0x566fe4,0x566fe8),
    dirty=0x520c10,pending=0x5134c8,provider_fn=0x48a360,factory_fn=0x45ec90,owner_fn=0x4071f0),
}


def u32(value):
    """명시적인 DWORD 감김을 입력 준비에만 사용한다."""
    return value&0xffffffff


class PostOracle(DisplayOracle):
    """실제 생성/표시/공간/후처리를 연결하고 모든 미허용 몸체/쓰기를 거부한다."""
    def __init__(self,edition):
        """새 읽기 전용 후처리 내보내기의 몸체 범위만 추가한다."""
        super().__init__(edition); self.post=POST[edition]
        self.mu.mem_map(ARENA,0x10000); self.effects=collections.Counter(); self.encodings=0
        with (ROOT/f'extracted/postpop/{edition}/functions.tsv').open(encoding='utf-8') as fp:
            # 불연속 함수 범위 사이의 코드는 허용하지 않는다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.facts.append(row)
                for part in row['ranges'].split(';'):
                    lo,hi=(int(v,16) for v in part.split('-')); self.allowed.append((lo,hi+1))

    def on_instruction(self,mu,address,size,data):
        """실제 목록 함수 도달을 상위 fixture 호출과 구분해 집계한다."""
        if hasattr(self,'post'):
            # 목록 도달은 상위 PostPop/Pop 수 안에 포함된다.
            for name in ('provider_fn','factory_fn','owner_fn'):
                if address==self.post[name]: self.effects[name]+=1
        super().on_instruction(mu,address,size,data)

    def start_post(self,control):
        """억제된 실제 Reset/Create 뒤 별도 후처리 쓰기만 추가한다."""
        self.start(control)
        self.write_ranges.extend([(ARENA,ARENA+0x10000),(self.post['cost'],self.post['cost']+4),
                                  (self.post['dirty'],self.post['dirty']+4)])
        # 세 Totalmade 표는 전체 DWORD 범위의 쓰기만 허용한다.
        for a in self.post['counts']: self.write_ranges.append((a,a+1024))
        for name in ('providers','factories'):
            address=self.post[name][2]; self.write_ranges.append((address,address+4))
        if self.edition=='originals': self.write_ranges.append((self.post['cost_table'],self.post['cost_table']+1024))

    def input_type(self,number,f1,f2,cost):
        """합성 타입/실제 float cost와 원본 loader의 비용 인코딩 접두 구간을 입력한다."""
        self.number=number
        self.type_space(number,f1,f2,1,1,1,1)
        typ=TYPES+number*self.creation['type_stride']
        self.mu.mem_write(typ+0xc4,struct.pack('<f',cost))
        # 별도 Squid 표시 헤더도 실제 표시 경로에서 사용한다.
        for i,frame in enumerate(((11,17,12,16),(19,23,-3,5))):
            offset=0x100+i*0x80
            self.mu.mem_write(SHAPE+8+i*8,struct.pack('<I',offset))
            self.mu.mem_write(SHAPE+offset-36,struct.pack('<2f',1,1))
            self.mu.mem_write(SHAPE+offset-12,struct.pack('<4h',*frame))
        if self.edition=='originals':
            self.registers(0); self.mu.reg_write(UC_X86_REG_FPCW,self.control)
            top=self.mu.reg_read(UC_X86_REG_FPSW)&0x3800
            self.mu.reg_write(UC_X86_REG_EBX,TYPES); self.mu.reg_write(UC_X86_REG_ESI,number*500)
            self.mu.reg_write(UC_X86_REG_EDI,number)
            self.mu.mem_write(STACK+0x10,struct.pack('<I',number*23))
            # loader 전체/파일 I/O를 실행했다고 세지 않는 fild→fadd→ftol→표 대입 접두 구간이다.
            self.execute(0x49c338,0x49c359,STACK); self.encodings+=1
            assert self.mu.reg_read(UC_X86_REG_FPSW)&0x3800==top
            assert self.mu.reg_read(UC_X86_REG_FPCW)==self.control

    def state(self,pc,fc,oc,initial,total,revision,seed,local,graphs,suppressed,depth):
        """원본 Player의 AI 포인터가 null이고 배치 선택이 없는 명시적인 상태를 만든다."""
        self.capacities=(pc,fc,oc); p=self.post
        self.mu.mem_write(ARENA,bytes(0x10000))
        # 원본 포인터/용량/count와 빈 AI 부착을 가진 9개의 Player 구조체다.
        for owner in range(9):
            values=[0x70000000+owner*100+i for i in range(oc)]
            if oc: self.mu.mem_write(OWNERS+owner*0x100,struct.pack('<'+'I'*oc,*values))
            self.mu.mem_write(ARENA+owner*p['stride'],struct.pack('<3I',OWNERS+owner*0x100,oc,min(initial,oc)))
        # 공급 목록은 첫 활성 항목에 현재 SID를 넣어 중복 경로를 검사한다.
        for address,capacity in ((PROVIDERS,pc),(FACTORIES,fc)):
            values=[self.sid if i==0 else 0x60000000+i for i in range(capacity)]
            if capacity: self.mu.mem_write(address,struct.pack('<'+'I'*capacity,*values))
        for globals_,address,capacity in ((p['providers'],PROVIDERS,pc),(p['factories'],FACTORIES,fc)):
            # 실제 목록 구조체의 포인터·capacity·count를 각각 설정한다.
            for a,v in zip(globals_,(address,capacity,min(initial,capacity))): self.mu.mem_write(a,struct.pack('<I',v))
        # 통계 표 전체를 다른 초기값으로 채워 대상 타입 외의 쓰기도 검사한다.
        for index,a in enumerate(p['counts']):
            values=[u32(seed+i*2654435761+index*17) for i in range(256)]
            self.mu.mem_write(a,struct.pack('<256I',*values))
        for a,v in [(p['cost'],u32(total)),(p['dirty'],revision),(p['players'],ARENA),(p['local'],local),
                    (p['graphs'],graphs),(p['invalid'],254),(p['pending'],0),(self.space['log'],suppressed),
                    (self.space['depth'],depth)]: self.mu.mem_write(a,struct.pack('<I',v))
        # 표시 활성도 실제 Pop/Unpop 시 함께 검사한다.
        self.view(640,480,0,0,65536,(0,0,640,480),0)

    def fill_post(self,owner,extra,state=4):
        """base fallback 확보 슬롯의 공간/owner/frame 필드를 명시적으로 입력한다."""
        self.slot(0,extra,state,20,20)
        address=POOL+self.sid*self.stride
        self.mu.mem_write(address+10,bytes([self.number]))
        self.mu.mem_write(address+(34 if self.edition=='originals' else 32),bytes([owner]))
        self.mu.mem_write(address+(30 if self.edition=='originals' else 28),bytes([17]))

    def output(self):
        """전체 통계 표·목록의 활성/미사용 DWORD·카운터·raw 슬롯·실제 dirty 표를 관찰한다."""
        p=self.post; pc,fc,oc=self.capacities
        cost=struct.unpack('<i',self.mu.mem_read(p['cost'],4))[0]
        revision=struct.unpack('<I',self.mu.mem_read(p['dirty'],4))[0]
        depth=struct.unpack('<I',self.mu.mem_read(self.space['depth'],4))[0]
        tables=[zlib.adler32(self.mu.mem_read(a,1024)) for a in p['counts']]
        data=bytearray(); counts=[]
        # inactive 영역도 C++ 배열과 같은 순서로 checksum을 낸다.
        for a,c in ((PROVIDERS,pc),(FACTORIES,fc)):
            data.extend(self.mu.mem_read(a,c*4))
        for key in ('providers','factories'): counts.append(struct.unpack('<I',self.mu.mem_read(p[key][2],4))[0])
        for owner in range(9):
            counts.append(struct.unpack('<I',self.mu.mem_read(ARENA+owner*p['stride']+8,4))[0])
            data.extend(self.mu.mem_read(OWNERS+owner*0x100,oc*4))
        return [cost,revision,depth,*tables,','.join(map(str,counts)),zlib.adler32(data),self.raw(),*self.dirty_output()]


def generate():
    """두 판본/정밀도의 실제 후처리·Pop/Unpop·재등록을 기대값으로 저장한다."""
    rows=['# 실제 공통 postPop 활성. 합성 타입/SHP, AI 미부착·배치 선택 없음. 대체 함수 없음.']
    counts=collections.Counter(); reports={}
    # 각 판본과 x87 정밀도를 독립 풀에서 검사한다.
    for edition in POST:
        oracle=PostOracle(edition)
        for control in (0x027f,0x037f):
            oracle.start_post(control); rows.append(f'Begin\t{edition}\t{control}\t{oracle.sid}')
            # 비용 소수/음수·DWORD 경계·full 목록·duplicate·추상/매몰·graph reset을 섞는다.
            for case in range(96):
                number=(74,148,169)[case%3]
                f1=(0,0x10000000,0x40000,0x800)[case%4]
                f2=(0,0x4200,0x10000,0x200)[(case//4)%4]
                cost=(0.,0.25,3.75,-0.5,600.,-12.25)[case%6]
                oracle.input_type(number,f1,f2,cost)
                rows.append(f'Type\t{number}\t{f1}\t{f2}\t{bits(cost)}')
                args=(case%4,(case//3)%4,(case//5)%4,case%2,
                      (-1,123,2147483640,-2147483640)[case%4],
                      (0,0xffffffff)[case%2],u32(case*31337),case%9,case%2,int(case%13==0),3)
                oracle.state(*args); rows.append('State\t'+'\t'.join(map(str,args)))
                owner=(case//2)%9; extra=(0,1,4,8,9,128)[(case//3)%6]
                oracle.fill_post(owner,extra); rows.append(f'Fill\t{owner}\t{extra}')
                flags=(1,0,2,4,0x201,0x2000001)[(case//5)%6]
                if f1&0x800 or (f2&0x50444208 and case%2): flags|=0x2000000
                # 같은 SID의 명시적 postPop 반복은 type 통계/목록의 원본 비멱등성을 검사한다.
                for _repeat in range(2):
                    oracle.invoke(oracle.space['post'],[flags],POOL+oracle.sid*oracle.stride,4)
                    rows.append('\t'.join(map(str,['PostPop',flags,*oracle.output()]))); counts['PostPop']+=1
                # 일반 walker/provider는 실제 Pop→Unpop→재등록과 공통 표시까지 연결한다.
                if f2==0x10000 and f1&0x800==0:
                    # 무조건 처음 Pop을 시험하되 후처리 억제 입력도 원본과 동일하게 유지한다.
                    oracle.invoke(oracle.space['pop'],[bits(30.25),bits(35.5),0],POOL+oracle.sid*oracle.stride,12)
                    rows.append('\t'.join(map(str,['Pop',0,bits(30.25),bits(35.5),*oracle.output()]))); counts['Pop']+=1
                    oracle.invoke(oracle.creation['unpop'],[0],POOL+oracle.sid*oracle.stride,4)
                    rows.append('\t'.join(map(str,['Unpop',0,*oracle.output()]))); counts['Unpop']+=1
                    oracle.invoke(oracle.space['pop'],[bits(40.75),bits(45.25),0x2000],POOL+oracle.sid*oracle.stride,12)
                    rows.append('\t'.join(map(str,['Pop',0x2000,bits(40.75),bits(45.25),*oracle.output()]))); counts['Pop']+=1
        reports[edition]=dict(binary_sha256=oracle.sha256,function_ranges=oracle.facts,
            actual_effect_calls=dict(oracle.effects),actual_lifecycle_calls=dict(oracle.actual_calls),
            actual_display_calls=dict(oracle.display_calls),cost_encode_segments=oracle.encodings,assert_calls=oracle.assertions)
        print(f'{edition}: 실제 공통 postPop 효과 완료',flush=True)
    fixture=ROOT/'cpppj/tests/fixtures/postpop-x86.tsv'
    fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    report=dict(schema=1,method='격리 실제 x86; 공통 postPop/표시 활성; 비전투·AI 미부착·배치 선택 없음',
        public_calls=dict(counts),total_calls=sum(counts.values()),sequences=4,editions=reports,stubbed=[],
        fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        dependencies={name:hashlib.sha256((ROOT/'tools'/name).read_bytes()).hexdigest() for name in
          ('decomp_oracle.py','decomp_sid_oracle.py','decomp_creation_oracle.py','decomp_pop_oracle.py','decomp_display_oracle.py')},
        limitations=['합성 타입/SHP·기존 확보 풀; graph flood/AI/배치 선택/건설·SP 차감/게임 완주 미복원',
          '패치 cost 인코딩은 loader의 접두 구간 실행이며 전체 로더 호출로 세지 않는다',
          '전체 표·목록은 Adler-32, raw 슬롯과 dirty 표는 모든 바이트/항목을 대조한다'])
    (ROOT/'cpppj/recovery-postpop-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'postPop x86 fixture: {sum(counts.values())}회 {dict(counts)}')


def verify():
    """원본 PE/도구/부모 도구/fixture SHA와 공개 호출 수를 실행 없이 확인한다."""
    r=json.loads((ROOT/'cpppj/recovery-postpop-evidence.json').read_text(encoding='utf-8'))
    assert r['tool_sha256']==hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    # 원본 파일은 읽기만 한다.
    for ed,data in r['editions'].items():
        p=ROOT/('originals/Netstorm.exe' if ed=='originals' else 'originalCD/NETSTORM.EXE')
        assert data['binary_sha256']==hashlib.sha256(p.read_bytes()).hexdigest() and data['assert_calls']==0
    for name,digest in r['dependencies'].items(): assert digest==hashlib.sha256((ROOT/'tools'/name).read_bytes()).hexdigest()
    p=ROOT/'cpppj/tests/fixtures/postpop-x86.tsv'; assert r['fixture_sha256']==hashlib.sha256(p.read_bytes()).hexdigest()
    counts=collections.Counter(line.split('\t')[0] for line in p.read_text(encoding='utf-8').splitlines())
    assert {key:counts[key] for key in r['public_calls']}==r['public_calls']
    assert sum(r['public_calls'].values())==r['total_calls'] and r['stubbed']==[]
    print(f'postPop 근거 SHA/호출 수 확인: {r["total_calls"]}회')


if __name__=='__main__':
    # --verify는 새 기계어 실행을 하지 않는다.
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify',action='store_true'); args=parser.parse_args()
    verify() if args.verify else generate()
