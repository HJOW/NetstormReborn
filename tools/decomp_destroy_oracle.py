#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""공통 destroy→실제 Unpop/반납과 다리 훅의 중첩 삭제를 세 PE의 제한 x86으로 대조한다.

가상 공통 pre/post·종속 release/destroy·선택·전파·로그·파편/소리/낙하는 명시적으로 대체한다.
공통 destroy·다리 pre/post·일반 finder·Unpop·SID 반납/삭제 기록은 원본 명령이다.
원본 게임/OS/창을 실행하지 않고 이전 도구/몸체/fixture를 변경하지 않는다.
"""
import argparse
import collections
import csv
import hashlib
import json
import struct
import zlib
from pathlib import Path

from decomp_graphremove_oracle import DeleteOracle,BINARIES,DEPENDENCIES
from decomp_creation_oracle import TYPES
from decomp_sid_oracle import POOL,SidOracle
from decomp_pop_oracle import HEADS,SPOTS,bits
from decomp_postpop_oracle import ARENA
from decomp_oracle import ROOT,STOP
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 실제 공통 삭제·전파·선택·로그·공통 훅 카운터와 다리 효과 진입점이다.
SPECS={
 'originals':dict(destroy=0x4af780,pre=0x4b0950,post=0x4b0840,pre_depth=0x5c8480,post_depth=0x5c8484,
    transmit=0x4ac260,selected=0x4d5430,clear=0x4d5c60,log=0x4c2630,
    bridge_pre=0x4221b0,bridge_post=0x422300,bridge_vtable=0x5034c8,link_type=0x5412c4,
    notify=0x460600,sound=0x4a9d70),
 'originalCD':dict(destroy=0x4ab7e0,pre=0x4add20,post=0x4adbe0,pre_depth=0x5395f8,post_depth=0x5395fc,
    transmit=0x4abb40,selected=0x41ea30,clear=0x41ed70,log=0x48ca70,
    bridge_pre=0x4498e0,bridge_post=0x449c60,bridge_vtable=0x501ab0,link_type=0x51cbb0,
    notify=0x454810,sound=0x438c00)}
# 합성 가상 표와 외부 가상 호출 주소. 실제 Unpop/표시·destroy는 원본 함수로 연결한다.
VTABLE=ARENA+0x9000
PRE,POST,RELEASE_CHILD,DESTROY_CHILD,FALL=(STOP+0x100+i*0x10 for i in range(5))
# 저장 결과와 별도 감사 기록의 위치다. 기존 독립 기록은 덮어쓰지 않는다.
FIXTURE=ROOT/'cpppj/tests/fixtures/destroy-x86.tsv'
REPORT=ROOT/'cpppj/recovery-destroy-evidence.json'
# x87 53/64비트 정밀도와 고정 상위 입력/내부 원본 호출 수다.
CONTROLS=(0x027f,0x037f)
EXPECTED_CASES={'Destroy':1800,'Bridge':192}
EXPECTED_NATIVE={'Destroy':712,'Unpop':440,'Release':600,'BridgePre':48,'Begin':96,'Next':704,'BridgePost':48}


def sha(path):
    """실제 입력 파일의 바이트 SHA를 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class DestroyOracle(DeleteOracle):
    """기존 실제 공간/반납 실행기에 공통 destroy와 명시적인 외부 효과만 더한다."""
    def __init__(self,binary):
        """새 몸체를 추가하고 원본 주소를 호스트에서 호출하지 않는 가상 표를 만든다."""
        super().__init__(binary);self.d=SPECS[self.edition]
        self.native=collections.Counter();self.substitutions=collections.Counter();self.events=[];self.mode='Destroy'
        self.body_paths=[]
        # 각 새 디컴파일 묶음의 실제 불연속 범위만 추가한다.
        for folder in ('destroy','bridgeeffects','finder'):
            directory=ROOT/f'extracted/{folder}/{binary}'
            self.body_paths.extend(directory/name for name in ('creation.c','functions.tsv'))
            with (directory/'functions.tsv').open(encoding='utf-8') as fp:
                # 각 함수의 실제 불연속 몸체를 허용한다. 주소 사이 전체를 실행 가능으로 넓히지 않는다.
                for row in csv.DictReader(fp,delimiter='\t'):
                    self.facts.append(row)
                    # 끝 주소가 포함된 Ghidra 범위를 Python의 열린 끝 범위로 바꾼다.
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        table=bytearray(self.mu.mem_read(self.creation['vtable'],0xcc))
        # 실행할 공통 destroy/Unpop/표시는 원본 진입점이고 미복원 가상 훅만 별도 진입점이다.
        for offset,address in [(0x10,self.d['destroy']),(0x14,PRE),(0x18,POST),(0x1c,RELEASE_CHILD),(0xc8,FALL)]:
            struct.pack_into('<I',table,offset,address)
        self.mu.mem_write(VTABLE,bytes(table))

    def on_instruction(self,mu,address,size,data):
        """외부 효과 경계를 기록하고 나머지 함수는 원본 명령만 실행한다."""
        if not hasattr(self,'d'):return super().on_instruction(mu,address,size,data)
        d=self.d;esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX)
        entries={d['destroy']:'Destroy',self.creation['unpop']:'Unpop',self.spec['entries']['Release']:'Release',
                 d['bridge_pre']:'BridgePre',d['bridge_post']:'BridgePost',
                 (0x4b16d0 if self.edition=='originals' else 0x4eae20):'Begin',
                 (0x4b1810 if self.edition=='originals' else 0x4eafe0):'Next'}
        if address in entries:self.native[entries[address]]+=1
        # 실제 다리 가상 훅 진입도 공통 훅과 같은 위치에서 관찰한다.
        if address in (d['bridge_pre'],d['bridge_post']):
            flag=struct.unpack('<I',mu.mem_read(esp+4,4))[0]
            sid=(ecx-POOL)//self.stride;state=mu.mem_read(ecx+11,1)[0]
            self.events.append(f'{"P" if address==d["bridge_pre"] else "O"}:{sid}:{flag}:{state}')
        stubs={PRE:'pre',POST:'post',RELEASE_CHILD:'release_child',DESTROY_CHILD:'destroy_child',FALL:'fall',
               d['pre']:'base_pre',d['post']:'base_post',d['transmit']:'transmit',d['selected']:'selected',
               d['clear']:'clear',d['log']:'log',d['notify']:'notify',d['sound']:'sound'}
        stub=stubs.get(address)
        if not stub:return super().on_instruction(mu,address,size,data)
        self.instructions+=1;self.substitutions[stub]+=1
        ret,a0,a1,a2,a3=struct.unpack('<5I',mu.mem_read(esp,20));purge=0
        sid=(ecx-POOL)//self.stride
        if stub in ('pre','post','base_pre','base_post'):
            pre=stub in ('pre','base_pre');base=stub.startswith('base_')
            tag=('A' if pre else 'B') if base else ('P' if pre else 'O')
            state=mu.mem_read(ecx+11,1)[0]
            self.events.append(f'{tag}:{sid}:{a0}:{state}')
            counter=d['pre_depth'] if pre else d['post_depth']
            value=struct.unpack('<I',mu.mem_read(counter,4))[0];mu.mem_write(counter,struct.pack('<I',(value-1)&0xffffffff))
            if pre and sid==self.root and self.mode=='Destroy':
                if self.case==6:mu.mem_write(ecx+6,struct.pack('<H',self.ids[2]))
                if self.case==7:mu.mem_write(ecx+11,bytes([state|4]))
            purge=4
        elif stub=='release_child':
            self.events.append(f'R:{sid}:{a0}')
            # release가 현재 next를 지워도 상위 커서에는 이미 다음 번호가 저장되어 있다.
            mu.mem_write(ecx+4,b'\x00\x00')
            if self.case==5 and sid==self.ids[1]:mu.mem_write(ecx+11,b'\x01')
            purge=4
        elif stub=='destroy_child':
            self.events.append(f'C:{sid}:{a0}');mu.mem_write(ecx+11,bytes([mu.mem_read(ecx+11,1)[0]|2]));purge=4
        elif stub=='transmit':
            if (a0,a1,a2)!=(0xffffffff,0xfffffffe,0):raise RuntimeError('삭제 전파 인자')
            self.events.append(f'T:{sid}:{a3}');purge=16
        elif stub=='selected':mu.reg_write(UC_X86_REG_EAX,self.selected_sid)
        elif stub=='clear':self.events.append(f'S:{self.selected_sid}');self.selected_sid=0
        elif stub=='notify':self.events.append(f'N:{a0}')
        elif stub=='sound':
            if bytes(mu.mem_read(a2,15)).split(b'\x00')[0]!=b'bridgeFall.wav':raise RuntimeError('다른 소리')
            self.events.append(f'W:{a0}:{a1}')
        elif stub=='fall':
            self.events.append(f'F:{sid}');mu.mem_write(ecx+11,bytes([mu.mem_read(ecx+11,1)[0]|2]))
        # 로그는 cdecl이고 선택 조회 외에는 반환값을 의미 있게 사용하지 않는다.
        mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def start_destroy(self,control):
        """실제 Reset/Create 이후의 풀을 저장하여 사례마다 원본 목록/카운터를 복구한다."""
        self.start_graph(control)
        self.write_ranges.extend((address,address+4) for address in (self.d['pre_depth'],self.d['post_depth']))
        self.initial_pool=bytes(self.mu.mem_read(POOL,self.capacity*self.stride))
        self.initial_globals={address:bytes(self.mu.mem_read(address,4)) for address in self.spec['globals'][:8]}
        self.mu.mem_write(self.d['link_type'],struct.pack('<I',75))

    def prepare_destroy(self,server,role,flags,case,mode):
        """입력 배치/참조/가상 훅만 구성한다. 삭제 결과를 Python에서 계산하지 않는다."""
        self.mode=mode;self.case=case;self.events=[]
        self.mu.mem_write(POOL,self.initial_pool)
        # 같은 실제 Reset/Create 상태에서 매 사례를 시작한다.
        for address,value in self.initial_globals.items():self.mu.mem_write(address,value)
        # 삭제 기록은 Reset 대상이 아니므로 입력 단계에서 명시적으로 비운다.
        for address in self.spec['logs']:self.mu.mem_write(address,bytes(160))
        self.mu.mem_write(self.spec['globals'][8],struct.pack('<I',int(server)));self.server=server
        self.mu.mem_write(HEADS,bytes(86272*2));self.mu.mem_write(SPOTS,bytes(65536))
        # 공통 훅 깊이의 시작값은 0이다. 중첩 삭제가 실제로 이를 공유한다.
        for address in (self.d['pre_depth'],self.d['post_depth']):self.mu.mem_write(address,b'\x00'*4)
        self.root=self.ids[0] if role==0 else (self.spec['server_first']+9 if role==1 else self.spec['predictable']+1)
        # 수신 서버/예측 번호는 실제 Take로 준비한다. 일반 할당 카운터를 임의로 차감하지 않는다.
        if role:self.creation_step('Take',74,self.root)
        active=list(dict.fromkeys([self.root,*self.ids]))
        self.selected_sid=self.root if case%2 else 0
        # 합성 타입/슬롯/버킷은 입력일 뿐 삭제 기대값을 계산하는 코드가 아니다.
        for index,sid in enumerate(active):
            typ=74 if sid==self.root else 74+self.ids.index(sid)
            state=(4 if case==1 else 2 if case==2 else 0) if sid==self.root else 0
            if mode=='Destroy' and 3<=case<=6 and sid in self.ids[1:3] and (case!=3 or sid==self.ids[1]):
                typ=20 if case==3 else 75;state=0 if case==3 else 8
            if mode=='Bridge':
                if sid in (self.ids[1],self.ids[4]):typ=75
                if sid==self.ids[2]:state=2
            raw=bytearray(self.stride);struct.pack_into('<I',raw,0,self.d['bridge_vtable'] if mode=='Bridge' and sid==self.root else VTABLE)
            raw[10]=typ;raw[11]=state
            struct.pack_into('<ff',raw,14,20.75,21.9)
            self.mu.mem_write(POOL+sid*self.stride,bytes(raw))
            self.type_space(typ,0,4 if sid==self.root and mode=='Bridge' else 0,1,1,1,1)
            if mode=='Bridge' and sid==self.ids[5]:self.mu.mem_write(TYPES+typ*self.creation['type_stride']+0xec,struct.pack('<I',0x10000))
            if not state&12 and typ>=70:
                level=0 if sid==self.root else 1;head=self.bases[level]+((21//(1 if level==0 else 2))*(256//(1 if level==0 else 2))+20//(1 if level==0 else 2))*2
                self.mu.mem_write(POOL+sid*self.stride+4,bytes(self.mu.mem_read(head,2)));self.mu.mem_write(head,struct.pack('<H',sid))
        if mode=='Destroy' and case>=3 and case<=6:
            children=[self.ids[1]] if case==3 else self.ids[1:3]
            self.mu.mem_write(POOL+self.root*self.stride+6,struct.pack('<H',children[0]))
            # 원본 조건은 contained 또는 form이다. 각 가상 효과의 실제 내부 구현은 대상 밖이다.
            for i,sid in enumerate(children):
                self.mu.mem_write(POOL+sid*self.stride+10,bytes([20 if case==3 else 75,0 if case==3 else 8]))
                self.mu.mem_write(POOL+sid*self.stride+4,struct.pack('<H',children[i+1] if i+1<len(children) else 0))
                self.mu.mem_write(VTABLE+0x10,struct.pack('<I',DESTROY_CHILD))
        else:self.mu.mem_write(VTABLE+0x10,struct.pack('<I',self.d['destroy']))
        if mode=='Bridge':
            # 별도 서버/예측 root와 같은 타입인 미사용 client 슬롯이 설정을 덮지 않게 한다.
            self.mu.mem_write(TYPES+74*self.creation['type_stride']+0xec,struct.pack('<I',4))
            # 첫 링크를 해제한 뒤 두 번째 링크가 읽는 참조도 같은 풀에 놓는다.
            for sid,first,second in [(self.ids[1],self.ids[2],self.ids[3]),(self.ids[4],self.ids[1],self.ids[3])]:
                self.mu.mem_write(POOL+sid*self.stride+12,struct.pack('<H',first));self.mu.mem_write(POOL+sid*self.stride+8,struct.pack('<H',second))
        self.active=active

    def output(self):
        """전체 슬롯·풀/해시/spot·삭제 기록과 목록/카운터를 읽는다. 합성 vtable 값만 기록값으로 정규화한다."""
        pool=bytearray(self.mu.mem_read(POOL,self.capacity*self.stride))
        # 외부 훅용 합성 주소만 기본 vtable 기록값으로 되돌린다. 다른 출력 바이트는 그대로다.
        for sid in self.active:
            offset=sid*self.stride
            if struct.unpack_from('<I',pool,offset)[0]==VTABLE:struct.pack_into('<I',pool,offset,self.creation['vtable'])
        slots=';'.join(f'{sid}:'+pool[sid*self.stride:(sid+1)*self.stride].hex() for sid in self.active)
        glob=self.spec['globals'];values=[struct.unpack('<I',self.mu.mem_read(glob[i],4))[0] for i in (5,7)]
        values.extend(struct.unpack('<H',self.mu.mem_read(POOL+i,2))[0] for i in (4,18))
        values.extend((struct.unpack('<I',self.mu.mem_read(glob[i],4))[0]-POOL)//self.stride for i in (2,4))
        values.extend([zlib.adler32(pool),zlib.adler32(self.mu.mem_read(HEADS,86272*2)),zlib.adler32(self.mu.mem_read(SPOTS,65536)),
            zlib.adler32(b''.join(bytes(self.mu.mem_read(a,160)) for a in self.spec['logs'])),self.selected_sid])
        values.extend(struct.unpack('<I',self.mu.mem_read(a,4))[0] for a in (self.d['pre_depth'],self.d['post_depth']))
        return ';'.join(self.events) or '-',slots,','.join(map(str,values))


def generate():
    """역할/flag/상태/종속 변경과 실제 다리 훅의 중첩 삭제를 고정 입력으로 실행한다."""
    rows=[];reports={};counts=collections.Counter();paths=set()
    # CD와 같은 배치의 추가 10.37도 별도 실제 PE 바이트로 실행한다.
    for binary in BINARIES:
        oracle=DestroyOracle(binary)
        # 정상 반환 때 원래 x87 설정/TOP와 ESP를 검사하는 부모 실행기를 사용한다.
        for control in CONTROLS:
            oracle.start_destroy(control)
            # client/server/predictable 번호·서버 권한·다양한 원본 flags를 교차한다.
            for server in (False,True):
                # 세 SID 영역은 원본 서버/클라이언트 분기를 바꾼다.
                for role in (0,1,2):
                    # 종속·전파 억제·그래프 관련 상위 비트도 훅에 그대로 전달되는지 관찰한다.
                    for flags in (0,8,16,0x40,0x50,0x800,0x2000000,0xffffffff):
                        # void/dead·form/contained·release 때 free·pre의 head/void 변경을 교차한다.
                        for case in range(8):
                            if not server and role and not flags&8 and case!=2:continue
                            oracle.prepare_destroy(server,role,flags,case,'Destroy')
                            oracle.invoke(oracle.d['destroy'],[flags],POOL+oracle.root*oracle.stride,4)
                            rows.append(['Destroy',binary,control,int(server),role,flags,case,*oracle.output()]);counts['Destroy']+=1
            # 일반 finder·링크 destroy·Unpop/반납까지 실제 원본 함수로 연결한다.
            for role in (0,1):
                # 링크의 destroy(0)는 부모 삭제 flags를 상속하지 않는다.
                for flags in (0,8,0x40,0x800):
                    # 정상·이미 void·이미 dead·선택 객체 삭제를 대조한다.
                    for case in (0,1,2,3):
                        oracle.prepare_destroy(True,role,flags,case,'Bridge')
                        oracle.invoke(oracle.d['destroy'],[flags],POOL+oracle.root*oracle.stride,4)
                        rows.append(['Bridge',binary,control,1,role,flags,case,*oracle.output()]);counts['Bridge']+=1
        reports[binary]=dict(binary_sha256=oracle.sha256,native_calls=dict(oracle.native),substitutions=dict(oracle.substitutions),asserts=oracle.assertions)
        paths.update(oracle.body_paths)
        # 기반 SID/생성/삭제 준비의 내보내기도 새 기록에 고정한다.
        for folder in ('sid','creation'):
            paths.update((ROOT/f'extracted/{folder}/{oracle.edition}').glob('*.c'))
            paths.update((ROOT/f'extracted/{folder}/{oracle.edition}').glob('*.tsv'))
        paths.update((ROOT/f'extracted/graphremove/{binary}').rglob('*.tsv'))
        paths.update((ROOT/f'extracted/graphremove/{binary}').rglob('*.c'))
        print(binary,'공통 destroy 완료',dict(counts),flush=True)
    if dict(counts)!=EXPECTED_CASES:raise RuntimeError('입력 수가 고정 계약과 다름')
    # 정상 상위 호출과 실제 중첩 해제/반납이 모두 실행됐는지 생성 단계에서도 확인한다.
    for binary,record in reports.items():
        if record['asserts'] or record['native_calls']!=EXPECTED_NATIVE:raise RuntimeError('원본 호출 경계: '+binary)
    FIXTURE.write_text('# 원본 공통 destroy/실제 Unpop·반납. 외부 가상 훅/종속 효과 등은 명시적 대체.\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths.update([FIXTURE,Path(__file__),ROOT/'tools/decomp_graphremove_oracle.py',ROOT/'tools/ghidra/ExportCreation.java',ROOT/'tools/ghidra/destroy-functions.json'])
    paths.update(ROOT/'tools'/name for name in DEPENDENCIES)
    report=dict(cases=dict(counts),total=sum(counts.values()),editions=reports,fpu_controls=list(CONTROLS),
        files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(paths)},
        limits=['합성 등록/타입/SHP·소진 전·표시 억제·비전투 null 큐',
            '공통 pre/post는 깊이 감소만 대체; 그래프 분할/통계/소유자 목록 효과는 이 도구에서 실행하지 않음',
            '종속 release/destroy·전파·선택·로그·파편/소리/낙하 효과 대체, 게임/OS/창 실행 없음',
            '실제 common destroy/다리 pre/post/finder/Unpop/SID 반납·삭제 기록; 합성 vtable만 출력에서 기본 기록값으로 정규화'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('새 공통 destroy x86:',report['total'])


def verify():
    """PE·도구·몸체·fixture SHA와 원본 assert/행 수를 감사한다. 기계어를 재실행하지 않는다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    # 모든 실제 사용 입력의 정확한 바이트를 확인한다.
    for name,value in report['files'].items():
        if sha(ROOT/name)!=value:raise RuntimeError('SHA 불일치: '+name)
    # 세 별도 PE와 실제 호출 수를 확인한다. Begin/Next/Unpop/Release를 대체한 기록은 통과하지 않는다.
    for binary,record in report['editions'].items():
        if sha(ROOT/BINARIES[binary])!=record['binary_sha256'] or record['asserts'] or record['native_calls']!=EXPECTED_NATIVE:raise RuntimeError('PE/assert/호출 감사 실패: '+binary)
    counts=collections.Counter(line.split('\t')[0] for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#'))
    if dict(counts)!=EXPECTED_CASES or dict(counts)!=report['cases'] or sum(counts.values())!=report['total'] or report['fpu_controls']!=list(CONTROLS):raise RuntimeError('행 수/정밀도 불일치')
    print('공통 destroy SHA/행 수/대체 경계 감사:',report['total'])


if __name__=='__main__':
    # 명시적 생성과 감사 경로를 구분한다.
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate()
