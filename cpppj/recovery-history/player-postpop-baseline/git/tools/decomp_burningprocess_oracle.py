#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Burning 모드0/1의 실제 생성자·표시등록·particle150 초기화를 두 보호 PE로 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
import zlib
from decomp_owner_oracle import OwnerOracle,ROOT,POOL,STACK,STOP,digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 생성자와 기존 팔레트/표시 목록의 실제 전역 주소다. 공통 Kernel 부착과 힙만 경계다.
SPEC={
 'originals':dict(ctor=0x423b20,attach=0x41bd20,new=0x4e4391,free=0x4e4354,
                  systems=0x595138,count=0x59513c,palette=0x5453d0,type=0x5410a8,vtable=0x503730),
 'originalCD':dict(ctor=0x4a5a50,attach=0x48f590,new=0x4f1650,free=0x4f1a50,
                   systems=0x5209c0,count=0x5209c4,palette=0x538d34,type=0x51c998,vtable=0x505870),
}
# 원본 포인터를 사용하는 격리 객체/목록/힙 주소와 결과의 명시 호스트 토큰이다.
OBJECT,TABLE,HEAP,PALETTE=0x11324000,0x11325000,0x11330000,0x11328000
TOKEN=777
# 초기화되는 의미 필드만 출력한다. 미초기화 float40/44와DWORD74/84..8c는 완료 값으로 만들지 않는다.
OFFSETS=(4,8,12,16,20,24,28,32,36,40,48,52,56,60,72,80,84,88,92,96,100,104,108,112,120,124,128)
KEYS=('parent','mode','registry','depth','zeroDepth','fill')
FIXTURE=ROOT/'cpppj/tests/fixtures/burningprocess-constructor-x86.tsv'
REPORT=ROOT/'cpppj/recovery-burningprocess-constructor-evidence.json'


def inputs():
    """캐시 팔레트가 이미 있는 명시 현재 월드의 부모/모드/구멍/확장/초기힙을 교차한다."""
    return [dict(zip(KEYS,values)) for values in itertools.product((0,50),(0,1),range(4),(-11,34),(-8,10),(0,90))]


