#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""전용 Help의 파일 실패·기본 주제·그림 제외·빈 요청에서 실제 전체 창 경로를 추가 관측한다."""
import argparse
import itertools
import json
from decomp_quick_help_window import Oracle as BaseOracle, ROOT, WINDOW, SOURCE, TEXT, digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_ESP

FIXTURE=ROOT/'cpppj/tests/fixtures/quick-help-window-paths-x86.tsv'  # 최초84행을 변경하지 않는 추가 경로다.
REPORT=ROOT/'cpppj/quick-help-window-paths-native.json'  # 명시 lookup0/치환과 실제 기본 주제 선택을 구별한다.


class Oracle(BaseOracle):
    """실제 요청/창/HTML/링크와 기존 GUI 경계를 재사용한다."""
    def __init__(self):
        """추가 경로에 필요한 실제 요청/CRT 범위만 더한다."""
        self.mode=(0,0,0,0);super().__init__();ranges(self,(0x4c5c00,0x4e65ed))

    def prepare(self):
        """기본 세 주제가 다른 본문을 갖도록 명시 원문과 현재 모드를 공급한다."""
        super().prepare();battle,challenge,empty,fail=self.mode;self.put(0x594fbc,battle);self.put(0x594fc8,challenge)
        self.helpText=b'<a name="introHelp">INTRO</a>\n<a name="challengeHelp">CHALLENGE</a>\n<a name="pyrosphereHelp">BATTLE</a>\n<a name="unitType">UNIT</a>\n';self.mu.mem_write(SOURCE,self.helpText+b'\0\0')

    def instruction(self,mu,address,size,data):
        """추가된 실패/빈 치환/빈 이름 TYPE0 경계만 공급하며 나머지 몸체는 그대로 둔다."""
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,6)
        if address==0x41baa0 and self.mode[3]:self.event('Load',self.string(args[0]).hex(),args[1],args[3]);self.put(args[2],0);self.ret(0);return
        if address==0x4c2630:self.event('LoadError',args[0],self.string(args[2]).hex());self.ret();return
        if address==0x441660 and self.mode[2]:self.event('Substitute',self.string(args[1]).hex(),args[2]);mu.mem_write(args[0],b'\0');self.ret();return
        if address==0x49a860:self.event('Lookup',self.string(args[0]).hex());self.ret(0);return
        if address==0x4c52c0:self.event('History',self.string(args[0]).hex());self.ret();return
        super().instruction(mu,address,size,data)


def cases():
    """파일 실패와 기본 주제 우선순위·빈 본문·그림 제외/빈 요청을 독립 공급한다."""
    rows=[('C',(0,0,0,0,0,0,1))]
    # 각 경로는 전체 창의 기본 주제 선택과 치환 후 빈 결과를 함께 관측한다.
    for battle,challenge,empty in itertools.product((0,1),(0,1),(0,1)):
        rows.append(('S',(0,'',3,0,battle,challenge,empty)))
    for type in (2,3):rows.append(('S',(type,'unitType',3,0,0,0,0)))
    for battle,challenge in ((0,0),(0,1),(1,0)):rows.append(('R',(battle,challenge)))
    return rows


def main():
    """최초 추가 관측을 저장하거나 해시만 읽으며 원문/제품 기대값을 바꾸지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text('utf-8'))
        for name,expected in record['sha256'].items():assert digest(ROOT/name)==expected,name
        print('Help 기본/실패/빈 요청 최초 경로 보존 확인');return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('최초 관측 덮어쓰기 금지')
    oracle=Oracle();rows=cases();lines=['# 실제 전체 창의 추가 경로. lookup0·치환/파일 실패·GUI는 명시 경계다.']
    # 입력을 먼저 고정하고 관측 결과는 실제 함수가 반환한 현재 상태에서만 읽는다.
    for kind,row in rows:
        if kind=='C':oracle.mode=(0,0,0,1);result=oracle.sample(kind,(0,0,0,1065353216))
        elif kind=='S':oracle.mode=tuple(row[4:])+(0,);result=oracle.sample(kind,row[:4])
        else:
            oracle.mode=tuple(row)+(0,0);oracle.prepare();oracle.mu.mem_write(TEXT+0xa000,b'\0');oracle.call(0x4c5c00,[0,TEXT+0xa000],WINDOW,8);oracle.normal+=1;result=oracle.output()
        lines.append('\t'.join((kind,json.dumps(row,separators=(',',':')),result)))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n');names=['originals/Netstorm.exe','tools/decomp_quick_help_window.py','tools/decomp_quick_help_window_paths.py','cpppj/tests/fixtures/quick-help-window-paths-x86.tsv']
    record={'inputs':len(rows),'normalReturns':oracle.normal,'instructions':oracle.instructions,'gameExecuted':False,'osExecuted':False,'scope':'파일 실패/기본 주제 우선순위/치환 후 빈 본문/그림 제외와 실제4c5c00 빈 요청→기존 전체 창. lookup의0은 명시 TYPE0 반환이며 실제 일반 레지스터/스택에서 접미 비교/요청 몸체를 실행한다.','sha256':{name:digest(ROOT/name) for name in names}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:record[key] for key in ('inputs','normalReturns','instructions')}))


if __name__=='__main__':main()
