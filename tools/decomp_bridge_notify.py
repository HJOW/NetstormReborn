#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 NotifySurface 정밀 범위만 격리 실행하고 현재 진단/복귀 결과를 기록한다."""
import argparse
import collections
import csv
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
import zlib
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW

ROOT=Path(__file__).resolve().parents[1]  # 보호 PE/정밀 표는 저장소에서 읽기만 한다.
POOL=0x10000000  # 원문 전역 포인터가 가리키는 격리 물리 입력이다.
STACK=0x20000000  # 함수/진단의 실제 cdecl 스택을 독립적으로 마련한다.
STOP=0x30000000  # 원문 ret 완료를 관측하고 명령 실행 전에 멈추는 호출자 경계다.
PREFIX=128  # 직접 raw/전체 체크섬을 대조할 물리 슬롯 수다.
SPECS={  # 원문 함수/전역 주소를 판본별로 구별하며 CD checking은 원문이 읽지 않는 장치 입력이다.
    'originals':dict(file='originals/Netstorm.exe',entry=0x4214a0,pool=0x5c8464,bridge=0x5411a0,checking=0x5e4794,assertion=0x4e0620,stride=50,extra=40),
    'originalCD':dict(file='originalCD/NETSTORM.EXE',entry=0x448c10,pool=0x5395dc,bridge=0x51ca8c,checking=STOP+0x1000,assertion=None,stride=36,extra=35),
}

def inputs():
    """원문의 TYPE/extra/checking/nibble 관문과 진단 복귀 입력만 공급한다."""
    rows=[]
    for sid,kind,bridge,extra,checking,low in itertools.product((0,4,50),(70,82,255),(82,70,338,0xffffffff),(0,8),(0,6,0x80000000),(0x3f,0x40,0x7f,0x80)):  # 숫자 비교·비영 DWORD·경계 nibble을 교차한다.
        rows.append((sid,kind,(0,1,2,4,8,9)[len(rows)%6],extra,0xa100|low,bridge,checking,0,1))
    for low in range(256):  # 낮은 BYTE의 모든 수명 nibble을 실제 명령으로 관측한다.
        rows.append((50,82,9,0x80,0xab00|low,82,0xffffffff,0,1))
    for sid,mutation,resume in itertools.product((0,4,50),(1,2),(0,1)):  # 진단의 명시 변경과 정상/중단을 서로 독립적으로 관측한다.
        rows.append((sid,82,0,0,0xab7f,82,6,mutation,resume))
    assert len(rows)==1132
    return rows

