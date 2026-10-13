#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""지정 free form 부착/Take와 단일 즉시 패킷 분배를 세 실제 PE의 전체 명령으로 대조한다."""
import argparse
import collections
import csv
import itertools
import json
import re
import struct
import pefile
from pathlib import Path
from decomp_process_oracle import ProcessOracle,ROOT,BINARIES,SPECS,CONTROLS,POOL,PROCS,ARENA,STUBS,STACK,CAPACITY,digest
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP

# 격리된 등록 표/연결/패킷/가상 표/명시 콜백 주소다. 호스트나 OS의 실행 주소가 아니다.
HANDLERS,OBSERVERS,VERSIONS,CONNECTION,PACKET,AUX,TAG_TABLE=(ARENA+x for x in (0x18000,0x19000,0x1a000,0x1b000,0x1c000,0x1d000,0x1e000))
RECEIVE,OBSERVER,ATTACHED=STUBS+0x300,STUBS+0x320,STUBS+0x340
# 실제 새 몸체/전역과 외부 수명/로그 경계의 주소다.
DATA={
 'originals':dict(take=0x4af610,register=0x4ded50,observe=0x4ded70,version=0x4ded90,validate=0x4df240,dispatch=0x4df740,
    tables=(0x5e4778,0x5e477c,0x5e4780),lengths=0x52ec40,last=137,printf=0x4e59a3,record=0x4dec30,
    terminate=0x4df9f0,format=0x4e0940,close=0x4e03d0,free=0x4e4354),
 'originalCD':dict(take=0x4ab440,register=0x418bb0,observe=0x418bd0,version=0x418bf0,validate=0x4190a0,dispatch=0x419130,
    tables=(0x512424,0x512428,0x51242c),lengths=0x50fd00,last=126,printf=0x4f23e0,record=0x418a30,
    terminate=0x419400,format=0x4a6c80,close=0x419880,free=0x4f1a50)}
DATA['original1037']=DATA['originalCD']
FIXTURE=ROOT/'cpppj/tests/fixtures/processform-x86.tsv'
REPORT=ROOT/'cpppj/processform.json'
# 복원 길이 표는 PE의 WORD 배열과 직접 대조하는 별도 자료다.
LENGTHS=ROOT/'cpppj/src/o/RawPacketLengths.h'
# 판본 독립인 입력 열이다. 형식/시각/소리의 기대 출력은 만들지 않는다.
ATTACH_KEYS=('form','state','head','flags','type')
DISPATCH_KEYS=('message','incoming','version','policy','present','required','declared','flags','depth','kill','sender')


def plan(edition):
    """원본의 정상 free form 경로와 모든 버전 정책/등록 유무/콜백 중 종료를 교차한다."""
    rows=[];first=SPECS[edition]['server_first']
    # free+dead 및 상위 상태 비트, 기존 머리의 form/자산 setter, 지역 flags를 비교한다.
    for form,state,head,flags,kind in itertools.product((first,first+50,20001,32767),(1,3,0x41,0x83),(0,1,2),(0x10,0x50),(30,46,61)):
        rows.append(('Attach',dict(zip(ATTACH_KEYS,(form,state,head,flags,kind)))))
    # 높은/낮은/같은 프로토콜과 high bit 제거·모든 종료/무시 정책을 비교한다.
    for incoming,version,policy in itertools.product((0,1,2,0x80,0x81,0x82,0x7f,0xff),(0,1,2,255),(0,1,2,4,8,15)):
        rows.append(('Dispatch',dict(message=55,incoming=incoming,version=version,policy=policy,present=1,required=65535,declared=17,flags=0,depth=0,kill=0,sender=0xfedcba98)))
    # 판본별 최대 타입, 처리기 없는 내부 타입, 고정 길이 및 닫힌 연결을 비교한다.
    for message,required,present,flags in itertools.product((0,1,6,9,10,55,126,137,138,255),(65535,4,17),(0,1),(0,0x40)):
        rows.append(('Dispatch',dict(message=message,incoming=1,version=1,policy=0,present=present,required=required,declared=4,flags=flags,depth=0,kill=0,sender=17)))
    # 콜백 전/뒤 중첩 DWORD와 종료 비트 변화, Observer 비호출을 확인한다.
    for depth,kill,message in itertools.product((0,5,0xffffffff),(0,1),(6,55)):
        rows.append(('Dispatch',dict(message=message,incoming=1,version=1,policy=0,present=1,required=65535,declared=17,flags=0,depth=depth,kill=kill,sender=0xffffffff)))
    return rows


