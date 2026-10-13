#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Builder 전체→실제 Regular 생성자 필드로 event0/payload현재float 계약을 독립 확인한다."""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_constructionbuilder_oracle import BuilderOracle, SPEC, KEYS, case, double_words, ROOT, digest
from decomp_returnpiece_oracle import ALLOCATION
from decomp_owner_oracle import OwnerOracle
from decomp_damageablepredestroy_oracle import CONTROLS, bits
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

# 생성자 실제 BaseProcess 부착만 대체한다. event/payload/time/count의 생성 몸체는 그대로 실행한다.
ATTACH={'originals':0x41bd20,'originalCD':0x48f590}
FIXTURE=ROOT/'cpppj/tests/fixtures/constructiontimer-x86.tsv'
REPORT=ROOT/'cpppj/constructiontimer.json'


class TimerOracle(BuilderOracle):
    """기존 경계에서 Regular 대체만 없애고 실제 constructor의 저장 필드를 관찰한다."""
    def __init__(self,edition):
        """정상 생성자 범위와 부착 경계·반환 시점 관찰을 준비한다."""
        super().__init__(edition);self.stubs.pop(self.r['timer']);self.pending=None
        self.timer_exports=[ROOT/f'extracted/constructiontimer/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with self.timer_exports[1].open(encoding='utf-8') as handle:
            # 새로운 실제 Regular 생성자만 기존 허용 목록에 더한다.
            for row in csv.DictReader(handle,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def put(self,key,value):
        """기본 준비값만 선택한 실제 payload 비트로 교체하고 callback 변형은 그대로 둔다."""
        super().put(key,self.payload_bits if key=='event' and value==0x25b else value)

    def on_instruction(self,mu,address,size,data):
        """전체 Builder/Regular는 actual 명령, attach만 명시 경계이며 반환 직후 실제 필드를 읽는다."""
        if address==self.r['timer']:
            self.event('T',*self.args(3));self.pending=struct.unpack('<I',mu.mem_read(mu.reg_read(UC_X86_REG_ESP),4))[0]
            self.write_ranges.append((ALLOCATION,ALLOCATION+0x28))
        elif address==ATTACH[self.edition]:
            process_type,parent,form,flags=self.args(4)
            if process_type!=46 or form!=0 or flags!=0x50 or mu.reg_read(UC_X86_REG_ECX)!=ALLOCATION:raise RuntimeError('Regular 실제 부착 인자 오류')
            mu.mem_write(ALLOCATION+4,struct.pack('<II',999,parent));self.stub_calls['attach']+=1;self.ret(0,16);return
        elif self.pending is not None and address==self.pending:
            event,payload,count=struct.unpack('<III',mu.mem_read(ALLOCATION+0x18,12));parent=struct.unpack('<I',mu.mem_read(ALLOCATION+8,4))[0]
            low,high=struct.unpack('<II',mu.mem_read(ALLOCATION+0x10,8));self.event('J',event,payload,parent,low,high,count);self.pending=None
        super().on_instruction(mu,address,size,data)


def inputs():
    """공급원/할당실패/실제 payload·생성 중 변형의 입력을 만들며 기대 필드를 계산하지 않는다."""
    result=[];low,high=double_words(2.5)
    for sources,allocation,payload,mutation in itertools.product(range(4),(0,1),(0,bits(0.05),bits(0.25),bits(-1.0),0x7fc01234,0x12345678),(0,5)):
        result.append((case(sources=sources,allocation=allocation,mutate=mutation,nowLow=low,nowHigh=high),payload))
    return result


def generate():
    """두 PE/두 정밀도로 실제 Builder→Regular 필드/ABI를 실행해 고유 fixture를 만든다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_constructionbuilder_oracle.py',ROOT/'tools/ghidra/constructiontimer-functions.json',FIXTURE}
    for edition in ATTACH:
        oracle=TimerOracle(edition)
        for item,payload in inputs():
            oracle.payload_bits=payload;observed=[oracle.run_builder('Move',item,control) for control in CONTROLS]
            assert observed[0]==observed[1],(edition,item,payload)
            rows.append([edition,','.join(str(item[key]) for key in KEYS),str(payload),*observed[0]])
        paths.update(oracle.exports);paths.update(oracle.timer_exports);paths.add(ROOT/oracle.spec['binary'])
        editions[edition]=dict(cases=len(inputs()),native=dict(oracle.native_calls),boundaries=dict(oracle.stub_calls))
        print(f'{edition} Builder→actual Regular {len(inputs())}개 통과',flush=True)
    FIXTURE.write_text('# actual Builder→actual Regular ctor; T:event:parent:payloadBits, J:event:payloadBits:parent:timeLow:timeHigh:count\n'+
        '\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(host='VM-W11-CODEX',date='2026-10-13',total=len(rows),editions=editions,
        files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        limits=['실제 Builder전체와 Regular생성자의 event/payload/time/count·정상ret12 실행','실제 ProcessForm/Kernel 부착은 명시 경계, C++ 통합에서 확인','기존1110 builder입력 재실행과별개의새192개입력','게임/GUI실행없음']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """현재 원본/함수/코드/fixture SHA 및 실제 생성자 호출과 새 입력 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['files'].items():assert digest(ROOT/name)==expected,name
    rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
    assert len(rows)==report['total']==2*len(inputs())
    for edition in ATTACH:
        assert report['editions'][edition]['native'][f"{SPEC[edition]['timer']:08x}"]>0
        chosen=[row for row in rows if row[0]==edition]
        assert all(token.split(':')[1]=='0' for row in chosen for token in row[3].split(';') if token.startswith('J:'))
    print(f'Builder→actual Regular SHA/생성자/event0 감사 {len(rows)}개 통과')


def main():
    """생성/보존 근거의 재감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');options=parser.parse_args()
    verify() if options.verify else generate()


if __name__=='__main__':main()