class BridgeNotifyOracle:
    """게임/OS를 실행하지 않고 현재 정밀 함수 한 개와 진단 장치만 허용한다."""
    def __init__(self,edition):
        """보호 이미지와 현재 PC 함수 범위에서만 실행 허용 집합을 만든다."""
        self.edition=edition;self.spec=SPECS[edition];pe=pefile.PE(str(ROOT/self.spec['file']));self.mu=Uc(UC_ARCH_X86,UC_MODE_32)
        self.mu.mem_map(pe.OPTIONAL_HEADER.ImageBase,(pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff);self.mu.mem_write(pe.OPTIONAL_HEADER.ImageBase,pe.get_memory_mapped_image())
        self.mu.mem_map(POOL,0x10000);self.mu.mem_map(STACK,0x10000);self.mu.mem_map(STOP,0x2000)
        with (ROOT/f'extracted/refined/{edition}/functions.tsv').open(encoding='utf-8') as source:
            row=next(item for item in csv.DictReader(source,delimiter='\t') if int(item['entry'],16)==self.spec['entry'])
        self.allowed=[tuple(int(value,16) for value in part.split('-')) for part in row['ranges'].split(';')]
        self.codeSha256={f'{lo:08x}-{hi:08x}':hashlib.sha256(bytes(self.mu.mem_read(lo,hi-lo+1))).hexdigest() for lo,hi in self.allowed}
        self.seed=bytes((index*29+7)&255 for index in range(PREFIX*self.spec['stride']));self.instructionCount=0;self.diagnostics=0
        self.mu.hook_add(UC_HOOK_CODE,self.instruction)

    def read(self,address,width=4):
        """실제 원문 바이트를 little endian 값으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def string(self,address):
        """진단에 전달한 원문 ASCII 문자열만 제한된 길이로 읽는다."""
        result=bytearray()
        for index in range(256):  # 알려진 짧은 원문 식/파일명만 허용한다.
            value=self.read(address+index,1)
            if not value:return bytes(result)
            result.append(value)
        raise ValueError('NotifySurface 진단 문자열 종료 없음')

    def instruction(self,mu,address,size,user):
        """선택한 진단만 장치 경계로 대체하며 그 외 실행은 정확한 원문 범위로 제한한다."""
        if address==STOP:
            self.outcome='return';mu.emu_stop();return
        if address==self.spec['assertion']:
            sp=self.read_register(UC_X86_REG_ESP);expression=self.string(self.read(sp+4));file=self.string(self.read(sp+8));line=self.read(sp+12)
            self.events.append(f'X:{expression.hex()}:{file.hex()}:{line}');self.diagnostics+=1
            sid,kind,flags,extra,word,bridge,checking,mutation,resume=self.case;slot=POOL+sid*self.spec['stride']
            if mutation==1:
                mu.mem_write(slot+10,bytes((255,9)));mu.mem_write(slot+12,struct.pack('<H',0xa1c2));mu.mem_write(slot+self.spec['extra'],bytes((8,)));mu.mem_write(self.spec['checking'],bytes(4));mu.mem_write(self.spec['bridge'],struct.pack('<I',70))
            elif mutation==2:
                mu.mem_write(slot+10,bytes((70,2)));mu.mem_write(slot+12,struct.pack('<H',0x00a5));mu.mem_write(slot+self.spec['extra'],bytes((0x80,)));mu.mem_write(self.spec['checking'],struct.pack('<I',0x80000000));mu.mem_write(self.spec['bridge'],struct.pack('<I',0xffffffff))
            if not resume:self.outcome='assertion';mu.emu_stop();return
            mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,self.read(sp));return
        if not any(lo<=address<=hi for lo,hi in self.allowed):raise RuntimeError(f'NotifySurface 허용 범위 밖 {address:08x}')
        self.instructionCount+=1

    def read_register(self,register):
        """원문 호출 스택/완료 상태의 현재 레지스터를 관측한다."""
        return self.mu.reg_read(register)

    def observe(self,case,precision):
        """판단 결과를 계산하지 않고 독립 raw/전역/trace와 정상 복귀를 수집한다."""
        self.case=case;self.events=[];self.outcome='unfinished';sid,kind,flags,extra,word,bridge,checking,mutation,resume=case;p=self.spec
        self.mu.mem_write(POOL,self.seed);slot=POOL+sid*p['stride'];self.mu.mem_write(slot+10,bytes((kind,flags)));self.mu.mem_write(slot+12,struct.pack('<H',word));self.mu.mem_write(slot+p['extra'],bytes((extra,)))
        self.mu.mem_write(p['pool'],struct.pack('<I',POOL));self.mu.mem_write(p['bridge'],struct.pack('<I',bridge));self.mu.mem_write(p['checking'],struct.pack('<I',checking));self.mu.mem_write(STACK,bytes(0x10000))
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)  # 호출자 레지스터의 과거 값을 다음 입력에 남기지 않는다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK+0x8000);self.mu.reg_write(UC_X86_REG_EFLAGS,0x202);self.mu.reg_write(UC_X86_REG_FPCW,precision);self.mu.mem_write(STACK+0x8000,struct.pack('<II',STOP,sid))
        self.mu.emu_start(p['entry'],STOP+1,count=1000);assert self.outcome!='unfinished' and self.read_register(UC_X86_REG_FPCW)==precision
        if self.edition=='originalCD':assert self.read_register(UC_X86_REG_EAX)==POOL+sid*p['stride']  # 버려지는 실제 CD 반환 포인터도 관측하되 C++ void API와 비교하지 않는다.
        return (';'.join(self.events) or '-',bytes(self.mu.mem_read(slot,p['stride'])).hex(),str(self.read(p['checking'])),str(self.read(p['bridge'])),self.outcome,str(zlib.adler32(self.mu.mem_read(POOL,len(self.seed)))))

def main():
    """두 독립 정밀도 관측의 전체 결과가 같은 경우에만 새 TSV/보고서를 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();assert not args.output.exists() and not args.report.exists();rows=[];details={}
    for edition in SPECS:  # 각 판본은 별도 이미지/가상 메모리 두 개에서 독립 관측한다.
        oracles=[BridgeNotifyOracle(edition),BridgeNotifyOracle(edition)];counts=collections.Counter()
        for case in inputs():  # 입력 격자는 공통으로 쓰되 기대값은 C++에서 가져오지 않는다.
            observed=[oracle.observe(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert observed[0]==observed[1],(edition,case);result=observed[0];rows.append((edition,','.join(map(str,case)),*result));counts[result[-2]]+=1;counts['diagnostics']+=result[0]!='-'
        details[edition]=dict(cases=sum(counts[key] for key in ('return','assertion')),outcomes=dict(counts),instructionCounts=[o.instructionCount for o in oracles],codeSha256=oracles[0].codeSha256,imageSha256=hashlib.sha256((ROOT/SPECS[edition]['file']).read_bytes()).hexdigest())
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text('# edition input9 events raw checking bridge outcome prefixAdler\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');report=dict(host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-11-03',cases=len(rows),observations=len(rows)*2,editions=details,tsvSha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),limits=['정밀 NotifySurface 한 함수만 실행·진단은 명시 변경/정상 복귀/중단 장치','CD의 checking은 원문이 읽지 않는 명시 장치 입력·실제 반환 포인터는 별도 관측 후 버림','게임/OS/진단 UI·Frame·공격·포획·희생·승리·MP 통신 실행0']);args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(report,ensure_ascii=False),flush=True)

if __name__=='__main__':main()
