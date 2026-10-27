#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Detonate의 실제 일반 입력/Spell·Object·Form 부모가 시전 요청 없이 끝나는지 관측한다."""
import argparse
import hashlib
import itertools
import json
import socket
import struct
from decomp_oracle import Oracle,ROOT,SCRATCH,STACK,STOP
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP

POOL=0x12000000  # 격리된 실제50BYTE 물리 슬롯이다.
TABLES=(0x502370,0x502448,0x502510,0x5025d8,0x5026a0,0x502778,0x502840,0x502908,0x5029d0,0x502aa8,0x502b70,0x502e68,0x502f30,0x502ff8,0x5030c0,0x503188)  # 보호PE의16개 Spell 가상6c/메뉴 표다.
FIXTURE=ROOT/'cpppj/tests/fixtures/detonate-input-x86.tsv'  # 현재 제품과 독립된 새 관측이다.
REPORT=ROOT/'cpppj/detonate-input-native-20261028.json'  # 실제 원문 구간과 명시 경계를 남긴다.


def digest(path):
    """보호 입력과 새 원문의 전체 바이트를 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def cases():
    """실제16VT·누름/뗌/보조키·y의낮은WORD·세 모드를 교차한다."""
    return list(itertools.product(TABLES,(0x03000074,0x83000074,0x13000074,0x23000074),(0,50,0x10032,0x10000),(0,1,2)))


class InputOracle(Oracle):
    """Guide/Gump 소비 뒤4d6eca부터 다음 입력4da477까지와 실제 세 가상 부모를 실행한다."""
    def __init__(self):
        """OS 없이 원문·가상 슬롯·검사 스택을 같은 격리 공간에 배치한다."""
        super().__init__('originals',[dict(name='General',originals='004d62b0')],[0x41e400,0x4ae010,0x4ac9b0,0x413d50,0x4e63e0,0x445330])
        self.mu.mem_map(POOL,0x10000);self.calls={};self.count=0;self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
        for table in TABLES:  # 이름 추정 대신 실제 두 슬롯을 직접 확인한다.
            assert self.word(table+0x6c)==0x41e400 and self.word(table+0xa8)==0x420b30

    def word(self,address):
        """현재 비정렬DWORD를 원문 폭으로 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def on_instruction(self,mu,address,size,data):
        """계산은 원문에 맡기고 메뉴 없음 조회와 다음 사건 경계만 분리한다."""
        self.count+=1
        if address==0x4da477:mu.emu_stop();return
        if address==0x460de0:
            self.popup+=1;sp=mu.reg_read(UC_X86_REG_ESP);target=self.word(sp);mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,target);return
        if address==0x41e400:self.trace.append('object:50:'+str(self.code))
        if address==0x4d6f09:self.trace.append('return:'+str(mu.reg_read(UC_X86_REG_EAX)))
        if address in (0x41e400,0x4ae010,0x4ac9b0,0x413d50,0x4e63e0,0x445330):self.calls[f'{address:08x}']=self.calls.get(f'{address:08x}',0)+1
        return super().on_instruction(mu,address,size,data)

    def write(self,mu,access,address,size,value,data):
        """몸체의 쓰기는 실제 검사 스택만 허용하고 월드/전역 변경은 즉시 거부한다."""
        if 0x30000000<=address and address+size<=0x30010000:return
        raise RuntimeError(f'Detonate 원문이 직접 상태를 변경함 {address:08x}+{size}')

    def observe(self,case):
        """입력만 설치하고 실제 가상 호출/반환·raw·스택과 정상 사건 끝을 읽는다."""
        table,self.code,parameter,mode=case;self.trace=[];self.popup=0
        raw=bytearray(50);struct.pack_into('<I',raw,0,table);raw[10]=114;raw[34]=1;struct.pack_into('<ff',raw,14,112,118);self.mu.mem_write(POOL+2500,bytes(raw))
        for address,value in ((0x5c8464,POOL),(0x540c70,1),(0x5c85a4,mode==1),(0x594fa4,1),(0x55a480,mode==2),(0x5325c8,1)):
            self.mu.mem_write(address,struct.pack('<I',value))  # 원래 UI가 완료한 현재 모드/깊이 입력이다.
        self.mu.mem_write(STACK,bytes(0x100));self.mu.mem_write(STACK+0x14,struct.pack('<III',self.code,parameter,parameter));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EDI,0)
        self.mu.emu_start(0x4d6eca,STOP,count=20000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==0x4da477 and self.mu.reg_read(UC_X86_REG_ESP)==STACK
        after=bytes(self.mu.mem_read(POOL+2500,50));assert after==raw
        return ';'.join(self.trace) or '-',after.hex(),str(self.popup)


def main():
    """새 fixture만 생성하거나 기존 바이트를 보존하며 원문을 다시 실행해 비교한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');options=parser.parse_args();oracle=InputOracle();rows=[]
    for case in cases():  # 제품의 출력을 기대값 생성에 사용하지 않는다.
        rows.append((','.join(map(str,case)),*oracle.observe(case)))
    raw='# input objectTrace raw50 popupBoundaryCalls\n'+'\n'.join('\t'.join(row) for row in rows)+'\n'
    if options.verify:
        record=json.loads(REPORT.read_text('utf-8'))
        for path,wanted in record['sha256'].items():assert digest(ROOT/path)==wanted,path  # 보호 출처/도구도 함께 확인한다.
        assert FIXTURE.read_text('utf-8')==raw and record['uniqueInputs']==len(rows);print('Detonate 원문 재실행',len(rows),'차이0');return
    assert not FIXTURE.exists() and not REPORT.exists(),'기존 기대값은 덮어쓰지 않는다';FIXTURE.write_text(raw,'utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_detonate_input.py','tools/decomp_oracle.py',FIXTURE.relative_to(ROOT).as_posix())
    result=dict(unit=521,host=socket.gethostname(),date='2026-10-28',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),uniqueInputs=len(rows),instructions=oracle.count,nativeCalls=oracle.calls,boundaries='Guide/Gump 소비 뒤4d6eca..다음 사건4da477과16VT의41e400→4ae010→4ac9b0·AI키·strchr·커서활성판정 전체. 460de0 팝업 없음만 명시 경계이며 실제 OS/게임/통신 없음. 원문은 아무 요청/월드 쓰기 없이 끝난다. 초기 데이터의 명령 ID와 비활성 커서 조건이다.10.72/모든 다른 입력 경로까지의 주장은 아니다.',sha256={p:digest(ROOT/p) for p in paths})
    REPORT.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(json.dumps(dict(uniqueInputs=len(rows),instructions=oracle.count)))


if __name__=='__main__':main()
