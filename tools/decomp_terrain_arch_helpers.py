#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 정밀 범위의 군도 공유 성장/제거만 격리 실행한다. 게임/MP/통신을 실행하지 않는다."""
import argparse
import collections
import csv
import itertools
import json
import socket
import struct
from pathlib import Path
import capstone
from decomp_terrain_seed import Oracle, ROOT, MAP, CHUNKS, STACK, STOP, digest, rle
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

SEED=MAP+0x21000  # 원본 로컬 난수 DWORD의 격리 주소이며 게임 전역 난수와 구별한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/terrain-arch-helpers-x86.tsv'  # 양판본의 전체 배열과 caller 시드 기대값이다.
HELPERS={  # 현행 전체 정밀 내보내기에서 직접 확인한 실제 진입주소다.
    'originals':dict(Grow=0x4c0800,Cell=0x4c0b30,Sparse=0x4c0db0,Clear=0x4c0fe0),
    'originalCD':dict(Grow=0x49d690,Cell=0x49da00,Sparse=0x49dbb0,Clear=0x49e030)}


def inputs():
    """출력을 계산하지 않고 BYTE 지역/비영 모드/직사각형/이웃 mask/빈 범위를 교차한다."""
    rows=[('Grow',(profile,mode,region,seed,target)) for profile,mode,region,seed,target in
          itertools.product(range(3),(0,0x80000000),(0,255),(0,7),(-1,1,31))]
    rows += [('Cell',(mask,threshold,no_region,code)) for mask,threshold in itertools.product(range(256),(-1,4,12))
             for no_region,code in ((127,120),)]
    rows += [('Cell',(255,12,255,code)) for code in (46,61,120)]
    rows += [('Sparse',(profile,no_region)) for profile,no_region in itertools.product(range(6),(127,255))]
    rows += [('Clear',(profile,no_region)) for profile,no_region in itertools.product(range(6),(127,255))]
    return rows


