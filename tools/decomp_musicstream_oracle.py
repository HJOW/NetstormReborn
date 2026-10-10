#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 세 PE의 음악 링 버퍼 갱신과 10.78 버퍼 생성을 정상 반환까지 관찰한다.

기존 음악 읽기/정지/helper도 실제 명령이다. COM/메모리 파일/잠금/기록만 명시 대체한다.
CD 생성은 공개 시작에 인라인되어 정적 대조만 하고 생성 독립 입력은 10.78에만 둔다.
게임/장치/스레드/OS를 실행하지 않는다. --verify는 SHA/정확한 입력/진입/반환/잠금을 감사한다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_musiccontrol_oracle import MusicOracle, OUTPUT, OUT_BYTES
from decomp_soundplay_oracle import ROOT, SPECS, CONTROLS, STUBS, BUFFERS, DEVICE, DEVICE_TABLE, BUFFER_TABLE, signed, digest

# 실제 갱신/생성 진입 주소다. CD 갱신은 초기화 검사까지 포함한 공개 void 함수다.
BODY={'originals':dict(fill=0x4aaad0,create=0x4aa040), 'originalCD':dict(fill=0x439a20)}
BODY['original1037']=dict(BODY['originalCD'])
# 새 COM 경계의 인자 바이트다. 기존 volume/stop/release는 앞 단계 대체를 재사용한다.
METHODS=(('musicstatus',0x24,8),('restore',0x50,4),('cursor',0x10,12),('lock',0x2c,32),('unlock',0x4c,20),('musicplay',0x30,16))
FAIL=0x80004005
FIXTURE=ROOT/'cpppj/tests/fixtures/musicstream-x86.tsv'
REPORT=ROOT/'cpppj/recovery-musicstream-evidence.json'


