#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 정밀 PE의 레지던스 event1 좌표/난수/탐색과 전체 결과를 독립 관찰한다."""
import csv
import json
import socket
import struct
import sys
from decomp_assetregular_oracle import AssetRegularOracle, SPEC, KEYS, inputs, CALLER, TEMP, UPDATE, DAMAGE
from decomp_rawfinder_oracle import FinderOracle, ROOT, HEAP, VTABLE, bits, sha
from decomp_bridgedecay_oracle import STOP, FPU_CONTROLS
from unicorn.x86_const import UC_X86_REG_ESP

# 기존 관찰/fixture는 보존하고 좌표 교차 검증을 별도 파일에 기록한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/residence-coordinates-x86.tsv'
REPORT=ROOT/'cpppj/residence-coordinates.json'


class Oracle(AssetRegularOracle):
    """과거 선택 내보내기 대신 현재 PC의 정밀 범위를 사용하고 원본 호출 인수도 관찰한다."""
    def __init__(self,edition):
        """기존 finder/명시 외부 경계를 유지하면서 선택한 실제 Handle/helper만 허용한다."""
        FinderOracle.__init__(self,edition)
        self.p=SPEC[edition];self.entries=set();self.returns=0  # 현재 판본과 정상 반환의 누적 관찰이다.
        selectors=json.loads((ROOT/'tools/ghidra/assetregular-functions.json').read_text(encoding='utf-8'))['editions'][edition]
        selectors+=json.loads((ROOT/'tools/ghidra/frameadvance-functions.json').read_text(encoding='utf-8'))['editions'][edition]
        selected={int(value,16) for value in selectors}  # 주소만 선택하며 결과/범위를 계산하지 않는다.
        if edition=='originals':selected.add(self.p['theme'])
        path=ROOT/f'extracted/refined/{edition}/functions.tsv';self.body_paths.append(path)
        with path.open(encoding='utf-8') as stream:
            functions={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for address in selected:  # 실제 불연속 함수 범위만 실행을 허용한다.
            self.entries.add(address)
            for span in functions[address]['ranges'].split(';'):  # 각 몸체의 양 끝 포함 범위를 보존한다.
                low,high=(int(value,16) for value in span.split('-'))
                self.allowed[low-self.base:high+1-self.base]=b'\x01'*(high+1-low)
        self.stubs={}  # 허용되지 않은 큰 몸체를 새로 성공 처리하지 않는다.
        self.mu.mem_write(CALLER,b'\xd9\x1d'+struct.pack('<I',TEMP)+b'\xe9'+struct.pack('<i',STOP-(CALLER+11)))
        self.put(VTABLE+0x88,UPDATE);self.put(VTABLE+0x98,DAMAGE);self.put(VTABLE+0x74,self.p['owner'])
        self.coordinates=(0,0);self.trace=[]  # 현재 입력 좌표 비트와 실제 호출 인수의 순서다.

    def setup(self,objects):
        """원본 입력의 레지던스 위치만 치환하고 실제 finder 버킷 등록을 재사용한다."""
        changed=[]
        for item in objects:  # 이웃의 고정 입력은 결과에 따라 바꾸지 않는다.
            values=list(item)
            if values[0]==50:values[4:6]=self.coordinates
            changed.append(tuple(values))
        FinderOracle.setup(self,changed)
        for sid in (50,60,61,62):  # 표시 경계의 실제1×1 입력과 같은 저장 해시 단계를 명시한다.
            self.put(self.slot(sid)+(33 if self.stride==50 else 31),1,1)

    def on_instruction(self,mu,address,size,data):
        """실제 난수 호출의 low/high와 finder 사각형을 기록하고 명령을 그대로 실행한다."""
        random=0x4558f0 if self.edition=='originals' else 0x48cc10
        if address in (random,self.finder_spec['Begin']):
            sp=mu.reg_read(UC_X86_REG_ESP);count=2 if address==random else 4
            self.trace.append(('R' if address==random else 'F')+':'+','.join(str(self.read(sp+4+i*4)) for i in range(count)))
        super().on_instruction(mu,address,size,data)


def generate():
    """좌표 경계/난수 입력을 두 판본·두 x87 정밀도로 실행하고 정상 반환 결과만 저장한다."""
    baseline=next(value for value in inputs() if value[0]==0 and value[1]==1)
    coordinates=((50.75,52.5),(118.25,54.25),(145.25,81.25),(220.25,10.25),(10.25,220.25),(5.25,4.75),(246.75,246.75),(80.0,80.0))
    seeds=tuple(range(32))+(0xffffffff,0x12345678,0x80000000,0x0bad0bad)
    rows=[];records={};files=[ROOT/'originals/Netstorm.exe',PathFile(),ROOT/'tools/decomp_assetregular_oracle.py',ROOT/'tools/ghidra/assetregular-functions.json',ROOT/'tools/ghidra/frameadvance-functions.json']
    for edition in SPEC:  # 10.78과 CD10.72의 결과를 서로 독립적으로 관찰한다.
        oracle=Oracle(edition);before=len(rows)
        for x,y in coordinates:  # 축 차이가64인 실제 실패 형태와 양 끝 경계를 포함한다.
            for seed in seeds:  # 난수 상태는 정해진 입력이며 원본 결과를 보고 선택하지 않는다.
                for scene in (0,1,2):  # 변화 없음/장식 앞으로/뒤로의 같은 고정 이웃 입력이다.
                    values=list(baseline);values[7]=seed;values[17]=scene
                    oracle.coordinates=(bits(x),bits(y));observations=[]
                    for control in FPU_CONTROLS:  # 같은 고유 입력의 정밀도 재생은 추가 입력으로 세지 않는다.
                        oracle.trace=[];result=oracle.run(values,control);observations.append((*result,';'.join(oracle.trace)))
                    if observations[0]!=observations[1]:raise RuntimeError('레지던스 좌표 x87 정밀도 차이')
                    rows.append([edition,','.join(map(str,values)),*oracle.coordinates,*observations[0]])
        records[edition]=dict(cases=len(rows)-before,returns=oracle.returns,assertions=oracle.assertions,external=dict(oracle.stub_calls))
        files.extend(oracle.body_paths+[ROOT/oracle.spec['binary']]);print(edition,records[edition],flush=True)
    FIXTURE.write_text('# edition inputs xbits ybits return rng maxscan events raw neighbors calls\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files.extend([FIXTURE,ROOT/'cpppj/src/o/RawAssetRegular.cpp',ROOT/'cpppj/tests/AssetRegularTests.cpp',ROOT/'tools/decomp_rawfinder_oracle.py',ROOT/'tools/decomp_bridgeeffects_oracle.py',ROOT/'tools/decomp_bridgedecay_oracle.py'])
    REPORT.write_text(json.dumps(dict(host=socket.gethostname(),last_full_decompile_host='VM-W11-Codex',last_full_decompile_date='2026-10-16',total=len(rows),editions=records,files={str(path.relative_to(ROOT)).replace('\\','/'):sha(path) for path in dict.fromkeys(files)},limits=['게임/GUI/OS 실행 없음. 기존 finder 입력 등록과 명시 하위 표시 효과 경계를 재사용한다.','선택 범위는 현재 정밀 디컴파일에서 읽는다. 과거 assetregular 보고서/fixture는 덮어쓰지 않는다.','호출 기록은 실제 원본 스택 인수다. C++ 비교는 반환값/공유 난수/최대 탐색/모든 효과/전체 부모 및 이웃 프레임을 사용한다.','Guide/AI전략/미션 완주/GUI 동일성을 증명하지 않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('레지던스 좌표 고유 입력',len(rows))


def PathFile():
    """관찰 도구 자신의 절대 경로를 보고서에 포함한다."""
    from pathlib import Path
    return Path(__file__).resolve()


def verify():
    """고정 출처/반환 횟수/중복 없는 입력과 두 x87 재생 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,digest in report['files'].items():  # 참조한 모든 원본/관찰/제품 파일의 바이트를 비교한다.
        if sha(ROOT/name)!=digest:raise RuntimeError('SHA 불일치 '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows)!=report['total']:raise RuntimeError('행 수 불일치')
    for edition,record in report['editions'].items():  # 판본마다 고유 입력/정상 반환/원본 assert를 확인한다.
        chosen=[row for row in rows if row[0]==edition]
        if len(chosen)!=record['cases'] or record['returns']!=len(chosen)*2 or record['assertions'] or len({tuple(row[:4]) for row in chosen})!=len(chosen):raise RuntimeError('판본 관찰 수 오류')
    print('레지던스 좌표',len(rows),'개 SHA/반환/고유 입력 감사 통과')


# 명시 명령으로만 보호 PE를 읽으며 기존 보고서는 변경하지 않는다.
if __name__=='__main__':
    verify() if '--verify' in sys.argv else generate()
