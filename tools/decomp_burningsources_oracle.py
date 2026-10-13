#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Burning 현재HP/고정source 행선택·난수/폴백/제거를 두 실제 PE로 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from decomp_burningprocess_oracle import BurningOracle,OBJECT
from decomp_owner_oracle import ROOT,TYPES,POOL,STACK,STOP,digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 실제source 몸체/게임난수/최대HP 옵션과 같은 타입 전역의 주소다.
SPEC={
 'originals':dict(source=0x424110,random=0x532710,weak=0x5ca8f0,mana=0x5412d0,checking=0x5e4794),
 'originalCD':dict(source=0x4a5d30,random=0x5308ec,weak=0x511c90,mana=0x51cbbc,checking=0x540a24),
}
# 격리된 현재입자/캡처마스크/행선택표는 같은 객체 메모리와 겹치지 않는다.
PARTICLES,IMAGE,ROWS=0x11350000,0x11360000,0x11361000
KEYS=('width','height','hp','maximum','kind','selected','mask','seed','capacity','pattern','weak','sid')
FIXTURE=ROOT/'cpppj/tests/fixtures/burningsources-x86.tsv'
REPORT=ROOT/'cpppj/recovery-burningsources-evidence.json'


def sample(**kwargs):
    """현재캡처/피해/입자 입력이며 결과 행수나 성공조건을 계산하지 않는다."""
    case=dict(zip(KEYS,(16,12,25,100,0,0,0,0,150,0,0,50)));case.update(kwargs);return case


def inputs():
    """작은마스크/opaque와300회실패·HP/guy·기존source 제거·capacity와정수난수를 교차한다."""
    rows=[]
    # 실제목표행 선택과 고정source의 생성/제거를 교차한다.
    for dims,hp,kind,selected,mask,seed in itertools.product(((2,2),(3,3),(8,8),(16,12)),(0,25,50),(0,1),(0,3),(0,1),(0,1,0x12345678)):
        rows.append(sample(width=dims[0],height=dims[1],hp=hp,kind=kind,selected=selected,mask=mask,seed=seed,pattern=1 if selected else 0))
    # 실제입자 부족에도 rowTaken/selected가 바뀌는 본체와 위크마나/odd최대HP를 포함한다.
    for capacity,pattern,hp,weak in itertools.product((0,5,15),(0,2),(0,25,50),(0,1)):
        rows.append(sample(capacity=capacity,pattern=pattern,hp=hp,weak=weak,kind=2,maximum=101,seed=1))
    rows.append(sample(sid=0));rows.append(sample(sid=65535))
    return list({tuple(case[key] for key in KEYS):case for case in rows}.values())


def old_particles(case):
    """원본24바이트 입자와 this+74/null 표식만 현재 입력으로 공급한다."""
    values=[]
    # 패턴1의 기존 세source 행과 패턴2의 가득찬 양수입자를 각각 명시한다.
    for index in range(case['capacity']):
        source=case['pattern']==1 and index<3
        points=((2,3),(7,3),(0,0)) if case['width']>=8 and case['height']>=8 else ((0,0),(0,0),(0,0))
        x,y=points[index] if source else (10+index,20+index)
        values.append((float(x),float(y),0.0,0.0,9999.0 if source else 1.25 if case['pattern']==2 else 0.0,OBJECT+0x74 if source else 0))
    return values