def inputs(edition):
    """활성/준비/상태·커서 경계, 각 HRESULT 실패와 분할/EOF/부분 읽기를 교차한다."""
    cases=[]
    # 갱신 인자는 flags, 초기화, status, 재생 커서, 내부 쓰기 위치, 파일 길이/위치, read 제한, seek 실패, COM 실패 종류, 첫 구간 크기다.
    for flags,ready,status,cursor,write in itertools.product((0,2,3,0xa5000003),(0,1),range(8),(0,3,15),(0,3,15)):
        cases.append('F:'+','.join(map(str,(flags,ready,status,cursor,write,64,0,-2,0,0,-1))))
    # 실패 번호 1~7은 status/restore/volume/cursor/lock/unlock/play다. 호출하지 않는 상태의 실패 설정도 포함한다.
    for failed,status,flags,split in itertools.product(range(1,8),(0,1,2,3,4,5),(1,3),(-1,0,5)):
        cases.append('F:'+','.join(map(str,(flags,1,status,3,12,64,0,-2,0,failed,split))))
    # 두 잠금 구간 중 실패하거나 정확히 EOF를 만나는 읽기를 실제 원본 Read로 실행한다.
    for length,flags,status,cap,seek,split in itertools.product((0,1,8,16,64),(1,3),(0,1),(-2,-1,2),(0,-1),(-1,0,5)):
        # 파일 안/끝의 중복 없는 시작 위치다.
        for offset in sorted({0,length//2,length}):
            cases.append('F:'+','.join(map(str,(flags,1,status,3,12,length,offset,cap,seek,0,split))))
    if edition=='originals':
        # 생성의 출력 토큰은 HRESULT와 별도로 쓰인다. 이미 버퍼가 있으면 COM을 호출하지 않는다.
        for exists,result,writes in itertools.product((0,1),(0,1,FAIL),(0,1)):
            cases.append('C:'+','.join(map(str,(exists,result,writes))))
    return cases


class StreamOracle(MusicOracle):
    """실제 음악 갱신에 COM 구간만 추가한다. 채움/실패/정지의 결과를 Python으로 계산하지 않는다."""
    def __init__(self,edition):
        """새 내보내기의 불연속 몸체와 COM 가상 표 대체를 등록한다."""
        super().__init__(edition)
        added=[ROOT/f'extracted/musicstream/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with added[1].open(encoding='utf-8') as fp:
            # 내보낸 실제 몸체만 실행 허용에 더한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 몸체 사이의 임의 코드는 계속 거부한다.
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.exports+=added
        # 음악 COM의 정확한 슬롯/스택 정리만 대체한다.
        for index,(name,slot,purge) in enumerate(METHODS):
            address=STUBS+0x120+index*16;self.stubs[address]=(name,purge)
            self.mu.mem_write(BUFFER_TABLE+slot,struct.pack('<I',address))
        self.stubs[STUBS+0x190]=('musiccreate',16)
        self.mu.mem_write(DEVICE_TABLE+0xc,struct.pack('<I',STUBS+0x190))
        self.returns=collections.Counter()

    def device(self,mu,name,purge):
        """COM이 제공하는 상태/커서/잠금 출력과 명시 HRESULT만 공급하고 사건을 기록한다."""
        if name not in {item[0] for item in METHODS} and name!='musiccreate':return super().device(mu,name,purge)
        args=self.arguments(mu,purge//4);buffer=args[0];result=0
        if name=='musiccreate':
            device,desc,target,outer=args
            size,flags,count,reserved,format_pointer=struct.unpack('<IIIII',mu.mem_read(desc,20))
            if device!=DEVICE or size!=20 or reserved or outer or format_pointer!=self.m['channel']+0x28:raise RuntimeError('생성 ABI/descriptor 오류')
            token=BUFFERS+16 if self.create_writes else 0
            mu.mem_write(target,struct.pack('<I',token));result=self.create_result
            self.events.append(f'N:{flags}:{count}:{bytes(mu.mem_read(format_pointer,18)).hex()}:{token}:{signed(result)}')
        else:
            if buffer!=BUFFERS+16:raise RuntimeError('음악 버퍼 토큰 오류')
            if name=='musicstatus':
                result=FAIL if self.failed==1 else 0;mu.mem_write(args[1],struct.pack('<I',self.music_status))
                self.events.append(f'T:#1:{self.music_status}:{signed(result)}')
            elif name=='restore':
                result=FAIL if self.failed==2 else 0;self.events.append(f'A:#1:{signed(result)}')
            elif name=='cursor':
                result=FAIL if self.failed==4 else 0
                mu.mem_write(args[1],struct.pack('<I',self.cursor));mu.mem_write(args[2],struct.pack('<I',13))
                self.events.append(f'Q:#1:{self.cursor}:13:{signed(result)}')
            elif name=='lock':
                _,offset,count,first,first_size,second,second_size,flags=args
                if count>32 or flags:raise RuntimeError('잠금 요청 범위/flags 오류')
                result=FAIL if self.failed==5 else 0
                a=count if self.split<0 else min(count,self.split);b=count-a
                if not result:
                    # 원본이 받은 두 독립 구간은 64바이트 경계로 분리해 출력 전체를 관찰한다.
                    for target,value in ((first,OUTPUT),(first_size,a),(second,OUTPUT+64),(second_size,b)):
                        mu.mem_write(target,struct.pack('<I',value))
                else:a=b=0
                self.events.append(f'K:#1:{offset}:{count}:{a}:{b}:{signed(result)}')
            elif name=='unlock':
                _,first,a,second,b=args
                if first!=OUTPUT or second!=OUTPUT+64 or a+b>32:raise RuntimeError('해제 구간 오류')
                result=FAIL if self.failed==6 else 0;self.events.append(f'U:#1:{a}:{b}:{signed(result)}')
            elif name=='musicplay':
                _,reserved,priority,flags=args
                if reserved or priority or flags!=1:raise RuntimeError('음악 Play ABI 오류')
                result=FAIL if self.failed==7 else 0;self.events.append(f'P:#1:{flags}:{signed(result)}')
        self.leave(mu,result,purge)

    def run_case(self,case,control):
        """고정 초기 입력 뒤 실제 생성 또는 갱신을 한 번 호출하고 전체 상태/출력/파일을 관찰한다."""
        kind,encoded=case.split(':');values=list(map(int,encoded.split(',')))
        self.failed,self.split,self.cursor,self.music_status=0,-1,0,0
        self.create_result,self.create_writes=0,1
        if kind=='F':
            flags,ready,status,cursor,write,length,offset,cap,seek,failed,split=values
            initial=f'seed|file:{length}:{offset}:0|f:4:16|f:8:{flags}|f:36:{write}|g:musicinit:{ready}|g:music:4294966519|cap:{cap}|seek:{seek}'
            self.music_status,self.cursor,self.failed,self.split=status,cursor,failed,split
        else:
            exists,self.create_result,self.create_writes=values;initial=f'seed|mb:{exists}'
        # 앞 단계의 입력 초기화/Lookup만 재사용한다. 음악 전이는 아래 실제 기계어 호출이 수행한다.
        super().run_script(initial,control)
        self.results['gain']=FAIL if self.failed==3 else 0
        self.put_global('device',DEVICE);self.events=[]
        key='fill' if kind=='F' else 'create'
        address=BODY[self.edition][key]
        result=self.call(address,self.m['channel'] if self.edition=='originals' else 0,(),0,control)
        self.returns[key]+=1
        if self.lock_depth:raise RuntimeError('반환 때 잠금 잔류')
        return [';'.join(self.events) or '-',str(result) if self.edition=='originals' else '-',
            bytes(self.mu.mem_read(self.m['channel'],0x60)).hex(),bytes(self.mu.mem_read(OUTPUT,OUT_BYTES)).hex(),self.position,int(self.opened)]


def generate(smoke=False):
    """두 x87 제어값에서 같은 원본 관찰만 저장하고 모든 의존 SHA와 반환/진입 횟수를 남긴다."""
    rows,editions=[],{}
    paths={Path(__file__),FIXTURE,ROOT/'tools/ghidra/musicstream-functions.json',ROOT/'tools/decomp_musiccontrol_oracle.py',
        ROOT/'tools/decomp_soundcontrol_oracle.py',ROOT/'tools/decomp_soundplay_oracle.py',ROOT/'tools/decomp_owner_oracle.py'}
    # 원본 PE별 관찰은 별도 실행기에 수집한다.
    for edition in SPECS:
        oracle=StreamOracle(edition);cases=inputs(edition)
        if smoke:cases=cases[::97]+cases[-3:]
        # 두 정밀도는 C++와 무관하게 실제 원본끼리 대조한다.
        for case in cases:
            first,second=(oracle.run_case(case,control) for control in CONTROLS)
            if first!=second:raise RuntimeError(f'원본 정밀도 관찰 불일치: {edition}/{case}')
            rows.append([edition,case,*first])
        editions[edition]=dict(cases=len(cases),calls=oracle.calls,returns=dict(oracle.returns),native_calls=dict(oracle.native_calls),
            substitutions=dict(oracle.substitutions),assertions=oracle.assertions,instructions=oracle.instructions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 음악 버퍼 {len(cases)}개 정상 반환',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 음악 버퍼 생성/채우기·원본 Read/Stop 관찰. COM/파일/잠금/기록만 명시 대체.\n'
        '# edition case events result raw96 output128 filePosition fileOpen\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-10',total=len(rows),controls=list(CONTROLS),os_calls=0,
        editions=editions,stubbed=['COM 생성/상태/복구/음량/커서/잠금/해제/재생/정지/참조 해제','메모리 파일 read/seek/close','잠금 관찰/기록/패치 CRT 로캘','초기 채널/파일/버퍼 배치'],
        limitations=['CD 생성은 공개 파일 열기 안에 인라인되어 정적 대조만 수행','파일 열기/작업 스레드/실제 장치/클라이언트 연결은 후속','손상된 null 버퍼/0 나눗셈/범위 밖 커서/잠금 구간 제외'],
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """저장 SHA·전체 입력/행/정상 반환/몸체 진입·균형 잠금·OS/예기치 않은 assert 0을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    if report['schema']!=1 or report['controls']!=list(CONTROLS) or report['os_calls'] or set(report['editions'])!=set(SPECS):raise RuntimeError('감사 스키마/판본 오류')
    # 원본·실행기·내보내기·관찰 바이트를 감사한다.
    for name,value in report['files'].items():
        if digest(ROOT/name)!=value:raise RuntimeError('SHA 불일치: '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows)!=report['total'] or len(rows)!=sum(len(inputs(e)) for e in SPECS) or any(len(r)!=8 for r in rows):raise RuntimeError('관찰 행/개수 오류')
    # 판본별 정확한 입력과 직접 반환/실제 진입을 확인한다.
    for edition,item in report['editions'].items():
        cases=inputs(edition);selected=[row for row in rows if row[0]==edition]
        if [row[1] for row in selected]!=cases or item['cases']!=len(cases) or item['assertions']:raise RuntimeError('입력/assert 오류')
        counts=collections.Counter('fill' if c.startswith('F:') else 'create' for c in cases)
        if item['returns']!={k:v*2 for k,v in counts.items()}:raise RuntimeError('정상 반환 횟수 오류')
        # 직접 호출 주소는 모두 실제 몸체에 진입해야 한다.
        for key,count in item['returns'].items():
            if item['native_calls'].get(f'{BODY[edition][key]:08x}',0)<count:raise RuntimeError('몸체 진입 누락')
        if item['substitutions']['EnterCriticalSection']!=item['substitutions']['LeaveCriticalSection']:raise RuntimeError('잠금 불균형')
    print(f'musicstream 감사 통과: {len(rows)}개')


def main():
    """생성/축소 관찰/저장 감사 중 하나를 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate(args.smoke)


if __name__=='__main__':main()
