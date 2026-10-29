"""실제게임창의Tab/Shift+Tab·감김·공백/콜론단어경계를같은Console에서검사한다."""
import argparse
import shutil
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT,GAME,save,sha
from cpp_game_menu_smoke import protected_run
from cpp_production_panel_smoke import repeated
from cpp_world_smoke import states

SIZES={'1024x768':(1024,768,False),'1280x800':(1280,800,True)}  #창4:3/호환전체화면16:10을순차실행한다.


def one(source,output,width,height,fullscreen):
    """원문시나리오를수정하지않은사본에서일반키만큐에넣고제출하지않는다."""
    output.mkdir(parents=True,exist_ok=False);data=output/'data';shutil.copytree(source,data)
    steps=[(5,'report','initial'),(10,'click','Go!'),(20,'key','enter'),(25,'key','tab'),(30,'report','first'),(35,'key','tab'),(40,'report','second'),(40,'snapshot',str(output/'second.bmp')),(45,'key','shift+tab'),(50,'report','back'),(55,'key','shift+tab'),(60,'report','reverseWrap'),(65,'key','tab'),(70,'report','forwardWrap'),(75,'key',' '),(76,'key','a'),(77,'key','l'),(80,'key','tab'),(85,'report','spaceWord'),(85,'snapshot',str(output/'space-word.bmp')),(90,'key',':'),(95,'key','tab'),(100,'report','colonWord'),(100,'snapshot',str(output/'colon-word.bmp')),(105,'key','f3'),(110,'report','hidden'),(115,'key','enter'),(120,'report','reshown'),(125,'key','f3'),(130,'esc',''),(135,'click','Game'),(140,'click','Leave Mission'),(148,'report','leave'),(155,'click','Main Menu'),(165,'report','main'),(170,'click','Quit')]
    report,run=protected_run(output/'run',width,height,steps,180,data_root=data,mission='TEST02',fullscreen=fullscreen)
    observed=states(report);inputs=repeated(report,'raw_chat_input');windows=repeated(report,'raw_chat_window');candidates=repeated(report,'raw_chat_completion_candidate');completion=repeated(report,'raw_chat_completion')
    firstCandidates=candidates['first'];assert len(firstCandidates)>=4 and [bytes.fromhex(row[1]).decode('latin1') for row in firstCandidates[:4]]==['BATTLE','EVERYONE','ALLIES','OBSERVER'];last=bytes.fromhex(firstCandidates[-1][1]).decode('latin1')  #같은싱글플레이AI가등록한이름도채널4개뒤에포함되므로실제마지막후보를읽는다.
    assert completion['reverseWrap'][0][0]==str(len(firstCandidates)-1) and completion['forwardWrap'][0][0]=='0'
    for label,text in (('first','BATTLE'),('second','EVERYONE'),('back','BATTLE'),('reverseWrap',last),('forwardWrap','BATTLE'),('spaceWord','BATTLE ALLIES'),('colonWord','BATTLE ALLIES:BATTLE'),('hidden','BATTLE ALLIES:BATTLE'),('reshown','BATTLE ALLIES:BATTLE')):  #직접버퍼설정없이실제편집결과만읽는다.
        assert len(inputs[label])==1 and bytes.fromhex(inputs[label][0][1]).decode('latin1')==text,(label,inputs[label]);assert len(windows[label])==1 and bool(int(windows[label][0][4])&1)==(label!='hidden')
        state=observed[label];assert state['phase']==['4'] and state['paused']==['0'] and state['size']==[str(width),str(height)]
    assert run['returnCode']==0 and observed['leave']['page'][0].casefold()=='abort' and observed['main']['phase']==['0']
    result={'size':[width,height],'fullscreen':fullscreen,'tabAndShiftTabUsedActualConsole':True,'forwardAndReverseWrap':True,'candidateCount':len(firstCandidates),'lastCandidateBytes':firstCandidates[-1][1],'spaceAndColonWordBoundaries':True,'inputPreservedWhenHidden':True,'nativeMainMenuReturned':True,'run':run};save(output/'summary.json',result);return result


def main():
    """보호디렉터리직접실행을거부하고새출력폴더만사용한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--data-root',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();source=args.data_root.resolve()
    assert source.is_dir() and all(source!=p.resolve() and p.resolve() not in source.parents for p in (ROOT/'originals',ROOT/'originalCD'));assert not args.output.exists();args.output.mkdir(parents=True)
    results=[]
    for label,(width,height,fullscreen) in SIZES.items():results.append(one(source,args.output/label,width,height,fullscreen))  #한게임이종료한뒤에만다음을연다.
    save(args.output/'summary.json',{'binarySha256':sha(GAME),'runs':results,'capturesSacrificesSpecificEnemyAttacksOrVictoriesReproduced':False,'networkExecuted':False,'sentChat':False})


if __name__=='__main__':main()
