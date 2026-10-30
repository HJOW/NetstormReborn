#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""별도 배포본의 실제 구름 Frame/메뉴·브리핑·ESC와 세 화면비 pan 출력을 확인한다."""
import argparse
import zipfile
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT,execute,save,sha
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from cpp_movie_runtime_smoke import verify_presentation
from cpp_camera_pan_presentation_smoke import run as run_pan,SIZES
from nscfg import encode_text


def run_frame(product,out,flags,mission):
    """현재 옵션과 일반 버튼/ESC만 공급하고 같은 장기 구름 전역의 진행/정리를 읽는다."""
    out.mkdir(parents=True,exist_ok=False);binary=product/'Netstorm.exe';data=product/'game'
    steps=[(8,'report','initial'),(8,'snapshot',str(out/'initial.bmp')),(8,'snapshot-output',str(out/'initial-output.bmp'))]
    if not mission: steps=[(3,'click-if','OK'),(5,'click-if','OK'),(7,'click-if','Main Menu')]+steps  # 새 사본의 원문 UserNotUpdated 안내를 실제 표시된 버튼으로 닫는다.
    if mission:
        steps += [(10,'click','Play Mission'),(25,'report','playing'),(45,'esc',''),(50,'report','escape'),(50,'snapshot',str(out/'escape.bmp')),(60,'esc','')]
    steps += [(100,'report','final'),(100,'snapshot',str(out/'final.bmp')),(100,'snapshot-output',str(out/'final-output.bmp'))]
    plan=out/'steps.tsv';plan.write_text(''.join(f'{frame}\t{op}\t{arg}\n' for frame,op,arg in steps),encoding='utf-8',newline='\n')
    command=[str(binary),'--run',str(data),'--no-audio','--frames','103','--set',f'SCREENW=1024;SCREENH=768;windowScreenFlags={flags};workingFullScreenFlags={flags};startInFullScreen=1;maxFPS=40;sleepPerLoop=1;autoDemo=0;tellTips=0;edgeScroll=0;scrollWhileMouseOut=0','--ui-script',str(plan),'--ui-report',str(out/'states.tsv')]
    if not flags&1: command.append('--window')  # 원문 창 모드는 시차16을 지우므로 시차 검사는 실제 전체화면에서만 실행한다.
    if mission: command += ['--mission','TheWarBegins']
    with preserve_game_settings(data,'d',out/'settings-backup'):
        (data/'d/options.cfg').write_bytes(encode_text('\r\nintro=1\r\nWelcomeGiven=1\r\ncurrentLanguage=english\r\ntellTips=0\r\nautoDemo=0\r\n'))
        result=execute(command,out,binary,180)
    save(out/'run.json',result);assert result['returnCode']==0 and not result['timedOut'],result
    observed=states(out/'states.tsv');initial=observed['initial'];final=observed['final']
    frames={label:list(map(int,scene['raw_renderer_frame'])) for label,scene in observed.items()}  # 게임 값을 쓰지 않는 실제 owner/cache 관측이다.
    for label,values in frames.items():  # 준비/정상 Draw 반환·청소와 현재 모드를 같은 프레임에서 비교한다.
        assert values[0]==values[5] and values[0]>0 and values[1]==values[2],(label,values)
        assert values[13]==0,(label,values)  # 부분 표시 플래그만 정상 정리한다. 구름을 끄면 미표시 allDirty는 원문대로 남을 수 있다.
        assert int(observed[label]['screen_mode'][0])==flags|128,(label,observed[label]['screen_mode'])
    assert frames['final'][0]>frames['initial'][0]
    animated=bool(flags&32) and bool(flags&8)
    if animated:
        assert frames['final'][10]!=frames['initial'][10] and frames['final'][3]>0,frames
    if not flags&8: assert frames['final'][3]==0 and frames['final'][4]>0 and frames['final'][12]==1,frames
    if mission:
        assert final['phase']==['4'] and final['paused']==['0'] and final['raw_session']==['1','1'],final.get('phase')
        assert observed['escape']['paused']==['0'] and observed['escape']['game_menu_bar'][1]=='1' and final['game_menu_bar'][1]=='0'  # 원문 ESC 막대는 자동 일시정지를 하지 않는다.
        assert float(final['time'][0])>float(observed['playing']['time'][0])
    else: assert final['phase']==['0'] and final['page']==['main']
    comparisons={}
    for label in ('initial','final'):  # 같은 논리8비트 프레임의 실제 창DC 전체RGB를 대조한다.
        comparisons[label]=verify_presentation(out/f'{label}.bmp',out/f'{label}-output.bmp',observed[label],out)
        save(out/f'{label}-presentation.json',comparisons[label]);assert comparisons[label]['same'],comparisons[label]
    summary={'passed':True,'mission':mission,'flags':flags,'frames':frames,'presentation':comparisons,'run':result}
    save(out/'summary.json',summary);return summary


