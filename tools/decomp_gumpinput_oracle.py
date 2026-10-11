#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 캡처/dialog/overlay 입력을 실제 선택/전달/위치/경계/CRT와 함께 격리 실행한다."""
import argparse
import collections
import csv
import json
import random
import struct
from decomp_gumpevents_oracle import GumpEventsOracle, nodes, changed, encoded as event_encoded, TOKENS, DATA, SIZE, COUNT, VTABLE
from decomp_drawsort_oracle import REGISTERS
from decomp_process_oracle import (ROOT,BINARIES,CONTROLS,STACK,STOP,STUBS,POOL,digest,fbits,
    UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 새 함수/공유 전역/전체 표와 원본 dialog 로그 경계의 주소다.
SPECS={
    'originals':dict(Capture=0x465250,Override=0x4654c0,Dialog=0x4654f0,Route=0x466200,Overlay=0x466150,Remove=0x465550,
        extra_globals=(0x565dd0,0x565dd8,0x565ddc,0x5427e4),tables=((0x563ca8,100),(0x565d88,16)),log=0x4c2630),
    'originalCD':dict(Capture=0x494120,Override=0x494510,Dialog=0x494540,Route=0x4928a0,Overlay=0x4927f0,Remove=0x4945a0,
        extra_globals=(0x535e88,0x535e90,0x535e94,0x51296c),tables=((0x56a480,100),(0x56a430,16)),log=0x48ca70),
}
SPECS['original1037']=dict(SPECS['originalCD'])
# 입력 시드와 완성 관찰/근거 보고서의 저장 경로다.
SEED=0x46620078
FIXTURE=ROOT/'cpppj/tests/fixtures/gumpinput-x86.tsv'
REPORT=ROOT/'cpppj/recovery-gumpinput-evidence.json'


def plan():
    """캡처/등록/우선 전달/overlay의 독립 raw 입력과 경계 응답만 만든다."""
    result=[]
    # 기본 원본 트리는 부모 visible이며 캐시/선택 전역은 기대 결과로 초기화하지 않는다.
    def add(actions,raw=None,focus=0,dialog=0,count=0,dialogs=None,overlay=None,binding=0,responses=(),anchors=(),root=None,override=TOKENS[5]):
        """표의 미사용 칸까지 복사하고 예상 대상/성공값은 계산하지 않는다."""
        raw=list(raw or nodes());raw[0]=changed(raw[0],**{'4':1}) if root is None else raw[0]
        result.append(dict(actions=list(actions),nodes=raw,binding=binding,responses=list(responses),
            anchors=list(anchors) if anchors else [(index,0,0) for index in range(COUNT)],
            globals=[TOKENS[0] if root is None else root,dialog,17,0xffffffe9,0x1234,0x80000000,TOKENS[4],focus,override,count,0x87654321],
            tables=[list(overlay or [0]*100),list(dialogs or [TOKENS[5]]*16)]))
    # 빈 캡처에만 등록하고 기존 캡처는 교체하지 않는다. 중간 상태가 남는 단일 호출이다.
    for focus in (0,TOKENS[1],TOKENS[2]):
        # 캡처하려는 토큰은 기존 캡처와 같거나 다르거나 0이다.
        for token in (0,TOKENS[1],TOKENS[2]): add([('Capture',token)],focus=focus)
    add([('Capture',TOKENS[1]),('Capture',TOKENS[2])])
    # 단일 override는 이미 차 있으면 enabled여도 해제한다.
    for initial in (0,TOKENS[1],TOKENS[2]):
        # 대상과 현재 override의 동일 여부도 독립 입력이다.
        for token in (0,TOKENS[1],TOKENS[2]):
            # 원본 정수 enabled는 0 또는 비영이다.
            for enabled in (0,7): add([('Override',token,enabled)],override=initial)
    # 사용 구간의 중복과 미사용 칸/한도 로그의 우선 순서를 나눈다.
    for count in (0,1,5,15,16,17):
        # 중복/새 토큰/0과 전체 한도를 각각 실행한다.
        for token in (0,TOKENS[1],TOKENS[2],TOKENS[3]):
            table=[TOKENS[1] if i%2==0 else TOKENS[2] for i in range(16)]
            if count<16: table[count]=token
            add([('Dialog',0,token)],count=count,dialogs=table,dialog=TOKENS[4])
    # 등록/중복/실제 CRT 제거 뒤 현재 dialog의 기본 가상 필터를 사용한다.
    for event in (1,0x10000,0x2000001,0x80000001):
        add([('Dialog',0,TOKENS[1]),('Dialog',0,TOKENS[2]),('Dialog',0,TOKENS[1]),('Remove',0,TOKENS[1]),
            ('Route',0,event,11,21,0)],binding=1,responses=[('Handle',TOKENS[2],9,())])
    # 캡처/현재 dialog/루트의 실제 전달이며 필터/처리기 응답은 명시 입력이다.
    for focus in (0,TOKENS[1],TOKENS[2]):
        # 현재 dialog가 캡처와 같거나 다르거나 없는 상태다.
        for dialog in (0,TOKENS[1],TOKENS[2]):
            # 전체 실패와 각 객체의 명시 처리기 성공 응답이다.
            for winner in (-1,0,1,2,3,4):
                raw=nodes();raw[3]=changed(raw[3],**{'4':0})
                responses=[('Filter',token,1,()) for token in TOKENS]
                if winner>=0: responses.append(('Handle',TOKENS[winner],0x80000007,()))
                add([('Route',0,0x81234567,0x76543210,0xfedcba98,7 if winner in (0,3) else 0)],raw,focus=focus,dialog=dialog,responses=responses)
    # 숨은 자식의 강제 전달 인자는 루트와 dialog 두 경로 모두 실제 하위 전달까지 간다.
    for dialog in (0,TOKENS[1]):
        # 최상위 force 값이 실제 하위 전달까지 가는지 관찰한다.
        for force in (0,7):
            raw=nodes();raw[3]=changed(raw[3],**{'4':0})
            add([('Route',0,1,2,3,force)],raw,dialog=dialog,responses=[('Filter',t,1,()) for t in TOKENS]+[('Handle',TOKENS[3],9,())])
    # 성공/실패 처리기 뒤 현재 캡처를 다시 읽는다. 실패해도 루트로 재전달하지 않는다.
    for focus in (TOKENS[1],TOKENS[2]):
        # 처리기가 설정한 현재 캡처를 성공/실패 두 경로에서 읽는다.
        for replacement in (0,TOKENS[2],TOKENS[3]):
            # 처리기의 raw DWORD만 공급하며 기대 반환은 계산하지 않는다.
            for value in (0,0x80000007):
                add([('Route',0,1,2,3,7)],focus=focus,responses=[('Handle',focus,value,((0,7,replacement),)),('Filter',TOKENS[0],1,()),('Handle',TOKENS[0],9,())])
    # overlay는 모든 조상 visible/자신 dead/실제 선택의 void 및 signed 깊이를 확인한다.
    for root_flags in (0,1,0x10001):
        # 자기 객체의 visible/void/dead/화면 밖 자식 통과 플래그다.
        for flags in (0,1,9,0x10001,0x21):
            # 루트 직계 자식과 손자의 실제 부모 조회를 실행한다.
            for target in (1,2,3,4):
                raw=nodes();raw[0]=changed(raw[0],**{'4':root_flags});raw[target]=changed(raw[target],**{'4':flags,'48':0xffffffff})
                table=[0]*100;table[0]=TOKENS[target]
                add([('Overlay',0,0x11223344,11,21)],raw,root=TOKENS[0],overlay=table,responses=[('Handle',TOKENS[target],0x80000007,())])
    # 경계 안/끝/밖을 실제 Bounds와 전체 Pick에서 독립 확인한다.
    for target in (1,3):
        # 동일 객체의 왼쪽/위/오른쪽/아래 경계를 교차한다.
        for x,y in ((10,20),(21,28),(22,20),(10,29),(-1,-1)):
            raw=nodes();raw[target]=changed(raw[target],**{'48':0xffffffff});table=[0]*100;table[0]=TOKENS[target]
            add([('Overlay',0,1,x,y)],raw,overlay=table,responses=[('Handle',TOKENS[target],7,())])
    # 첫/끝 슬롯과 중복: 첫 일치의 반환 0도 다음 overlay를 호출하지 않는다.
    for slot in (0,1,99):
        # 반환 0과 상위 비트가 있는 성공값의 두 독립 응답이다.
        for value in (0,0x80000007):
            raw=nodes();raw[1]=changed(raw[1],**{'48':0xffffffff});table=[0]*100;table[slot]=TOKENS[1]
            if slot<99: table[99]=TOKENS[1]
            add([('Overlay',0,1,11,21)],raw,overlay=table,responses=[('Handle',TOKENS[1],value,((0,10,123),))])
    # dialog/root 0은 하위 호출 없이 끝나지만 실제 dialog가 있으면 루트 0이어도 전달한다.
    for root,dialog in ((0,0),(0,TOKENS[2])):
        add([('Route',0,1,2,3,0)],root=root,dialog=dialog,binding=1,responses=[('Handle',TOKENS[2],7,())])
    # 한도 로그의 콜백 쓰기를 보존하며 등록 실패에서는 추가 표 쓰기가 없다.
    add([('Dialog',0,TOKENS[1])],count=16,responses=[('Overflow',19,0,((0,1,TOKENS[3]),(2,15,TOKENS[4])))])
    # 세계 투영/위치/경계/전체 선택은 실제 몸체이며 투영 정수점만 공급한다.
    for target in (1,2):
        raw=nodes();raw[target]=changed(raw[target],**{'4':0x2001,'48':0xffffffff,'76':fbits(.75),'80':fbits(-.75)})
        table=[0]*100;table[0]=TOKENS[target]
        add([('Overlay',0,1,12,23)],raw,overlay=table,anchors=[(target,11,22)],responses=[('Handle',TOKENS[target],7,())])
    # 경계 호출에서 미래 overlay 칸과 선택/노드 전역을 바꾸는 명시 입력이다.
    raw=nodes();raw[1]=changed(raw[1],**{'4':0x2001});raw[2]=changed(raw[2],**{'48':0xffffffff})
    table=[0]*100;table[0]=TOKENS[1]
    add([('Overlay',0,1,11,21)],raw,overlay=table,anchors=[(1,10,20)],responses=[('Project',1,0,((1,99,TOKENS[2]),)),('Handle',TOKENS[2],5,())])
    rng=random.Random(SEED)
    # 실제 이벤트 종류/플래그/깊이/overlay 위치를 유한 임의 입력으로 교차한다.
    for index in range(24):
        raw=nodes()
        # 정상 링크는 유지하고 각 객체의 소비 플래그/깊이만 바꾼다.
        for item in range(1,COUNT): raw[item]=changed(raw[item],**{'4':rng.choice((0,1,9,0x201,0x401,0x801,0x10001)),'48':rng.getrandbits(32)})
        table=[0]*100;table[index%100]=rng.choice(TOKENS[1:])
        add([('Route',0,rng.choice((1,0x10000,0x2000001)),11,21,index%2),('Overlay',0,0x11223344,11,21)],raw,
            overlay=table,binding=1,responses=[('Handle',token,rng.choice((0,7,0x80000007)),()) for token in TOKENS])
    return result


def tables_encoded(tables):
    """표의 모든 실제 DWORD를 배열 순서로 기록한다."""
    return ';'.join(','.join(map(str,values)) for values in tables)


def encoded(case):
    """완성 입력은 이벤트 직렬화를 공유하고 두 전체 표를 추가한다."""
    fields=event_encoded(case);return [fields[0],fields[1],tables_encoded(case['tables']),*fields[2:]]


class GumpInputOracle(GumpEventsOracle):
    """새 입력 몸체와 기존 실제 선택/전달/등록 제거의 정상 실행만 관찰한다."""
    def __init__(self,edition):
        """서로 연결되는 실제 원본 몸체와 표/로그 경계 주소를 준비한다."""
        super().__init__(edition);spec=SPECS[edition];self.d=dict(self.d,**spec)
        self.d['globals']=self.d['globals']+spec['extra_globals']
        self.extra.extend(ROOT/f'extracted/{group}/{edition}/{name}' for group in ('gumpinput','gumplifetime') for name in ('creation.c','functions.tsv'))
        # 기존 지연 삭제 묶음에서 실제 Remove/CRT 몸체를 공유한다.
        for group in ('gumpinput','gumplifetime'):
            with (ROOT/f'extracted/{group}/{edition}/functions.tsv').open(encoding='utf-8') as fp:
                # 함수 진입과 실행 허용 범위를 실제 내보낸 메타데이터로 제한한다.
                for row in csv.DictReader(fp,delimiter='\t'):
                    self.entry_names[int(row['entry'],16)]=row['entry']
                    # 함수의 불연속 조각 사이 미검토 주소는 허용하지 않는다.
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def tables(self):
        """두 전체 표를 실제 메모리에서 읽는다."""
        return tables_encoded([struct.unpack('<'+'I'*size,self.mu.mem_read(base,size*4)) for base,size in self.d['tables']])

    def response(self,kind,token,args):
        """콜백 직전 전체 전역/노드/표를 관찰하고 명시 응답 쓰기만 수행한다."""
        self.calls.append(','.join(map(str,(kind,token,*args)))+'@'+self.state()+'@'+self.nodes()+'@'+self.tables());self.boundaries[kind]+=1
        value,patches=self.responses.get((kind,token),(0,()))
        # target 0은 전역 index, 1/2는 표 index이며 나머지는 실제 노드 토큰이다.
        for target,offset,number in patches:
            address=self.d['globals'][offset] if target==0 else self.d['tables'][target-1][0]+offset*4 if target in (1,2) else target+offset
            self.put(address,number&0xffffffff)
        return value

    def on_instruction(self,mu,address,size,data):
        """dialog 한도 로그만 추가 경계이며 선택/전달/좌표와 CRT는 실제 명령이다."""
        if address==self.d['log']:
            esp=mu.reg_read(UC_X86_REG_ESP);code,pointer=struct.unpack('<2I',mu.mem_read(esp+4,8))
            if bytes(mu.mem_read(pointer,28))!=b'Dialog gump list overflow.\n\x00': raise RuntimeError('dialog 로그 인자 오류')
            self.response('Overflow',code,());mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,self.u32(esp));return
        super().on_instruction(mu,address,size,data)

    def run(self,case,control):
        """전체 상태/콜백/정상 반환/ABI/x87/SEH/허용 쓰기를 관찰한다. void EAX는 비교하지 않는다."""
        mu,d=self.mu,self.d;self.calls,self.results=[],[];self.entries,self.row_returns=collections.Counter(),collections.Counter()
        self.responses={(kind,token):(value,patches) for kind,token,value,patches in case['responses']};self.anchors={sid:(x,y) for sid,x,y in case['anchors']}
        mu.mem_write(DATA,b'\xa5'*4096)
        # 노드/SID/전역/두 전체 표는 완성 raw 입력이다.
        for index,(token,raw) in enumerate(zip(TOKENS,case['nodes'],strict=True)):
            mu.mem_write(token,raw);self.put(POOL+index*self.c['stride']+14,index);self.put(POOL+index*self.c['stride']+18,0)
        self.put(self.c['pool'],POOL)
        # 입력 전역의 전체 DWORD 비트를 보존한다.
        for address,value in zip(d['globals'],case['globals'],strict=True): self.put(address,value)
        # 미사용 칸까지 포함한 overlay/dialog 표의 raw 입력이다.
        for (base,size),values in zip(d['tables'],case['tables'],strict=True): mu.mem_write(base,struct.pack('<'+'I'*size,*values))
        self.put(VTABLE+0x28,STUBS+0xb20);self.put(VTABLE+0x34,d['Base'] if case['binding'] else STUBS+0xb00)
        before_pool=bytes(mu.mem_read(POOL,COUNT*self.c['stride']));before_vtable=bytes(mu.mem_read(VTABLE,64))
        self.writes=[(0,4),(STACK-0xf000,STACK+4096)]+[(token+108,token+116) for token in TOKENS]+[(address,address+4) for address in d['globals']]+[(d['tables'][1][0],d['tables'][1][0]+64)]
        # 연속 연산에서는 실제 상태를 유지하고 호출 프레임만 독립 초기화한다.
        for op,token,*args in case['actions']:
            # 호출자가 보존을 요구하는 레지스터는 매 연산에서 독립 비교한다.
            for reg,value in REGISTERS.items(): mu.reg_write(reg,value)
            # 휘발 임시 레지스터는 원본 정상 ABI에서 보존을 요구하지 않는다.
            for reg in (UC_X86_REG_EAX,UC_X86_REG_EDX): mu.reg_write(reg,0)
            mu.reg_write(UC_X86_REG_ECX,token);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
            mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);self.put(0,0xffffffff)
            mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*(value&0xffffffff for value in args)))
            mu.emu_start(d[op],STOP,timeout=30000000,count=2000000)
            purge=len(args)*4 if op in ('Capture','Override') else 0
            if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+purge or any(mu.reg_read(reg)!=value for reg,value in REGISTERS.items()): raise RuntimeError(f'Gump 입력 정상 반환/ABI 오류: {self.binary} {op}')
            if mu.reg_read(UC_X86_REG_FPSW)&0x3800 or mu.reg_read(UC_X86_REG_FPCW)!=control or self.u32(0)!=0xffffffff: raise RuntimeError('Gump 입력 x87/SEH 오류')
            self.results.append('-' if op in ('Capture','Override') else str(mu.reg_read(UC_X86_REG_EAX)))
            self.row_returns[f'{d[op]:08x}']+=1;self.returns[f'{d[op]:08x}']+=1
        protected=bytearray(mu.mem_read(DATA,4096))
        # 전체 노드 전후와 읽기 전용 SID/가상 표의 바이트를 보호한다.
        for token in TOKENS: protected[token-DATA:token-DATA+SIZE]=b'\xa5'*SIZE
        if protected!=b'\xa5'*4096 or before_pool!=bytes(mu.mem_read(POOL,COUNT*self.c['stride'])) or before_vtable!=bytes(mu.mem_read(VTABLE,64)): raise RuntimeError('Gump 입력 보호/읽기 전용 변경')
        return [*encoded(case),self.nodes(),self.state(),self.tables(),','.join(self.results),'|'.join(self.calls) or '-',json.dumps(dict(self.entries),sort_keys=True,separators=(',',':')),json.dumps(dict(self.row_returns),sort_keys=True,separators=(',',':'))]


