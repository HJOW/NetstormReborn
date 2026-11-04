#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Vortex의 진단 복귀/현재 Player·WORD·목록을 실제 PATCH 명령으로 격리 관측한다."""
import argparse
import collections
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
import zlib
from decomp_assetpostpop_oracle import AssetOracle, KEYS, TARGET, LIST_BASE, SPEC
from decomp_owner_oracle import OwnerOracle, ROOT, PLAYERS, TYPES, SPECS, CAPACITY, STOP, STACK_BASE, Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, pefile
from decomp_file_name_leaf import add_ranges
from decomp_basefile_whole_read import BoundaryFailure
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

BRIDGE=0x5411a0  # WORD setter가 현재 raw TYPE와 비교하는 전체 DWORD다.


def inputs():
    """기존 자산 입력 형식에 owner/섬 존재/진단 복귀/변경/다리 입력만 더한다."""
    base=dict(zip(KEYS,(9,1,0,0,0,0,7,0,25,0,1000,0,0,0)))
    result=[]
    for kind,flags,extra,owner,exists,resume in itertools.product((9,10,11),(0,1,2,3),(0,1,8,9),(0,1,8),(0,1),(0,1)):  # 진단 관문과 미도달 입력을 교차한다.
        case=base.copy();case.update(kind=kind,flags=flags,extra=extra)
        result.append((case,(owner,exists,resume,0,82)))
    for kind,flags,extra,resume,mutation,bridge in itertools.product((9,10,11),(1,2,3),(0,8),(0,1),range(1,6),(70,82)):  # 진단/음향 효과 뒤 현재 값을 다음 원문이 다시 읽게 한다.
        case=base.copy();case.update(kind=kind,flags=flags,extra=extra)
        result.append((case,(0,0,resume,mutation,bridge)))
    return result


