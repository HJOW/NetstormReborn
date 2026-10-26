# 수정한 실제 다리 연결 뒤 C/D의 지식 생산과 E의 첫 생성기 입력을 별도 관찰한다.
import hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent  # 원본에는 읽기만 하며 이전 실패 원시를 보존한다.
sys.path.insert(0,str(ROOT/'tools'))
from cpp_bridge_ritual_runtime import execute,GAME
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states

def main():
    # 검증된 실제 입력 앞부분을 재사용하고 이전 종료 프레임 이후 사건은 복사하지 않는다.
    out=ROOT/'extracted/cpp-tutorial3-468-attempt5';out.mkdir();data=ROOT/'extracted/cpp-tutorial3-468-data'
    rows=[]  # 실제 입력과 읽기 전용 관측만 기록하며 완료/자원/객체 상태를 쓰지 않는다.
    for line in (ROOT/'extracted/cpp-tutorial3-468-attempt4/steps.tsv').read_text(encoding='utf-8').splitlines():
        frame,operation,argument=line.split('\t',2);frame=int(frame)  # 이미 실행된 앞부분의 실제 시각과 사건이다.
        if frame>2100: continue
        if operation=='snapshot': argument=str(out/Path(argument).name)
        rows.append((frame,operation,argument))
    # 같은 단계가 유지될 때만 실제 메뉴를 열고 선택하여 원문 다음 관문이 반복을 멈추게 한다.
    for frame in range(2150,3101,150):
        rows.extend(((frame,'when',r'page\tmission\n&&mission_gate\t3\t67\t;;right;;world:sunFactory'),(frame+10,'when',r'page\tmission\n&&mission_gate\t3\t67\t;;click;;Put Knowledge into Production'),(frame+20,'when',r'page\tmission\n&&mission_gate\t3\t67\t;;click;;Sun Disc Thrower')))
        rows.extend(((frame+30,'when',r'page\tD\.\n;;click;;MORE'),(frame+40,'when',r'page\tD1\.\n;;click;;OK'),(frame+50,'when',r'page\tmission\n&&mission_gate\t3\t68\t;;right;;world:sunFactory'),(frame+60,'when',r'page\tmission\n&&mission_gate\t3\t68\t;;click;;Put Knowledge into Production'),(frame+70,'when',r'page\tmission\n&&mission_gate\t3\t68\t;;click;;Wind Generator'),(frame+80,'when',r'page\tE\.\n;;click;;MORE'),(frame+90,'when',r'page\tE1\.\n;;click;;OK')))
    rows.extend(((3300,'report','beforeGenerator'),(3301,'snapshot',str(out/'beforeGenerator.bmp')),(3320,'when',r'page\tmission\n&&mission_gate\t3\t69\t;;click;;production:75'),(3330,'when',r'page\tmission\n&&mission_gate\t3\t69\t&&raw_cursor\t0\t[0-9]+\t75\t[1-9][0-9]*\t;;click;;placement-near:127,104'),(3340,'report','afterGenerator'),(3341,'snapshot',str(out/'afterGenerator.bmp')),(3450,'report','final'),(3451,'snapshot',str(out/'final.bmp'))))
    for frame in range(2200,3401,200): rows.append((frame,'report','f'+str(frame)))  # 실제 관문과 생산 덱/좌표를 읽는다.
    rows.sort(key=lambda row:row[0]);assert rows[-1][0]<3460
    script=out/'steps.tsv';script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in rows),encoding='utf-8',newline='\n')
    identity=hashlib.sha256(GAME.read_bytes()).hexdigest();original=snapshots(ROOT/'originals');before=snapshots(data)  # 실행하는 제품과 모든 보호 원본을 고정한다.
    command=[str(GAME),'--run',str(data),'--mission','Tutorial3','--window','--no-audio','--frames','3460','--set','SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;useProductTimers=1','--ui-script',str(script),'--ui-report',str(out/'states.tsv')]
    profile=dict(command=command,gameSha256=identity,originalGameExecuted=False,testStateInjection=False,testSpHpOwnerInjection=False,testBridgePatterns=False,initialProgressInjected=False)
    (out/'prepared.json').write_text(json.dumps(profile,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    with preserve_game_settings(data,'d',out/'settings-backup'): result=execute(command,out,GAME,480)
    observed=states(out/'states.tsv') if (out/'states.tsv').exists() else {}  # 실제 보고만 최종 결과에 요약한다.
    result.update(profile=profile,originalsUnchanged=snapshots(ROOT/'originals')==original,dataUnchanged=snapshots(data)==before,binaryUnchanged=hashlib.sha256(GAME.read_bytes()).hexdigest()==identity,observations={name:{key:value for key,value in state.items() if key in ('page','mission_gate','raw_sp','raw_cursor')} for name,state in observed.items()})
    (out/'execution.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('TUTORIAL3',result['returnCode'],result['timedOut'],flush=True)
    if result['returnCode'] or result['timedOut'] or not all(result[key] for key in ('originalsUnchanged','dataUnchanged','binaryUnchanged')): raise RuntimeError('실제 Tutorial3/보호 실패. 원시 보존')

if __name__=='__main__': main()  # 다음 최종 제품 빌드 뒤 실행하며 이번 작성만으로 통과를 주장하지 않는다.
