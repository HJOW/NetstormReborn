#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 음악 채널 초기화/종료와 worker를 세 PE의 실제 명령으로 관찰한다.

커널/COM/메모리 파일/잠금/기록만 대체하며 스레드 함수도 직접 정상 반환까지 실행한다.
실제 게임/스레드/장치는 실행하지 않는다. C++ 기대 상태를 계산하지 않는다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_musicstream_oracle import StreamOracle,FAIL
from decomp_musiccontrol_oracle import OUTPUT,OUT_BYTES,FILE
from decomp_soundplay_oracle import ROOT,SPECS,CONTROLS,STUBS,digest
from decomp_owner_oracle import pefile

# 실제 초기화/종료/worker와 스레드/이벤트/번호 전역이다.
BODY={'originals':dict(init=0x4aadd0,shutdown=0x4aaf00,worker=0x4aad90,thread=0x5c7b50,event=0x5c7b54,id=0x5c7b58),
      'originalCD':dict(init=0x439000,shutdown=0x439130,worker=0x438fc0,thread=0x51a758,event=0x51a75c,id=0x51a760)}
BODY['original1037']=dict(BODY['originalCD'])
# 커널 함수의 stdcall 인자 수와 독립 입력 핸들이다.
IMPORTS=dict(InitializeCriticalSection=1,DeleteCriticalSection=1,CreateEventA=4,CreateThread=6,WaitForSingleObject=2,SetEvent=1,TerminateThread=2,CloseHandle=1)
EVENT,THREAD=0x18001000,0x18002000
FIXTURE=ROOT/'cpppj/tests/fixtures/musicruntime-x86.tsv'
REPORT=ROOT/'cpppj/recovery-musicruntime-evidence.json'


def inputs():
    """초기화/종료 실패·대기 결과와 worker 준비/활성·갱신 실패를 교차한다."""
    rows=[]
    def add(kind,ready=0,sound=1,flags=3,exists=1,event_seed=1,thread_seed=1,event_result=1,thread_result=1,signal=1,wait='0',status=0,failed=0):
        """입력만 고정 순서로 기록한다. TIMEOUT/실패의 상태 전이는 원본이 수행한다."""
        rows.append(','.join(map(str,(kind,ready,sound,flags,exists,event_seed,thread_seed,event_result,thread_result,signal,wait,status,failed))))
    # 이미 준비된 경우/소리 없음과 이벤트/스레드 실패에서 raw192 보존 범위를 확인한다.
    for ready,sound,flags,exists,event,thread in itertools.product((0,1),(0,1),(0,3),(0,1),(0,1),(0,1)):
        add('I',ready=ready,sound=sound,flags=flags,exists=exists,event_result=event,thread_result=thread)
    waits=('0','258|0','258|258|0','258|258|258|0','258|258|258|258','4294967295|4294967295')
    # 두 번 중단 후 성공한 경우에도 마지막 경고를 기록하는 원본 분기를 포함한다.
    for ready,flags,exists,event,thread,signal,wait in itertools.product((0,1),(0,3,0xa5000003),(0,1),(0,1),(0,1),(0,1),waits):
        add('D',ready=ready,flags=flags,exists=exists,event_seed=event,thread_seed=thread,signal=signal,wait=wait)
    # worker는 TIMEOUT일 때만 다시 갱신한다. 이벤트 없음/준비 전/비활성 및 COM 실패 후 정지도 관찰한다.
    for ready,flags,event,rounds,last,status,failed in itertools.product((0,1),(0,3),(0,1),(0,1,3),(0,128,0xffffffff),(0,1,2),(0,1,3,7)):
        add('W',ready=ready,flags=flags,event_seed=event,wait='|'.join(map(str,[258]*rounds+[last])),status=status,failed=failed)
    # 같은 수명을 두 번 초기화/종료해 no-op과 핸들 재정리 규칙도 실제 원본에서 확인한다.
    for sound,event,thread,signal,wait in itertools.product((0,1),(0,1),(0,1),(0,1),waits):
        add('L',sound=sound,event_seed=0,thread_seed=0,event_result=event,thread_result=thread,signal=signal,wait=wait)
    return rows


