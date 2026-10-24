#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78의 실제 HTML 순회/배경 태그가 쓰는80BYTE를 독립 관측한다."""
import argparse
import hashlib
import itertools
import json
import socket
from pathlib import Path
from decomp_htmltext_oracle import Oracle,ROOT

FIXTURE=ROOT/'cpppj/tests/fixtures/html-background-x86.tsv'  # 원본 출력만 저장하는 새 자료다.
BACKGROUND=0x565e70  # 다음565ec0의 단락 억제DWORD 앞에 있는 실제80BYTE다.


class BackgroundOracle(Oracle):
    """기존 기본 HTML 본체에 배경 전역 쓰기만 추가로 허용한다."""
    def write(self,mu,access,address,size,value,data):
        """배경 범위 밖 쓰기는 기존 원문 관측의 제한을 유지한다."""
        if BACKGROUND<=address and address+size<=BACKGROUND+80:
            return
        super().write(mu,access,address,size,value,data)


def cases():
    """태그 대소문자/구분자/빈 이름/첫NUL과 기존 뒤쪽 바이트를 입력으로 정한다."""
    seeds=[bytes(80),b'prior-long-background-name\0'.ljust(80,b'\xa5'),bytes(range(1,80))+b'\0']  # 각 초기80BYTE를 원문에 그대로 제공한다.
    sources=[b'plain',b'<background>',b'<background="nsstart">body',b'<Background="NsIsles">body',b'<BACKGROUND="wrong">body',b'<bAckground="wrong">body',b'<ba="short">body',b'</background="closed">body',b'<background=unquoted>body',b'<background= "space">body',b'<background="">body',b'<background=>body',b'<background="first"><background="b">body',b'<background="a>b">body',b'<background="unfinished',b'<background=x\0later>body',b'<background="\x80\xff">body',b'<background="'+b'x'*79+b'">body']  #79BYTE와NUL까지의 유효 범위만 native에서 실행한다.
    return list(itertools.product(seeds,sources,(0,1,0xffffffff),(0,2),(0,1)))


def generate():
    """C++를 실행하거나 읽지 않고 PE의 반환과 실제80BYTE를 읽는다."""
    oracle=BackgroundOracle('originals');rows=[]  # 모든 입력에서 같은 보호PE의 실제 본체를 사용한다.
    for seed,source,shown,hidden,suppress in cases():  # 원본 입력 조합마다 전역 초기값을 명시한다.
        oracle.mu.mem_write(BACKGROUND,seed)
        output=oracle.run((source.hex(),8192,shown,hidden,suppress))
        rows.append([seed.hex(),source.hex(),shown,hidden,suppress,*output,bytes(oracle.mu.mem_read(BACKGROUND,80)).hex()])
    content='# 실제10.78 HTML 배경 전역80BYTE. CRT tolower ASCII 경계, 게임/OS 실행 없음.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n'
    return content,oracle


def main():
    """최초 자료 작성과 자료를 바꾸지 않는 원본 재생을 구별한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    content,oracle=generate()
    if args.check:
        if FIXTURE.read_text(encoding='utf-8')!=content:
            raise RuntimeError('원본 배경 자료 재생 불일치')
    else:
        if FIXTURE.exists():
            raise RuntimeError('기존 자료 덮어쓰기 금지: --check 사용')
        FIXTURE.write_text(content,encoding='utf-8',newline='\n')
    paths=[Path(__file__),ROOT/'tools/decomp_htmltext_oracle.py',ROOT/'originals/Netstorm.exe',FIXTURE]  # 원문/장치/입출력 식별자다.
    report={'host':socket.gethostname(),'edition':'10.78','rows':oracle.returns,'instructions':oracle.instructions,'readOnlyReplay':args.check,'nativeRanges':oracle.spec['ranges'],'observedBackground':[BACKGROUND,80],'osCalls':0,'limits':['CRT tolower의 ASCII 장치만 공급한다. 게임/UI/OS 전체 실행 검증은 아니다.','최대79BYTE 이름만 실행한다. 원문80BYTE 밖 쓰기는 호스트에서 명시 거부한다.','정수/문자 경로이며 두 x87 정밀도 비교는 수행하지 않았다.'],'hashes':{p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'rows':oracle.returns,'instructions':oracle.instructions,'readOnlyReplay':args.check}))


if __name__=='__main__':
    main()
