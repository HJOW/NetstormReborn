#!/usr/bin/env python3
"""빈manager초기화연구의UTF-8/도구AST와추가관측파일SHA만고정한다."""
import ast
import hashlib
import json
from pathlib import Path

# 현재준비본만읽고기존NewFort준비본이나제품파일에는접근하지않는다.
STAGE=Path(__file__).resolve().parents[1]


def main():
    """제품빌드없이각텍스트를확인하고자신을포함한manifest입력해시를기록한다."""
    files={}
    # manifest자신과Python캐시만제외하여재생성할때같은해시목록을얻는다.
    for path in sorted(STAGE.rglob('*')):
        if not path.is_file() or '__pycache__' in path.parts or path.name=='manifest.json':
            continue
        payload=path.read_bytes()
        text=payload.decode('utf-8')
        if '\ufffd' in text:
            raise RuntimeError('UTF-8대체문자:'+str(path))
        if path.suffix=='.py':
            ast.parse(text)
        files[path.relative_to(STAGE).as_posix()]=dict(bytes=len(payload),sha256=hashlib.sha256(payload).hexdigest())
    report=dict(host='HJOW-Athlon',lastFullDecompile='HJOW-Athlon / 2026-10-23',frozenDate='2026-10-27',status='read-only child research; Native76 verified twice; C++ implementation/tests absent; no product/build/GUI changes',files=files)
    target=STAGE/'manifest.json'
    target.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(files=len(files),sha256=hashlib.sha256(target.read_bytes()).hexdigest())))


if __name__=='__main__':
    main()
