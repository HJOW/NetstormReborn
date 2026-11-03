#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Squid 표시 부모와 실제 TYPE번호 getter를 실행하고 오류 writer의 세 인자를 기록한다."""
import argparse
import json
from pathlib import Path
import socket
from decomp_squid_display_parent import SquidDisplayParentOracle,cases,TYPES
from decomp_file_name_leaf import add_ranges
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP

class DisplayNumberOracle(SquidDisplayParentOracle):
    """804의 독립 입력을 재사용하며49adc0 전체와43d40의 실제 세인자 계약을 추가한다."""
    def __init__(self):
        """실제 타입 번호 leaf의 정밀 범위만 추가한다."""
        super().__init__();add_ranges(self,(0x49adc0,))

    def instruction(self,mu,address,size,data):
        """번호 getter는 대체하지 않으며 진입의 명시 효과만 공급한다."""
        if address==0x49adc0:
            selected=(mu.reg_read(UC_X86_REG_ECX)-TYPES)//500;assert selected in (70,71);self.events.append('N:'+str(selected));self.mutate(2);return
        if address==0x436d40:
            args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,3);self.events.append('L:'+':'.join(map(str,args)));self.mutate(3);self.ret();return
        super().instruction(mu,address,size,data)

    def observe(self,case,precision):
        """동일 표의 논리 번호를 실제 포인터 차이/500으로 반환하게 한다."""
        self.put(0x59ab20,TYPES);return super().observe(case,precision)

def main():
    """동일 입력의 두 원문 정밀도 결과를 비교하고 별도 fixture에 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();oracles=[DisplayNumberOracle(),DisplayNumberOracle()];rows=[]
    for case in cases():  # 원문 부모/번호 getter의 결과끼리 비교하며 기대값을 재계산하지 않는다.
        outputs=[oracle.observe(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1],case;rows.append((','.join(map(str,case)),*outputs[0]))
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text('# input7 events raw50 typeFields8 checking outcome\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');report=dict(host=socket.gethostname(),entries=['004ad500','0049adc0'],rows=len(rows),observations=len(rows)*2,normalReturns=sum(row[-1]=='return' for row in rows),assertionStops=sum(row[-1]=='assertion' for row in rows),diagnostics=sum('D:' in row[1] for row in rows),gameExecutions=0,boundaries=['표시 부모/번호 getter 전체·TYPE조회/세인자오류writer/진단/표시자식만 명시','43d40 내부 포맷/strdup은 이 원문 실행에 포함하지 않음','번호조회 전에 고정한 frame/abstract 인자를 caller 스택에서 관측']);args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False))

if __name__=='__main__':main()
