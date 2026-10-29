"""실제 단독 Console의 일반/행동/서식/채널 메시지를 같은 입력 큐와 공유 로그에서 확인한다."""
import argparse
import json
from pathlib import Path
from cpp_bridge_ritual_runtime import GAME, execute, save, sha
from cpp_world_smoke import states
from cpp_production_panel_smoke import repeated


def plain_lines(report,label):
    """실제 스타일 조각의 Text 바이트만 줄별로 이어 읽으며 본문 기대값을 만들지는 않는다."""
    rows=repeated(report,'raw_chat_fragment').get(label,[]);lines={}
    for row in rows:  # 실제 공유 원문 줄의 조각 순서를 유지한다.
        if row[2]!='0':continue
        lines.setdefault(int(row[0]),bytearray()).extend(bytes.fromhex(row[3]).split(b'\0',1)[0])
    return {str(index):bytes(text).decode('cp1252') for index,text in lines.items()}


def main():
    """사용자가 지정한 자료 사본만 실행하고 모든 원시 입력/실행/보고·판정을 새 폴더에 남긴다."""
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True);parser.add_argument('--game-dir',type=Path,required=True);parser.add_argument('--binary',type=Path,default=GAME);args=parser.parse_args();binary=args.binary.resolve()
    directory=args.output.resolve();directory.mkdir(parents=True,exist_ok=False);data=args.game_dir.resolve()
    if data.name.lower() in ('originals','originalcd'):raise ValueError('실제 채팅 검사는 별도 게임 자료 사본을 지정해야 합니다')
    steps=[(5,'report','initial'),(10,'click','Go!'),(20,'report','ready')]
    condition=r'raw_chat_window\t\d+\t\d+\t\d+\t\d+\t[0-9]*[02468]\t'  # 현재 Console visible bit0이 꺼졌을 때만 Enter로 연다.
    commands=[(25,'hello','plain'),(60,'!waves','action'),(100,'.front ~g','format'),(140,'styled','styled'),(180,'EVERYONE: team','channel')]
    for begin,command,label in commands:  # 서로 다른 실제 제출을 같은 세션과 같은 수신자/스타일 상태에서 수행한다.
        steps.append((begin,'when',condition+';;key;;enter'))
        for index,letter in enumerate(command):steps.append((begin+5+index,'key',letter))  # 붙여넣기나 직접 로그 쓰기 없이 실제 문자 큐를 통과한다.
        typed=begin+6+len(command);steps.extend(((typed,'report',label+'-typed'),(typed+2,'key','enter'),(begin+31,'report',label),(begin+31,'snapshot',str(directory/(label+'.bmp')))))
    steps.append((215,'report','final'));plan=directory/'steps.tsv';plan.write_text(''.join(f'{frame}\t{op}\t{arg}\n' for frame,op,arg in sorted(steps,key=lambda row:row[0])),encoding='utf-8',newline='\n')
    command=[str(binary),'--run',str(data),'--mission','TEST02','--window','--no-audio','--frames','217','--set','SCREENW=1024;SCREENH=768;maxFPS=60;sleepPerLoop=8;autoDemo=0;tellTips=0','--ui-script',str(plan),'--ui-report',str(directory/'states.tsv')]
    run=execute(command,directory,binary,75);save(directory/'run.json',run);assert run['returnCode']==0 and not run['timedOut'],run
    report=directory/'states.tsv';observed=states(report);logs=repeated(report,'raw_chat_log');inputs=repeated(report,'raw_chat_input');fragments=repeated(report,'raw_chat_fragment');details=[]
    for begin,command,label in commands:  # 동일 최종 제품의 실제 입력과 수신 로그를 따로 판정한다.
        typed=bytes.fromhex(inputs[label+'-typed'][0][1]).decode('cp1252');assert typed==command,(label,typed,command)
        assert observed[label]['phase']==['4'] and observed[label]['paused']==['0'];actual=plain_lines(report,label);details.append({'label':label,'input':typed,'log':logs[label],'lines':actual,'fragments':fragments.get(label,[])})
        expected={'plain':1,'action':2,'format':2,'styled':3,'channel':4}[label];assert int(logs[label][0][0])==expected,(label,logs[label],actual)
        if label!='format':assert any({'plain':'hello','action':'waves','styled':'styled','channel':'team'}[label] in text for text in actual.values()),(label,actual)
    final_lines=plain_lines(report,'final');assert any('> hello' in text for text in final_lines.values()) and any(' waves' in text for text in final_lines.values())
    chat_packets=observed['final'].get('raw_packets',[]).count('65');assert chat_packets==4,('실제 Session 큐의41h 수신 기록',chat_packets)
    plain_colors={row[6] for row in fragments['plain'] if row[2]=='0' and b'hello' in bytes.fromhex(row[3])};styled_colors={row[6] for row in fragments['styled'] if row[2]=='0' and b'styled' in bytes.fromhex(row[3])}
    assert plain_colors and styled_colors and plain_colors!=styled_colors,('새 설정 사본의 실제 front 스타일 색 변화',plain_colors,styled_colors)
    save(directory/'summary.json',{'passed':True,'gameSha256':sha(binary),'run':run,'commands':details,'finalLines':final_lines,'actualChatPackets':chat_packets,'plainColors':sorted(plain_colors),'styledColors':sorted(styled_colors),'networkExecuted':False,'capturesSacrificesSpecificEnemyAttacksOrVictoriesReproduced':False})
    print(json.dumps({'passed':True,'seconds':run['seconds'],'messages':4,'finalLines':final_lines},ensure_ascii=False))


if __name__=='__main__':main()  # import만으로 게임/파일을 실행·변경하지 않는다.
