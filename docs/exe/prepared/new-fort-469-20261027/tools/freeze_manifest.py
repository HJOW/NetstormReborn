#!/usr/bin/env python3
"""NewFort 준비본의 UTF-8/Python문법을 확인하고 채택 전 전체 파일 SHA를 고정한다."""
import ast
import hashlib
import json
from pathlib import Path

# 이 도구가 속한 staging만 읽고 새 manifest만 기록한다.
STAGE=Path(__file__).resolve().parents[1]


def main():
    """빌드 없이 각 준비 파일을 읽으며 manifest 자신과 Python 캐시는 제외한다."""
    files={}
    replacements=[]
    # 모든 텍스트가 UTF-8로 읽히는지 확인하고 Python은 AST만 해석한다. 실행/컴파일은 하지 않는다.
    for path in sorted(STAGE.rglob('*')):
        if not path.is_file() or '__pycache__' in path.parts or path.name=='manifest.json':
            continue
        data=path.read_bytes()
        text=data.decode('utf-8')
        if '\ufffd' in text:
            replacements.append(str(path.relative_to(STAGE)))
        if path.suffix=='.py':
            ast.parse(text)
        files[path.relative_to(STAGE).as_posix()]=dict(bytes=len(data),sha256=hashlib.sha256(data).hexdigest())
    report=dict(host='HJOW-Athlon',lastFullDecompile='HJOW-Athlon / 2026-10-23',frozenDate='2026-10-27',status='staging only; Native fixtures verified twice; C++ unbuilt/unregistered; product/GUI unchanged',nativeRows=1400,testsDrafted=4,utf8ReplacementFiles=replacements,files=files)
    target=STAGE/'manifest.json'
    target.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(files=len(files),utf8ReplacementFiles=replacements,manifestSha256=hashlib.sha256(target.read_bytes()).hexdigest()),ensure_ascii=False))


if __name__=='__main__':
    main()