def generate(smoke):
    """두 x87의 전체 관찰이 같을 때만 기록한다. 예상 UI 동작을 계산하지 않는다."""
    rows,editions=[],{};files={FIXTURE,ROOT/'tools/ghidra/gumpinput-functions.json'}
    files.update(ROOT/'tools'/name for name in ('decomp_gumpinput_oracle.py','decomp_gumpevents_oracle.py','decomp_gumpcollection_oracle.py','decomp_drawsort_oracle.py','decomp_process_oracle.py'))
    # 각 판본의 실제 원본 몸체와 명시 입력 경계를 독립 메모리에서 실행한다.
    for edition in BINARIES:
        oracle=GumpInputOracle(edition);indexed=list(enumerate(plan()))
        if smoke: indexed=indexed[::23]+indexed[-5:]
        # 각 입력의 두 x87 전체 관찰은 동일해야 한다.
        for index,case in indexed:
            observed=[oracle.run(case,control) for control in CONTROLS]
            if observed[0]!=observed[1]: raise RuntimeError(f'Gump 입력 x87 차이: {edition} {index}')
            rows.append([edition,str(index),*observed[0]])
        editions[edition]=dict(cases=len(indexed),entries=dict(oracle.hits),returns=dict(oracle.returns),boundaries=dict(oracle.boundaries))
        files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(f'{edition}: {len(indexed)}개 통과',flush=True)
    if smoke: return
    FIXTURE.write_text('# Gump 입력 캡처/dialog/override/overlay와 실제 선택·전달·위치/경계/CRT의 전체 관찰.\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-12',controls=list(CONTROLS),os_calls=0,assertions=0,editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},
        limits=['새 입력/부모 visible와 실제 기본 필터/전달/전체 선택/위치/경계/CRT/dialog 제거의 전체 정상 몸체',
            '전체 6노드·11전역·100/16칸 두 표·반환·콜백 직전 전체 상태/인자',
            '가상 필터/처리기·투영·dialog 한도 로그만 명시 입력 경계; void EAX는 의미 비교 제외',
            'OS 사건 큐/파생 버튼·전체 Client UI/손상 링크·실제 예외 전파 제외',
            '정상 반환/보존 레지스터/스택/x87/SEH/허용 실행·쓰기/보호·읽기 전용/SHA/집계 감사']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA/독립 완성 입력/전체 폭/진입·정상 반환·경계 집계만 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 실행한 도구/원본/내보내기/fixture의 정확한 바이트를 감사한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha: raise RuntimeError('Gump 입력 SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls'] or report['assertions'] or set(report['editions'])!=set(BINARIES): raise RuntimeError('Gump 입력 총계 오류')
    # 저장 관찰을 집계하고 예상 선택/전달 결과는 재계산하지 않는다.
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition];entries,returns,boundaries=collections.Counter(),collections.Counter(),collections.Counter()
        if len(chosen)!=len(plan()) or item['cases']!=len(chosen): raise RuntimeError('Gump 입력 판본 행 수 오류')
        # 완성 입력의 순서와 전체 노드/전역/표 폭을 검증한다.
        for index,(row,case) in enumerate(zip(chosen,plan(),strict=True)):
            if len(row)!=16 or row[1]!=str(index) or row[2:9]!=encoded(case): raise RuntimeError('Gump 입력 순서/입력 오류')
            if len(row[9].split(';'))!=COUNT or any(len(bytes.fromhex(part.split('=')[1]))!=SIZE for part in row[9].split(';')) or len(row[10].split(','))!=11 or tuple(len(part.split(',')) for part in row[11].split(';'))!=(100,16): raise RuntimeError('Gump 입력 전체 폭 오류')
            if len(row[12].split(','))!=len(case['actions']) or dict(collections.Counter(f'{SPECS[edition][action[0]]:08x}' for action in case['actions']))!=json.loads(row[15]): raise RuntimeError('Gump 입력 정상 반환 누락')
            # 저장 진입 관찰은 두 모드 집계의 정확히 절반이다.
            for address,count in json.loads(row[14]).items(): entries[address]+=count*2
            # 최상위 정상 반환도 실제 주소별 횟수를 집계한다.
            for address,count in json.loads(row[15]).items(): returns[address]+=count*2
            if row[13]!='-':
                # 경계 종류는 실제 호출 기록에서만 읽는다.
                for call in row[13].split('|'): boundaries[call.split(',')[0]]+=2
        if dict(entries)!=item['entries'] or dict(returns)!=item['returns'] or dict(boundaries)!=item['boundaries']: raise RuntimeError('Gump 입력 호출 집계 오류')
    print(f'gumpinput 감사 통과: {len(rows)}개, 두 정밀도 정상 반환 {sum(sum(value["returns"].values()) for value in report["editions"].values())}회, OS/assert 0')


def main():
    """전체 기록/무저장 표본/저장 근거 감사 중 하나를 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify: verify()
    else: generate(args.smoke)


if __name__=='__main__': main()
