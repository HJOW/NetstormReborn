#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 설정 해석의 개발 모드0/userCheat 허용값 슬라이스를 독립 관측한다."""
import argparse
import hashlib
import itertools
import json
import socket
from pathlib import Path
from decomp_game_menu_bar import Oracle,ROOT,STACK
from unicorn.x86_const import UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS

BEGIN,END=0x4353ea,0x43541a  #435220 전체 함수 안의 독립 설정 슬라이스다. 원문 함수 반환과 구별한다.
CHEAT,SPECIAL,AUXILIARY=0x5cab30,0x55a480,0x55a484  # 같은 실제 세 전역이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/html-checked-options-x86.tsv'  # 원문 출력만 저장하는 새 자료다.


class OptionsOracle(Oracle):
    """실제 정수 본체와 현재 ReadInt 장치 하나만 실행한다."""
    def instruction(self,mu,address,size,data):
        """선택 원문 밖에서는 userCheat ReadInt 한 번만 허용한다."""
        if BEGIN<=address<END:
            self.instructions+=1
            return
        if address!=0x441270:
            raise RuntimeError(f'원문 슬라이스 밖 진입 {address:08x}')
        esp=mu.reg_read(UC_X86_REG_ESP)
        if self.text(self.word(esp+4))!=b'userCheat' or self.word(esp+8)!=CHEAT or self.word(SPECIAL) or self.word(AUXILIARY):
            raise RuntimeError('실제 ReadInt 키/현재 주소/선행0 쓰기 불일치')
        self.reads+=1
        if self.value is not None:
            self.word(CHEAT,self.value)
        self.returned(0 if self.value is None else 1)

    def run(self,case):
        """실제 비교/쓰기와 기존 호출 인자 정리 뒤 continuation 위치를 읽는다."""
        before,self.value,special,auxiliary=case;self.reads=0;self.instructions=0  # 현재 ReadInt 공급값/선택 호출 수/원문 명령 수를 매 입력에서 초기화한다.
        for address,value in ((CHEAT,before),(SPECIAL,special),(AUXILIARY,auxiliary)):  # 각 전역의 현재 입력을 독립적으로 정한다.
            self.word(address,value)
        self.mu.mem_write(STACK,b'\xcc'*128);self.mu.reg_write(UC_X86_REG_EBX,0);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.mu.emu_start(BEGIN,END,count=1000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=END or self.reads!=1:
            raise RuntimeError('선택 원문 정상 완료/ReadInt 횟수 불일치')
        return [self.word(CHEAT),self.word(SPECIAL),self.word(AUXILIARY),self.mu.reg_read(UC_X86_REG_ESP)-STACK,self.reads]


def generate():
    """C++와 무관하게 보호PE의 현재 출력만 관측한다."""
    oracle=OptionsOracle();rows=[];instructions=0
    for case in itertools.product((0,0x845fed,1,0xffffffff),(None,0,0x845fed,0x845fec,0x845fee,0xffffffff),(0,1,0xffffffff),(0,0xffffffff)):  # 누락/허용/거부와 이전 값 보존을 조합한다.
        output=oracle.run(case);instructions+=oracle.instructions;rows.append([case[0],'-' if case[1] is None else case[1],case[2],case[3],*output])
    return '# 실제10.78 설정 슬라이스4353ea..43541a. ReadInt는 명시 현재값 장치이며 게임/OS 실행 없음.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',len(rows),instructions


def main():
    """새 자료 작성과 기존 자료를 바꾸지 않는 원문 재생을 구별한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();content,rows,instructions=generate()
    if args.check:
        if FIXTURE.read_text(encoding='utf-8')!=content:
            raise RuntimeError('설정 슬라이스 원문 재생 불일치')
    else:
        if FIXTURE.exists():
            raise RuntimeError('기존 자료 덮어쓰기 금지: --check 사용')
        FIXTURE.write_text(content,encoding='utf-8',newline='\n')
    paths=[Path(__file__),ROOT/'tools/decomp_game_menu_bar.py',ROOT/'originals/Netstorm.exe',FIXTURE]  # 원문/장치/결과의 현재 식별자다.
    report={'host':socket.gethostname(),'rows':rows,'instructions':instructions,'readOnlyReplay':args.check,'nativeSlice':[BEGIN,END],'entryEbx':0,'entryEbxEvidence':'43523b xor ebx,ebx 이후4353ea 전 EBX 대입 없음; 하위 호출은 보존 ABI 장치','osCalls':0,'limits':['435220 전체 설정 함수가 아니라4353ea..43541a 슬라이스다. ReadInt의 현재 DWORD/누락 처리는 명시 장치다.','x87을 사용하지 않는 정수 경로이며 두 정밀도 비교를 별도로 수행하지 않았다.','Client의 기존55a480 표현은 bool이다. 임의 비영 DWORD의 전체 비트 표현 복원은 주장하지 않는다.'],'hashes':{p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({'rows':rows,'instructions':instructions,'readOnlyReplay':args.check}))


if __name__=='__main__':
    main()
