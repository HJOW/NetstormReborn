#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""공통 pre/postDestroy 장부를 세 PE의 제한 x86으로 실행한다. 게임/창/OS 실행은 없다."""
import argparse
import collections
import csv
import json
import struct
import zlib
from pathlib import Path

from decomp_destroy_oracle import DestroyOracle,VTABLE,CONTROLS,sha
from decomp_graphremove_oracle import BINARIES,DEPENDENCIES
from decomp_postpop_oracle import PostOracle,ARENA,PROVIDERS,FACTORIES,OWNERS
from decomp_creation_oracle import TYPES
from decomp_sid_oracle import POOL,SidOracle
from decomp_pop_oracle import bits
from decomp_oracle import ROOT
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_ECX,UC_X86_REG_EIP

# 선택 복구/손실 좌표와 효과 억제·삭제 보상의 명시적 외부 경계다.
LIFECYCLE={
 'originals':dict(placement=(0x5caf38,0x5caf3c),lost=(0x5c848c,0x5c8490),
    suppress=(0x5c89b8,0x594fb8),silent=0x5412a4,refund=0x44c2f0,sound=0x4a9cb0,
    owner_remove=0x4900a0,provider_remove=0x472ff0,factory_remove=0x44f7a0,ai=0x4166e0),
 'originalCD':dict(placement=(0x549b38,0x549b3c),lost=(0x56fa08,0x56fa0c),
    suppress=(0x5178d4,0x540a1c),silent=None,refund=0x4626d0,sound=0x438b50,
    owner_remove=0x407250,provider_remove=0x48a3c0,factory_remove=0x45ecc0,ai=0x4d8230)}
# 상위 입력 수는 내부 pre/post/list/Unpop/Release 도달과 중복 집계하지 않는다.
EXPECTED_CASES={'Pair':1152,'Destroy':576}
EXPECTED_NATIVE={'Destroy':192,'Unpop':104,'Release':192}
EXPECTED_LIFECYCLE={'Pre':576,'Post':576,'provider_remove':60,'ai':140,'owner_remove':232,'factory_remove':58}
FIXTURE=ROOT/'cpppj/tests/fixtures/destroylifecycle-x86.tsv'
REPORT=ROOT/'cpppj/recovery-destroylifecycle-evidence.json'


def list_input(pattern,base,root):
    """기대값을 계산하지 않고 중복·미사용 꼬리를 포함한 6 DWORD 입력을 만든다."""
    values=[base+i for i in range(6)]
    count=(0,1,5,6,4,3)[pattern]
    if pattern==1:values[0]=root
    if pattern==3:values[0]=values[2]=values[5]=root
    if pattern==4:values[:4]=[root]*4
    if pattern==5:values[1]=root
    return count,values


