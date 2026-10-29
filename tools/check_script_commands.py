#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Kernel 완료안내의명령예약→다음입력→옵션저장·다음미션을검사한다. 플레이완주검사는아니다."""
import json
import subprocess
from pathlib import Path
import check_dialog_controls as dialogs
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings

# 실제클론과이번검사전용출력경로다.
ROOT=Path(__file__).resolve().parents[1]
EXE=ROOT/'cpppj/build/bin/Release/Netstorm.exe'
OUTPUT=ROOT/'extracted/script-commands-ui'


def check_tutorials():
    """원본Early1/2완료섹션과다음미션버튼에서현재저장값/인계순서를확인한다."""
    steps=[(3,'report','early1-a'),(4,'click','MORE'),(6,'click','OK'),(7,'report','early1-playing'),
        (8,'kernel-tell','G.'),(8,'report','early1-queued'),(9,'report','early1-applied'),
        (9,'snapshot',str(OUTPUT/'early1-complete.bmp')),(10,'click','Next Tutorial'),(13,'report','early2-a'),
        (14,'click','MORE'),(16,'click','MORE'),(18,'click','OK'),(19,'report','early2-playing'),
        (20,'kernel-tell','I.'),(20,'report','early2-queued'),(21,'report','early2-applied'),
        (21,'snapshot',str(OUTPUT/'early2-complete.bmp')),(22,'click','Leave Tutorials'),(24,'report','left')]
    states=dialogs.run_scene(EXE,'tutorials',steps,'tutorial1','DoneTutorial1=0;DoneTutorial2=0')
    # 실제G./I.명령은Kernel에서예약한같은프레임에는설정이바뀌지않고다음Input에서적용된다.
    for prefix,section in (('early1','G.'),('early2','I.')):
        assert states[prefix+'-playing']['page']==['mission']
        assert states[prefix+'-queued']['page']==[section] and states[prefix+'-queued']['mission_progress']==['0']
        assert states[prefix+'-applied']['mission_progress']==['1']
    assert states['early2-a']['mission'][0]=='Tutorial2' and states['left']['page']==['main']
    # 원래저장경로를읽고메모리상태만바뀐것으로오인하지않는다.
    for key in ('DoneTutorial1','DoneTutorial2'):
        value=subprocess.run([str(EXE),'--config',str(ROOT/'originals/d/options.cfg'),key],capture_output=True,check=True).stdout.decode('utf-8').strip()
        assert value=='1',(key,value)
    return dict(states=len(states),deferred_config=True,next_tutorial=True,options_persisted=True)


def main():
    """보호원본의전체해시와허용설정의바이트를성공·실패에모두보존한다."""
    OUTPUT.mkdir(exist_ok=True);dialogs.OUTPUT=OUTPUT;before=snapshots(ROOT/'originals')
    try:
        with preserve_game_settings(ROOT/'originals','d'):result=check_tutorials()
    finally:
        assert snapshots(ROOT/'originals')==before,'보호원본파일이변경됨'
    result.update(originals_unchanged=True,original_game_executions=0,actual_gameplay_completion=False)
    (OUTPUT/'report.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(result,ensure_ascii=False))


if __name__=='__main__':main()
