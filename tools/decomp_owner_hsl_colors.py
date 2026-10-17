#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제43b5a0/43b750의 다섯 전용 소유자 색표를 전체 ColorSort 입력으로 관측한다."""
import argparse
import csv
import hashlib
import json
import socket
import struct
import sys
from pathlib import Path
from decomp_gumpbackground_oracle import BackgroundOracle, ROOT, CONTROLS, palette_inputs, LOGICAL, SORT

# 실제43be30/43bc40의 대상과 소스 색 목록이다. 출력 번호는 원본 함수가 결정한다.
SURFACE=(44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,3,177,200,203,224,220,221,222,223,114,201,226)
SPECS=((111,0.0,(86,188,85,247,84,31,34,75,82,74,83,156,153,70,72,71,73,152,13,144)),
       (141,-0.15000000596046448,(87,88,85,86,84,156,75,82,153,74,73,83,155,72,71,44,152,154,70,13,144)),
       (137,0.0,(218,219,220,52,53,54,55,56,57,58,59,66,161,210,19,21,214,141,18)),
       (94,0.0,SURFACE),(97,None,SURFACE))
# 기존 큰 읽기/쓰기 영역 안의 전용2304바이트 결과와 두 원본 진입점이다.
MAP=SORT+0x6000
ENTRIES=(0x43b5a0,0x43b750)
BOOT_NAMES={"__setdefaultprecision","__controlfp","__control87","__abstract_cw","__hw_cw"}  # 실제 CRT 기본 정밀도의 전체 호출 사슬이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/owner-hsl-colors-x86.tsv'
REPORT=ROOT/'cpppj/owner-hsl-colors.json'
RANGES=ROOT/'extracted/refined/originals/functions.tsv'


