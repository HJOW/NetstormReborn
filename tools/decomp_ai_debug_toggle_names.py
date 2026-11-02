#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문 ToggleReporting/ToggleDebug의 이름·토큰·FIFO만 독립 관측한다."""
import sys
sys.dont_write_bytecode=True
from pathlib import Path
import argparse,hashlib,itertools,json,socket
ROOT=Path(__file__).resolve().parents[1]  # 저장소의 보호 PE/정밀 함수표를 읽고 결과 경로는 인자로 받는다.
WORK=Path(__file__).resolve().parent  # 같은 tools 안의 원문 이름/FIFO 관측기만 빌린다.
sys.path.insert(0,str(WORK))
from decomp_ai_debug_names import Oracle

def cases():
    """실제이름대소문자/독립인자/첫NUL/미등록alias를기대와분리한다."""
    result=[]
    for name,argument in itertools.product(('ToggleReporting','togglereporting','TOGGLEREPORTING','ToggleDebug','toggledebug','TOGGLEDEBUG'),('0','1','4294967295','section')):  # 정상등록두이름의세표기와네독립인자다.
        result.append(('registered', (name+','+argument+'>').encode('ascii')))
    for name in ('ToggleReporting','ToggleDebug'):  # 모든첫NUL위치를명시BYTE입력으로지정한다.
        n=name.encode('ascii')
        result.extend((('nul-after-command',n+b',1>\0ignored'),('nul-in-number',n+b',4294967295\0tail>'),('nul-after-name',n+b'\0,1>'),('nul-mid-name',n[:6]+b'\0'+n[6:]+b',1>'),('nul-in-string',n+b',section\0tail>')))
    result.append(('nul-first',b'\0ToggleReporting,1>'))
    for name,argument in itertools.product(('AIReporting','aireporting','AIREPORTING','DebugSwitch','debugswitch','DEBUGSWITCH'),('0','1','4294967295','section')):  # 옛21행의이름은교정/삭제하지않고명시미등록alias로도공급한다.
        result.append(('unregistered-alias',(name+','+argument+'>').encode('ascii')))
    for argument in ('0','1','4294967295','section'):  # 실제이름과무관한음성등록도분리한다.
        result.append(('unregistered-missing',('MissingAiDebugCommand,'+argument+'>').encode('ascii')))
    return result

def main():
    """원문표/Parser/FIFO최초관측또는별도반복만저장하며기존기대를덮지않는다."""
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--verify',action='store_true');args=ap.parse_args()
    assert not args.report.exists() and (args.fixture.exists() if args.verify else not args.fixture.exists())
    oracle=Oracle();rows=['# operation source delimiter existed value next queue ring events'];coverage={};successes=0
    for category,source in cases():  # 기대는C++출력이아닌원문실제조회/토큰/큐로부터만읽는다.
        row=oracle.run('Command',source);rows.append('\t'.join(row));coverage[category]=coverage.get(category,0)+1;successes+=row[6]!='-'
    content='\n'.join(rows)+'\n'
    if args.verify:assert args.fixture.read_bytes()==content.encode('utf-8')
    else:args.fixture.write_text(content,encoding='utf-8',newline='\n')
    digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    report=dict(unit=775,kind='toggle-name-followup',host=socket.gethostname(),lastFullDecompilation='HJOW-Athlon / 2026-10-23',inputs=len(rows)-1,observations=len(rows)-1,x87Precision='0x027f (name/token/queue integer path)',coverage=coverage,queued=successes,unregistered=len(rows)-1-successes,nativeInstructions=oracle.instructions,nativeLookup='004d52c0',nativeParser='004c9090',nativeTable='00542b90',actualNameEntries=[{'entry':'00543208','name':'ToggleReporting','codePointer':'00542a34','code':'020000d0'},{'entry':'00543210','name':'ToggleDebug','codePointer':'00542a38','code':'020000d1'}],sha256={str(p):digest(p) for p in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',ROOT/'tools/decomp_ai_debug_names.py',Path(__file__),args.fixture)},repeatVerification=args.verify,gameExecutions=0,multiplayerExecutions=0,cppExecutions=0,builds=0,nlsExecutions=0,limitations=['실제명령표/토큰/숫자변환/문자열고리/FIFO만실행','메뉴픽셀/413d50writer/전략/게임/MP를실행하지않음','할당/반납/TLS/로그는기존명시장치경계','원래21fixture및동결준비본/제품수정0'])
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(inputs=len(rows)-1,queued=successes,fixtureSha256=digest(args.fixture))))

if __name__=='__main__':main()
