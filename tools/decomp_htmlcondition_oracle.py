#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 HTML 조건/토큰/CRT와 별도 실제 Config 치환 몸체를 시점별로 대조한다."""
import argparse
import csv
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_htmltext_oracle import Oracle as HtmlOracle,SPEC,ROOT,OBJECT,SOURCE,OUTPUT,STACK,STOP
from decomp_config_oracle import ConfigOracle
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 추가 TLS/독립 치환 반환 저장소는 기본 HTML VM과 겹치지 않는다.
THREAD,VALUES=0x11400000,0x11401000
FIXTURE=ROOT/'cpppj/tests/fixtures/htmlcondition-x86.tsv'
REPORT=ROOT/'cpppj/htmlcondition.json'
CONFIG=b'A="1"\r\nB="2"\r\nS="same"\r\nT="same"\r\n'
EXTRA={
 'originals':dict(tag=0x46c090,substitute=0x441740,tls=0x4ea61d,locale=0x543efc,
     helpers=[0x487620,0x487710,0x4e63e0,0x4e6656],alias=(0x4e66de,0x4e66e3)),
 'originalCD':dict(tag=0x495420,substitute=0x4a9bd0,tls=0,locale=0,
     helpers=[0x41a3d0,0x41a4a0,0x4f1bb0,0x4f1df0,0x4f1ea0,0x4f2600],alias=(0,0))}