def main():
    """보호 원천과 다른 한글 경로에 풀고 순차 실제 창/설정 복구·배포 바이트를 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--package',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--resume',action='store_true');args=parser.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=args.resume);product=out/'한글 배포본/NetstormReborn-cpppj'
    with zipfile.ZipFile(args.package) as archive:
        if not args.resume: archive.extractall(out/'한글 배포본')
        entries=archive.infolist();assets=0
        for entry in entries:  # 압축/추출본과 게임 자료 원천을 전수 비교한다.
            relative=entry.filename.removeprefix('NetstormReborn-cpppj/');content=archive.read(entry);assert (product/relative).read_bytes()==content
            if relative.startswith('game/'):
                asset=relative.removeprefix('game/');source=ROOT/('originalCD' if asset.startswith('MOVIE/') else 'originals')/asset
                assert source.read_bytes()==content,asset;assets+=1
        assert sha(product/'Netstorm.exe')==sha(ROOT/'cpppj/build/bin/Release/Netstorm.exe')
        runs=[]
        for name,flags,mission in (('main-clouds',170,False),('main-tint',130,False),('mission-clouds',170,True),('mission-parallax-fullscreen',187,True)):  # 메뉴/날씨 바탕·월드·시차는 같은 제품에서 일반 옵션으로만 선택한다.
            prior=out/name/'summary.json'
            if args.resume and prior.exists():
                import json
                record=json.loads(prior.read_text(encoding='utf-8'));assert record['passed'] and record['flags']==flags and record['mission']==mission and record['run']['binarySha256']==sha(product/'Netstorm.exe')
                runs.append(record)  # 이미 완결된 같은 제품 관측만 재사용하며 실패/부분 실행은 성공으로 세지 않는다.
            else: runs.append(run_frame(product,out/name,flags,mission))
        for width,height in SIZES:  # 각 창을 종료/복구한 뒤 다음 화면비/전체화면을 연다.
            pair=[run_pan(product,out/f'{width}x{height}-{mode}',width,height,mode=='fullscreen') for mode in ('window','fullscreen')]
            assert pair[0]['changes']==pair[1]['changes'];runs.extend(pair)
        for entry in entries:  # 실행 뒤 옵션/저장 파일을 포함한 기존 배포 항목 바이트를 보존한다.
            assert (out/'한글 배포본'/entry.filename).read_bytes()==archive.read(entry),entry.filename
    save(out/'summary.json',{'passed':True,'runs':len(runs),'entries':len(entries),'gameAssets':assets,'binarySha256':sha(product/'Netstorm.exe'),'zipSha256':sha(args.package),'results':runs,'allArchivedBytesUnchanged':True,'scope':'실제 클론 Frame/구름·메뉴/브리핑·ESC·pan과 창DC 출력이다. 자연 완주/포획/희생/특정 공격/승리·공중3종 전용/신규MP·Win11·PCM 검사는 아니다.'})
    print('실제 Frame/구름·메뉴·ESC/시차4회와 세 화면비 pan6회 통과',flush=True)


if __name__=='__main__':
    main()
