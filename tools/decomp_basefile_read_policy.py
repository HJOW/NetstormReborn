#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""BaseFile 읽기 부모의 실제 장치 판정·접두 절삭·조회 순서를 제품 전에 고정한다."""
import csv
import itertools
import json
import socket
import struct
import subprocess
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP

# 실제 원본 전역과 격리 파일 객체, 최초 자료의 위치다.
PRIMARY,SECONDARY,OBJECT=0x544d80,0x544e80,POOL+0x4000
FIXTURE=ROOT/'cpppj/tests/fixtures/basefile-read-policy-x86.tsv'
REPORT=ROOT/'extracted/cpp-basefile-read-policy-20261031-native.json'


class LookupFailure(Exception):
    """원문이 예외 객체를 만들기 직전의 읽기 실패를 구별한다."""


class Oracle(MemoryOracle):
    """FileSpec·장치 getter·경로 조합·읽기 부모는 실제 CPU이며 파일 열기만 명시 응답이다."""
    def __init__(self):
        """보호 PE/함수 범위와 SEH 첫 슬롯을 읽기 전용 입력으로 준비한다."""
        super().__init__();self.mu.mem_map(0,0x1000)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as file:
            facts={int(row['entry'],16):row for row in csv.DictReader(file,delimiter='\t')}
        self.allowed=[]
        for entry in (0x41b4c0,0x45a220,0x459610,0x4595f0,0x459900,0x459c60):  # 파일 이름 판정/접두 계산을 스텁으로 계산하지 않는다.
            for part in facts[entry]['ranges'].split(';'):  # 실제 떨어진 범위만 허용한다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def instruction(self,mu,address,size,data):
        """경로/조회 인자를 기록하며 원문의 분기·substring·조합은 명령 그대로 실행한다."""
        self.instructions+=1
        if self.instructions>30000:
            raise RuntimeError('BaseFile 읽기 원문 명령 상한')
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,5);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address in (0x4e63e0,0x4e7620):
            value=self.string(args[0]);character=args[1]&255
            index=len(value) if not character else (value.find(bytes([character])) if address==0x4e63e0 else value.rfind(bytes([character])))
            self.ret(args[0]+index if index>=0 else 0)
        elif address==0x41a5b0:
            caller=self.words(esp,1)[0];site={0x41b53a:1,0x41b5cb:3,0x41b678:0,0x41b743:3}[caller]
            assert ecx==OBJECT and args[1:3]==(0,0)
            self.trace.append(f'{site}:{self.string(args[0]).hex()}');success=bool(self.mask&(1<<site))
            if success:
                self.selected=site
            self.ret(int(success),12)
        elif address==0x41a340:
            assert ecx in (0x5450b8,0x544f80)
            site=4 if ecx==0x544f80 else 2;self.trace.append(f'{site}:{self.string(args[0]).hex()}');self.pending=site
            self.put(args[1],0);self.put(args[1]+4,1);self.put(args[2],0)
            self.ret(TEXT+0x5000+site*256 if self.mask&(1<<site) else 0,12)
        elif address==0x41b460:
            assert ecx==OBJECT and args[1:3]==(0,1)
            if self.archive_open:
                self.selected=self.pending
            else:
                self.archive_failed=True
            self.ret(self.archive_open,12)
        elif address==0x4e59a3:
            self.ret(0)
        elif address==0x41a0d0:
            raise LookupFailure('archive-failed' if self.archive_failed else 'missing')
        elif not any(low<=address<high for low,high in self.allowed):
            raise RuntimeError('BaseFile 읽기 범위 밖 명령:'+hex(address))

    def sample(self,case):
        """원시 경로/조회 성공 입력만 준비하고 반환 경로와 전체 조회 순서를 관측한다."""
        base,name,mask,archive_open=case;self.trace=[];self.instructions=0;self.mask=mask;self.archive_open=archive_open;self.archive_failed=False;self.selected=-1
        self.mu.mem_write(STACK-4096,bytes(8192));self.mu.mem_write(OBJECT,bytes(64));self.put(0,0);self.put(0x55b160,0)
        self.mu.mem_write(PRIMARY,bytes(256));self.mu.mem_write(SECONDARY,bytes(256));self.mu.mem_write(PRIMARY,base+b'\0');self.mu.mem_write(SECONDARY,b'D:\\disc\0')
        self.mu.mem_write(TEXT,name+b'\0')
        for site in (2,4):  # 반환한 아카이브 이름은 부모의 열기/오류 인자 수명 동안 유지한다.
            self.mu.mem_write(TEXT+0x5000+site*256,('archive'+str(site)+'.tarc').encode('ascii')+b'\0')
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 독립 caller의 입력이다.
            self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,OBJECT);self.mu.mem_write(STACK,struct.pack('<IIII',STOP,TEXT,0,0))
        outcome='ok'
        try:
            self.mu.emu_start(0x41b4c0,STOP)
        except LookupFailure as failure:
            outcome=str(failure)
        if outcome=='ok' and self.mu.reg_read(UC_X86_REG_ESP)!=STACK+16:
            raise RuntimeError('BaseFile 읽기 원문 스택 복귀')
        return '\t'.join((base.hex() or '-',name.hex() or '-',str(mask),str(archive_open),outcome,str(self.selected),';'.join(self.trace)))


