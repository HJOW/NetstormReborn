#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""작은 투명 캡처에서 원문 Burning이 픽셀 뒤에만 rowTaken을 읽는지 실제 두 PE로 관측한다."""
import collections
import json
import platform
from pathlib import Path
from decomp_burningsources_oracle import BurningSourcesOracle,sample,KEYS,IMAGE,ROWS
from decomp_owner_oracle import ROOT,digest
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_FPCW

DIMENSIONS=((2,7),(3,6),(4,5),(5,9),(6,8),(7,7),(8,6),(9,5),(16,12)) # 마지막은 배열 끝 후보가 없는 기존 크기의 대조 입력이다.
SEEDS=(0,0x12345678) # 결과를 계산하지 않고 후보 좌표의 실제 난수 소비를 교차한다.
PRECISIONS=(0x027f,0x037f) # 실제53/64비트 x87 저장 환경을 구별한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/burning-sources-corner-x86.tsv' # 기존326행 fixture와 분리된 원문 출력이다.
REPORT=ROOT/'cpppj/burning-sources-corner-native-20261027.json' # 최초 반환/읽기/파일 해시를 보존한다.


class CornerOracle(BurningSourcesOracle):
    """기존 실제 Source/난수/HP/Append 몸체를 유지하며 읽기 주소만 추가 관측한다."""
    def __init__(self,edition):
        """두 보호 PE의 기존 허용 범위와 별도 메모리 읽기 관찰자를 준비한다."""
        self.precision=0x027f;super().__init__(edition);self.case=None;self.pixel_sites=collections.defaultdict(list);self.row_reads=[];self.readable_rows=0
        self.mu.hook_add(UC_HOOK_MEM_READ,self.observe_read)

    def on_instruction(self,mu,address,size,data):
        """실제 첫 명령 앞에서 선택한 x87 환경만 공급한다. 함수 본체/하위는 원문이다."""
        if address==self.s['source']:
            mu.reg_write(UC_X86_REG_FPCW,self.precision)
        super().on_instruction(mu,address,size,data)

    def observe_read(self,mu,access,address,size,value,data):
        """실행 중 캡처 픽셀과 행 배열의 실제 읽기 주소/명령 위치만 기록한다."""
        if self.case is None:
            return
        if IMAGE<=address<IMAGE+self.case['width']*self.case['height']:
            self.pixel_sites[mu.reg_read(UC_X86_REG_EIP)].append(address-IMAGE)
        if ROWS<=address<ROWS+0x1000:
            self.row_reads.append(address-ROWS)

    def observe(self,case,precision):
        """최초 Source의300후보와 폴백 픽셀 읽기를 명령 위치별로 구별한다."""
        self.precision=precision;self.pixel_sites=collections.defaultdict(list);self.row_reads=[]
        self.readable_rows=(case['width']+case['height'])//5+(1 if self.edition=='originalCD' and case['width']<10 else 0) # CD는 행을 먼저 읽으므로 추가 접근 가능한0BYTE를 명시 입력으로 공급한다. 실제 caller 힙 복원으로 합산하지 않는다.
        result=self.run(case);candidates=[(site,addresses) for site,addresses in self.pixel_sites.items() if len(addresses)==300]
        if result[0]!='return' or len(candidates)!=1:
            raise RuntimeError('원문 투명 Source의 정상 반환/300후보 읽기 지점 불일치')
        site,addresses=candidates[0];width,height=case['width'],case['height'];logical_rows=(width+height)//5
        beyond=sum(((offset%width)+(offset//width-1))//5>=logical_rows for offset in addresses)
        expected_reads=0 if self.edition=='originals' else 300
        if len(self.row_reads)!=expected_reads or any(offset>=self.readable_rows for offset in self.row_reads) or (width<10 and not beyond):
            raise RuntimeError('투명 픽셀의 원문 행 단락/끝 후보 관측 불일치: '+str(dict(edition=self.edition,input=case,precision=precision,rowReads=len(self.row_reads),beyond=beyond,sites={hex(k):len(v) for k,v in self.pixel_sites.items()})))
        result[3]=bytes(self.mu.mem_read(ROWS,self.readable_rows)).hex() or '-'
        return result,dict(precision=precision,normal=True,candidatePixelSite=hex(site),candidatePixelReads=len(addresses),beyondLogicalRowCandidates=beyond,rowReads=len(self.row_reads),beyondLogicalRowReads=sum(offset>=logical_rows for offset in self.row_reads),readableRows=self.readable_rows)


def main():
    """기존 gold를 수정하지 않고 실제 두 판본/두 정밀도의 결과가 같은 새 행만 저장한다."""
    if FIXTURE.exists() or REPORT.exists():
        raise FileExistsError('최초 corner fixture/보고서를 덮어쓰지 않는다')
    old_fixture=ROOT/'cpppj/tests/fixtures/burningsources-x86.tsv';old_hash=digest(old_fixture);rows=[];observations=[];files={Path(__file__).resolve(),old_fixture}
    for edition in ('originals','originalCD'): # 각 판본은 독립 원문 명령/전역/구조체 폭으로 실행한다.
        oracle=CornerOracle(edition);files.update(oracle.exports);files.add(ROOT/oracle.spec['binary'])
        for width,height in DIMENSIONS: # 작은 폭의 실제 끝 후보와 기존 큰 크기를 교차한다.
            for seed in SEEDS: # 각 입력은 현재 픽셀/HP/난수일 뿐 출력은 원문이 정한다.
                case=sample(width=width,height=height,seed=seed,mask=1);outputs=[];reads=[]
                for precision in PRECISIONS: # 같은 입력의53/64비트 반환과 읽기 경계를 각각 보존한다.
                    result,observed=oracle.observe(case,precision);outputs.append(result);reads.append(observed)
                if outputs[0]!=outputs[1]:
                    raise RuntimeError('corner 원문 정밀도별 실제 출력 불일치')
                rows.append([edition,*[case[key] for key in KEYS],oracle.readable_rows,*outputs[0]])
                observations.append(dict(edition=edition,input=case,readableRows=oracle.readable_rows,runs=reads))
    assert digest(old_fixture)==old_hash
    with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# 원문 Source 투명 모서리. 10.78은 픽셀 우선, CD는 행 우선/추가 읽기 가능한0BYTE를 명시 입력으로 공급한다.\n')
        stream.write('\n'.join('\t'.join(map(str,row)) for row in rows)+'\n')
    files.add(FIXTURE);report=dict(host=platform.node(),cases=len(rows),runs=sum(len(o['runs']) for o in observations),normalReturns=sum(len(o['runs']) for o in observations),originalGameExecutions=0,oldFixtureUnchanged=True,
        boundaries=[],inputNotes=['캡처/행표식/기존 입자는 명시 입력이다.','10.78은 투명 픽셀에서 행을 읽지 않는다.','CD는 행을 먼저 읽으므로 작은 입력은 논리 행 뒤 추가 접근 가능한0BYTE1개를 명시한다. 실제 caller의 힙 패딩/안전성 복원이 아니다.'],observations=observations,sha256={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)})
    with REPORT.open('x',encoding='utf-8',newline='\n') as stream:
        json.dump(report,stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('observations','sha256')},ensure_ascii=False))


if __name__=='__main__':
    main()
