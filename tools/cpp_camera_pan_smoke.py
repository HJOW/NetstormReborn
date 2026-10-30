#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""별도 배포본 실제 창에서 검사 폴링의 Alt/가운데 이동·해제·큐 소비를 확인한다."""
import argparse
import zipfile
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT,execute,save,sha
from cpp_world_smoke import states
from cpp_movie_runtime_smoke import verify_presentation
from nscfg import encode_text

SIZES=((1024,768,0),(1280,720,1),(1280,800,0))  # 세 화면비와 창 밖 이동 옵션의 두 값을 순차 확인한다.


def run(product,output,width,height,outside):
    """같은 일반 입력 큐/폴링에 포인터와 F2만 공급하고 결과 상태를 관측한다."""
    output.mkdir(parents=True,exist_ok=False);binary=product/'Netstorm.exe';data=product/'game'
    (data/'d/options.cfg').write_bytes(encode_text('\r\nintro=1\r\nWelcomeGiven=1\r\ncurrentLanguage=english\r\ntellTips=0\r\nautoDemo=0\r\n'))
    released=f'{0x80010000},{width//2},{height//2}'  # 실제 입력을 뗀 같은 포인터 사건이며 상태/카메라 직접 쓰기가 아니다.
    steps=[(5,'report','briefing'),(10,'click','Play Mission'),(18,'pointer',released),(20,'report','baseline'),
        (25,'pointer',f'{0x120000},{width*3//4},{height//2}'),(26,'report','alt1'),(28,'key','f2'),(36,'report','alt2'),
        (40,'pointer',released),(41,'report','released'),(48,'report','idle'),
        (50,'pointer',f'{0x40000},{width//8},{height//2}'),(51,'report','middle1'),(53,'key','f2'),(61,'report','middle2'),
        (65,'pointer',released),(66,'report','middle-stop'),(68,'key','f2'),(69,'report','key-works'),(72,'key','f2'),(73,'report','key-restored'),
        (76,'pointer',f'{0x4120000},{width+40},{height//2}'),(77,'report','outside1'),(85,'report','outside2'),(89,'pointer',released),
        (90,'report','outside-stop'),(92,'key','enter'),(96,'report','chat-shown'),(98,'report','final'),(98,'snapshot',str(output/'final.bmp')),(98,'snapshot-output',str(output/'final-output.bmp'))]
    plan=output/'steps.tsv';plan.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in steps),encoding='utf-8',newline='\n')
    command=[str(binary),'--run',str(data),'--mission','TheWarBegins','--window','--no-audio','--frames','100','--set',f'SCREENW={width};SCREENH={height};maxFPS=40;sleepPerLoop=1;edgeScroll=0;scrollWhileMouseOut={outside};autoDemo=0;tellTips=0','--ui-script',str(plan),'--ui-report',str(output/'states.tsv')]
    result=execute(command,output,binary,120);save(output/'run.json',result);assert result['returnCode']==0 and not result['timedOut']
    observed=states(output/'states.tsv');frames={label:list(map(int,scene['raw_camera_frame'][:7])) for label,scene in observed.items()}
    for label in ('baseline','alt1','alt2','released','idle','middle1','middle2','key-works','key-restored','outside1','outside2','final'):  # 상태/소유자를 입력 없이 읽고 같은 짧은 단독 미션이 살아 있는지 확인한다.
        scene=observed[label];assert scene['phase']==['4'] and scene['paused']==['0'] and scene['raw_session']==['1','1'];assert scene['mouse_cursor'][1]=='1'
    assert frames['alt2'][0]>frames['alt1'][0],frames
    assert frames['middle2'][0]<frames['middle1'][0],frames
    assert frames['released'][:4]==frames['idle'][:4],frames
    assert (frames['alt2'][5]-frames['alt1'][5])&0xffffffff==10,frames
    assert (frames['middle2'][5]-frames['middle1'][5])&0xffffffff==10,frames
    outside_requests=(frames['outside2'][5]-frames['outside1'][5])&0xffffffff
    assert outside_requests==(8 if outside else 0),frames
    if outside: assert frames['outside2'][0]>frames['outside1'][0],frames
    else: assert frames['outside2'][:4]==frames['outside1'][:4],frames
    hide={name:int(scene['raw_view_keys'][0]) for name,scene in observed.items()}
    assert hide['alt2']==hide['baseline']==hide['middle2'] and hide['key-works']==int(not hide['baseline']) and hide['key-restored']==hide['baseline'],hide
    final=observed['final'];presentation=verify_presentation(output/'final.bmp',output/'final-output.bmp',final,output);assert presentation['same']
    delta=float(final['time'][0])-float(observed['baseline']['time'][0]);assert delta>0
    summary={'passed':True,'size':[width,height],'scrollWhileMouseOut':outside,'cameraFrames':frames,'hideBuildings':hide,'panQueueDropsOrdinaryF2':True,'outsideRequests':outside_requests,'gameSecondsDelta':delta,'presentation':presentation,'run':result,'scope':'실제 게임 창/일반 큐와 명시 검사 폴링 입력이다. 물리 키 누름/원본 화면 대조·자연완주·전체화면/PCM/MP·공격/포획/희생/승리를 재현하지 않는다.'};save(output/'summary.json',summary);return summary


def main():
    """배포 ZIP을 새 경로에만 풀고 원본 자산과 대조한 뒤 세 창을 순차 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--package',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    with zipfile.ZipFile(args.package) as archive:
        archive.extractall(output/'한글 배포본');entries=archive.infolist();assets=0
        for entry in entries:  # 실행 전에 기존 모든 ZIP/추출본/원천 자산 바이트를 확인한다.
            assert not entry.is_dir();content=archive.read(entry);assert content==(output/'한글 배포본'/entry.filename).read_bytes();prefix='NetstormReborn-cpppj/game/'
            if entry.filename.startswith(prefix):
                relative=entry.filename[len(prefix):];source=ROOT/('originalCD' if relative.startswith('MOVIE/') else 'originals')/relative;assert source.read_bytes()==content;assets+=1
    assert len(entries)==1376 and assets==1358;product=output/'한글 배포본/NetstormReborn-cpppj';assert sha(product/'Netstorm.exe')==sha(ROOT/'cpppj/build/bin/Release/Netstorm.exe')
    save(output/'package-audit.json',{'entries':len(entries),'gameAssets':assets,'zipSha256':sha(args.package),'gameSha256':sha(product/'Netstorm.exe'),'allEntryBytesMatch':True,'allGameAssetsMatchSources':True})
    results=[]
    for width,height,outside in SIZES: results.append(run(product,output/f'{width}x{height}',width,height,outside))  # 현재 게임 프로세스가 정상 종료한 뒤 다음 화면비를 실행한다.
    save(output/'summary.json',{'passed':True,'binarySha256':sha(product/'Netstorm.exe'),'results':results,'protectedActionsReproduced':False,'networkExecuted':False});print('카메라 pan 실제창/검사폴링 3회 통과')


if __name__=='__main__':
    main()
