#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다리 삭제 전후와 지연 낙하 좌표를 세 실제 PE의 제한 x86으로 대조한다.

일반 탐색의 결과 목록, destroy/walker 가상 함수, 소리·제거 통지·공통 삭제, 할당·이벤트 등록은
진입점에서 대체한다. 다리 함수/좌표 절삭/참조 검사/타입 genus/좌표 포장은 실제 명령이다.
원본 게임·OS·업데이터를 실행하지 않는다. 이전 독립 도구와 기대값은 수정하지 않는다.
"""
import argparse
import collections
import csv
import hashlib
import itertools
import json
import struct
from pathlib import Path

from decomp_bridgedecay_oracle import (DecayOracle,ROOT,POOL,HEAP,CAPACITY,STOP,STACK,
    FPU_CONTROLS,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP)

# 분석 전용 가상 vtable과 두 외부 가상 호출의 진입점이다. 호스트 주소가 아니다.
VTABLE = HEAP+0x8000
DESTROY,FALL = STOP+0x100,STOP+0x110
# 새 결과 파일은 이전 붕괴 기록/fixture와 분리한다.
FIXTURE = ROOT/'cpppj/tests/fixtures/bridgeeffects-x86.tsv'
REPORT = ROOT/'cpppj/recovery-bridgeeffects-evidence.json'
# 첫 번째 참조가 dead인 연결 객체, 이를 참조하는 다음 연결 객체, 다른 타입 객체의 번호다.
BRIDGE,LINK,FIRST,SECOND,FOLLOW,OTHER = 50,60,61,62,63,64
# 판본별 실제 함수와 대체 진입점. CD의 연결 검사와 좌표 처리는 호출자에 인라인되어 있다.
SPECS = {
 'originals': dict(entries=dict(Pre=0x4221b0,Link=0x422290,Post=0x422300,Delay=0x421f90,Pack=0x421530),
   stubs={0x4b16d0:'begin',0x4b1810:'next',0x4b0950:'basepre',0x4b0840:'basepost',
     0x460600:'notify',0x4a9d70:'sound',0x4e4391:'new',0x496e80:'schedule'},link_type=0x5412c4),
 'originalCD': dict(entries=dict(Pre=0x4498e0,Post=0x449c60,Delay=0x4490b0),
   stubs={0x4eae20:'begin',0x4eafe0:'next',0x4add20:'basepre',0x4adbe0:'basepost',
     0x454810:'notify',0x438c00:'sound',0x4f1650:'new',0x48efd0:'schedule'},link_type=0x51cbb0),
}
SPECS['original1037'] = SPECS['originalCD']


def bits(value):
    """입력 float 비트를 넘긴다. 기대값 계산에는 사용하지 않는다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def sha(path):
    """감사 대상 파일의 SHA-256을 읽는다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class EffectsOracle(DecayOracle):
    """기존 격리 메모리에서 새로 검토한 함수 범위만 실행한다."""
    def __init__(self,edition):
        """기존 도구는 읽기만 하고 새 몸체 범위·후크로 교체한다."""
        super().__init__(edition)
        self.spec = dict(self.spec,entries=SPECS[edition]['entries'])
        self.effect_spec = SPECS[edition]
        self.stubs = dict(self.effect_spec['stubs'])
        self.stubs.update({DESTROY:'destroy',FALL:'fall'})
        self.allowed = bytearray(len(self.allowed))
        self.body_paths = [ROOT/f'extracted/bridgeeffects/{edition}/{name}' for name in ['creation.c','functions.tsv']]
        # 새 내보내기의 불연속 함수 범위만 허용한다.
        with self.body_paths[1].open(encoding='utf-8') as fp:
            for row in csv.DictReader(fp,delimiter='\t'):
                for part in row['ranges'].split(';'):
                    low,high = (int(value,16) for value in part.split('-'))
                    self.allowed[low-self.base:high+1-self.base] = b'\x01'*(high+1-low)
        self.native_calls = collections.Counter()
        self.precision = 0
        self.mu.mem_write(VTABLE+0x10,struct.pack('<I',DESTROY))
        self.mu.mem_write(VTABLE+0xc8,struct.pack('<I',FALL))
        self.mu.mem_write(self.effect_spec['link_type'],struct.pack('<I',75))
        # 합성 타입의 genus. walker가 아닌 객체도 섞는다.
        for number,genus in [(74,4),(75,0),(76,0x10004),(77,0x10000)]:
            self.mu.mem_write(0x11100000+number*self.type_stride+0xec,struct.pack('<I',genus))

    def prepare(self,extra=0,x=20.75,y=21.9,case=0,post=False):
        """호출 전 입력 슬롯과 탐색 결과를 만든다. 판단 결과는 계산하지 않는다."""
        self.active = [BRIDGE,LINK,FIRST,SECOND,FOLLOW,OTHER]
        # 각 슬롯을 입력 상태로 다시 쓰며 실제 효과 후 전체 바이트와 비교한다.
        for sid in self.active:
            raw = bytearray(self.stride)
            struct.pack_into('<I',raw,0,VTABLE)
            raw[10] = 75 if sid in [LINK,FOLLOW] else 74
            if sid==FIRST: raw[11] = 2
            struct.pack_into('<ff',raw,14,x,y)
            raw[self.o['extra']] = extra if sid==BRIDGE else 0
            self.mu.mem_write(self.slot(sid),bytes(raw))
        self.mu.mem_write(self.slot(LINK)+12,struct.pack('<H',FIRST))
        self.mu.mem_write(self.slot(LINK)+8,struct.pack('<H',SECOND))
        self.mu.mem_write(self.slot(FOLLOW)+12,struct.pack('<H',LINK))
        self.mu.mem_write(self.slot(FOLLOW)+8,struct.pack('<H',SECOND))
        selections = [[],[LINK,FOLLOW,OTHER],[FOLLOW,LINK,OTHER],[LINK,LINK,FOLLOW],[OTHER],[LINK,FOLLOW],[FOLLOW,LINK]]
        self.selected = selections[case]
        self.dynamic = False
        if case==5: self.mu.mem_write(self.slot(FIRST)+self.o['extra'],b'\x09')
        if case==6: self.mu.mem_write(self.slot(SECOND)+11,b'\x02')
        if post:
            plans = [[],[LINK,FOLLOW,OTHER],[LINK,FOLLOW,OTHER],[LINK,LINK,OTHER],[FOLLOW]]
            self.selected = plans[case]
            self.mu.mem_write(self.slot(LINK)+10,b'\x4d')
            self.mu.mem_write(self.slot(FOLLOW)+10,b'\x4b')
            self.mu.mem_write(self.slot(OTHER)+10,b'\x4c')
            self.dynamic = case==2
        self.position = 0
        self.before = {sid:bytes(self.mu.mem_read(self.slot(sid),self.stride)) for sid in self.active}
        self.payload = None

    def on_instruction(self,mu,address,size,data):
        """기계어 함수 진입을 세고 검토한 외부 효과만 기록·대체한다."""
        for name,entry in self.spec['entries'].items():
            if address==entry: self.native_calls[name]+=1
        stub = self.stubs.get(address)
        if not stub: return super().on_instruction(mu,address,size,data)
        self.instructions+=1
        self.stub_calls[stub]+=1
        esp = mu.reg_read(UC_X86_REG_ESP)
        values = struct.unpack('<IIIIII',mu.mem_read(esp,24))
        ret,a0,a1,a2,a3,_ = values
        ecx = mu.reg_read(UC_X86_REG_ECX)
        purge = 0
        if stub=='begin':
            # 실제 좌표 절삭·범위 구성이 끝난 일반 탐색기의 시작점이다.
            self.events.append('Q:'+':'.join(str(struct.unpack('<i',struct.pack('<I',v))[0]) for v in [a0,a1,a2,a3]))
            self.finder = ecx
            self.position = 0
            mu.mem_write(ecx+0x34,struct.pack('<I',self.selected[0] if self.selected else 0))
            mu.reg_write(UC_X86_REG_EAX,ecx)
            purge = 16
        elif stub=='next':
            if ecx!=self.finder: raise RuntimeError('다른 탐색기의 next')
            self.position+=1
            mu.mem_write(ecx+0x34,struct.pack('<I',self.selected[self.position] if self.position<len(self.selected) else 0))
        elif stub in ['destroy','fall']:
            sid = self.sid_of(ecx)
            self.events.append(('D' if stub=='destroy' else 'F')+f':{sid}'+(f':{a0}' if stub=='destroy' else ''))
            raw = mu.mem_read(ecx+11,1)[0]
            mu.mem_write(ecx+11,bytes([raw|2]))
            if stub=='destroy': purge=4
            elif self.dynamic and sid==LINK:
                mu.mem_write(self.slot(FOLLOW)+10,b'\x4c')
        elif stub in ['basepre','basepost']:
            self.events.append(('A' if stub=='basepre' else 'B')+f':{self.sid_of(ecx)}:{a0}')
            purge=4
        elif stub=='notify': self.events.append(f'N:{a0}')
        elif stub=='sound':
            name = bytes(mu.mem_read(a2,15)).split(b'\0')[0]
            if name!=b'bridgeFall.wav' or a3!=0: raise RuntimeError('다른 소리')
            self.events.append(f'S:{a0}:{a1}')
        elif stub=='new':
            if a0!=0x28: raise RuntimeError('다른 할당 크기')
            mu.reg_write(UC_X86_REG_EAX,HEAP)
        elif stub=='schedule':
            if a0!=0x2692 or ecx!=HEAP: raise RuntimeError('다른 이벤트 등록')
            self.events.append(f'E:{a0}:{a1}:{a2}')
            self.payload = a2
            purge=12
        mu.reg_write(UC_X86_REG_ESP,esp+4+purge)
        mu.reg_write(UC_X86_REG_EIP,ret)

    def invoke(self,name,args,ecx,purge,control):
        """정상 반환·스택·x87·FS 복구를 기존 엄격 호출기로 확인한다."""
        self.precision=control
        self.call(name,args,ecx,purge,control)
        # 대체된 destroy/fall/type 전환 외에 슬롯 변화가 없어야 한다.
        for sid,before in self.before.items():
            after = bytearray(self.mu.mem_read(self.slot(sid),self.stride))
            original = bytearray(before)
            after[10:12]=original[10:12]
            if after!=original: raise RuntimeError(f'예상하지 않은 슬롯 변경 {sid}')
        return ';'.join(self.events) or '-'

    def states(self):
        """콜백 이후 슬롯 타입·state를 직접 읽는다."""
        return ';'.join(f'{sid},{self.mu.mem_read(self.slot(sid)+10,1)[0]},{self.mu.mem_read(self.slot(sid)+11,1)[0]}' for sid in self.active)


def inputs():
    """원본 판단을 포함하지 않는 입력 조합과 순서 변형을 만든다."""
    references = [(FIRST,SECOND),(0,SECOND),(FIRST,0),(CAPACITY,SECOND),(FIRST,CAPACITY),(65535,SECOND),(FIRST,65535)]
    states = [(0,0),(2,0),(0,2),(2,2),(1,4),(3,0),(4,6)]
    extras = [(0,0),(1,0),(8,0),(9,0),(16,0),(0,1),(0,8),(0,9),(1,8)]
    return references,states,extras


def generate():
    """세 실제 PE와 두 x87 정밀도에서 새 fixture·SHA 근거를 만든다."""
    rows = []
    counts = collections.Counter()
    reports = {}
    references,states,extras = inputs()
    # 0.999989986의 경계 위/아래를 넣어 무보정 절삭이나 표면 조회의 0.9999와 구별한다.
    coordinates = [(-1.75,2.9),(20.75,21.9),(0.0,-0.0),(255.99,254.01),(20.00002,21.00002),(20.000005,21.000005)]
    payload_values = [-1.75,0.0,1.999,255.99,256.75,65535.0,-65536.0,8388609.0,1e30,float('inf'),float('nan')]
    # PE마다 실제 바이트와 허용 범위를 별도로 사용한다.
    for edition in SPECS:
        oracle = EffectsOracle(edition)
        for control in FPU_CONTROLS:
            # 직접 helper는 패치판에만 존재한다. CD/10.37의 같은 인라인 조건은 Pre 행으로 검사한다.
            if edition=='originals':
                for (first,second),(a,b),(extra_a,extra_b) in itertools.product(references,states,extras):
                    oracle.prepare()
                    oracle.mu.mem_write(oracle.slot(LINK)+12,struct.pack('<H',first))
                    oracle.mu.mem_write(oracle.slot(LINK)+8,struct.pack('<H',second))
                    oracle.mu.mem_write(oracle.slot(FIRST)+11,bytes([a]))
                    oracle.mu.mem_write(oracle.slot(SECOND)+11,bytes([b]))
                    oracle.mu.mem_write(oracle.slot(FIRST)+oracle.o['extra'],bytes([extra_a]))
                    oracle.mu.mem_write(oracle.slot(SECOND)+oracle.o['extra'],bytes([extra_b]))
                    oracle.before={sid:bytes(oracle.mu.mem_read(oracle.slot(sid),oracle.stride)) for sid in oracle.active}
                    events=oracle.invoke('Link',[],oracle.slot(LINK),0,control)
                    rows.append(['Link',edition,control,first,second,a,b,extra_a,extra_b,events])
                    counts['Link']+=1
            for name,cases in [('Pre',7),('Post',5)]:
                for extra,flags,(x,y),case in itertools.product([0,1,8,9,16],[0,0x2000800],coordinates,range(cases)):
                    oracle.prepare(extra,x,y,case,name=='Post')
                    events=oracle.invoke(name,[flags],oracle.slot(BRIDGE),4,control)
                    rows.append([name,edition,control,extra,flags,bits(x),bits(y),case,events,oracle.states()])
                    counts[name]+=1
            for x,y in itertools.product(payload_values,payload_values):
                oracle.prepare()
                events=oracle.invoke('Delay',[BRIDGE,bits(x),bits(y)],0,0,control)
                rows.append(['Delay',edition,control,bits(x),bits(y),oracle.payload])
                counts['Delay']+=1
                if edition=='originals':
                    wrapper=oracle.payload
                    oracle.prepare()
                    oracle.invoke('Pack',[bits(x),bits(y)],oracle.slot(BRIDGE),8,control)
                    if oracle.payload!=wrapper: raise RuntimeError('wrapper/direct payload 불일치')
                    rows.append(['Pack',edition,control,bits(x),bits(y),oracle.payload])
                    counts['Pack']+=1
        reports[edition] = dict(binary=oracle.effect_spec.get('binary',oracle.spec['binary']),sha256=oracle.sha256,
            entries={k:f'{v:08x}' for k,v in oracle.spec['entries'].items()},
            native_calls=dict(oracle.native_calls),stubs=dict(oracle.stub_calls),assertions=oracle.assertions)
    FIXTURE.write_text('# 실제 PE 제한 x86 다리 삭제 전후/낙하 좌표 기대값; 일반 탐색과 외부 효과는 계약 대체\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths = [Path(__file__),ROOT/'tools/decomp_bridgedecay_oracle.py',FIXTURE]
    # 기존 도구가 생성한 기록과 입력 PE/새 내보내기도 해시를 고정한다.
    for edition in SPECS:
        paths.extend([ROOT/reports[edition]['binary'],ROOT/f'extracted/bridgeeffects/{edition}/creation.c',ROOT/f'extracted/bridgeeffects/{edition}/functions.tsv'])
    report = dict(cases=dict(counts),total=sum(counts.values()),editions=reports,os_calls=0,
        files={p.relative_to(ROOT).as_posix():sha(p) for p in paths},
        limits=['prepared ordered finder results','external destroy/walker/base hooks','synthetic type metadata','no GameWorld or event execution'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(cases=dict(counts),total=sum(counts.values())),ensure_ascii=False))


def verify():
    """저장된 입력/도구/fixture/몸체 SHA와 행 수를 확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['files'].items():
        if sha(ROOT/name)!=expected: raise RuntimeError(f'SHA 불일치: {name}')
    rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    counts=collections.Counter(line.split('\t',1)[0] for line in rows)
    if dict(counts)!=report['cases'] or len(rows)!=report['total']: raise RuntimeError('fixture 행 수 불일치')
    if any(item['assertions'] for item in report['editions'].values()): raise RuntimeError('assert 도달')
    print(f'bridgeeffects 검증 통과: {len(rows)}개')


def main():
    """새 기대값 생성과 저장 결과 감사 중 하나를 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    verify() if args.verify else generate()


if __name__=='__main__':
    main()
