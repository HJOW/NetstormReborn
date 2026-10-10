#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 음악 경로/실제 WAVE 헤더/길이/생성/공개 시작을 정상 반환까지 관찰한다.

Find/config fallback·COM·Winmm 파일/잠금/기록만 명시 대체한다. 게임/장치/스레드는 실행하지 않는다.
x87 53/64비트에서 duration이 다르면 두 관찰을 모두 저장한다. C++ 기대값을 계산하지 않는다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_musicstream_oracle import StreamOracle, FAIL
from decomp_soundplay_oracle import ROOT, SPECS, CONTROLS, STUBS, NAMES, named, signed, digest
from decomp_owner_oracle import pefile, UC_X86_REG_ESP
from decomp_sounddevice_oracle import chunk

# 공개 시작/실제 헤더와 외부 조회/옵션 경계 및 두 디렉터리 전역이다.
BODY={'originals':dict(play=0x4aace0,header=0x4a8b50,find=0x41ad10,fallback=0x435200,primary=0x5c7b40,secondary=0x5c7b3c),
      'originalCD':dict(play=0x4393b0,header=0x4375c0,find=0x4bb6d0,fallback=0x484aa0,primary=0x51a748,secondary=0x51a744)}
BODY['original1037']=dict(BODY['originalCD'])
# 헤더에 필요한 추가 Winmm stdcall 인자 개수와 열린 파일의 독립 토큰이다.
IMPORTS=dict(mmioOpenA=3,mmioDescend=4,mmioAscend=3,mmioRead=3,mmioSeek=3,mmioClose=2)
FILE=1
FIXTURE=ROOT/'cpppj/tests/fixtures/musicopen-x86.tsv'
REPORT=ROOT/'cpppj/recovery-musicopen-evidence.json'


def wave(channels=2,rate=22050,bits=16,length=53,size=18,junk=0):
    """실제 RIFF 입력만 만든다. 짧은 format은 bits=99를 넣어 원본 보정도 실행한다."""
    align=channels*bits//8
    fmt=struct.pack('<HHIIHHH',1,channels,rate,(rate*align)&0xffffffff,align,99 if size<18 else bits,0)[:size]
    body=b'WAVE'+(chunk(b'JUNK',b'X') if junk else b'')+chunk(b'fmt ',fmt)+chunk(b'data',bytes((i*7+11)&255 for i in range(length)))
    return b'RIFF'+struct.pack('<I',len(body))+body


def inputs():
    """null/빈/후행 구분자 경로·준비/장치/활성·헤더/COM/seek 실패·길이 계산을 교차한다."""
    cases=[]
    # 하나의 입력은 경로/이름/조회/파일/열기 실패/준비/장치/flags/기존 버퍼/생성 HRESULT/출력/loop/seek/상위 활성이다.
    def add(primary='music',secondary='backup',name='track.mus',find=1,file=None,openfail=0,ready=1,device=1,flags=2,exists=0,hr=0,writes=1,loop=1,seek=0,fallback=0):
        """null 문자열은 '-'로, 그 밖의 문자열/파일은 hex로 보존해 TSV 구분자를 피한다."""
        values=[named(s) if s is not None else '-' for s in (primary,secondary,name)]
        values += [str(find),(wave() if file is None else file).hex(),str(openfail),str(ready),str(device),str(flags),str(exists),str(hr),str(writes),str(loop),str(seek),str(fallback)]
        cases.append(','.join(values))
    # 실제 경로 문자열의 null/빈/이중 구분자와 demo 비교를 관찰한다.
    for p,s,name,find,fallback in itertools.product((None,'','music','music\\'),(None,'','backup'),('track.mus','DeMo.MuS'),range(3),(0,1)):
        add(primary=p,secondary=s,name=name,find=find,fallback=fallback)
    # 직접 계산 결과를 만들지 않고 다양한 유효 PCM의 원본 x87 저장값을 관찰한다.
    for channels,rate,bits,length,size,junk in itertools.product((1,2,3),(11025,22050,44101,0x80000003),(8,16,24),(1,17,53),(16,18),(0,1)):
        if rate>0x7fffffff and size<18:continue
        add(file=wave(channels,rate,bits,length,size,junk))
    # 선행 조건은 파일/COM 호출을 하지 않아야 한다. 높은 다른 비트는 활성 검사에 영향을 주지 않는다.
    for ready,device,flags,name in itertools.product((0,1),(0,1),(0,1,2,3,0xa5000002),(None,'track.mus')):
        add(ready=ready,device=device,flags=flags,name=name)
    # 원본 헤더 실패와 부분 format 출력/남은 토큰 및 상위 fallback 반환을 관찰한다.
    bodies=(b'AVI ',b'WAVE'+chunk(b'data',b'ab'),b'WAVE'+chunk(b'fmt ',struct.pack('<HHIIHH',1,1,22050,22050,1,8)),
        b'WAVE'+chunk(b'fmt ',struct.pack('<HHIIHHH',1,2,22050,88200,4,16,0))+chunk(b'data',b''))
    for body,exists,fallback in itertools.product(bodies,(0,1),(0,1)):
        add(file=b'RIFF'+struct.pack('<I',len(body))+body,exists=exists,fallback=fallback)
    for exists,hr,writes,loop,seek in itertools.product((0,1),(0,FAIL),(0,1),(0,1,8),(0,-1)):
        if not exists and hr==0 and not writes:continue  # null 버퍼 assert 계약 위반은 제외한다.
        add(exists=exists,hr=hr,writes=writes,loop=loop,seek=seek)
    for name,fallback in itertools.product(('track.mus','DEMO.MUS'),(0,1)):
        add(name=name,openfail=1,fallback=fallback)
    return cases