class RuntimeOracle(StreamOracle):
    """실제 음악 정책/갱신 몸체와 커널 응답만 분리한다. 강제 중단은 명시 경계로만 관찰한다."""
    def __init__(self,edition):
        """현재 PC 내보내기와 여덟 커널 IAT 경계를 등록한다."""
        super().__init__(edition);self.r=BODY[edition];self.runtime_returns=collections.Counter()
        added=[ROOT/f'extracted/musicruntime/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with added[1].open(encoding='utf-8') as fp:
            # 실제 함수의 불연속 범위만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 몸체 사이의 임의 코드는 실행하지 않는다.
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.exports+=added;self.kernel={};pe=pefile.PE(str(ROOT/self.spec['binary']))
        # 정확한 이름의 커널 import만 공급한 응답으로 대체한다.
        for directory in pe.DIRECTORY_ENTRY_IMPORT:
            # IAT 슬롯을 스택 정리가 명시된 독립 대체 주소로 바꾼다.
            for item in directory.imports:
                name=item.name.decode() if item.name else ''
                if name in IMPORTS:
                    address=STUBS+0x600+len(self.kernel)*16;self.kernel[address]=name;self.mu.mem_write(item.address,struct.pack('<I',address))
        if set(self.kernel.values())!=set(IMPORTS):raise RuntimeError('커널 IAT 누락')

    def handle(self,value):
        """가짜 핸들을 사건의 일정한 E/T 이름으로 바꾼다."""
        return 'E' if value==EVENT else 'T' if value==THREAD else str(value)

    def on_instruction(self,mu,address,size,data):
        """커널 인자/순서를 관찰하고 그 외 정책/채널 명령은 제한 원본 실행기에 맡긴다."""
        if self.api.get(address)=='mmioClose':
            # 초기화로 잊힌 파일이 0/다른 토큰의 닫기 때문에 반납되었다고 관찰하지 않는다.
            token,flags=self.arguments(mu,2)
            if flags:raise RuntimeError('파일 닫기 flags 오류')
            self.substitutions['mmioClose']+=1;self.events.append(f'C:{token}')
            if token==FILE:self.opened=False
            return self.leave(mu,0,8)
        name=self.kernel.get(address)
        if not name:return super().on_instruction(mu,address,size,data)
        self.substitutions[name]+=1;args=self.arguments(mu,IMPORTS[name]);result=0
        if name in ('InitializeCriticalSection','DeleteCriticalSection'):
            index=(args[0]-self.m['channel']-0x44)//0x60
            if index not in (0,1) or args[0]!=self.m['channel']+index*0x60+0x44:raise RuntimeError('두 채널 잠금 주소 오류')
            self.events.append(('I:' if name.startswith('Initialize') else 'D:')+str(index))
        elif name=='CreateEventA':
            if args!=(0,1,0,0):raise RuntimeError('이벤트 인자 오류')
            result=EVENT if self.event_result else 0;self.events.append(f'A:{self.event_result}')
        elif name=='CreateThread':
            if args[:2]!=(0,0) or args[2]!=self.r['worker'] or self.u32(args[3]) or args[4]!=0 or args[5]!=self.r['id']:raise RuntimeError('스레드 인자 오류')
            mu.mem_write(args[5],struct.pack('<I',77));result=THREAD if self.thread_result else 0;self.events.append(f'T:77:{self.thread_result}')
        elif name=='WaitForSingleObject':
            if not self.waits:raise RuntimeError('대기 응답 소진')
            result=self.waits.pop(0);self.events.append(f'W:{self.handle(args[0])}:{args[1]}:{result}')
        elif name=='SetEvent':
            result=self.signal;self.events.append(f'Z:{self.handle(args[0])}:{result}')
        elif name=='TerminateThread':
            if args!=(THREAD,0):raise RuntimeError('중단 인자 오류')
            self.events.append('J:T');result=1
        elif name=='CloseHandle':self.events.append('O:'+self.handle(args[0]));result=1
        self.leave(mu,result,IMPORTS[name]*4)

    def invoke_runtime(self,key,control):
        """실제 stdcall worker 또는 cdecl 정책을 정상 반환까지 직접 호출한다."""
        result=self.call(self.r[key],0,(0,) if key=='worker' else (),4 if key=='worker' else 0,control)
        self.runtime_returns[key]+=1
        if key=='worker' and result:raise RuntimeError('worker 반환 오류')
        return result

    def run_case(self,case,control):
        """두 채널/핸들 입력을 배치하고 실제 초기화/worker/종료 명령의 관찰만 반환한다."""
        kind,ready,sound,flags,exists,event_seed,thread_seed,event_result,thread_result,signal,wait,status,failed=case.split(',')
        self.failed=int(failed);self.split=-1;self.cursor=0;self.music_status=int(status);self.create_result=0;self.create_writes=1
        super().run_script(f'seed|g:musicinit:{ready}|g:initialized:{sound}|g:music:4294966519|f:8:{flags}|mb:{exists}',control)
        self.results['gain']=FAIL if self.failed==3 else 0;self.events=[]
        self.mu.mem_write(self.m['channel']+0x60,bytes((i*11+5)&255 for i in range(0x60)))
        self.write_ranges+=[(self.m['channel']+0x60,self.m['channel']+0xc0),(self.r['thread'],self.r['id']+4),(self.m['musicinit'],self.m['musicinit']+4)]
        self.mu.mem_write(self.r['thread'],struct.pack('<III',THREAD if int(thread_seed) else 0,EVENT if int(event_seed) else 0,91))
        self.event_result,self.thread_result,self.signal=map(int,(event_result,thread_result,signal));self.waits=list(map(int,wait.split('|')))
        result='-'
        if kind=='I':self.invoke_runtime('init',control)
        elif kind=='D':self.invoke_runtime('shutdown',control)
        elif kind=='W':result=self.invoke_runtime('worker',control)
        else:
            # 같은 수명의 재호출도 기대 상태 계산 없이 실행한다.
            for key in ('init','init','shutdown','shutdown'):self.invoke_runtime(key,control)
        if self.lock_depth:raise RuntimeError('채널 잠금 잔류')
        return [';'.join(self.events) or '-',bytes(self.mu.mem_read(self.m['channel'],0xc0)).hex(),bytes(self.mu.mem_read(OUTPUT,OUT_BYTES)).hex(),
            self.u32(self.m['musicinit']),self.u32(self.r['thread']),self.u32(self.r['event']),self.u32(self.r['id']),self.position,int(self.opened),result]


def generate(smoke=False):
    """두 x87 관찰을 비교해 저장하고 의존 SHA/직접 반환/실제 명령 진입을 기록한다."""
    rows,editions=[],{};cases=inputs();paths={Path(__file__),FIXTURE,ROOT/'tools/ghidra/musicruntime-functions.json'}
    # 기존 실행 도구는 수정하지 않고 SHA 의존성만 기록한다.
    for name in ('musicstream','musiccontrol','soundcontrol','soundplay','owner'):paths.add(ROOT/f'tools/decomp_{name}_oracle.py')
    if smoke:cases=cases[::173]+cases[-4:]
    # 세 PE는 별도 실행기에서 독립적으로 관찰한다.
    for edition in SPECS:
        oracle=RuntimeOracle(edition)
        # C++와 무관한 실제 원본 관찰 두 개만 비교한다.
        for case in cases:
            a,b=(oracle.run_case(case,control) for control in CONTROLS)
            if a!=b:raise RuntimeError('원본 정밀도 관찰 불일치 '+edition+'/'+case)
            rows.append([edition,case,*a])
        editions[edition]=dict(cases=len(cases),calls=oracle.calls,returns=dict(oracle.runtime_returns),native_calls=dict(oracle.native_calls),
            substitutions=dict(oracle.substitutions),assertions=oracle.assertions,instructions=oracle.instructions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 음악 Runtime {len(cases)}개 정상 반환',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 음악 초기화/종료/worker·Stop/갱신/Read 명령. 커널/COM/파일/잠금/기록만 대체.\n'
        '# edition case events raw192 output128 ready thread event id filePosition fileOpen workerResult\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-10',total=len(rows),controls=list(CONTROLS),os_calls=0,editions=editions,
        stubbed=['커널 이벤트/스레드 생성·대기·신호·강제 중단·닫기','COM/메모리 파일/채널 잠금','기록/assert 보고/패치 CRT 로캘','두 채널/핸들 초기 배치'],
        limitations=['실제 Windows 강제 중단을 실행하지 않음; 호스트 경계는 협조 종료/완전 합류로 바꿈','GUI/실제 장치 연결 미검증','잠금 초기화/삭제는 raw에 호스트 구조체를 쓰지 않는 관찰 경계','실제 동시성은 C++ worker 검사에서 별도로 확인'],
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA/정확한 입력·반환·정책/worker 진입·명시 assert·균형 잠금·OS 0을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));cases=inputs()
    if report['schema']!=1 or report['controls']!=list(CONTROLS) or report['os_calls'] or set(report['editions'])!=set(SPECS):raise RuntimeError('감사 스키마/판본 오류')
    # 모든 의존 파일의 생성 당시 바이트를 확인한다.
    for name,value in report['files'].items():
        if digest(ROOT/name)!=value:raise RuntimeError('SHA 불일치 '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows)!=report['total'] or len(rows)!=len(cases)*3 or any(len(r)!=12 for r in rows):raise RuntimeError('행/개수 오류')
    expected=collections.Counter();roots=0
    # 호출 종류별 실제 직접 반환 수를 입력에서 센다. 상태 결과는 계산하지 않는다.
    for case in cases:
        keys={'I':('init',),'D':('shutdown',),'W':('worker',),'L':('init','init','shutdown','shutdown')}[case[0]]
        expected.update(keys);roots+=len(keys)+2
    # 각 판본의 정확한 입력/정상 반환과 kernel/COM 대체 범위를 확인한다.
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if [row[1] for row in selected]!=cases or item['cases']!=len(cases) or item['assertions']:raise RuntimeError('입력/assert 오류')
        if item['returns']!={key:count*2 for key,count in expected.items()} or item['calls']!=roots*2:raise RuntimeError('root/반환 수 오류')
        for key,count in item['returns'].items():
            if item['native_calls'].get(f'{BODY[edition][key]:08x}',0)!=count:raise RuntimeError('정책/worker 실제 진입 누락')
        if item['substitutions'].get('assert',0)!=sum(sum(event.startswith('!') for event in row[2].split(';')) for row in selected)*2:raise RuntimeError('명시 실패 보고 수 오류')
        if item['substitutions'].get('EnterCriticalSection',0)!=item['substitutions'].get('LeaveCriticalSection',0):raise RuntimeError('잠금 불균형')
        if any(len(row[3])!=384 or len(row[4])!=256 or row[11] not in ('-','0') for row in selected):raise RuntimeError('raw/worker 출력 오류')
    print(f'musicruntime 감사 통과: {len(rows)}개')


def main():
    """전체 생성/축소 실행/저장 감사 중 하나를 수행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate(args.smoke)


if __name__=='__main__':main()
