#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""입력28개+패치만으로 새 경로에 결과32개를 복원한다. main/CPP/PE는 실행하지 않는다."""
from pathlib import Path
import hashlib, json, subprocess, sys

P=Path(__file__).resolve().parent

def main():
    """지정한 빈 새 경로만 만들고 각 결과의 정규화 SHA를 동결 manifest와 대조한다."""
    if len(sys.argv)!=2:raise SystemExit('사용법: python replay.py <존재하지 않는 출력 경로>')
    output=Path(sys.argv[1]).resolve()
    if output.exists():raise SystemExit('출력 경로가 이미 존재하여 변경하지 않았습니다.')
    manifest=json.loads((P/'manifest.json').read_text(encoding='utf-8'))
    output.mkdir(parents=True)
    for row in manifest['files']:
        # 원시 입력은 기존 EOL을 보존한 채 별도 출력에만 복사한다.
        if row['new']:continue
        source=P/'original'/row['path'];data=source.read_bytes()
        assert hashlib.sha256(data).hexdigest()==row['originalSHA256'],row['path']
        target=output/row['path'];target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    patch=P/manifest['patch']['path'];assert hashlib.sha256(patch.read_bytes()).hexdigest()==manifest['patch']['sha256']
    results=[]
    for argv in [['git','init','--quiet'],['git','-c','core.autocrlf=false','apply','--check','--ignore-space-change',str(patch)],['git','-c','core.autocrlf=false','apply','--ignore-space-change',str(patch)]]:
        # 실제 main과 독립된 새 저장소에서 Git만 실행한다.
        run=subprocess.run(argv,cwd=output,capture_output=True,text=True,encoding='utf-8',errors='replace');results.append({'argv':argv,'exitCode':run.returncode,'stdout':run.stdout,'stderr':run.stderr});assert run.returncode==0,results[-1]
    for row in manifest['files']:
        # 패치 복원 결과의 원시 EOL은 달라도 정규화 본문은 동결 기대 SHA와 같아야 한다.
        data=(output/row['path']).read_bytes();text=data.decode('utf-8-sig').replace('\r\n','\n').replace('\r','\n');assert hashlib.sha256(text.encode('utf-8')).hexdigest()==row['normalizedSHA256'],row['path']
    report={'status':'Git 패치 복원 및 정규화 SHA 대조만 완료; CPP/PE/게임 실행 없음','inputs':manifest['inputFiles'],'outputs':manifest['preparedFiles'],'matched':len(manifest['files']),'commands':results}
    (output/'replay-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'inputs':report['inputs'],'outputs':report['outputs'],'matched':report['matched'],'exitCodes':[r['exitCode'] for r in results]},ensure_ascii=False))

if __name__=='__main__':main()
