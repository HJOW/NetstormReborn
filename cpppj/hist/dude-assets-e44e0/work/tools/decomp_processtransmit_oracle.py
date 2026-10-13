#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 BaseProcess 생성·Kernel 등록·Form Pop의 서버 전송 관찰을 두 PE에서 기록한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_process_oracle import ProcessOracle, ROOT, BINARIES, SPECS, CONTROLS, POOL, PROCS, ARENA, STUBS, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 입력 열과 독립 산출물 경로다. 기존 ProcessForm/재수신 근거는 덮어쓰지 않는다.
KEYS=('server','named','state','head','flags','type','size')
FIXTURE=ROOT/'cpppj/tests/fixtures/processtransmit-x86.tsv'
REPORT=ROOT/'cpppj/processtransmit.json'
# 프로세스의 합성 가상 표와 OnAttached 관찰 주소다. 원본 메모리 밖의 격리 영역만 사용한다.
TAG_TABLE,ATTACHED,WRITE=ARENA+0x1e000,STUBS+0x340,STUBS+0x360


def plan():
    """서버/클라이언트·신규/지정 free·전송/억제 flags와 종속 이웃을 교차한다."""
    rows=[]
    for server,named,flags,size in itertools.product((0,1),(0,1),(0,1,2,4,8,0x10,0x40,0x50,0x80),(32,0xffffffff)):
        if not server and not named and not flags&0x10:continue
        for state in ((1,0x83) if named else (1,)):
            profile=len(rows)%3
            rows.append(dict(zip(KEYS,(server,named,state,profile,flags,(23,30,54)[profile],size))))
    return rows