def sha(path):
    """줄 끝을 변환하지 않고 실제 입력 파일의 바이트를 고정한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def generate():
    """할당/항등 초기화는 명시 입력이며 실제 두 전용 변환 몸체를 실행한다."""
    oracle=BackgroundOracle('originals');ranges=[];boot={}
    with RANGES.open(encoding='utf-8') as stream:
        for row in csv.DictReader(stream,delimiter='	'):
            if int(row['entry'],16) not in ENTRIES and row['name'] not in BOOT_NAMES:continue
            if row['name'] in BOOT_NAMES:boot[row['name']]=int(row['entry'],16)
            for part in row['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));ranges.append([low,high+1]);oracle.allowed_bytes.update(range(low,high+1))
            oracle.entries.add(int(row['entry'],16))
    if len(ranges)<2:raise RuntimeError('소유자 HSL 전체 원본 범위 누락')
    oracle.write_ranges.append((MAP,MAP+9*256));rows=[];differences=[]
    # 원본 CRT가64비트 입력 제어값을 실제로53비트로 바꾸는지 별도로 실행한다.
    from decomp_owner_oracle import STACK, STOP
    from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_EBX, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI
    if set(boot)!=BOOT_NAMES:raise RuntimeError('CRT 기본 정밀도 전체 범위 누락')
    preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_EBP:0x23456789,UC_X86_REG_ESI:0x34567890,UC_X86_REG_EDI:0x45678901}  # CRT 탐측도 실제 cdecl 보존 레지스터를 검사한다.
    for register,value in preserved.items():oracle.mu.reg_write(register,value)
    oracle.mu.mem_write(STACK,struct.pack('<I',STOP));oracle.mu.reg_write(UC_X86_REG_ESP,STACK);oracle.mu.reg_write(UC_X86_REG_FPCW,CONTROLS[1]);oracle.mu.reg_write(UC_X86_REG_FPSW,0);oracle.mu.emu_start(boot['__setdefaultprecision'],STOP,count=100000)
    default_control=oracle.mu.reg_read(UC_X86_REG_FPCW)
    if oracle.mu.reg_read(UC_X86_REG_EIP)!=STOP or oracle.mu.reg_read(UC_X86_REG_ESP)!=STACK+4 or (default_control&0x300)!=0x200 or (default_control&0xc00) or (oracle.mu.reg_read(UC_X86_REG_FPSW)&0x3800) or any(oracle.mu.reg_read(r)!=v for r,v in preserved.items()):raise RuntimeError(f'실제 CRT 기본 정밀도 반환 오류 CW={default_control:x} EIP={oracle.mu.reg_read(UC_X86_REG_EIP):x} ESP={oracle.mu.reg_read(UC_X86_REG_ESP):x} STACK={STACK:x} controls={CONTROLS}')
    oracle.returns+=1
    # 세 실제/혼합/중복 입력마다 ColorSort 전체 생성자를 두 번 실행한다.
    for palette,logical in enumerate(palette_inputs()):
        outputs=[]
        for control in CONTROLS:
            oracle.mu.mem_write(LOGICAL,struct.pack('<256I',*logical));oracle.mu.mem_write(SORT,bytes(256*56));oracle.call(oracle.b['sort'],struct.pack('<I',LOGICAL),control,4,SORT);selected=[]
            for type,offset,sources in SPECS:
                oracle.mu.mem_write(MAP,bytes(range(256))*9);oracle.mu.mem_write(0x557940+type*4,struct.pack('<I',MAP))
                for source in sources:
                    entry=ENTRIES[1] if offset is None else ENTRIES[0];arguments=struct.pack('<II',type,source)+(b'' if offset is None else struct.pack('<d',offset));oracle.call(entry,arguments,control)
                selected.append(bytes(oracle.mu.mem_read(MAP,9*256)).hex())
            outputs.append(selected)
        for spec,first,second in zip(SPECS,outputs[0],outputs[1]):
            if first!=second:differences.append([palette,spec[0]])
            rows.append([str(palette),str(spec[0]),struct.pack('<256I',*logical).hex(),first,second])
        print('소유자 HSL 원본 팔레트',palette,'통과',flush=True)
    FIXTURE.write_text('# 실제 ColorSort/43b5a0/43b750의53/64비트 전용 색표 관측.'+chr(10)+chr(10).join(chr(9).join(row) for row in rows)+chr(10),encoding='utf-8',newline=chr(10))
    paths={Path(__file__).resolve(),ROOT/'originals/Netstorm.exe',ROOT/'originals/d/GIFCLOUD.COL',FIXTURE,RANGES,*oracle.exports}
    for module in tuple(sys.modules.values()):
        path=getattr(module,'__file__',None)
        if path and Path(path).resolve().is_relative_to(ROOT/'tools'):paths.add(Path(path).resolve())
    report={'host':socket.gethostname(),'date':'2026-10-17','unit':135,'rows':len(rows),'controls':list(CONTROLS),'normalReturns':oracle.returns,'precisionDifferences':differences,'primaryControl':CONTROLS[0],'bootControlRaw':default_control,'bootPrecisionBits':default_control&0x300,'bootRoundingBits':default_control&0xc00,'bootEntries':boot,'instructions':oracle.instructions,'nativeCalls':dict(oracle.native_calls),'assertions':oracle.assertions,'boundaries':dict(oracle.boundaries),'extraRanges':ranges,'files':{p.relative_to(ROOT).as_posix():sha(p) for p in sorted(paths)},'limits':['10.78 전용5종의 각 소스 변환 몸체 전체 실행; 할당/항등9표는 명시 입력으로 기존43b950 복원을 재사용','CRT4e81da/4f2a7b/4f2a49/변환 두 함수는 실제 반환/스택/보존GP/정밀도53/nearest/빈x87스택을 별도 확인; Unicorn rawCW23f의 예약bit6 차이를27f와 같다고 주장하지 않음','43be30/43bc40 부모의 등록/해제/ThemeMode 수명은 새 CPU 실행 아님','ColorSort/스택/보존 레지스터/x87 균형/쓰기 감사는 기존 실행기를 유지; 응답/게임/장치 없음']}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+chr(10),encoding='utf-8',newline=chr(10))


def verify():
    """전용 소스 입력과 모든 출력 폭/실제 함수 진입·반환·해시를 확인한다."""
    report=json.loads(REPORT.read_text('utf-8'));rows=[line.split('	') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
    for path,wanted in report['files'].items():
        if sha(ROOT/path)!=wanted:raise RuntimeError('소유자 HSL SHA 불일치: '+path)
    expected=7+6*sum(len(item[2]) for item in SPECS)
    if len(rows)!=15 or report['rows']!=15 or report['normalReturns']!=expected or report['controls']!=list(CONTROLS) or report['primaryControl']!=CONTROLS[0] or report['assertions'] or report['boundaries']:raise RuntimeError('소유자 HSL 계수 오류')
    for index,row in enumerate(rows):
        palette=index//5;spec=SPECS[index%5]
        if row[:3]!=[str(palette),str(spec[0]),struct.pack('<256I',*palette_inputs()[palette]).hex()] or len(row)!=5 or any(len(bytes.fromhex(value))!=9*256 for value in row[3:]):raise RuntimeError('소유자 HSL 입력/출력 오류')
    differences=[[int(row[0]),int(row[1])] for row in rows if row[3]!=row[4]]
    if differences!=report['precisionDifferences'] or report['bootPrecisionBits']!=0x200 or report['bootRoundingBits'] or (report['bootControlRaw']&0x300)!=0x200:raise RuntimeError('소유자 HSL 정밀도 기록 오류')
    print('소유자 HSL 근거 감사:',len(rows),'행/',report['normalReturns'],'정상 반환')


# 명시 요청에서만 새 메모리 관측 또는 저장 근거 감사를 수행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
