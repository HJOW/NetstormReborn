"""실제 싱글플레이 Chat HUD와자기 사제의Player메뉴를열고현재대상조회/Native메인복귀를확인한다."""
import argparse
import shutil
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT, GAME, save, sha
from cpp_game_menu_smoke import protected_run, menu_items
from cpp_production_panel_smoke import repeated
from cpp_world_smoke import states

SIZES={'1024x768':(1024,768,False),'1280x720':(1280,720,True)}  #실제창4:3/호환전체화면16:9를순서대로검사한다.


def one(source,output,width,height,fullscreen):
    """명시작업사본의안내버튼으로Chat을열고실제우클릭의Player하위생성까지진행한다."""
    output.mkdir(parents=True,exist_ok=False);data=output/'data';shutil.copytree(source,data)
    english=data/'d/TEST02.english';before=english.read_bytes();needle=b'$Button=Go!,DoNothing,0';assert before.count(needle)==1
    english.write_bytes(before.replace(needle,b'$Button=Toggle Bottom Window,ToggleBottomGump,2\r\n'+needle));save(output/'scenario-variant.json',{'sourceEnglishSha256':sha(source/'d/TEST02.english'),'englishSha256':sha(english),'sourceFortSha256':sha(source/'d/TEST02.fort'),'fortSha256':sha(data/'d/TEST02.fort'),'onlyChange':'안내Go앞의원문ToggleBottomGump2버튼한줄','testDataScope':'기존500000SP/기술을가진UI전용사본.기본게임규칙/AI/시나리오완주검증이아님'})
    steps=[(5,'report','initial'),(10,'click','Toggle Bottom Window'),(20,'report','shown'),(20,'snapshot',str(output/'shown.bmp')),(25,'right','world:priest'),(35,'report','ownMenu'),(35,'snapshot',str(output/'own-menu.bmp')),(45,'outside',''),(55,'esc',''),(60,'click','Game'),(65,'click','Leave Mission'),(72,'report','leave'),(75,'click','Main Menu'),(90,'report','main'),(90,'snapshot',str(output/'main.bmp')),(95,'click','Quit')]
    report,run=protected_run(output/'run',width,height,steps,105,data_root=data,mission='TEST02',fullscreen=fullscreen)
    observed=states(report);rows=repeated(report,'raw_hud_divider');headings=repeated(report,'raw_menu_heading');own=menu_items(report,'ownMenu')
    assert run['returnCode']==0,run
    for label in ('shown','ownMenu'):  #같은실제월드/HUD와비정지진행만확인한다.
        state=observed[label];assert state['phase']==['4'] and state['raw_session']==['1','1'] and state['raw_world_modes']==['0','1'];assert state['paused']==['0'] and state['size']==[str(width),str(height)];assert bool(int(state['screen_mode'][0])&1)==fullscreen
        divider=[row for row in rows[label] if int(row[6])&2];assert len(divider)==1 and float(divider[0][3])==height-96
    assert any(row['label']=='Player' for row in own),own
    assert any('Player ' in row[-1] for row in headings['ownMenu']),headings['ownMenu']  #실제RawObjectMenu가만든Player하위제목은42f640/현재Chat조회까지정상복귀한증거다.
    assert not any('CHEAT' in row['label'].upper() and row['visible'] for row in own)
    assert observed['leave']['page'][0].casefold()=='abort' and observed['main']['phase']==['0'] and observed['main']['state_hud'][0]=='0'
    result={'size':[width,height],'fullscreen':fullscreen,'actualOwnPlayerMenu':True,'actualCurrentChatQueried':True,'inputPrefixScope':'입력줄표시/typing은별도후속.이번GUI는원문기본빈입력이다.','nativeMainMenuReturned':True,'run':run};save(output/'summary.json',result);return result


def main():
    """원본/CD직접실행은거부하고지정한새출력폴더에서두화면비를순차실행한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--data-root',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();source=args.data_root.resolve()
    assert source.is_dir() and all(source!=p.resolve() and p.resolve() not in source.parents for p in (ROOT/'originals',ROOT/'originalCD'));assert not args.output.exists();args.output.mkdir(parents=True)
    results=[]
    for label,(width,height,fullscreen) in SIZES.items():results.append(one(source,args.output/label,width,height,fullscreen))  #한실행과자식이종료한뒤에만다음을시작한다.
    save(args.output/'summary.json',{'binarySha256':sha(GAME),'runs':results,'capturesSacrificesSpecificEnemyAttacksOrVictoriesReproduced':False,'networkExecuted':False})


if __name__=='__main__':main()
