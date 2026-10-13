#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""공유 Dude Assets 한 inline 토큰을 전체 이전13/현재Player 자료와 정확 역변환으로 연결한다."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
import analyze_dude_postpop_binding_lineage as dude
import analyze_dudespawn_draft_lineage as spawn
import analyze_dudedrop_draft_lineage as drop
import analyze_player_world_attach_lineage as attach
import analyze_player_postpop_binding_lineage as player
import decomp_player_world_oracle as native_player
from analyze_campaign_runtime6 import read_log

# 현재공유헤더와변경전전체원문·기존보고서의별도successor경로다.
ROOT=Path(__file__).resolve().parent.parent
HISTORY=ROOT/'cpppj/hist/dude-assets-e44e0'
REPORT=ROOT/'cpppj/dude-assets-inline-lineage.json'
SUPPORT='cpppj/tests/DudeSpawnSupport.h'
COMMIT='860030e73e9e11599c89334b2e6ab4b0c6e1702d'
OLD=b'const DudeAssets& Assets(OriginalEdition edition)'
NEW=b'inline const DudeAssets& Assets(OriginalEdition edition)'
SHARED=('cpppj/src/o/SquidPostPop.h','cpppj/src/o/SquidPostPop.cpp')


def sha(body):
    """전체바이트SHA256이며주석/개행을생략하지않는다."""
    return hashlib.sha256(body).hexdigest()


def observe():
    """한토큰역변환과Git/LF·작업원문및기존Native/Player감사를재현한다."""
    capture=json.loads((HISTORY/'capture.json').read_text(encoding='utf-8'));assert capture['baseline_commit']==COMMIT
    old=(HISTORY/'work'/SUPPORT).read_bytes();current=(ROOT/SUPPORT).read_bytes();git_old=(HISTORY/'git'/SUPPORT).read_bytes()
    if current.count(NEW)!=1 or current.replace(NEW,OLD,1)!=old:raise RuntimeError('Dude Assets 전체inline 정확역변환 오류')
    actual_git=subprocess.run(['git','show',COMMIT+':'+SUPPORT],cwd=ROOT,capture_output=True,check=True).stdout
    if actual_git!=git_old or old.replace(b'\r\n',b'\n')!=git_old.replace(b'\r\n',b'\n'):raise RuntimeError('Dude Assets 기준13 Git/작업원문 오류')
    preserved={}
    # 이전보고서/전수/도구/본문/실패로그와모든참조whole자료를해시로보존한다.
    for name,item in capture['files'].items():
        saved=HISTORY/'work'/name;body=saved.read_bytes()
        if sha(body)!=item['work_sha256'] or len(body)!=item['bytes']:raise RuntimeError('Dude Assets 이전전체자료 SHA 오류 '+name)
        preserved[saved.relative_to(ROOT).as_posix()]=item['work_sha256']
    preserved[(HISTORY/'git'/SUPPORT).relative_to(ROOT).as_posix()]=sha(git_old);preserved[(HISTORY/'capture.json').relative_to(ROOT).as_posix()]=sha((HISTORY/'capture.json').read_bytes())
    # 기존보고서는당시SHA를덮지않으며현재JSON/fixture/Native생산/도구도불변이어야한다.
    unchanged=set(capture['reports'])|{SUPPORT,'cpppj/tests/DudeDropTests.cpp','cpppj/tests/DudeSpawnTests.cpp','cpppj/tests/PlayerWorldTests.cpp','cpppj/tests/fixtures/dudespawn-x86.tsv','cpppj/tests/fixtures/dudedrop-x86.tsv','cpppj/tests/fixtures/player-world-x86.tsv','cpppj/src/o/RawDudeSpawn.h','cpppj/src/o/RawDudeSpawn.cpp','cpppj/src/o/RawDudeDrop.h','cpppj/src/o/RawDudeDrop.cpp','cpppj/src/o/RawPlayerWorld.h','cpppj/src/o/RawPlayerWorld.cpp','tools/decomp_dudespawn_oracle.py','tools/decomp_dudedrop_oracle.py','tools/decomp_player_world_oracle.py','tools/analyze_dude_postpop_binding_lineage.py','tools/analyze_dudespawn_draft_lineage.py','tools/analyze_dudedrop_draft_lineage.py','tools/analyze_player_world_attach_lineage.py','tools/analyze_player_postpop_binding_lineage.py','extracted/run14-build-third.log'}
    for name in unchanged-{SUPPORT}:
        if (ROOT/name).read_bytes()!=(HISTORY/'work'/name).read_bytes():raise RuntimeError('Dude Assets 기존report/Native/body/fixture 변경 '+name)
    log=read_log(ROOT/'extracted/run14-build-third.log')
    if 'LNK2005' not in log or 'PlayerWorldTests' not in log or 'DudeDropTests' not in log:raise RuntimeError('Dude Assets 완료third-build 중복정의근거 오류')
    unit13=json.loads((ROOT/'cpppj/run13.json').read_text(encoding='utf-8'));previous_dependencies={}
    # 단위13최종전체파일은당시Player등록전보존view에서검증한다. 현재14바이너리로대체하지않는다.
    for name,value in unit13['files'].items():
        saved=player.HISTORY/'work'/name;path=saved if saved.is_file() else ROOT/name
        if sha(path.read_bytes())!=value:raise RuntimeError('Dude Assets 이전13 전체근거 오류 '+name)
        previous_dependencies[name]=dict(reported_sha256=value,verified_path=path.relative_to(ROOT).as_posix())
    # Player새분배만제거한두공유본문은실제기준13전체바이트와같아야한다.
    historical_views={SUPPORT:old}
    for name in SHARED:
        before=(player.HISTORY/'work'/name).read_bytes();restored=player.restore(name,(ROOT/name).read_text(encoding='utf-8'));body=restored.replace('\n','\r\n').encode('utf-8') if b'\r\n' in before else restored.encode('utf-8')
        if body!=before:raise RuntimeError('Dude Assets 이전13 공유view 역변환 오류 '+name)
        historical_views[name]=before
    # 현재Player Native/attach/공유lineage 자체는보고서를고치지않고그대로감사한다.
    native_player.verify();attach.verify();player.main(True)
    original_bytes,original_text=Path.read_bytes,Path.read_text
    views={(ROOT/name).resolve():body for name,body in historical_views.items()}
    def previous_bytes(path):
        """역변환확인한Support와13공유2파일만당시전체바이트로읽고파일을쓰지않는다."""
        body=views.get(path.resolve());return body if body is not None else original_bytes(path)
    def previous_text(path,*args,**kwargs):
        """같은역변환view를UTF8/보편개행으로읽는다. 나머지읽기는기존함수에넘긴다."""
        body=views.get(path.resolve())
        if body is not None:return body.decode(kwargs.get('encoding','utf-8')).replace('\r\n','\n')
        return original_text(path,*args,**kwargs)
    Path.read_bytes,Path.read_text=previous_bytes,previous_text
    try:
        dude.main(True);spawn.verify();drop.main(True)
    finally:
        Path.read_bytes,Path.read_text=original_bytes,original_text
    return dict(history=preserved,whole_old_work_bytes_inverse=True,whole_old_git_normalized_inverse=True,
        source=dict(path=SUPPORT,baseline_git_sha256=sha(git_old),previous_work_sha256=sha(old),current_sha256=sha(current)),
        previous_unit13=dict(all_tests=unit13['all_tests'],dependencies=previous_dependencies),
        reproduced_audits=['DudePostPop기준12→13전체역변환','DudeSpawn초안288','DudeDrop초안308','현재Player1680/실제Attach80','Player기준13→14전체역변환'],
        files={name:sha((ROOT/name).read_bytes()) for name in sorted(unchanged|set(SHARED))})


