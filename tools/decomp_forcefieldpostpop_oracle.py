#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호막 postPop/Regular/좌표 타입 finder를 세 실제 PE에서 정상 반환까지 실행한다.

Regular 확보/생성·공통 postPop·프레임 진행·가상 destroy만 명시 기록 대체한다.
실제 보호막 가상 표·후처리/사건 분기·좌표/일반 finder/CRT/반환 상수는 원본 명령이다.
게임/OS/창은 실행하지 않는다. --verify는 저장된 SHA/입력/진입/ABI 근거를 감사한다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_priestforcefield_oracle import ForcefieldOracle, CONTROLS, TARGET, LISTS, ROOT, SPECS, STOP, STACK, digest
from decomp_owner_oracle import OwnerOracle
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 원본이 아닌 격리 메모리의 확보 반환 주소/float 관찰 출력/복귀 명령이다.
MEMORY, RESULT, LAND = LISTS+0x8000, LISTS+0x9000, STOP+0x400
# 실제 보호막의 몸체/전역/가상 표와 확보 경계다.
BODY={
 'originals':dict(prefix=0x448d20,handler=0x448e00,find=0x4b1fa0,base=0x4b0d30,new=0x4e4391,
    regular=0x496e80,advance=0x4afc90,destroy=0x4af780,table=0x507770,priest=0x5412d0,debug=0x5e4794),
 'originalCD':dict(prefix=0x4847d0,handler=0x4848c0,find=0x4eb4b0,base=0x4ae180,new=0x4f1650,
    regular=0x48efd0,advance=0x4acce0,destroy=0x4ab7e0,table=0x5042b8,priest=0x51cbbc,debug=0x540a24),
}
BODY['original1037']=dict(BODY['originalCD'])
# 결과 fixture/근거는 앞 단계 파일과 분리한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/forcefieldpostpop-x86.tsv'
REPORT=ROOT/'cpppj/recovery-forcefieldpostpop-evidence.json'
KEYS=('kind','flags','first','second','event','count','payload','x','y','priestType','layout','extra')


