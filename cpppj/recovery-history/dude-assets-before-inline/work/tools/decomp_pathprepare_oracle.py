#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Path Prepare 가상+0x30의 override 판본 차이를 실제 두 PE에서 관찰한다. 게임/GUI 실행은 없다."""
import argparse
import csv
import itertools
import json
import struct
from decomp_pathdata_oracle import PathDataOracle, SPEC, SOURCES, ROOT, PATH, TYPES, STACK, STOP, digest, bits, dbits
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,
    UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 실제 Process 가상 Prepare이며 경로 배열의 하위 Prepare와 구분한다.
PREPARE={'originals':0x48a200,'originalCD':0x47e710}
FIXTURE=ROOT/'cpppj/tests/fixtures/pathprepare-x86.tsv' # 독립 원본 시각 배열 관찰이다.
REPORT=ROOT/'cpppj/recovery-pathprepare-evidence.json' # 실제 범위·소스·PE SHA와 반환/정밀도 근거다.
POINTS=[(10,20),(13.1,24.7),(15.2,19.8),(23.9,27.6)] # 원본 float 배열에 공급할 합성 입력 좌표다.
DIRECTIONS=[7,0,4,6] # 원본 가상 함수가 첫 방향을 직접 읽도록 준비한 입력이다.
SOURCE_PATHS=SOURCES+['tools/decomp_pathprepare_oracle.py','cpppj/src/o/RawPathProcess.cpp'] # 소스 감사 대상이다.


def invoke(oracle,override,type_address,control):
    """원본 가상 ret8 몸체를 직접 실행해 정상 반환과 x87 스택을 확인한다."""
    mu=oracle.mu
    # 레지스터는 독립 입력으로 초기화하되 시각/경로 계산을 대체하지 않는다.
    for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(reg,0)
    mu.reg_write(UC_X86_REG_ECX,PATH);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
    mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
    mu.mem_write(STACK,struct.pack('<3I',STOP,bits(override),type_address));mu.emu_start(PREPARE[oracle.binary],STOP,timeout=30000000,count=2000000)
    assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+12
    assert mu.reg_read(UC_X86_REG_FPSW)&0x3800==0


def generate():
    """동일한 override·타입속도·회전속도·현재시각 입력을 두 판본에서 별도로 관찰한다."""
    rows=[];observations={};precision_differences=[]
    # actual 가상 함수와 실제 하위 시각 함수의 모든 호출을 보존한다.
    for edition in PREPARE:
        oracle=PathDataOracle(edition)
        for override,speed,turning,now in itertools.product((0,2.5,-1.125),(0.7,1.8),(17.5,90),(-1,123.125)):
            results=[]
            # 53/64비트 정밀도 반복은 신규 입력 수에 추가하지 않는다.
            for control in (0x027f,0x037f):
                oracle.reset();oracle.path(POINTS,DIRECTIONS,[0]*4);oracle.put(oracle.m['now'],dbits(now),8)
                type_address=TYPES+134*oracle.s['type_stride'];oracle.put(type_address+0xe0,bits(speed));oracle.put(type_address+0xe4,bits(turning))
                invoke(oracle,override,type_address,control)
                results.append(tuple(struct.unpack('<Q',oracle.mu.mem_read(PATH+oracle.m['times']+i*8,8))[0] for i in range(4)))
            if results[0]!=results[1]:
                precision_differences.append(dict(edition=edition,override=bits(override),speed=bits(speed),turning=bits(turning),now=dbits(now),result53=results[0],result64=results[1]))
            rows.append([edition,bits(override),bits(speed),bits(turning),dbits(now),','.join(map(str,results[0]))])
        observations[edition]=dict(cases=24,hits=dict(oracle.hits),assertions=0,normal_returns=True)
        print(edition,'가상 Prepare24개 정상 반환')
    with FIXTURE.open('w',encoding='utf-8',newline='') as stream:
        stream.write('# 실제 Process 가상+0x30의 타입/override 판본 차이. 원본 게임 설정인 x87 53비트 결과이며64비트 차이는 보고서에 별도 보존한다.\n');csv.writer(stream,delimiter='\t',lineterminator='\n').writerows(rows)
    data_report=json.loads((ROOT/'cpppj/recovery-pathdata-evidence.json').read_text(encoding='utf-8'))
    report=dict(cases=len(rows),scope='실제 Process 가상 Prepare와 하위 Path 시각 함수; 명시 타입 포인터 입력',observations=observations,
      sources={path:digest(ROOT/path) for path in SOURCE_PATHS},exports=data_report['exports'],protected_pe=data_report['protected_pe'],fixture=digest(FIXTURE),
      last_decompile_host='VM-W11-CODEX',last_decompile_date='2026-10-13',game_gui_runs=0,fixture_control_word='0x027f',precision_differences=precision_differences)
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def verify():
    """독립 원본 관찰의 현재 SHA·고유입력 수·실제 가상 호출/정상 반환을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));assert report['cases']==48 and digest(FIXTURE)==report['fixture']
    for group in ('sources','exports','protected_pe'):
        for path,sha in report[group].items():assert digest(ROOT/path)==sha,path
    for edition,observation in report['observations'].items():
        assert observation['cases']==24 and observation['hits'][f'{PREPARE[edition]:08x}']==48
        assert observation['normal_returns'] and observation['assertions']==0
    assert report['fixture_control_word']=='0x027f'
    print(f'실제 가상 Prepare48개·SHA·정상 반환·53비트 결과 확인; 64비트 차이{len(report["precision_differences"])}개 별도 보존')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