class BurningOracle(OwnerOracle):
    """생성자와 하위 초기화를 원본 명령으로 실행하고 할당·실제 Host 부착을 명시 경계로 관찰한다."""
    def __init__(self,edition):
        """현재 호스트의 좁은 Ghidra 자료만 허용하며 보호 실행 파일은 읽기 전용으로 사용한다."""
        super().__init__(edition);self.p=SPEC[edition];self.mu.mem_map(0,0x1000);self.mu.mem_map(0x11320000,0x100000)
        names=('burning-process',) if edition=='originals' else ('burning-process','burning-process-particle-helper')
        # CD가150번호출하는두좌표초기화helper는 별도 좁은실제body를요구한다.
        for name in names:
            paths=[ROOT/f'extracted/{name}/{edition}/{part}' for part in ('creation.c','functions.tsv')];self.exports.extend(paths)
            with paths[1].open(encoding='utf-8') as fp:
                # 실제 내보낸 함수의 불연속 몸체 밖은 실행하지 않는다.
                for row in csv.DictReader(fp,delimiter='\t'):
                    self.entries.add(int(row['entry'],16))
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.events=[];self.nextHeap=HEAP;self.boundaries=collections.Counter();self.returns=0

    def read(self,address,width=4):
        """x86의 현재 비정렬 필드를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def on_instruction(self,mu,address,size,data):
        """힙과 실제 ProcessForm 부착 인자만 관찰하며 생성자/표시등록/입자 초기화는 직접 실행한다."""
        p=self.p;esp=mu.reg_read(UC_X86_REG_ESP);purge=0;value=0
        if address==p['new']:
            length=self.read(esp+4);value=self.nextHeap;self.nextHeap+=(length+4095)&~4095;self.events.append('N:'+str(length))
            if self.nextHeap>HEAP+0xc0000:raise RuntimeError('Burning 격리힙 범위 오류')
            mu.mem_write(value,bytes([self.case['fill']])*length)
        elif address==p['free']:
            self.events.append('L:registry')
        elif address==p['attach']:
            args=[self.read(esp+4+index*4) for index in range(4)];self.events.append('B:'+':'.join(map(str,args)))
            mu.mem_write(mu.reg_read(UC_X86_REG_ECX)+4,struct.pack('<II',83,args[1]));purge=16
        else:super().on_instruction(mu,address,size,data);return
        self.boundaries[f'{address:08x}']+=1;ret=self.read(esp);mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,case):
        """실제 ctor 정상RET/스택/x87과 현재 의미필드·150입자·목록/순서를 보존한다."""
        mu=self.mu;p=self.p;self.case=case;self.events=[];self.nextHeap=HEAP
        rows=([],[0],[111,0,222],[111,222])[case['registry']]
        mu.mem_write(OBJECT,bytes([case['fill']])*144);mu.mem_write(TABLE,struct.pack('<'+'I'*len(rows),*rows) if rows else bytes(4));mu.mem_write(p['systems'],struct.pack('<I',TABLE if rows else 0));mu.mem_write(p['count'],struct.pack('<I',len(rows)));mu.mem_write(p['palette'],struct.pack('<I',PALETTE));mu.mem_write(p['type'],struct.pack('<I',14));mu.mem_write(POOL,bytes(128*self.stride));mu.mem_write(0,bytes(4))
        depthOffset=35 if self.edition=='originals' else 33
        mu.mem_write(self.slot(0)+depthOffset,bytes([case['zeroDepth']&255]));mu.mem_write(self.slot(50)+depthOffset,bytes([case['depth']&255]))
        self.write_ranges=[(0,4),(OBJECT,OBJECT+144),(TABLE,TABLE+320),(HEAP,HEAP+0xc0000),(p['systems'],p['systems']+4),(p['count'],p['count']+4)]
        # 정상 유한 입력의53비트 x87과 독립 스택을 매 호출 다시 공급한다.
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(reg,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,0x027f);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_ECX,OBJECT);mu.reg_write(UC_X86_REG_ESP,STACK);mu.mem_write(STACK,struct.pack('<III',STOP,case['parent'],case['mode']));mu.emu_start(p['ctor'],STOP,count=300000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+12 or mu.reg_read(UC_X86_REG_EAX)!=OBJECT or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('Burning 생성자 정상RET/스택/x87 오류')
        if self.read(OBJECT)!=p['vtable']:raise RuntimeError('Burning 실제가상표 오류')
        self.returns+=1;capacity=self.read(p['count']);array=self.read(p['systems']);slots=[self.read(array+index*4) for index in range(capacity)];slots=[TOKEN if value==OBJECT else value for value in slots];particle=bytes(mu.mem_read(self.read(OBJECT+44),3600))
        return [','.join(str(self.read(OBJECT+offset)) for offset in OFFSETS),','.join(map(str,slots)),';'.join(self.events),str(zlib.adler32(particle)),str(sum(particle))]


def generate(smoke):
    """실제완료 생성자 입력만 저장하며 캐시가없는 첫 팔레트/프레임 몸체는 구분한다."""
    cases=inputs();cases=cases[:1] if smoke else cases;rows=[];editions={};files={ROOT/'tools/decomp_burningprocess_oracle.py',ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/ghidra/burning-process-functions.json',ROOT/'tools/ghidra/burning-process-particle-helper-functions.json'}
    # 원본 양판본 독립 입력의 정상 반환을 각각 기록한다.
    for edition in SPEC:
        oracle=BurningOracle(edition)
        for case in cases:
            try:result=oracle.run(case)
            except Exception as error:raise RuntimeError(f'{edition} {case}: {error}') from error
            rows.append([edition,*[case[key] for key in KEYS],*result])
        editions[edition]=dict(returns=oracle.returns,instructions=oracle.instructions,assertions=oracle.assertions,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.boundaries));files.update(oracle.exports);files.add(ROOT/oracle.spec['binary']);print(edition,oracle.returns,flush=True)
    if smoke:return
    for relative in ('cpppj/src/o/RawBurningVisual.cpp','cpppj/src/o/RawBurningVisual.h','cpppj/tests/BurningVisualTests.cpp','cpppj/src/o/RawConstructionLifetime.cpp','cpppj/src/o/RawConstructionLifetime.h','cpppj/recovery-constructionlifetime-evidence.json'):
        files.add(ROOT/relative)
    FIXTURE.write_text('# 실제Burning 모드0/1 warm-palette 생성자 정상반환. cold팔레트/후속particle frame은별도경계다.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8');files.add(FIXTURE)
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',total=len(rows),editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},os_calls=0,cpp_shared_tests_completed=0,
        limitations=['기존팔레트캐시가nonzero인현재월드의전체생성자/표시목록/particle150초기화를원본몸체로실행한다.','힙과ProcessForm부착은명시관찰경계이며현재부모/form필드효과를공급한다.','기존표시목록초기입력은공통RawRenderRegistry와같은명시토큰배열이다.','첫팔레트cache0의색표생성과후속프레임/표시/삭제는완료로세지않는다.','C++초안의전체source/test출처를현재SHA로기록하지만공유C++대조성공을이보고서만으로주장하지않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def verify():
    """독립자료의전체SHA·행수·고유생성자입력과assert0을 검사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    # 현재실제근거의전체바이트를각각확인한다.
    for relative,expected in report['files'].items():
        if digest(ROOT/relative)!=expected:raise RuntimeError('Burning SHA 오류 '+relative)
    if len(rows)!=report['total']:raise RuntimeError('Burning 행수 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len({tuple(row[1:7]) for row in selected})!=len(selected) or len(selected)!=item['returns'] or item['assertions']:raise RuntimeError('Burning 고유입력/반환 오류')
    print(json.dumps(dict(verified=True,total=len(rows),cpp_connected=False,cold_palette_completed=False),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