def main(check):
    """새Native0/한inline토큰의현재소비자SHA를별도보고서에만기록한다."""
    data=dict(schema=1,host='HJOW-Athlon',date='2026-10-14',primary_target='10.78',cd_target='10.72',baseline_commit=COMMIT,new_native_inputs=0,new_cpp_tests=0,cpp_build_completed=False,
        unchanged_native=dict(dudespawn=288,dudedrop_normal=306,dudedrop_assertions=2,player_normal_unique=1452,player_assert_unique=228,player_fixture_rows=1680),
        lineage=observe(),tool_sha256=sha(Path(__file__).read_bytes()),
        scope='DudeSpawnSupport.h의Assets함수앞inline한토큰만추가한다. PlayerWorldTests와DudeDropTests의LNK2005완료로그/기준13및현재Player전체근거를보존한다. 댓글·함수body·서버Native·fixture변경0이다. 이전Dude/Drop감사는inline제거및Player분배제거가정확확인된당시readview로재현하며기존보고서를덮어쓰지않는다. C++재빌드/대조는root후속이다.')
    if check:
        if json.loads(REPORT.read_text(encoding='utf-8'))!=data:raise RuntimeError('Dude Assets inline successor 현재SHA 오류')
    else:
        if REPORT.exists():raise RuntimeError('Dude Assets inline successor 덮어쓰기 금지')
        REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(verified=True,previous_all_tests=1066,new_native_inputs=0,changed_tokens=1,cpp_build_completed=False),ensure_ascii=False))


# 명시CLI로별도successor를생성하거나현재정확역변환을감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();main(args.verify)
