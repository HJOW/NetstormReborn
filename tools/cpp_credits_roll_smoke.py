#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 Credits 자료의 두20초 자동 전환 사슬을 실제 메뉴/벽시계로 끝까지 관측한다."""
import argparse
import hashlib
import json
import re
import subprocess
import time
from pathlib import Path
from cpp_menu_smoke import source
from taff import TaffArchive

ROOT=Path(__file__).resolve().parents[1]  # 보호 자료/현재 제품을 찾는 저장소 기준이다.
GAME=ROOT/'cpppj/build/bin/Release/Netstorm.exe'  # 원본 실행파일은 실행하지 않는다.


def digest(path):
    """현재 실제 파일의 전체 SHA256을 읽는다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path,value):
    """이번 실행의 작은 관측/판정을 UTF-8로 보존한다."""
    Path(path).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def catalog(reference):
    """보호 tell 자료에서 실제 메뉴 라벨/다음 섹션/초/배경을 독립적으로 읽는다."""
    archive=TaffArchive(reference/'netstorm.tarc')
    text=source(reference,archive,'d/tell.english').decode('cp1252').replace('\r','')
    matches=list(re.finditer(r'(?m)^\[([^\]\n]+)\]',text));sections={}
    for index,match in enumerate(matches):  # 파일의 실제 섹션 경계를 유지한다.
        sections[match[1]]=text[match.end():matches[index+1].start() if index+1<len(matches) else len(text)]
    menus=re.findall(r'(?m)^\$Menu=([^,\n]+),Tell,([^\n]+)',sections['Credits'])
    result=[]
    for label,first in menus:  # 선택한 원본 두 메뉴를 실제 timeout 사슬로 확장한다.
        current=first.strip();pages=[];seen=set()
        while current!='Blank':  # Blank는 별도 제품 상태를 합성하지 않고 원본 Tell의 끝이다.
            if current in seen: raise RuntimeError('원본 Credits timeout 순환: '+current)
            seen.add(current);body=sections[current]
            timeout=re.search(r'(?m)^\$Timeout=(\d+),Tell,([^\n]+)',body)
            if not timeout: raise RuntimeError('원본 Credits timeout 없음: '+current)
            background=re.search(r'<background="([^"]*)">',body)
            pages.append({'page':current,'seconds':int(timeout[1]),'next':timeout[2].strip(),'background':background[1] if background else None})
            current=timeout[2].strip()
        result.append({'label':label,'pages':pages})
    if len(result)!=2: raise RuntimeError('원본 Credits 분류2개 계약')
    backgrounds={page['background'] for branch in result for page in branch['pages'] if page['background']}
    availability={}
    for background in backgrounds:  # 없는 nsstart를 다른 그림으로 대체하지 않는다.
        try: source(reference,archive,'d/'+background+'.gif');availability[background]=True
        except (FileNotFoundError,StopIteration): availability[background]=False
    return result,availability


def observe(path):
    """큰 화면 보고서에서 현재 페이지/시간/실제 HTML 수명/원문 버튼만 간결하게 읽는다."""
    result=[];record=None
    with Path(path).open(encoding='utf-8') as stream:
        for line in stream:  # 게임 자료나 상태는 쓰지 않고 원시 관측을 순서대로 읽는다.
            fields=line.rstrip('\n').split('\t')
            if fields[0]=='frame':
                record={'frame':int(fields[1]),'label':fields[2],'buttons':[],'htmlRoots':[]};result.append(record)
            elif record is not None and fields[0] in ('page','phase','time','raw_html_globals','html_window_state','fort_selection'):
                record[fields[0]]=fields[1:]
            elif record is not None and fields[0]=='raw_html_button' and fields[-2]=='1': record['buttons'].append(fields[-1])
            elif record is not None and fields[0]=='raw_html_root': record['htmlRoots'].append({'token':int(fields[1]),'flags':int(fields[3])})
    return result


def run(branch,index,data,output,fps):
    """실제 Credits 메뉴 선택 뒤 More/Back 입력 없이 각20초 자연 기한만 기다린다."""
    directory=output/('patch' if index==0 else 'original');directory.mkdir(exist_ok=False)
    total=sum(page['seconds'] for page in branch['pages']);frames=(total+18)*fps
    steps=[(3,'click-if','OK'),(5,'click-if','OK'),(10,'report','main'),(12,'click','Credits'),(14,'report','choices'),(16,'click',branch['label'])]
    for frame in range(20,frames-1):
        steps.append((frame,'report',str(frame)))  # 모든 실제 프레임을 관측하여 Idle의Kill→다음Input 사이 한 프레임도 구별한다.
        ready=r'page\tmain\n&&html_window_state\t0\t0\t0\n'  # 실제 최종 HTML 반납 뒤에만 메인 Quit을 누른다.
        steps.append((frame,'when',ready+';;click;;Quit'))  # Idle의dead 한프레임에는 종료하지 않는다.
    for page_index,page in enumerate(branch['pages']):
        frame=16+(sum(item['seconds'] for item in branch['pages'][:page_index])+5)*fps  # 각 페이지의 예상 중간 지점이며 판정은 실제 관측 페이지를 따른다.
        steps.append((frame,'snapshot',str(directory/f'sample-{page_index}-{frame}.bmp')))  # 파일명은 페이지를 단정하지 않으며 실제 관측 프레임과 함께 기록한다.
    steps.extend([(frames-1,'report','final'),(frames-1,'snapshot',str(directory/'final.bmp'))]);steps.sort(key=lambda item:item[0])
    plan=directory/'steps.tsv';plan.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in steps),encoding='utf-8')
    command=[str(GAME),'--run',str(data),'--window','--no-audio','--frames',str(frames),'--set',f'SCREENW=1024;SCREENH=768;maxFPS={fps};sleepPerLoop={1000//fps};autoDemo=0;tellTips=0','--ui-script',str(plan),'--ui-report',str(directory/'states.tsv')]
    started=time.monotonic();identity=digest(GAME)
    with (directory/'stdout.log').open('wb') as stdout,(directory/'stderr.log').open('wb') as stderr:
        process=subprocess.Popen(command,cwd=directory,stdout=stdout,stderr=stderr)
        save(directory/'running.json',{'pid':process.pid,'command':command,'binarySha256':identity})
        print(json.dumps({'branch':directory.name,'pid':process.pid,'expectedPages':len(branch['pages']),'nativeSeconds':total}),flush=True)
        code=process.wait(timeout=total+180)
    record={'branch':directory.name,'returncode':code,'elapsedSeconds':time.monotonic()-started,'gameSha256':identity,'observations':0,'passed':False}  # 실제 보고 수/판정은 아래 원시 재검증 함수에서 읽는다.
    save(directory/'result.json',record)
    return verify(branch,directory,record)


def verify(branch,directory,record,result_name='result.json'):
    """현재 PE/제품을 재실행하지 않고 같은 원시 프레임 전체를 원문 수명 순서와 대조한다."""
    if record['returncode']: raise RuntimeError('Credits 실제 실행 종료 실패: '+directory.name)
    observations=observe(directory/'states.tsv');record['observations']=len(observations)
    save(directory/('observations-'+result_name),observations)
    continuous=[];transients=[];successors={page['page']:('main' if page['next']=='Blank' else page['next']) for page in branch['pages']}
    for state_index,state in enumerate(observations):  # 원문 Input→Cleanup→Idle→그리기의 한 프레임 dead 상태만 별도 보존한다.
        if state.get('page')==['main'] and state.get('html_window_state')==['1','0','0']:
            prior=observations[state_index-1] if state_index else {};following=observations[state_index+1] if state_index+1<len(observations) else {}
            prior_page=prior.get('page',[''])[0];roots=state['htmlRoots']
            # 원문 Kill은 idle/overlay 제거→Push의void8→dead를 쓴다. 나머지 비트와 실제 root를 그대로 대조한다.
            valid=len(roots)==1 and bool(roots[0]['flags']&0x10000) and roots==[{'token':item['token'],'flags':(item['flags']|0x10008)&~0x80080} for item in prior.get('htmlRoots',[])]
            valid=valid and prior_page in successors and following.get('page')==[successors.get(prior_page)] and prior.get('frame')==state['frame']-1 and following.get('frame')==state['frame']+1
            if not valid: raise RuntimeError('Credits 원문 한프레임 dead/다음입력 계약 불일치: '+repr(state['frame']))
            transients.append({'frame':state['frame'],'from':prior_page,'to':successors[prior_page],'deadRoot':roots[0]['token']});continue
        continuous.append(state)
    groups=[]
    for state in continuous:  # 확인된 한 프레임 이외의 페이지는 생략/보정하지 않는다.
        page=state.get('page',[''])[0]
        if not groups or groups[-1]['page']!=page: groups.append({'page':page,'first':state,'last':state})
        else: groups[-1]['last']=state
    expected=['main','Credits']+[page['page'] for page in branch['pages']]+['main']
    actual=[group['page'] for group in groups]
    if actual!=expected: raise RuntimeError('Credits 실제 페이지 순서 불일치: '+repr(actual))
    timings=[]
    for page_index,page in enumerate(branch['pages']):  # 입력 없이 바뀐 실제 두 보고 지점의 벽시계 간격을 대조한다.
        first=groups[page_index+2]['first'];next_first=groups[page_index+3]['first'];duration=float(next_first['time'][1])-float(first['time'][1])
        if not page['seconds']-2.5<=duration<=page['seconds']+2.5: raise RuntimeError('Credits 기한 관측 불일치: '+repr((page['page'],duration)))
        if first.get('html_window_state')!=['1','1','1'] or first.get('raw_html_globals',[''])[0]!='1': raise RuntimeError('Credits 실제 단일 HTML 수명 불일치')
        timings.append({'page':page['page'],'nativeSeconds':page['seconds'],'observedSeconds':duration,'firstFrame':first['frame'],'nextFrame':next_first['frame']})
    final=observations[-1]
    if final.get('phase')!=['0'] or final.get('html_window_state')!=['0','0','0'] or final.get('raw_html_globals',[''])[0]!='0': raise RuntimeError('Credits 마지막 자동 닫기/메인 복귀 수명 불일치')
    record.update({'passed':True,'pages':actual,'timings':timings,'transientDeadFrames':transients,'finalHtmlInstances':0,'fullNaturalTimeoutChain':True,'audioExecuted':False,'originalPixelComparison':False})
    save(directory/result_name,record);print(json.dumps({'branch':directory.name,'passed':True,'pages':len(timings),'returncode':record['returncode']}),flush=True)
    return record


def main():
    """원본과 분리한 데이터 사본/새 출력 폴더만 허용하며 두 분류는 순차 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--game-dir',type=Path);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--branch',choices=('all','patch','original'),default='all');parser.add_argument('--analyze',type=Path);args=parser.parse_args()
    if not args.analyze and not args.game_dir: parser.error('실제 실행에는 --game-dir 사본이 필요합니다')
    data=args.game_dir.resolve() if args.game_dir else None;output=args.output.resolve()
    for protected in (ROOT/'originals',ROOT/'originalCD'):  # 허용 설정 외의 게임 쓰기가 보호 폴더에 도달하지 않도록 원시 사본을 요구한다.
        if data is not None and (data==protected or protected in data.parents): parser.error('--game-dir에는 원본과 분리한 사본을 지정해야 합니다')
    output.mkdir(parents=True,exist_ok=False);branches,backgrounds=catalog(ROOT/'originals');save(output/'reference.json',{'branches':branches,'backgroundAvailability':backgrounds})
    results=[]
    for index,branch in enumerate(branches):  # 별도 활성 창/포커스 간섭 없이 실제 한 게임씩 실행한다.
        if args.branch!='all' and args.branch!=('patch' if index==0 else 'original'): continue
        if args.analyze:
            directory=args.analyze.resolve()/('patch' if index==0 else 'original');prior=json.loads((directory/'result.json').read_text(encoding='utf-8'))
            results.append(verify(branch,directory,prior,'revalidated.json'))  # 최초 실패 result/log를 덮어쓰거나 실행 성공으로 재분류하지 않는다.
        else: results.append(run(branch,index,data,output,10))
    report={'runs':results,'gameSha256':digest(GAME),'referenceBackgroundAvailability':backgrounds,'originalGameExecuted':False,'clockManipulated':False,'gameplayExecuted':False,'passed':all(row['passed'] for row in results)}
    save(output/'results.json',report)


if __name__=='__main__':
    main()