def main():
    """새 정책 모듈이 없고 기존 VFS가 깨끗할 때만 최초 자료를 생성한다."""
    assert not FIXTURE.exists() and not REPORT.exists(),'최초 읽기 정책 관측 덮어쓰기 금지'
    assert not (ROOT/'cpppj/src/o/RawBaseFileReadPolicy.cpp').exists(),'제품 전 관측 필요'
    subprocess.run(['git','diff','--exit-code','--','cpppj/src/o/BaseFile.cpp','cpppj/src/client/TechnicalHelp.cpp'],cwd=ROOT,check=True,capture_output=True)
    paths=((b'C:\\base',b'.\\d\\a'),(b'C:\\base',b'd/a'),(b'C:\\base',b'C:\\base\\d\\a'),(b'C:\\base\\',b'C:\\base\\d\\a'),(b'C:\\base',b'C:\\base2\\d\\a'),(b'C:\\base',b'C:\\BASE\\d\\a'),(b'C:\\base',b'D:\\else\\d\\a'),(b'C:\\base',b'C:d\\a'),(b'C:\\base',b'\\\\server\\share\\d\\a'),(b'C:\\base',b'//server/share/d/a'),(b'C:\\base',b'//server\\share/d/a'),(b'',b'C:\\base\\d\\a'),(b'C:\\base',b'a:b'),(b'C:\\base',b'\\d\\a'),(b'C:\\base',b'd\\a\0ignored:C'),(b'C:\\base',b''))
    oracle=Oracle();lines=['# baseHex / nameHex / lookupMask / archiveOpen / nativeOutcome / selectedSite / actualLookupTrace'];instructions=0
    for pair,mask,opened in itertools.product(paths,range(32),(0,1)):  # 모든 조회 성공 조합과 아카이브 열기 실패를 별도로 준다.
        case=(*pair,mask,opened);first=oracle.sample(case);instructions+=oracle.instructions;second=oracle.sample(case);instructions+=oracle.instructions;assert first==second;lines.append(first)
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths_to_hash=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_basefile_read_policy.py','cpppj/tests/fixtures/basefile-read-policy-x86.tsv')
    report={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','observedBeforeProduct':True,'cases':len(lines)-1,'observations':(len(lines)-1)*2,'instructions':instructions,'gameOrOsExecuted':False,'scope':'모드0/플래그0의 실제 FileSpec 생성·장치 판정·경로 조합·접두 절삭·조회 부모다. FILE/TAFF 효과와 오류 객체 진입만 경계다. 전체 FILE/예외 객체 ABI는 범위 밖이다.','sourceBeforeSha256':{name:digest(ROOT/name) for name in ('cpppj/src/o/BaseFile.cpp','cpppj/src/client/TechnicalHelp.cpp')},'sha256':{name:digest(ROOT/name) for name in paths_to_hash}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':
    main()
