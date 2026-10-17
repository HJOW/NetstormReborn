#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 펜스 연결 정리·이웃 갱신·전처리의 스냅샷/효과/전체 raw를 관찰한다."""
import argparse
import csv
import itertools
import json
import re
import socket
import struct
from decomp_cursor_mana import Oracle as ManaOracle, ROOT, STACK, STOP, POOL, PROCESS, VTABLE, DETACH, UNPOP, digest
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

ENTRIES=(0x4577f0,0x4578f0,0x457990)  # 실제 세 펜스 몸체다.
FIXTURE=ROOT/'cpppj/tests/fixtures/fence-lifetime-x86.tsv'  # 기존 펜스 프레임 자료와 별개인 새 원문이다.
REPORT=ROOT/'cpppj/fence-lifetime.json'  # 현재 호스트·가상 표 증거·효과·반환과 SHA다.


def cases():
    """현재 parent/이웃/extra와 할당·변경 입력만 조합하고 기대값을 계산하지 않는다."""
    result=[]
    # 연결 정리/갱신/전처리의 원본 현재 상태를 교차한다.
    for operation,dead,extra,neighbor_state,kind,mask,reserve in itertools.product((0,1,2),(0,2),(0,8),(0,2),(136,128),(0,15),(0,3)):
        result.append((operation,dead,extra,neighbor_state,kind,mask,reserve,0,1,0x40))
    # 각 효과가 다음 현재 입력을 바꾸는 순서를 따로 관찰한다.
    for operation,mutation in itertools.product((0,1,2),range(1,6)):
        result.append((operation,0,0,0,136,0,3,mutation,2,0x12345678))
    return list(dict.fromkeys(result))


