#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""정상 UI 입력 뒤 부분 출력과 다음 명시 전체 새로고침의 픽셀을 대조한다."""
import argparse,json
from pathlib import Path
from PIL import Image,ImageChops
from cpp_bridge_ritual_runtime import execute,save,sha
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from cpp_movie_runtime_smoke import verify_presentation
from nscfg import encode_text


def run(product,out):
    """정적인 메뉴 배경/일반 입력으로 갱신 누락과 실제 창 출력을 관측한다."""
    out.mkdir(parents=True,exist_ok=False);binary=product/'Netstorm.exe';data=product/'game'
    steps=[(3,'click-if','OK'),(5,'click-if','OK'),(7,'click-if','Main Menu'),(16,'up','Options')]
    checks=(('hover',20),('options',38),('closed-options',54),('help',70),('closed-help',86))  # 각 상태는 실제 입력 후 안정된 같은 UI다.
    steps.extend(((30,'click','Options'),(48,'esc',''),(64,'click','Help'),(80,'esc','')))
    for label,frame in checks:  # 정상 부분 출력과 명시 refresh의 다음 프레임을 구분해 저장한다.
        steps.extend(((frame,'report',label),(frame,'snapshot',str(out/(label+'.bmp'))),(frame,'snapshot-output',str(out/(label+'-output.bmp'))),(frame,'refresh',''),(frame+1,'report',label+'-full'),(frame+1,'snapshot',str(out/(label+'-full.bmp')))))
    steps.sort(key=lambda item:item[0]);plan=out/'steps.tsv';plan.write_text(''.join(f'{frame}\t{op}\t{arg}\n' for frame,op,arg in steps),encoding='utf-8',newline='\n')
    command=[str(binary),'--run',str(data),'--window','--no-audio','--frames','90','--set','SCREENW=1024;SCREENH=768;windowScreenFlags=130;maxFPS=40;sleepPerLoop=1;autoDemo=0;tellTips=0;edgeScroll=0;scrollWhileMouseOut=0','--ui-script',str(plan),'--ui-report',str(out/'states.tsv')]
    with preserve_game_settings(data,'d',out/'settings-backup'):
        (data/'d/options.cfg').write_bytes(encode_text('\r\nintro=1\r\nWelcomeGiven=1\r\ncurrentLanguage=english\r\ntellTips=0\r\nautoDemo=0\r\n'))
        result=execute(command,out,binary,180)
    save(out/'run.json',result);assert result['returnCode']==0 and not result['timedOut'],result
    observed=states(out/'states.tsv');comparisons={}
    for label,_ in checks:  # 구름이 없는 정적 메뉴에서 전체 새로고침 전후의 모든 RGB 픽셀을 검사한다.
        before=Image.open(out/(label+'.bmp')).convert('RGB');after=Image.open(out/(label+'-full.bmp')).convert('RGB');difference=ImageChops.difference(before,after);box=difference.getbbox()
        present=verify_presentation(out/(label+'.bmp'),out/(label+'-output.bmp'),observed[label],out)
        comparisons[label]={'same':box is None,'differenceBox':box,'presentation':present,'uiDirty':observed[label]['raw_ui_dirty'],'rendererArea':observed[label]['raw_renderer_dirty_area']}
        save(out/(label+'-comparison.json'),comparisons[label])
        if box: difference.save(out/(label+'-difference.png'))
    last=observed['closed-help-full'];ui=list(map(int,last['raw_ui_dirty']));frame=list(map(int,last['raw_renderer_frame']));area=list(map(int,last['raw_renderer_dirty_area']))
    assert len(ui)==3 and ui[0]>0 and ui[1]>0 and ui[2]>0,(ui,frame,area)
    assert frame[1]>area[0] and area[1]<frame[1]*1024*768,(frame,area)
    summary={'passed':all(item['same'] and item['presentation']['same'] for item in comparisons.values()),'comparisons':comparisons,'run':result,'binarySha256':sha(binary),'partialRectangles':frame[1]-area[0],'collectedRectangles':frame[1],'pixels':area[1],'uiDirty':ui,'scope':'단일 고정 제품의 정상 hover/Options/Help/닫기와 다음 명시 refresh 간 정적 메뉴 전체RGB/실제 창 비교다. UI 상태를 주입하지 않으며 게임/MP/포획/희생/공격/승리 검사는 아니다.'}
    save(out/'summary.json',summary);assert summary['passed'],comparisons;return summary


def main():
    """이미 분리한 배포 사본만 실행하고 성공/실패 원시 결과를 새 폴더에 남긴다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--product',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    result=run(args.product.resolve(),args.output.resolve());print(json.dumps({'passed':result['passed'],'partialRectangles':result['partialRectangles'],'collectedRectangles':result['collectedRectangles']},ensure_ascii=False),flush=True)


if __name__=='__main__':  # 명시 요청에서만 실제 클론 창을 실행한다.
    main()
