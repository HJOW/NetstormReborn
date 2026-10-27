#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78의 화면 조정 분기에서 실제 push/Tell/설정 인자를 관찰한다. OS/게임을 실행하지 않는다."""
import argparse
import hashlib
import json
from pathlib import Path
import decomp_game_menu_actions as native
from unicorn.x86_const import UC_X86_REG_ESP

ROOT=Path(__file__).resolve().parent.parent  # 기존 명령 관측기와 같은 보호 PE/격리 메모리의 기준이다.


class Probe(native.Oracle):
    """현재 사건 본체를 실행하고 화면 모드/설정 외부 장치만 명시한다."""
    def instruction(self,mu,address,size,data):
        """호출 직전 실제 스택을 기록하며 하위 OS 장치를 실행하지 않는다."""
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address==0x435ba0:
            self.body_event=True;self.event('mode',(self.number(esp+4),));self.returned();return
        if address==0x4414d0:
            self.body_event=True;self.event('remember',(self.cstring(self.number(esp+4)),self.cstring(self.number(esp+8))));self.returned();return
        super().instruction(mu,address,size,data)


def main():
    """원문 실제 분기/명령 번호와 두 x87 제어어의 반환을 기록하며 C++ 기대값은 만들지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    probe=Probe();results=[]
    # 모드 비트235/반전64/창10/확인1은 결과가 아닌 실제 명시 입력이다.
    for control in (0x27f,0x37f): # 두 원문 x87 정밀도에서 같은 세 명령을 각각 실행한다.
        for address,value in ((0x542704,64),(0x5427b8,10),(0x542a84,1)): # 조정/절대 복구/확인의 실제 전역 ID를 원본에서 읽는다.
            command=probe.number(address);case=native.cases()[0];case.update(name=str(command),event=native.packed((command,0,value)))
            probe.number(0x5c78d4,235);probe.number(0x5c7910,1);result=probe.run(case,control)
            results.append(dict(control=control,address=address,command=command,value=value,completed=result['completed'],stackDelta=result['stackdelta'],trace=result['bodytrace'],instructions=result['nativeinstructions']))
    report=dict(host=__import__('platform').node(),originalExecutions=0,cases=3,runs=len(results),boundaries=['00435ba0 실제 모드 장치만 관찰','004414d0 실제 기억 문자열만 관찰','기존 메뉴 관측기의 Guide/Gump/기타 UI 장치'],observations=results,
        sha256={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in ('originals/Netstorm.exe','tools/decomp_screen_mode_commands.py','tools/decomp_game_menu_actions.py','tools/decomp_production_shortcuts.py')})
    with args.output.open('x',encoding='utf-8') as stream: json.dump(report,stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps(dict(cases=3,runs=len(results),traces=[row['trace'] for row in results[:3]]),ensure_ascii=False))


if __name__=='__main__':
    main()
