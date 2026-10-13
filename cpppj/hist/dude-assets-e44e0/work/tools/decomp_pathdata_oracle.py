#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 두 PE의 실제 Path WriteData·공통 헤더·genus 조회를 격리 x86에서 실행한다. 게임/GUI 실행은 없다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from decomp_pathruntime_oracle import PathOracle, SPEC as PATH_SPEC, PATH, PARENT, ROOT, ARENA, POOL, TYPES, STACK, STOP, digest, bits, dbits
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 실제 보호 PE의 가상 WriteData·공통 헤더 전역·로그 경계다.
SPEC={
 'originals':dict(write=0x4895b0,header=(0x52edce,0x52edcf,0x544c50),log=0x4c2630,tail=0x186e4),
 'originalCD':dict(write=0x47d630,header=(0x50ff68,0x50ff6c,0x50ff70),log=0x48ca70,tail=0x2f1c),
}
PACKET=ARENA+0x58000 # 실제 공유 버퍼를 모사하는 격리 메모리이며 원본 파일에는 기록하지 않는다.
PACKET_SIZE=2048 # Host가 허용하는 최대 송신 크기보다1바이트 큰 관찰 버퍼다.
FIXTURE=ROOT/'cpppj/tests/fixtures/pathdata-x86.tsv' # 원본 관찰 결과를 담는 UTF-8 독립 fixture다.
REPORT=ROOT/'cpppj/pathdata.json' # 코드·원본·범위·관찰 SHA 및 실행 경계 기록이다.
SOURCES=['tools/decomp_pathdata_oracle.py','tools/decomp_pathruntime_oracle.py','tools/decomp_reward_oracle.py',
 'tools/ghidra/pathdata-functions.json','tools/ghidra/pathdatahelpers-functions.json',
 'cpppj/src/o/RawPathData.h','cpppj/src/o/RawPathData.cpp','cpppj/tests/RawPathDataTests.cpp']


def path_inputs(profile):
    """방향 결과를 계산하지 않는 독립 꼭짓점·방향 입력이다. 실제 정수 절삭/분할은 PE가 한다."""
    cases=[([(10,20)],[0]), ([(10,20),(10,20),(10,20)],[0,7,2]),
      ([(10,20),(11,20)],[0,0]), ([(10,20),(41,20)],[0,0]),
      ([(10,20),(42,20)],[0,0]), ([(10,20),(73,20)],[0,0]),
      ([(10,20),(100,110)],[0,1]), ([(10,20),(10,40),(80,40),(20,100)],[0,2,4,6]),
      ([(10.9,20.1),(10.01,20.9),(11.9,20.1)],[0,1,8]),
      ([(-1.9,-2.1),(-32.9,20.9),(255.9,260.1)],[0,7,15]),
      ([(255.9,256.9),(1.9,-1.9)],[0,-1]), ([(10,20),(10,80),(30,80)],[0,3,0])]
    return cases[profile]


def inputs():
    """직접/압축 genus·헤더의7비트/최소길이·stale buffer·시간 양자화 경계를 교차한다."""
    result=[]
    # 각 프로필에서32개의 독립 raw 입력을 생성한다. parentChanged/force는 실제 가상 인자지만 본체는 읽지 않는다.
    for profile,genus,variant,ignored in itertools.product(range(12),(0,0x20000,0x100000,0x120000),range(2),range(4)):
        time=(-0.1251,0.124999999,0.125,123456.123)[(profile+ignored)%4]
        result.append((profile,genus,variant,ignored,dbits(time)))
    return result


