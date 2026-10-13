#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Burning의 실제24바이트 입자추가와 만료칸 선택을 두 PE의 독립 격자로 관찰한다."""
import argparse
import csv
import itertools
import json
import struct
from decomp_burningprocess_oracle import BurningOracle,ROOT,OBJECT,HEAP,STACK,STOP,digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 실제6필드쓰기와 공통Particle.New의 판본별 진입점이다.
ENTRY={'originals':0x423e80,'originalCD':0x4a60a0}
KEYS=('capacity','total','lifes','life','source')
FIXTURE=ROOT/'cpppj/tests/fixtures/burningparticle-x86.tsv'
REPORT=ROOT/'cpppj/recovery-burningparticle-evidence.json'


def bits(value):
    """기대값 계산없이 float32 실제 입력 비트를 만든다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def inputs():
    """용량/현재수와 ordered zero/negative/NaN·signed DWORD 재사용 차이를 포함한다."""
    rows=[]
    # 뜻이 같은 프로파일이 아니라 실제입력 메모리가 다른 경우만 남긴다.
    for capacity in (0,1,3):
        patterns=[tuple([bits(1.25)]*capacity)]
        if capacity:
            for value,index in itertools.product((0,bits(-1),0x7fc00000,0xffc00000),(0,capacity-1)):
                values=[bits(1.25)]*capacity;values[index]=value;patterns.append(tuple(values))
        for total,lifes,life,source in itertools.product(range(capacity+1),dict.fromkeys(patterns),(bits(0.5),bits(-2),0x7fc00000),(0,1)):
            rows.append(dict(capacity=capacity,total=total,lifes=lifes,life=life,source=source))
    return rows


class ParticleOracle(BurningOracle):
    """실제추가몸체는 외부경계호출없이 실행하며 old입자와 currentcount 전체를관찰한다."""
    def run_particle(self,case):
        """현재24바이트 입력만공급하고 첫여유/만료칸 계산은원본명령에맡긴다."""
        mu=self.mu;self.events=[];raw=bytearray(144);struct.pack_into('<III',raw,36,case['capacity'],case['total'],HEAP);mu.mem_write(OBJECT,bytes(raw));initial=bytearray(case['capacity']*24)
        # 각기존입자의숫자/표식은명시입력이며 추가결과를미리대입하지않는다.
        for index,life in enumerate(case['lifes']):struct.pack_into('<6I',initial,index*24,bits(10.5+index),bits(20+index),bits(index-1),bits(index+1),life,OBJECT+116 if index&1 else 0)
        mu.mem_write(HEAP,bytes(initial) if initial else bytes(24));self.write_ranges=[(OBJECT,OBJECT+144),(HEAP,HEAP+len(initial))]
        # 독립53비트 x87/레지스터/정확한thiscall6인자를매입력공급한다.
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(reg,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,0x027f);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_ECX,OBJECT);mu.reg_write(UC_X86_REG_ESP,STACK);mu.mem_write(STACK,struct.pack('<7I',STOP,bits(100.5),bits(50.75),bits(-0.25),bits(1.5),case['life'],OBJECT+116 if case['source'] else 0));mu.emu_start(ENTRY[self.edition],STOP,count=10000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+28 or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('Burning 입자 실제RET/스택/x87 오류')
        value=mu.reg_read(UC_X86_REG_EAX);current=bytearray(mu.mem_read(HEAP,len(initial)));self.returns+=1
        # source표식은실제this+74 또는null만허용하고호스트 bool로정규화한다.
        for index in range(case['capacity']):
            source=struct.unpack_from('<I',current,index*24+20)[0]
            if source not in (0,OBJECT+116):raise RuntimeError('Burning source표식범위 오류')
            struct.pack_into('<I',current,index*24+20,int(source!=0))
        return [str(int(value!=0)),str(self.read(OBJECT+40)),bytes(current).hex() or '-']


def generate(smoke):
    """그대로반환한실제입자/숫자출력과고유입력만저장한다."""
    cases=inputs();cases=cases[:1] if smoke else cases;rows=[];editions={};files={ROOT/'tools/decomp_burningparticle_oracle.py',ROOT/'tools/decomp_burningprocess_oracle.py',ROOT/'tools/decomp_owner_oracle.py',ROOT/'cpppj/src/o/RawBurningVisual.cpp',ROOT/'cpppj/src/o/RawBurningVisual.h',ROOT/'cpppj/tests/BurningParticleTests.cpp',ROOT/'cpppj/recovery-burningprocess-constructor-evidence.json'}
    for edition in ENTRY:
        oracle=ParticleOracle(edition)
        # 각판본고유메모리입력은원본몸체를한번씩정상반환까지실행한다.
        for case in cases:
            result=oracle.run_particle(case);rows.append([edition,case['capacity'],case['total'],','.join(map(str,case['lifes'])) or '-',case['life'],case['source'],*result])
        editions[edition]=dict(returns=oracle.returns,instructions=oracle.instructions,assertions=oracle.assertions,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.boundaries));files.update(oracle.exports);files.add(ROOT/oracle.spec['binary']);print(edition,oracle.returns,flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제Burning6필드추가/여유칸과첫만료칸선택. particle source는this+74/null의명시입력이다.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8');files.add(FIXTURE)
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',total=len(rows),editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},os_calls=0,
        limitations=['원본추가함수와공통Particle.New를외부경계없이실행한다. 원본생성자256과이새입자격자를구분한다.','입력particle수명/좌표/속도/source표식은명시입력이다. source는this+74또는null만관찰한다.','실제Sprite캡처/연소점선택/frame/화면표시/전체효과수명은이격자의완료가아니다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def verify():
    """전체SHA/고유입력/정상반환과실제추가몸체에외부경계가없음을감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    for relative,sha in report['files'].items():
        if digest(ROOT/relative)!=sha:raise RuntimeError('Burning 입자SHA 오류 '+relative)
    if len(rows)!=report['total']:raise RuntimeError('Burning 입자행수 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len({tuple(row[1:6]) for row in selected})!=len(selected) or len(selected)!=item['returns'] or item['assertions'] or item['boundaries']:raise RuntimeError('Burning 입자고유입력/반환/경계 오류')
    print(json.dumps(dict(verified=True,total=len(rows),external_boundaries=0),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
