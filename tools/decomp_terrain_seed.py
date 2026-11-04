#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 호스트의 양판본 정밀 범위에서 첫 raw 청크 시드/흩뿌리기만 격리 관찰한다."""
import argparse
import collections
import csv
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

ROOT=Path(__file__).resolve().parents[1]  # 보호 PE/현행 정밀 산출물/fixture의 기준 경로다.
MAP,CHUNKS,STACK,STOP=0x12000000,0x12100000,0x20000800,0x30000000  # 게임/OS와 분리한 context/청크/스택이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/terrain-seed-x86.tsv'  # 입력과 실제 반환·진단·전체 배열의 독립 기록이다.
SPECS={  # 두 판본의 실제 몸체와 필드 오프셋을 명시하고 복원된 C++는 실행하지 않는다.
    'originals':dict(first=0x4c0e20,scatter=0x4c13f0,getter=0x432180,no_region=0x50c038,code=8,
        pointer=0x20814,bounds=0x20820,assertion=0x4e0620,
        entries=(0x4c0e20,0x4c13f0,0x432180,0x432300,0x4322e0,0x4bca60,0x4e49c0)),
    'originalCD':dict(first=0x49de40,scatter=0x49e4e0,getter=0x404090,no_region=0x505eb0,code=4,
        pointer=0x20810,bounds=0x2081c,assertion=0,
        entries=(0x49de40,0x49e4e0,0x404090,0x404370,0x404350,0x4a59a0,0x4a59d0,0x4a59f0,0x49d580,0x4f161c))}