class PathDataOracle(PathOracle):
    """이미 검증한 PE 로더/Path ctor만 재사용하고 직렬화 본체의 실행 범위와 쓰기 전역을 추가한다."""
    def __init__(self,edition):
        """실제 헤더/field/coordinate code와 진단 로그 관찰 경계를 구분한다."""
        super().__init__(edition);self.d=SPEC[edition]
        # 두 정밀 export에 있는 모든 범위를 검토 목록에 더하되 실제 진입 횟수만 관찰에 센다.
        for name in ('pathdata','pathdatahelpers'):
            with (ROOT/f'extracted/{name}/{edition}/functions.tsv').open(encoding='utf-8') as stream:
                for row in csv.DictReader(stream,delimiter='\t'):
                    self.entry_names[int(row['entry'],16)]=row['entry']
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.log_hits=0

    def on_instruction(self,mu,address,size,data):
        """실제 출력은 PE가 계산한다. 진단 로그는 가변 인자를 관찰하고 OS 파일 입출력만 생략한다."""
        if address==self.d['log']:
            self.log_hits+=1;self.stub_hits['diagnostic_log']+=1
            sp=mu.reg_read(UC_X86_REG_ESP);mu.reg_write(UC_X86_REG_EIP,self.u32(sp));mu.reg_write(UC_X86_REG_ESP,sp+4);return
        return super().on_instruction(mu,address,size,data)

    def call_write(self,control,ignored):
        """실제 ret12 가상 WriteData를 실행하고 정상 반환/스택·x87 스택을 검사한다."""
        # 레지스터를 매 호출 독립 입력으로 초기화한다. 가상 인자2/3은 변경하되 원본 계산은 대체하지 않는다.
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0)
        self.mu.reg_write(UC_X86_REG_ECX,PATH);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.mem_write(STACK,struct.pack('<4I',STOP,PACKET,ignored&1,ignored>>1))
        self.mu.emu_start(self.d['write'],STOP,timeout=30000000,count=2000000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+16
        assert self.mu.reg_read(UC_X86_REG_FPSW)&0x3800==0
        return self.mu.reg_read(UC_X86_REG_EAX)

    def setup(self,item):
        """기대 패킷 없이 원본 raw 경로·버퍼·현재 전역 입력만 준비한다."""
        profile,genus,variant,ignored,time=item;points,directions=path_inputs(profile)
        self.reset(priest=profile&1);self.put(PATH+4,32700+profile);self.path(points,directions,[0]*len(points));self.put(PATH+self.m['times'],time,8)
        self.put(PATH+self.m['dead'],0);self.put(PATH+(0x186d4 if self.patch else 0x2f14),0x12340000+profile*65537)
        self.put(PATH+self.d['tail'],0x87654321+profile);self.put(PATH+self.d['tail']+4,0xa5+profile)
        type_address=TYPES+(158 if profile&1 else 134)*self.s['type_stride'];self.put(type_address+0xec,genus)
        # 네트워크 헤더 현재 전역값과 stale 버퍼 채움 입력은 실제 body가 언제 덮는지 관찰한다.
        header=(0x30+variant,0x81 if variant else 0xfe,0 if variant else 0x8123)
        for address,value,width in zip(self.d['header'],header,(1,1,2)):self.put(address,value,width)
        self.mu.mem_write(PACKET,bytes([0xa5 if variant else 0x5a])*PACKET_SIZE)
        return points,directions,header


def generate():
    """두 x87 정밀도에서 같은 입력의 실제 반환 길이·전체 공유 버퍼 보존 범위·raw 입력 불변을 기록한다."""
    rows=[];observations={};exports=[]
    # 원본과 CD를 별도로 실행해 판본별 배열/헤더 차이를 보존한다.
    for edition in SPEC:
        oracle=PathDataOracle(edition)
        # 정밀도 반복은 입력 대조 수에 더하지 않는다.
        for case,item in enumerate(inputs()):
            outputs=[]
            for control in (0x027f,0x037f):
                points,directions,header=oracle.setup(item);before=bytes(oracle.mu.mem_read(PATH,oracle.m['size']))
                length=oracle.call_write(control,item[3]);packet=bytes(oracle.mu.mem_read(PACKET,PACKET_SIZE))
                assert before==bytes(oracle.mu.mem_read(PATH,oracle.m['size']))
                outputs.append((length,packet))
            assert outputs[0]==outputs[1],(edition,case,'x87 정밀도 불일치')
            length,packet=outputs[0];profile,genus,variant,ignored,time=item
            rows.append([edition,profile,genus,variant,ignored,time,length,packet.hex()])
        observations[edition]=dict(cases=len(inputs()),hits=dict(oracle.hits),stubs=dict(oracle.stub_hits),log_hits=oracle.log_hits,assertions=0,normal_returns=True,x87_precision_equal=True)
        # 실행 범위를 결정한 정밀 원문과 표를 함께 보존한다.
        for name in ('pathdata','pathdatahelpers','pathruntime','pathruntimehelpers','pathmath'):
            for filename in ('creation.c','functions.tsv'):exports.append(f'extracted/{name}/{edition}/{filename}')
        print(edition,len(inputs()),'개 정상 반환')
    FIXTURE.parent.mkdir(parents=True,exist_ok=True)
    with FIXTURE.open('w',encoding='utf-8',newline='') as stream:
        stream.write('# 두 보호 PE의 실제 Path WriteData 독립 관찰. x87 정밀도 반복을 입력 수에 더하지 않는다.\n')
        csv.writer(stream,delimiter='\t',lineterminator='\n').writerows(rows)
    report=dict(scope='Path WriteData·공통 헤더·payload/genus getter 실제 몸체; 서버 Host 연결 및 사제 명령 전체 완료는 별도',cases=len(rows),observations=observations,
      last_decompile_host='VM-W11-CODEX',last_decompile_date='2026-10-13',game_gui_runs=0,boundaries=['할당: 기존 Path ctor의 격리 메모리','진단 로그: 호출만 관찰'],
      sources={p:digest(ROOT/p) for p in SOURCES},exports={p:digest(ROOT/p) for p in exports},fixture=digest(FIXTURE),
      protected_pe={p:digest(ROOT/p) for p in ('originals/Netstorm.exe','originalCD/NETSTORM.EXE')})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def verify():
    """최종 fixture·소스·보호 PE·실행 범위 SHA와 실제 entry 반환 수를 다시 확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));assert report['cases']==768
    for group in ('sources','exports','protected_pe'):
        for path,sha in report[group].items():assert digest(ROOT/path)==sha,path
    assert digest(FIXTURE)==report['fixture']
    for edition,observation in report['observations'].items():
        assert observation['cases']==384 and observation['hits'][f'{SPEC[edition]["write"]:08x}']==768
        assert observation['normal_returns'] and observation['x87_precision_equal'] and observation['assertions']==0
    print('Path native 768개·SHA·정상 반환·두 x87 정밀도 동일 확인')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate()
