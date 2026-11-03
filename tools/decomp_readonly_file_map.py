#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제004dd580의 Win32인자/진단·복귀뒤진행을 독립 CPU에서 관측한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,TEXT,STACK,STOP,digest
from decomp_basefile_whole_read import BoundaryFailure
from decomp_file_name_leaf import add_ranges
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_FPCW
import pefile

NAMES=(b'case.shp',b'',b'part\0hidden')  # 최초 NUL/유효 빈 이름을 구별하는 독립 입력이다.

class FileMapOracle(MemoryOracle):
    """실제 부모와 명시 OS 장치만 실행하며 원본/소켓/게임은 구동하지 않는다."""
    def __init__(self):
        """확인한 코드 범위와 세 import만 격리 스텁에 연결한다."""
        super().__init__();add_ranges(self,(0x4dd580,));self.imports={};index=0
        image=pefile.PE(str(ROOT/'originals/Netstorm.exe'))
        for library in image.DIRECTORY_ENTRY_IMPORT:  # 실제 세 IAT 슬롯만 바꾸며 다른 OS 호출은 허용하지 않는다.
            for item in library.imports:
                if item.name not in (b'CreateFileA',b'CreateFileMappingA',b'MapViewOfFile'):continue
                entry=STOP+0x100+index*16;index+=1;self.put(item.address,entry);self.imports[entry]=item.name.decode('ascii')
        assert len(self.imports)==3

    def instruction(self,mu,address,size,data):
        """원문이 작성한 실제 보안 필드/모드·진단을 읽고 stdcall 규약만 복귀한다."""
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,7);entry=self.imports.get(address)
        if entry=='CreateFileA':
            security=self.words(args[3],3);self.events.append('F:'+self.string(args[0]).hex()+':'+','.join(map(str,(args[1],args[2],*security,args[4],args[5],args[6]))));self.ret(self.file,28);return
        if entry=='CreateFileMappingA':self.events.append('M:'+','.join(map(str,args[:6])));self.ret(self.mapping,24);return
        if entry=='MapViewOfFile':self.events.append('V:'+','.join(map(str,args[:5])));self.ret(self.view,20);return
        if address==0x4e0620:
            self.events.append('D:'+self.string(args[0]).hex()+':'+self.string(args[1]).hex()+':'+str(args[2]))
            if not self.return_assert:raise BoundaryFailure('assertion')
            self.ret();return
        if not any(low<=address<high for low,high in self.allowed):raise RuntimeError('미확인 매핑 명령 '+hex(address))

    def sample(self,case,precision):
        """입력 핸들과 이름만 제공하고 실제 반환·효과열을 읽는다."""
        name,self.file,self.mapping,self.view,self.return_assert=case;self.events=[];self.mu.mem_write(STACK-0x1000,b'\x65'*0x1100);self.mu.mem_write(TEXT,NAMES[name]+b'\0');self.mu.mem_write(STACK,struct.pack('<2I',STOP,TEXT));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='ok';returned='-'
        try:self.mu.emu_start(0x4dd580,STOP,count=10000)
        except BoundaryFailure:outcome='assertion'
        if outcome=='ok':assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4;returned=str(self.mu.reg_read(UC_X86_REG_EAX))
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision
        return '\t'.join([','.join(map(str,case)),outcome,returned,';'.join(self.events)])

def cases():
    """NULL/INVALID/유효 파일·매핑/view 실패와 진단 복귀를 독립 조합한다."""
    return list(itertools.product(range(3),(0,0xffffffff,0x12345678),(0,0x23456789),(0,0x3456789a),range(2)))

def main():
    """최초 바이트 작성 또는 재관측 검증만 허용한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--verify',action='store_true');args=parser.parse_args();assert not args.output.exists() and (args.fixture.exists() if args.verify else not args.fixture.exists())
    oracle=FileMapOracle();rows=['# case outcome returned events']
    for case in cases():  # 두 x87 정밀도에서도 동일한 원문 반환과 호출열이어야 한다.
        first=oracle.sample(case,0x027f);assert first==oracle.sample(case,0x037f);rows.append(first)
    content='\n'.join(rows)+'\n'
    if args.verify:assert args.fixture.read_text(encoding='utf-8')==content
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text(content,encoding='utf-8',newline='\n')
    report=dict(unit=790,rows=len(rows)-1,observations=2*(len(rows)-1),host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',gameExecutions=0,multiplayerTests=0,boundary='실제004dd580·OS3장치/진단은명시. 실제파일/Win32실패·SHP소비/반납·전체SEH는미관측.',sha256={str(path):digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__),args.fixture)})
    args.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(rows=len(rows)-1,observations=2*(len(rows)-1))))

if __name__=='__main__':main()