def digest(path):
    """현재 보호/코드/입력 파일의 실제 바이트 SHA-256을 기록한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def scene(profile,seed):
    """예상 결과 없이 청크/경계/기존 배열의 입력만 반환한다."""
    bounds=(-1,-1,1,1) if profile==4 else (3,3,3,5) if profile==0 else (3,3,5,5)
    entries=[]
    if profile==2:
        entries=[(4,3,0,seed),(3,4,1,(seed+19)&255)]
    elif profile==3:
        entries=[(3,3,255,seed),(4,3,128,(seed+19)&255),(3,4,0,(seed+37)&255)]
    elif profile==4:
        entries=[(0,0,0,seed)]
    elif profile in (5,6,7,8,9):
        entries=[(3,3,0,seed),(4,3,1,(seed+13)&255),(3,4,127,(seed+19)&255),(4,4,255,(seed+37)&255)]
    return bounds,entries


def inputs():
    """raw BYTE/전체 signed DWORD·checking·진단 복귀를 서로 독립인 입력으로 둔다."""
    cases=[('First',(p,s,n,c,0 if p==4 else 1,0,0))
           for p,s,n,c in itertools.product(range(6),(0,128,255),(127,-1,255),(0,0x80000000))]
    cases.extend(('First',(4,37,127,1,0,m,0)) for m in (1,2,3))
    cases.extend(('Scatter',(p,s,n,1,1,0,count))
                 for p,s,n,count in itertools.product(range(6,11),(0,128,255),(127,-1,255),(0,54))
                 if p!=10 or n==127)
    return cases


def rle(data):
    """관찰한 전체 배열을 값/연속 길이로 손실 없이 보관한다."""
    return ','.join(f'{value:02x}:{len(list(group))}' for value,group in itertools.groupby(data))


class Oracle:
    """원본 getter/PRNG/ftol까지 실행하며 진단 표시만 명시 외부 경계로 둔다."""

    def __init__(self,edition):
        """보호 PE를 변경 없이 읽고 현재 functions.tsv의 정확한 불연속 범위만 허용한다."""
        self.edition,self.s=edition,SPECS[edition]
        self.pepath=ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE')
        self.table=ROOT/f'extracted/refined/{edition}/functions.tsv'
        image=pefile.PE(str(self.pepath)).get_memory_mapped_image()
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32)
        self.mu.mem_map(0x400000,(len(image)+4095)&~4095);self.mu.mem_write(0x400000,image)
        for address,size in ((MAP,0x22000),(CHUNKS,4096),(STACK&~4095,4096),(STOP,4096)):
            self.mu.mem_map(address,size)  # 실행이 쓸 물리만 명시적으로 매핑한다.
        with self.table.open(encoding='utf-8') as stream:
            rows={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        self.selected=[rows[entry] for entry in self.s['entries']]
        self.allowed=[]
        for row in self.selected:
            for part in row['ranges'].split(';'):  # 각 함수의 실제 불연속 끝 명령 바이트까지 허용한다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.context=self.mu.context_save()
        self.entries=collections.Counter();self.getters=[];self.assertions=[];self.draws=[];self.cell_writes=0
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def put(self,address,value,width=4):
        """호스트 signed 값도 원문 DWORD/BYTE 비트로 격리 메모리에 쓴다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def get(self,address):
        """비정렬 DWORD를 little endian으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,4),'little')

    def instruction(self,mu,address,size,data):
        """실제 방문/난수 횟수/진단 인자를 관찰하고 선택한 진단 효과만 적용한다."""
        stack=mu.reg_read(UC_X86_REG_ESP)
        if address==self.s['assertion']:
            expression=bytes(mu.mem_read(self.get(stack+4),100)).split(b'\0')[0].decode('ascii')
            file=bytes(mu.mem_read(self.get(stack+8),100)).split(b'\0')[0].decode('ascii')
            line=self.get(stack+12);self.assertions.append(f'{expression}|{file}|{line}')
            if self.mutation==1 and line==0xb7:
                self.put(CHUNKS,1);self.put(self.s['no_region'],0)
            if self.mutation==2 and line==0xb8:
                self.put(MAP+self.s['bounds']+24,0)  # 현재right=0으로 바꾸어 원래left−1 한 번만 순회하는 입력이다.
                self.put(MAP+self.s['bounds']+8,0)
                self.put(CHUNKS+12,127,1)
            if self.mutation==3 and line==0xb7:
                self.put(CHUNKS,1);self.put(0x5e4794,0)
                self.put(CHUNKS+12,127,1)
            mu.reg_write(UC_X86_REG_ESP,stack+4);mu.reg_write(UC_X86_REG_EIP,self.get(stack));return
        if address==self.s['getter']:
            self.getters.append(f'{self.get(stack+4):08x},{self.get(stack+8):08x}')
        if address in (0x4bca60,0x4a59f0):
            self.draws.append((self.get(stack+4),self.get(stack+8)))
        if address in self.s['entries']:
            self.entries[f'{address:08x}']+=1
        assert any(low<=address<high for low,high in self.allowed),(hex(address),self.case)

    def write(self,mu,access,address,size,value,data):
        """원본은 스택과 code/region BYTE만 쓸 수 있다. 범위 밖/청크/전역 쓰기는 거부한다."""
        if STACK-768<=address and address+size<=STACK+32:return
        code=MAP+self.s['code']
        if code<=address and address+size<=code+0x20000 and size==1:
            self.cell_writes+=1;return
        raise RuntimeError(f'시드 원문 허용 밖 쓰기 {address:08x}+{size}, {self.case}')

    def run(self,kind,case,control,stalled=False):
        """독립 입력을 새로 놓고 전체 배열·반환 규약·청크 불변·게임 난수 불변을 확인한다."""
        self.case,self.mutation=(kind,case),case[5]
        profile,seed,no_region,checking,mode,mutation,count=case
        mu=self.mu;mu.context_restore(self.context)
        mu.mem_write(MAP,bytes(0x22000));mu.mem_write(CHUNKS,bytes(4096));mu.mem_write(STACK-768,b'\xa5'*800)
        self.put(CHUNKS,mode);self.put(CHUNKS+4,16);self.put(CHUNKS+8,16)
        for index in range(256):self.put(CHUNKS+12+index*12,127,1)  # 입력의 빈 청크 BYTE만 고정한다.
        bounds,entries=scene(profile,seed)
        for x,y,island,raw_seed in entries:  # 출력 지형 없이 원본 청크 원시 바이트만 공급한다.
            self.put(CHUNKS+12+(x*16+y)*12,island,1);self.put(CHUNKS+17+(x*16+y)*12,raw_seed,1)
        self.put(MAP+self.s['pointer'],CHUNKS)
        for index,value in enumerate(bounds):  # context의 청크 bounds와 칸 bounds는 별개 입력이다.
            self.put(MAP+self.s['bounds']+16+index*4,value);self.put(MAP+self.s['bounds']+index*4,value*16)
        if profile==8:
            for index,value in enumerate((50,50,53,53)):self.put(MAP+self.s['bounds']+index*4,value)
        code=MAP+self.s['code'];mu.mem_write(code,b'.'*65536);mu.mem_write(code+65536,bytes((no_region&255,))*65536)
        if profile==9:
            mu.mem_write(code,b'x'*65536);mu.mem_write(code+65536,b'\x09'*65536)
        self.put(self.s['no_region'],no_region)
        if self.edition=='originals':self.put(0x5e4794,checking)
        rng=0x532710 if self.edition=='originals' else 0x5308ec;self.put(rng,0x12345678)
        chunks_before=bytes(mu.mem_read(CHUNKS,4096))
        self.getters,self.assertions,self.draws,self.cell_writes=[],[],[],0
        arguments=() if kind=='First' else (count,)
        self.put(STACK,STOP)
        for index,value in enumerate(arguments):self.put(STACK+4+index*4,value)
        mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_ECX,MAP)
        saved={r:0x12345670+i for i,r in enumerate((UC_X86_REG_EBX,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI))}
        for register,value in saved.items():mu.reg_write(register,value)  # 비휘발 레지스터를 원본 규약과 비교한다.
        mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        mu.emu_start(self.s['first' if kind=='First' else 'scatter'],STOP,count=20000 if stalled else 2000000)
        if stalled:
            assert mu.reg_read(UC_X86_REG_EIP)!=STOP and not self.cell_writes and len(self.draws)>100,self.case
            return dict(instruction_budget=20000,returned=False,writes=self.cell_writes,range_draws=len(self.draws),
                        last_instruction=f'{mu.reg_read(UC_X86_REG_EIP):08x}',input=list(case))
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+4+len(arguments)*4,self.case
        assert not(mu.reg_read(UC_X86_REG_FPSW)&0x3800) and all(mu.reg_read(r)==v for r,v in saved.items()),self.case
        assert bytes(mu.mem_read(STACK-768,16))==b'\xa5'*16 and self.get(rng)==0x12345678
        if not mutation:assert bytes(mu.mem_read(CHUNKS,4096))==chunks_before
        result=mu.reg_read(UC_X86_REG_EAX) if kind=='First' else 0
        return (result,';'.join(self.assertions) or '-', ';'.join(self.getters) or '-',len(self.draws),self.cell_writes,
                rle(bytes(mu.mem_read(code,65536))),rle(bytes(mu.mem_read(code+65536,65536))))


def main():
    """생성과 기존fixture 재관측을 구별하며 검증 때 기대값은 다시 쓰지 않는다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--report',required=True)
    args=parser.parse_args();rows=[];editions={}
    for edition in SPECS:  # 양판본의 같은 입력을 각각 실제 몸체로 독립 관측한다.
        oracle=Oracle(edition)
        for kind,case in inputs():
            result=oracle.run(kind,case,0x027f)
            assert result==oracle.run(kind,case,0x037f),(edition,kind,case)
            rows.append((edition,kind,','.join(map(str,case)),*result))
        stall=oracle.run('Scatter',(10,0,-1,1,1,0,54),0x027f,stalled=True)
        editions[edition]=dict(rows=len(inputs()),entries=dict(oracle.entries),selected=oracle.selected,impossible_domain=stall,
                              pe_sha256=digest(oracle.pepath),ranges_sha256=digest(oracle.table))
        print(edition,len(inputs()),flush=True)
    output='# 실제 양판본 첫 raw+5 시드/흩뿌리기: 진단 표시만 외부 경계, 전체 code/region RLE.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8')==output,'시드 원문 fixture 재관측 불일치'
    else:FIXTURE.write_text(output,encoding='utf-8',newline='\n')
    report=dict(host=socket.gethostname(),last_precise_host='HJOW-Athlon',last_precise_date='2026-11-03',rows=len(rows),
                repeated_x87_controls=[0x027f,0x037f],editions=editions,verify=args.verify,fixture_sha256=digest(FIXTURE),
                game_runs=0,multiplayer_tests=0,limits=['격리된 공유 지형 부품만 실행했다. MP세션/패킷/통신/게임 실행은0이다.',
                '생성 root004c1510·밀도·영역 성장/제거·도장/군집/실제 세션 연결 완료 근거가 아니다.'])
    Path(args.report).write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(rows=len(rows),verify=args.verify,sha256=report['fixture_sha256'])))


if __name__=='__main__':main()
