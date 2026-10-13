#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE Unit.cpp의 압축 총 시각/직접 피해/전달 몸체를 독립 관찰한다. OS/게임 실행 없음."""
import argparse
import collections
import csv
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle, ROOT, TYPES, POOL, STACK, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,
    UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 원본 몸체/원본 전역과 명시 자료 query 경계다.
SPEC={
 'originals':dict(entries=[0x4d4da0,0x4d4e10,0x4d4f00,0x4d4eb0,0x4d5000],dps=0x4d4d40,group=0x4ac950,genus=0x4ac200,type=0x4ac550,rate=0x49acf0,now=0x55b4d0,notify=0x4214a0),
 'originalCD':dict(entries=[0x443160,0x443200,0x4432f0,0x4432a0,0x4433e0],dps=None,group=0x4ac420,genus=0x4abae0,type=0x4abf50,rate=None,now=0x5484a8,notify=0x448c10),
}
# 외부 피해/내보낸 double 결과/합성 CD DPS 가상 callback 저장소다.
VTABLE,OUTPUT,VALUE,CD_DPS,DAMAGE=0x11318000,0x11319000,0x1131a000,STOP+0x100,STOP+0x120
KEYS=('kind','word','now','dps','target','targetstate','special','genus')
FIXTURE=ROOT/'cpppj/tests/fixtures/gunhit-x86.tsv'
REPORT=ROOT/'cpppj/gunhit.json'


def wide(value):
    """double 입력의 raw 비트다. 기대값을 계산하지 않는다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def single(value):
    """float DPS 입력의 raw 비트다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def inputs():
    """1/8초 경계/WORD 감김/30초 패치 제한/무효 표적/DPS 경계를 교차한다."""
    result=[]
    # 각 숫자 helper와 전체 Hit의 독립 경계를 골고루 방문한다.
    for kind in range(5):
        for word in (0,1,20,160,0xfffe,0xffff):
            for now in (0.0,0.124,0.125,10.5,31.25,4095.875,4096.0,4127.0):
                for dps in ((0,8,19.5) if kind==2 else (8,)):
                    for target,targetstate in ((60,0),(60,2),(0,0)):
                        result.append(dict(zip(KEYS,(kind,word,wide(now),single(dps),target,targetstate,0,0))))
    # 패치 실제 DPS 가상 getter의 flyer/balloon 별도값을 방문한다.
    for genus in (0,0x100000,0x20000,0x120000):
        for special in (0,1):
            for now in (0.125,10.5,31.25):result.append(dict(zip(KEYS,(2,1,wide(now),single(8),60,0,special,genus))))
    unique=[];seen=set()
    # 각 정상 입력의 재방문은 독립 관찰 누계에서 한 번만 센다.
    for case in result:
        key=tuple(case[name] for name in KEYS)
        if key not in seen:seen.add(key);unique.append(case)
    return unique


