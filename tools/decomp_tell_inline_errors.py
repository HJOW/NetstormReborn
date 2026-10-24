#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""% 행의 명시 Config 예외에서 원문 Catch/공유 꼬리의 문자열 복구를 관측한다."""
import argparse
import itertools
import json
import socket
from decomp_tell_inline_list import Oracle as InlineOracle
from decomp_game_menu_bar import ROOT,PE,HEAP,CONTROLS,digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP

CONTINUE=HEAP+0x2e00  # 실제 EH 런타임 대신 Catch 반환주소/스택 복구만 담당하는 명시 장치다.
FIXTURE=ROOT/'cpppj/tests/fixtures/tell-inline-errors-x86.tsv'  # 정상288행과 구별한 새 예외 경로 관측이다.


class Oracle(InlineOracle):
    """첫/두 번째 조회 예외를 원문 Catch로 전달하고 실제 Catch/공유 꼬리를 실행한다."""
    def instruction(self, mu, address, size, data):
        """EH 전달/스택 복구만 명시하며 ERROR 문자열/현재 행 복구·후속 행은 원문이다."""
        if address==CONTINUE:
            if mu.reg_read(UC_X86_REG_EAX)!=0x4CE144:
                raise RuntimeError('native Catch continuation mismatch')
            mu.reg_write(UC_X86_REG_ESP,self.frame_stack)
            mu.reg_write(UC_X86_REG_EIP,mu.reg_read(UC_X86_REG_EAX))
            return
        if address==0x441740:
            esp=mu.reg_read(UC_X86_REG_ESP)
            if self.text(self.word(esp+4))==b'{@probe.value}' and self.probe+1==self.throw_at:
                self.probe+=1
                self.event('Q',self.current_file().hex(),self.probe)
                self.event('E',self.current_file().hex(),self.probe)
                frame=mu.reg_read(UC_X86_REG_EBP)
                if self.word(frame-4)!=6:
                    raise RuntimeError('exception outside native row try scope')
                self.frame_stack=self.word(frame-0x10)
                self.word(self.frame_stack-4,CONTINUE)
                mu.reg_write(UC_X86_REG_ESP,self.frame_stack-4)
                mu.reg_write(UC_X86_REG_EIP,0x4CE0EF)
                self.caught+=1
                return
        super().instruction(mu,address,size,data)

    def run_error(self, paths, profile, existing, throw_at, control):
        """실제 Config가 n번째 조회에서 실패한다는 장치 입력만 제공한다."""
        self.throw_at,self.caught=throw_at,0
        return self.run_inline(paths,profile,existing,b'{@probe.value}',control)


def main():
    """부분 출력 보존과 다음 행 진행을 두 정밀도로 관측하고 재생한다."""
    parser=argparse.ArgumentParser()
    parser.add_argument('--check',action='store_true')
    options=parser.parse_args()
    if digest(PE)!='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236':
        raise RuntimeError('protected PE mismatch')
    cases=[[],[b'd/z.english'],[b'd/b.extra.english',b'd/a.english',b'd/z.english'],[b'd/A.english',b'e/a.german',b'd/b.english'],[b'd/a.english',b'd/a.english'],[b'plain']]
    cases.extend(list(value) for value in itertools.permutations((b'd/c.english',b'd/a.english',b'd/b.english')))
    oracle,rows,caught=Oracle(),[],0
    for index,(paths,profile,existing,throw_at) in enumerate(itertools.product(cases,range(2),range(2),range(7))):  # 기대 ERROR 결과나 정렬을 계산하지 않는 입력 조합이다.
        previous=None
        for control in CONTROLS:  # 실제 Catch/현재 행 복구와 최종 String을 독립 관측한다.
            observed=oracle.run_error(paths,profile,existing,throw_at,control)
            if previous is not None and observed!=previous:
                raise RuntimeError('precision difference')
            previous=observed
            caught+=oracle.caught
            rows.append(f'{index}-cw{control:03x}\t'+','.join(path.hex() for path in paths)+f'\t{profile}\t{existing}\t{throw_at}\t{observed}')
    value='# name\tpaths\tprofile\texisting\tthrow_at\tobserved\n'+'\n'.join(rows)+'\n'
    if options.check:
        if FIXTURE.read_text(encoding='utf-8')!=value:
            raise RuntimeError('fixture mismatch')
    else:
        FIXTURE.write_text(value,encoding='utf-8',newline='\n')
    report={'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','rows':len(rows),'caughtTransfers':caught,'precisionDifferences':0,'precisions':oracle.totals,'gameExecuted':False,'scope':'실제4cdd40 정상 본체와4ce0ef Catch·4ce144 행 복구·4ce14d 공유 꼬리를 실행해 부분 String 보존/ERROR 링크/다음 행을 관측한다.','boundaries':['Config의 선택된 n번째 조회 예외와 EH의 Catch 진입/저장ESP 복구/반환 continuation 전달은 명시 장치다. 실제 Windows C++ EH 런타임 전체나 그 unwind를 실행하지 않는다.','FileSpec/Config/String/할당/CRT 경계는 정상 % 도구와 같다. Catch 자체의 추가 할당 실패나 임의 비동기 예외/GUI·미션 완주는 검증하지 않는다.'],'sha256':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in (PE,FIXTURE,ROOT/'tools/decomp_tell_inline_errors.py',ROOT/'tools/decomp_tell_inline_list.py',ROOT/'tools/decomp_tell_file_menus.py',ROOT/'extracted/refined/originals/Netstorm.c')}}
    (ROOT/'extracted/cpp-tell-errors-411-native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'caughtTransfers':caught,'precisionDifferences':0,'check':options.check}))


if __name__=='__main__':
    main()
