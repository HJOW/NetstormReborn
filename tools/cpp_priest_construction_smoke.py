#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제1-1 사제 메뉴에서 워크샾 배치/3e왕복/비용 표시/이동까지 검사한다. 건설 완공 검사가 아니다."""
import argparse
import json
import subprocess
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT, EXE
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states


def main():
    """같은 실제 시작 자금/기술로 GUI만 조작하며 원본 보호 자료와 관측 결과를 보존한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    output=args.output.resolve()
    if output.exists() and any(output.iterdir()):raise RuntimeError('새 관측 경로가 필요합니다: '+str(output))
    output.mkdir(parents=True,exist_ok=True)
    steps=[(5,'click','Play Mission'),(7,'right','world:priest'),(35,'click','Construct'),(65,'click','Workshop'),
           (95,'click',' Build Sun Workshop for 800'),(102,'up','cell:107,103'),(105,'report','prepared'),
           (110,'click','cell:107,103'),(120,'report','placed'),(120,'snapshot',str(output/'placed.bmp')),
           (200,'report','moving'),(200,'snapshot',str(output/'moving.bmp')),(230,'report','costDeleted'),(230,'snapshot',str(output/'cost-deleted.bmp'))]
    script=output/'steps.tsv';report=output/'states.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in steps),encoding='utf-8',newline='\n')
    command=[str(EXE),'--run',str(ROOT/'originals'),'--mission','thewarbegins','--window','--no-audio','--frames','233',
             '--set','SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8','--ui-script',str(script),'--ui-report',str(report)]
    before=snapshots(ROOT/'originals')
    with preserve_game_settings(ROOT/'originals','d'):
        startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
        result=subprocess.run(command,cwd=ROOT,capture_output=True,startupinfo=startup,timeout=60)
        (output/'stdout.log').write_bytes(result.stdout);(output/'stderr.log').write_bytes(result.stderr)
    unchanged=snapshots(ROOT/'originals')==before
    assert result.returncode==0 and unchanged,dict(exitCode=result.returncode,stderr=result.stderr.decode('utf-8',errors='replace'),originals_unchanged=unchanged)
    observed=states(report);prepared=observed['prepared'];placed=observed['placed'];moving=observed['moving'];deleted=observed['costDeleted']
    assert prepared['raw_sp'][1]=='3000' and prepared['raw_cursor'][4:6]==['109','107'] and prepared['raw_cursor'][8:]==['1','1','1']
    before_packets=list(map(int,prepared['raw_packets']));after_packets=list(map(int,placed['raw_packets']))
    assert after_packets[:len(before_packets)]==before_packets and after_packets[len(before_packets):]==[0x3e]
    #현재 실제 저장소의 SP/선택/커서/워크샾을 검사하며 성공 플래그를 주입하지 않는다.
    for current in (placed,moving,deleted):
        assert current['phase']==['4'] and current['paused']==['0'] and current['popup']==['']
        assert current['raw_input'][:3]==['0','0','14'] and current['raw_sp'][1]=='2200'
        assert current['raw_cursor'][0]=='2' and current['raw_cursor'][3]=='0'
    workshops={sid:row for sid,row in placed['raw_objects'].items() if row[:2]==['85','1'] and row[-2:]==['109','107']}
    assert len(workshops)==1,workshops
    priest=prepared['raw_objects'][15000];moved=moving['raw_objects'][15000]
    assert priest[-2:]==['94','108'] and moved[-2:]!=priest[-2:]
    floating=placed['raw_sp_gump'];later=moving['raw_sp_gump']
    assert floating[:3]==later[:3] and floating[7]==later[7]=='-2' and int(floating[9])!=0
    assert float(later[5])<float(floating[5]) and float(later[6])<float(floating[6])
    assert 'raw_sp_gump' not in deleted and all(current['raw_menus'][0]=='0' for current in (placed,moving,deleted))
    summary=dict(passed=True,scope='실제GUI 사제 메뉴/TYPE85 배치/실제3e왕복/SP 차감/원본 서식 비용 표시·이동·지연삭제/사제 경로 이동. 도착 뒤 건설 프로세스 시작/완공·미션 완주는 별도 후속이다.',
                 command=command,originals_unchanged=unchanged,newPacketTypes=after_packets[len(before_packets):],workshops=workshops,priestBefore=priest,priestMoving=moved,
                 observed={label:{key:record[key] for key in ('time','raw_input','raw_cursor','raw_sp','raw_menus','raw_sp_gump') if key in record} for label,record in observed.items()})
    (output/'results.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(passed=True,newPacketTypes=summary['newPacketTypes'],workshops=workshops,priestBefore=priest,priestMoving=moved,originals_unchanged=unchanged),ensure_ascii=False))


if __name__=='__main__':main()