class FormOracle(ProcessOracle):
    """기존 프로세스 로더와 실제 종속/Take 몸체에 새 패킷 등록/분배 범위만 더한다."""
    def __init__(self,edition):
        """PE는 읽기 전용으로 적재하며 응답 경계 주소를 실행 전에 확정한다."""
        super().__init__(edition);self.d=DATA[edition];self.boundaries=collections.Counter();self.returns=collections.Counter()
        self.extra=[ROOT/f'extracted/processform/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with self.extra[1].open(encoding='utf-8') as fp:
            # 함수 사이 영역은 허용하지 않고 Ghidra의 실제 불연속 범위만 추가한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entry_names[int(row['entry'],16)]=row['entry']
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.original_lengths=bytes(self.mu.mem_read(self.d['lengths'],512))

    def ret(self,value=0,purge=0):
        """명시 경계에서만 원본 호출 규약대로 반환한다."""
        from unicorn.x86_const import UC_X86_REG_EAX
        esp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,value)
        self.mu.reg_write(UC_X86_REG_EIP,self.u32(esp));self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge)

    def args(self,count):
        """현재 경계의 스택 DWORD 인자를 읽는다."""
        return struct.unpack('<'+'I'*count,self.mu.mem_read(self.mu.reg_read(UC_X86_REG_ESP)+4,count*4))

    def on_instruction(self,mu,address,size,data):
        """Kernel 응답/외부 수명/로그·가상 통지만 대체하고 새 몸체는 실제로 실행한다."""
        d=self.d
        if address==(0x471950 if self.patch else 0x4ed500):
            if self.args(1)!=(PROCS,):raise RuntimeError('지정 form Kernel 인자 오류')
            self.boundaries['kernel']+=1;self.ret(7);return
        if address==ATTACHED:
            if mu.reg_read(UC_X86_REG_ECX)!=PROCS:raise RuntimeError('지정 form 가상 통지 this 오류')
            self.events.append(f'N:{self.u32(PROCS+4)}:{self.u32(PROCS+8)}');self.boundaries['attached']+=1;self.ret();return
        if address==RECEIVE:
            sender,connection,packet=self.args(3)
            if (sender,connection,packet)!=(self.case['sender'],CONNECTION,PACKET):raise RuntimeError('패킷 처리기 인자 오류')
            self.events.append(f'H:{sender}:{self.u32(CONNECTION+0x8c)}');self.boundaries['handler']+=1
            if self.case['kill']:self.put(CONNECTION+0x34,self.u32(CONNECTION+0x34)|0x40)
            self.ret();return
        if address==OBSERVER:raise RuntimeError('단일 패킷에서 observer 호출')
        if address==d['record']:
            zero,peer,sender,packet,length=self.args(5)
            if (zero,peer,sender,packet,length)!=(0,123,self.case['sender'],PACKET,self.case['declared']):raise RuntimeError('패킷 기록 인자 오류')
            self.events.append(f'L:{sender}:{length}');self.boundaries['record']+=1;self.ret();return
        if address in (d['terminate'],d['close'],d['free']):
            if address==d['free']:
                if self.args(1)!=(CONNECTION,):raise RuntimeError('패킷 해제 인자 오류')
            elif mu.reg_read(UC_X86_REG_ECX)!=CONNECTION:raise RuntimeError('연결 수명 this 오류')
            event='T' if address==d['terminate'] else 'C' if address==d['close'] else 'F'
            self.events.append(event);self.boundaries[event]+=1;self.ret();return
        if address in (d['printf'],d['format']):
            self.boundaries['log']+=1;self.ret(AUX if address==d['format'] else 0);return
        ProcessOracle.on_instruction(self,mu,address,size,data)

    def root(self,address,args=(),this=0,purge=0):
        """루트를 정상 반환까지 실행하고 보존 레지스터/ABI/x87/SEH를 확인한다."""
        result=self.call(address,list(args),this=this,purge=purge)
        if any(self.mu.reg_read(reg) for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)):raise RuntimeError('보존 레지스터 오류')
        self.returns[f'{address:08x}']+=1;return result

    def run(self,operation,c,control):
        """입력 메모리만 만들고 실제 명령의 출력 바이트/콜백/반환을 관찰한다."""
        self.case=c;self.begin(dict(server=0,boss=1,now=0,rotation=0),control);s=self.s;d=self.d
        if operation=='Attach':
            form=POOL+c['form']*s['stride'];parent=POOL+20000*s['stride'];head=POOL+50*s['stride']
            # form과 부모/기존 머리의 주변 표식은 기대 출력을 계산하지 않는 입력이다.
            for ptr in (form,parent,head):self.mu.mem_write(ptr,bytes((i*29+7)&255 for i in range(s['stride'])))
            self.put(form+11,c['state'],1);self.put(parent+10,100,1);self.put(parent+11,0,1);self.put(parent+6,50 if c['head'] else 0,2)
            self.put(head,s['form_vtable'] if c['head']==1 else s['base_vtable']);self.put(head+10,46 if c['head']==1 else 100,1);self.put(head+11,8,1)
            table=bytearray(24);struct.pack_into('<I',table,20,ATTACHED);self.mu.mem_write(TAG_TABLE,bytes(table));self.put(PROCS,TAG_TABLE)
            before=[self.u32(a) for a in s['sid'][1:8]]
            self.root(s['base_ctor'],(c['type'],20000,c['form'],c['flags']),PROCS,16)
            after=[self.u32(a) for a in s['sid'][1:8]]
            if before!=after:raise RuntimeError('지정 form 부착에서 풀 목록/카운터 변동')
            return [';'.join(self.events) or '-',bytes(self.mu.mem_read(form,s['stride'])).hex(),bytes(self.mu.mem_read(parent,s['stride'])).hex(),
                bytes(self.mu.mem_read(head,s['stride'])).hex(),f'{self.u32(PROCS+4)}:{self.u32(PROCS+8)}',
                ':'.join(map(str,(self.u32(s['sid'][5]),self.u32(s['sid'][7]),struct.unpack('<H',self.mu.mem_read(POOL+4,2))[0],(self.u32(s['sid'][2])-POOL)//s['stride'],
                    struct.unpack('<H',self.mu.mem_read(POOL+18,2))[0],(self.u32(s['sid'][4])-POOL)//s['stride'])))]
        # 원본 길이 표를 복구한 뒤 이 입력의 등록/길이 값만 공급한다.
        self.mu.mem_write(d['lengths'],self.original_lengths)
        for ptr,table in zip(d['tables'],(HANDLERS,OBSERVERS,VERSIONS),strict=True):self.put(ptr,table)
        kind=c['message'];self.put(d['lengths']+kind*2,c['required'],2)
        self.root(d['register'],(kind,RECEIVE if c['present'] else 0));self.root(d['observe'],(kind,OBSERVER));self.root(d['version'],(kind,c['version'],c['policy']))
        packet=bytearray([0xcc])*64;packet[0]=c['incoming'];packet[1]=kind;struct.pack_into('<H',packet,2,c['declared']);self.mu.mem_write(PACKET,bytes(packet))
        for offset,value in ((0x34,c['flags']),(0x40,c['sender']),(0x44,11),(0x48,22),(0x50,123),(0x60,PACKET),(0x8c,c['depth'])):self.put(CONNECTION+offset,value)
        self.mu.mem_write(0x55b4d8 if self.patch else 0x548490,struct.pack('<d',42.75))
        result=self.root(d['dispatch'],this=CONNECTION)
        observed=':'.join(map(str,(result,*[self.u32(CONNECTION+off) for off in (0x34,0x44,0x48,0x8c)])))
        registration=':'.join(map(str,(self.u32(HANDLERS+kind*4),self.u32(OBSERVERS+kind*4),*self.mu.mem_read(VERSIONS+kind*2,2))))
        return [';'.join(self.events) or '-',observed,bytes(self.mu.mem_read(PACKET,64)).hex(),registration,'-','-']


def generate(smoke):
    """각 PE의 두 정밀도 관찰이 일치할 때만 fixture와 SHA/집계를 저장한다."""
    rows=[];editions={};files={Path(__file__),ROOT/'tools/decomp_process_oracle.py',ROOT/'tools/ghidra/processform-functions.json',FIXTURE,LENGTHS}
    # 세 PE는 각각 독립 가상 메모리로 로드한다.
    for edition in BINARIES:
        oracle=FormOracle(edition);cases=plan(edition);cases=cases[::37] if smoke else cases;counts=collections.Counter()
        # 각 실제 루트의 결과를 정밀도별로 실행하고 그대로 비교한다.
        for operation,c in cases:
            observed=[oracle.run(operation,c,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError('지정 form/분배 정밀도 관찰 차이')
            keys=ATTACH_KEYS if operation=='Attach' else DISPATCH_KEYS
            rows.append([edition,operation,','.join(str(c[key]) for key in keys),*observed[0]]);counts[operation]+=1
        editions[edition]=dict(cases=dict(counts),returns=dict(oracle.returns),entries=dict(oracle.hits),boundaries=dict(oracle.boundaries),assertions=oracle.asserts)
        files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(f'{edition}: processform {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 지정 free form/단일 즉시 패킷 분배 관찰. Kernel 등록·가상 부착 통지·기록/수명만 응답 경계.\n'+
        '\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),controls=list(CONTROLS),host='HJOW-Athlon',date='2026-10-11',os_calls=0,
        editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},limits=[
        '지정 free 서버 form만; 사용 중인 form의 중복 Kernel 등록/재부착·예약 예측 머리 제외',
        '단일 패킷/즉시 분배만; 타입 4 집합/지연 큐·소켓·종료 재분배는 경계/미지원',
        'Take/ProcessForm 생성자/종속 부착·패킷 등록/길이/버전/분배/콜백 깊이는 실제 명령',
        'Kernel 등록은 번호 7 응답, 새 프로세스 부착 통지는 관찰 경계; 기존 process 근거와 C++ 실제 Kernel로 별도 검증',
        '외부 로그/기록/연결 수명은 경계; double 시계 입력의 두 판본 정수 변환은 실제 CRT ftol',
        '원본 assert/OS/게임 0, 두 정밀도 ABI/보존 레지스터/x87/SEH/허용 실행·쓰기/SHA/집계 감사']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'total: {len(rows)}',flush=True)


def verify():
    """도구/PE/내보내기 SHA와 입력 순서·출력 폭·정상 루트/등록/Kernel 경계 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError('SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('지정 form/분배 합계 오류')
    for edition,item in report['editions'].items():
        # C++ 길이 상수는 실제 PE WORD를 그대로 저장했는지 주소에서 직접 읽어 감사한다.
        d=DATA[edition];pe=pefile.PE(str(ROOT/BINARIES[edition]),fast_load=True)
        name='kPatchPacketLengths' if edition=='originals' else 'kCdPacketLengths'
        body=LENGTHS.read_text(encoding='utf-8').split(name+'{',1)[1].split('}',1)[0]
        copied=[int(value,16) for value in re.findall(r'0x[0-9a-f]+',body)]
        actual=list(struct.unpack('<'+'H'*(d['last']+1),pe.get_data(d['lengths']-pe.OPTIONAL_HEADER.ImageBase,2*(d['last']+1))))
        if copied!=actual:raise RuntimeError('PE/C++ 기본 패킷 길이 표 불일치')
        chosen=[row for row in rows if row[0]==edition];cases=plan(edition);counts=collections.Counter(op for op,c in cases)
        if len(chosen)!=len(cases) or item['cases']!=dict(counts) or item['assertions']:raise RuntimeError('입력/assert 집계 오류')
        for row,(op,c) in zip(chosen,cases,strict=True):
            keys=ATTACH_KEYS if op=='Attach' else DISPATCH_KEYS
            if len(row)!=9 or row[1]!=op or row[2]!=','.join(str(c[k]) for k in keys):raise RuntimeError('관찰 행/입력 순서 오류')
            if op=='Attach' and any(len(bytes.fromhex(row[i]))!=SPECS[edition]['stride'] for i in (4,5,6)):raise RuntimeError('form/부모/머리 폭 오류')
            if op=='Dispatch' and (len(row[4].split(':'))!=5 or len(bytes.fromhex(row[5]))!=64 or len(row[6].split(':'))!=4):raise RuntimeError('분배 관찰 폭 오류')
        expected={f'{SPECS[edition]["base_ctor"]:08x}':2*counts['Attach']}
        expected.update({f'{DATA[edition][key]:08x}':2*counts['Dispatch'] for key in ('register','observe','version','dispatch')})
        if item['returns']!=expected or item['boundaries'].get('kernel')!=2*counts['Attach'] or item['boundaries'].get('attached')!=2*counts['Attach']:raise RuntimeError('정상 반환/Kernel/가상 통지 집계 오류')
        if item['entries'].get(f'{DATA[edition]["take"]:08x}')!=2*counts['Attach']:raise RuntimeError('실제 Take 진입 집계 오류')
        accepted=sum(2 for row in chosen if row[1]=='Dispatch' and '42' in row[4].split(':')[2:4])
        if item['entries'].get('004e49c0' if edition=='originals' else '004f161c')!=accepted:raise RuntimeError('실제 CRT 시각 변환 집계 오류')
    print(f'processform 검증 통과: {len(rows)}개')


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');options=parser.parse_args()
    verify() if options.verify else generate(options.smoke)
