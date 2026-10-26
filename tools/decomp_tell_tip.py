#!/usr/bin/env python3
"""원본 Guide/Gump/release 뒤 TellTip 번호/서식/Config/Tell 분기를 실제 CPU로 관찰한다."""
import argparse
import hashlib
import itertools
import json
import struct
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]  # 같은 보호 PE·현재 정밀 디컴파일·신규 결과의 기준이다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
sys.path.insert(0,str(ROOT/'tools'))
import decomp_production_shortcuts as gate
from unicorn.x86_const import UC_X86_REG_ESP

TIP=0x531910  # 보호 PE의 초기값1인 실제 DWORD다.
FIXTURE=ROOT/'cpppj/tests/fixtures/tell-tip-x86.tsv'
REPORT=ROOT/'cpppj/tell-tip-native-20261026.json'


class TipOracle(gate.Oracle):
    """기존 일반 입력의 실제 관문과 TellTip 분기를 바꾸지 않고 외부 서식/Config/Tell만 관찰한다."""
    def __init__(self):
        """번호/장치 trace만 별도로 저장하고 기존 원문 코드/쓰기 보호를 유지한다."""
        super().__init__();self.body=[]

    def string(self,address):
        """원문 명시 문자열의 첫NUL까지 읽는다."""
        result=bytearray()
        for offset in range(4096):  # 손상 문자열의 무한 조회를 허용하지 않는다.
            value=self.mu.mem_read(address+offset,1)[0]
            if not value: return bytes(result).decode('ascii')
            result.append(value)
        raise RuntimeError('TellTip NUL 없는 명시 문자열')

    def guard(self,mu,access,address,size,value,data):
        """원문 번호 쓰기와 기존 스택/큐 쓰기만 허용한다."""
        if address==TIP and size==4:
            self.body.append('number:'+str(value));return
        return super().guard(mu,access,address,size,value,data)

    def instruction(self,mu,address,size,data):
        """명시 CRT서식/Config/Tell의 실제 인자를 기록하고 나머지 원문 흐름은 그대로 실행한다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if address==0x4a89c0:
            output,pattern,value=(self.number(sp+offset) for offset in (4,8,12))
            if self.string(pattern)!='%d': raise RuntimeError('TellTip 다른 서식')
            signed=value if value<0x80000000 else value-0x100000000
            self.body.append('format:'+str(signed));mu.mem_write(output,str(signed).encode('ascii')+b'\0');self.returned(output);return
        if address==0x4414d0:
            key,value=self.string(self.number(sp+4)),self.string(self.number(sp+8));self.body.append('set:'+key+':'+value);self.returned();return
        if address==0x4cf960:
            self.body.append('tell:'+self.string(self.number(sp+4)));self.returned();return
        return super().instruction(mu,address,size,data)


def main():
    """최초 fixture를 새로 만들고 verify는 원문 재관찰과 바이트 동일성만 검사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if FIXTURE.exists()!=args.verify: raise RuntimeError('TellTip 신규 생성/기존 재현 모드 불일치')
    oracle=TipOracle();command=oracle.number(0x542aa0);rows=[]
    numbers=(0,1,2,10,0x7ffffffe,0x7fffffff,0x80000000,0xffffffff)
    # signed 인자와 감김 경계·서로 다른 두 인자·release/상위플래그를 실제 관문부터 교차한다.
    for number,parameter,flag,first in itertools.product(numbers,(0,1,2,3,4,0x7fffffff,0x80000000,0xffffffff),(0,0x80000000,0x38000000),(0,99)):
        oracle.number(TIP,number);oracle.body=[]
        case=dict(events=f'{command|flag}:{first}:{parameter}',guideResult=0,gumpResult=0,pauseDepth=0,console=0,command=0x020000f4,read=0,filled=0)
        observation=oracle.run(case,gate.CONTROLS[0])
        rows.append('\t'.join(map(str,(command|flag,first,parameter,number,oracle.number(TIP),';'.join(oracle.body) or '-',observation['stackdelta'],observation['nativeinstructions']))))
    # Guide 또는 실제 Gump가 먼저 소비하면 TellTip 자체 효과가 없는지 따로 확인한다.
    for guide,gump in ((1,0),(0,1),(0xffffffff,0),(0,0xffffffff)):
        oracle.number(TIP,1);oracle.body=[];case=dict(events=f'{command}:9:2',guideResult=guide,gumpResult=gump,pauseDepth=0,console=0,command=0x020000f4,read=0,filled=0)
        observation=oracle.run(case,gate.CONTROLS[0]);rows.append('\t'.join(map(str,(command,9,2,1,oracle.number(TIP),';'.join(oracle.body) or '-',observation['stackdelta'],observation['nativeinstructions'],guide,gump))))
    encoded=('# event/first/second/number/after/trace/stackDelta/instructions/[guide/gump]\n'+'\n'.join(rows)+'\n').encode('utf-8')
    if args.verify:
        if FIXTURE.read_bytes()!=encoded: raise RuntimeError('TellTip 원문 재현 불일치')
    else: FIXTURE.write_bytes(encoded)
    report=dict(host='HJOW-Athlon',lastFullDecompilation='HJOW-Athlon / 2026-10-23',command=command,initialTipNumber=1,rows=len(rows),originalGameExecuted=False,
        binarySha256=hashlib.sha256(gate.PE_PATH.read_bytes()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),gateToolSha256=hashlib.sha256(Path(gate.__file__).read_bytes()).hexdigest(),fixtureSha256=hashlib.sha256(encoded).hexdigest(),
        scope='실제 Guide/Gump/release 관문·TellTip0xEB 분기·직접 번호 쓰기·현재 signed %d 입력·Config/Tell 선택·분기 끝과 스택 균형',
        replaced=['Guide/Gump 소비 반환','004a89c0 %d 서식 장치','004414d0 Config 장치','004cf960 Tell 장치'])
    REPORT.write_bytes((json.dumps(report,ensure_ascii=False,indent=2)+'\n').encode('utf-8'));print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__': main()
