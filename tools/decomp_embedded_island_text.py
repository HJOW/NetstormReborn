#!/usr/bin/env python3
"""보호 설명의 실제 Config 섹션 조회와 기본 HTML 출력을 두 PE에서 관측한다."""
import argparse
import hashlib
import json
import socket
from pathlib import Path
from decomp_config_oracle import ConfigOracle
from decomp_htmltext_oracle import Oracle as HtmlOracle
from taff import TaffArchive
from unicorn.x86_const import UC_X86_REG_FPCW

ROOT=Path(__file__).resolve().parent.parent  # 보호 PE/TAFF는 읽기만 한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/embedded-island-text-x86.tsv'  # C++ 결과를 사용하지 않는 새 원문 자료다.
REPORT=ROOT/'cpppj/embedded-island-text-native-20261027.json'  # 관측 수/범위와 실제 입력 해시다.
SOURCES={'originals':('originals/Netstorm.exe','originals/netstorm.tarc'),'originalCD':('originalCD/NETSTORM.EXE','originalCD/NETSTORM.TARC')}  # 판본별 실제 PE와 자료다.

def digest(path):
    """현재 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def generate():
    """실제 섹션 포인터/길이로 자른 원문을 같은 판본의 기본 HTML 몸체에 전달한다."""
    rows=[]
    facts=[]
    paths=[Path(__file__),ROOT/'tools/decomp_config_oracle.py',ROOT/'tools/decomp_htmltext_oracle.py',ROOT/'tools/decomp_oracle.py']
    for edition,(binary,archivePath) in SOURCES.items():  # 각 판본의 자료와 전역을 섞지 않는다.
        archive=TaffArchive(ROOT/archivePath)
        entry=next(item for item in archive.entries if item.name.replace('\\','/').lstrip('/').lower()=='d/tell.english')
        resource=archive.read(entry)
        start=resource.index(b'[PlacingIslands]')
        following=resource.index(b'\n[',start+16)+1
        end=resource.index(b'\n',following)+1
        excerpt=resource[start:end]  # 원래 연속 바이트의 두 섹션 경계까지 보존하며 본문을 다시 쓰지 않는다.
        config=ConfigOracle(edition)
        html=HtmlOracle(edition)
        observations=[]
        for precision in (0x27f,0x37f):  # 두 정밀도에서 실제 조회/문자열 상태 전체가 같은지 확인한다.
            config.mu.reg_write(UC_X86_REG_FPCW,precision)
            html.mu.reg_write(UC_X86_REG_FPCW,precision)
            offset,length=config.section(excerpt,b'PlacingIslands')
            assert offset>=0 and length>0 and offset+length<=len(excerpt)
            body=excerpt[offset:offset+length]
            result=html.run((body.hex(),8192,1,0,0))
            prefix=b'{Text} ' if edition=='originals' else b'To the left '
            assert bytes.fromhex(result[0]).startswith(prefix),(edition,body[:64],bytes.fromhex(result[0])[:64],offset,length)
            observations.append([edition,excerpt.hex(),b'PlacingIslands'.hex(),body.hex(),offset,length,*result])
        assert observations[0]==observations[1]
        rows.append(observations[0])
        facts.append(dict(edition=edition,resourceBytes=len(resource),resourceSha256=hashlib.sha256(resource).hexdigest(),excerptOffset=start,excerptBytes=len(excerpt),sectionOffset=offset,sectionBytes=length,outputPrefix=bytes.fromhex(result[0])[:40].decode('ascii')))
        paths.extend((ROOT/binary,ROOT/archivePath))
    FIXTURE.write_text('# 실제 보호 PlacingIslands의 Config 섹션/기본 HTML. 조회 CRT만 기존 ABI 경계이며 문자열 기대값은 원문 실행에서만 읽는다.\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows),encoding='utf-8',newline='\n')
    paths.append(FIXTURE)
    report=dict(host=socket.gethostname(),lastFullDecompilation='HJOW-Athlon / 2026-10-23',uniqueInputs=2,normalSections=4,normalHtmlParses=4,precisionDifferences=[],facts=facts,
        limits='원래 연속 섹션/다음 머리글 발췌를 원본 Section에 공급한 뒤 실제 기본 HTML로 출력한다. 전체 파일 검색/원본 창·OS·다중 줄 픽셀은 실행하지 않는다. 후행004ba8f0/004b9790/004b93c0의 리터럴 복사는 정적 연결 근거로 별도 기록한다.',sha256={path.relative_to(ROOT).as_posix():digest(path) for path in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(inputs=2,sections=4,htmlParses=4,precisionDifferences=0,facts=facts),ensure_ascii=False))

def verify():
    """이전 관측을 덮지 않고 실제 파일 해시/입력/반환 수를 확인한다."""
    report=json.loads(REPORT.read_text('utf-8'))
    assert report['uniqueInputs']==2 and report['normalSections']==report['normalHtmlParses']==4 and not report['precisionDifferences']
    assert all(digest(ROOT/name)==value for name,value in report['sha256'].items())
    rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==2 and all(len(row)==14 for row in rows)
    print('원문 섬 설명 조회/출력 근거 감사 통과')

if __name__=='__main__':  # 게임 창을 시작하지 않는 명시 생성/감사 진입이다.
    parser=argparse.ArgumentParser()
    parser.add_argument('--verify',action='store_true')
    arguments=parser.parse_args()
    verify() if arguments.verify else generate()
