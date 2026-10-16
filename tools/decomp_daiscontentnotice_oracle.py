#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다이스 실제 내용물 알림+24의 부착/제거·제단 plain 프레임/WORD·로컬cue를 제한 x86에서 관찰한다."""
import argparse,csv,itertools,json,struct
from pathlib import Path
from decomp_altarsacrifice_oracle import AltarOracle,ROOT,HEAP,VTABLE,STACK,STOP,FPU_CONTROLS,bits,sha
from unicorn.x86_const import *

SPEC={'originals':dict(notice=0x449660,cue=0x542a50,priest=0x5412d0),'originalCD':dict(notice=0x4816c0,cue=0x512bd8,priest=0x51cbbc)} # 실제 가상 알림과 현재 전역 주소다.
FIXTURE=ROOT/'cpppj/tests/fixtures/daiscontentnotice-x86.tsv' # 현재 입력/효과/raw의 독립 원본 결과다.
REPORT=ROOT/'cpppj/daiscontentnotice.json' # 새1/1 몸체만의 출처와 명시 공간/장치 경계다.

def inputs():
    """현재 reason/kind/owner/로컬/제단 존재/WORD를 교차한다."""
    result=[]
    for reason,kind,owner,local,exists,word in itertools.product((0,1,2,3),(0,158,165),(0,1,2),(1,2),(0,1),(0,2,0x82,0x1234,0xffff)):
        result.append(dict(reason=reason,kind=kind,owner=owner,local=local,exists=exists,word=word,first=3))
    # plain 쓰기는 현재 제단 타입의 첫표를 읽으며 기존 현재frame로 대신하지 않는다.
    for first in (1,7):result.append(dict(reason=1,kind=158,owner=1,local=1,exists=1,word=0x82,first=first))
    return result

class NoticeOracle(AltarOracle):
    """기존 typedAltar/Type/WORD helper를 실행하고 새1/1 원본 범위만 허용한다."""
    def __init__(self,edition):
        """현재 읽기 전용 내보내기 범위와 새 cue 전역을 추가한다."""
        super().__init__(edition);self.n=SPEC[edition];self.notice_returns=0
        paths=[ROOT/f'extracted/daiscontentnotice/{edition}/{suffix}' for suffix in ('creation.c','functions.tsv')];self.paths.extend(paths);self.paths.append(ROOT/'tools/ghidra/daiscontentnotice-functions.json')
        with paths[1].open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):
                self.entries[int(row['entry'],16)]=row['entry']
                for span in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in span.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\1'*(high+1-low)

    def run(self,value,control):
        """실제 typed 조회와 plain 쓰기/가상 요청 뒤 raw 전체를 직접 읽는다."""
        saved_hits=self.hits.copy();saved_stubs=self.stub_calls.copy()
        base=dict(mode='count',daisFrame=14,altarFrame=13,count=0,seed=0,boss=1,reserve=0,captured=0,oldOwner=2,alliance=0,local=value['local'],suppress=0,forms=0,extra=0,owner=value['owner']);super().run(base,control);self.hits=saved_hits;self.stub_calls=saved_stubs
        self.table(164,'A'*value['first']+'O'*4+'P'*8);self.put(self.slot(10)+18,value['kind'],2);self.put(self.slot(10)+12,value['word'],2);self.put(self.n['cue'],99);self.put(self.n['priest'],158)
        if not value['exists']:
            # 실제 해시에서 제단을 제외하며 읽지 않는 슬롯의 raw는 보존한다.
            address=self.bases[1]+((41//2)*128+40//2)*2;assert self.read(address,2)==7;self.put(address,self.read(self.slot(7)+4,2),2)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,self.slot(5));self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0);self.mu.mem_write(0,bytes(4));self.mu.mem_write(STACK,struct.pack('<III',STOP,10,value['reason']));self.events=[];self.write_ranges=[(HEAP,HEAP+0x10000)];self.mu.emu_start(self.n['notice'],STOP,count=4000000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+12 and self.mu.reg_read(UC_X86_REG_FPCW)==control and not(self.mu.reg_read(UC_X86_REG_FPSW)&0x3800) and self.read(0)==0
        self.notice_returns+=1;return [';'.join(self.events) or '-', ';'.join(bytes(self.mu.mem_read(self.slot(sid),self.stride)).hex() for sid in (5,7,10))]

def generate(smoke):
    """새1/1의 동일입력을 두 x87에서 대조하고 기존 준비 호출은 합산하지 않는다."""
    cases=inputs();cases=cases[::61] if smoke else cases;rows=[];editions={};files={Path(__file__),FIXTURE}
    files.update(ROOT/name for name in ('cpppj/src/o/RawDaisContentNotice.h','cpppj/src/o/RawDaisContentNotice.cpp','cpppj/tests/DaisContentNoticeTests.cpp','tools/decomp_altarsacrifice_oracle.py','tools/decomp_daisregular_oracle.py','tools/decomp_runepostpop_oracle.py','tools/decomp_templeritual_oracle.py','tools/decomp_rawfinder_oracle.py','tools/decomp_bridgeeffects_oracle.py','tools/decomp_bridgedecay_oracle.py'))
    for edition in SPEC:
        oracle=NoticeOracle(edition)
        for index,case in enumerate(cases):
            try:result=oracle.run(case,FPU_CONTROLS[0]);assert oracle.run(case,FPU_CONTROLS[1])==result
            except Exception as error:raise RuntimeError(f'{edition} #{index} {case}: {error}') from error
            rows.append([edition,json.dumps(case,sort_keys=True,separators=(',',':')),*result])
        editions[edition]=dict(returns=oracle.notice_returns,entries=dict(oracle.hits),boundaries=dict(oracle.stub_calls),asserts=oracle.assertions);files.update(oracle.paths);files.add(ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'))
    FIXTURE.write_text('# edition inputJSON effects raw5,7,10\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',decompile_date='2026-10-14',total=len(rows),smoke=smoke,game_runs=0,editions=editions,files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(files)},scope='실제Dais+24·typedAltar/현재타입첫표/WORD쓰기/현재owner·cue. 가상Unpop/Repop와cue장치는명시경계. 전체같은Host/Pool 희생소비자는후속이며기존관찰은합산하지않는다.'),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('다이스 포획 내용물 알림 독립 원본:',len(rows))

def verify():
    """현재 source/fixture/원본 출처와 고유 입력/두 정상 반환을 전수 감사한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==data['total'] and not data['smoke'] and data['game_runs']==0
    assert len({(row[0],row[1]) for row in rows})==len(rows)
    for name,value in data['files'].items():assert sha(ROOT/name)==value,name
    for edition,item in data['editions'].items():assert item['returns']==2*sum(row[0]==edition for row in rows) and item['asserts']==0
    print('다이스 내용물 알림 현재SHA·원본반환 감사:',len(rows))

# 명시 CLI 요청에서만 새 독립 관찰을 실행하거나 보존 자료를 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
