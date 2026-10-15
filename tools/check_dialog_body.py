"""실제 Early1/2 입력 후 Kernel의 현재 미션 섹션과 일반 Warning fallback을 검사한다."""
import json
import sys
from pathlib import Path

# 기존 실제 UI/보호 파일 보관 도구를 공유한다.
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import check_dialog_controls as dialog
from cpp_smoke_files import preserve_game_settings
from cpp_renderer_smoke import snapshots

# 이번 검사 전용 출력은 이전 단위36 자료와 분리한다.
dialog.OUTPUT=ROOT/'extracted/dialog-body-mission-tell'
dialog.OUTPUT.mkdir(exist_ok=True)
EXE=ROOT/'cpppj/build/bin/Release/NetstormCpp.exe'


def check(name,more):
    """최초브리핑은 실제 버튼으로 닫고 같은월드의 B/C를 실제Kernel에서 요청한다."""
    steps=[(3,'report','initial')]
    # 원본 A1/A2의 MORE 버튼을 실제 명령으로 순회한다.
    for index in range(more):
        steps.extend([(4+index*2,'click','MORE'),(5+index*2,'report',f'a{index+1}')])
    frame=4+more*2
    steps.extend([(frame,'click','OK'),(frame+1,'report','playing'),
        (frame+2,'kernel-tell','B.'),(frame+2,'report','b'),
        (frame+2,'snapshot',str(dialog.OUTPUT/f'{name}-b.bmp'))])
    frame+=3
    if name=='tutorial2':
        steps.extend([(frame,'click','MORE'),(frame+1,'report','b1')]);frame+=2
    steps.extend([(frame,'click','OK'),(frame+1,'report','after-b'),
        (frame+2,'kernel-tell','C.'),(frame+2,'report','c'),
        (frame+2,'snapshot',str(dialog.OUTPUT/f'{name}-c.bmp'))]);frame+=3
    if name=='tutorial1':
        steps.extend([(frame,'click','MORE'),(frame+1,'report','c1'),
            (frame+2,'click','BACK'),(frame+3,'report','c-again'),
            (frame+4,'click','MORE'),(frame+5,'report','c1-again')]);frame+=6
    steps.extend([(frame,'click','OK'),(frame+1,'report','after-c'),
        (frame+2,'kernel-tell','PriestImmobile'),(frame+2,'report','warning_queued'),(frame+3,'report','warning')])
    states=dialog.run_scene(EXE,name,steps,mission=name)
    assert states['initial']['page']==['A.']
    assert states['playing']['page']==['mission'] and states['playing']['paused']==['0']
    for key,target in [('b','B.'),('c','C.')]:
        assert states[key]['page']==[target] and states[key]['phase']==['1'] and states[key]['paused']==['1']
    for key in ['after-b','after-c','warning']:
        assert states[key]['page']==['mission'] and states[key]['paused']==['0']
    assert states['warning_queued']['warning_count'][0]=='0'
    assert states['warning']['warning_count'][0]=='1'
    if name=='tutorial1':
        assert states['c1']['page']==['C1.'] and states['c-again']['page']==['C.'] and states['c1-again']['page']==['C1.']
    else:assert states['b1']['page']==['B1.']
    return {'states':len(states),'kernel_current_sections':True,'buttons_and_back':True,'common_warning_fallback':True}


# 두 실제클론이 허용설정을 원상복구하고 다른원본파일도 바꾸지 않았는지 확인한다.
before=snapshots(ROOT/'originals')
with preserve_game_settings(ROOT/'originals','d'):
    report={name:check(name,more) for name,more in [('tutorial1',1),('tutorial2',2)]}
assert snapshots(ROOT/'originals')==before
report['originals_unchanged']=True
(dialog.OUTPUT/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,ensure_ascii=False))
