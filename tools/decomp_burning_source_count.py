#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""양판본 연소점 부모/실제 TYPE HP를 실행하되 높이2의 이른 복귀로 입자/픽셀을 제외한다."""
import argparse
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH, STACK, STOP
from unicorn import UcError, UC_HOOK_INTR
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP

CONTROL=SCRATCH+0x1000  # 실제 부모 this의 선택 피해 번호/이미지 크기/목표 행 입력이다.
POOL=SCRATCH+0x4000  # 실제 부모가 SID*stride로 접근하는 격리 raw 슬롯이다.
TYPES=0x12000000  # 같은 선택 TYPE this를 유지하는 격리 표다.
FIXTURE=ROOT/'cpppj/tests/fixtures/burning-source-count-current-x86.tsv'  # 최초 원문 결과와 진단 뒤 후상태를 동결한다.
SPEC={  # 현재 정밀 산출물의 함수/전역/실제 IDIV 주소다.
    'originals':dict(root=0x424110,helpers=[0x4adc60,0x4ac550,0x4ac200,0x49fce0,0x49a840],robust=0x4ac140,assertion=0x4e0620,base=0x59ab20,pool=0x5c8464,stride=500,slot=50,quarter=0x5ca8f0,checking=0x5e4794,priest=0x5412d0,dais=0x5412ec,idiv=0x4241b6,getterReturn=0x4241a2),
    'originalCD':dict(root=0x4a5d30,helpers=[0x4aecf0,0x4abf50,0x4abae0,0x4abac0,0x444720,0x4448e0],robust=0x4ab670,assertion=0x490040,base=0x51c960,pool=0x5395dc,stride=468,slot=36,quarter=0x511c90,checking=0x540a24,priest=0x51cbbc,dais=0x51cbd8,idiv=0x4a5da2,getterReturn=0x4a5d81)}


