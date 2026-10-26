#!/usr/bin/env python3
"""목록두MapProbe와주Storage가같은Client표를빌리는세개작은제안hunk를만든다."""
import difflib
import hashlib
import json
from pathlib import Path
from prepare_storage_patch import replace_once

# 부모work안의새기준복사본만만들며현재제품에는쓰지않는다.
WORK=Path(__file__).resolve().parents[1]
ROOT=WORK.parents[2]


def main():
    """현행Catalog/Uber소스SHA를보존하고별도wiring patch를제공한다."""
    names=['cpppj/src/client/SoloMapCatalog.h','cpppj/src/client/SoloMapCatalog.cpp','cpppj/src/client/UberGump.cpp']
    sources={name:(ROOT/name).read_text(encoding='utf-8') for name in names}
    outputs=dict(sources)
    outputs[names[0]]=replace_once(outputs[names[0]],'namespace netstorm::client {','namespace netstorm::client {\nclass ClientTemplateSectionNames; // 모든Templatemanager가빌리는같은Client이름표owner다.')
    before='    std::function<std::optional<std::vector<std::uint8_t>>(std::string_view)> readFile; //선택적 현재 파일 reader다. 분류/메타마다 다시 호출하며 생략하면 기존 디스크/TAFF/보조 읽기를 유지한다.'
    outputs[names[0]]=replace_once(outputs[names[0]],before,before+'\n    std::shared_ptr<ClientTemplateSectionNames> sectionNames; // 두MapProbe도주Storage와같은005c8aa4표를빌린다.생략하면catalog수명에한개를소유한다.')
    outputs[names[1]]=replace_once(outputs[names[1]],'storage(effects.serializationGlobals),state(','storage(effects.serializationGlobals,effects.sectionNames),state(')
    marker='        if (!effects.fileSpecKey) effects.fileSpecKey='
    outputs[names[1]]=replace_once(outputs[names[1]],marker,'        if (!effects.sectionNames) effects.sectionNames=std::make_shared<ClientTemplateSectionNames>(); // 목록의모든임시manager가동일lazy표를공유한다.\n'+marker)
    marker='    mapEffects.serializationGlobals=serializationGlobals_;'
    outputs[names[2]]=replace_once(outputs[names[2]],marker,'    mapEffects.sectionNames=templateStorage_->SectionNamesOwner(); // 주Template/분류/메타가같은Client이름표와변환상태를빌린다.\n'+marker)
    patch=[]
    report={}
    # 원문기준은한번만기록하고각제안은같은work상대경로에둔다.
    for name in names:
        relative=Path(name).relative_to('cpppj')
        baseline=WORK/'baseline'/relative
        destination=WORK/relative
        baseline.parent.mkdir(parents=True,exist_ok=True)
        destination.parent.mkdir(parents=True,exist_ok=True)
        if baseline.exists() or destination.exists():
            raise RuntimeError('wiring기준/제안덮어쓰기금지:'+name)
        baseline.write_text(sources[name],encoding='utf-8',newline='')
        destination.write_text(outputs[name],encoding='utf-8',newline='')
        report[name]=dict(sourceSha256=hashlib.sha256((ROOT/name).read_bytes()).hexdigest(),proposalSha256=hashlib.sha256(destination.read_bytes()).hexdigest())
        patch.extend(difflib.unified_diff(sources[name].splitlines(True),outputs[name].splitlines(True),fromfile='a/'+name,tofile='b/'+name))
    (WORK/'client-wiring.patch').write_text(''.join(patch),encoding='utf-8',newline='')
    (WORK/'client-wiring-baselines.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':
    main()
