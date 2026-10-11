#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 건설 상태 쓰기/읽기/수신/가상 래퍼 전체를 실행해 독립 비트 관찰을 저장한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_constructionprocess_oracle import ProcessOracle,OBJECT,SOUND
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,STACK,STOP,digest
from decomp_damageablepredestroy_oracle import CONTROLS
from unicorn.x86_const import UC_X86_REG_ECX

# 객체와 겹치지 않는 패킷/출력 부모 저장소다. 모두 에뮬레이터 내부 주소다.
PACKET,OUTPUT=OBJECT+0x400,OBJECT+0x500
# 실제 네 몸체와 Kernel 조회, 현재 두 헤더 전역 주소다.
SPEC={'originals':dict(Write=0x4426d0,Read=0x442760,Receive=0x4427b0,Wrapper=0x443bd0,kernel=0x471aa0,prefix=(0x5325b9,0x5325b8)),
      'originalCD':dict(Write=0x4d0430,Read=0x4d04d0,Receive=0x4d0540,Wrapper=0x4d2b20,kernel=0x4ed640,prefix=(0x53fc30,0x53fc2c))}
SPEC['original1037']=dict(SPEC['originalCD'])
# 입력 DWORD의 순서와 저장 관찰 경로다. double 입력은 산술 없이 비트로 공급한다.
KEYS=('dirty','form','parent','force','includeParent','startLow','startHigh','endLow','endHigh','additional','protocol','message','free','pid','present','mutate','processType','pattern')
FIXTURE=ROOT/'cpppj/tests/fixtures/constructiondata-x86.tsv'
REPORT=ROOT/'cpppj/recovery-constructiondata-evidence.json'


def case(**changes):
    """기대 패킷을 계산하지 않고 객체/패킷/경계 응답의 입력만 만든다."""
    value=dict(dirty=8,form=90,parent=20000,force=0,includeParent=0,startLow=0x12345678,startHigh=0x40000000,
        endLow=0x87654321,endHigh=0x40200000,additional=0x5a,protocol=1,message=0x37,free=0,pid=7,present=1,mutate=0,processType=30,pattern=0)
    value.update(changes);return value


def inputs():
    """모든 변경 비트·부모/force 조건·특수 실수 비트·빈/기존 form·경계 뒤 변동을 교차한다."""
    rows=[]
    # 기존 bit 1이 켜졌지만 부모 인자가 0인 원본의 비대칭 패킷도 저장한다.
    for dirty,parent,force in itertools.product(range(256),(0,0x100003c),(0,1)):
        rows.append(('Write',case(dirty=dirty,parent=parent,force=force,form=0xfedcba98)))
    # 실제 래퍼는 두 플래그 중 하나라도 켜져 있을 때만 부모를 넘긴다.
    for dirty,include,force,form in itertools.product((0,2,4,8,255),(0,1),(0,1),(5,6)):
        rows.append(('Wrapper',case(dirty=dirty,includeParent=include,force=force,form=form)))
    # 모든 incoming 비트와 헤더 길이를 무시하는 읽기, 임의 payload/NaN 비트를 비교한다.
    for mask,pattern in itertools.product(range(256),(0,1)):
        rows.append(('Read',case(dirty=mask,pattern=pattern)))
    # null 객체여도 객체 필드를 선택하지 않으면 부모/비트만 읽을 수 있다.
    for mask in range(256):
        if not mask&14:rows.append(('ReadNull',case(dirty=mask,present=0)))
    # 기존 form의 부모 비트는 원본 assert라 제외한다. Kernel 없는 경우도 쓰기 없는 비트만 허용한다.
    for mask in range(256):
        if mask&1:continue
        rows.append(('Receive',case(dirty=mask,pid=0x10007)))
        if not mask&14:rows.append(('Receive',case(dirty=mask,present=0,pid=0xffffffff)))
    # free form은 0xff만 허용한다. 할당 뒤 현재 타입/패킷은 다시 읽되 캡처한 form 번호는 유지한다.
    for parent,mutate in itertools.product((0,60,20000,0xffff,0x100003c,0xffffffff),(0,1,2)):
        rows.append(('Receive',case(dirty=255,free=1,parent=parent,mutate=mutate)))
    # 출력 필드에 특수 double·추가 BYTE·헤더 전역을 공급한다.
    for dirty,parent in itertools.product((2,4,8,14,255),(0,0xffffffff)):
        rows.append(('Write',case(dirty=dirty,parent=parent,startLow=1,startHigh=0x7ff00000,endLow=0,endHigh=0x80000000,
            additional=255,protocol=0xe1,message=0xfe)))
    return rows


