#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Path Init의 표면0 반환·void Pop·BFS·current 쓰기를 두 보호 PE에서 관찰한다."""
import argparse
import collections
import itertools
import json
from pathlib import Path
from decomp_pathsearch_oracle import SearchOracle,ROOT,RAWPOOL,MAP,PATH,ARENA,bits,digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 원본 가상 Pop 대신 인자/현재 raw 상태를 관찰하는 격리 주소다. 게임/OS 주소를 실행하지 않는다.
TABLE,POP=ARENA+0xcd000,ARENA+0xcff00
FIXTURE=ROOT/'cpppj/tests/fixtures/pathinit-x86.tsv'
REPORT=ROOT/'cpppj/pathinit.json'


def cases():
    """같은/다른 Graph·flags·표면0/존재·부모void·기존 경로를 교차한다."""
    return list(itertools.product((0,9),(0,1),(0,1),(0,1),(0,1)))


class InitOracle(SearchOracle):
    """기존 실제 BFS와 Init 몸체를 실행하고 가상 Pop만 현재 인자 관찰 경계로 받는다."""
    def on_instruction(self,mu,address,size,data):
        """같은 raw 부모에 Pop 좌표/void 효과를 적용하고 thiscall 스택을 정리한다."""
        if address!=POP:return super().on_instruction(mu,address,size,data)
        sp=mu.reg_read(UC_X86_REG_ESP);ret=self.u32(sp);raw=RAWPOOL+5*self.s['stride']
        assert mu.reg_read(UC_X86_REG_ECX)==raw
        x,y,flags=(self.u32(sp+offset) for offset in (4,8,12));assert flags==0
        self.put(raw+14,x);self.put(raw+18,y);self.put(raw+11,self.mu.mem_read(raw+11,1)[0]&~4,1)
        self.events.append(f'pop,{x},{y},{flags}');self.stub_hits['init_pop']+=1
        mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,sp+16);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,case):
        """기존 경로/부모/지도 입력만 공급하고 실제 Init의 출력을 그대로 읽는다."""
        profile,flags,surface,void,legacy=case
        _,_,start,destination=self.setup(profile,1,1,6,legacy,0);self.events=[]
        raw=RAWPOOL+5*self.s['stride'];self.put(raw,TABLE);self.put(TABLE+0x90,POP)
        self.mu.mem_write(POP,b'\xc2\x0c\x00')
        self.put(raw+11,4 if void else 0,1);self.put(raw+14,bits(start[0]));self.put(raw+18,bits(start[1]))
        self.put(PATH+self.m['current'],1 if legacy else 0)
        if not surface:self.put(MAP+(10*256+10)*2,0,2)
        entry=0x48a660 if self.patch else 0x47ec60
        self.invoke(entry,(5,bits(destination[0]),bits(destination[1]),flags,bits(start[0]),bits(start[1])),PATH)
        points,distance,queue,mask=self.observed()
        return [';'.join(self.events) or '-',self.u32(PATH+self.m['current']),self.u32(PATH+0x10),
            self.mu.mem_read(raw+11,1)[0],self.u32(raw+14),self.u32(raw+18),points,distance,queue,mask]


def generate():
    """실제 정상 반환/허용 범위 결과만 저장하며 이전 BFS 입력은 신규 수에 다시 더하지 않는다."""
    rows=[];entries=collections.Counter();external=collections.Counter();files={Path(__file__),FIXTURE}
    for edition in ('originals','originalCD'):
        oracle=InitOracle(edition)
        # 각 입력에서 원본 Init이 직접 검색을 호출하며 원본 출력을 C++ 계산으로 만들지 않는다.
        for case in cases():rows.append([edition,','.join(map(str,case)),*oracle.run(case)])
        entries.update(oracle.hits);external.update(oracle.stub_hits)
        files.add(ROOT/f'{edition}/{"Netstorm.exe" if edition=="originals" else "NETSTORM.EXE"}')
        # 실행한 정확한 불연속 몸체와 재사용 도구를 함께 기록한다.
        for name in ('pathruntime','pathruntimehelpers','pathmath','pathsearch'):
            for leaf in ('creation.c','functions.tsv'):files.add(ROOT/f'extracted/{name}/{edition}/{leaf}')
        print(f'{edition}: 실제 Init {len(cases())}개',flush=True)
    FIXTURE.write_text('# edition input events current count state x y points distance queue mask\n'+'\n'.join('\t'.join(map(str,r)) for r in rows)+'\n',encoding='utf-8',newline='\n')
    for name in ('tools/decomp_pathsearch_oracle.py','tools/decomp_pathruntime_oracle.py','tools/decomp_reward_oracle.py',
        'cpppj/src/o/RawPathSearch.h','cpppj/src/o/RawPathSearch.cpp','cpppj/src/o/RawPathProcess.h','cpppj/src/o/RawPathProcess.cpp',
        'cpppj/tests/RawPathSearchTests.cpp'):files.add(ROOT/name)
    REPORT.write_text(json.dumps(dict(total=len(rows),host='VM-W11-CODEX',entries=dict(entries),external=dict(external),os_calls=0,
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},limits=[
            'Init·표면 조회·BFS·경로 축약·current 쓰기는 실제 명령, 부모 가상 Pop은 명시 좌표/void 관찰 경계다.',
            '부착/직렬화·명령·피해/운반·의식 및 캠페인 완주 근거로 세지 않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """PE/소스/입력/SHA와 실제 Init 정상 진입·Pop·BFS 집계를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['files'].items():assert digest(ROOT/name)==expected,name
    rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    assert len(rows)==report['total']==2*len(cases()) and report['os_calls']==0
    for edition in ('originals','originalCD'):
        chosen=[r for r in rows if r[0]==edition];assert len(chosen)==len(cases())
        for row,case in zip(chosen,cases(),strict=True):assert len(row)==12 and row[1]==','.join(map(str,case))
        assert report['entries']['0048a660' if edition=='originals' else '0047ec60']==len(cases())
    assert report['external']['init_pop']==sum(c[2] and c[3] for c in cases())*2
    print(f'실제 Path Init {len(rows)}개 SHA/입력 감사 통과')


# 명시 실행에서만 생성 또는 SHA 감사를 시작한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate()
