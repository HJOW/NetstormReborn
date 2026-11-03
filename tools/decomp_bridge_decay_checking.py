#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""기존 다리 원문 실행기에 checking/진단 복귀 입력만 추가해 실제 Cell을 관측한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_bridgedecay_oracle import DecayOracle,ROOT,bridge,island,packed,joined,digest
from decomp_basefile_whole_read import BoundaryFailure
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX

FIXED=ROOT/'cpppj/tests/fixtures/bridgedecay-x86.tsv'  # 기존 실제 bridge 프레임 코드만 입력으로 재사용한다.

def codes():
    """동결된 최초 관측의 실제 프레임 표를 읽고 결과 열은 사용하지 않는다."""
    for line in FIXED.read_text(encoding='utf-8').splitlines():  # 프레임 표0 한 행만 찾는다.
        row=line.split('\t')
        if row[:2]==['Frames','0']:return bytes.fromhex(row[2])
    raise RuntimeError('기존 다리 프레임 표 없음')

def scene(code,shape,life):
    """열린 끝·이웃 없음·닫힌 판자·단단함·작은 Graph 입력만 만든다."""
    root=6101;objects=[island(6100,99,100),bridge(code,root,100,100,'K',life,20 if shape==3 else 1)]
    if shape==1:objects=objects[1:]
    if shape==2:objects.append(island(6102,101,100))
    objects=[(*item[:-1],item[-1]|0xa583) if item[7]==0 else item for item in objects]
    return objects,[(1,4 if shape==4 else 9,1)],root

class CheckingDecayOracle(DecayOracle):
    """전체 Cell/기하/수명은 기존 실제 코드이며 진단 장치의 복귀/변형만 공급한다."""
    def __init__(self):
        """동일 원본 범위·부모 장치와 허용 쓰기 경계를 재사용한다."""
        super().__init__('originals')

    def text(self,address):
        """진단 인자가 가리키는 첫 NUL까지 원문 바이트를 읽는다."""
        result=bytearray()
        for offset in range(512):  # 확인된 조건/파일명 범위만 읽는다.
            value=self.mu.mem_read(address+offset,1)[0]
            if not value:return bytes(result)
            result.append(value)
        raise RuntimeError('원문 진단 문자열 끝 없음')

    def on_instruction(self,mu,address,size,data):
        """선택된 Bridge.cpp268 진단의 인자와 복귀 전 접두 상태를 보존한다."""
        if address==self.spec['assert_report']:
            sp=mu.reg_read(UC_X86_REG_ESP);ret,condition,source,line=struct.unpack('<4I',mu.mem_read(sp,16))
            assert line==0x268 and self.text(source)==b'\\Ns\\O\\Bridge.cpp',(hex(line),self.text(condition))
            self.events.append('D:'+self.text(condition).hex()+':'+self.text(source).hex()+':'+str(line))
            if self.mutate:
                self.write_global('asserts',0);word=struct.unpack('<H',mu.mem_read(self.slot(self.root)+12,2))[0];mu.mem_write(self.slot(self.root)+12,struct.pack('<H',(word&~0x78)|8))
            if not self.resume:raise BoundaryFailure('assertion')
            mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,ret);return
        super().on_instruction(mu,address,size,data)

    def sample(self,case,code,precision):
        """명시 진단 중단과 정상 반환을 구별하며 모든 입력 슬롯의 결과를 읽는다."""
        shape,life,checking,self.resume,self.mutate=case;objects,records,self.root=scene(code,shape,life);self.scene(objects,records,code);self.modes(1,1,0,0,0);self.write_global('asserts',checking);outcome='ok'
        try:self.call('Cell',[],self.slot(self.root),0,precision)
        except BoundaryFailure:outcome='assertion'
        return '\t'.join([','.join(map(str,case)),packed(objects),packed(records),str(self.root),outcome,str(self.read_global('asserts')),joined(self.events),packed(self.after())])

def cases():
    """정상/초과4비트 수명과 전체DWORD·진단 복귀 조합을 구성한다."""
    result=[(0,life,checking,resume,mutate) for life,checking,resume,mutate in itertools.product(range(16),(0,1,0x80000000,0xffffffff),range(2),range(2))]
    result.extend((shape,9,checking,resume,0) for shape,checking,resume in itertools.product(range(1,5),(0,0x80000000),range(2)))
    assert len(result)==len(set(result))==272;return result

def main():
    """최초 기대값은 새 파일에만 쓰고 재관측은 기존 바이트와 대조한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--fixture',required=True,type=Path);parser.add_argument('--output',required=True,type=Path);parser.add_argument('--verify',action='store_true');args=parser.parse_args();assert not args.output.exists() and (args.fixture.exists() if args.verify else not args.fixture.exists())
    oracle=CheckingDecayOracle();code=codes();rows=['# case objects records root outcome finalChecking events after']
    for case in cases():  # 두 x87 제어값에서 같은 실제 부모 결과를 독립 관측한다.
        first=oracle.sample(case,code,0x027f);assert first==oracle.sample(case,code,0x037f);rows.append(first)
    content='\n'.join(rows)+'\n'
    if args.verify:assert args.fixture.read_text(encoding='utf-8')==content
    else:args.fixture.write_text(content,encoding='utf-8',newline='\n')
    record=dict(unit=795,rows=272,observations=544,host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',gameExecutions=0,multiplayerTests=0,boundary='전체 실제 Cell·기하/수명·프레임·Graph. 기존 할당/해제·기본삭제·표시·이동체·소리 장치는 유지. 진단 비복귀는 명시 중단이며 전체SEH/UI/실제게임 아님.',sha256={str(path):digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/bridgedecay/originals/functions.tsv',FIXED,Path(__file__),args.fixture)})
    args.output.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(rows=272,observations=544)))

if __name__=='__main__':main()
