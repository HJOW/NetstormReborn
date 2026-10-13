#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""같은월드 의식소비자의 CD 표면조회 최소교정과현재생산/선행원본근거를연결한다."""
import argparse,json
from pathlib import Path
from decomp_daiscontentnotice_oracle import ROOT,sha

TEST='cpppj/tests/DaisRitualConsumerTests.cpp' # 실제같은Pool/Host/Kernel 삭제사슬의새검사다.
HISTORY=ROOT/'cpppj/hist/dais-ritual-47665' # 원본fadd확인전의전체새검사를보존한다.
REPORT=ROOT/'cpppj/dais-ritual-consumer-binding.json' # 새원본관찰을추가하지않는현재소비자연결근거다.

def expected():
    """전체옛검사역변환과실제몸체/선행독립근거현재SHA를연결한다."""
    current=(ROOT/TEST).read_text(encoding='utf-8');previous=(HISTORY/TEST).read_text(encoding='utf-8');new='return RawPathSurfaceAt(hash,{x,y});';old='return patch ? RawPathSurfaceAt(hash,{x,y}) : Sid{hash.Cell(0,static_cast<int>(x),static_cast<int>(y))};'
    assert current.count(new)==1 and current.replace(new,old)==previous
    files={Path(__file__),ROOT/TEST,HISTORY/TEST}
    # 같은Pool삭제에실제연결한생산몸체와현재표면helper/분배기를빠짐없이묶는다.
    for name in ('RawDaisRegular','RawAltarSacrifice','RawDaisLifecycle','RawDaisContentNotice','RawDamageablePostDestroy','RawDamageablePreDestroy','RawDamageableRelease','RawCarrierPreDestroy','RawCarrierPostPop','RawPathSearch','ContentFormLifetime','ContentFormHost','SquidFormDispatch','SquidDestroyLifecycle','SquidProcess','SquidPostPop','SquidPop','SquidUnpop','SquidFrame','SquidFactory','SquidOwner','SquidReward'):
        for suffix in ('.h','.cpp'):files.add(ROOT/f'cpppj/src/o/{name}{suffix}')
    # 정밀원본부모몸체와새API/12입력셀교정은각자당시관찰수로보존한다.
    prior={}
    for name in ('daislifecycle-current.json','daislifecycle-current-successor.json','daiscontentnotice.json','daiscontentnotice-successor.json','daisprogress.json','dais-postpop-lineage.json','damageablepostdestroy.json','damageablepostdestroy-lineage.json'):
        path=ROOT/'cpppj'/name;data=json.loads(path.read_text(encoding='utf-8'));files.add(path);prior[name]=dict(sha256=sha(path),normal_inputs=data.get('total',data.get('normal_inputs')),additional_unique_cases=0)
    return dict(host='HJOW-Athlon',decompile_date='2026-10-14',new_native_inputs=0,entire_previous_test_restored_exactly=True,previous_test_sha256=sha(HISTORY/TEST),
        correction='CD0048153f/48154e의fadd0.9999→__ftol에맞춰CD도같은RawPathSurfaceAt/hash0를호출한다. plain조회삼항식하나외전체새검사는정확히불변이다.',
        scope='실제최초Pop/포획Notice/Altar24/전체pre/post/Content/Process분배/같은Pool반납의새C++소비자. 비전투·HP16·genus0·flags300000없음으로Dude/주변피해/다리/섬정리분기는자연생략. 장치/UI·전체Progress음향/Bridge·사제해방전체Pop는미완료경계이며C++실행결과는별도공유로그다.',
        prior_reports=prior,files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(files)})

def main(check):
    """현재소비자연결SHA를작성하거나정확최소교정을감사한다."""
    data=expected()
    if check:assert json.loads(REPORT.read_text(encoding='utf-8'))==data
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('실제의식소비자CD표면교정: 전체역변환/현재생산·선행원본SHA/새관찰0')

# 명시CLI에서만현재연결자료를작성하거나감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();main(args.verify)
