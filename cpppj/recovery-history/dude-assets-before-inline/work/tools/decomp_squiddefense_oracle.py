#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 기본 가상 방어5바이트와 캠페인 소비자 가상 표를 보호 PE에서 독립 관찰한다."""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle,ROOT,STACK,STOP,digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 두 보호 원본의 실제 기본 메서드와 base/71/128/158/160 현재 가상 표다.
SPEC={
 'originals':dict(entry=0x43d9b0,tables=(0x501dd0,0x50b888,0x50b9b8,0x50f210,0x509758)),
 'originalCD':dict(entry=0x4aed90,tables=(0x5058d8,0x5014f8,0x501120,0x5003e0,0x503b30)),
}
FIXTURE=ROOT/'cpppj/tests/fixtures/squiddefense-x86.tsv'
REPORT=ROOT/'cpppj/recovery-squiddefense-evidence.json'


def observe(edition):
    """명시5바이트만 허용해실제 RET4/반환/전체raw불변을 검사한다."""
    oracle=OwnerOracle(edition);p=SPEC[edition];body=bytes(oracle.mu.mem_read(p['entry'],5)).hex()
    if body!='33c0c20400':raise RuntimeError('기본 방어 실제5바이트 불일치')
    oracle.allowed=[(p['entry'],p['entry']+5)];oracle.entries={p['entry']};rows=[];catalog=[]
    for table in p['tables']:
        address=struct.unpack('<I',oracle.mu.mem_read(table+0x9c,4))[0]
        if address!=p['entry']:raise RuntimeError('캠페인 현재+9c 가상 표 불일치')
        catalog.append(dict(vtable=f'{table:08x}',address=f'{address:08x}'))
    # 인자에의존하지않는원본을 유효 victim/자유로운 공격SID 입력으로 직접 방문한다.
    for sid,attacker in itertools.product((5,50,70,127),(0,1,50,70,127,65535)):
        raw=bytes((i*11+sid)&255 for i in range(oracle.stride));oracle.mu.mem_write(oracle.slot(sid),raw);oracle.mu.reg_write(UC_X86_REG_EAX,0xffffffff);oracle.mu.reg_write(UC_X86_REG_ECX,oracle.slot(sid));oracle.mu.reg_write(UC_X86_REG_ESP,STACK);oracle.mu.mem_write(STACK,struct.pack('<II',STOP,attacker));oracle.mu.emu_start(p['entry'],STOP,count=3)
        if oracle.mu.reg_read(UC_X86_REG_EIP)!=STOP or oracle.mu.reg_read(UC_X86_REG_ESP)!=STACK+8:raise RuntimeError('방어 실제 RET4/스택 오류')
        rows.append([edition,sid,attacker,oracle.mu.reg_read(UC_X86_REG_EAX),bytes(oracle.mu.mem_read(oracle.slot(sid),oracle.stride)).hex()])
    return rows,dict(body=body,catalog=catalog,returns=len(rows),native_calls=dict(oracle.native_calls),instructions=oracle.instructions,assertions=oracle.assertions),ROOT/oracle.spec['binary']


def generate():
    """두 PE의 고유 반환과 실제 가상 표/현재 소비자 SHA를 저장한다."""
    rows=[];editions={};files={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',FIXTURE,ROOT/'cpppj/src/o/RawSquidDefense.cpp',ROOT/'cpppj/src/o/RawSquidDefense.h',ROOT/'cpppj/tests/SquidDefenseTests.cpp',ROOT/'cpppj/src/o/DerivedConstructors.inc',ROOT/'cpppj/src/o/TypeConstructors.inc'}
    for edition in SPEC:
        selected,report,binary=observe(edition);rows.extend(selected);editions[edition]=report;files.add(binary)
    FIXTURE.write_text('# 실제 두 PE 기본가상+9c 몸체5바이트. current-vtable 분배와 다른파생거부는별도C++consumer다.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8');REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',total=len(rows),editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},os_calls=0,limitations=['5바이트 기본 false 몸체만 복원했으며 다른 파생 방어는 strict fallback이다.','기존 일반 피해/사제/08 보고서와 원본 파일을 변경하지 않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(total=len(rows)),ensure_ascii=False))


def verify():
    """현재 PE/소스/fixture SHA와 실제5바이트/가상 표/고유 반환을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r for r in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if r and not r[0].startswith('#')]
    if len(rows)!=report['total']:raise RuntimeError('방어 원본 관찰 행수 불일치')
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('방어 SHA 불일치 '+name)
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition]
        if len({tuple(r[1:3]) for r in selected})!=item['returns'] or item['body']!='33c0c20400' or item['assertions']:raise RuntimeError('방어5바이트/고유 반환 오류')
    print(json.dumps(dict(verified=True,total=len(rows)),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
