#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제1-1 우클릭→Construct→Workshop→커서취득/취소/다시열기의 메뉴·명령 수명을 검사한다."""
import argparse
import json
import subprocess
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT,EXE
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states


def menu_items(path):
    """각 실제 보고 시점의 원본 항목/명령/인자/부모 표시 상태를 읽는다."""
    result={};label=None
    # 행 시작의 실제 frame만 경계로 쓰며 raw_renderer_frame 같은 통계 이름을 나누지 않는다.
    for line in path.read_text('utf-8').splitlines():
        fields=line.split('\t')
        if fields[0]=='frame':
            label=fields[2];result[label]=[]
        elif label is not None and fields[0]=='raw_menu_item':
            result[label].append(dict(token=int(fields[1]),index=int(fields[2]),flags=int(fields[3]),command=int(fields[4]),parameter=int(fields[5]),visible=fields[6]=='1',label=fields[7]))
    return result


def main():
    """같은 실제 원본 자금/기술로 메뉴 입력만 실행하고 보호 설정/전체 관측을 보존한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    output=args.output.resolve()
    if output.exists() and any(output.iterdir()):raise RuntimeError('새 관측 경로가 필요합니다: '+str(output))
    output.mkdir(parents=True,exist_ok=True)
    steps=[(5,'click','Play Mission'),(7,'right','world:priest'),(30,'report','opened'),(30,'snapshot',str(output/'opened.bmp')),
           (35,'click','Construct'),(60,'report','construct'),(65,'click','Workshop'),(90,'report','workshop'),(90,'snapshot',str(output/'workshop.bmp')),
           (95,'click',' Build Sun Workshop for 800'),(100,'report','taken'),(100,'snapshot',str(output/'taken.bmp')),
           (105,'esc',''),(110,'report','cancelled'),(115,'right','world:priest'),(140,'report','reopened'),
           (145,'esc',''),(148,'report','closed'),(148,'snapshot',str(output/'closed.bmp'))]
    script=output/'steps.tsv';report=output/'states.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in steps),encoding='utf-8',newline='\n')
    command=[str(EXE),'--run',str(ROOT/'originals'),'--mission','thewarbegins','--window','--no-audio','--frames','151',
             '--set','SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8','--ui-script',str(script),'--ui-report',str(report)]
    before=snapshots(ROOT/'originals')
    with preserve_game_settings(ROOT/'originals','d'):
        startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
        result=subprocess.run(command,cwd=ROOT,capture_output=True,startupinfo=startup,timeout=40)
        (output/'stdout.log').write_bytes(result.stdout);(output/'stderr.log').write_bytes(result.stderr)
    unchanged=snapshots(ROOT/'originals')==before
    assert result.returncode==0 and unchanged,dict(exitCode=result.returncode,stderr=result.stderr.decode('utf-8',errors='replace'),originals_unchanged=unchanged)
    observed=states(report);items=menu_items(report)
    #모든 실제 관측은 사용자 미션 상태이며 메뉴를 열어도 시계를 정지하지 않는다.
    for label,current in observed.items():
        assert current['phase']==['4'] and current['paused']==['0'] and current['popup']==[''],(label,current['phase'],current['paused'])
        assert current['mission_start'][:3]==['1','1','0'] and current['raw_sp'][1]=='3000',(label,current['raw_sp'])
        assert current['raw_world'][6:9]==['0','0','0'],(label,current['raw_world'])
    assert float(observed['closed']['time'][0])>float(observed['opened']['time'][0])
    assert observed['opened']['raw_menus'][0]==observed['reopened']['raw_menus'][0]=='5'
    build_label=' Build Sun Workshop for 800' #원본 config.english/TYPE85 설명과 실제 원본 비용800이다.
    assert any(row['visible'] and row['label']=='Construct' for row in items['opened'])
    assert all(not row['visible'] for row in items['opened'] if row['label']==build_label)
    assert any(row['visible'] and row['label']=='Workshop' for row in items['construct'])
    build=next(row for row in items['workshop'] if row['visible'] and row['label']==build_label)
    assert not (build['flags']&0x200) and build['command']==0x02000060 and build['parameter']==((85<<16)|15000),build
    taken=observed['taken']['raw_cursor'];assert taken[0]=='0' and taken[2]=='85' and int(taken[3])!=0 and taken[7]=='352',taken
    cancelled=observed['cancelled']['raw_cursor'];assert cancelled[0]=='1' and cancelled[3]=='0',cancelled
    assert any(row['visible'] and row['label']=='Construct' for row in items['reopened'])
    assert all(not row['visible'] for row in items['closed'])
    summaries={label:{key:current[key] for key in ('time','raw_menus','raw_cursor','raw_input','raw_sp')} for label,current in observed.items()}
    summary=dict(passed=True,scope='실제 사제 메뉴/원본 정수명령/TYPE85 커서취득·취소·재개방. 실제배치 클릭은 원본 비용표시Gump 미연결로 별도 후속이며 이 검사의 성공에 합산하지 않는다.',command=command,originals_unchanged=unchanged,build=build,observed=summaries,items=items)
    (output/'results.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('PASS actual priest menu, Build85/0x160, cancel/reopen/close, game clock, unchanged originals')


if __name__=='__main__':main()
