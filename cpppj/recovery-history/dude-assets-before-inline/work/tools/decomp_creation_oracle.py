#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""창/게임 진입점 없이 생성자 표와 기본 Squid 생성/Take/가상 초기화의 실제 x86을 검증한다."""
import argparse
import collections
import csv
import hashlib
import json
import random
import struct
import zlib
from decomp_sid_oracle import SidOracle, POOL, SPECS
from decomp_oracle import ROOT, SCRATCH, STOP, STACK
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

# SID 풀과 겹치지 않는 합성 타입 배열의 주소와 재현 입력 시드다.
TYPES=SCRATCH+0x1000
SEED=0xC1078
# 타입 배열·기본 vtable·메서드·mana 약화 옵션·dais·금지 surface의 판본별 주소/번호다.
CREATION={
    'originals':{'type_global':0x59ab20,'type_stride':500,'count':188,'ctor_offset':0x1d0,
        'vtable':0x501dd0,'mana':158,'dais':165,'fake':162,'weak_global':0x5ca8f0,
        'entries':{'Create':0x4af530,'Take':0x4af610,'PostCreate':0x4b0c60,'PostTake':0x4ad4a0},
        'catalog_segments':[(0x49eca7,0x49ed18),(0x49ed5b,0x49fa47)],'catalog_globals':[],'unpop':0x4afe50},
    'originalCD':{'type_global':0x51c960,'type_stride':468,'count':171,'ctor_offset':0x1b0,
        'vtable':0x5058d8,'mana':158,'dais':165,'fake':-1,'weak_global':0x511c90,
        'entries':{'Create':0x4ab390,'Take':0x4ab440,'PostCreate':0x4ae040,'PostTake':0x4ae120},
        'catalog_segments':[(0x4435bb,0x443642),(0x443674,0x444177)],
        'catalog_globals':[0x51c6e8,0x51c6ec],'unpop':0x4ad0b0},
}