def bits(value):
    """단정도 입력을 반올림한 뒤 비트로 저장한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def inputs():
    """분기 기대값 없이 확보/flags와 사건/공간/현재 타입 입력을 고정 순서로 만든다."""
    rows=[]
    # 두 확보 실패를 독립으로 교차하고 extra는 접두의 차단 조건이 아님을 관찰한다.
    for flags,first,second,extra in itertools.product((0,1,8,9,0x800,0x801,0x2000,0xffffffff),(0,1),(0,1),(0,1,8,9)):
        rows.append(('Prefix',flags,first,second,0,0,0,bits(20.75),bits(21.9),158,0,extra))
    # count/payload는 처리기가 읽지 않는 인자다. 높은 DWORD 타입을 BYTE로 줄이지 않는다.
    for event,pos,typ,layout,count,payload in itertools.product((1,2,99),((20.75,21.9),(1.0,1.0),(254.9,254.25)),(158,159,256),range(10),(0,0xffffffff),(0,0x7fc12345)):
        rows.append(('Regular',0,0,0,event,count,payload,bits(pos[0]),bits(pos[1]),typ,layout,0))
    return rows


def nodes(case):
    """장면의 합성 후보만 공급한다. 존재 판단과 탐색 순서는 원본이 정한다."""
    x,y=(struct.unpack('<f',struct.pack('<I',case[k]))[0] for k in ('x','y'))
    layout=case['layout']
    if layout==0: return []
    typ=159 if layout in (2,9) else 158
    state=2 if layout==4 else 4 if layout==5 else 0
    extra=8 if layout==3 else 0
    width=2 if layout==7 else 1
    ax=min(255.5,x+(1 if layout==7 else 2 if layout==8 else 0))
    result=[(60,typ,state,extra,bits(ax),bits(y),width,1,8 if layout==6 else 1,1)]
    if layout==9: result.append((61,158,0,0,case['x'],case['y'],1,1,2,3))
    return result


class LifecycleOracle(ForcefieldOracle):
    """기존 실제 일반 finder 실행기에 보호막 후처리/처리기와 명시 경계만 추가한다."""
    def __init__(self,edition):
        """읽기 전용 몸체를 허용하고 실제 보호막 가상 표는 수정하지 않는다."""
        super().__init__(edition);self.b=BODY[edition]
        self.mu.mem_map(0,0x1000)
        self.exports.extend(ROOT/f'extracted/forcefieldpostpop/{edition}/{n}' for n in ('creation.c','functions.tsv'))
        with self.exports[-1].open(encoding='utf-8') as fp:
            # Ghidra가 내보낸 실제 불연속 몸체만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 구간 사이 임의 코드는 허용하지 않는다.
                for part in row['ranges'].split(';'):
                    lo,hi=(int(v,16) for v in part.split('-'));self.allowed.append((lo,hi+1))
        self.allowed.append((LAND,LAND+11))
        self.mu.mem_write(LAND,b'\xd9\x1d'+struct.pack('<I',RESULT)+b'\xe9'+struct.pack('<i',STOP-(LAND+11)))
        self.vtable={f'{off:02x}':f'{self.u32(self.b["table"]+off):08x}' for off in (0x10,0x14,0x18,0x20,0x30,0x48,0x5c,0x90)}
        if self.u32(self.b['table']+0x20)!=self.b['prefix'] or self.u32(self.b['table']+0x5c)!=self.b['handler'] or self.u32(self.b['table']+0x10)!=self.b['destroy']:
            raise RuntimeError('실제 보호막 가상 표 오류')
        self.returns=collections.Counter();self.substitutions=collections.Counter()

    def u32(self,address):
        """비정렬 원본 DWORD를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def on_instruction(self,mu,address,size,data):
        """원본 호출 인자만 기록한다. finder/상수/분기 기대값을 만들지 않는다."""
        b=self.b;esp=mu.reg_read(UC_X86_REG_ESP)
        if address not in (b['new'],b['regular'],b['base'],b['advance'],b['destroy']):
            OwnerOracle.on_instruction(self,mu,address,size,data);return
        self.substitutions[f'{address:08x}']+=1
        ret,first=struct.unpack('<II',mu.mem_read(esp,8));ecx=mu.reg_read(UC_X86_REG_ECX);result,purge=0,4
        if address==b['new']:
            if first!=40: raise RuntimeError('Regular 확보 크기 오류')
            self.events.append('A:40');result=MEMORY if self.case['first' if self.allocations==0 else 'second'] else 0
            self.allocations+=1;purge=0
        elif address==b['regular']:
            parent,payload=struct.unpack('<II',mu.mem_read(esp+8,8))
            if ecx!=MEMORY or parent!=TARGET: raise RuntimeError('Regular 생성 this/부모 오류')
            self.events.append(f'R:{parent}:{first}:{payload}');result=MEMORY;purge=12
        else:
            if ecx!=self.slot(TARGET): raise RuntimeError('효과 this 오류')
            if address==b['advance']:
                flag=self.u32(esp+8);self.events.append(f'A:{TARGET}:{first}:{flag}');purge=8
            elif address==b['destroy']: self.events.append(f'D:{TARGET}:{first}')
            else: self.events.append(f'B:{TARGET}:{first}')
        mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run_lifecycle(self,row,control):
        """정상 ret/보존 레지스터·스택·SEH·x87와 전체 raw/순서/반환 비트를 관찰한다."""
        c=self.case=dict(zip(KEYS,row));b=self.b
        self.setup((c['x'],c['y'],1,0,c['extra'],167,0,nodes(c)))
        raw=self.slot(TARGET);self.mu.mem_write(raw,struct.pack('<I',b['table']));self.mu.mem_write(raw+10,bytes([167]))
        self.mu.mem_write(b['priest'],struct.pack('<I',c['priestType']));self.mu.mem_write(b['debug'],bytes(4))
        self.mu.mem_write(0,struct.pack('<I',0x12345678));self.mu.mem_write(RESULT,bytes(4))
        self.events=[];self.allocations=0;self.write_ranges=[(0,4),(RESULT,RESULT+4)]
        saved=((UC_X86_REG_EBX,0x11223344),(UC_X86_REG_ESI,0x22334455),(UC_X86_REG_EDI,0x33445566),(UC_X86_REG_EBP,0x44556677))
        # 비영 callee 보존 레지스터를 확인한다.
        for reg,value in saved:self.mu.reg_write(reg,value)
        self.mu.reg_write(UC_X86_REG_EAX,0);self.mu.reg_write(UC_X86_REG_EDX,0);self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.reg_write(UC_X86_REG_ECX,raw);self.mu.reg_write(UC_X86_REG_ESP,STACK)
        prefix=c['kind']=='Prefix'
        self.mu.mem_write(STACK,struct.pack('<II',STOP,c['flags']) if prefix else struct.pack('<IIII',LAND,c['event'],c['count'],c['payload']))
        self.mu.emu_start(b['prefix'] if prefix else b['handler'],STOP,count=100000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+(8 if prefix else 16):raise RuntimeError('정상 반환/스택 오류')
        if any(self.mu.reg_read(reg)!=value for reg,value in saved) or self.u32(0)!=0x12345678:raise RuntimeError('보존 레지스터/SEH 오류')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('x87 제어/TOP 오류')
        self.returns[c['kind']]+=1
        return [';'.join(self.events) or '-', '-' if prefix else f'{self.u32(RESULT):08x}',bytes(self.mu.mem_read(raw,self.stride)).hex()]


def generate(smoke=False):
    """세 판본의 두 정밀도 관찰이 같은 입력만 fixture로 저장한다."""
    cases=inputs()[::101] if smoke else inputs();rows=[];editions={}
    paths={Path(__file__),FIXTURE,ROOT/'tools/ghidra/forcefieldpostpop-functions.json',ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_priestforcefield_oracle.py'}
    # 판본별 PE와 실제 finder/보호막 몸체는 독립으로 읽는다.
    for edition in SPECS:
        oracle=LifecycleOracle(edition)
        # Python에서 사건 반환/생성 여부를 계산하지 않고 원본의 관찰끼리 비교한다.
        for case in cases:
            first,second=(oracle.run_lifecycle(case,c) for c in CONTROLS)
            if first!=second:raise RuntimeError('두 x87 정밀도 관찰 불일치')
            rows.append([edition,*case,*first])
        editions[edition]=dict(cases=len(cases),returns=dict(oracle.returns),native_calls=dict(oracle.native_calls),substitutions=dict(oracle.substitutions),vtable=oracle.vtable,assertions=oracle.assertions,instructions=oracle.instructions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary'])
        print(f'{edition}: 보호막 후처리/Regular {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 보호막 postPop/Regular/좌표 타입 finder. 생성/base/진행/destroy는 명시 경계.\n'
        '# edition kind flags first second event count payload x y priestType layout extra events result raw\n'
        +'\n'.join('\t'.join(map(str,r)) for r in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-10',total=len(rows),controls=list(CONTROLS),editions=editions,os_calls=0,
        stubbed=['Regular new/생성','공통 postPop','프레임 진행','가상 destroy'],limitations=['합성 raw/타입/해시','오디오/GUI/일반 Pop 전체 원본 실행 제외'],
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA/정확한 입력/정상 반환/실제 몸체 진입과 assert/OS 0을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));expected=inputs()
    if report['schema']!=1 or report['controls']!=list(CONTROLS) or report['os_calls'] or set(report['editions'])!=set(SPECS):raise RuntimeError('감사 스키마/정밀도/판본/OS 오류')
    # PE/실행기/모든 내보내기/fixture의 바이트를 확인한다.
    for name,value in report['files'].items():
        if digest(ROOT/name)!=value:raise RuntimeError(f'SHA 불일치: {name}')
    rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    if len(rows)!=len(expected)*3 or report['total']!=len(rows) or any(len(r)!=16 for r in rows):raise RuntimeError('행/열 개수 오류')
    counts=collections.Counter(c[0] for c in expected)
    finder_calls=sum(c[0]=='Regular' and c[4]==2 for c in expected)*2
    # 판본마다 누락/중복 없이 전체 입력을 같은 순서로 실행해야 한다.
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition]
        if [(r[1],*map(int,r[2:13])) for r in selected]!=expected or item['cases']!=len(expected) or item['assertions']:raise RuntimeError(f'입력/assert 오류: {edition}')
        if item['native_calls'].get(f'{BODY[edition]["find"]:08x}')!=finder_calls:
            raise RuntimeError(f'실제 좌표 타입 finder 진입 오류: {edition}')
        # 두 정밀도 각각에서 모든 입력이 실제 접두/처리기에 진입하고 정상 반환해야 한다.
        for kind,key in (('Prefix','prefix'),('Regular','handler')):
            if item['returns'][kind]!=counts[kind]*2 or item['native_calls'].get(f'{BODY[edition][key]:08x}')!=counts[kind]*2:raise RuntimeError(f'진입/반환 오류: {edition}/{kind}')
    print(f'forcefieldpostpop 검증 통과: {len(rows)}개')


def main():
    """새 원본 관찰 생성/저장 감사/소수 입력 점검을 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
