#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Early5의 추가 워크샾·네 기술 생산과 자연 C안내를 검증한다."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT, EXE
from cpp_production_panel_smoke import repeated
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_tutorial_harvest_smoke import original
from cpp_world_smoke import states

PLAN = ROOT/'cpppj/tests/fixtures/tutorial-five-production-ui-input.tsv'  # 실제 입력과 읽기 전용 보고만 있는 재현 계획이다.
REPORT = ROOT/'cpppj/tutorial-five-production-runtime.json'  # 현재 제품·실패 이력·실제 자연 진행의 출처다.


def sha(path):
    """전체 원문 바이트를 SHA256으로 고정한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def inspect(directory):
    """보호 TYPE 비용과 실제 워크샾 수명·소속 생산 지식·단계/돈을 대조한다."""
    execution = json.loads((directory/'run.json').read_text(encoding='utf-8'))  # 같은 실제 실행의 종료/보호 결과다.
    assert execution['returnCode']==0 and execution['originalsUnchanged']
    observed = states(directory/'states.tsv')  # 보고 이름마다 같은 실제 월드의 스냅샷을 읽는다.
    initial, placed, ready, goal = (observed[name] for name in ('initial','placedWorkshop','workshopReady','stageC'))
    definition = original('sunfactory.type')  # 제품과 독립적으로 보호 아카이브 원문을 읽는다.
    workshop_cost = int(re.search(r'\bcost\s*=\s*(\d+)', definition)[1])
    maximum = int(re.search(r'\bmaxHitPoints\s*=\s*(\d+)', definition)[1])
    stone_cost = int(re.search(r'\bcost\s*=\s*(\d+)', original('sunblocker.type'))[1])
    assert workshop_cost==800 and maximum==3000 and stone_cost==400
    old_ids = {sid for sid,row in initial['objects'].items() if row[:2]==['sunFactory','1']}  # 시작부터 있던 실제 워크샾 한 채다.
    new_ids = {sid for sid,row in placed['objects'].items() if row[:2]==['sunFactory','1']}-old_ids  # 이번 건설로 생긴 실제 SID다.
    assert len(old_ids)==len(new_ids)==1
    old_sid, new_sid = next(iter(old_ids)), next(iter(new_ids))  # 생산 내용물의 부모를 별개로 대조한다.
    assert int(placed['objects'][new_sid][12])==0 and int(ready['objects'][new_sid][12])==maximum
    assert ready['objects'][new_sid][3:5]==goal['objects'][new_sid][3:5]==['130','90']
    contents = repeated(directory/'states.tsv','raw_factory_content')  # 사전으로 합쳐 중복 행을 잃지 않는 원래 Form 목록이다.
    production = {(int(row[0]),int(row[2])) for row in contents['stageC'] if int(row[3])&16}
    assert production=={(old_sid,78),(old_sid,71),(new_sid,128),(new_sid,75)}
    assert not contents['initial']
    assert {(int(row[0]),int(row[2])) for row in contents['twoKnowledge'] if int(row[3])&16}=={(old_sid,78),(old_sid,71)}
    menus = repeated(directory/'states.tsv','raw_menu_object')  # 원본 메뉴가 새 워크샾 SID를 대상으로 열렸는지 확인한다.
    assert any(int(row[1])==new_sid for row in menus['newFactoryMenu'])
    assert float(initial['raw_sp'][1])==4000
    assert float(observed['placed']['raw_sp'][1])==4000-stone_cost
    assert float(placed['raw_sp'][1])==float(goal['raw_sp'][1])==4000-stone_cost-workshop_cost
    stages = list(dict.fromkeys(int(current['mission_gate'][1]) for current in observed.values()))  # 실제 안내 진행 순서를 유지한다.
    assert stages==[65,66,67]
    assert all(current['mission'][0].lower()=='tutorial5' and current['mission_gate'][0]=='5' for current in observed.values())
    assert goal['mission_gate'][:6]==['5','67','0','1','0','1'] and goal['page']==['C.'] and goal['paused']==['1']
    assert 'MORE' in goal['controls']
    return dict(naturalStageC=True, stages=['A','B','C'], oldWorkshop=old_sid, newWorkshop=new_sid, point=[130,90],
                workshopCost=workshop_cost, workshopHp=maximum, sp=4000-stone_cost-workshop_cost,
                production=[dict(parent=parent,type=kind) for parent,kind in sorted(production)],
                gameTime=float(goal['raw_world'][0]), originalsUnchanged=True, naturalTutorialFiveComplete=False, goalComplete=False)


def run(directory):
    """새 출력 경로에 같은 실제 입력만 재생하며 허용 설정과 보호 원본을 복구/대조한다."""
    assert not directory.exists(), directory
    directory.mkdir(parents=True)
    rows = []  # 그림 파일 경로만 이번 출력 폴더에 맞춘다.
    # 입력 순서와 실제 버튼/좌표는 그대로 유지한다.
    for line in PLAN.read_text(encoding='utf-8').splitlines():
        if not line or line.startswith('#'):
            continue
        frame, operation, argument = line.split('\t')
        assert operation in ('click','right','report','snapshot')
        if operation=='snapshot':
            argument = str(directory/(argument+'.bmp'))
        rows.append(f'{frame}\t{operation}\t{argument}\n')
    script = directory/'steps.tsv'  # 실제 입력 큐에 전달한 사본이다.
    script.write_text(''.join(rows),encoding='utf-8',newline='\n')
    command = [str(EXE),'--run',str(ROOT/'originals'),'--mission','tutorial5','--window','--no-audio','--frames','1452',
               '--set','SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0','--ui-script',str(script),'--ui-report',str(directory/'states.tsv')]
    startup = subprocess.STARTUPINFO()  # 별도 콘솔 창은 숨긴다.
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    before = snapshots(ROOT/'originals')  # 보호 원본 전체 파일의 실행 전 상태다.
    with preserve_game_settings(ROOT/'originals','d'):
        result = subprocess.run(command,cwd=ROOT,capture_output=True,startupinfo=startup,timeout=120)
        (directory/'stdout.log').write_bytes(result.stdout)
        (directory/'stderr.log').write_bytes(result.stderr)
    execution = dict(command=command,binarySha256=sha(EXE),returnCode=result.returncode,originalsUnchanged=snapshots(ROOT/'originals')==before)
    (directory/'run.json').write_text(json.dumps(execution,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    result = inspect(directory)  # 저장된 원문에서 검사하며 게임 상태를 변경하지 않는다.
    (directory/'results.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result,ensure_ascii=False))


def main():
    """실제 실행·저장 관측 재검사·현재 출처 검사 중 하나만 수행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--inspect',type=Path)
    parser.add_argument('--verify',action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 현재 소스와 원본/실제 관측을 원문 바이트로 각각 검사한다.
        for group in ('sources','evidence'):
            # 원본 기대값과 실패 로그의 해시도 보존한다.
            for name,expected in report[group].items():
                assert sha(ROOT/name)==expected,name
        tested = json.loads((ROOT/report['validation']['fullTests']).read_text(encoding='utf-8'))
        assert tested['tests']==1729 and tested['failed']==tested['returnCode']==0
        assert inspect(ROOT/report['validation']['naturalRun'])['naturalStageC']
        assert not report['goalComplete'] and not report['fullFidelityVerified']
        print('Early5 워크샾 본체 클릭/네 생산 지식/자연 C안내 출처 통과')
    elif args.inspect:
        print(json.dumps(inspect(args.inspect.resolve()),ensure_ascii=False))
    elif args.output:
        run(args.output.resolve())
    else:
        parser.error('--output, --inspect 또는 --verify가 필요합니다')


if __name__=='__main__':
    main()