def setup_codes(profile):
    """입력 문자/지역만 만든다. 성장과 가중치/제거 결과는 계산하지 않는다."""
    codes=bytearray(b'.'*65536);regions=bytearray(b'\x09'*65536)
    for x in range(48,112):  # 넓은 입력 영역에 서로 다른 문자/지역을 놓는다.
        for y in range(48,112):  # 출력 순회와 독립인 고정 입력 수식이다.
            if profile==0:code=120 if (x,y)==(70,70) else 46
            elif profile==1:code=120 if (x*3+y)%7==0 else 61 if (x+y)%3==0 else 46
            elif profile==2:code=120
            elif profile==3:code=120 if x==70 or y==70 else 46
            elif profile==4:code=120 if (x//3+y//3)%2==0 else 46
            else:code=120 if (x+y)%2==0 else 46
            codes[x*256+y]=code;regions[x*256+y]=(x+y)%256
    return codes,regions


class ArchHelpersOracle(Oracle):
    """선행 시드 실행기의 보호 PE/진단/쓰기 관문에 현재 공유 helper의 실제 범위만 추가한다."""
    def __init__(self,edition):
        """직접 call의 실제 정밀 범위 closure를 선택하며 OS/진단 몸체는 실행하지 않는다."""
        super().__init__(edition)
        self.helpers=HELPERS[edition]
        with self.table.open(encoding='utf-8') as stream:
            rows={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);decoder.detail=True
        pending=list(self.helpers.values());selected={}
        while pending:  # 원본 몸체가 직접 호출하는 실제 helper만 재귀 선택한다.
            entry=pending.pop()
            if entry in selected or entry==self.s['assertion']:continue
            row=rows[entry];selected[entry]=row
            for part in row['ranges'].split(';'):  # 불연속 범위의 끝 명령도 포함한다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
                for instruction in decoder.disasm(bytes(self.mu.mem_read(low,high+1-low)),low):
                    if instruction.mnemonic=='call' and instruction.operands[0].type==capstone.x86.X86_OP_IMM:
                        target=instruction.operands[0].imm
                        if target!=self.s['assertion']:
                            if target not in rows:raise RuntimeError(f'정밀 call 범위 누락 {target:08x}')
                            pending.append(target)
        self.selected_helpers=list(selected.values())
        self.helper_entries=collections.Counter()
        self.helper_addresses=set(self.helpers.values())  # 각 명령의 진입 확인을 상수 시간으로 처리한다.
        self.allowed_bytes=bytearray(0x400000)  # 허용한 실제 코드 바이트만 1인 읽기 전용 실행 관문이다.
        for low,high in self.allowed:  # 선행/신규 불연속 범위를 동일하게 표시한다.
            self.allowed_bytes[low-0x400000:high-0x400000]=b'\x01'*(high-low)

    def instruction(self,mu,address,size,data):
        """새 helper 진입을 세고 기존 현재 진단/허용주소 관문을 적용한다."""
        if address in self.helper_addresses:self.helper_entries[f'{address:08x}']+=1
        if address==self.s['assertion']:super().instruction(mu,address,size,data);return
        assert 0x400000<=address<0x800000 and self.allowed_bytes[address-0x400000],(hex(address),self.case)

    def write(self,mu,access,address,size,value,data):
        """명시 caller 로컬 시드 쓰기만 추가 허용하고 나머지는 선행 보호 관문을 유지한다."""
        if address==SEED and size==4:return
        super().write(mu,access,address,size,value,data)

    def run_helper(self,kind,case,control):
        """같은 입력의 전체 배열/현재 DWORD/시드와 반환 규약을 실제 기계어에서 관찰한다."""
        self.case=(kind,case);self.mutation=0;mu=self.mu;mu.context_restore(self.context)
        mu.mem_write(MAP,bytes(0x22000));mu.mem_write(CHUNKS,bytes(4096));mu.mem_write(STACK-768,b'\xa5'*800)
        self.put(CHUNKS,1);self.put(CHUNKS+4,16);self.put(CHUNKS+8,16)
        self.put(MAP+self.s['pointer'],CHUNKS)
        for index,value in enumerate((64,64,96,80)):self.put(MAP+self.s['bounds']+index*4,value)
        profile=case[0] if kind!='Cell' else 0;codes,regions=setup_codes(profile)
        seed=0;mode=0;no_region=127;returned=False
        if kind=='Grow':
            profile,mode,region,seed,target=case
            for x in range(256):regions[x*256:(x+1)*256]=bytes((region,))*256  # 현재 seed 지역만 입력으로 맞춘다.
            if profile==1:bounds=(4,4,5,6)
            else:bounds=(4,4,6,5)
            arguments=(region,*bounds,target,SEED)
        elif kind=='Cell':
            mask,threshold,no_region,code=case;codes[70*256+70]=code
            directions=((0,-1),(1,-1),(1,0),(1,1),(0,1),(-1,1),(-1,0),(-1,-1))
            for direction,(dx,dy) in enumerate(directions):  # 이웃 mask는 원문 고정 방향의 입력이다.
                codes[(70+dx)*256+70+dy]=120 if mask&(1<<direction) else 46
            arguments=(70,70,threshold);returned=True
        elif kind=='Sparse':
            profile,no_region=case;arguments=()
        else:
            profile,no_region=case
            arguments=((4,4,6,5),(3,3,4,5),(6,5,4,4),(4,4,4,5),
                       (4,5,6,4),(0x10000004,4,0x10000005,5))[profile]
        code_address=MAP+self.s['code'];mu.mem_write(code_address,bytes(codes));mu.mem_write(code_address+65536,bytes(regions))
        self.put(MAP+self.s['bounds']-4,mode);self.put(self.s['no_region'],no_region);self.put(SEED,seed)
        rng=0x532710 if self.edition=='originals' else 0x5308ec;self.put(rng,0x12345678)
        chunks_before=bytes(mu.mem_read(CHUNKS,4096));self.getters=[];self.assertions=[];self.draws=[];self.cell_writes=0
        self.put(STACK,STOP)
        for index,value in enumerate(arguments):self.put(STACK+4+index*4,value)  # 원문은 thiscall이며 스택 인자의 정확한 폭을 지킨다.
        mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_ECX,MAP)
        saved={r:0x12345670+i for i,r in enumerate((UC_X86_REG_EBX,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI))}
        for register,value in saved.items():mu.reg_write(register,value)  # 비휘발 레지스터와 스택 canary를 확인한다.
        mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        mu.emu_start(self.helpers[kind],STOP,count=2000000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+4+len(arguments)*4,self.case
        assert all(mu.reg_read(r)==v for r,v in saved.items()) and not(mu.reg_read(UC_X86_REG_FPSW)&0x3800),self.case
        assert bytes(mu.mem_read(STACK-768,16))==b'\xa5'*16 and self.get(rng)==0x12345678,self.case
        assert bytes(mu.mem_read(CHUNKS,4096))==chunks_before and not self.assertions,self.case
        assert self.get(MAP+self.s['bounds']-4)==mode
        return (mu.reg_read(UC_X86_REG_EAX) if returned else 0,self.get(SEED),
                rle(bytes(mu.mem_read(code_address,65536))),rle(bytes(mu.mem_read(code_address+65536,65536))))


def main():
    """최초 기록과 읽기 전용 재관측을 구별하고 양판본/두 x87 결과를 각각 확인한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--report',required=True)
    args=parser.parse_args();rows=[];editions={}
    for edition in HELPERS:  # 같은 명시 입력을 보호 두 PE에서 따로 관찰한다.
        oracle=ArchHelpersOracle(edition)
        for kind,case in inputs():  # 두 정밀도 반복은 고유 입력 수에 합산하지 않는다.
            result=oracle.run_helper(kind,case,0x027f)
            assert result==oracle.run_helper(kind,case,0x037f),(edition,kind,case)
            rows.append((edition,kind,','.join(map(str,case)),*result))
        editions[edition]=dict(rows=len(inputs()),entries=dict(oracle.helper_entries),selected=oracle.selected_helpers,
                              pe_sha256=digest(oracle.pepath),ranges_sha256=digest(oracle.table))
        print(edition,len(inputs()),flush=True)
    output='# 실제 군도 공유 성장/성긴칸/플레이어 배치 제거: 전체 code/region RLE, caller seed.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n'
    if args.verify:assert FIXTURE.read_text(encoding='utf-8')==output,'군도 공유 helper 재관측 불일치'
    else:FIXTURE.write_text(output,encoding='utf-8',newline='\n')
    report=dict(host=socket.gethostname(),last_precise_host='HJOW-Athlon',last_precise_date='2026-11-03',rows=len(rows),
                kinds=dict(collections.Counter(row[1] for row in rows)),editions=editions,verify=args.verify,
                fixture_sha256=digest(FIXTURE),repeated_x87_controls=[0x027f,0x037f],game_runs=0,multiplayer_tests=0,
                limits=['공유 지형 helper만 격리 실행했다. 생성 root/밀도/실제 Session·Fort·Edit 연결은 다음 단위다.'])
    Path(args.report).write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(rows=len(rows),verify=args.verify,sha256=report['fixture_sha256'])))


if __name__=='__main__':main()
