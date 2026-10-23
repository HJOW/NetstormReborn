#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""기존 전체439de0 실행기로 raw0x3800·미작성padding·쓰기순서·두 x87 정밀도를 제품 전에 고정한다."""
import argparse
import collections
import csv
import hashlib
import json
from pathlib import Path
import random
import socket
import struct
import sys

from decomp_gumpbackground_oracle import BackgroundOracle, ROOT, LOGICAL, SORT, palette_inputs
from decomp_owner_oracle import STACK, STOP
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EBP,UC_X86_REG_ECX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG)

# 최초 fixture/보고서는 기존6관측 및 캐시130자료와 분리한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/colorsort-primary-x86.tsv'
REPORT=ROOT/'cpppj/colorsort-primary-native.json'
# 전체 생성자와 실제56BYTE×256의 물리 크기다.
ENTRY=0x439de0
SIZE=0x3800
# 원문 제어어 두 값을 각각 보존하며037f 출력은 제품53비트 기대에 섞지 않는다.
CONTROLS=(0x027f,0x037f)


def digest(path):
    """입력/최초 근거의 줄끝까지 포함하는 원시 SHA다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def inputs():
    """기존 실제/혼합/회색 입력과 HLS 모든 분기를 고르는 RGB만 준비한다."""
    result=[('existing'+str(index),struct.pack('<256I',*logical)) for index,logical in enumerate(palette_inputs())]
    # 주색·동률 최대·회색·양쪽 명도와 음수 hue를 같은256색 입력에 섞는다.
    colors=[(0,0,0),(255,255,255),(255,0,0),(0,255,0),(0,0,255),(255,255,0),(0,255,255),(255,0,255),
            (64,128,192),(192,64,128),(128,192,64),(128,128,64),(64,128,128),(128,64,128),
            (127,128,129),(129,128,127),(1,2,255),(255,2,1),(128,128,128),(1,1,1)]
    result.append(('branches',bytes(channel for index in range(256) for channel in (*colors[index%len(colors)],(index*37)&255))))
    result.append(('white',bytes((255,255,255,0xa5))*256))
    generator=random.Random(0x439de0)  # 출력과 무관한 결정적 난수 입력 생성기다.
    result.append(('mixed2',bytes(generator.randrange(256) for _ in range(1024))))
    return result


class PrimaryOracle(BackgroundOracle):
    """기존 PE·주소·ABI 감사 장치를 재사용하고 이번에는439de0 범위만 실행한다."""
    def __init__(self):
        """기존 원문을 바꾸지 않고 이 생성자의 정확한 비연속 범위를 고른다."""
        super().__init__('originals')
        allowed=[]  # 현재 내보낸 생성자 전체의 실제 범위다.
        with self.exports[1].open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):  # 다른 maps/Gump 몸체는 이번 실행에 허용하지 않는다.
                if int(row['entry'],16)!=ENTRY:
                    continue
                for part in row['ranges'].split(';'):  # 생성자 정렬 padding 바깥의 실제 몸체만 허용한다.
                    low,high=(int(value,16) for value in part.split('-'))
                    allowed.append((low,high+1))
        if not allowed:
            raise RuntimeError('기존 실행기의 실제439de0 범위 없음')
        self.allowed_bytes={address for low,high in allowed for address in range(low,high)}
        self.entries={ENTRY}
        self.write_ranges=[(SORT,SORT+SIZE)]
        self.active=False  # 호스트 입력 준비의 mem_write를 원문 쓰기로 세지 않는다.
        self.trace=[]  # 실행 중 완료되는 원문 쓰기 순서다.
        self.stop_after=-1  # 음수는 정상 반환까지 실행한다.
        self.stopped=False  # 다음 명령 전 명시 관측 중단 여부다.

    def on_instruction(self,mu,address,size,data):
        """지정 개수의 실제 쓰기가 끝난 다음 명령 직전에 중단하거나 전체 원문을 감사한다."""
        if self.active and self.stop_after>=0 and len(self.trace)>=self.stop_after:
            self.stopped=True
            mu.emu_stop()
            return
        self.instructions+=1
        if address==self.spec['assert_report']:
            self.assertions+=1
            raise RuntimeError('BYTE RGB 유효 입력에서 원문 진단에 도달')
        if address not in self.allowed_bytes or address+size-1 not in self.allowed_bytes:
            raise RuntimeError(f'생성자 범위 밖 실행 {address:08x}')
        if address==ENTRY:
            self.native_calls[f'{address:08x}']+=1

    def on_write(self,mu,access,address,size,value,data):
        """기존 물리 쓰기 감사를 유지하고 실제 raw 기록 store만 순서대로 관측한다."""
        super().on_write(mu,access,address,size,value,data)
        if self.active and SORT<=address and address+size<=SORT+SIZE:
            raw=(value&((1<<(size*8))-1)).to_bytes(size,'little')  # 실제 store가 전달한 비트이며 기대 계산이 아니다.
            self.trace.append(f'{address-SORT}:{size}:{raw.hex()}')

    def run_primary(self,logical,alias,initial,control,stop_after):
        """비영 미정 입력·실제 alias와 독립 x87 초기상태에서 한 생성자 호출을 관측한다."""
        self.active=False
        self.mu.mem_write(SORT,initial)
        pointer=LOGICAL if alias<0 else SORT+alias  # 실제 입력 pointer와 this가 겹치는 경우도 물리 그대로 둔다.
        self.mu.mem_write(pointer,logical)
        before=bytes(self.mu.mem_read(SORT,SIZE))  # alias 입력을 올린 뒤의 실제 초기0x3800이다.
        self.mu.mem_write(STACK-0x800,bytes(0x800))
        self.mu.mem_write(STACK,struct.pack('<II',STOP,pointer))
        saved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_EBP:0x23456789,UC_X86_REG_ESI:0x34567890,UC_X86_REG_EDI:0x45678901}  # thiscall 보존 GP 입력이다.
        for register,value in saved.items():  # 원문 ABI 입력의 실제 레지스터를 준비한다.
            self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_EAX,0x6789abcd)
        self.mu.reg_write(UC_X86_REG_ECX,SORT)
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.reg_write(UC_X86_REG_FPTAG,0xffff)
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control:
            raise RuntimeError('요청한027f/037f의 실제 초기 제어어 불일치')
        self.instructions=0
        self.trace=[]
        self.stopped=False
        self.stop_after=stop_after
        self.active=True
        self.mu.emu_start(ENTRY,STOP,count=1000000)
        self.active=False
        returned=self.mu.reg_read(UC_X86_REG_EIP)==STOP  # 명시 중단을 정상 반환으로 세지 않는다.
        end_control=self.mu.reg_read(UC_X86_REG_FPCW)
        status=self.mu.reg_read(UC_X86_REG_FPSW)
        if returned:
            if self.stopped or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+8 or self.mu.reg_read(UC_X86_REG_EAX)!=SORT:
                raise RuntimeError('생성자 반환 this/ret4/중단 상태 불일치')
            if any(self.mu.reg_read(register)!=value for register,value in saved.items()):
                raise RuntimeError('생성자 보존 GP 불일치')
            if end_control!=control or status&0x3800:
                raise RuntimeError('정상 반환의 x87 제어/스택 불일치')
        elif not self.stopped or len(self.trace)!=stop_after:
            raise RuntimeError('예상하지 않은 생성자 부분 중단')
        output=bytes(self.mu.mem_read(SORT,SIZE))  # 실제 미작성padding도 포함한 원문 전체 출력이다.
        if returned and any(output[index*56+52:index*56+56]!=before[index*56+52:index*56+56] for index in range(256)):
            raise RuntimeError('원문 미작성4BYTE padding 변경')
        after_logical=bytes(self.mu.mem_read(pointer,1024))  # alias일 때 원문 쓰기가 입력을 바꾼 부분도 보존한다.
        if alias<0 and after_logical!=logical:
            raise RuntimeError('외부 읽기 입력 변경')
        return before,output,after_logical,returned,end_control,status,self.mu.reg_read(UC_X86_REG_EAX),';'.join(self.trace),self.instructions


def generate():
    """제품을 읽지 않고 최초 raw 레코드·쓰기 순서·명시 부분효과를 PE에서만 생성한다."""
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('최초 생성자 fixture/report 덮어쓰기 금지')
    palettes=inputs()  # 기존 세 입력과 추가 분기용 입력만 고른다.
    rows=[]  # 전체 원문 출력/부분출력의 독립 관측이다.
    totals=collections.Counter()  # 정상/중단과 실제 명령 수를 구별한다.
    normal={}  # 두 제어어 필드 비트 차이를 계산 없이 원문끼리 비교한다.
    oracle=PrimaryOracle()  # 기존 전체 원문 실행기의 물리/범위 장치를 재사용한다.
    cases=[]  # 정상 입력과 부분 중단 요청이다.
    for name,logical in palettes:  # 모든 팔레트에서 외부/두 alias를 관측한다.
        for alias in (-1,0,24):
            cases.append((name,logical,alias,-1))
    for name,logical in (palettes[0],palettes[3]):  # 기존 실제색과 HLS 분기색의 명시 부분효과를 기록한다.
        for stop in (0,1,2,3,4,1023,1024,1025,1026,1791,1792):
            cases.append((name,logical,0,stop))
    for ordinal,(name,logical,alias,stop_after) in enumerate(cases):  # 모든 요청은 같은 두 실제 제어어에서 독립 실행한다.
        initial=bytes((index*73+ordinal*19+0xa5)&255 for index in range(SIZE))  # 매 입력의 비영 padding/이전 할당 BYTE다.
        for control in CONTROLS:
            before,output,after_logical,returned,end_control,status,result,trace,instructions=oracle.run_primary(logical,alias,initial,control,stop_after)
            label=f'{name}:alias{alias}:stop{stop_after}'  # 두 정밀도의 같은 입력을 찾는 식별자다.
            rows.append([label,f'{control:04x}',str(alias),str(stop_after),logical.hex(),before.hex(),output.hex(),after_logical.hex(),str(int(returned)),f'{result:08x}',f'{end_control:04x}',f'{status:04x}',trace,str(instructions)])
            totals['returned' if returned else 'stopped']+=1
            totals['instructions']+=instructions
            if returned:
                normal[(label,control)]=output
        print('ColorSort primary 원문 입력',ordinal+1,'/',len(cases),'관측',flush=True)
    differences=collections.Counter()  # 기록 필드별53/64 결과 차이를 분리한다.
    examples=[]  # 첫 차이의 실제 비트를 인수인계한다.
    for label,control in normal:  # 같은 물리 입력의 정상 결과 두 개만 비교한다.
        if control!=CONTROLS[0]:
            continue
        first,second=normal[(label,CONTROLS[0])],normal[(label,CONTROLS[1])]
        for index in range(256):  # 모든 원문 항목의6double/index/padding을 별도로 대조한다.
            for field,offset,width in (('hue',0,8),('lightness',8,8),('saturation',16,8),('red',24,8),('green',32,8),('blue',40,8),('index',48,4),('padding',52,4)):
                a,b=first[index*56+offset:index*56+offset+width],second[index*56+offset:index*56+offset+width]  # raw 비트 비교이며 제품 수학은 없다.
                if a!=b:
                    differences[field]+=1
                    if len(examples)<16:
                        examples.append({'case':label,'index':index,'field':field,'027f':a.hex(),'037f':b.hex()})
    FIXTURE.write_text('# 실제439de0 전체0x3800: id control alias stop logical before output logicalAfter returned eax endControl status writes instructions\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths={Path(__file__).resolve(),ROOT/'originals/Netstorm.exe',ROOT/'originals/d/GIFCLOUD.COL',*oracle.exports,FIXTURE}  # 기존 실행기와 보호 입력도 원시 SHA로 고정한다.
    for module in tuple(sys.modules.values()):  # 실제 재사용한 도구 소스만 기존 파일로 기록한다.
        name=getattr(module,'__file__',None)
        if name and Path(name).resolve().is_relative_to(ROOT/'tools'):
            paths.add(Path(name).resolve())
    report={'host':socket.gethostname(),'date':'2026-10-24','entry':'00439de0','rows':len(rows),'controls':list(CONTROLS),'normalReturns':totals['returned'],'explicitStops':totals['stopped'],'instructions':totals['instructions'],
            'precisionFieldDifferences':dict(differences),'firstDifferenceExamples':examples,'rawBytes':SIZE,'recordBytes':56,'unwrittenPaddingOffset':52,'unwrittenPaddingBytes':4,'nativeCalls':dict(oracle.native_calls),'assertions':oracle.assertions,
            'scope':'기존 실제439de0 전체만 실행; raw 레코드/alias/비영padding/완료된 store 뒤 명시중단/027f와037f를 구별한다. 게임/할당부모/Client/Screen/검색객체/전체MSVCRT는 실행하지 않는다.',
            'partialBoundary':'중단은 N개 raw store가 완료된 다음 명령 전이다. 정상반환/GP·x87스택 계약은 returned 행만 검증한다.',
            'files':{path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({key:report[key] for key in ('rows','normalReturns','explicitStops','instructions','precisionFieldDifferences')},ensure_ascii=False))


def verify():
    """최초 원문 자료를 재생성하지 않고 SHA/폭/정밀도·명시 부분효과의 형식만 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))  # 최초 Native 보고서다.
    for name,wanted in report['files'].items():  # 제품 소스 변경으로 기대값을 바꾸지 않는다.
        if digest(ROOT/name)!=wanted:
            raise RuntimeError('ColorSort primary 최초 SHA 불일치: '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    counts=collections.Counter()  # 정상/중단 행 수를 원시 fixture에서 다시 센다.
    for row in rows:  # 모든 입력·출력 물리 폭과 요청된 store 경계를 확인한다.
        if len(row)!=14 or int(row[1],16) not in CONTROLS or len(bytes.fromhex(row[4]))!=1024 or any(len(bytes.fromhex(row[index]))!=SIZE for index in (5,6)) or len(bytes.fromhex(row[7]))!=1024:
            raise RuntimeError('ColorSort primary fixture 물리 폭 오류')
        counts['returned' if row[8]=='1' else 'stopped']+=1
        if row[8]=='0' and len(row[12].split(';') if row[12] else [])!=int(row[3]):
            raise RuntimeError('ColorSort primary 명시 중단 store 수 오류')
    if len(rows)!=report['rows'] or counts['returned']!=report['normalReturns'] or counts['stopped']!=report['explicitStops'] or report['controls']!=list(CONTROLS) or report['assertions']:
        raise RuntimeError('ColorSort primary 원문 계수 오류')
    print('ColorSort primary 최초 감사:',len(rows),'행',dict(counts))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)  # 명시 generate/verify 진입만 제공한다.
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()  # 기존 최초 자료의 덮어쓰기는 generate가 거부한다.
    verify() if args.verify else generate()
