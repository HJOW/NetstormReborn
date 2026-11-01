#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""메인 메뉴만 열어 구 Not Validated 안내 제외와 검증값0 보존을 확인한다(684)."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from cpp_deck_reactivation_smoke import ROOT, protection
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states


def main():
    """실제 제품 전후 창만 비교하며 원격 접속/멀티플레이/미션을 시작하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--expect-notice',action='store_true')
    args=parser.parse_args()
    out=args.output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    plan,report=out/'steps.tsv',out/'states.tsv'
    plan.write_text('20\treport\tfirst\n60\treport\tfinal\n',encoding='utf-8',newline='\n')
    before=protection()
    (out/'protection-before.json').write_text(json.dumps(before,ensure_ascii=False),encoding='utf-8')
    command=[str(args.exe.resolve()),'--run',str(ROOT/'originals'),'--window','--no-audio','--frames','63',
             '--set','SCREENW=1024;SCREENH=768;maxFPS=40;autoDemo=0;tellTips=0;sleepPerLoop=1;intro=1;WelcomeGiven=1',
             '--ui-script',str(plan),'--ui-report',str(report)]
    with preserve_game_settings(ROOT/'originals','d',out/'settings-backup'):
        run=subprocess.run(command,cwd=ROOT,capture_output=True,timeout=90)
    (out/'stdout.log').write_bytes(run.stdout)
    (out/'stderr.log').write_bytes(run.stderr)
    after=protection()
    (out/'protection-after.json').write_text(json.dumps(after,ensure_ascii=False),encoding='utf-8')
    assert before==after and run.returncode==0,run.returncode
    final=states(report)['final']
    text=report.read_text('utf-8')
    present='Not Validated' in text
    assert present==args.expect_notice,(present,args.expect_notice)
    assert final['phase']==(['1'] if args.expect_notice else ['0']) and final['main_menu_globals'][1]=='0',(final.get('phase'),final.get('main_menu_globals'))
    if not args.expect_notice:
        assert final['current_dialog']==['0'],final['current_dialog']
    result=dict(exit=run.returncode,exeSha256=hashlib.sha256(args.exe.read_bytes()).hexdigest(),expectedNotice=args.expect_notice,notice=present,
                validationValue=int(final['main_menu_globals'][1]),dialog=final['current_dialog'],phase=final['phase'],protectedUnchanged=True,networkProbes=0,
                limits='명시 옵션으로 Welcome/Tips/자동 데모를 끄고 초기 메인 메뉴만 관측했다. 원문 게임/실제 인터넷 검증·DNS/HTTP/MP/미션·포획/희생·공격·승리/물리 입력·PCM 검사가 아니다.')
    (out/'summary.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(result,ensure_ascii=False))


if __name__=='__main__':
    main()