class OpenOracle(StreamOracle):
    """기존 실제 음악 생성/시작/되감기에 경로 경계와 메모리 RIFF만 공급한다."""
    def __init__(self,edition):
        """현재 PC의 추가 내보내기와 필요한 Winmm IAT를 등록한다."""
        super().__init__(edition);self.o=BODY[edition]
        added=[ROOT/f'extracted/musicopen/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with added[1].open(encoding='utf-8') as fp:
            # 명시 내보낸 실제 몸체 범위만 실행한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 불연속 범위 사이의 임의 명령은 허용하지 않는다.
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.exports+=added
        self.stubs[self.o['find']]=('musicfind',0);self.stubs[self.o['fallback']]=('musicfallback',0)
        self.fileapi={};pe=pefile.PE(str(ROOT/self.spec['binary']))
        # 정확한 여섯 IAT 경계만 대체한다.
        for directory in pe.DIRECTORY_ENTRY_IMPORT:
            # 이름으로 식별한 Winmm API만 허용한다.
            for item in directory.imports:
                name=item.name.decode() if item.name else ''
                if name in IMPORTS:
                    address=STUBS+0x600+len(self.fileapi)*16;self.fileapi[address]=name
                    self.mu.mem_write(item.address,struct.pack('<I',address))
        if set(self.fileapi.values())!=set(IMPORTS):raise RuntimeError('음악 파일 IAT 누락')
        self.play_returns=0;self.header_returns=0

    def text(self,address):
        """로그의 null %s는 실제 MS CRT의 '(null)' 출력 경계로 기록한다."""
        return '(null)' if not address else super().text(address)

    def device(self,mu,name,purge):
        """외부 파일 탐색과 상위 옵션 callback의 입력만 제공한다."""
        if name=='musicfind':
            query,flags=self.arguments(mu,2)
            if flags:raise RuntimeError('음악 조회 flags 오류')
            self.find_calls+=1;found=self.find_mode==self.find_calls
            self.events.append('F:'+named(self.text(query))+':'+(named('resolved.mus') if found else '-'))
            self.leave(mu,NAMES+0x600 if found else 0,0)
        elif name=='musicfallback':
            (pointer,)=self.arguments(mu,1);self.events.append('D:'+named(self.text(pointer)))
            flags=self.u32(self.m['channel']+8);mu.mem_write(self.m['channel']+8,struct.pack('<I',(flags&~1)|self.fallback_active))
            self.leave(mu,0,0)
        else:super().device(mu,name,purge)

    def mmio(self,mu,name):
        """Winmm의 독립 파일/청크 위치를 공급한다. format 보정/채널 전이는 실제 PE가 수행한다."""
        args=self.arguments(mu,IMPORTS[name]);result=0
        if name=='mmioOpenA':
            if args[2]!=0x10000:raise RuntimeError('음악 파일 열기 flags 오류')
            self.position=0;self.opened=not self.open_failure;result=FILE if self.opened else 0
        elif name=='mmioClose':
            if args[1]:raise RuntimeError('음악 닫기 flags 오류')
            if not self.header_return:self.events.append('C:'+str(args[0]))
            self.opened=False
        elif name=='mmioSeek':
            handle,offset,origin=args
            if origin:raise RuntimeError('음악 절대 seek 계약 오류')
            result=offset if self.opened and handle==FILE and self.seek_result!=-1 else -1
            if not self.header_return:self.events.append(f'S:{handle}:{offset}:{result}')
            if result!=-1:self.position=offset
        elif name=='mmioRead':
            handle,pointer,count=args;old=self.position;result=-1
            if self.opened and handle==FILE:
                sample=self.file[self.position:self.position+count]
                if sample:mu.mem_write(pointer,sample)
                result=len(sample);self.position+=result
            if not self.header_return:self.events.append(f'R:{handle}:{old}:{count}:{result}')
        elif name=='mmioDescend':
            handle,pointer,parent,flags=args;found=None
            if handle!=FILE or not self.opened:raise RuntimeError('음악 청크 핸들 오류')
            if flags==0x20:
                if self.file[:4]==b'RIFF' and self.file[8:12]==b'WAVE':found=(b'RIFF',len(self.file)-8,b'WAVE',8,12)
            elif flags==0x10:
                wanted=bytes(mu.mem_read(pointer,4));offset=max(self.position,12)
                # 청크 순서/홀수 패딩은 파일 API의 입력이며 원본 헤더를 대신 계산하지 않는다.
                while offset+8<=len(self.file):
                    count=struct.unpack_from('<I',self.file,offset+4)[0]
                    if self.file[offset:offset+4]==wanted:found=(wanted,count,b'\0'*4,offset+8,offset+8);break
                    offset+=8+count+(count%2)
            else:raise RuntimeError('음악 청크 flags 오류')
            if found:
                kind,count,form,offset,pos=found;mu.mem_write(pointer,kind+struct.pack('<I',count)+form+struct.pack('<II',offset,0));self.position=pos
            else:result=1
        elif name=='mmioAscend':
            _,pointer,_=args;_,count,_,offset,_=struct.unpack('<IIIII',mu.mem_read(pointer,20));self.position=offset+count+(count%2)
        self.leave(mu,result&0xffffffff,IMPORTS[name]*4)

    def on_instruction(self,mu,address,size,data):
        """헤더의 실제 진입/반환을 기록하고 파일 대체 외 명령/쓰기는 앞 단계 감사를 유지한다."""
        if address==self.header_return:
            self.header_return=0;self.header_returns+=1
        if address==self.o['header']:
            self.events.append('H:'+named(self.text(self.arguments(mu,1)[0])))
            self.header_return=self.u32(mu.reg_read(UC_X86_REG_ESP))
        name=self.fileapi.get(address)
        if name:self.substitutions[name]+=1;self.mmio(mu,name)
        else:super().on_instruction(mu,address,size,data)

    def run_case(self,case,control):
        """초기 입력을 공급하고 공개 Play의 원본 반환·전체 raw96·파일 상태를 관찰한다."""
        p,s,name,find,file,openfail,ready,device,flags,exists,hr,writes,loop,seek,fallback=case.split(',')
        self.header_return=0;self.failed=0;self.split=-1;self.cursor=0;self.music_status=0
        self.create_result=int(hr);self.create_writes=int(writes)
        super().run_script(f'seed|f:8:{flags}|f:12:0|mb:{exists}|g:musicinit:{ready}|g:device:{device}|seek:{seek}',control)
        self.file=bytes.fromhex(file);self.opened=False;self.position=0;self.events=[]
        self.open_failure=int(openfail);self.find_mode=int(find);self.find_calls=0;self.fallback_active=int(fallback)
        # null 포인터와 빈 문자열 포인터를 구별해 전역/공개 인자를 배치한다.
        for offset,value,key in ((0x100,p,'primary'),(0x200,s,'secondary')):
            self.mu.mem_write(NAMES+offset,(bytes.fromhex(value) if value!='-' else b'')+b'\0')
            self.mu.mem_write(self.o[key],struct.pack('<I',0 if value=='-' else NAMES+offset))
        self.mu.mem_write(NAMES+0x400,(bytes.fromhex(name) if name!='-' else b'')+b'\0');self.mu.mem_write(NAMES+0x600,b'resolved.mus\0')
        result=self.call(self.o['play'],0,(0 if name=='-' else NAMES+0x400,int(loop)),0,control);self.play_returns+=1
        if self.lock_depth or self.header_return:raise RuntimeError('공개 반환에 잠금/헤더 잔류')
        return [';'.join(self.events) or '-',result,bytes(self.mu.mem_read(self.m['channel'],0x60)).hex(),self.position if self.opened else -1,int(self.opened)]


def generate(smoke=False):
    """두 정밀도 관찰을 각각 저장하고 원본/입력/정상 반환/명시 대체 SHA를 기록한다."""
    rows,editions=[],{}
    paths={Path(__file__),FIXTURE,ROOT/'tools/ghidra/musicopen-functions.json'}
    # 상속한 실행기의 SHA도 모두 기록한다. 기존 파일은 수정하지 않는다.
    for name in ('musicstream','musiccontrol','soundcontrol','soundplay','sounddevice','owner'):
        paths.add(ROOT/f'tools/decomp_{name}_oracle.py')
    cases=inputs()
    if smoke:cases=cases[::79]+cases[-4:]
    # 각 PE를 별도로 실행해 원본 기계어에서 기대 관찰을 얻는다.
    for edition in SPECS:
        oracle=OpenOracle(edition);precision=0;max_ulp=0
        # 정밀도 차이는 숨기지 않고 duration 저장 8바이트만 허용한다.
        for case in cases:
            a,b=(oracle.run_case(case,control) for control in CONTROLS)
            raw_a,raw_b=bytes.fromhex(a[2]),bytes.fromhex(b[2])
            if a[:2]!=b[:2] or a[3:]!=b[3:] or raw_a[:24]!=raw_b[:24] or raw_a[32:]!=raw_b[32:]:raise RuntimeError(f'길이 이외 정밀도 불일치: {edition}/{case}')
            ulp=abs(struct.unpack_from('<Q',raw_a,24)[0]-struct.unpack_from('<Q',raw_b,24)[0]);max_ulp=max(max_ulp,ulp)
            if ulp>2:raise RuntimeError('duration 정밀도 허용 범위 초과')
            precision+=int(ulp!=0);rows.append([edition,case,*a,b[2]])
        editions[edition]=dict(cases=len(cases),calls=oracle.calls,play_returns=oracle.play_returns,header_returns=oracle.header_returns,
            precision_differences=precision,max_duration_ulp=max_ulp,native_calls=dict(oracle.native_calls),substitutions=dict(oracle.substitutions),assertions=oracle.assertions,instructions=oracle.instructions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 음악 열기 {len(cases)}개 정상 반환, 정밀도 차이 {precision}개/{max_ulp} ULP',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 공개 음악 시작/경로/헤더/길이/버퍼 생성/되감기. find/config·COM·Winmm만 명시 대체.\n'
        '# edition case events result raw96_53 filePosition fileOpen raw96_64\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-10',total=len(rows),controls=list(CONTROLS),os_calls=0,
        editions=editions,stubbed=['파일 경로 조회','상위 음악 옵션 demo.mus 선택/활성 반환','COM 버퍼 생성/기존 경계','Winmm 파일/청크/잠금/기록/패치 CRT 로캘','초기 채널/파일/버퍼 배치'],
        limitations=['C++ 길이는 x87 53비트 관찰과 비교; 64비트 중간값의 저장 차이는 2 ULP 이내 별도 보존','상위 옵션 fallback 본체/두 채널/이벤트/스레드/실제 오디오는 후속','제로 분모/음수 길이/경로 overflow/추가 코덱 자료 제외'],
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """전체 입력/두 raw/원본 진입·반환/잠금/OS/정밀도/SHA를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));cases=inputs()
    if report['schema']!=1 or report['controls']!=list(CONTROLS) or report['os_calls'] or set(report['editions'])!=set(SPECS):raise RuntimeError('감사 스키마/판본 오류')
    # 모든 저장 근거가 생성 당시와 같은지 확인한다.
    for name,value in report['files'].items():
        if digest(ROOT/name)!=value:raise RuntimeError('SHA 불일치: '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows)!=report['total'] or len(rows)!=len(cases)*3 or any(len(row)!=8 for row in rows):raise RuntimeError('행/개수 오류')
    # 공개 호출은 모두 정상 반환하고 실제 진입해야 한다. 실제 헤더 반환/호출도 일치해야 한다.
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if [row[1] for row in selected]!=cases or item['cases']!=len(cases) or item['assertions']:raise RuntimeError('입력/assert 오류')
        if item['play_returns']!=len(cases)*2 or item['native_calls'].get(f"{BODY[edition]['play']:08x}",0)!=item['play_returns']:raise RuntimeError('공개 진입/반환 오류')
        headers=sum('H:' in row[2] for row in selected)*2
        if item['header_returns']!=headers or item['native_calls'].get(f"{BODY[edition]['header']:08x}",0)!=headers:raise RuntimeError('헤더 진입/반환 오류')
        if item['calls']!=len(cases)*6:raise RuntimeError('초기 Lookup/공개 root 호출 횟수 오류')
        if item['substitutions']['EnterCriticalSection']!=item['substitutions']['LeaveCriticalSection']:raise RuntimeError('잠금 불균형')
        precision=0;maximum=0
        # duration 이외 바이트가 같은지와 기록된 정밀도 차이를 감사한다.
        for row in selected:
            a,b=bytes.fromhex(row[4]),bytes.fromhex(row[7])
            if len(a)!=96 or len(b)!=96 or a[:24]!=b[:24] or a[32:]!=b[32:]:raise RuntimeError('raw 범위/정밀도 오류')
            ulp=abs(struct.unpack_from('<Q',a,24)[0]-struct.unpack_from('<Q',b,24)[0]);precision+=int(ulp!=0);maximum=max(maximum,ulp)
        if maximum>2 or precision!=item['precision_differences'] or maximum!=item['max_duration_ulp']:raise RuntimeError('정밀도 감사 오류')
    print(f'musicopen 감사 통과: {len(rows)}개')


def main():
    """축소 원본 실행/전체 생성/저장 감사 중 하나를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate(args.smoke)


if __name__=='__main__':main()
