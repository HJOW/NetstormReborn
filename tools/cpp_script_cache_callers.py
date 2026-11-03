#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 Script 캐시의 직접 호출과 원문 missionSpec 파일 부모를 정적으로 분류한다."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import socket
import subprocess

ROOT=Path(__file__).resolve().parents[1]  # 현재 저장소/원본을 읽기만 하는 기준이다.

def sha(path):
    """파일 전체 실제 바이트를 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    """새 보고서만 작성하며 제품/기존 분류/기대값을 덮어쓰지 않는다."""
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();assert not args.output.exists()
    paths=subprocess.check_output(['rg','--files','cpppj/src','-g','*.cpp','-g','*.h','-g','*.inl'],cwd=ROOT,text=True,encoding='utf-8').splitlines();matches=[]
    for relative in paths:  # 모든 현재 C++ 소스의 직접 Script 토큰을 조사한다. AST/외부 바이너리 간접호출 부재 증명은 아니다.
        for number,line in enumerate((ROOT/relative).read_text(encoding='utf-8-sig').splitlines(),1):
            if re.search(r'\bScript\s*\(',line):matches.append(dict(path=Path(relative).as_posix(),line=number,text=line.strip()))
    assert len(matches)==3
    declarations=[row for row in matches if row['path'].endswith('UberGump.h')];definitions=[row for row in matches if 'UberGump::Script(' in row['text']];calls=[row for row in matches if 'Script("tell")' in row['text']]
    assert len(declarations)==len(definitions)==len(calls)==1 and calls[0]['path']=='cpppj/src/client/UberGump.cpp'
    assert 'Edition()==o::OriginalEdition::Patch1078 ? "UCampaign" : Script("tell").Section("UCampaign")' in calls[0]['text']
    raw_path=ROOT/'extracted/refined/originals/raw.c';raw=raw_path.read_text(encoding='utf-8');parents=[]
    for occurrence in re.finditer(r'"missionSpec"',raw):  # 같은 원문 함수/현재 줄에서 뒤40380 호출을 직접 확인한다.
        lo=raw.rfind('// ==== ',0,occurrence.start());hi=raw.find('// ==== ',occurrence.start());header=raw[lo:raw.find('\n',lo)];load=raw.find('FUN_00440380(',occurrence.end(),hi)
        assert load>=0;parents.append(dict(function=header,line=raw.count('\n',0,occurrence.start())+1,loadLine=raw.count('\n',0,load)+1,load='00440380'))
    assert len(parents)==4
    sources=['cpppj/src/client/UberGump.cpp','cpppj/src/client/UberGump.h','cpppj/src/client/ClientMain.cpp','cpppj/src/o/ConfigInterface.cpp','tools/cpp_script_cache_callers.py','extracted/refined/originals/raw.c','originals/Netstorm.exe']
    record=dict(unit=792,date='2026-11-03',host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',status='정적 분류 완료·전체 목표 진행 중',runtimeProduct=791,displayVersion='11.01',exeSha256=sha(ROOT/'cpppj/build/bin/Release/Netstorm.exe'),
        cppSourceFilesScanned=len(paths),scriptTokens=matches,currentDirectCallCount=1,currentPatch1078DirectCallsFound=0,currentCdHostFallbackDirectCalls=1,originalMissionSpecParents=parents,
        disposition='현행 10.78 직접 caller가 없는 CD/호스트 캐시의 기존 필수 snapshot 읽기/수명을 유지한다. 기존771 Config40380과 동일 부모라 가정해 교체하지 않는다.',
        productCodeChanged=False,gameExecutions=0,newNativeObservations=0,multiplayerTests=0,newTests=0,builds=0,fullSuiteRun=False,
        boundary='정규식 소스 조사와 명시 조건식의 직접 호출 분류다. AST/외부 바이너리/모든 간접 경로의 부재 증명이나 CD 전체 동일성 검증으로 확대하지 않는다.',
        sourceSha256={name:sha(ROOT/name) for name in sources},next='checking잔여50·나머지비멀티UI/Edit/수명/호환성')
    args.output.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(sourceFiles=len(paths),tokens=len(matches),patchCalls=0,cdHostCalls=1,rawParents=len(parents))))

if __name__=='__main__':main()
