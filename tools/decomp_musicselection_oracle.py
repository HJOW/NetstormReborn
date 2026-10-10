#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""상위 음악 옵션/현재 곡 전환·실제 존재 검사/strncpy·demo 재선택을 세 PE에서 관찰한다.

상위 fallback 대체를 제거해 공개 Play/Stop·실제 헤더/생성/되감기까지 실행한다.
파일 조회/COM/Winmm/잠금/기록만 명시 대체하며 게임/장치/스레드는 실행하지 않는다.
"""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_musicopen_oracle import OpenOracle, IMPORTS, wave
from decomp_soundplay_oracle import ROOT, SPECS, CONTROLS, NAMES, named, digest

# 상위 옵션 래퍼/선택과 현재 이름/음악 옵션 전역이다.
BODY={'originals':dict(wrapper=0x435200,select=0x435160,current=0x54dd60,option=0x531880),
      'originalCD':dict(wrapper=0x484aa0,select=0x484a10,current=0x52eb38,option=0x52e828)}
BODY['original1037']=dict(BODY['originalCD'])
# 새 저장 fixture와 근거 기록이다. 기존 자료는 수정하지 않는다.
FIXTURE=ROOT/'cpppj/tests/fixtures/musicselection-x86.tsv'
REPORT=ROOT/'cpppj/recovery-musicselection-evidence.json'


def inputs():
    """동일/다른/빈 이름·옵션/준비/활성·경로/파일 실패·특수 곡을 교차한다."""
    rows=[]
    def add(old='old.mus',name='track.mus',global_option=1,option=1,ready=1,device=1,flags=3,exists=1,find=1,file=0,hr=0,seek=0,primary='music',secondary='backup',wrapper=0):
        """문자열은 hex, null 디렉터리는 '-'로 보존한다. 기대값은 만들지 않는다."""
        values=[named(old),named(name),str(global_option),str(option),str(ready),str(device),str(flags),str(exists),str(find),str(file),str(hr),str(seek),
            named(primary) if primary is not None else '-',named(secondary) if secondary is not None else '-',str(wrapper)]
        rows.append(','.join(values))
    # 준비/장치/활성과 동일 이름 비교·특수 곡 loop·옵션은 모두 실제 선택 몸체에서 처리한다.
    for old,name,option,ready,device,flags in itertools.product(('','old.mus','TRACK.MUS'),('track.mus','FANFARE.MUS','defeat.mus','missing.mus',''),(0,1),(0,1),(0,1),(0,3)):
        add(old=old,name=name,option=option,ready=ready,device=device,flags=flags)
    # fallback은 전역 옵션을 다시 읽고 demo가 또 실패해도 재귀를 끝내야 한다.
    for name,global_option,option,find,file,flags in itertools.product(('track.mus','DeMo.MuS'),(0,1),(0,1,0xffffffff),(0,1,2,3),(0,1,2,3),(0,3)):
        add(name=name,global_option=global_option,option=option,find=find,file=file,flags=flags)
    # 존재 검사도 원본의 null/빈/후행 경로와 이름 재붙이기를 사용한다.
    for p,s,find in itertools.product((None,'','music\\'),(None,'','backup'),(0,1,2,3)):
        add(primary=p,secondary=s,find=find)
    # 래퍼의 전역 옵션과 버퍼 생성/되감기 실패는 곡 이름 저장을 되돌리지 않는다.
    for option,exists,hr,seek in itertools.product((0,1,0xffffffff),(0,1),(0,0x80004005),(0,-1)):
        add(global_option=option,exists=exists,hr=hr,seek=seek,wrapper=1)
    # 255바이트 strncpy의 절단/패딩/마지막 바이트 보존을 재생 준비 전 상태에서 관찰한다.
    for length in (1,254,255,260):add(old='',name='x'*length,ready=0)
    return rows


class SelectionOracle(OpenOracle):
    """상위 선택도 실제 명령으로 실행하며 Winmm에는 재사용하지 않는 파일 토큰을 공급한다."""
    def __init__(self,edition):
        """새 내보내기/쓰기를 허용하고 이전 상위 fallback stub을 제거한다."""
        super().__init__(edition);self.selection=BODY[edition]
        del self.stubs[self.o['fallback']]
        added=[ROOT/f'extracted/musicselection/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with added[1].open(encoding='utf-8') as fp:
            # 내보낸 불연속 실제 명령 범위만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 함수 사이 임의 명령은 실행하지 않는다.
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.exports+=added;self.selection_returns={'wrapper':0,'select':0}

    def device(self,mu,name,purge):
        """파일 조회의 응답만 공급한다. 선택/존재 검사와 fallback 본체는 대체하지 않는다."""
        if name!='musicfind':return super().device(mu,name,purge)
        query,flags=self.arguments(mu,2);text=self.text(query);lower=text.lower()
        if flags:raise RuntimeError('조회 flags 오류')
        found=self.find_mode!=0 and 'missing.mus' not in lower
        if self.find_mode==2:found=found and 'demo.mus' not in lower
        if self.find_mode==3:found=found and 'demo.mus' in lower
        mu.mem_write(NAMES+0x600,text.encode()+b'\0');self.events.append('F:'+named(text)+':'+(named(text) if found else '-'))
        self.leave(mu,NAMES+0x600 if found else 0,0)

    def mmio(self,mu,name):
        """독립 RIFF/파일 위치를 제공한다. 헤더·부분 출력·선택 상태는 원본 명령이 계산한다."""
        args=self.arguments(mu,IMPORTS[name]);result=0
        if name=='mmioOpenA':
            if args[2]!=0x10000:raise RuntimeError('열기 flags 오류')
            demo='demo.mus' in self.text(args[0]).lower();failure=self.file_mode==1 or (self.file_mode==3 and not demo)
            result=0 if failure else self.next_file
            if result:
                self.next_file+=1;data=wave()
                if self.file_mode==2:
                    # fmt만 존재하는 정상 RIFF 구조로 data 청크 누락 실패를 관찰한다.
                    data=wave()[:38];data=data[:4]+struct.pack('<I',len(data)-8)+data[8:]
                self.files[result]=[data,0]
        elif name=='mmioClose':
            if args[1]:raise RuntimeError('닫기 flags 오류')
            if not self.header_return:self.events.append('C:'+str(args[0]))
            self.files.pop(args[0],None)
        elif name=='mmioSeek':
            token,offset,origin=args
            if origin:raise RuntimeError('절대 seek 오류')
            result=offset if token in self.files and self.seek_result!=-1 else -1
            if result!=-1:self.files[token][1]=offset
            self.events.append(f'S:{token}:{offset}:{result}')
        elif name=='mmioRead':
            token,target,count=args;item=self.files.get(token);result=-1
            if item:
                sample=item[0][item[1]:item[1]+count]
                if sample:mu.mem_write(target,sample)
                item[1]+=len(sample);result=len(sample)
        elif name=='mmioDescend':
            token,pointer,parent,flags=args;item=self.files[token];file,position=item;found=None
            if flags==0x20:
                if file[:4]==b'RIFF' and file[8:12]==b'WAVE':found=(b'RIFF',len(file)-8,b'WAVE',8,12)
            elif flags==0x10:
                wanted=bytes(mu.mem_read(pointer,4));offset=max(position,12)
                # Winmm 청크 순서/홀수 패딩만 독립 경계에서 공급한다.
                while offset+8<=len(file):
                    count=struct.unpack_from('<I',file,offset+4)[0]
                    if file[offset:offset+4]==wanted:found=(wanted,count,b'\0'*4,offset+8,offset+8);break
                    offset+=8+count+(count%2)
            else:raise RuntimeError('청크 flags 오류')
            if found:
                kind,count,form,offset,pos=found;mu.mem_write(pointer,kind+struct.pack('<I',count)+form+struct.pack('<II',offset,0));item[1]=pos
            else:result=1
        elif name=='mmioAscend':
            token,pointer,_=args;_,count,_,offset,_=struct.unpack('<IIIII',mu.mem_read(pointer,20));self.files[token][1]=offset+count+(count%2)
        self.leave(mu,result&0xffffffff,IMPORTS[name]*4)

    def run_case(self,case,control):
        """전역/채널 입력을 배치하고 실제 상위 선택을 한 번 정상 반환까지 실행한다."""
        old,name,global_option,option,ready,device,flags,exists,find,file,hr,seek,p,s,wrapper=case.split(',')
        self.header_return=0;self.failed=0;self.split=-1;self.cursor=0;self.music_status=0;self.create_result=int(hr);self.create_writes=1
        self.run_script(f'seed|f:8:{flags}|f:12:1|mb:{exists}|g:musicinit:{ready}|g:device:{device}|seek:{seek}',control)
        self.files={1:[wave(),5]};self.next_file=2;self.find_mode=int(find);self.file_mode=int(file);self.events=[]
        current=self.selection['current'];self.write_ranges.append((current,current+256))
        initial=bytearray([0xae]*256);encoded=bytes.fromhex(old);initial[:len(encoded)]=encoded;initial[len(encoded)]=0;initial[255]=0x7d
        self.mu.mem_write(current,bytes(initial));self.mu.mem_write(self.selection['option'],struct.pack('<I',int(global_option)))
        # 공개 인자의 이름과 null/빈 디렉터리를 원본 포인터 위치에 배치한다.
        for offset,value,key in ((0x100,p,'primary'),(0x200,s,'secondary')):
            self.mu.mem_write(NAMES+offset,(bytes.fromhex(value) if value!='-' else b'')+b'\0')
            self.mu.mem_write(self.o[key],struct.pack('<I',0 if value=='-' else NAMES+offset))
        self.mu.mem_write(NAMES+0x400,bytes.fromhex(name)+b'\0')
        key='wrapper' if int(wrapper) else 'select';args=(NAMES+0x400,) if int(wrapper) else (NAMES+0x400,int(option))
        self.call(self.selection[key],0,args,0,control);self.selection_returns[key]+=1
        if self.lock_depth or self.header_return:raise RuntimeError('선택 반환에 잠금/헤더 잔류')
        token=self.u32(self.m['channel']+12);item=self.files.get(token)
        return [';'.join(self.events) or '-',bytes(self.mu.mem_read(self.m['channel'],96)).hex(),bytes(self.mu.mem_read(current,256)).hex(),item[1] if item else -1,len(self.files)]


def generate(smoke=False):
    """두 제어값에서 같은 원본 관찰을 저장하고 모든 의존 SHA/진입/반환을 기록한다."""
    rows,editions=[],{};cases=inputs();paths={Path(__file__),FIXTURE,ROOT/'tools/ghidra/musicselection-functions.json'}
    # 상속한 실제 실행기 전부의 SHA를 기록한다.
    for name in ('musicopen','musicstream','musiccontrol','soundcontrol','soundplay','sounddevice','owner'):paths.add(ROOT/f'tools/decomp_{name}_oracle.py')
    if smoke:cases=cases[::137]+cases[-4:]
    # 각 판본/정밀도의 관찰을 원본끼리 대조한다.
    for edition in SPECS:
        oracle=SelectionOracle(edition)
        # 기대값은 C++와 무관하게 실제 PE에서 얻는다.
        for case in cases:
            a,b=(oracle.run_case(case,control) for control in CONTROLS)
            if a!=b:raise RuntimeError('원본 정밀도 관찰 불일치: '+edition+'/'+case)
            rows.append([edition,case,*a])
        editions[edition]=dict(cases=len(cases),calls=oracle.calls,returns=oracle.selection_returns,header_returns=oracle.header_returns,native_calls=dict(oracle.native_calls),
            substitutions=dict(oracle.substitutions),assertions=oracle.assertions,instructions=oracle.instructions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 음악 선택 {len(cases)}개 정상 반환',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 상위 음악 선택/존재/strncpy/Play/Stop/헤더/기본 곡 재선택. config fallback 대체 없음.\n'
        '# edition case events raw96 current256 filePosition openFiles\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-10',total=len(rows),controls=list(CONTROLS),os_calls=0,editions=editions,
        stubbed=['파일 조회','COM 생성/정지/참조 해제','Winmm 파일/청크','잠금/기록/패치 CRT 로캘','초기 채널/파일/현재 이름 배치'],
        limitations=['작업 스레드/실제 오디오/GUI 옵션 연결은 후속','긴 이름 절단은 음악 준비 전 상태에서 관찰; 경로 overflow/null 이름/종료 없는 현재 이름 제외','음악 형식은 22050 Hz/stereo16의 유효 RIFF 또는 명시 헤더 실패'],
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """전체 입력/행·원본 진입/반환·잠금/OS/SHA와 상위 fallback 대체 제거를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));cases=inputs()
    if report['schema']!=1 or report['controls']!=list(CONTROLS) or report['os_calls'] or set(report['editions'])!=set(SPECS):raise RuntimeError('감사 스키마/판본 오류')
    # 각 근거의 생성 당시 바이트를 검사한다.
    for name,value in report['files'].items():
        if digest(ROOT/name)!=value:raise RuntimeError('SHA 불일치: '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows)!=report['total'] or len(rows)!=len(cases)*3 or any(len(r)!=7 for r in rows):raise RuntimeError('행/개수 오류')
    # 직접 호출 반환/실제 헤더 반환·callback 명령 진입과 대체 제거를 검사한다.
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];wrappers=sum(case.endswith(',1') for case in cases)
        if [row[1] for row in selected]!=cases or item['cases']!=len(cases) or item['assertions']:raise RuntimeError('입력/assert 오류')
        if item['returns']!=dict(wrapper=wrappers*2,select=(len(cases)-wrappers)*2) or item['calls']!=len(cases)*6:raise RuntimeError('반환/root 횟수 오류')
        for key,count in item['returns'].items():
            if item['native_calls'].get(f"{BODY[edition][key]:08x}",0)<count:raise RuntimeError('실제 선택 진입 누락')
        if item['header_returns']!=sum(row[2].count('H:') for row in selected)*2:raise RuntimeError('헤더 반환 오류')
        if item['substitutions'].get('musicfallback',0):raise RuntimeError('상위 fallback을 대체했습니다')
        if item['substitutions']['EnterCriticalSection']!=item['substitutions']['LeaveCriticalSection']:raise RuntimeError('잠금 불균형')
        if any(len(row[3])!=192 or len(row[4])!=512 or not row[4].endswith('7d') for row in selected):raise RuntimeError('raw/이름 보존 오류')
    print(f'musicselection 감사 통과: {len(rows)}개')


def main():
    """전체 생성/축소 원본 실행/저장 감사 중 하나를 수행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate(args.smoke)


if __name__=='__main__':main()
