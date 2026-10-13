#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""건설 끝 시각 재설정 전체·form getter·CD 계산/기본 시간 helper를 실제 PE로 대조한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_constructionprocess_oracle import ProcessOracle,OBJECT,SOUND,FORM
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,TYPES,POOL,STACK,STOP,digest
from decomp_damageablepredestroy_oracle import CONTROLS,bits

# 실제 재설정 루트와 Kernel 조회 경계다. 나머지 helper는 모두 실제 명령으로 실행한다.
SPEC={'originals':dict(entry=0x443dc0,kernel=0x471aa0,getter=0x4ae2d0),
      'originalCD':dict(entry=0x4d0930,kernel=0x4ed640,getter=0x4af4e0)}
SPEC['original1037']=dict(SPEC['originalCD'])
# 입력 열 순서와 새 관찰 경로다. 모든 시각은 double의 두 DWORD 비트다.
KEYS=('nowLow','nowHigh','startLow','startHigh','endLow','endHigh','maximum','damage','dirty','mutate','parent')
FIXTURE=ROOT/'cpppj/tests/fixtures/constructiontime-x86.tsv'
REPORT=ROOT/'cpppj/constructiontime.json'
# 실제 host 통합 검사와 같은 합성 form과 Kernel 번호다.
TIME_FORM,TIME_PID=5,7


def case(now=2.5,start=0.0,end=8.0,**changes):
    """기대 끝 시각을 계산하지 않고 현재 시각·프로세스·최대 HP 입력만 만든다."""
    value=dict(maximum=100,damage=5,dirty=0,mutate=0,parent=20000)
    # double 입력을 하위/상위 DWORD로 나눈다.
    for key,number in (('now',now),('start',start),('end',end)):
        value[key+'Low'],value[key+'High']=struct.unpack('<2I',struct.pack('<d',number))
    value.update(changes);return value


def inputs():
    """시각·최대 HP 부호·damage DWORD·변경 비트·경계 중 현재 자료 변동을 교차한다."""
    rows=[]
    # 인자와 기존 변경 비트에 관계없이 실제 루트가 쓰는 시각을 비교한다.
    for now,maximum,damage,dirty in itertools.product((0.0,2.5,7.75,-3.25,1000.25),(0,100,0xffffffff),
            (0,1,100,0x80000000,0xffffffff),(0,1,2,8,0xff)):
        rows.append(case(now=now,maximum=maximum,damage=damage,dirty=dirty))
    # CD의 진행 계산·0 나누기·음수 구간도 helper를 거친 실제 결과로 기록한다.
    for (start,end,now),damage in itertools.product(((0,8,4),(0,8,-2),(4,4,4),(4,4,6),(8,0,4),(2,10,4)),
            (0,1,100,0x80000000,0xffffffff)):
        rows.append(case(start=start,end=end,now=now,damage=damage,parent=60))
    # 최대 HP 경계가 현재 시각·시작/끝·소리/dirty를 바꾸면 다시 읽거나 그대로 보존한다.
    for mutate,damage,dirty in itertools.product((1,2,3),(0,5,0xffffffff),(0,8)):
        rows.append(case(mutate=mutate,damage=damage,dirty=dirty))
    return rows


