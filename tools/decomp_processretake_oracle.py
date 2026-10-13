#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""지정 live/free ProcessForm 재수신·Kernel 중복 등록/삭제를 두 실제 PE에서 격리 관찰한다.

사용법: processretake 내보내기 후 python -X utf8 tools/decomp_processretake_oracle.py [--verify]
기대값은 실제 명령에서만 얻는다. 가상 부착/삭제 통지·소멸자의 외부 효과는 관찰 경계다.
"""
import argparse
import collections
import csv
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_process_oracle import (ProcessOracle, ROOT, BINARIES, CONTROLS, POOL, PROCS,
    VTABLE, STUBS, STACK, TYPES, PROC_STRIDE, digest)
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

# 명시 가상 통지/소멸자와 합성 프로세스 가상 표의 격리 메모리 주소다.
ATTACHED, DYING, DESTRUCTOR, NATIVE_RETURN, PROCESS_TABLE = STUBS+0x300, STUBS+0x320, STUBS+0x340, STUBS+0x360, VTABLE+0x400
# 실제 두 PE의 Kernel 등록/삭제/ProcessForm preDestroy 진입점이다.
ENTRIES = {'originals': (0x471950,0x4719e0,0x4ae240), 'originalCD': (0x4ed500,0x4ed590,0x4af450)}
# 실제 Kernel 전체 정리는 슬롯 증가 순서의 단일 순회다.
CLEAR={'originals':0x471af0,'originalCD':0x4ed4d0}
# 원본에서 저장한 행만 C++가 읽는 fixture와 검증 보고서 경로다.
FIXTURE=ROOT/'cpppj/tests/fixtures/processretake-x86.tsv'
REPORT=ROOT/'cpppj/recovery-processretake-evidence.json'
# 입력 열은 기존 Kernel 번호·SID·상태·이웃 형태·인자 타입·flags·회전·PID 변경 여부다.
KEYS=('slot','form','state','head','type','flags','rotation','redirect')


def plan(edition):
    """live 유지와 free Take를 분리하고 앞 빈 슬롯·타입 인자 차이·PID 재조회를 교차한다."""
    from decomp_process_oracle import SPECS
    rows=[];first=SPECS[edition]['server_first']
    # live Form의 실제 타입은 46으로 둔다. 전달 타입을 바꿔도 슬롯을 쓰지 않는지 확인한다.
    for form,state,kind,flags,rotation in itertools.product((5,first,32767),(8,0x88),(30,46,61),(0,0x50),(0,187)):
        rows.append(('Live',(7,form,state,1,kind,flags,rotation,0)))
    # free 지정 슬롯은 새 소유권/새 PID 없이 Take하며 Form/자산 이웃의 previous 필드를 구별한다.
    for state,kind,head,flags,rotation in itertools.product((1,3,0x83),(30,46,61),(0,1,2),(0x10,0x50),(0,187)):
        rows.append(('Free',(7,first,state,head,kind,flags,rotation,0)))
    # 중복 포인터는 앞에 빈 번호가 있어도 기존 번호를 반환한다.
    for slot,rotation in itertools.product((1,7,63),(0,187)):
        rows.append(('Register',(slot,first,8,1,46,0x50,rotation,0)))
    # preDestroy 통지가 PID를 바꾸면 실제 Kernel 제거는 그 현재 번호를 다시 읽는다.
    for slot,redirect in itertools.product((1,7),(0,1)):
        rows.append(('Delete',(slot,first,8,1,46,0x18,0,redirect)))
    # 통상 정리·지난 슬롯 신규 등록·뒤 슬롯 신규 등록·미래 슬롯 중첩 제거를 직접 실행한다.
    for mode in (0,1,2,3): rows.append(('Clear',(1,first,8,1,46,0x18,0,mode)))
    return rows


class RetakeOracle(ProcessOracle):
    """기존 실제 프로세스 몸체에 새 내보내기를 추가하고 가상 통지/소멸자만 관찰한다."""
    def __init__(self,edition):
        """현재 PC의 읽기 전용 내보내기와 실제 PE를 함께 준비한다."""
        super().__init__(edition)
        self.extra=[ROOT/f'extracted/processretake/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with self.extra[1].open(encoding='utf-8') as stream:
            # 불연속 함수 몸체마다 실제 명령 범위를 추가한다.
            for row in csv.DictReader(stream,delimiter='\t'):
                self.entry_names[int(row['entry'],16)]=row['entry']
                # 사이 주소를 허용하지 않고 내보내기 범위만 사용한다.
                for span in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in span.split('-'));self.allowed.append((low,high+1))
        self.boundaries=collections.Counter();self.case={};self.pending=None;self.kind=''

    def ret(self,purge=0):
        """명시 경계에서만 원본 호출 규약대로 스택과 EIP를 돌려놓는다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EIP,self.u32(esp))
        self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge)

    def on_instruction(self,mu,address,size,data):
        """통지·소멸자만 기록하고 Kernel/BaseProcess/Take/연결/깊이 코드는 실제로 실행한다."""
        if address==NATIVE_RETURN:
            esp,event=self.pending;self.pending=None
            self.events.append(event+str(mu.reg_read(UC_X86_REG_EAX)) if event=='A:2:' else event)
            mu.reg_write(UC_X86_REG_ESP,esp);self.ret(4);return
        if address in (ATTACHED,DYING,DESTRUCTOR):
            pointer=mu.reg_read(UC_X86_REG_ECX);index=(pointer-PROCS)//PROC_STRIDE
            if address==ATTACHED:
                self.events.append(f'N:{self.u32(pointer+4)}:{self.u32(pointer+8)}');self.ret()
            elif address==DYING:
                flags=self.u32(mu.reg_read(UC_X86_REG_ESP)+4)
                self.events.append(f'P:{index}:{flags}')
                if self.case['redirect']: self.put(POOL+self.case['form']*self.s['stride']+18,9)
                self.ret(4)
            else:
                flag=self.u32(mu.reg_read(UC_X86_REG_ESP)+4)
                if self.kind=='Clear' and index==1:
                    self.events.append(f'G:1:{int(self.u32(self.s["kernel"]+self.case["slot"]*4)!=0)}')
                visible=int(any(self.u32(self.s['kernel']+slot*4)==pointer for slot in range(1,65)))
                self.events.append(f'D:{index}:{visible}:{flag}')
                mode=self.case['redirect'];register,remove,_=ENTRIES[self.binary]
                trigger=self.kind=='Clear' and ((mode==1 and index==1) or (mode in (2,3) and index==0))
                if trigger:
                    esp=mu.reg_read(UC_X86_REG_ESP);self.pending=(esp,'R:9' if mode==3 else 'A:2:')
                    argument=9 if mode==3 else PROCS+2*PROC_STRIDE
                    self.put(esp-16,NATIVE_RETURN);self.put(esp-12,argument)
                    mu.reg_write(UC_X86_REG_ESP,esp-16);mu.reg_write(UC_X86_REG_EIP,remove if mode==3 else register)
                else: self.ret(4)
            self.boundaries[{ATTACHED:'attached',DYING:'preDestroy',DESTRUCTOR:'destructor'}[address]]+=1
            return
        super().on_instruction(mu,address,size,data)

    def prepare(self,kind,values,control):
        """입력 raw 바이트·Kernel 포인터만 공급하고 기대 출력은 만들지 않는다."""
        self.case=dict(zip(KEYS,values));case=self.case;s=self.s;self.kind=kind;self.pending=None
        self.begin(dict(server=0,boss=1,now=0,rotation=case['rotation']),control)
        for sid in (case['form'],20000,50):
            self.mu.mem_write(POOL+sid*s['stride'],bytes((i*29+7)&255 for i in range(s['stride'])))
        raw=POOL+case['form']*s['stride'];parent=POOL+20000*s['stride'];head=POOL+50*s['stride']
        self.put(parent+10,100,1);self.put(parent+11,0,1)
        self.put(parent+6,case['form'] if kind!='Free' else (50 if case['head'] else 0),2)
        self.put(raw,s['form_vtable']);self.put(raw+10,46,1);self.put(raw+11,case['state'],1)
        self.put(raw+4,50,2);self.put(raw+6,0,2);self.put(raw+14,0,2);self.put(raw+16,20000,2);self.put(raw+18,case['slot'])
        self.put(head,s['form_vtable'] if case['head']==1 else s['base_vtable'])
        self.put(head+10,46 if case['head']==1 else 100,1);self.put(head+11,8,1)
        self.put(head+4,0,2);self.put(head+14,case['form'],2)
        self.put(PROCESS_TABLE,DESTRUCTOR);self.put(PROCESS_TABLE+12,DYING);self.put(PROCESS_TABLE+20,ATTACHED)
        self.put(PROCS,PROCESS_TABLE);self.put(PROCS+4,case['form']);self.put(PROCS+8,20000)
        self.put(s['kernel']+case['slot']*4,PROCS)
        if case['redirect'] or kind=='Clear':
            self.put(PROCS+PROC_STRIDE,PROCESS_TABLE);self.put(s['kernel']+9*4,PROCS+PROC_STRIDE)
        self.put(PROCS+2*PROC_STRIDE,PROCESS_TABLE)
        self.events=[]

    def observe(self,result):
        """실행 뒤의 전체 슬롯·필드·FIFO/카운터·깊이·Kernel을 직접 읽는다."""
        s=self.s;case=self.case
        raws=[bytes(self.mu.mem_read(POOL+sid*s['stride'],s['stride'])).hex() for sid in (case['form'],20000,50)]
        slots=';'.join(f'{slot}:{(self.u32(s["kernel"]+slot*4)-PROCS)//PROC_STRIDE}'
            for slot in range(1,65) if self.u32(s['kernel']+slot*4)) or '-'
        meta=[self.u32(s['sid'][5]),self.u32(s['sid'][7]),self.u32(POOL+4)&0xffff,
            (self.u32(s['sid'][2])-POOL)//s['stride'],self.u32(POOL+18)&0xffff,
            (self.u32(s['sid'][4])-POOL)//s['stride']]
        rotation=self.u32(s['rotation']) if s['rotation'] else 0
        depth=','.join(str(self.u32(s[name])) for name in ('pre_depth','post_depth','pop_depth'))
        return [';'.join(self.events) or '-',*raws,f'{self.u32(PROCS+4)}:{self.u32(PROCS+8)}',slots,
            str(rotation),':'.join(map(str,meta)),depth,str(result)]

    def run(self,kind,values,control):
        """검사 루트 한 개만 실행하고 정상 EIP/ESP·x87·FS 반환을 확인한다."""
        self.prepare(kind,values,control);case=self.case;register,remove,pre=ENTRIES[self.binary]
        if kind in ('Live','Free'):
            self.call(self.s['base_ctor'],[case['type'],20000,case['form'],case['flags']],PROCS,16);result=0
        elif kind=='Register': result=self.call(register,[PROCS])
        elif kind=='Clear': self.call(CLEAR[self.binary],[]);result=0
        else:
            self.call(self.s['destroy'],[case['flags']],POOL+case['form']*self.s['stride'],4);result=0
        return self.observe(result)


def generate():
    """각 입력을 실제 두 PE에서 실행해 x87 정밀도 독립 관찰만 UTF-8로 저장한다."""
    rows=['# 실제 두 PE 명령의 지정 ProcessForm/Kernel 관찰. 가상 통지·소멸자 효과만 명시 경계.']
    report=dict(schema=1,host=socket.gethostname(),method='읽기 전용 PE + Unicorn x86; 게임/GUI/OS 실행 없음',
        tool_sha256=digest(Path(__file__)),editions={},fpu_controls=list(CONTROLS),keys=list(KEYS))
    for edition in ('originals','originalCD'):
        oracle=RetakeOracle(edition);counts=collections.Counter()
        # 같은 입력의 다른 x87 재실행은 새 관찰 수에 더하지 않는다.
        for kind,values in plan(edition):
            first=oracle.run(kind,values,CONTROLS[0]);second=oracle.run(kind,values,CONTROLS[1])
            if first!=second: raise RuntimeError('x87 정밀도 의존 관찰')
            rows.append('\t'.join([edition,kind,','.join(map(str,values)),*first]));counts[kind]+=1
        report['editions'][edition]=dict(binary_sha256=digest(ROOT/BINARIES[edition]),rows=sum(counts.values()),
            cases=dict(counts),hits=dict(oracle.hits),boundaries=dict(oracle.boundaries),
            exports={str(path.relative_to(ROOT)):digest(path) for path in [*oracle.exports,*oracle.extra]})
    FIXTURE.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    report.update(total_rows=len(rows)-1,fixture_sha256=digest(FIXTURE),
        stubbed=['프로세스 OnAttached/OnFormDestroy 가상 통지', '가상 소멸자 효과: 호출 인자/슬롯 존재 관찰'],
        limitations=['합성 raw/타입/로컬 프로세스 입력', '프로세스 교체/직렬화/멀티플레이 전송 제외',
            '미등록 참조 소유권/잘못된 부모·PID assert 이후 실행 제외', 'Kernel은 앞 64슬롯 직접 비교',
            'Clear 통지의 입력 콜백만 대체하며 그 안의 등록/제거도 실제 명령으로 실행'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'ProcessForm 재수신 실제 PE 독립 관찰 {report["total_rows"]}개 저장')


def verify():
    """실제 PE/도구/몸체/fixture SHA와 각 입력 행의 개수·집합을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    assert report['tool_sha256']==digest(Path(__file__))
    assert report['fixture_sha256']==digest(FIXTURE)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==report['total_rows']
    for edition,data in report['editions'].items():
        assert data['binary_sha256']==digest(ROOT/BINARIES[edition])
        for path,sha in data['exports'].items(): assert digest(ROOT/path)==sha
        selected=[row for row in rows if row[0]==edition]
        expected={(kind,','.join(map(str,values))) for kind,values in plan(edition)}
        assert len(selected)==data['rows']==len(expected)
        assert {(row[1],row[2]) for row in selected}==expected
        assert all(len(row)==13 for row in selected)
    print(f'ProcessForm 재수신 SHA·입력 감사 {len(rows)}개 통과')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify: verify()
    else: generate()
