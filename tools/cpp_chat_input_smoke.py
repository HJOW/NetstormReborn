"""실제 게임창에서 문자 Enter·타이핑·Backspace·F3 숨김과 재표시를 확인한다."""
import argparse
import shutil
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT,GAME,save,sha
from cpp_game_menu_smoke import protected_run
from cpp_production_panel_smoke import repeated
from cpp_world_smoke import states

SIZES={'1024x768':(1024,768,False),'1280x720':(1280,720,True)}  #창4:3과호환전체화면16:9의실제입력을순차실행한다.


def one(source,output,width,height,fullscreen):
    """원문 시나리오를 바꾸지 않은 작업사본으로 같은 입력 큐와 Console을 사용한다."""
    output.mkdir(parents=True,exist_ok=False);data=output/'data';shutil.copytree(source,data)
    text='EVERYONE: hello'  #송신하지 않고 편집/표시만 검증하는 실제 문자 BYTE다.
    steps=[(5,'report','initial'),(10,'click','Go!'),(20,'report','closed'),(25,'keyup','enter'),(30,'report','release'),(35,'key','enter'),(40,'report','shown'),(40,'snapshot',str(output/'shown.bmp'))]
    steps.extend((45+index,'key',byte) for index,byte in enumerate(text))  #각 일반 문자 사건을 별도 실제 프레임에 넣는다.
    steps.extend([(65,'report','typed'),(65,'snapshot',str(output/'typed.bmp')),(70,'key','backspace'),(75,'report','erased'),(80,'key','o'),(85,'key','f3'),(90,'report','hidden'),(95,'key','enter'),(100,'report','reshown'),(100,'snapshot',str(output/'reshown.bmp')),(105,'key','f3'),(110,'esc',''),(115,'click','Game'),(120,'click','Leave Mission'),(128,'report','leave'),(135,'click','Main Menu'),(145,'report','main'),(150,'click','Quit')])
    report,run=protected_run(output/'run',width,height,steps,160,data_root=data,mission='TEST02',fullscreen=fullscreen)
    state=states(report);windows=repeated(report,'raw_chat_window');inputs=repeated(report,'raw_chat_input');globals_=repeated(report,'raw_chat_globals')
    for label,visible,expected in (('closed',False,''),('release',False,''),('shown',True,''),('typed',True,text),('erased',True,text[:-1]),('hidden',False,text),('reshown',True,text)):  #같은현재root/Consoleflags와본문만관측한다.
        rows=windows[label];assert len(rows)==1,(label,rows);assert bool(int(rows[0][4])&1)==visible,(label,rows)
        assert int(globals_[label][0][0])==int(rows[0][0]);assert globals_[label][0][1]=='0'
        assert len(inputs[label])==1 and bytes.fromhex(inputs[label][0][1]).decode('ascii')==expected,(label,inputs[label])
        assert state[label]['phase']==['4'] and state[label]['paused']==['0'];assert state[label]['size']==[str(width),str(height)]
    assert run['returnCode']==0 and state['leave']['page'][0].casefold()=='abort' and state['main']['phase']==['0']
    result={'size':[width,height],'fullscreen':fullscreen,'characterEnterOpenedActualConsole':True,'keyReleaseDidNotOpen':True,'typedBytes':text.encode().hex(),'backspaceEditedActualBuffer':True,'f3HidThenEnterRestoredText':True,'nativeMainMenuReturned':True,'run':run};save(output/'summary.json',result);return result


def main():
    """보호 원본 직접실행을 거부하고 새 폴더에서 두 실제 창 실행을 마친다."""
    parser=argparse.ArgumentParser();parser.add_argument('--data-root',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();source=args.data_root.resolve()
    assert source.is_dir() and all(source!=p.resolve() and p.resolve() not in source.parents for p in (ROOT/'originals',ROOT/'originalCD'));assert not args.output.exists();args.output.mkdir(parents=True)
    results=[]
    for label,(width,height,fullscreen) in SIZES.items():results.append(one(source,args.output/label,width,height,fullscreen))  #한 실행이 종료된 후에만 다음 창을 시작한다.
    save(args.output/'summary.json',{'binarySha256':sha(GAME),'runs':results,'capturesSacrificesSpecificEnemyAttacksOrVictoriesReproduced':False,'networkExecuted':False,'sentChat':False})


if __name__=='__main__':main()
