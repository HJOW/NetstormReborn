#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 동적 정보의 중첩 링크가 본문 링크 쓰기 포인터를 전진시키지 않는지 최초 관측한다."""
import argparse
import json
from decomp_quick_help_html import Oracle as BaseOracle, ROOT, TEXT, UC_X86_REG_ESP, digest

FIXTURE=ROOT/'cpppj/tests/fixtures/quick-help-html-nested-x86.tsv'  # 첫83행은 그대로 보존한 추가 독립 관측이다.
REPORT=ROOT/'cpppj/quick-help-html-nested-native.json'  # 명시 설명 문자열과 실제 두 파서의 pointer 결과다.


class Oracle(BaseOracle):
    """설명 getter만 중첩 링크로 공급하고 실제 info/두 HTML/링크 몸체를 그대로 실행한다."""
    def instruction(self,mu,address,size,data):
        """합성 설명은 실제 TYPE40바이트 버퍼에 쓰지 않고 명시 getter 경계로 제공한다."""
        if address==0x49af80:
            mu.mem_write(TEXT+0x9000,self.description.encode()+b'\0');self.ret(TEXT+0x9000);return
        super().instruction(mu,address,size,data)


def main():
    """최초3행을 보존하거나 현재 해시만 읽으며 게임/OS/C++는 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        for name,expected in report['sha256'].items():assert digest(ROOT/name)==expected,name
        print('Help 정보 중첩 링크 최초3행 보존 확인');return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('최초 관측 덮어쓰기 금지')
    oracle=Oracle();lines=['# 명시 설명 getter / 실제 info HTML→기본 링크·본문 링크의 두 cursor 관측이다.']
    text='<a name="unitType"><info><a href="#outer">Out</a></a>\nSTOP'
    # 앞 링크 길이와 중첩2개·인용없는 대상도 실제 몸체로 교차한다.
    for description in ('<a href="#X">L</a>','<a href="#longTopic">L</a><a href="cmd:Go">G</a>','<a href=bad>L</a>'):
        oracle.description=description;lines.append(description.encode().hex()+'\t'+oracle.sample('D',(text,'unitType',1,1,1,8)))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n');names=['tools/decomp_quick_help_html.py','tools/decomp_quick_help_html_nested.py','originals/Netstorm.exe','cpppj/tests/fixtures/quick-help-html-nested-x86.tsv']
    REPORT.write_text(json.dumps({'inputs':3,'normalReturns':oracle.normal,'instructions':oracle.instructions,'gameExecuted':False,'osExecuted':False,'sha256':{name:digest(ROOT/name) for name in names}},ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('native nested',oracle.normal,oracle.instructions)


if __name__=='__main__':main()