class Oracle(ManaOracle):
    """기존 격리 ABI/메모리 검사를 쓰고 이웃/연결 효과·예약/기존 Damageable만 경계로 둔다."""
    def __init__(self):
        """보호 PE의 현재 세 정밀 함수 범위만 추가한다."""
        self.fence_ready=False;super().__init__();self.fence_ready=True
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            rows={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        # 함수 사이의 미확인 명령은 실행하지 않는다.
        for entry in ENTRIES:
            for part in rows[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def mutate(self,kind):
        """첫 선택 효과만 다음 처리의 현재 좌표/상태/타입을 바꾼다."""
        if self.mutated:return
        mode=self.case[7]
        if (mode,kind) not in ((1,'M'),(2,'D'),(3,'B'),(4,'L'),(5,'N')):return
        self.mutated=True
        if mode==1:self.put(POOL+50*50+14,struct.unpack('<I',struct.pack('<f',125.5))[0]);self.put(POOL+50*50+34,3,1)
        if mode==2:self.put(POOL+52*50+11,2,1)
        if mode==3:self.put(POOL+50*50+10,128,1)
        if mode==4:self.put(POOL+50*50+11,2,1)
        if mode==5:self.put(POOL+53*50+11,2,1)

    def instruction(self,mu,address,size,data):
        """실제 몸체를 실행하며 명시한 현재 이웃과 가상/기존 모듈 효과만 기록한다."""
        if not self.fence_ready:return super().instruction(mu,address,size,data)
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        esp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<9I',mu.mem_read(esp+4,36));ecx=mu.reg_read(UC_X86_REG_ECX);sid=(ecx-POOL)//50
        if address in (0x455fb0,0x456220):
            self.instructions+=1;direction=args[0];allied=address==0x455fb0;index=direction//2
            result=(51+(index if allied else 3-index)) if sid==50 else (50 if not direction else 0)
            if self.case[5]&(1<<index):result=0
            self.event('S',sid,direction,int(allied),result);self.ret(result,4);return
        if address==0x4566d0:self.instructions+=1;self.event('M',sid,args[0]);self.mutate('M');self.ret(0,4);return
        if address==0x455b20:self.instructions+=1;self.event('L',*args[:9]);self.mutate('L');self.ret();return
        if address==0x457460:self.instructions+=1;self.event('B',sid);self.mutate('B');self.ret();return
        if address==0x44b4b0:self.instructions+=1;self.event('P',sid,args[0]);self.ret(0,4);return
        if address==0x4e4391:
            self.instructions+=1;self.event('N',args[0]);assert args[0]==0x28;success=bool(self.case[6]&(1<<self.reserved));self.reserved+=1;self.mutate('N');self.ret(PROCESS if success else 0);return
        if address==0x496e80:self.instructions+=1;self.event('A',*args[:3]);self.ret(PROCESS,12);return
        return super().instruction(mu,address,size,data)

    def sample(self,case,precision):
        """새 raw5개로 실제 한 몸체를 호출하고 가상 효과와 모든250바이트를 읽는다."""
        self.case=case;self.events=[];self.mutated=False;self.reserved=0;self.put(0,0);self.put(0x5c8464,POOL);self.put(0x5c847c,24000)
        # 부모와 네 이웃의 원문 입력을 독립 표식으로 채운다.
        for sid in range(50,55):
            raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=136 if sid==50 else case[4];raw[11]=case[1] if sid==50 else case[3];raw[34]=case[8] if sid==50 else 2;raw[40]=case[2] if sid==50 else 0;struct.pack_into('<2f',raw,14,float(100+sid),float(120+sid));self.mu.mem_write(POOL+sid*50,bytes(raw))
        self.call(ENTRIES[case[0]],POOL+50*50,struct.pack('<I',case[9]) if case[0]==2 else b'',4 if case[0]==2 else 0,precision);self.returns+=1;raw=[]
        # 합성 가상 표만1로 바꾸고 나머지 바이트는 모두 보존한다.
        for sid in range(50,55):
            data=bytearray(self.mu.mem_read(POOL+sid*50,50));struct.pack_into('<I',data,0,1);raw.append(data.hex())
        return ';'.join(self.events) or '-', ''.join(raw)


def main():
    """두 정밀도 결과와 현재 일반 피해 가능 객체의 공통 가상 표를 보존한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in facts['sha256'].items():assert digest(ROOT/path)==wanted,path
        rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==facts['uniqueInputs'];print('펜스 수명 SHA 감사:',len(rows));return
    oracles=[Oracle(),Oracle()];rows=[]
    # 실제 두 원문의 전체 바이트/효과만 기대값으로 사용한다.
    for case in cases():
        result=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert result[0]==result[1];rows.append((','.join(map(str,case)),*result[0]))
    source=(ROOT/'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8');body=re.search(r'kPlainDamageableTables\{([^}]+)\}',source).group(1);tables=[int(v,16) for v in re.findall(r'0x[0-9a-f]+',body)]
    # 이름을 추정하지 않고 실제+10/+14/+18 세 진입점의 일치를 확인한다.
    for table in tables:assert [oracles[0].get(table+offset) for offset in (0x10,0x14,0x18)]==[0x4af780,0x44b4b0,0x44b9e0]
    assert [oracles[0].get(0x509470+offset) for offset in (0x10,0x14,0x18)]==[0x4af780,0x457990,0x44b9e0]
    FIXTURE.write_text('# input10 events raw5slots\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/decomp_cursor_mana.py','tools/decomp_fence_lifetime.py','cpppj/src/client/RawCampaignSession.cpp',FIXTURE.relative_to(ROOT).as_posix())  # 실제 관측과 가상 표 목록의 현재 바이트다.
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',entries=[f'{entry:08x}' for entry in ENTRIES],uniqueInputs=len(rows),normalReturns=sum(oracle.returns for oracle in oracles),instructions=sum(oracle.instructions for oracle in oracles),plainDamageableVtables=[f'{table:08x}' for table in tables],precisionDifferentInputs=[],sha256={path:digest(ROOT/path) for path in paths},limitations=['세 펜스 몸체는 실제 명령이며 이웃/표식/연결선/재생성/기존 Damageable/Regular 생성은 명시 경계다.','원본 전처리는 네 일반 이웃을 저장하지만 첫 두 칸에만 예약한다. 실제 GUI/플레이 증거는 아니다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('펜스 수명:',len(rows),'고유 입력, 두 정밀도 차이0')


if __name__=='__main__':main()
