#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다리 postPop의 flag/extra 접두와 연결→공통 후처리 호출 순서를 실제 세 PE에서 얻는다.

004213b0/CD 00448b00의 연결·소유자 전파 및 공통 postPop은 명시적인 대체다.
게임/OS/창을 실행하지 않는다. 나머지 함수 몸체·정상 반환·x87 상태·쓰기 범위를 검사한다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path

from decomp_bridgeevent_oracle import (EventOracle,ROOT,SPECS,CONTROLS,BRIDGE,base_case,digest)
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 실제 다리 전용 postPop·연결 생성·공통 postPop의 판본별 진입점이다.
ENTRIES={'originals':(0x422150,0x4213b0,0x4b0d30),'originalCD':(0x449890,0x448b00,0x4ae180)}
ENTRIES['original1037']=ENTRIES['originalCD']
# 새 결과는 기존 postpop/bridgeevent fixture와 분리한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/bridgepostpop-x86.tsv'
REPORT=ROOT/'cpppj/recovery-bridgepostpop-evidence.json'
# 최초/재등록·매몰 활성화·무관한 비트와 abstract/buried/기존 접두 비트 입력이다.
FLAGS=(0,1,2,3,4,5,6,7,0x20001,0x20004)
EXTRAS=(0,1,2,4,8,0x10,0xff,0x81)


class PostOracle(EventOracle):
    """다리 접두 몸체만 실제 실행하고 두 후속 함수의 호출을 기록한다."""
    def __init__(self,edition):
        """실제 함수 범위를 더하고 공통 후처리/연결의 대체 경계를 지정한다."""
        super().__init__(edition)
        entry,connect,common=ENTRIES[edition]
        self.spec=dict(self.spec,entries=dict(self.spec['entries'],BridgePost=entry))
        self.stubs[connect]='bridge_connect'
        self.stubs[common]='common_post'
        paths=[ROOT/f'extracted/bridgepostpop/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.exports.extend(paths)
        with paths[1].open(encoding='utf-8') as fp:
            # 내보낸 불연속 몸체만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'))
                    self.allowed.append((low,high+1))

    def on_instruction(self,mu,address,size,data):
        """후속 연결/공통 함수만 대체하고 인자·호출 순서를 보존한다."""
        stub=self.stubs.get(address)
        if stub in ('bridge_connect','common_post'):
            self.stub_calls[stub]+=1
            esp=mu.reg_read(UC_X86_REG_ESP)
            ret,a0,a1,a2=struct.unpack('<IIII',mu.mem_read(esp,16))
            if stub=='bridge_connect':
                self.events.append(f'L:{a0}:{a1}:{a2}')
                purge=0
            else:
                self.events.append(f'B:{self.sid_of(mu.reg_read(UC_X86_REG_ECX))}:{a0}')
                purge=4
            mu.reg_write(UC_X86_REG_EAX,0)
            mu.reg_write(UC_X86_REG_ESP,esp+4+purge)
            mu.reg_write(UC_X86_REG_EIP,ret)
            return
        super().on_instruction(mu,address,size,data)

    def run(self,flags,extra,control):
        """접두가 바꿀 수 있는 extra 한 바이트만 허용하고 두 후속 호출을 관찰한다."""
        self.prepare(base_case(flag=extra))
        address=self.slot(BRIDGE)+self.o['flag']
        self.write_ranges=[(address,address+1)]
        self.call('BridgePost',[flags],self.slot(BRIDGE),4,control,returns_float=False)
        return int.from_bytes(self.mu.mem_read(address,1),'little'),';'.join(self.events)


def generate():
    """세 실제 PE와 두 정밀도에서 동일한 접두 관찰만 저장한다."""
    rows=[]
    oracles={edition:PostOracle(edition) for edition in SPECS}
    # flag와 extra의 모든 입력 조합을 독립 실행한다.
    for edition,oracle in oracles.items():
        for flags,extra in itertools.product(FLAGS,EXTRAS):
            values=[oracle.run(flags,extra,control) for control in CONTROLS]
            if values[0]!=values[1]:raise RuntimeError(f'x87 정밀도 차이: {edition}')
            rows.append([edition,flags,extra,*values[0]])
        print(f'{edition}: 다리 postPop 접두 80개 통과',flush=True)
    FIXTURE.write_text('# 실제 다리 postPop 접두. 연결/공통 후처리는 호출 기록 대체다.\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths={path for oracle in oracles.values() for path in oracle.exports}
    paths.update((Path(__file__),ROOT/'tools/decomp_bridgeevent_oracle.py',FIXTURE,ROOT/'tools/ghidra/bridgepostpop-functions.json'))
    report=dict(total=len(rows),fpu_controls=list(CONTROLS),
        files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        editions={edition:dict(binary=oracle.spec['binary'],sha256=oracle.sha256,assertions=oracle.assertions,
            native_calls=dict(oracle.native_calls),substitutions=dict(oracle.stub_calls)) for edition,oracle in oracles.items()},
        limits=['다리 postPop의 접두만 실행한다. 연결 객체/소유자 전파와 공통 postPop은 대체한다'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """도구/몸체/fixture/PE SHA와 행 수·실제 몸체 호출·assert 0을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for path,expected in report['files'].items():
        if digest(ROOT/path)!=expected:raise RuntimeError(f'SHA 불일치: {path}')
    rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows)!=report['total']:raise RuntimeError('행 수 불일치')
    for edition,data in report['editions'].items():
        if digest(ROOT/data['binary'])!=data['sha256'] or data['assertions'] or data['native_calls'].get('BridgePost')!=160:
            raise RuntimeError(f'원본 실행 증거 오류: {edition}')
    print(f'bridgepostpop 감사 통과: {report["total"]}개')


def main():
    """접두 기대값 생성과 저장된 증거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    if parser.parse_args().verify:verify()
    else:generate()


if __name__=='__main__':main()