def case_input(case,kind):
    """독립적으로 재생 가능한 고정 합성 입력을 정의한다. 삭제 결과는 넣지 않는다."""
    number=(74,148,169)[case%3]
    f1=(0,0x10000000,0x800,0x10000800)[case%4]
    f2=(0,0x200,0x4000,0x4200,0x10000,0x20000,0x220000)[(case//4)%7]
    cost=(0.,0.25,3.75,-0.5,600.,-12.25)[case%6]
    owner=(case//2)%9;local=owner if case%2==0 else (owner+1)%9
    extra=(0,1,8,9)[(case//7)%4]
    flags=(0,0x800,0x200000,0x2000000,0x90000,0xf0000,0x200010,0xffffffff)[(case//3)%8]
    total=(-1,123,2147483640,-2147483640)[case%4]
    # 실제 건물 Unpop은 후속이므로 통합 삭제의 해당 입력만 이미 void로 제한한다.
    root_void=int(kind=='Pair' or bool(f2&0x4200))
    return [number,f1,f2,bits(cost),10 if case%7==0 else 0,owner,local,extra,flags,
            int(case%3!=0),int(case%11==0),int(case%13==0),int(case%17==0),148,
            case%6,total,0xffffffff if case%2 else 0,root_void,case*31337]


def validate_calls(binary,record):
    """고정 입력의 실제 진입 수와 대체 경계를 생성/감사 양쪽에서 확인한다."""
    if record['asserts'] or record['native_calls']!=EXPECTED_NATIVE or record['lifecycle_calls']!=EXPECTED_LIFECYCLE:
        raise RuntimeError('실제 공통 훅/반납 경계 불일치: '+binary)
    expected=dict(selected=768,clear=384,refund=98,unit_lost=20 if binary=='originals' else 24,log=192)
    if record['substitutions']!=expected:raise RuntimeError('외부 효과 대체 경계 불일치: '+binary)


class LifecycleOracle(DestroyOracle):
    """기존 실제 destroy/Unpop/반납에 실제 공통 훅과 목록/타입 helper를 연결한다."""
    def __init__(self,binary):
        """불연속 내보내기 범위만 허용하며 기존 도구/기록은 변경하지 않는다."""
        super().__init__(binary);self.life=LIFECYCLE[self.edition];self.lifecycle_native=collections.Counter()
        directory=ROOT/f'extracted/destroylifecycle/{binary}'
        self.body_paths.extend(directory/name for name in ('creation.c','functions.tsv'))
        with (directory/'functions.tsv').open(encoding='utf-8') as fp:
            # 함수 사이 코드는 여전히 실행 금지다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.facts.append(row)
                # Ghidra의 포함 끝 주소를 열린 끝으로 바꾼다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.table=bytearray(self.mu.mem_read(VTABLE,0xcc))
        struct.pack_into('<I',self.table,0x14,self.d['pre']);struct.pack_into('<I',self.table,0x18,self.d['post'])
        # 대부분의 초기화/CRT 명령은 주소 집합 검사 후 바로 원본 범위 감사로 보낸다.
        self.interesting={self.d[key] for key in ('destroy','pre','post','selected','clear','log','transmit')}
        self.interesting.update(self.life[key] for key in ('refund','sound','owner_remove','provider_remove','factory_remove','ai'))
        self.interesting.update((self.creation['unpop'],self.spec['entries']['Release']))

    def on_instruction(self,mu,address,size,data):
        """공통 pre/post는 실제 명령이며 보상/SP와 실제 소리만 사건으로 대체한다."""
        if not hasattr(self,'life'):return super().on_instruction(mu,address,size,data)
        if address not in self.interesting:return SidOracle.on_instruction(self,mu,address,size,data)
        esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX)
        entries={self.d['pre']:'Pre',self.d['post']:'Post'}
        entries.update({self.life[key]:key for key in ('owner_remove','provider_remove','factory_remove','ai')})
        if address in entries:self.lifecycle_native[entries[address]]+=1
        if address in (self.d['pre'],self.d['post']):
            flag=struct.unpack('<I',mu.mem_read(esp+4,4))[0];sid=(ecx-POOL)//self.stride
            self.events.append(f'{"P" if address==self.d["pre"] else "O"}:{sid}:{flag}:{mu.mem_read(ecx+11,1)[0]}')
            return SidOracle.on_instruction(self,mu,address,size,data)
        if address in (self.life['refund'],self.life['sound']):
            self.instructions+=1;ret=struct.unpack('<I',mu.mem_read(esp,4))[0]
            if address==self.life['refund']:
                sid,recipient,last=struct.unpack('<3I',mu.mem_read(esp+4,12))
                if sid!=self.root or last!=0:raise RuntimeError('삭제 보상 인자 오류')
                self.events.append(f'Q:{sid}:{recipient}');self.substitutions['refund']+=1
            else:
                path,a1,a2,a3,a4,a5=struct.unpack('<6I',mu.mem_read(esp+4,24))
                if bytes(mu.mem_read(path,13)).split(b'\0')[0]!=b'unitLost.wav' or (a1,a2,a3,a4,a5)!=(0,0,0,1,0):
                    raise RuntimeError('unitLost 소리 인자 오류')
                self.events.append(f'L:{self.root}');self.substitutions['unit_lost']+=1
            mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret);return
        return super().on_instruction(mu,address,size,data)

    def start_lifecycle(self,control):
        """실제 Reset/Create를 준비하고 새 좌표 전역 쓰기만 더한다."""
        self.start_destroy(control)
        self.write_ranges.extend((a,a+4) for a in (*self.life['placement'],*self.life['lost']))
        self.write_ranges.append((self.post['pending'],self.post['pending']+4))

    def prepare_lifecycle(self,args,kind):
        """동일한 초기 풀/해시와 타입·장부 입력을 원본 주소에 넣는다."""
        number,f1,f2,cost_bits,group,owner,local,extra,flags,selected,suppressed,s1,s2,silent,pattern,total,revision,root_void,seed=args
        self.prepare_destroy(True,0,flags,0,'Destroy');self.sid=self.root
        PostOracle.state(self,6,6,6,0,total,revision,seed,local,0,suppressed,3)
        self.mu.mem_write(VTABLE,bytes(self.table));self.selected_sid=self.root if selected else 0
        cost=struct.unpack('<f',struct.pack('<I',cost_bits))[0];self.input_type(number,f1,f2,cost)
        self.mu.mem_write(TYPES+number*self.creation['type_stride']+0x9c,struct.pack('<I',group))
        address=POOL+self.root*self.stride
        self.mu.mem_write(address+10,bytes([number,4 if root_void else 0]))
        self.mu.mem_write(address+(34 if self.edition=='originals' else 32),bytes([owner]))
        self.mu.mem_write(address+(40 if self.edition=='originals' else 35),bytes([extra]))
        if root_void:self.mu.mem_write(self.bases[0]+(21*256+20)*2,b'\0\0')
        # 공급/작업장과 모든 소유자 목록의 inactive DWORD도 별도로 입력한다.
        for pointer,count_address,base in [(PROVIDERS,self.post['providers'][2],0x60000000),
                (FACTORIES,self.post['factories'][2],0x61000000),
                *((OWNERS+i*0x100,ARENA+i*self.post['stride']+8,0x70000000+i*100) for i in range(9))]:
            count,values=list_input(pattern,base,self.root)
            self.mu.mem_write(pointer,struct.pack('<6I',*values));self.mu.mem_write(count_address,struct.pack('<I',count))
        # 기존 좌표를 구별되는 float 비트로 채워 생략된 쓰기도 확인한다.
        for a,value in zip((*self.life['placement'],*self.life['lost']),(bits(-7.25),bits(8.5),bits(90.25),bits(-91.5))):
            self.mu.mem_write(a,struct.pack('<I',value))
        for a,value in zip(self.life['suppress'],(s1,s2)):self.mu.mem_write(a,struct.pack('<I',value))
        if self.life['silent']:self.mu.mem_write(self.life['silent'],struct.pack('<I',silent))

    def bookkeeping_output(self):
        """세 통계 표와 모든 목록의 물리 꼬리까지 실제 실행 결과를 읽는다."""
        p=self.post;data=bytearray();counts=[]
        for address,key in ((PROVIDERS,'providers'),(FACTORIES,'factories')):
            data.extend(self.mu.mem_read(address,24));counts.append(struct.unpack('<I',self.mu.mem_read(p[key][2],4))[0])
        # 소유자 0 목록도 검사하지만 원본 제거 helper는 중립 소유자를 건너뛴다.
        for owner in range(9):
            data.extend(self.mu.mem_read(OWNERS+owner*0x100,24))
            counts.append(struct.unpack('<I',self.mu.mem_read(ARENA+owner*p['stride']+8,4))[0])
        values=[struct.unpack('<i',self.mu.mem_read(p['cost'],4))[0],struct.unpack('<I',self.mu.mem_read(p['dirty'],4))[0],
                struct.unpack('<I',self.mu.mem_read(self.space['depth'],4))[0]]
        values.extend(zlib.adler32(self.mu.mem_read(a,1024)) for a in p['counts']);values.extend(counts);values.append(zlib.adler32(data))
        return ','.join(map(str,values))

    def context_output(self):
        """선택 복구 타입/좌표와 손실 좌표를 비트 그대로 관찰한다."""
        return ','.join(str(struct.unpack('<I',self.mu.mem_read(a,4))[0]) for a in (self.post['pending'],*self.life['placement'],*self.life['lost']))


def generate():
    """각 실제 PE/정밀도에서 직접 훅 쌍과 실제 전체 삭제를 실행한다."""
    rows=[];reports={};counts=collections.Counter();paths=set()
    for binary in BINARIES:
        oracle=LifecycleOracle(binary)
        # 두 x87 제어 워드에서 절삭/정상 반환을 검사한다.
        for control in CONTROLS:
            oracle.start_lifecycle(control)
            # Pair는 공통 훅만, Destroy는 실제 Unpop/반납까지 포함한다.
            for kind,length in (('Pair',192),('Destroy',96)):
                for case in range(length):
                    args=case_input(case,kind);oracle.prepare_lifecycle(args,kind);address=POOL+oracle.root*oracle.stride
                    if kind=='Pair':
                        oracle.invoke(oracle.d['pre'],[args[8]],address,4);oracle.invoke(oracle.d['post'],[args[8]],address,4)
                    else:oracle.invoke(oracle.d['destroy'],[args[8]],address,4)
                    rows.append([kind,binary,control,','.join(map(str,args)),*oracle.output(),oracle.bookkeeping_output(),oracle.context_output()]);counts[kind]+=1
        reports[binary]=dict(binary_sha256=oracle.sha256,native_calls=dict(oracle.native),lifecycle_calls=dict(oracle.lifecycle_native),
            substitutions=dict(oracle.substitutions),asserts=oracle.assertions)
        validate_calls(binary,reports[binary])
        paths.update(oracle.body_paths)
        # 실제 의존 몸체와 기반 생성/SID 내보내기도 정확한 바이트로 고정한다.
        for folder in ('sid','creation'):
            paths.update((ROOT/f'extracted/{folder}/{oracle.edition}').glob('*.c'));paths.update((ROOT/f'extracted/{folder}/{oracle.edition}').glob('*.tsv'))
        paths.update((ROOT/f'extracted/graphremove/{binary}').rglob('*.tsv'));paths.update((ROOT/f'extracted/graphremove/{binary}').rglob('*.c'))
        print(binary,'장부 원본 명령 대조 완료',dict(counts),flush=True)
    if dict(counts)!=EXPECTED_CASES:raise RuntimeError('고정 입력 수 불일치')
    FIXTURE.write_text('# 실제 공통 pre/postDestroy 장부. Graph 비활성/AI null/보상·소리·UI·전파 대체.\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths.update([FIXTURE,Path(__file__),ROOT/'tools/decomp_destroy_oracle.py',ROOT/'tools/decomp_graphremove_oracle.py',
                  ROOT/'tools/ghidra/ExportCreation.java',ROOT/'tools/ghidra/destroylifecycle-functions.json'])
    paths.update(ROOT/'tools'/name for name in DEPENDENCIES)
    report=dict(cases=dict(counts),total=sum(counts.values()),fpu_controls=list(CONTROLS),editions=reports,
        files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(paths)},
        limits=['실제 공통 pre/post/list/선택 복구/통계/비용·destroy/Unpop/Release와 CRT 기록 이동',
                '합성 타입/목록/등록·표시 억제·소진 전·Graph 비활성·AI null·비전투 null 큐',
                '보상/SP·unitLost 실제 소리·선택 UI·전파·로그는 명시적 사건 대체',
                'Pair는 직접 훅 호출의 DWORD 깊이 underflow, Destroy는 균형 깊이·실제 반납',
                '건물 통합 삭제 입력은 이미 void: 실제 건물 Unpop 완료 증명이 아님; 게임/OS/창 실행 없음'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('새 공통 삭제 장부 x86:',report['total'])


def verify():
    """저장 PE/도구/몸체/fixture SHA와 호출 경계·행 수만 확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in report['files'].items():
        if sha(ROOT/name)!=value:raise RuntimeError('SHA 불일치: '+name)
    for binary,record in report['editions'].items():
        if sha(ROOT/BINARIES[binary])!=record['binary_sha256']:
            raise RuntimeError('PE/assert 감사 실패: '+binary)
        validate_calls(binary,record)
    counts=collections.Counter(line.split('\t')[0] for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#'))
    if dict(counts)!=EXPECTED_CASES or dict(counts)!=report['cases'] or report['total']!=1728 or report['fpu_controls']!=list(CONTROLS):
        raise RuntimeError('행 수/정밀도 불일치')
    print('공통 삭제 장부 SHA/행 수/원본 경계 감사:',report['total'])


if __name__=='__main__':
    # 재실행 생성과 읽기 전용 감사를 명시적으로 구분한다.
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate()
