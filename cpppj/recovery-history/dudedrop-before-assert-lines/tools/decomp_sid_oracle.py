#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""SID 함수의 실제 x86을 격리 실행한다. 게임/GUI/OS API는 실행하지 않는다.

초기화는 기존 메모리 경로만, 일반 할당은 소진 전 경로만 실행한다.
삭제 기록의 CRT 겹친 복사도 실제 원본 기계어로 실행한다.
Ghidra ExportSid.java가 확인한 몸체 범위 밖 실행과 풀/카운터/기록 밖 쓰기는 거부한다.
"""
import argparse
import collections
import csv
import hashlib
import json
import random
import struct
import zlib
from decomp_oracle import ROOT, SCRATCH, STOP, STACK
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)
import pefile

# SID raw 풀은 호스트 포인터와 분리된 가상 주소다. 최대 65535×50바이트를 담는다.
POOL=SCRATCH+0x20000
# 판본별 함수 시작 주소와 SID pool/머리·꼬리/freeCount/개수·예측 커서/server 전역이다.
SPECS={
    'originals':{'stride':50,'server_first':15000,'predictable':23001,
        'entries':{'Reset':0x4abb10,'Allocate':0x4af1d0,'Release':0x4abf60,'Rebuild':0x4abe00},
        'globals':(0x5c8464,0x5c8468,0x5c846c,0x5c8470,0x5c8474,0x5c8478,0x5c847c,0x5424b0,0x540bc0),
        'logs':(0x59a698,0x59a738),'copy':0x4e5b40,'assert':0x4e0620},
    'originalCD':{'stride':36,'server_first':6000,'predictable':14000,
        'entries':{'Reset':0x4aaad0,'Allocate':0x4aae40,'Release':0x4ab250,'Rebuild':0x4aad30},
        'globals':(0x5395dc,0x5395e0,0x5395e4,0x5395e8,0x5395ec,0x5395f0,0x5395f4,0x5395c0,0x540a28),
        'logs':(0x5890b0,0x589010),'copy':0x4f1a60,'assert':0x490040},
}
# 실행/저장 결과의 입력 시드와 최대 명령 수다. 최대 풀 초기화는 1000만 명령까지 허용한다.
SEED=0x5101078
MAX_INSTRUCTIONS=10000000


class SidOracle:
    """SID의 새 Ghidra 범위에서 원본 명령과 정상 스택 복구만 허용한다."""
    def __init__(self,edition):
        """두 PE를 읽어 별도 에뮬레이터에 복사하고 SID 몸체·쓰기 범위를 구성한다."""
        self.edition=edition
        self.spec=SPECS[edition]
        binary=ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE')
        self.sha256=hashlib.sha256(binary.read_bytes()).hexdigest()
        pe=pefile.PE(str(binary))
        mapped=pe.get_memory_mapped_image()
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32)
        self.mu.mem_map(pe.OPTIONAL_HEADER.ImageBase,(len(mapped)+4095)&~4095)
        self.mu.mem_write(pe.OPTIONAL_HEADER.ImageBase,mapped)
        self.mu.mem_map(SCRATCH,0x420000)
        self.mu.mem_map(STOP,0x1000)
        self.mu.mem_map(0x30000000,0x10000)
        self.entries=self.spec['entries']
        self.facts=[]
        self.allowed=[]
        with (ROOT/f'extracted/sid/{edition}/functions.tsv').open(encoding='utf-8') as fp:
            # 새 헤드리스 디컴파일의 실제 불연속 몸체 범위만 사용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.facts.append(row)
                for part in row['ranges'].split(';'):
                    lo,hi=(int(value,16) for value in part.split('-'))
                    self.allowed.append((lo,hi+1))
        self.assertions=0
        self.copies=0
        self.instructions=0
        self.max_instructions=0
        self.write_ranges=[]
        self.mu.hook_add(UC_HOOK_CODE,self.on_instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.on_write)

    def on_write(self,_mu,_access,address,size,_value,_data):
        """원본 코드·타입·다른 전역·풀 밖 쓰기를 거부한다."""
        if not any(lo<=address and address+size<=hi for lo,hi in self.write_ranges):
            raise RuntimeError(f'허용하지 않은 SID 쓰기: {self.edition} {address:08x}+{size}')

    def on_instruction(self,mu,address,size,_data):
        """CRT 기록 이동을 실제 실행하며 assert·소진 복구·OS 경로에 도달하면 즉시 실패한다."""
        self.instructions+=1
        if address==self.spec['assert']:
            self.assertions+=1
            raise RuntimeError(f'SID assert 도달: {self.edition}')
        if address==self.spec['copy']:
            esp=mu.reg_read(UC_X86_REG_ESP)
            _ret,dest,source,length=struct.unpack('<4I',mu.mem_read(esp,16))
            if length!=152 or source not in self.spec['logs'] or dest!=source+8:
                raise RuntimeError('삭제 기록 복사 계약 오류')
            self.copies+=1
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):
            raise RuntimeError(f'허용하지 않은 SID 코드: {self.edition} {address:08x}')

    def begin(self,capacity,server):
        """malloc 없이 이미 확보된 전체 풀과 비어 있는 삭제 기록을 입력한다."""
        self.capacity=capacity
        self.server=server
        self.stride=self.spec['stride']
        self.mu.mem_write(POOL,bytes([0xa5])*(capacity*self.stride))
        glob=self.spec['globals']
        self.write_ranges=[(POOL,POOL+capacity*self.stride),(0x30000000,0x30010000)]
        self.write_ranges.extend((address,address+4) for address in glob[:8])
        self.write_ranges.extend((address,address+160) for address in self.spec['logs'])
        # 실제 할당 금지/전투 소진 복구 플래그는 PE의 0 기본값을 유지한다.
        for address,value in [(glob[0],POOL),(glob[6],capacity),(glob[8],int(server))]:
            self.mu.mem_write(address,struct.pack('<I',value))
        for address in self.spec['logs']:
            self.mu.mem_write(address,bytes(160))

    def step(self,name,arg=0):
        """정상 반환·cdecl 스택 복구를 확인하고 전체 풀/삭제 기록/카운터를 읽는다."""
        # 공통 call의 제한보다 긴 초기화 반복도 EIP/ESP를 동일하게 확인한다.
        # 이전 호출의 레지스터와 DF가 다음 결과에 영향을 주지 않도록 초기화한다.
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
                         UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):
            self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.mem_write(STACK,struct.pack('<2I',STOP,arg))
        self.instructions=0
        self.mu.emu_start(self.entries[name],STOP,timeout=30000000,count=MAX_INSTRUCTIONS)
        self.max_instructions=max(self.max_instructions,self.instructions)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:
            raise RuntimeError(f'SID 반환/스택 오류: {name} EIP={self.mu.reg_read(UC_X86_REG_EIP):08x}, 명령={self.instructions}')
        result=self.mu.reg_read(UC_X86_REG_EAX) if name=='Allocate' else 0
        glob=self.spec['globals']
        free,cursor=[struct.unpack('<I',self.mu.mem_read(glob[index],4))[0] for index in (5,7)]
        client_head=struct.unpack('<H',self.mu.mem_read(POOL+4,2))[0]
        server_head=struct.unpack('<H',self.mu.mem_read(POOL+18,2))[0]
        client_tail,server_tail=[(struct.unpack('<I',self.mu.mem_read(glob[index],4))[0]-POOL)//self.stride for index in (2,4)]
        pool=zlib.adler32(self.mu.mem_read(POOL,self.capacity*self.stride))
        logs=zlib.adler32(b''.join(bytes(self.mu.mem_read(address,160)) for address in self.spec['logs']))
        return [result,free,cursor,client_head,client_tail,server_head,server_tail,pool,logs]

    def fill(self,sid,seed,state,type_id):
        """파생 생성자가 아직 없는 슬롯에 명시적인 검사 입력 바이트를 놓는다."""
        payload=bytearray((seed+i*37)&255 for i in range(self.stride))
        payload[10],payload[11]=type_id,state
        self.mu.mem_write(POOL+sid*self.stride,bytes(payload))


def generate():
    """판본별 크기 경계·서버/클라이언트·FIFO·타입 보존·카운터 누적을 검사한다."""
    rows=['# SID x86: 기존 풀 초기화/소진 전 할당/void 반납/서버 목록 재구성. CRT 기록 복사도 실제 기계어.']
    counts=collections.Counter()
    reports={}
    # 각 판본은 실제 경계가 다르므로 별도 초기 상태와 결과를 저장한다.
    for edition,spec in SPECS.items():
        oracle=SidOracle(edition)
        rng=random.Random(SEED)
        for capacity in (spec['predictable']+2,spec['predictable']+256,32768,65535):
            for server in (False,True):
                rows.append(f'Begin\t{edition}\t{capacity}\t{int(server)}')
                counts['scenarios']+=1
                oracle.begin(capacity,server)
                live=[]
                # 모든 기계어 호출 직후의 전체 raw 풀과 기록을 독립 결과로 저장한다.
                def step(name,arg=0):
                    result=oracle.step(name,arg)
                    rows.append('\t'.join(map(str,['Step',name,arg,*result])))
                    counts[name]+=1
                    return result[0]
                step('Reset')
                # 20개 삭제 기록 용량과 일반 free FIFO 순서를 넘어선 재사용 입력을 만든다.
                for i in range(96):
                    flags=2 if not server or i%3==0 else 0
                    if i%7==0 and (not server or capacity>spec['predictable']+2):
                        # 작은 예측 영역의 마지막 서버 예약은 검사 입력에서 소진시키지 않는다.
                        if capacity>spec['predictable']+2 or i==0:
                            flags=1 if i%2 else 3
                    sid=step('Allocate',flags)
                    seed=rng.randrange(256)
                    state=4 if i%2 else 6
                    type_id=rng.randrange(1,256)
                    rows.append(f'Fill\t{sid}\t{seed}\t{state}\t{type_id}')
                    oracle.fill(sid,seed,state,type_id)
                    live.append(sid)
                    if len(live)>7:
                        index=rng.randrange(len(live))
                        step('Release',live.pop(index))
                # 무작위 반납 뒤 재구성하면 자유 슬롯은 번호순이고 freeCount는 누적된다.
                for sid in reversed(live):
                    step('Release',sid)
                step('Rebuild')
                step('Rebuild')
                sid=step('Allocate',2 if not server else 0)
                step('Release',sid)
                step('Reset')
                print(f'{edition}: capacity={capacity}, server={int(server)} 완료',flush=True)
        reports[edition]={'binary_sha256':oracle.sha256,'function_ranges':oracle.facts,
            'assert_reports':oracle.assertions,'crt_copies':oracle.copies,'max_instructions_observed':oracle.max_instructions}
        print(f'{edition}: SID 시퀀스 완료',flush=True)
    fixture=ROOT/'cpppj/tests/fixtures/sid-x86.tsv'
    fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    source_paths=['tools/decomp_sid_oracle.py','tools/decomp_oracle.py','tools/ghidra/ExportSid.java']
    report={'schema':1,'classification':'scoped-sid-pool','cases':dict(counts),
        'total_cases':sum(value for key,value in counts.items() if key!='scenarios'),
        'editions':reports,'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),
        'source_sha256':{path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in source_paths},
        'limits':['Init은 기존 풀 경로만; CRT malloc 분기 미실행',
            '일반/예측 할당은 소진 전 유효 입력만; 소진 UI/다리 파괴 복구 미실행',
            '삭제 기록 함수와 겹친 152바이트 CRT 복사 모두 실제 기계어',
            'raw 풀 전체/삭제 기록 전체는 Adler-32와 주요 카운터/머리/꼬리로 대조',
            '파생 생성자/공간 등록/삭제 효과·네트워크 Take·GameWorld 연결 미실행',
            '게임 진입점/OS API/원본·복사본·클론 GUI 실행 없음']}
    (ROOT/'cpppj/recovery-sid-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'cases':dict(counts),'total_cases':report['total_cases']}),flush=True)


def verify():
    """저장된 기록·원본·도구·fixture 해시와 호출 행 수를 검사한다."""
    report=json.loads((ROOT/'cpppj/recovery-sid-evidence.json').read_text(encoding='utf-8'))
    fixture=ROOT/'cpppj/tests/fixtures/sid-x86.tsv'
    assert hashlib.sha256(fixture.read_bytes()).hexdigest()==report['fixture_sha256']
    # 바이너리와 도구는 기록 이후 변하지 않았는지 확인한다.
    for edition,item in report['editions'].items():
        path=ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE')
        assert hashlib.sha256(path.read_bytes()).hexdigest()==item['binary_sha256']
        assert item['assert_reports']==0
    for path,sha in report['source_sha256'].items():
        assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==sha,path
    counts=collections.Counter()
    # 행 형식과 실제 호출 개수도 검사한다. 입력 Fill은 기계어 사례 수에 넣지 않는다.
    for line in fixture.read_text(encoding='utf-8').splitlines():
        if not line or line.startswith('#'):
            continue
        row=line.split('\t')
        if row[0]=='Begin':
            assert len(row)==4 and row[1] in SPECS
            counts['scenarios']+=1
        elif row[0]=='Fill':
            assert len(row)==5
        else:
            assert row[0]=='Step' and len(row)==12 and row[1] in ('Reset','Allocate','Release','Rebuild')
            counts[row[1]]+=1
    assert dict(counts)==report['cases']
    assert sum(value for key,value in counts.items() if key!='scenarios')==report['total_cases']
    print(json.dumps({'verified':True,'cases':dict(counts)}),flush=True)


# 명시적인 직접 실행에서만 기대값을 생성하거나 검사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    verify() if args.verify else generate()