class GunHitOracle(OwnerOracle):
    """실제 time/Hit/Apply 몸체를 실행하며 피해/공간 통지 경계를 기록한다."""
    def __init__(self,edition):
        """내보낸 몸체와 실제 패치 DPS getter의 단일 PE float thunk를 확인한다."""
        super().__init__(edition);self.p=SPEC[edition]
        paths=[ROOT/f'extracted/gunhit/{edition}/{part}' for part in ('creation.c','functions.tsv')];self.exports.extend(paths)
        with paths[1].open(encoding='utf-8') as fp:
            # 내보낸 불연속 실제 함수만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.rate_bytes=None
        if self.p['rate']:
            self.rate_bytes=bytes(self.mu.mem_read(self.p['rate'],4)).hex()
            if self.rate_bytes!='d94148c3':raise RuntimeError('실제 DPS float getter 변화')
            self.allowed.append((self.p['rate'],self.p['rate']+4));self.entries.add(self.p['rate'])
        self.mu.mem_write(CD_DPS,b'\xd9\x05'+struct.pack('<I',VALUE)+b'\xc2\x04\x00');self.allowed.append((CD_DPS,CD_DPS+9));self.boundaries=collections.Counter();self.returns=0;self.events=[]

    def read(self,address,width=4):
        """실행기 메모리의 원본 필드 비트를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def on_instruction(self,mu,address,size,data):
        """원본 피해·공간 통지와 현재 type/group/genus 자료 query를 관찰한다."""
        p=self.p;esp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX);args=[self.read(esp+4+i*4) for i in range(3)];ret=self.read(esp);purge=0;value=0
        if address==p['group']:value=1
        elif address==p['genus']:value=self.case['genus'] if (this-POOL)//self.stride==60 else 0
        elif address==p['type']:value=TYPES+self.read(this+10,1)*self.type_stride
        elif address==p['notify']:self.events.append(f'N:{args[0]}')
        elif address==DAMAGE:self.events.append('D:60:'+':'.join(map(str,args)));purge=12
        else:super().on_instruction(mu,address,size,data);return
        self.boundaries[f'{address:08x}']+=1;mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run_body(self,case):
        """독립 압축 WORD/DPS/시각 입력에서 실제 raw 변경·피해·반환을 기록한다."""
        self.case=case;self.events=[];mu=self.mu;p=self.p;patch=self.edition=='originals';mu.mem_write(VTABLE,bytes(0x200));mu.mem_write(OUTPUT,bytes((i*7+3)&255 for i in range(8)))
        mu.mem_write(VTABLE+0xbc,struct.pack('<I',p['dps'] or CD_DPS));mu.mem_write(VTABLE+0xc0,struct.pack('<I',p['entries'][3]));mu.mem_write(VTABLE+0x100+0x98,struct.pack('<I',DAMAGE))
        for sid,typeid in ((50,128),(60,71)):
            raw=bytearray((i*11+5)&255 for i in range(self.stride));struct.pack_into('<I',raw,0,VTABLE if sid==50 else VTABLE+0x100);raw[10]=typeid;raw[11]=case['targetstate'] if sid==60 else 0;raw[34 if patch else 32]=1
            if sid==50:struct.pack_into('<H',raw,12,case['word'])
            mu.mem_write(self.slot(sid),bytes(raw));mu.mem_write(TYPES+typeid*self.type_stride,bytes(self.type_stride))
        source=TYPES+128*self.type_stride;mu.mem_write(source+0x48,struct.pack('<I',case['dps']));mu.mem_write(source+0x1b0,struct.pack('<I',case['special']));mu.mem_write(source+0x1b4,struct.pack('<f',16));mu.mem_write(p['now'],struct.pack('<Q',case['now']));mu.mem_write(VALUE,struct.pack('<I',case['dps']))
        self.write_ranges=[(self.slot(50)+12,self.slot(50)+14),(OUTPUT,OUTPUT+8)]
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(register,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,0x27f);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_ECX,self.slot(50));mu.reg_write(UC_X86_REG_ESP,STACK)
        nowargs=[case['now']&0xffffffff,case['now']>>32];args=[[OUTPUT],nowargs,[case['target']],[case['target'],17],[]][case['kind']];mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.emu_start(p['entries'][case['kind']],STOP,count=30000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+len(args)*4:raise RuntimeError('GunHit 반환/스택 오류')
        if mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('GunHit x87 스택 오류')
        self.returns+=1;return [';'.join(self.events),bytes(mu.mem_read(self.slot(50),self.stride)).hex(),bytes(mu.mem_read(OUTPUT,8)).hex()]


def generate():
    """실제 원본 관찰과 저장된 도구/PE/몸체/fixture의 SHA를 기록한다."""
    rows=[];editions={};files=[Path(__file__),FIXTURE,ROOT/'tools/ghidra/gunhit-functions.json']
    for edition in SPEC:
        oracle=GunHitOracle(edition)
        # 독립 입력 격자의 실제 body 정상 반환을 관찰한다.
        for case in inputs():
            try:output=oracle.run_body(case)
            except Exception as error:raise RuntimeError(f'{edition} {case}: {error}') from error
            rows.append([edition,*[case[key] for key in KEYS],*output])
        editions[edition]=dict(returns=oracle.returns,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.boundaries),rate_bytes=oracle.rate_bytes,assertions=oracle.assertions,instructions=oracle.instructions);files.extend(oracle.exports);files.append(ROOT/oracle.spec['binary'])
    FIXTURE.write_text('# 두 실제 PE Gun Hit·시간·피해 전달 관찰. 실제 피해 target+98과공간통지는명시 경계다.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8')
    REPORT.write_text(json.dumps(dict(schema=1,total=len(rows),host='HJOW-Athlon',editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in files},limitations=['현재 type/group/genus query·피해 target+98·공간 통지는 명시 경계다.','패치는 실제 DPS getter를 실행하며 CD DPS getter만 독립 float 반환 경계다.','실제 HP/원작 피해·사운드·삭제/월드 연결 완료를 이 격자로 세지 않는다.'],os_calls=0),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(total=len(rows),editions=editions),ensure_ascii=False))


def verify():
    """현재 SHA·고유 입력·정상 반환·실제 패치 DPS 몸체 실행을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    if len(rows)!=report['total']:raise RuntimeError('GunHit 행 수 오류')
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('GunHit SHA 불일치 '+name)
    for edition,data in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len(selected)!=data['returns'] or data['assertions']:raise RuntimeError('GunHit 정상 반환/assert 오류')
        if len({tuple(row[1:9]) for row in selected})!=len(selected):raise RuntimeError('GunHit 입력 중복')
    if not report['editions']['originals']['native_calls'].get('004d4d40'):raise RuntimeError('GunHit 패치 실제 DPS 실행 없음')
    print(json.dumps(dict(verified=True,total=len(rows)),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