class VortexOracle(AssetOracle):
    """실제 세 wrapper/공통 부모·WORD setter·중복 목록을 실행하고 외부 장치만 명시한다."""
    def __init__(self):
        """현재 정밀 함수 범위만 추가하며 게임/OS/통신으로 진입하지 않는다."""
        self.edition='originals';self.spec=SPECS[self.edition];binary=ROOT/self.spec['binary'];self.sha256=hashlib.sha256(binary.read_bytes()).hexdigest()
        pe=pefile.PE(str(binary));self.base=pe.OPTIONAL_HEADER.ImageBase;image=pe.get_memory_mapped_image();self.mu=Uc(UC_ARCH_X86,UC_MODE_32)
        self.mu.mem_map(self.base,(len(image)+4095)&~4095);self.mu.mem_write(self.base,image)
        self.stride,self.type_stride,self.player_stride=(self.spec[name] for name in ('stride','type_stride','player_stride'));self.o,self.g=self.spec['offsets'],self.spec['globals']
        for address,size in ((0,0x1000),(TYPES,0x20000),(PLAYERS,0x20000),(0x12000000,(CAPACITY*self.stride+4095)&~4095),(STOP,0x1000),(STACK_BASE,0x40000)):  # 실제 PE와 겹치지 않는 분석 전용 물리다.
            self.mu.mem_map(address,size)
        self.exports=[ROOT/'extracted/refined/originals/Netstorm.c',ROOT/'extracted/refined/originals/functions.tsv'];self.allowed=[];self.entries=set();self.p=SPEC[self.edition]
        add_ranges(self,(0x4dce40,0x4dd020,0x4dd1f0,0x4dc700,0x414450,0x4ac220,0x414490,0x40eb40,0x49a840))
        for name,value in (('types',TYPES),('pool',0x12000000),('players',PLAYERS)):  # 원문의 세 장기 포인터만 분석 물리에 연결한다.
            self.mu.mem_write(self.g[name],struct.pack('<I',value))
        for kind in (9,10,11):  # 선택할 세 원문 VT+20을 실제 PE 바이트와 대조한다.
            assert self.read(self.p['tables'][kind]+0x20)==self.p['entries'][kind]
        self.native_calls=collections.Counter();self.assertions=0;self.instructions=0;self.write_ranges=[];self.events=[];self.stub_calls=collections.Counter();self.returns=0
        self.mu.hook_add(UC_HOOK_CODE,self.on_instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.on_write)

    def string(self,address):
        """진단의 원문 ASCII를 NUL까지 읽는다."""
        result=bytearray()
        for offset in range(512):  # 손상 문자열의 무제한 읽기를 거부한다.
            value=self.mu.mem_read(address+offset,1)[0]
            if not value: return result.hex()
            result.append(value)
        raise RuntimeError('Vortex 원문 진단 문자열 길이 오류')

    def mutate(self,mutation):
        """명시 효과 입력만 쓰며 원문의 이후 분기/쓰기 결과는 계산하지 않는다."""
        raw=self.slot(TARGET);mu=self.mu;p=self.p
        if mutation==1:
            mu.mem_write(raw+34,b'\x08');mu.mem_write(raw+11,b'\x09');mu.mem_write(raw+40,b'\x09')
            mu.mem_write(raw+12,struct.pack('<H',0x4567));mu.mem_write(raw+36,struct.pack('<I',0x6789));mu.mem_write(p['load'],struct.pack('<I',1));mu.mem_write(BRIDGE,struct.pack('<I',75));mu.mem_write(raw+10,b'\x4b')
        elif mutation==2:
            mu.mem_write(raw+34,b'\0');mu.mem_write(p['local'],struct.pack('<I',8));mu.mem_write(BRIDGE,struct.pack('<I',70));mu.mem_write(raw+12,struct.pack('<H',0x7777))
        elif mutation in (3,4,5):
            mu.mem_write(raw+10,b'\x4b');mu.mem_write(raw+11,b'\x09');mu.mem_write(raw+40,b'\0');mu.mem_write(raw+34,bytes([8 if mutation in (4,5) else 1]));mu.mem_write(p['load'],bytes(4))
            if mutation==4:
                mu.mem_write(p['selected'],struct.pack('<I',62));mu.mem_write(p['local'],struct.pack('<I',8))
            if mutation==5: mu.mem_write(BRIDGE,struct.pack('<I',75))

    def on_instruction(self,mu,address,size,data):
        """현재 진단/외부 섬 장치에서만 멈추고 나머지는 기존 실제 원문을 실행한다."""
        p=self.p
        if address==p['entries'][self.case['kind']]:
            mu.mem_write(self.slot(TARGET)+34,bytes([self.control[0]]));mu.mem_write(BRIDGE,struct.pack('<I',self.control[4]));mu.reg_write(UC_X86_REG_FPCW,self.precision)
            mu.mem_write(TYPES+75*self.type_stride,bytes(self.type_stride));mu.mem_write(TYPES+75*self.type_stride+0xec,struct.pack('<I',0x200));mu.mem_write(TYPES+75*self.type_stride+0x74,struct.pack('<I',17));mu.mem_write(TYPES+75*self.type_stride+0x130,struct.pack('<II',27,39))
        if address==p['exists']:
            sp=mu.reg_read(UC_X86_REG_ESP);mu.reg_write(UC_X86_REG_EAX,self.control[1]);mu.reg_write(UC_X86_REG_ESP,sp+8);mu.reg_write(UC_X86_REG_EIP,self.read(sp));return
        if address==0x4e0620:
            sp=mu.reg_read(UC_X86_REG_ESP);expression,file,line=(self.read(sp+4+4*i) for i in range(3))
            self.events.append('X:'+self.string(expression)+':'+self.string(file)+':'+str(line))
            if self.control[3] in (1,2,3,4): self.mutate(self.control[3])
            if not self.control[2]: raise BoundaryFailure('assertion')
            mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,self.read(sp));return
        if address==p['element'] and self.control[3]==5: self.mutate(5)
        if address==0x414450:  # 중복 목록을 기존 외부 장치로 대체하지 않고 실제 signed 관문/검색/쓰기를 실행한다.
            OwnerOracle.on_instruction(self,mu,address,size,data);return
        super().on_instruction(mu,address,size,data)

    def observe(self,case,control,precision):
        """정상/명시 진단 중단 모두에서 전체 raw와9 Player·목록·전역을 수집한다."""
        self.control,self.precision=control,precision;outcome='return'
        try: self.run_body(case)
        except BoundaryFailure as error: outcome=str(error)
        raw=bytearray(self.mu.mem_read(self.slot(TARGET),self.stride));struct.pack_into('<I',raw,0,self.p['tables'][case['kind']])
        lists='|'.join(str(self.read(self.p[name]+8))+':'+','.join(str(self.read(LIST_BASE+i*0x100+4*j)) for j in range(4)) for i,name in enumerate(('residence','temples','geysers')))
        records=bytes(self.mu.mem_read(PLAYERS,9*self.player_stride));fields=','.join(str(self.read(PLAYERS+owner*self.player_stride+offset)) for owner in (0,1,8) for offset in (0x7c,0x80,0x84,0x88))
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision
        return (';'.join(self.events),raw.hex(),lists,str(self.read(self.p['count'])),str(zlib.adler32(records)),fields,str(self.read(self.p['load'])),str(self.read(self.p['selected'])),str(self.read(self.p['local'])),str(self.read(BRIDGE)),outcome)


def main():
    """서로 다른 두 x87 정밀도 결과를 비교하고 최초 관측을 별도 경로에 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    assert not args.output.exists() and not args.report.exists()
    oracles=[VortexOracle(),VortexOracle()];rows=[]
    for case,control in inputs():  # 기대값은 C++ 구현이 아닌 두 실제 원문 실행에서만 읽는다.
        outputs=[oracle.observe(case,control,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))]
        assert outputs[0]==outputs[1],(case,control)
        rows.append((','.join(str(case[key]) for key in KEYS),','.join(map(str,control)),*outputs[0]))
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text('# input14 control5 events raw50 lists count playersAdler fields12 load selected local bridge outcome\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    report=dict(host=socket.gethostname(),entries=['004dce40','004dd020','004dd1f0','004dc700','00414450','004ac220'],cases=len(rows),observations=2*len(rows),normalReturns=sum(row[-1]=='return' for row in rows),assertionStops=sum(row[-1]=='assertion' for row in rows),diagnosticInputs=sum('X:' in row[2] for row in rows),tsvSha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),boundaries=['외부 Damageable/음향·섬/현재 소유자·내용물·예약/진단 장치는 명시 입력이다','WORD setter/중복 목록과 세 wrapper/공통 부모는 실제 원문이다','실제 게임/Frame·Process 틱/공격·의식·MP/진단 UI 실행 없음'])
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':
    main()