class CreationOracle(SidOracle):
    """기존 SID의 실제 명령에 새 Ghidra 몸체만 더하고 대체 함수 없이 실행한다."""
    def __init__(self,edition):
        """함수 범위와 판본 메타데이터를 준비한다."""
        super().__init__(edition)
        self.creation=CREATION[edition]
        self.unpop_calls=0
        self.catalog_writes=0
        with (ROOT/f'extracted/creation/{edition}/functions.tsv').open(encoding='utf-8') as fp:
            # 읽기 전용 Ghidra가 확인한 새 함수의 실제 몸체만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.facts.append(row)
                for part in row['ranges'].split(';'):
                    lo,hi=(int(value,16) for value in part.split('-'))
                    self.allowed.append((lo,hi+1))

    def on_instruction(self,mu,address,size,data):
        """사용 중 Take의 기존 Unpop도 실제 void 조기 반환 경로인지 확인한다."""
        if address==self.creation['unpop']:
            slot=mu.reg_read(UC_X86_REG_ECX)
            if not mu.mem_read(slot+11,1)[0]&4:
                raise RuntimeError('공간 해제가 필요한 Take는 이번 검증 범위 밖입니다')
            self.unpop_calls+=1
        super().on_instruction(mu,address,size,data)

    def on_write(self,mu,access,address,size,value,data):
        """타입 표는 생성자 표 복원 구간에서만 변경할 수 있다."""
        if TYPES<=address<TYPES+self.creation['count']*self.creation['type_stride']:
            self.catalog_writes+=1
        super().on_write(mu,access,address,size,value,data)

    def registers(self,this=0):
        """호출마다 일반 레지스터와 DF를 초기화한다."""
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
                         UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):
            self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ECX,this)
        self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.instructions=0

    def execute(self,start,end,expected_esp):
        """명령/시간 제한과 종료 EIP/ESP를 확인한다. OS API는 몸체 허용 목록 밖이다."""
        self.mu.emu_start(start,end,timeout=30000000,count=10000000)
        self.max_instructions=max(self.max_instructions,self.instructions)
        if self.mu.reg_read(UC_X86_REG_EIP)!=end or self.mu.reg_read(UC_X86_REG_ESP)!=expected_esp:
            raise RuntimeError(f'생성 경로 반환/구간 종료 오류: {self.edition} {start:08x}')

    def catalog(self):
        """파일 로딩/이름 복사를 제외한 두 생성자 대입 구간만 실제 실행한다."""
        c=self.creation
        self.mu.mem_write(TYPES,bytes(c['count']*c['type_stride']))
        self.mu.mem_write(c['type_global'],struct.pack('<I',TYPES))
        self.write_ranges=[(TYPES,TYPES+c['count']*c['type_stride']),(0x30000000,0x30010000)]
        self.write_ranges.extend((a,a+4) for a in c['catalog_globals'])
        self.registers()
        # 프로세스 이름 복사 구간은 실행하지 않는다. 직전 레지스터 상태는 두 번째 구간에 이어진다.
        for start,end in c['catalog_segments']:
            self.execute(start,end,STACK)
        values=[struct.unpack('<I',self.mu.mem_read(TYPES+i*c['type_stride']+c['ctor_offset'],4))[0]
                for i in range(c['count'])]
        expected=122 if self.edition=='originals' else 105
        assert sum(bool(v) for v in values)==expected and self.catalog_writes==expected
        return values

    def setup(self,capacity,server,weak):
        """소진 전 SID와 합성 타입 필드를 구성한다. 런타임 중 타입 쓰기는 금지한다."""
        self.begin(capacity,server)
        self.step('Reset')
        self.mu.mem_write(self.creation['weak_global'],struct.pack('<I',int(weak)))
        self.mu.mem_write(self.creation['type_global'],struct.pack('<I',TYPES))

    def type_input(self,number,flags1,flags2,hp,zorder,constructor):
        """base 가상 메서드가 읽는 타입 입력만 지정한다. 파생 생성자는 실행하지 않는다."""
        c=self.creation
        address=TYPES+number*c['type_stride']
        for offset,value in [(0,hp),(0xe8,flags1),(0xec,flags2),(0x104,zorder),(c['ctor_offset'],constructor)]:
            self.mu.mem_write(address+offset,struct.pack('<I',value&0xffffffff))

    def creation_step(self,name,type_id,arg):
        """cdecl 생성/Take 또는 ECX base 초기화를 정상 반환까지 실행한다."""
        direct=name.startswith('Post')
        self.registers(POOL+arg*self.stride if direct else 0)
        self.mu.mem_write(STACK,struct.pack('<3I',STOP,type_id,arg))
        self.execute(self.creation['entries'][name],STOP,STACK+4)
        result=arg if direct else (self.mu.reg_read(UC_X86_REG_EAX)-POOL)//self.stride
        if not direct and self.mu.reg_read(UC_X86_REG_EAX)!=POOL+result*self.stride:
            raise RuntimeError('반환 포인터가 SID 슬롯 시작이 아닙니다')
        glob=self.spec['globals']
        free,cursor=[struct.unpack('<I',self.mu.mem_read(glob[i],4))[0] for i in (5,7)]
        heads=[struct.unpack('<H',self.mu.mem_read(POOL+i,2))[0] for i in (4,18)]
        tails=[(struct.unpack('<I',self.mu.mem_read(glob[i],4))[0]-POOL)//self.stride for i in (2,4)]
        return [result,free,cursor,heads[0],tails[0],heads[1],tails[1],
                zlib.adler32(self.mu.mem_read(POOL,self.capacity*self.stride)),
                bytes(self.mu.mem_read(POOL+result*self.stride,self.stride)).hex()]


def generate():
    """판본별 실제 생성자 표와 base 생성/Take·서버 HP·dais abstract 분기를 저장한다."""
    rows=['# 합성 타입 입력 + 실제 base 생성/Take/초기화. 파생 생성자와 non-void Unpop은 미실행.']
    catalog_rows=['# 판본별 생성자 대입 두 구간의 실제 x86 결과. 전체 RiftType 초기화 호출이 아니다.']
    catalogs={}
    counts=collections.Counter()
    reports={}
    # 두 PE를 별도 주소 공간에 복사한다. 원본 파일은 읽기만 한다.
    for edition,c in CREATION.items():
        oracle=CreationOracle(edition)
        values=oracle.catalog()
        catalogs[edition]=values
        catalog_rows.extend(f'{edition}\t{i}\t{value}' for i,value in enumerate(values))
        rng=random.Random(SEED)
        for server in (False,True):
            rows.append(f'Begin\t{edition}\t32768\t{int(server)}\t1')
            oracle.setup(32768,server,True)
            # 매 입력을 타입 필드로 기록하며 생성자를 0으로 둔 base 경로만 dispatch한다.
            def type_row(number,f1,f2,hp,z,ctor=0):
                rows.append('\t'.join(map(str,['Type',number,f1,f2,hp,z,ctor])))
                oracle.type_input(number,f1,f2,hp,z,ctor)
            # 기대값은 원본 실행 결과이고 C++ 구현을 호출하지 않는다.
            def step(name,number,arg):
                result=oracle.creation_step(name,number,arg)
                rows.append('\t'.join(map(str,['Step',name,number,arg,*result])))
                counts[name]+=1
                return result[0]
            # 모든 서버/클라이언트·예측 flags 조합과 HP/깊이 경계값을 섞는다.
            for i in range(48):
                number=74 if i%2==0 else 80
                type_row(number,16 if i%3 else 0,0,(-1,0,1,65535,65536,0x12345678)[i%6],rng.randrange(-512,513))
                flags=(0,1,2,3)[i%4] if server else (1,2,3)[i%3]
                sid=step('Create',number,flags)
                # 전체 payload를 합성 입력으로 오염시킨 뒤 두 base 가상 메서드의 정확한 쓰기만 대조한다.
                seed=rng.randrange(256)
                state=(4,6,0x84,0xe4)[i%4]
                rows.append(f'Fill\t{sid}\t{seed}\t{state}\t{number}\t{c["vtable"]}')
                oracle.fill(sid,seed,state,number)
                oracle.mu.mem_write(POOL+sid*oracle.stride,struct.pack('<I',c['vtable']))
                step('PostCreate',number,sid)
                step('PostTake',number,sid)
                if sid>=oracle.spec['server_first']:
                    step('Take',number,sid)
            # free 서버 슬롯, 예측 머리/최종 슬롯도 Take 자체의 명시적인 범위 안이다.
            for sid in (oracle.spec['server_first']+300,oracle.spec['predictable'],32767):
                type_row(74,16,0,1234,-127)
                step('Take',74,sid)
                step('Take',74,sid)
            # mana의 int /4는 음수에서 0 방향 절삭한다. dais는 abstract 비트가 HP 필요성을 정한다.
            type_row(74,16,0,1234,-127)
            step('Take',74,oracle.spec['server_first']+350)
            for number in (c['mana'],c['dais']):
                for hp in (-2147483648,-7,-1,0,1,7,2147483647):
                    type_row(number,16,0x400000 if number==c['dais'] else 0,hp,-300,values[number])
                    sid=oracle.spec['server_first']+350
                    for abstract in (0,1):
                        rows.append(f'Fill\t{sid}\t{abstract}\t4\t{number}\t{c["vtable"]}')
                        oracle.fill(sid,abstract,4,number)
                        oracle.mu.mem_write(POOL+sid*oracle.stride,struct.pack('<I',c['vtable']))
                        oracle.mu.mem_write(POOL+sid*oracle.stride+(40 if edition=='originals' else 35),bytes([abstract]))
                        rows.append(f'Abstract\t{sid}\t{abstract}')
                        step('PostCreate',number,sid)
                        step('PostTake',number,sid)
        reports[edition]={'binary_sha256':oracle.sha256,'function_ranges':oracle.facts,
            'catalog_segments':c['catalog_segments'],'constructor_bindings':oracle.catalog_writes,
            'assert_reports':oracle.assertions,'void_unpop_calls':oracle.unpop_calls,
            'max_instructions_observed':oracle.max_instructions}
        print(f'{edition}: 생성자 표/기본 생성/Take 완료',flush=True)
    fixture=ROOT/'cpppj/tests/fixtures/creation-x86.tsv'
    catalog=ROOT/'cpppj/tests/fixtures/constructors-x86.tsv'
    fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    catalog.write_text('\n'.join(catalog_rows)+'\n',encoding='utf-8',newline='\n')
    # raw 원본 주소는 기록값이다. C++에서 함수/호스트 포인터로 캐스팅하지 않는다.
    lines=['// 실제 x86 생성자 대입 구간에서 생성한 판본별 주소 표. 호스트 실행 주소로 사용하지 않는다.']
    for edition,values in catalogs.items():
        lines.append('// '+edition+' 타입 번호 순서의 생성자 주소. 0은 base Squid fallback이다.')
        lines.append(f'constexpr std::array<std::uint32_t,{len(values)}> k{"Patch" if edition=="originals" else "Cd"}Constructors{{')
        for i in range(0,len(values),8):
            lines.append('    '+','.join(f'0x{v:08x}U' for v in values[i:i+8])+',')
        lines.append('};')
    generated=ROOT/'cpppj/src/o/TypeConstructors.inc'
    generated.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    sources=['tools/decomp_creation_oracle.py','tools/decomp_sid_oracle.py','tools/decomp_oracle.py','tools/ghidra/ExportCreation.java']
    report={'schema':1,'classification':'scoped-base-creation-and-void-take','cases':dict(counts),
        'total_cases':sum(counts.values()),'setup_resets':4,'catalog_rows':359,'editions':reports,
        'artifact_sha256':{str(p.relative_to(ROOT)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest()
                           for p in (fixture,catalog,generated)},
        'source_sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sources},
        'limits':['합성 타입 필드; 생성은 constructor=0인 base 경로만',
            'Take는 free 또는 이미 void인 base vtable만; 실제 Unpop 조기 반환, 대체 함수 없음',
            '생성자 표는 파일 로딩/프로세스 이름 복사를 제외한 대입 두 구간만 실제 실행',
            '가상 초기화 결과 슬롯은 모든 바이트 직접 비교; 풀 전체는 Adler-32',
            '파생 생성자/공간 해제/네트워크 패킷/GUI/게임 진입점 미실행']}
    (ROOT/'cpppj/recovery-creation-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'cases':dict(counts),'total_cases':report['total_cases'],'catalog_rows':359}),flush=True)


def verify():
    """저장된 원본/도구/fixture/주소 표 해시와 실제 호출 행 수를 확인한다."""
    report=json.loads((ROOT/'cpppj/recovery-creation-evidence.json').read_text(encoding='utf-8'))
    for group in ('artifact_sha256','source_sha256'):
        for path,sha in report[group].items():
            assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==sha,path
    for edition,item in report['editions'].items():
        binary=ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE')
        assert hashlib.sha256(binary.read_bytes()).hexdigest()==item['binary_sha256']
        assert item['assert_reports']==0
    counts=collections.Counter(line.split('\t')[1] for line in
        (ROOT/'cpppj/tests/fixtures/creation-x86.tsv').read_text(encoding='utf-8').splitlines() if line.startswith('Step\t'))
    assert dict(counts)==report['cases'] and sum(counts.values())==report['total_cases']
    catalog=[line for line in (ROOT/'cpppj/tests/fixtures/constructors-x86.tsv').read_text(encoding='utf-8').splitlines()
             if line and not line.startswith('#')]
    assert len(catalog)==report['catalog_rows']
    print(json.dumps({'verified':True,'cases':dict(counts),'catalog_rows':len(catalog)}),flush=True)


# 명시적인 직접 실행에서만 독립 기대값을 생성하거나 해시를 확인한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    verify() if args.verify else generate()
