#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""raw SID의 일반 non-void Unpop→Take/반납을 원본 x86과 창 없이 대조한다."""
import argparse
import collections
import csv
import hashlib
import json
import random
import struct
import zlib
from decomp_derived_oracle import DerivedOracle
from decomp_creation_oracle import CREATION
from decomp_sid_oracle import SidOracle, POOL
from decomp_oracle import ROOT, SCRATCH, STOP, STACK
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_FPCW,UC_X86_REG_FPSW

# 최대 SID 풀과 겹치지 않는 해시/spot/합성 SHP 주소다.
HEADS=0x11000000
SPOTS=HEADS+0x30000
SHAPE=SCRATCH+0x360000
# 네 단계 해시 머리 수·원본 보드 크기·재현 시드다.
HEAD_COUNT=86272
SCALES=(1,2,4,16)
SEED=0xA11078
# 원본 표시 갱신은 비활성화하지만 가상 함수/Renderer 조기 반환까지 실제 실행한다.
SPACE={
 'originals':{'hash':0x542508,'surface':0x5c84bc,'spots':0x5c7c44,'board':0x531928,
              'debug':0x5e4794,'display_off':0x59a8b0,'extra':40,'frame':36,'level':33,
              'first':0x4ad470,'update88':0x4ad500,'update8c':0x4ad670,'foot':0x1d4},
 'originalCD':{'hash':0x5670c0,'surface':0x52d590,'spots':0x52fe48,'board':0x52e9a8,
              'debug':0x540a24,'display_off':0x52039c,'extra':35,'frame':34,'level':31,
              'first':0x4ae0f0,'update88':0x4ae380,'update8c':0x4ae3e0,'foot':0x1b4},
}


