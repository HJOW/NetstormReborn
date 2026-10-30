#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""같은 배포본의 실제 단일 표시 목록·창 출력과 QWASZX 다리 입력을 확인한다."""
import argparse
import json
import sys
import zipfile
from pathlib import Path
import cpp_renderer_frame_smoke as frame
import cpp_production_shortcuts_smoke as shortcuts
from cpp_bridge_ritual_runtime import ROOT,execute,save,sha
from cpp_world_smoke import states
from cpp_movie_runtime_smoke import verify_presentation
from cpp_smoke_files import preserve_game_settings
from nscfg import encode_text


def scene_rows(path):
    """실제 보고의 수집/분배 누계를 대조하고 관측된 종류만 반환한다."""
    result={}
    for label,state in states(path).items():  # 같은 실제 dirty 목록을 수집/정렬/분배한 양쪽 누계다.
        row=list(map(int,state['raw_renderer_scene']))
        assert len(row)==11 and 0<=row[0]<=5000 and row[1:6]==row[6:11],(label,row)
        result[label]=row
    assert result,path
    return result


def construction(product,out):
    """기존 Early5 입력의 첫 스톤 타워 건설만 재생하여 실제 종류3도 관측한다."""
    out.mkdir(parents=True,exist_ok=False);steps=[];binary=product/'Netstorm.exe'
    for line in (ROOT/'cpppj/tests/fixtures/tutorial-five-production-ui-input.tsv').read_text(encoding='utf-8').splitlines():  # 첫 배치 뒤 안내/워크샾 생산은 이 표시 검사에서 요청하지 않는다.
        if not line or line.startswith('#'): continue
        frame_number,operation,argument=line.split('\t')
        if int(frame_number)<=140: steps.append((frame_number,operation,argument))
    steps.extend([('200','report','building'),('200','snapshot',str(out/'building.bmp')),('200','snapshot-output',str(out/'building-output.bmp')),('500','report','completed'),('500','snapshot',str(out/'completed.bmp')),('500','snapshot-output',str(out/'completed-output.bmp'))])
    plan=out/'steps.tsv';plan.write_text(''.join('\t'.join(row)+'\n' for row in steps),encoding='utf-8',newline='\n')
    command=[str(binary),'--run',str(product/'game'),'--mission','tutorial5','--window','--no-audio','--frames','503','--set','SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0','--ui-script',str(plan),'--ui-report',str(out/'states.tsv')]
    with preserve_game_settings(product/'game','d',out/'settings-backup'):
        (product/'game/d/options.cfg').write_bytes(encode_text('\r\nintro=1\r\nWelcomeGiven=1\r\ncurrentLanguage=english\r\ntellTips=0\r\nautoDemo=0\r\n'))
        run=execute(command,out,binary,240)
    save(out/'run.json',run);assert run['returnCode']==0 and not run['timedOut'],run
    observed=states(out/'states.tsv');rows=scene_rows(out/'states.tsv')
    before={sid for sid,row in observed['initial']['objects'].items() if row[:2]==['sunBlocker','1']}
    after={sid for sid,row in observed['placed']['objects'].items() if row[:2]==['sunBlocker','1']}
    assert len(after-before)==1 and max(row[4] for row in rows.values())>0,(before,after,rows)
    presentation={}
    for label in ('building','completed'):  # 자연 건설 중/뒤의 실제 현재 창을 각각 확인한다.
        comparison=verify_presentation(out/f'{label}.bmp',out/f'{label}-output.bmp',observed[label],out);assert comparison['same'],comparison;presentation[label]=comparison
    summary={'passed':True,'run':run,'sceneRows':rows,'newTowerSids':sorted(after-before),'presentation':presentation}
    save(out/'summary.json',summary);return summary


