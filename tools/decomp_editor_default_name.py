#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""State1의 실제 이름 선택 앞단과 기본 이름00460c20을 파일 설치 직전까지 관측한다."""
import argparse
import hashlib
import itertools
import json
import platform
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TEXT, STACK, STOP
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EBP

CURRENT,PENDING,CONFIGURED=TEXT,TEXT+0x400,TEXT+0x800  # 각 C문자열 물리는 서로 겹치지 않는다.


class Oracle(MemoryOracle):
    """기본 이름의 조건/호출은 실제 명령으로 실행하고 Config/이름 부모만 명시 장치로 둔다."""
    def instruction(self,mu,address,size,data):
        """현재 문자열/미션과 두 이름 선택의 순서를 기록하며 전체 파일 설치는 실행하지 않는다."""
        if address==0x4b7350:
            self.completed=True
            mu.emu_stop()
            return
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,4)
        if address==0x4e65ed:
            left,right=self.string(args[0]),self.string(args[1])
            self.events.append('C:'+left.hex()+':'+right.hex())
            self.ret(int(left.lower()!=right.lower()))
            return
        if address==0x460c40:
            self.events.append('M:'+self.string(args[0]).hex())
            self.ret(self.recognized)
            return
        if address==0x463b20:
            self.events.append('G')
            self.put(0x594fa0,0)
            self.ret()
            return
        if address==0x4410f0:
            assert self.string(args[0])==b'newFort' and args[1:]==(0,0,0)
            self.events.append('Q:newFort')
            self.ret(CONFIGURED)
            return
        if address==0x460980:
            name=self.string(args[0])
            self.events.append('N:'+name.hex())
            self.names+=1
            if self.names==1 and self.mutation:
                name=b'.fromSetter' if self.mutation==1 else b''
            mu.mem_write(CURRENT,name+b'\0')
            self.ret()
            return
        if not (0x4b72eb<=address<0x4b7350 or 0x460c20<=address<0x460c3a):
            raise RuntimeError(f'기본 Editor 이름 범위 밖 {address:08x}')

    def run(self,case):
        """명시 문자열/미션/분류/현재값 변이를 실제 주소에 놓고 끝 문자열을 읽는다."""
        current,pending,configured,mission,self.recognized,self.mutation=case
        self.events,self.names,self.completed=[],0,False
        self.mu.mem_write(STACK-0x1000,bytes(0x1100))
        for pointer,text in ((CURRENT,current),(PENDING,pending),(CONFIGURED,configured)):
            self.mu.mem_write(pointer,text.encode('ascii')+b'\0')  # 원문 기대 결과와 독립인 고정 입력이다.
        self.put(0x55b490,CURRENT)
        self.put(0x55b494,PENDING)
        self.put(0x594fa0,mission)
        self.put(STACK,STOP)
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_EBP,0)
        self.mu.emu_start(0x4b72eb,STOP+1,count=5000)
        assert self.completed,case
        return dict(current=self.string(CURRENT).hex() or '-',mission=self.words(0x594fa0,1)[0],events=';'.join(self.events))


def main():
    """원문252입력을 재관측해 일치한 최초 결과만 새 디렉터리에 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path)
    out=parser.parse_args().output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    pairs=(('',''),('.temp','.TEMP'),('Existing','Existing'),('Old',''),('Old','.temp'),('Old','Target'),('Existing','eXiStInG'))
    cases=[[*pair,configured,mission,recognized,mutation] for pair,configured,mission,recognized,mutation in itertools.product(pairs,('Default','','.again'),(0,1),(0,1),(0,1,2))]
    oracle=Oracle()
    observed=[oracle.run(case) for case in cases]
    assert observed==[oracle.run(case) for case in cases]
    with (out/'editor-default-name-x86.jsonl').open('x',encoding='utf-8',newline='\n') as stream:
        for case,result in zip(cases,observed):
            stream.write(json.dumps(dict(input=case,**result),separators=(',',':'))+'\n')  # 실제 호출과 끝 C문자열만 보존한다.
    with (out/'editor-default-name-x86.tsv').open('x',encoding='utf-8',newline='\n') as stream:
        stream.write('# current/pending/configured/mission/recognized/mutation / finalCurrent/mission/events\n')
        for case,result in zip(cases,observed):
            encoded=[text.encode('ascii').hex() or '-' for text in case[:3]]
            stream.write('\t'.join(map(str,(*encoded,*case[3:],result['current'],result['mission'],result['events'])))+'\n')
    report=dict(unit=653,host=platform.node(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-23',cases=len(cases),observations=len(cases)*2,differences=0,executableSha256=hashlib.sha256((ROOT/'originals/Netstorm.exe').read_bytes()).hexdigest(),scope='4b72eb..4b7350와 실제460c20. 비교/Config/Guide/전체 이름 부모는 명시 장치. 파일/게임/GUI/통신 실행 없음.')
    (out/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':
    main()
