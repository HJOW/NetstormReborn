#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Rune 등록/분배 추가를 역제거해 b76adf9 공통postPop 전체소스/토큰과 이전근거 보존을 감사한다."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent # 고정된공유저장소다.
REPORT=ROOT/'cpppj/recovery-runepostpop-lineage-evidence.json' # 기존완료보고서와분리한새등록lineage다.
SOURCES=('cpppj/src/o/SquidPostPop.h','cpppj/src/o/SquidPostPop.cpp') # 새Rune등록/검사/분배만추가하는기존두파일이다.
PRIOR=('cpppj/recovery-postpop-evidence.json','cpppj/recovery-pop-evidence.json','cpppj/recovery-priestpostpoptail-evidence.json','cpppj/recovery-constructionprocess-evidence.json') # 기존공통/공간/사제/건설원본관찰을다시합산하지않는다.


def sha(data):
    """파일과정규화토큰의정확한SHA를계산한다."""
    return hashlib.sha256(data).hexdigest()


def git(*args):
    """고정git객체를읽으며commit/push를실행하지않는다."""
    return subprocess.run(['git',*args],cwd=ROOT,capture_output=True,check=True).stdout


def tokens(text):
    """리터럴을보존하고주석/공백만빼서기존전체토큰을비교한다."""
    parts=re.findall(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\w+|[^\s]',text);return '\0'.join(part for part in parts if not part.startswith(('//','/*'))).encode('utf-8')


def restore(name,text):
    """명시된새Rune선언/정의/분기/설명만역제거한다."""
    if name.endswith('.h'):
        patterns=('class RawRunePostPop;','    // Rune304/건설후처리를','    void SetRunePostPop(','    // 현재Rune타입/실제가상표','    bool HandlesRune(','    const RawRunePostPop* runePostPop_')
        lines=text.splitlines(keepends=True);removed=[line for line in lines if any(line.startswith(p) for p in patterns)];assert len(removed)==6
        return ''.join(line for line in lines if line not in removed)
    text=text.replace('#include "o/RawRunePostPop.h"\n','').replace(' || HandlesRune(sid);',';').replace('    if (HandlesRune(sid)) runePostPop_->Validate(sid,flags,pop);\n','')
    start=text.index('// 실제Rune후처리의같은풀을');end=text.index('// 복원한 전용 자산 몸체',start);text=text[:start]+text[end:]
    start=text.index('    // Rune은304예약/건설후');end=text.index('    if (HandlesPriest(sid)) {',start);return text[:start]+text[end:]


def observe(commit):
    """새등록만역제거하면기존두파일의전체문자/토큰이기준07과같음을증명한다."""
    result={}
    # 이전다리/섬/사제/보호막/Carrier/Factory/자산의각전체본문까지전부비교한다.
    for name in SOURCES:
        baseline=git('show',commit+':'+name).decode('utf-8').replace('\r\n','\n');current=(ROOT/name).read_text(encoding='utf-8');existing=restore(name,current);assert existing==baseline and tokens(existing)==tokens(baseline),name
        result[name]=dict(baseline_normalized_sha256=sha(baseline.encode('utf-8')),current_sha256=sha((ROOT/name).read_bytes()),existing_token_sha256=sha(tokens(existing)),all_existing_source_unchanged=True)
    prior={}
    # 이전보고서바이트와그보고서의보호PE/export/oracle/fixture를바꾸지않는다.
    for name in PRIOR:
        path=ROOT/name
        if not path.exists():continue
        assert path.read_bytes()==git('show',commit+':'+name);data=json.loads(path.read_text(encoding='utf-8'));prior[name]=dict(sha256=sha(path.read_bytes()),total=data.get('total'),new_unique_cases=0)
        for file,value in data.get('files',{}).items():
            if file in SOURCES or '/src/' in file or '/tests/' in file and '/fixtures/' not in file:continue
            assert sha((ROOT/file).read_bytes())==value,file
    return dict(files=result,prior_reports=prior)


def generate():
    """기준07과새Rune등록의현재SHA를별도보고서로연결한다."""
    commit=git('rev-parse','b76adf9').decode('ascii').strip();REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',baseline_commit=commit,lineage=observe(commit),tool_sha256=sha(Path(__file__).read_bytes()),new_unique_cases=0,scope='새Rune등록/Validate/Activate분기만역제거하면기존전체소스/토큰동일. 기존공통/Pop/사제/건설native는새관찰에합산하지않는다.'),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');verify()


def verify():
    """현재등록/전체기존body/이전관찰출처불변을다시감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));assert report['new_unique_cases']==0 and report['lineage']==observe(report['baseline_commit']) and report['tool_sha256']==sha(Path(__file__).read_bytes());print('Rune등록 역제거·기준07 공통postPop 전체소스/토큰·이전원본근거 보존')


# 명시호출에서만새lineage를생성하거나현재출처를감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
