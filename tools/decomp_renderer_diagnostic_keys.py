"""원문 일반입력7/10/13의 특수관문과현재진단DWORD변경만 독립관측한다."""
from pathlib import Path
import argparse
import hashlib
import itertools
import json
import socket
import sys

ROOT=Path(__file__).resolve().parents[1]  # 보호원문만읽는다.
OUT=ROOT/'extracted/cpp-renderer-diagnostics'  # 첫관측과실패원시를분리보관한다.
sys.path.insert(0,str(ROOT/'tools'))
import decomp_game_menu_actions as general
from unicorn.x86_const import UC_X86_REG_ESP


class Oracle(general.Oracle):
    """네선택store만허용하고대상외관리키는호출경계를기록한다."""
    def guard(self,mu,access,address,size,value,data):
        """진단세플래그외의새게임메모리쓰기는허용하지않는다."""
        if address in (0x54d920,0x54d924,0x54d92c) and size==4: return
        super().guard(mu,access,address,size,value,data)

    def instruction(self,mu,address,size,data):
        """Shift-P좌표로그는별도효과로기록하고화면/로그장치본문을실행하지않는다."""
        if address in (0x4971e0,0x4c2630):
            self.body_event=True;self.event('position' if address==0x4971e0 else 'position-log',());self.returned();return
        super().instruction(mu,address,size,data)


def main():
    """명시특수전역/상위비트/정지/Console조합을두정밀도로고정한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();destination=args.output.resolve();destination.mkdir(parents=True,exist_ok=False)
    fixture=destination/'modified/cpppj/tests/fixtures/renderer-diagnostic-keys-x86.tsv';report=destination/'native.json'
    if report.exists() or fixture.exists(): raise FileExistsError('최초키기대덮어쓰기거부')
    oracle=Oracle();seed=general.cases()[0];rows=['# event special auxiliary initial redraw paused console fire spot board finalRedraw bodytrace']
    cases=itertools.product((7,0x10,0x13),(0,0x80000000,0x20000000,0x7c000000),(0,29),(0,99),(0,7,0xffffffff),(0,3),(0,1))
    for code,flags,special,auxiliary,initial,paused,console in cases:  # 전체DWORD는독립입력이고기대효과는원문에서만읽는다.
        values=list(map(int,seed['globals'].split(':')));values[7:10]=92,paused,console;event=code|flags
        case=dict(seed,event=general.packed((event,0,0)),globals=general.packed(values),guideResult=0,gumpResult=0,mutation='-',stop='-');samples=[]
        for precision in (0x27f,0x37f):  # 같은비MP입력만두정밀도로관측한다.
            for address,value in ((0x55a480,special),(0x55a484,auxiliary),(0x54d920,initial),(0x54d924,initial),(0x54d92c,initial)): oracle.number(address,value)
            result=oracle.run(case,precision);samples.append((*(oracle.number(address) for address in (0x54d920,0x54d924,0x54d92c,0x59a8b0)),result['bodytrace']))
        assert samples[0]==samples[1]
        rows.append('\t'.join(map(str,(event,special,auxiliary,initial,92,paused,console,*samples[0]))))
    payload='\n'.join(rows)+'\n'
    fixture.parent.mkdir(parents=True,exist_ok=True);fixture.write_text(payload,'utf-8',newline='\n')
    record=dict(unit=776,host=socket.gethostname(),inputs=len(rows)-1,observations=(len(rows)-1)*2,fixtureSha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),gameExecutions=0,multiplayerTests=0,cppTests=0,builds=0,boundary='실제Guide/Gump/공통release/Console/7·10·13분기store; 명시특수전역입력은DebugSwitch의완료관측이아니며ShiftP좌표/로그는경계다.')
    report.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8');print(json.dumps(dict(inputs=len(rows)-1,observations=(len(rows)-1)*2)))


if __name__=='__main__':
    main()