class BurningSourcesOracle(BurningOracle):
    """현재source/HP/난수/입자몸체만 실행하며 heap/부착/캡처를 대체하지 않는다."""
    def __init__(self,edition):
        """기존18/19와Sources8/7·별도CDflag/위크마나 helper의 실제 범위를 합친다."""
        super().__init__(edition);self.s=SPEC[edition];self.sourceReturns=0;self.sourceAssertions=0
        names=['burning-sources-helper']
        if edition=='originalCD':names+=['burning-sources-flag-helper','burning-sources-maxhp-helper']
        # 기존8/7 내보내기 바이트를 덮지 않고 추가함수의 별도 metadata를 읽는다.
        for name in names:
            paths=[ROOT/f'extracted/{name}/{edition}/{part}' for part in ('creation.c','functions.tsv')];self.exports.extend(paths)
            with paths[1].open(encoding='utf-8') as fp:
                # 허용 범위는 각 실제 body의 불연속 구간뿐이다.
                for row in csv.DictReader(fp,delimiter='\t'):
                    self.entries.add(int(row['entry'],16))
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def on_instruction(self,mu,address,size,data):
        """원본assert를 별도 관찰하고 이단위에 없는 생성/캡처/힙 효과를 거부한다."""
        if address==self.spec['assert_report']:
            self.sourceAssertions+=1;mu.emu_stop();return
        if address in (self.p['new'],self.p['free'],self.p['attach']):raise RuntimeError('Burning source 미허용외부생성 호출')
        # 생성자경계 callback을 사용하지 않으며 원본 body와 현재함수도달만 기록한다.
        from decomp_owner_oracle import OwnerOracle
        OwnerOracle.on_instruction(self,mu,address,size,data)

    def prepare(self,case):
        """현재HP/타입/캡처/rowTaken/입자/게임난수만 입력으로 설정한다."""
        self.case=case;self.assertStart=self.sourceAssertions;mu=self.mu;s=self.s;patch=self.edition=='originals';mu.mem_write(POOL,bytes(128*self.stride));mu.mem_write(TYPES,bytes(0x20000));mu.mem_write(OBJECT,bytes(144));mu.mem_write(PARTICLES,bytes(150*24));mu.mem_write(ROWS,bytes(0x1000));mu.mem_write(0,bytes(4));mu.mem_write(s['random'],struct.pack('<I',case['seed']));mu.mem_write(s['weak'],struct.pack('<I',case['weak']));mu.mem_write(s['mana'],struct.pack('<I',158));mu.mem_write(s['checking'],bytes(4))
        typeid=158 if case['kind']==2 else 70 if case['kind']==1 else 89;typ=TYPES+typeid*self.type_stride;mu.mem_write(typ,struct.pack('<i',case['maximum']));mu.mem_write(typ+0xe8,struct.pack('<I',16));mu.mem_write(typ+0xec,struct.pack('<I',0x400 if case['kind']==1 else 0))
        raw=bytearray(self.stride);raw[10]=typeid;struct.pack_into('<i' if patch else '<h',raw,26,case['hp']);mu.mem_write(self.slot(50),bytes(raw));total=case['capacity'] if case['pattern']==2 else min(3,case['capacity']) if case['pattern']==1 else 0
        for offset,value in ((0x24,case['capacity']),(0x28,total),(0x2c,PARTICLES),(0x58,IMAGE),(0x5c,case['width']),(0x60,case['height']),(0x6c,case['sid']),(0x78,ROWS),(0x7c,case['selected'])):mu.mem_write(OBJECT+offset,struct.pack('<I',value))
        mu.mem_write(IMAGE,bytes([255 if case['mask'] else 0])*case['width']*case['height']);rows=bytearray((case['width']+case['height'])//5)
        # 실제source 좌표의 기존행 표식만 공급하며 다음제거행을 선택하지 않는다.
        for index,value in enumerate(old_particles(case)):
            mu.mem_write(PARTICLES+index*24,struct.pack('<5fI',*value))
            if value[5]:
                row=(int(value[0])+int(value[1]))//5
                if row<len(rows):rows[row]=1
        mu.mem_write(ROWS,bytes(rows));self.write_ranges=[(0,4),(OBJECT,OBJECT+144),(PARTICLES,PARTICLES+case['capacity']*24),(ROWS,ROWS+len(rows)),(s['random'],s['random']+4)]

    def run(self,case):
        """정상RET/x87 또는정확assert시점과 모든수명/source/rowTaken/난수 비트를 기록한다."""
        self.prepare(case);mu=self.mu
        # 같은 정상53비트 x87 환경이며 source의 반환EAX는호출자가사용하지않는다.
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(reg,0)
        mu.reg_write(UC_X86_REG_ECX,OBJECT);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,0x027f);mu.reg_write(UC_X86_REG_FPSW,0);mu.mem_write(STACK,struct.pack('<I',STOP));mu.emu_start(self.s['source'],STOP,count=2000000);asserted=self.sourceAssertions!=self.assertStart
        if not asserted:
            if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4 or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('Burning source RET/스택/x87 오류')
            self.sourceReturns+=1
        particles=bytearray(mu.mem_read(PARTICLES,case['capacity']*24))
        # 실제 this+74/null의 두 source 포인터만0/1 의미로 정규화한다.
        for index in range(case['capacity']):
            pointer=struct.unpack_from('<I',particles,index*24+20)[0]
            if pointer not in (0,OBJECT+0x74):raise RuntimeError('Burning source 임의포인터 출력')
            struct.pack_into('<I',particles,index*24+20,int(pointer!=0))
        return ['assert' if asserted else 'return',str(self.read(OBJECT+0x7c)),str(self.read(OBJECT+0x28)),bytes(mu.mem_read(ROWS,(case['width']+case['height'])//5)).hex() or '-',particles.hex() or '-',str(self.read(self.s['random']))]


def generate(smoke):
    """기존820격자와분리한 새로운source 입력/실제body출력을 저장한다."""
    rows=[];editions={};files={ROOT/'tools/decomp_burningsources_oracle.py',ROOT/'tools/decomp_burningprocess_oracle.py',ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/ghidra/burning-sources-helper-functions.json',ROOT/'tools/ghidra/burning-sources-flag-helper-functions.json',ROOT/'tools/ghidra/burning-sources-maxhp-helper-functions.json'}
    for edition in SPEC:
        oracle=BurningSourcesOracle(edition);cases=inputs();cases=[sample(),sample(selected=3,pattern=1)] if smoke else cases
        # 각현재입력은 각원본에서 한 번씩만 관찰한다.
        for case in cases:
            try:result=oracle.run(case)
            except Exception as error:raise RuntimeError(f'{edition} {case}: {error}') from error
            rows.append([edition,*[case[key] for key in KEYS],*result])
        editions[edition]=dict(returns=oracle.sourceReturns,assertions=oracle.sourceAssertions,instructions=oracle.instructions,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.boundaries));files.update(oracle.exports);files.add(ROOT/oracle.spec['binary']);print(edition,editions[edition],flush=True)
    if smoke:return
    for path in ('cpppj/src/o/RawBurningSources.h','cpppj/src/o/RawBurningSources.cpp','cpppj/tests/BurningSourcesTests.cpp','cpppj/src/o/RawBurningVisual.h','cpppj/src/o/RawBurningVisual.cpp','cpppj/src/o/SpStore.h','cpppj/src/o/SpStore.cpp','cpppj/recovery-burningparticle-evidence.json','cpppj/recovery-burningprocess-constructor-evidence.json'):files.add(ROOT/path)
    FIXTURE.write_text('# 실제 Burning source행 선택/HP/게임정수난수/기존source 제거. 캡처는현재픽셀입력, 외부관찰경계0.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8');files.add(FIXTURE)
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',primary_target='10.78',cd_target='10.72',total=len(rows),editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},cpp_tests_completed=0,os_calls=0,limitations=['새source몸체/현재HP/최대HP옵션/게임정수난수/Append/행제거는실제코드다.','현재캡처마스크/행표식/기존입자는명시입력이며 실제프레임캡처를새완료로세지않는다.','source반환EAX는원본caller가무시하므로뜻없는포인터/이전레지스터값을C++반환값으로복원하지않는다.','진단전역0/유한좌표/양수최대HP/정상마스크와구분한SID0/65535반환을포함한다.','전체Step/Draw/첫cache0색표/효과factory/실제Host삭제와캠페인월드는별도후속이다.'],),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def verify():
    """현재SHA와새고유입력·정상/assert/외부경계의분류를감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    for path,expected in report['files'].items():
        if digest(ROOT/path)!=expected:raise RuntimeError('Burning source SHA 불일치 '+path)
    if len(rows)!=report['total'] or report['os_calls']:raise RuntimeError('Burning source 사례/OS 경계 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len({tuple(row[1:13]) for row in selected})!=len(selected) or sum(row[13]=='return' for row in selected)!=item['returns'] or sum(row[13]=='assert' for row in selected)!=item['assertions'] or item['boundaries']:raise RuntimeError('Burning source 고유/반환/경계 오류')
    print(json.dumps(dict(verified=True,total=len(rows),cpp_completed=False,boundaries=0),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
