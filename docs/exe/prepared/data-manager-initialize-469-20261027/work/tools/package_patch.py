#!/usr/bin/env python3
"""미변경기준사본과work제안만비교해좁은전체patch와새manifest를정리한다."""
import ast
import difflib
import hashlib
import json
from pathlib import Path

# 부모Native76행및NewFort동결본은읽기전용근거로참조하고여기서는work만기록한다.
WORK=Path(__file__).resolve().parents[1]
ROOT=WORK.parents[2]


def main():
    """C++빌드없이UTF-8/Python AST/기준SHA/제안전체diff를만든다."""
    baseline={path.relative_to(WORK/'baseline').as_posix():path for path in (WORK/'baseline').rglob('*') if path.is_file()}
    sources={}
    # 최초제품의rawSHA와읽기용LF기준사본의SHA를구별한다.
    for record in ('storage-baselines.json','client-wiring-baselines.json'):
        sources.update(json.loads((WORK/record).read_text(encoding='utf-8')))
    patch=[]
    changes={}
    # 실제제품SHA는최초기준과달라도덮어쓰지않고명시보고하여Root가hunk별검토하게한다.
    for relative in sorted(baseline):
        before=baseline[relative].read_text(encoding='utf-8')
        after=(WORK/relative).read_text(encoding='utf-8')
        product='cpppj/'+relative
        changes[product]=dict(baselineSourceSha256=sources[product]['sourceSha256'],baselineNormalizedSha256=hashlib.sha256(baseline[relative].read_bytes()).hexdigest(),proposalSha256=hashlib.sha256((WORK/relative).read_bytes()).hexdigest(),currentProductSha256=hashlib.sha256((ROOT/product).read_bytes()).hexdigest())
        patch.extend(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+product,tofile='b/'+product))
    # 신규raw/Client모듈과검사도같은patch의새파일hunk로묶는다.
    for folder in ('src','tests'):
        for path in sorted((WORK/folder).rglob('*')):
            if not path.is_file():
                continue
            relative=path.relative_to(WORK).as_posix()
            if relative in baseline:
                continue
            after=path.read_text(encoding='utf-8')
            changes['cpppj/'+relative]=dict(newFile=True,sha256=hashlib.sha256(path.read_bytes()).hexdigest())
            patch.extend(difflib.unified_diff([],after.splitlines(True),fromfile='/dev/null',tofile='b/cpppj/'+relative))
    target=WORK/'data-manager-initialize-469.patch'
    target.write_text(''.join(patch),encoding='utf-8',newline='')
    (WORK/'final-changes.json').write_text(json.dumps(changes,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    files={}
    # manifest자신과Python캐시를제외한모든work문서/코드가UTF-8인지확인한다.
    for path in sorted(WORK.rglob('*')):
        if not path.is_file() or path.name=='manifest.json' or '__pycache__' in path.parts:
            continue
        payload=path.read_bytes()
        text=payload.decode('utf-8')
        if '\ufffd' in text:
            raise RuntimeError('UTF-8대체문자:'+str(path))
        if path.suffix=='.py':
            ast.parse(text)
        files[path.relative_to(WORK).as_posix()]=dict(bytes=len(payload),sha256=hashlib.sha256(payload).hexdigest())
    report=dict(host='HJOW-Athlon',lastFullDecompile='HJOW-Athlon / 2026-10-23',frozenDate='2026-10-27',status='work proposal only; C++ unbuilt/unregistered; Native76 baseline unchanged; no product/build/GUI changes',testsDrafted=6,patchSha256=hashlib.sha256(target.read_bytes()).hexdigest(),files=files)
    (WORK/'manifest.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(changedFiles=len(changes),patchSha256=report['patchSha256'],manifestSha256=hashlib.sha256((WORK/'manifest.json').read_bytes()).hexdigest())))


if __name__=='__main__':
    main()
