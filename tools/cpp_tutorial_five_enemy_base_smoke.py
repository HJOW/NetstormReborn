#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Early5의 C/C1·적 훨리기그 베이스 선택·대기 후 D안내를 확인한다."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT, EXE
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states

PLAN = ROOT/'cpppj/tests/fixtures/tutorial-five-enemy-base-ui-input.tsv'  # 앞선 건설/생산에 선택 입력을 이어 붙인 계획이다.
REPORT = ROOT/'cpppj/tutorial-five-enemy-base-runtime.json'  # 현재 제품과 실제 관측을 연결하는 출처다.


def sha(path):
    """전체 원문 바이트의 SHA256이다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def inspect(directory):
    """적 두 건물이 살아 있을 때 베이스 선택·대기·선택 해제·D안내를 대조한다."""
    execution = json.loads((directory/'run.json').read_text(encoding='utf-8'))  # 실제 실행의 종료와 보호 원본 대조 결과다.
    assert execution['returnCode']==0 and execution['originalsUnchanged']
    observed = states(directory/'states.tsv')  # 실제 저장 프레임만 읽는다.
    stage_c, c1, view, selected, early, goal = (observed[name] for name in ('stageC','stageC1','enemyView','enemySelected','selectionEarly','stageD'))
    assert stage_c['page']==['C.'] and c1['page']==['C1.'] and {'BACK','OK'}<=set(c1['controls'])
    assert view['page']==['mission'] and view['paused']==['0'] and view['selected']==['0']
    sid = int(selected['selected'][0])  # 실제 클릭으로 선택된 같은 원본 SID다.
    assert sid and selected['objects'][sid][:2]==['sunAviary','2']
    assert selected['objects'][sid][3:5]==['139','129'] and selected['objects'][sid][12]=='200'
    assert selected['mission_gate'][1]=='67' and selected['paused']==['0']
    assert early['selected']==[str(sid)] and early['mission_gate'][1]=='67' and early['page']==['mission']
    assert 0<float(early['raw_world'][0])-float(selected['raw_world'][0])<4
    assert goal['selected']==['0'] and goal['objects'][sid][:2]==['sunAviary','2'] and goal['objects'][sid][12]=='200'
    assert any(row[:2]==['thunderCannon','2'] and row[12]=='1000' for row in goal['objects'].values())
    assert float(goal['raw_world'][0])-float(selected['raw_world'][0])>=4
    assert goal['mission_gate'][:6]==['5','68','0','1','0','1'] and goal['page']==['D.'] and goal['paused']==['1']
    assert 'MORE' in goal['controls'] and float(goal['raw_sp'][1])==2800
    stages = list(dict.fromkeys(int(current['mission_gate'][1]) for current in observed.values()))  # 원래 관측 순서를 유지한다.
    assert stages==[65,66,67,68]
    assert all(current['mission'][0].lower()=='tutorial5' and current['mission_gate'][0]=='5' for current in observed.values())
    assert view['camera'][0]==stage_c['camera'][0] and int(view['camera'][1])>int(stage_c['camera'][1])
    return dict(naturalStageD=True,stages=['A','B','C','D'],selectedBase=sid,type=72,owner=2,baseHp=200,cannonHp=1000,
                earlyWait=float(early['raw_world'][0])-float(selected['raw_world'][0]),
                observedWait=float(goal['raw_world'][0])-float(selected['raw_world'][0]),sp=2800,
                cameraBefore=stage_c['camera'],cameraAfter=view['camera'],gameTime=float(goal['raw_world'][0]),
                originalsUnchanged=True,naturalTutorialFiveComplete=False,goalComplete=False)


def run(directory):
    """현재 실제 입력 큐만 사용하고 새 관측과 보호 원본을 별개로 보관한다."""
    assert not directory.exists(),directory
    directory.mkdir(parents=True)
    rows = []  # 그림 경로 외 실제 입력 내용은 유지한다.
    # 앞선 생산 계획의 모든 입력도 같은 순서로 실행한다.
    for line in PLAN.read_text(encoding='utf-8').splitlines():
        if not line or line.startswith('#'):
            continue
        frame,operation,argument = line.split('\t')
        assert operation in ('click','right','report','snapshot','key','keyup')
        if operation=='snapshot':
            argument = str(directory/(argument+'.bmp'))
        rows.append(f'{frame}\t{operation}\t{argument}\n')
    script = directory/'steps.tsv'  # 실제로 전달한 입력 원문이다.
    script.write_text(''.join(rows),encoding='utf-8',newline='\n')
    command = [str(EXE),'--run',str(ROOT/'originals'),'--mission','tutorial5','--window','--no-audio','--frames','1952',
               '--set','SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0','--ui-script',str(script),'--ui-report',str(directory/'states.tsv')]
    startup = subprocess.STARTUPINFO()  # 별도 콘솔 창은 숨긴다.
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    before = snapshots(ROOT/'originals')  # 보호 원본 전체 파일을 대조한다.
    with preserve_game_settings(ROOT/'originals','d'):
        result = subprocess.run(command,cwd=ROOT,capture_output=True,startupinfo=startup,timeout=160)
        (directory/'stdout.log').write_bytes(result.stdout)
        (directory/'stderr.log').write_bytes(result.stderr)
    execution = dict(command=command,binarySha256=sha(EXE),returnCode=result.returncode,originalsUnchanged=snapshots(ROOT/'originals')==before)
    (directory/'run.json').write_text(json.dumps(execution,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    result = inspect(directory)  # 저장된 원문만 검사하고 게임 값을 주입하지 않는다.
    (directory/'results.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result,ensure_ascii=False))


def main():
    """새 실행·저장 관측·현재 출처 확인 중 요청한 하나만 수행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--inspect',type=Path)
    parser.add_argument('--verify',action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 과거 실패와 현재 소스의 전체 바이트를 함께 검사한다.
        for group in ('sources','evidence'):
            # 현재 해시가 다르면 해당 파일에서 실패한다.
            for name,expected in report[group].items():
                assert sha(ROOT/name)==expected,name
        assert inspect(ROOT/report['validation']['naturalRun'])['naturalStageD']
        assert report['binarySha256']==sha(EXE) and not report['goalComplete'] and not report['fullFidelityVerified']
        print('Early5 적 훨리기그 베이스 선택/대기/자연 D안내 출처 통과')
    elif args.inspect:
        print(json.dumps(inspect(args.inspect.resolve()),ensure_ascii=False))
    elif args.output:
        run(args.output.resolve())
    else:
        parser.error('--output, --inspect 또는 --verify가 필요합니다')


if __name__=='__main__':
    main()