class TransmitOracle(ProcessOracle):
    """Kernel은 실제 몸체로 실행하고 전송/부착 가상 통지만 관찰 경계로 받는다."""
    def __init__(self,edition):
        """기존 실제 Process/Take/Unpop 범위를 재사용하며 새로운 대체 몸체를 넣지 않는다."""
        super().__init__(edition);self.boundaries=collections.Counter();self.returns=collections.Counter()
        self.extra=[ROOT/f'extracted/processretake/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with self.extra[1].open(encoding='utf-8') as fp:
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entry_names[int(row['entry'],16)]=row['entry']
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))

    def ret(self,purge=0,value=0):
        """관찰 경계의 원본 thiscall 스택 정리량으로 반환한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,value)
        self.mu.reg_write(UC_X86_REG_EIP,self.u32(esp));self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge)

    def args(self,count):
        """현재 관찰 경계의 DWORD 인자를 읽는다."""
        return struct.unpack('<'+'I'*count,self.mu.mem_read(self.mu.reg_read(UC_X86_REG_ESP)+4,count*4))

    def root(self,address,args=(),this=0,purge=0):
        """실제 몸체의 ABI/x87/SEH 정상 반환을 확인하고 진입 개수를 기록한다."""
        result=self.call(address,list(args),this=this,purge=purge);self.returns[f'{address:08x}']+=1;return result
    def on_instruction(self,mu,address,size,data):
        """전송 순간의 raw Form·부모 체인·프로세스 필드를 기록하고 정상 ABI로 반환한다."""
        s=self.s
        if address==(0x471950 if self.patch else 0x4ed500) and not self.native_kernel:
            # 전체 Kernel 몸체는 기존 process/processretake 근거와 아래 대표 입력에서 실행한다.
            # 나머지 교차 입력은 같은 번호1 응답 경계를 쓰고 C++에서는 실제 Kernel 등록으로 대조한다.
            if self.args(1)!=(PROCS,):raise RuntimeError('Kernel 등록 포인터 오류')
            self.put(s['kernel']+4,PROCS)
            if s['rotation']:self.put(s['rotation'],(self.u32(s['rotation'])+1)%s['type_count'])
            self.boundaries['kernelResponse']+=1;self.ret(value=1);return
        if address==WRITE:
            buffer,changed,force=self.args(3);expected=0x5c7c50 if self.patch else 0x56f200
            if buffer!=expected or force:raise RuntimeError(f'프로세스 WriteData 인자 오류: {buffer:x}/{force}')
            form=self.u32(PROCS+4)
            raw=bytes(mu.mem_read(POOL+form*s['stride'],s['stride'])).hex()
            parent=bytes(mu.mem_read(POOL+20000*s['stride'],s['stride'])).hex()
            self.events.append(f'W:{form}:{changed}:{raw}:{parent}:{form}:{self.u32(PROCS+8)}')
            self.boundaries['writeData']+=1;self.ret(purge=12,value=self.case['size']);return
        if address==(0x4e2e30 if self.patch else 0x4c0420):
            self.events.append('S:'+str(self.case['size']));self.boundaries['sendBuffer']+=1;self.ret();return
        if address==s['stubs']['transmit']:
            peer,command,payload,flags=self.args(4)
            if (peer,command,payload)!=(0xffffffff,0xffffffff,0):raise RuntimeError('Form Pop 전송 인자 오류')
            form=self.sid_of(mu.reg_read(UC_X86_REG_ECX))
            raw=bytes(mu.mem_read(POOL+form*s['stride'],s['stride'])).hex()
            parent=bytes(mu.mem_read(POOL+20000*s['stride'],s['stride'])).hex()
            self.events.append(f'X:{form}:{flags}:{raw}:{parent}:{self.u32(PROCS+4)}:{self.u32(PROCS+8)}')
            self.boundaries['transmit']+=1;self.ret(purge=16);return
        if address==ATTACHED:
            self.events.append(f'N:{self.u32(PROCS+4)}:{self.u32(PROCS+8)}')
            self.boundaries['attached']+=1;self.ret();return
        # 지정 Form 도구의 Kernel 번호 대체는 사용하지 않는다.
        previous=self.previous
        try:ProcessOracle.on_instruction(self,mu,address,size,data)
        except RuntimeError as error:raise RuntimeError(f'{error}; 직전 {previous}; 입력 {getattr(self,"case",None)}') from error

    def run(self,c,control,refresh=False):
        """주변 표식은 입력으로만 만들고 생성자/Take/Pop 출력은 실제 명령에서 읽는다."""
        self.case=c;self.native_kernel=c['server']==1 and not c['named'] and c['flags']==0 and c['size']==32 and not refresh
        self.begin(dict(server=c['server'],boss=1,now=0,rotation=0),control);s=self.s
        parent=POOL+20000*s['stride'];head=POOL+50*s['stride']
        for ptr in (parent,head):self.mu.mem_write(ptr,bytes((i*29+7)&255 for i in range(s['stride'])))
        self.put(parent+10,100,1);self.put(parent+11,0,1);self.put(parent+6,50 if c['head'] else 0,2)
        self.put(head,s['form_vtable'] if c['head']==1 else s['base_vtable'])
        self.put(head+10,46 if c['head']==1 else 100,1);self.put(head+11,8,1)
        form=s['server_first']+50 if c['named'] else 0
        if form:
            self.mu.mem_write(POOL+form*s['stride'],bytes((i*29+7)&255 for i in range(s['stride'])))
            self.put(POOL+form*s['stride']+11,c['state'],1)
        table=bytearray(24);struct.pack_into('<I',table,8,WRITE);struct.pack_into('<I',table,20,ATTACHED);self.mu.mem_write(TAG_TABLE,bytes(table));self.put(PROCS,TAG_TABLE)
        self.root(s['base_ctor'],(c['type'],20000,form,c['flags']),PROCS,16)
        form=self.u32(PROCS+4)
        if refresh:
            self.events=[];self.root(0x41c010 if self.patch else 0x48f8d0,this=PROCS)
        return [';'.join(self.events) or '-',bytes(self.mu.mem_read(POOL+form*s['stride'],s['stride'])).hex(),
            bytes(self.mu.mem_read(parent,s['stride'])).hex(),bytes(self.mu.mem_read(head,s['stride'])).hex(),
            f'{form}:{self.u32(PROCS+8)}',str(self.u32(s['rotation']) if s['rotation'] else 0),
            ':'.join(map(str,(self.u32(s['sid'][5]),self.u32(s['sid'][7]),int(self.u32(s['kernel']+4)==PROCS))))]


def generate(smoke):
    """두 정밀도 출력/ABI/assert를 확인한 뒤 실제 관찰과 정확한 SHA를 저장한다."""
    rows=[];editions={};files={Path(__file__),FIXTURE,ROOT/'tools/decomp_process_oracle.py'}
    for edition in ('originals','originalCD'):
        oracle=TransmitOracle(edition);cases=plan();cases=cases[::41] if smoke else cases
        for refresh,c in itertools.product((False,True),cases):
            observed=[oracle.run(c,control,refresh) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError('Form 전송 정밀도 출력 차이')
            rows.append([edition,'Refresh' if refresh else 'Attach',','.join(str(c[k]) for k in KEYS),*observed[0]])
        editions[edition]=dict(cases=2*len(cases),returns=dict(oracle.returns),entries=dict(oracle.hits),boundaries=dict(oracle.boundaries),assertions=oracle.asserts)
        files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(f'{edition}: Form 전송 {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 BaseProcess/Kernel/Form Pop 서버 전송 관찰. 전송 직렬화·파생 OnAttached만 외부 경계.\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='VM-W11-CODEX',controls=list(CONTROLS),os_calls=0,editions=editions,
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},limits=[
            '실제 Allocate/Take/ProcessForm Attach/Pop이다. Kernel은 각 PE 대표 입력2회 실제 실행, 다른 입력은 번호1 응답 경계이며 기존 독립 Kernel 근거와 C++ 실제 등록을 함께 사용한다.',
            '파생 WriteData·버퍼 송신·OnAttached는 현재 raw 상태를 관찰하는 경계다. 실제 Gun serializer는 별도 독립 근거로 대조한다.',
            '멀티플레이 소켓/메시지 전송 및 실제 Gun/Path 파생 몸체의 검증 완료를 뜻하지 않는다.',
            '신규·지정 free 슬롯만; live 재수신은 기존 독립 processretake 근거를 유지한다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA/입력 순서/폭/실제 Kernel·Pop 진입/전송·통지 집계를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError('SHA 불일치: '+name)
    if report['total']!=len(rows) or report['os_calls'] or report['controls']!=list(CONTROLS):raise RuntimeError('전송 합계 오류')
    for edition,item in report['editions'].items():
        chosen=[r for r in rows if r[0]==edition];s=SPECS[edition]
        if len(chosen)!=2*len(plan()) or item['cases']!=len(chosen) or item['assertions']:raise RuntimeError('전송 입력/assert 오류')
        for row,(refresh,c) in zip(chosen,itertools.product((False,True),plan()),strict=True):
            if len(row)!=10 or row[1]!=('Refresh' if refresh else 'Attach') or row[2]!=','.join(str(c[k]) for k in KEYS):raise RuntimeError('전송 행/입력 오류')
            if any(len(bytes.fromhex(row[i]))!=s['stride'] for i in (4,5,6)):raise RuntimeError('전송 raw 폭 오류')
        count=2*len(chosen);pop=0x4ac560 if s['patch'] else 0x4abfb0;kernel=0x471950 if s['patch'] else 0x4ed500
        if item['entries'].get(f'{pop:08x}')!=count*3//2 or item['entries'].get(f'{kernel:08x}')!=2 or item['boundaries'].get('kernelResponse')!=count-2 or item['boundaries'].get('attached')!=count:raise RuntimeError('실제 Pop/Kernel/부착 통지 오류')
        attached=[r for r in chosen if r[1]=='Attach'];refreshed=[r for r in chosen if r[1]=='Refresh']
        if item['boundaries'].get('transmit',0):raise RuntimeError('서버 ProcessForm 공통 Pop 전송 억제 누락')
        for key,prefix in (('writeData','W:'),('sendBuffer','S:')):
            if item['boundaries'].get(key,0)!=4*sum(prefix in r[3] for r in attached)+2*sum(prefix in r[3] for r in refreshed):raise RuntimeError('파생 직렬화/송신 집계 오류')
    print(f'Form 전송 검증 통과: {len(rows)}개')


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');options=parser.parse_args()
    verify() if options.verify else generate(options.smoke)