def main():
    """기존 Frame/pan 검사와 정상 다리 입력을 같은 한글 배포 사본에서 순차 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--resume',action='store_true')
    parser.add_argument('--bridge-run-name',default='production-keys')
    args=parser.parse_args();out=args.output.resolve();package=args.package.resolve()
    product=out/'한글 배포본/NetstormReborn-cpppj';binary=product/'Netstorm.exe'
    if args.resume and (out/'summary.json').exists():
        prior=json.loads((out/'summary.json').read_text(encoding='utf-8'))
        assert prior['passed'] and prior['runs']==10 and prior['binarySha256']==sha(binary) and prior['zipSha256']==sha(package)
    else:
        sys.argv=['cpp_renderer_frame_smoke','--package',str(package),'--output',str(out)]+(['--resume'] if args.resume else [])
        frame.main()
    names=('main-clouds','main-tint','mission-clouds','mission-parallax-fullscreen')+tuple(f'{width}x{height}-{mode}' for width,height in frame.SIZES for mode in ('window','fullscreen'))
    observations={name+'/states.tsv':scene_rows(out/name/'states.tsv') for name in names}  # 실패/부분 폴더를 완결10회의 관측에 섞지 않는다.
    bridge=out/args.bridge_run_name;shortcuts.GAME=binary

    def capture(command,directory,exe,timeout):
        """기존 입력 계획의 각 논리 캡처 옆에 실제 창DC 캡처만 추가한다."""
        plan=Path(command[command.index('--ui-script')+1]);steps=[]
        for line in plan.read_text(encoding='utf-8').splitlines():  # 원래 입력/프레임은 유지하고 같은 프레임의 표시만 읽는다.
            fields=line.split('\t');steps.append(fields)
            if fields[1]=='snapshot': steps.append([fields[0],'snapshot-output',str(Path(fields[2]).with_name(Path(fields[2]).stem+'-output.bmp'))])
        plan.write_text(''.join('\t'.join(fields)+'\n' for fields in steps),encoding='utf-8',newline='\n')
        return execute(command,directory,exe,max(timeout,240))

    shortcuts.execute=capture
    if args.resume and (bridge/'summary.json').exists():
        prior=json.loads((bridge/'summary.json').read_text(encoding='utf-8'));run=json.loads((bridge/'run.json').read_text(encoding='utf-8'))
        assert prior['passed'] and run['binarySha256']==sha(binary) and run['returnCode']==0 and not run['timedOut']
    else:
        sys.argv=['cpp_production_shortcuts_smoke','--output',str(bridge),'--data-root',str(product/'game')]
        with preserve_game_settings(product/'game','d',out/(args.bridge_run_name+'-settings-backup')):
            (product/'game/d/options.cfg').write_bytes(encode_text('\r\nintro=1\r\nWelcomeGiven=1\r\ncurrentLanguage=english\r\ntellTips=0\r\nautoDemo=0\r\n'))
            shortcuts.main()
    observed=states(bridge/'states.tsv');observations[str((bridge/'states.tsv').relative_to(out))]=scene_rows(bridge/'states.tsv')
    presentation={}
    for label in ('mature','rotated','placed','refilled'):  # 정상 다리 취득/양방향 회전/배치/자연 보충의 실제 창 출력을 확인한다.
        comparison=verify_presentation(bridge/f'{label}.bmp',bridge/f'{label}-output.bmp',observed[label],bridge)
        assert comparison['same'],comparison;presentation[label]=comparison
    workshop=out/'construction-display'
    if args.resume and (workshop/'summary.json').exists():
        built=json.loads((workshop/'summary.json').read_text(encoding='utf-8'));assert built['passed'] and built['run']['binarySha256']==sha(binary)
    else: built=construction(product,workshop)
    observations['construction-display/states.tsv']=built['sceneRows']
    with zipfile.ZipFile(package) as archive:
        for entry in archive.infolist():  # 모든 배포 바이트와 보호 원천은 실행 뒤에도 같다.
            target=out/'한글 배포본'/entry.filename
            assert target.read_bytes()==archive.read(entry),entry.filename
    summary={'passed':True,'frameRuns':10,'bridgeRuns':1,'constructionRuns':1,'binarySha256':sha(binary),'zipSha256':sha(package),'sceneRows':observations,'bridgePresentation':presentation,'construction':built,'allArchivedBytesUnchanged':True,'scope':'같은 단일 목록의 수집/분배 누계와 실제 클론 창/정상 다리 및 스톤 타워 건설 입력이다. 원본 전체 픽셀/자연 완주/포획/희생/특정 적 공격/승리/공중3종 전용/신규MP/Win11/PCM/물리키 검사는 아니다.'}
    save(out/'native-scene-summary.json',summary);print(json.dumps({'passed':True,'runs':12,'observedReports':sum(map(len,observations.values()))},ensure_ascii=False),flush=True)


if __name__=='__main__':  # 명시 실행 요청에서만 실제 클론 창을 연다.
    main()
