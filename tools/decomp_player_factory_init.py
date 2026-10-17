#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제4910a0의9개 Player 공장 목록 할당/용량/초기 기록을 격리 관측한다."""
import argparse,json,struct
from decomp_mana_bolt_arrival import Oracle as BaseOracle,ROOT,POOL,STOP,STACK,sha
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW

FIXTURE=ROOT/'cpppj/tests/fixtures/player-factory-init-x86.tsv' # 원본 초기화 기록이며 기존 Player fixture를 바꾸지 않는다.
REPORT=ROOT/'cpppj/player-factory-init.json' # 현재 PC/실제 호출/입력 경계/SHA다.

class Oracle(BaseOracle):
    """확보와 동맹 설정만 경계로 두고 Player 초기화의 전체 몸체를 실행한다."""
    def write(self,mu,access,address,size,value,data):
        """원본이 초기화하는 Player 배열/두 전역에만 쓰기를 추가 허용한다."""
        if POOL<=address and address+size<=POOL+0x654:return
        if address in (0x59534c,0x5951f8,0x5951fc) and size==4:return
        return BaseOracle.write(self,mu,access,address,size,value,data)

    def instruction(self,mu,address,size,data):
        """실제 할당 크기와9개의 동맹 요청을 기록한다. 예상 용량을 쓰지 않는다."""
        if address in (0x43fa80,0x4e4391,0x490ea0):
            self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);arg=int.from_bytes(mu.mem_read(sp+4,4),'little')
            if address==0x43fa80:assert arg==0x654;result=POOL;self.events.append(['malloc',arg])
            elif address==0x4e4391:
                self.events.append(['new',arg]);result=POOL+0x10000+self.allocated*0x100;self.allocated+=1
            else:
                mask=int.from_bytes(mu.mem_read(sp+8,4),'little');self.events.append(['alliance',arg,mask]);result=0
            mu.reg_write(UC_X86_REG_EAX,result);self.return_stub();return
        return BaseOracle.instruction(self,mu,address,size,data)

    def run(self,control):
        """처음 Player 배열이 없는 실제 초기화 분기를 실행한다."""
        mu=self.mu;self.allocated=0;self.events=[];self.put(0x59534c,0);mu.reg_write(UC_X86_REG_ECX,0);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);mu.mem_write(STACK,struct.pack('<I',STOP));mu.emu_start(0x4910a0,STOP,count=1000000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+4 and not(mu.reg_read(UC_X86_REG_FPSW)&0x3800)
        rows=[]
        # 원본이 기록한9개 용량/개수/지역/자기 동맹을 그대로 읽는다.
        for owner in range(9):
            raw=mu.mem_read(POOL+owner*0xb4,0xb4);pointer,capacity,count=struct.unpack_from('<3I',raw);region=int.from_bytes(raw[0x78:0x7c],'little');mask=int.from_bytes(raw[0x54:0x58],'little');rows.append((owner,pointer,capacity,count,region,mask))
        return rows,self.events

def main():
    """두 정밀도의 실제 초기화 결과와 현재 SHA를 확인한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        d=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in d['sha256'].items():assert sha(ROOT/path)==wanted,path
        rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows)==d['records']==9 and d['normalReturns']==2 and not d['precisionDifferentInputs']
        print('Player 공장 초기화 SHA 확인');return
    oracle=Oracle();a=oracle.run(0x027f);b=oracle.run(0x037f);assert a==b
    FIXTURE.write_text('# owner pointer capacity count region selfAllyMask\n'+'\n'.join('\t'.join(map(str,row)) for row in a[0])+'\n',encoding='utf-8',newline='\n')
    paths=['tools/decomp_player_factory_init.py','tools/decomp_mana_bolt_arrival.py','tools/decomp_constructionarrival_oracle.py','originals/Netstorm.exe','extracted/refined/originals/functions.tsv',FIXTURE.relative_to(ROOT).as_posix()]
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',lastDecompileHost='HJOW-Athlon / 2026-10-16',normalReturns=2,records=9,precisionDifferentInputs=[],events=a[1],instructions=oracle.instructions,sha256={p:sha(ROOT/p) for p in paths}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('Player 원본 초기화9기록/두 정밀도2정상 반환/차이0',oracle.instructions,'명령')

if __name__=='__main__':main()