def sha(path):
    """실제 도구/입력/보호 PE의 같은 바이트 정체성을 기록한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class SourceCountOracle(Oracle):
    """진단 UI와 robust의 명시 반환만 대체하고 부모/TYPE 조회·HP/정수식을 실행한다."""
    def __init__(self,edition):
        """현재 refined 범위와 프로그램에서 분리한 입력 메모리를 준비한다."""
        self.s=SPEC[edition]  # 각 판본의 실제 폭/주소다.
        super().__init__(edition,[dict(name='sources',**{edition:f'{self.s["root"]:08x}'})],helpers=self.s['helpers'])
        self.mu.mem_map(TYPES,0x20000)
        self.mu.hook_add(UC_HOOK_INTR,self.on_interrupt);self.cpu=self.mu.context_save()  # 원문 CPU 예외를 관측하고 매 입력의 CPU 문맥을 독립 복원한다.
        self.profile=0;self.diagnostics=[];self.maximum=None;self.robust_calls=0;self.fault=None  # 외부 복귀 입력과 실제 관측이다.

    def on_interrupt(self,mu,number,data):
        """x86 #DE의 실제 발생 위치/제수를 읽고 예외 후 다음 입력으로 진행한다."""
        assert number==0 and mu.reg_read(UC_X86_REG_EIP)==self.s['idiv']
        register=UC_X86_REG_ECX if self.edition=='originals' else UC_X86_REG_EDI
        self.fault='zero' if not mu.reg_read(register) else 'overflow';mu.emu_stop()

    def put(self,address,value,width=4):
        """외부 장치의 명시 변경을 원문 저장 폭으로 공급한다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def get(self,address,width=4,signed=False):
        """실제 후상태를 원문 폭으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little',signed=signed)

    def resume(self,value):
        """cdecl 진단/robust가 실제 반환 주소로 정상 복귀하도록 공급한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);ret=self.get(sp)
        self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_ESP,sp+4);self.mu.reg_write(UC_X86_REG_EIP,ret)

    def on_instruction(self,mu,address,size,data):
        """허용된 원문 몸체 외에는 진단 UI/번호 확인 장치만 명시 복귀시킨다."""
        if address==self.s['robust']:
            self.robust_calls+=1;self.resume(1);return
        if address==self.s['assertion']:
            sp=mu.reg_read(UC_X86_REG_ESP);line=self.get(sp+12)
            self.diagnostics.append(self.read_string(self.get(sp+4)).decode('ascii')+'|'+self.read_string(self.get(sp+8)).decode('ascii')+'|'+str(line))
            assert line==0x1e3
            if self.profile:
                target=POOL+50*self.s['slot'];self.put(target+26,5 if self.profile==2 else 0,4)
                self.put(TYPES+74*self.s['stride'],200);self.put(self.s['quarter'],0 if self.profile==3 else 1);self.put(self.s['priest'],75 if self.profile==3 else 74)
                if self.profile in (2,3):
                    self.mu.mem_write(target+10,bytes([75]));self.put(CONTROL+0x6c,51);self.put(TYPES+75*self.s['stride']+0xec,0x400 if self.profile==2 else 0)
                if self.profile==3:
                    self.put(self.s['dais'],75)
            self.resume(0);return
        if address==self.s['getterReturn']:
            self.maximum=struct.unpack('<i',struct.pack('<I',mu.reg_read(UC_X86_REG_EAX)))[0]
        super().on_instruction(mu,address,size,data)

    def observe(self,case):
        """원문 부모 반환 또는 실제 IDIV 예외와 이미 남은 모든 선택 후상태를 읽는다."""
        self.mu.context_restore(self.cpu)
        hp,maximum,quarter,checking,flag,width,priest,self.profile=case;self.diagnostics=[];self.maximum=None;self.robust_calls=0;self.fault=None
        self.mu.mem_write(CONTROL,bytes(256));self.mu.mem_write(POOL,bytes(4096));self.mu.mem_write(TYPES,bytes(0x20000))
        self.put(CONTROL+0x5c,width);self.put(CONTROL+0x60,2);self.put(CONTROL+0x6c,50)
        self.put(self.s['base'],TYPES);self.put(self.s['pool'],POOL)
        for address,value in [(self.s['quarter'],quarter),(self.s['checking'],checking),(self.s['priest'],priest),(self.s['dais'],165)]:  # 단위별 현재 전역의 폭/위치를 그대로 설정한다.
            self.put(address,value)
        self.put(TYPES+74*self.s['stride'],maximum);self.put(TYPES+74*self.s['stride']+0xe8,0 if checking else 16);self.put(TYPES+74*self.s['stride']+0xec,0x400000|(0x400 if flag else 0));self.put(TYPES+75*self.s['stride'],999)
        target=POOL+50*self.s['slot'];self.mu.mem_write(target+10,bytes([74]));self.mu.mem_write(target+(40 if self.edition=='originals' else 35),bytes([1]));self.put(target+26,hp,4 if self.edition=='originals' else 2)
        outcome='ok'
        try:
            self.call('sources',[],CONTROL)
        except RuntimeError as error:
            if self.fault:
                outcome=self.fault
            else:
                raise RuntimeError(f'{case}: {error}; EIP={self.mu.reg_read(UC_X86_REG_EIP):08x}, ESP={self.mu.reg_read(UC_X86_REG_ESP):08x}') from error
        except UcError:
            assert self.mu.reg_read(UC_X86_REG_EIP)==self.s['idiv']
            register=UC_X86_REG_ECX if self.edition=='originals' else UC_X86_REG_EDI
            outcome='zero' if not self.mu.reg_read(register) else 'overflow'
        assert self.maximum is not None
        post=[self.get(CONTROL+0x7c,signed=True),self.get(target+26,4 if self.edition=='originals' else 2,True),self.mu.mem_read(target+10,1)[0],self.get(CONTROL+0x6c),self.get(self.s['checking']),self.get(self.s['quarter']),self.get(self.s['priest']),self.get(self.s['dais']),self.get(TYPES+74*self.s['stride'],signed=True)]
        return outcome,','.join(map(str,post)),';'.join(self.diagnostics) or '-',self.maximum,self.robust_calls


def inputs():
    """C++ 결과와 독립인 전체 DWORD 감김/WORD HP/진단 복귀 입력을 교차한다."""
    maxima=(-2147483648,-1073741825,-1073741824,-37,-7,-1,0,1,7,37,1073741823,1073741824,2147483647)
    hp=(-2147483648,-858993459,-7,-1,0,7,50,32767,2147483647)
    cases=[(h,m,q,c,f,w,74,0) for h,m,q,c,f,w in itertools.product(hp,maxima,(0,1),(0,0x80000000),(0,1),(30,2147483647,-7))]
    cases.extend((h,29,0,0x80000000,f,30,158,profile) for h,f,profile in itertools.product((0,7,50),(0,1),(1,2,3)))
    return cases


def main():
    """새 동결 입력을 한 번 생성하거나 실제 부모를 다시 실행해 읽기 전용 대조한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    rows=['# 실제 양판본 연소점 부모/TYPE/HP/정수식. robust=1/진단UI만 외부 장치이며 높이2로 입자·난수·픽셀 미실행.'];editions={}
    for edition in SPEC:  # 각 보호 PE의 실제 부모와 순수 조회 자식을 독립 실행한다.
        oracle=SourceCountOracle(edition);counts={}
        for case in inputs():  # 원문 반환/CPU 예외와 후상태만 동결한다.
            result=oracle.observe(case);counts[result[0]]=counts.get(result[0],0)+1;rows.append('\t'.join(map(str,(edition,','.join(map(str,case)),*result))))
        editions[edition]=dict(samples=len(inputs()),outcomes=counts,binarySha256=oracle.sha256,refinedIndexSha256=sha(ROOT/f'extracted/refined/{edition}/functions.tsv'))
    content='\n'.join(rows)+'\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8')==content
    else:
        with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:
            stream.write(content)
    report=dict(host=socket.gethostname(),mode='verify' if args.verify else 'first',samples=len(rows)-1,editions=editions,fixtureSha256=sha(FIXTURE),toolSha256=sha(Path(__file__)),boundaries=['robust 반환1','진단UI 정상복귀/명시HP·TYPE·전역 변경'],actualGetter=True,particleLoops=False,randomCalls=0,pixelAccess=False)
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(report['mode'],report['samples'],'연소점 원문 행 일치',flush=True)


if __name__=='__main__':
    main()