def digest(path):
    """근거와 보호 파일의 현재 바이트를 SHA-256으로 고정한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def cases(edition):
    """조건 연산/부정/경계/가림/치환과 전체 순회의 입력을 독립적으로 정한다."""
    values=[]
    # 상태가 같아도 닫힘은 Config를 읽지 않아야 한다. 깊이의 DWORD 경계도 입력한다.
    for raw,shown,depth,closing in itertools.product((b'?',b'?0>',b'?1>',b'?!0>',b'?!1>',b'??S=T>',b'?gA=B>'),(0,1,0xffffffff),(0,1,2,0xffffffff),(0,1)):
        values.append(('Tag',raw,shown,depth,closing,(CONFIG,)))
    operators=(b'',b'!',b'?',b'!?',b'?!',b'!?',b'g',b'G',b'ge',b'gE',b'l',b'le',b'LE',b'!g',b'g!',b'!g!',b'gl',b'gel',b'gle',b'??') if edition=='originals' else (b'',b'!',b'!!',b'?',b'g')
    # 작은/큰 signed 숫자와 토큰의 문자 동등/차이를 실제 CRT로 비교한다.
    pairs=[(b'0',b'0'),(b'1',b'2'),(b'2',b'1'),(b'-1',b'0'),(b'2147483648',b'0'),(b'4294967296',b'0'),(b'-2147483649',b'1'),(b'999999999999999999999',b'0'),(b' +0012x',b'12'),(b'\x801',b'0'),(b'"same"',b'"same"'),(b'"Same"',b'"same"'),(b'"\xff"',b'"\x80"'),(b'{A}',b'{B}'),(b'{S}',b'{T}'),(b'{missing|0}',b'0')]
    for operator,(left,right) in itertools.product(operators,pairs):values.append(('Tag',b'?'+operator+left+b'='+right+b'>tail',1,0,0,(CONFIG,)))
    # 토큰78바이트/인용/연속 구분자와 Config가 치환한 오른쪽/왼쪽의 현재 값 교체다.
    for raw in (b'? {A}=>',b'?? "a=b"="a=b">',b'??'+b'a'*78+b'='+b'a'*78+b'>',b'??'+b'a'*79+b'=x>',b'?\n1>',b'?\t+1>',b'??{X}={X}>',b'?g{X}={X}>'):
        values.append(('Tag',raw,1,0,0,(b'X=1\r\n',b'X=2\r\n')))
    # 실제 전체 순회는 숨긴 열림을 평가하지 않으며 첫 닫힘에서 깊이가0이면 바로 다시 보인다.
    for text in (b'a<?0>hidden</?0>b',b'a<?1>shown</?1>b',b'a<?0>x<?0>y</?0>z</?0>b',b'<?!{A}>no</?{A}>yes',b'<??{S}={T}>same</?x>end',b'<?g{B}={A}>greater</?x>end',b'<?0><b>hidden</b></?0>shown',b'<?0><a>hidden</a></?0>shown',b'<?1>open'):
        values.append(('Html',text,1,0,0,(CONFIG,)))
    # 같은 원문/상태/설정 계획은 한 번만 고유 입력으로 저장한다.
    return list(dict.fromkeys(values))


class CountingConfig(ConfigOracle):
    """기존 실제 Config 치환 몸체를 같은 명시 CRT/외부 경계에서 실행하고 명령 수만 더 센다."""
    def __init__(self,edition):
        """기존 근거와 같은 실제 치환 VM을 준비한다."""
        self.instructions=0;super().__init__(edition)

    def on_instruction(self,mu,address,size,data):
        """기존 제한/경계를 유지하며 실제 진입한 명령 수를 기록한다."""
        self.instructions+=1;super().on_instruction(mu,address,size,data)


class Oracle(HtmlOracle):
    """주 VM에서 HTML/토큰/CRT를 실행하고 치환 호출마다 별도 실제 Config VM의 결과를 공급한다."""
    def __init__(self,edition):
        """보호 PE와 합성 현재 파서/설정/치환 저장소를 분리한다."""
        super().__init__(edition);self.edition=edition;self.extra=EXTRA[edition];self.config=CountingConfig(edition);self.mu.mem_map(THREAD,0x10000)
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/f'extracted/refined/{edition}/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        ranges=list(self.spec['ranges'])
        # HTML 분기가 부르는 실제 토큰/CRT 함수 범위를 추가한다.
        for address in self.extra['helpers']:
            for part in facts[address]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));ranges.append((low,high+1))
        ranges.append(self.extra['alias']);self.spec={**self.spec,'ranges':ranges}
        if self.extra['tls']:self.put(THREAD+0x64,self.get(self.extra['locale']))
        self.events=[];self.configs=();self.substitutions=0  # 현재 호출별 실제 치환 인자/결과와 설정 교체 계획이다.

    def ret(self,value=0):
        """cdecl 경계를 반환하며 실제 호출자가 인자를 정리하게 둔다."""
        stack=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_ESP,stack+4);self.mu.reg_write(UC_X86_REG_EIP,self.get(stack))

    def string(self,address):
        """합성/원본 메모리에서 실제 첫 NUL까지 읽는다."""
        output=bytearray()
        # 원본 토큰/치환 저장소의 범위를 넘는 끝없는 문자열은 거부한다.
        for offset in range(8192):
            value=self.mu.mem_read(address+offset,1)[0]
            if not value:return bytes(output)
            output.append(value)
        raise RuntimeError('HTML 조건의 NUL 없는 입력')

    def instruction(self,mu,address,size,data):
        """Config 호출 시점/요청100/인자0을 관찰하고 실제 다른 VM의 치환 바이트를 반환한다."""
        if address==self.extra.get('tls') and self.extra['tls']:self.ret(THREAD);return
        if address==self.extra['substitute']:
            stack=mu.reg_read(UC_X86_REG_ESP);source,capacity,a,b,c=struct.unpack('<IIIII',mu.mem_read(stack+4,20))
            if capacity!=100 or a or b or c:raise RuntimeError('HTML 조건 Config ABI 불일치')
            token=self.string(source);configuration=self.configs[min(len(self.events),len(self.configs)-1)]
            value=self.config.substitute([(None,configuration,False),(b'local',None,False)],0,token)
            if value is None or len(value)>=100:raise RuntimeError('HTML 조건의 치환 실패/원본 버퍼 초과')
            target=VALUES+len(self.events)*256;mu.mem_write(target,value+b'\0');self.events.append(token.hex()+'='+value.hex());self.substitutions+=1;self.ret(target);return
        super().instruction(mu,address,size,data)

    def run_case(self,case):
        """직접 태그와 실제 전체 순회 양쪽의 출력/가림/커서/치환 순서를 저장한다."""
        operation,source,shown,depth,closing,configurations=case;self.events=[];self.configs=configurations
        if operation=='Html':result=super().run((source.hex(),8192,shown,depth,0))
        else:
            self.mu.mem_write(SOURCE,source+b'\0\0\0');self.mu.mem_write(OBJECT,bytes(0x100));self.mu.mem_write(OUTPUT,bytes(0x10000))
            self.call(self.spec['ctor'],[]);self.call(self.spec['output'],[OUTPUT,8192,0,0]);self.call(self.spec['input'],[SOURCE]);self.put(OBJECT+0x1c,shown);self.put(OBJECT+0x20,depth)
            self.call(self.extra['tag'],[SOURCE,closing]);self.returns+=1
            result=[self.string(OUTPUT).hex(),0,*[self.get(OBJECT+x) for x in (0x1c,0x20,0x24,0x28,0x2c)],0]
        return [*result,';'.join(self.events) or '-']


def encode(case):
    """원문/설정 바이트를 공백/개행 없는 독립 입력 열로 표현한다."""
    operation,source,shown,depth,closing,configurations=case
    return [operation,source.hex() or '-',shown,depth,closing,','.join(value.hex() for value in configurations)]


def generate():
    """C++ 없이 두 실제 PE의 조건과 Config 치환 관찰을 저장한다."""
    rows=[];returns={};instructions={};configInstructions={};substitutions={}
    # CD boolean 문법과 PATCH 비교 문법의 입력 계획을 각 원본에 별도로 적용한다.
    for edition in SPEC:
        oracle=Oracle(edition)
        # 기대 가림 상태/문자열/비교 결과를 Python 모델로 계산하지 않는다.
        for case in cases(edition):rows.append([edition,*encode(case),*oracle.run_case(case)])
        returns[edition]=oracle.returns;instructions[edition]=oracle.instructions;configInstructions[edition]=oracle.config.instructions;substitutions[edition]=oracle.substitutions
    FIXTURE.write_text('# 실제 HTML조건/토큰/CRT + 별도 실제Config치환. 호출시점/원문/결과를함께관찰; 게임/GUI/OS없음.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),ROOT/'tools/decomp_htmltext_oracle.py',ROOT/'tools/decomp_config_oracle.py',ROOT/'tools/decomp_oracle.py',FIXTURE,ROOT/'originals/Netstorm.exe',ROOT/'originalCD/NETSTORM.EXE']
    report={'host':socket.gethostname(),'date':'2026-10-15','unique_inputs':len(rows),'returns':returns,'instructions':instructions,'configInstructions':configInstructions,'substitutions':substitutions,'os_calls':0,
        'boundaries':['HTML ConfigInterface ABI→분리 VM의 실제 Config::Substitute + 같은 현재 설정/빈 local 객체','HTML tolower ASCII mapping','PATCH CRT TLS pointer','Config oracle ASCII toupper/strnicmp/strchr/getenv/sprintf/assert boundaries'],
        'files':{p.relative_to(ROOT).as_posix():digest(p) for p in paths}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('HTML 조건 관찰',len(rows),returns,substitutions)


def verify():
    """기존 native 관찰의 원본/생성기/입력계획/집계를 재실행 없이 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 원본·의존 도구·fixture의 실제 바이트가 저장 근거와 같아야 한다.
    for path,expected in report['files'].items():
        if digest(ROOT/path)!=expected:raise RuntimeError('HTML 조건 SHA 불일치 '+path)
    plan=[[edition,*encode(case)] for edition in SPEC for case in cases(edition)]
    if len(rows)!=len(plan) or report['unique_inputs']!=len(rows) or report['os_calls']:raise RuntimeError('HTML 조건 입력 집계 불일치')
    # 결과를 이용하지 않고 독립 입력 계획/고유 행/정상 반환 수를 확인한다.
    for row,case in zip(rows,plan):
        if row[:7]!=list(map(str,case)):raise RuntimeError('HTML 조건 입력 계획 불일치')
    if any(value!=len(cases(edition)) for edition,value in report['returns'].items()):raise RuntimeError('HTML 조건 정상 반환 집계 불일치')
    print('HTML 조건 근거 감사 통과',len(rows))


def main():
    """새 독립 native 관찰과 저장 근거 감사만 실행한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()


if __name__=='__main__':main()