class TimeOracle(ProcessOracle):
    """실제 재설정 루트·form/부모 getter·CD CRT/기본 시간을 실행하며 Kernel/최대 HP만 응답한다."""
    def __init__(self,edition):
        """공통 로더 뒤 새 허용 범위와 두 효과 경계를 연결한다."""
        super().__init__(edition);self.t=SPEC[edition];self.exports=[ROOT/f'extracted/constructiontime/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set();self.kernel_calls=collections.Counter();self.max_calls=0
        with self.exports[1].open(encoding='utf-8') as fp:
            # 함수별 실제 불연속 주소만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 포함 끝 주소를 반열린 범위로 바꾼다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def on_instruction(self,mu,address,size,data):
        """Kernel/최대 HP의 인자만 관찰하며 다른 함수 몸체는 대체하지 않는다."""
        if address==self.t['kernel']:
            index,=self.args(1)
            if index!=TIME_PID:raise RuntimeError('form의 현재 Kernel 번호 오류')
            self.kernel_calls[index]+=1;self.ret(OBJECT+0x28,0);return
        if address==self.r['maxHp']:
            parent=self.this_sid();self.events.append('H:'+str(parent));self.max_calls+=1
            # 경계 입력 쓰기는 native 쓰기 허용 범위에 포함하지 않는다.
            if self.case['mutate']==1:mu.mem_write(self.r['globals']['now'],struct.pack('<d',7.75))
            if self.case['mutate']==2:mu.mem_write(OBJECT+0x10,struct.pack('<2d',-1.5,4.0))
            if self.case['mutate']==3:
                mu.mem_write(OBJECT+8,bytes([0x81]));mu.mem_write(OBJECT+0x34,struct.pack('<I',0x12345678))
            self.ret(self.case['maximum'],0);return
        OwnerOracle.on_instruction(self,mu,address,size,data)

    def run_time(self,c,control):
        """입력 자료를 공급하고 정상 ABI·필드·부모/form 불변을 기록한다."""
        self.case=c;self.events=[];mu=self.mu;parent=self.slot(c['parent']);form=self.slot(TIME_FORM)
        raw=bytearray((i*29+7)&255 for i in range(self.stride));raw[10]=100;raw[11]=0
        struct.pack_into('<H',raw,6,TIME_FORM);mu.mem_write(parent,bytes(raw))
        formraw=bytearray(self.stride);formraw[10]=30;struct.pack_into('<I',formraw,18,TIME_PID);mu.mem_write(form,bytes(formraw))
        # 부착된 프로세스의 시작/끝·소리와 form/부모만 입력으로 준비한다.
        image=bytearray([0xa5])*0x38;image[8]=c['dirty']
        struct.pack_into('<4I',image,0x10,c['startLow'],c['startHigh'],c['endLow'],c['endHigh'])
        struct.pack_into('<3I',image,0x2c,TIME_FORM,c['parent'],SOUND);mu.mem_write(OBJECT,bytes(image))
        mu.mem_write(self.r['globals']['now'],struct.pack('<2I',c['nowLow'],c['nowHigh']));self.put('debug',0)
        self.write_ranges=[(OBJECT+8,OBJECT+9),(OBJECT+0x18,OBJECT+0x20)]
        self.prepare_registers(control);mu.mem_write(STACK,struct.pack('<3I',STOP,TIME_FORM,c['damage']))
        mu.emu_start(self.t['entry'],STOP,count=100000);self.check(control,STACK+4)
        address=f"{self.t['entry']:08x}";self.returns[address]+=1
        if bytes(mu.mem_read(parent,self.stride))!=bytes(raw) or bytes(mu.mem_read(form,self.stride))!=bytes(formraw):raise RuntimeError('재설정이 부모/form을 변경함')
        fields=':'.join(map(str,(mu.mem_read(OBJECT+8,1)[0],*struct.unpack('<4I',mu.mem_read(OBJECT+0x10,16)),struct.unpack('<I',mu.mem_read(OBJECT+0x34,4))[0])))
        return [';'.join(self.events),fields,bytes(raw).hex()]


def generate(smoke=False):
    """세 PE의 두 x87 정밀도 관찰이 같을 때만 fixture와 근거를 저장한다."""
    rows=[];editions={};paths={Path(__file__),FIXTURE,ROOT/'tools/ghidra/constructiontime-functions.json',
        *[ROOT/f'tools/decomp_{name}_oracle.py' for name in ('owner','priestgeometry','returnpiece','damageablepredestroy','constructionprocess')]}
    # 실제 PE마다 별도 로더와 메모리로 실행한다.
    for edition in SPECS:
        oracle=TimeOracle(edition);plan=inputs()[::29] if smoke else inputs()
        # 기대 시각을 계산하지 않고 원본 출력 둘을 비교한다.
        for c in plan:
            observed=[oracle.run_time(c,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError('끝 시각 x87 관찰 차이: '+edition)
            rows.append([edition,','.join(str(c[key]) for key in KEYS),*observed[0]])
        editions[edition]=dict(cases=len(plan),returns=dict(oracle.returns),native_calls=dict(oracle.native_calls),
            kernel_calls=dict(oracle.kernel_calls),max_calls=oracle.max_calls,assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary'])
        # 공통 로더가 읽는 앞 내보내기 자료도 SHA 근거에 포함한다.
        for group in ('owner','priestgeometry','returnpiece','constructionprocess'):
            paths.update(ROOT/f'extracted/{group}/{edition}/{name}' for name in ('creation.c','functions.tsv'))
        print(f'{edition}: constructiontime {len(plan)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 건설 끝 시각 재설정 전체/Getter/CD 계산·기본 시간. Kernel 조회/최대 HP는 명시 경계.\n'
        '# 판본 입력('+' '.join(KEYS)+') 사건 필드 부모raw\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-11',controls=list(CONTROLS),os_calls=0,editions=editions,
        files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},limits=[
            '끝 시각 재설정 전체·form/부모 getter·CD 진행/뺄셈/CRT/기본 시간 실제 명령',
            'Kernel 번호 7 조회와 최대 HP는 명시 경계, null 프로세스/assert/게임/OS 미실행',
            '합성 유한 시각·부모 60/20000·form 5·최대 HP 0/100/-1·damage DWORD·dirty/경계 변동',
            '정상 스택/보존 레지스터/x87·허용 실행/쓰기·부모/form 불변·SHA/진입/반환 감사']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'total: {len(rows)}',flush=True)


def verify():
    """기대 결과를 재계산하지 않고 SHA·입력·출력 폭·실제 진입/반환·경계 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 실행에 쓴 모든 자료가 생성 시점 그대로인지 확인한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError('SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('끝 시각 근거 합계 오류')
    # 실제 PE별 입력 순서와 스택/반환/경계 집계를 확인한다.
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition];plan=inputs();count=2*len(plan)
        if len(chosen)!=len(plan) or item['cases']!=len(plan) or item['assertions']:raise RuntimeError('끝 시각 입력/assert 오류')
        # 출력 폭만 검사하며 시각/dirty 기대값을 재생성하지 않는다.
        for row,c in zip(chosen,plan,strict=True):
            if len(row)!=5 or row[1]!=','.join(str(c[key]) for key in KEYS) or len(row[3].split(':'))!=6 or len(bytes.fromhex(row[4]))!=SPECS[edition]['stride']:raise RuntimeError('끝 시각 자료 폭/순서 오류')
        entry=f"{SPEC[edition]['entry']:08x}";getter=f"{SPEC[edition]['getter']:08x}"
        if item['returns'].get(entry)!=count or item['native_calls'].get(entry)!=count or item['native_calls'].get(getter)!=count:raise RuntimeError('끝 시각 실제 진입/반환 오류')
        if item['kernel_calls']!={str(TIME_PID):count} or item['max_calls']!=count:raise RuntimeError('끝 시각 효과 경계 수 오류')
        if item['native_calls'].get(f"{SPEC[edition]['kernel']:08x}",0):raise RuntimeError('Kernel 경계 몸체 실행')
    print(f'constructiontime 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 자료 감사를 선택한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)


if __name__=='__main__':main()