class DataOracle(ProcessOracle):
    """할당/Kernel/명시 form 부착만 응답하고 패킷·수신 생성자·분기는 실제 명령으로 실행한다."""
    def __init__(self,edition):
        """공통 메모리/SEH 로더 뒤 이번 네 함수의 실제 불연속 범위만 허용한다."""
        super().__init__(edition);self.d=SPEC[edition];self.boundaries=collections.Counter()
        self.exports=[ROOT/f'extracted/constructiondata/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # Ghidra 함수마다 포함 끝 주소를 반열린 실행 범위로 옮긴다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 분리된 범위를 하나의 큰 실행 영역으로 합치지 않는다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def on_instruction(self,mu,address,size,data):
        """세 외부 경계의 실제 인자와 지정된 현재 전역/패킷 변동만 공급한다."""
        c=self.case
        if address==self.r['allocate']:
            size,=self.args(1)
            if size!=56:raise RuntimeError('수신 객체 할당 크기 오류')
            self.events.append('A:56');self.boundaries['allocate']+=1
            if c['mutate']==1:self.put('processType',44)
            if c['mutate']==2:
                mu.mem_write(PACKET+4,struct.pack('<I',91));mu.mem_write(PACKET+9,struct.pack('<I',0xffffffff))
            self.ret(OBJECT,0);return
        if address==self.r['attach']:
            self.this_process();self.events.append('J:'+':'.join(map(str,self.args(4))));self.boundaries['attach']+=1
            self.ret(0,16);return
        if address==self.d['kernel']:
            index,=self.args(1)
            if index!=c['pid']:raise RuntimeError('수신 Kernel DWORD 번호 오류')
            self.events.append('K:'+str(index));self.boundaries['kernel']+=1
            self.ret(OBJECT+0x28 if c['present'] else 0,0);return
        OwnerOracle.on_instruction(self,mu,address,size,data)

    def run_data(self,operation,c,control):
        """패킷/객체 입력 뒤 전체 정상 반환과 SEH/ABI/x87·필드·주변 불변을 기록한다."""
        self.case=c;self.events=[];mu=self.mu
        image=bytearray([0xa5])*56;image[8]=c['dirty'];image[32]=c['additional']
        struct.pack_into('<4I',image,16,c['startLow'],c['startHigh'],c['endLow'],c['endHigh'])
        struct.pack_into('<3I',image,44,c['form'],c['parent'],SOUND);mu.mem_write(OBJECT,bytes(image))
        packet=bytearray([0xcc])*64
        if operation in ('Read','ReadNull','Receive'):
            packet[0:2]=bytes((c['protocol'],c['message']));struct.pack_into('<HI',packet,2,0x1234,c['form']);packet[8]=c['dirty']
            # payload 자체만 임의 입력으로 만든다. 선택 필드의 기대 읽기/쓰기 값은 계산하지 않는다.
            for i in range(21):packet[9+i]=255 if c['pattern'] else (i*17+11)&255
            if operation=='Receive' and c['free']:struct.pack_into('<I',packet,9,c['parent'])
        before=packet.hex();mu.mem_write(PACKET,bytes(packet));mu.mem_write(OUTPUT,struct.pack('<I',0xdeadbeef))
        form=self.slot(c['form']) if operation=='Receive' else self.slot(90)
        raw=bytearray([0xa7])*self.stride;raw[11]=c['free'];struct.pack_into('<I',raw,18,c['pid']);mu.mem_write(form,bytes(raw))
        self.put('processType',c['processType'])
        # 두 헤더 전역은 PE 기본값과 다른 입력도 받는다.
        for address,value in zip(self.d['prefix'],(c['protocol'],c['message']),strict=True):mu.mem_write(address,bytes([value]))
        mu.mem_write(0,struct.pack('<I',0x11223344));self.prepare_registers(control)
        self.write_ranges=[(0,4),(PACKET,PACKET+64),(OUTPUT,OUTPUT+4),(OBJECT+8,OBJECT+9)]
        if operation in ('Read','ReadNull','Receive'):self.write_ranges.append((OBJECT+16,OBJECT+33))
        if operation=='Receive' and c['free']:self.write_ranges.extend(((OBJECT,OBJECT+4),(OBJECT+40,OBJECT+52)))
        key='Read' if operation=='ReadNull' else operation;entry=self.d[key];purge=0
        if operation=='Write':args=[PACKET,c['form'],c['parent'],c['force']];purge=16;this=OBJECT
        elif operation=='Wrapper':args=[PACKET,c['includeParent'],c['force']];purge=12;this=OBJECT+40
        elif operation in ('Read','ReadNull'):args=[PACKET,OUTPUT];purge=8;this=0 if operation=='ReadNull' else OBJECT
        else:args=[0,0,PACKET];this=0
        mu.reg_write(UC_X86_REG_ECX,this);mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args))
        mu.emu_start(entry,STOP,count=100000);self.check(control,STACK+4+purge);self.returns[f'{entry:08x}']+=1
        if bytes(mu.mem_read(0,4))!=struct.pack('<I',0x11223344):raise RuntimeError('수신 SEH 복구 오류')
        if bytes(mu.mem_read(form,self.stride))!=bytes(raw):raise RuntimeError('명시 부착 경계 밖 form 변경')
        fields=':'.join(map(str,(mu.mem_read(OBJECT+8,1)[0],*struct.unpack('<4I',mu.mem_read(OBJECT+16,16)),
            struct.unpack('<I',mu.mem_read(OBJECT+52,4))[0],mu.mem_read(OBJECT+32,1)[0])))
        output=str(struct.unpack('<I',mu.mem_read(OUTPUT,4))[0]) if operation in ('Read','ReadNull') else '-'
        return [';'.join(self.events) or '-',fields,output,before,bytes(mu.mem_read(PACKET,64)).hex(),bytes(raw).hex()]


