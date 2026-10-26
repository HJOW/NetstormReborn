# 실제 Report 좌표를 적용하여 골렘의 자연 채취/귀환과 다음 단계 입력을 관찰한다.
import hashlib,json,runpy,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent  # 보호 원본에는 읽기만 하며 기존 실패 원시를 덮지 않는다.
sys.path.insert(0,str(ROOT/'tools'))
from cpp_bridge_ritual_runtime import execute,GAME
from cpp_smoke_files import preserve_game_settings
from cpp_renderer_smoke import snapshots
from cpp_world_smoke import states

def main():
    # 최초 두 시도의 입력 생성만 재사용하고 정정한 좌표/새 제품을 별도 관측으로 기록한다.
    out=ROOT/'extracted/cpp-tutorial3-468-attempt4';out.mkdir();data=ROOT/'extracted/cpp-tutorial3-468-data'
    previous=runpy.run_path(str(ROOT/'extracted/cpp-tutorial3-468-attempt2/driver-source.py'));rows,_=previous['prepare_two'](out)
    rows=[(frame,operation,argument.replace('103,106','106,103')) for frame,operation,argument in rows]  # 실제 Geyser106,103이며 정적 Fort 레코드의 반대 순서와 구별한다.
    # ?? ? ?? ?? ??? ????? ?? ??? ?? ??/????? ????.
    rows=[(frame+(360 if frame>550 else 0),operation,argument) for frame,operation,argument in rows]
    for frame in range(560,841,40):
        rows.append((frame,'when',r'page\tmission\n&&mission_gate\t3\t65\t;;click;;production:82'))
        rows.append((frame+10,'when',r'page\tmission\n&&mission_gate\t3\t65\t&&raw_cursor\t0\t[0-9]+\t82\t[1-9][0-9]*\t;;click;;placement-reach:106,103'))
    rows.extend(((900,'report','connected'),(900,'snapshot',str(out/'connected.bmp'))))
    mission=r'page\tmission\n'
    for frame in range(2400,3801,100):
        for offset,page,label in ((0,'B.','MORE'),(5,'B1.','OK'),(10,'C.','OK'),(15,'D.','MORE'),(20,'D1.','OK'),(25,'E.','MORE'),(30,'E1.','OK'),(35,'F.','OK')):
            rows.append((frame+offset,'when','page\\t'+page.replace('.','\\.')+'\\n;;click;;'+label))
        rows.append((frame+45,'when',mission+r'&&mission_gate\t3\t66\t;;click;;production:82'))
        rows.append((frame+55,'when',mission+r'&&mission_gate\t3\t66\t&&raw_cursor\t0\t[0-9]+\t82\t[1-9][0-9]*\t;;click;;placement-reach:130,129'))
        rows.append((frame+60,'when',mission+r'&&mission_gate\t3\t68\t;;right;;world:sunFactory'))
        rows.append((frame+70,'when',mission+r'&&mission_gate\t3\t68\t;;click;;Put Knowledge into Production'))
        rows.append((frame+80,'when',mission+r'&&mission_gate\t3\t68\t;;click;;Wind Generator'))
    for frame in range(2500,4001,250):
        rows.append((frame,'report','f'+str(frame)))
        if frame%500==0: rows.append((frame,'snapshot',str(out/('f'+str(frame)+'.bmp'))))
    rows.sort(key=lambda row:row[0]);script=out/'steps.tsv';script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in rows),encoding='utf-8',newline='\n')
    identity=hashlib.sha256(GAME.read_bytes()).hexdigest();command=[str(GAME),'--run',str(data),'--mission','Tutorial3','--window','--no-audio','--frames','3404','--set','SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;useProductTimers=1','--ui-script',str(script),'--ui-report',str(out/'states.tsv')]
    original=snapshots(ROOT/'originals');before=snapshots(data);profile=dict(command=command,productSource='468 Source1 provisional catalog; mission core from467 Source6',gameSha256=identity,originalGameExecuted=False,testStateInjection=False,testSpHpOwnerInjection=False,testBridgePatterns=False,initialProgressInjected=False,correctedGeyser=[106,103],additionalBridgeClicks=8);(out/'prepared.json').write_text(json.dumps(profile,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    with preserve_game_settings(data,'d',out/'settings-backup'): result=execute(command,out,GAME,480)
    result.update(profile=profile,originalsUnchanged=snapshots(ROOT/'originals')==original,dataUnchanged=snapshots(data)==before,binaryUnchanged=hashlib.sha256(GAME.read_bytes()).hexdigest()==identity)
    if (out/'states.tsv').exists():
        observed=states(out/'states.tsv');result['observations']={tag:{key:value for key,value in state.items() if key in ('mission','page','mission_gate','raw_input','raw_cursor','raw_sp','mission_progress')} for tag,state in observed.items()}
    (out/'execution.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('TUTORIAL3',result['returnCode'],result['timedOut'],flush=True)
    if result['returnCode'] or result['timedOut'] or not all(result[key] for key in ('originalsUnchanged','dataUnchanged','binaryUnchanged')): raise RuntimeError('자연 Tutorial3 실행/보호 실패. 원시 보존')

if __name__=='__main__': main()  # 완료값이나 운반 상태를 쓰지 않는다.