class UnpopOracle(DerivedOracle):
    """기존 생성자/SID 범위에 실제 공간/표시 비활성 함수 몸체만 더한다."""
    def __init__(self,edition):
        """두 PE의 새 함수 범위와 서로 분리된 공간 메모리를 준비한다."""
        super().__init__(edition)
        self.space=SPACE[edition]
        self.unpop_states=collections.Counter()
        self.display_calls=collections.Counter()
        self.mu.mem_map(HEADS,0x50000)
        self.bases=[]
        address=HEADS
        # 원본 네 단계 배열을 원래 크기대로 배치한다.
        for scale in SCALES:
            self.bases.append(address); address+=(256//scale)**2*2
        with (ROOT/f'extracted/lifecycle/{edition}/functions.tsv').open(encoding='utf-8') as fp:
            for row in csv.DictReader(fp,delimiter='\t'):
                self.facts.append(row)
                for part in row['ranges'].split(';'):
                    lo,hi=(int(v,16) for v in part.split('-'))
                    self.allowed.append((lo,hi+1))

    def on_instruction(self,mu,address,size,data):
        """non-void도 실제 실행하며 원본 assert/허용 범위 밖 호출은 거부한다."""
        if address==self.creation['unpop']:
            state=mu.mem_read(mu.reg_read(UC_X86_REG_ECX)+11,1)[0]
            self.unpop_states['free' if state&1 else ('void' if state&4 else 'active')]+=1
        if address in (self.space['update88'],self.space['update8c']):
            self.display_calls[f'{address:08x}']+=1
        # 기존 CreationOracle의 void 전용 제한 대신 SID의 같은 실제 코드/assert 제한을 쓴다.
        SidOracle.on_instruction(self,mu,address,size,data)

    def setup(self,capacity,server,control):
        """malloc 없이 기존 풀을 실제 Reset하고 표시/debug 전역만 비활성 입력으로 지정한다."""
        super().setup(capacity,server,False)
        self.control=control
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        s=self.space
        for ptr,value in [(s['surface'],self.bases[0]),(s['spots'],SPOTS),(s['board'],256),
                          (s['debug'],0),(s['display_off'],1)]:
            self.mu.mem_write(ptr,struct.pack('<I',value))
        self.mu.mem_write(s['hash'],struct.pack('<7I',0,1,256,*self.bases))
        self.write_ranges.extend([(HEADS,HEADS+HEAD_COUNT*2),(SPOTS,SPOTS+65536)])
        self.reset_space(0)

    def reset_space(self,seed):
        """Pop을 복원했다고 가정하지 않고 기존 배치 상태를 합성 입력으로 놓는다."""
        self.mu.mem_write(HEADS,bytes(HEAD_COUNT*2))
        self.mu.mem_write(SPOTS,bytes([seed])*65536)

    def type_input(self,number,f1,f2,hp,z,ctor,width,height):
        """합성 타입의 생성자/발자국/프레임을 원본 주소와 연결한다."""
        super().type_input(number,f1,f2,hp,z,ctor)
        typ=SCRATCH+0x1000+number*self.creation['type_stride']
        # 표시 비활성 Renderer는 SHP를 역참조하지 않지만 base frameCheck 입력은 정상으로 둔다.
        self.mu.mem_write(typ+0xdc,struct.pack('<I',SHAPE))
        self.mu.mem_write(typ+0x114,struct.pack('<I',4))
        self.mu.mem_write(typ+self.space['foot'],struct.pack('<2I',width,height))

    def input_slot(self,sid,seed,state,number,vtable,next_sid,x,y,extra,level):
        """기존 활성 객체 raw 바이트를 명시적으로 입력한다. actual Pop/월드 생성은 아니다."""
        self.fill(sid,seed,state,number)
        slot=POOL+sid*self.stride
        self.mu.mem_write(slot,struct.pack('<I',vtable))
        self.mu.mem_write(slot+4,struct.pack('<H',next_sid))
        self.mu.mem_write(slot+14,struct.pack('<2f',x,y))
        self.mu.mem_write(slot+self.space['extra'],bytes([extra]))
        self.mu.mem_write(slot+self.space['level'],bytes([level]))
        self.mu.mem_write(slot+self.space['frame'],bytes(4 if self.edition=='originals' else 1))

    def head(self,level,x,y,sid):
        """기존 배치의 기준점 버킷 머리만 입력한다. 원본의 절삭/단계 나눗셈을 사용한다."""
        scale=SCALES[level]
        self.mu.mem_write(self.bases[level]+(int(y)//scale*(256//scale)+int(x)//scale)*2,struct.pack('<H',sid))

    def snapshot(self,sid,result):
        """전체 raw 풀/기록/공간 지도와 대상 슬롯의 모든 바이트를 저장한다."""
        base=super().snapshot(sid)
        logs=zlib.adler32(b''.join(bytes(self.mu.mem_read(a,160)) for a in self.spec['logs']))
        return [result,*base[1:8],logs,zlib.adler32(self.mu.mem_read(HEADS,HEAD_COUNT*2)),
                zlib.adler32(self.mu.mem_read(SPOTS,65536)),base[8]]

    def step_case(self,name,number,arg,sid):
        """실제 일반 가상 호출과 정상 EIP/ESP/x87 복구를 확인한다. 대체 함수는 없다."""
        self.mu.reg_write(UC_X86_REG_FPCW,self.control)
        top=self.mu.reg_read(UC_X86_REG_FPSW)&0x3800
        result=sid
        if name in ('Create','Take'):
            result=self.creation_step(name,number,arg)[0]
            sid=result
        elif name=='Release':
            SidOracle.step(self,'Release',sid)
        else:
            self.registers(POOL+sid*self.stride)
            self.mu.mem_write(STACK,struct.pack('<2I',STOP,arg))
            first=name=='FirstPop'
            self.execute(self.space['first'] if first else self.creation['unpop'],STOP,STACK+(4 if first else 8))
            if first: result=self.mu.reg_read(UC_X86_REG_EAX)
        assert self.mu.reg_read(UC_X86_REG_FPCW)==self.control
        assert self.mu.reg_read(UC_X86_REG_FPSW)&0x3800==top
        return sid,self.snapshot(sid,result)


def bits(value):
    """TSV는 float 값을 반올림하지 않고 실제 32비트 입력으로 보관한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def generate():
    """두 판본의 체인 머리/중간/꼬리·4단계·Take·반납·firstPop 비트를 검증한다."""
    original=json.loads((ROOT/'cpppj/recovery-derived-evidence.json').read_text(encoding='utf-8'))
    catalog=collections.defaultdict(dict)
    # 검증된 주소 표를 읽기만 하며 이 도구의 실제 PE vtable 판독과 대조한다.
    for line in (ROOT/'cpppj/tests/fixtures/constructors-x86.tsv').read_text(encoding='utf-8').splitlines():
        if line and not line.startswith('#'):
            edition,number,address=line.split('\t'); catalog[edition][int(number)]=int(address)
    rows=['# 실제 일반 non-void Unpop/Take/반납·공통 firstPop. 표시 비활성, 실제 Pop/월드 미실행.']
    table_rows=['# 실제 PE vtable의 update88/update8c와 공통 firstPop. 메타데이터 행은 호출 수와 별도.']
    lines=['// 실제 PE 가상 주소 비교로 생성한 표시 비활성 Unpop 지원 표. 호스트 포인터가 아니다.']
    counts=collections.Counter(); reports={}; begins=0
    # 각 PE는 별도 Unicorn 메모리를 쓰며 원본 파일을 수정하지 않는다.
    for edition,item in original['editions'].items():
        oracle=UnpopOracle(edition); rng=random.Random(SEED)
        vtables={r['address']:r['vtable'] for r in item['recipes']}
        metadata={}
        # 동일 vtable은 한 번만 기록하고 update8c의 update88 꼬리 호출도 검증한다.
        for vtable in sorted({oracle.creation['vtable'],*vtables.values()}):
            u88,u8c,first=[struct.unpack('<I',oracle.mu.mem_read(vtable+offset,4))[0] for offset in (0x88,0x8c,0x30)]
            assert first==oracle.space['first']
            mask=int(u88==oracle.space['update88'])*(1+2*int(u8c==oracle.space['update8c']))
            metadata[vtable]=mask
            table_rows.append(f'{edition}\t{vtable}\t{u88}\t{u8c}\t{first}\t{mask}')
        lines.append('// '+edition+' 판본별 공통 표시 가상 함수의 지원 비트다.')
        lines.append(f'constexpr std::array<DisplayVtable,{len(metadata)}> k{"Patch" if edition=="originals" else "Cd"}DisplayVtables{{{{')
        lines.extend('    {'+f'0x{vtable:08x}U,{mask}'+'},' for vtable,mask in metadata.items())
        lines.append('}};')
        supported=[(number,address,vtables.get(address,oracle.creation['vtable'])) for number,address in catalog[edition].items()
            if number>=70 and (not address or address in vtables) and metadata[vtables.get(address,oracle.creation['vtable'])]==3]
        # 512회 firstPop은 각 extra 바이트 값을 양 판본의 실제 명령으로 검사한다.
        def begin(capacity,server,control):
            nonlocal begins
            rows.append(f'Begin\t{edition}\t{capacity}\t{int(server)}\t{control}')
            oracle.setup(capacity,server,control); begins+=1
        # 모든 기대값은 원본 호출 출력이며 Python은 C++ 구현 모델을 만들지 않는다.
        def step(name,number,arg,sid):
            actual_sid,result=oracle.step_case(name,number,arg,sid)
            rows.append('\t'.join(map(str,['Step',name,number,arg,actual_sid,*result])))
            counts[name]+=1
            return actual_sid
        def typ(number,f1,f2,ctor,width,height):
            values=(number,f1,f2,1234,-17,ctor,width,height)
            rows.append('\t'.join(map(str,['Type',*values]))); oracle.type_input(*values)
        def fill(sid,seed,state,number,vtable,next_sid,x,y,extra,level):
            rows.append('\t'.join(map(str,['Fill',sid,seed,state,number,vtable,next_sid,bits(x),bits(y),extra,level])))
            oracle.input_slot(sid,seed,state,number,vtable,next_sid,x,y,extra,level)
        def head(level,x,y,sid):
            rows.append(f'Head\t{level}\t{bits(x)}\t{bits(y)}\t{sid}'); oracle.head(level,x,y,sid)
        begin(32768,False,0x027f); typ(74,0,0,0,1,1)
        sid=step('Create',74,2,0)
        for extra in range(256):
            fill(sid,19,4,74,oracle.creation['vtable'],0,20.,20.,extra,0)
            step('FirstPop',74,0,sid)
        # 크기/권한/정밀도 조합마다 기존 배치의 raw 공간 입력을 별도로 구성한다.
        for capacity in (32768,65535):
            for server in (False,True):
                for control in (0x027f,0x037f):
                    begin(capacity,server,control); typ(74,0,0,0,1,1)
                    peers=[step('Create',74,2,0),step('Create',74,2,0)]
                    for case in range(24):
                        number,ctor,vtable=supported[case%len(supported)]
                        width,height=1+case%3,1+(case//3)%3
                        f1=0x800 if case%5==0 else 0
                        f2=(0,0x11,0x80,0x10000,0x20000,6|0x50444200)[case%6]
                        extra=(0,1,4,8,9,0x88)[case%6]
                        typ(number,f1,f2,ctor,width,height)
                        sid=(50000 if case%2==0 else 65534) if capacity==65535 else oracle.spec['server_first']+700+case
                        step('Take',number,sid,sid)
                        x,y=(30.99995 if case%2 else 30.25),(45.5 if case%3 else 45.)
                        level=case%4; mode=case%3; seed=rng.randrange(256)
                        def place():
                            rows.append(f'Space\t{seed}'); oracle.reset_space(seed)
                            order=[sid,*peers] if mode==0 else ([peers[0],sid,peers[1]] if mode==1 else [*peers,sid])
                            for position,current in enumerate(order):
                                next_sid=order[position+1] if position+1<len(order) else 0
                                target=current==sid
                                fill(current,seed,(0,2,0x80)[case%3],number if target else 74,vtable if target else oracle.creation['vtable'],
                                     next_sid,x,y,extra if target else 0,3-level)
                            if edition=='originalCD' and case==23:
                                # CD는 활성 객체가 체인에 없어도 assert 없이 void/표시 비활성 경로를 끝낸다.
                                oracle.mu.mem_write(POOL+peers[0]*oracle.stride+4,struct.pack('<H',peers[1]))
                                oracle.mu.mem_write(POOL+peers[1]*oracle.stride+4,bytes(2))
                                rows.append(f'Next\t{peers[0]}\t{peers[1]}'); rows.append(f'Next\t{peers[1]}\t0')
                                head(level,x,y,peers[0])
                            else:
                                head(level,x,y,order[0])
                        place(); step('FirstPop',number,0,sid)
                        step('Take',number,sid,sid); step('Unpop',number,0,sid)
                        place(); step('Unpop',number,0x2000 if case&8 else 0,sid)
                        step('Unpop',number,0,sid); step('Release',number,0,sid); step('Unpop',number,0,sid)
        reports[edition]={'binary_sha256':oracle.sha256,'function_ranges':oracle.facts,
            'vtable_rows':len(metadata),'nonvoid_unpop_calls':oracle.unpop_states['active'],
            'unpop_states':dict(oracle.unpop_states),'display_calls':dict(oracle.display_calls),
            'assert_reports':oracle.assertions,'max_instructions_observed':oracle.max_instructions,
            'missing_chain_inputs':16 if edition=='originalCD' else 0}
        print(f'{edition}: 일반 공간 해제/Take/반납·firstPop 완료',flush=True)
    artifacts={'cpppj/tests/fixtures/unpop-x86.tsv':'\n'.join(rows)+'\n',
        'cpppj/tests/fixtures/unpop-vtables.tsv':'\n'.join(table_rows)+'\n',
        'cpppj/src/o/UnpopVtables.inc':'\n'.join(lines)+'\n'}
    for name,body in artifacts.items(): (ROOT/name).write_text(body,encoding='utf-8',newline='\n')
    sources=['tools/decomp_unpop_oracle.py','tools/decomp_derived_oracle.py','tools/decomp_creation_oracle.py',
             'tools/decomp_sid_oracle.py','tools/decomp_oracle.py','tools/ghidra/ExportCreation.java']
    report={'schema':1,'classification':'scoped-raw-nonvoid-unpop-display-disabled','cases':dict(counts),
        'total_cases':sum(counts.values()),'begins':begins,'setup_resets':begins,'editions':reports,
        'artifact_sha256':{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in artifacts},
        'source_sha256':{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in sources},
        'limits':['실제 SID/파생 ctor/공통 firstPop/Unpop/가상 표시 비활성 반환/Take/void 반납; 대체 함수 없음',
                  '합성 타입·기존 배치/풀/해시/spot; actual Pop·원본 파일/월드 초기화 없음',
                  '일반 자산 또는 매몰: 섬/다리·건물 부착/영역 효과·다른 화면 override 제외',
                  '표시/debug 비활성; dirty/화면·파생 삭제/의존 객체·네트워크/참조 수명 미복원',
                  '정상 x87 두 정밀도; 전체 슬롯 직접 비교·전체 풀/기록/해시/spot Adler-32',
                  '게임 진입점/원본 프로세스/GUI/OS API 미실행']}
    (ROOT/'cpppj/recovery-unpop-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'cases':dict(counts),'total_cases':report['total_cases'],'begins':begins}),flush=True)


def verify():
    """원본·도구·fixture/주소 표 SHA-256과 실제 호출 행 수를 확인한다."""
    report=json.loads((ROOT/'cpppj/recovery-unpop-evidence.json').read_text(encoding='utf-8'))
    for group in ('artifact_sha256','source_sha256'):
        for name,sha in report[group].items(): assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==sha,name
    for edition,item in report['editions'].items():
        path=ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE')
        assert hashlib.sha256(path.read_bytes()).hexdigest()==item['binary_sha256'] and item['assert_reports']==0
    counts=collections.Counter(line.split('\t')[1] for line in (ROOT/'cpppj/tests/fixtures/unpop-x86.tsv').read_text(encoding='utf-8').splitlines() if line.startswith('Step\t'))
    assert dict(counts)==report['cases'] and sum(counts.values())==report['total_cases']
    print(json.dumps({'verified':True,'cases':dict(counts),'total_cases':sum(counts.values())}),flush=True)


# 명시적 호출에서만 격리 원본 함수 실행이나 고정 기록 검증을 수행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--verify',action='store_true')
    args=parser.parse_args(); verify() if args.verify else generate()