def generate(smoke=False):
    """세 PE의 독립 두 정밀도 관찰이 같을 때만 자료를 저장한다."""
    rows=[];editions={};paths={Path(__file__),FIXTURE,ROOT/'tools/ghidra/constructiondata-functions.json',
        *[ROOT/f'tools/decomp_{name}_oracle.py' for name in ('owner','priestgeometry','returnpiece','damageablepredestroy','constructionprocess')]}
    # 판본마다 별도 PE/가상 메모리에서 전체 입력을 실행한다.
    for edition in SPECS:
        oracle=DataOracle(edition);plan=inputs()[::37] if smoke else inputs();counts=collections.Counter()
        # 기대 패킷을 재계산하지 않고 실제 두 실행의 출력을 비교한다.
        for operation,c in plan:
            observed=[oracle.run_data(operation,c,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError('건설 상태 정밀도 관찰 차이: '+edition)
            rows.append([edition,operation,','.join(str(c[key]) for key in KEYS),*observed[0]]);counts[operation]+=1
        editions[edition]=dict(cases=dict(counts),returns=dict(oracle.returns),native_calls=dict(oracle.native_calls),boundaries=dict(oracle.boundaries),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary'])
        # 공통 로더가 읽은 앞 단계 내보내기도 해시 감사에 포함한다.
        for group in ('owner','priestgeometry','returnpiece','constructionprocess'):
            paths.update(ROOT/f'extracted/{group}/{edition}/{name}' for name in ('creation.c','functions.tsv'))
        print(f'{edition}: constructiondata {len(plan)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 건설 상태 쓰기/읽기/수신/래퍼. 할당/Kernel/명시 form 부착만 응답 경계.\n# 판본 연산 입력 사건 필드 부모출력 패킷전 패킷후 formraw\n'+
        '\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-11',controls=list(CONTROLS),os_calls=0,editions=editions,
        files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},limits=[
            '쓰기/읽기/수신 생성자·분기/가상 래퍼 전체 실제 명령; 할당/Kernel/명시 form 부착만 경계',
            '모든 변경 비트·부모 DWORD·force/래퍼 플래그·임의 double/추가 BYTE·현재 타입/패킷 경계 변동',
            '원본 assert·null 객체 필드 접근·할당 실패 뒤 null 쓰기·게임/OS 미실행',
            '정상 ABI/x87/SEH·허용 실행/쓰기·form 불변·SHA/진입/반환/경계 집계']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'total: {len(rows)}',flush=True)


def verify():
    """SHA·입력/필드/패킷 폭·실제 진입/반환/경계 수를 감사하며 기대 결과는 계산하지 않는다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 생성 당시 모든 입력/실행 자료의 해시를 확인한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError('SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('건설 상태 합계 오류')
    # 각 PE의 입력 순서·출력 폭·루트와 중첩 진입 집계를 검증한다.
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition];plan=inputs();counts=collections.Counter(op for op,c in plan)
        if len(chosen)!=len(plan) or item['cases']!=dict(counts) or item['assertions']:raise RuntimeError('건설 상태 입력/assert 오류')
        # 패킷은 앞뒤 64바이트, 객체 필드는 일곱 항목이며 주변 form raw도 보존한다.
        for row,(operation,c) in zip(chosen,plan,strict=True):
            if len(row)!=9 or row[1]!=operation or row[2]!=','.join(str(c[key]) for key in KEYS) or len(row[4].split(':'))!=7:raise RuntimeError('건설 상태 자료 폭/순서 오류')
            if len(bytes.fromhex(row[6]))!=64 or len(bytes.fromhex(row[7]))!=64 or len(bytes.fromhex(row[8]))!=SPECS[edition]['stride']:raise RuntimeError('건설 상태 raw 폭 오류')
        roots=dict(Write=counts['Write'],Wrapper=counts['Wrapper'],Read=counts['Read']+counts['ReadNull'],Receive=counts['Receive'])
        # 정상 루트 반환과 래퍼/수신에서 중첩 진입한 실제 쓰기·읽기를 구분한다.
        for key,count in roots.items():
            address=f'{SPEC[edition][key]:08x}';entered=count+(counts['Wrapper'] if key=='Write' else counts['Receive'] if key=='Read' else 0)
            if item['returns'].get(address)!=2*count or item['native_calls'].get(address)!=2*entered:raise RuntimeError('건설 상태 실제 진입/반환 오류')
        free=sum(op=='Receive' and c['free'] for op,c in plan);existing=counts['Receive']-free
        if item['boundaries']!=dict(allocate=2*free,attach=2*free,kernel=2*existing):raise RuntimeError('건설 상태 경계 수 오류')
    print(f'constructiondata 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)


if __name__=='__main__':main()
