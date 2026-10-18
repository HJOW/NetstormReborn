#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제1-1 두 제네레이터 연장/캐논 건설·자동 공격을 단계별 실제 상태 조건으로 검사한다."""
import argparse,json,subprocess
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT,EXE
from cpp_smoke_files import preserve_game_settings
from cpp_renderer_smoke import snapshots
from cpp_world_smoke import states
from cpp_priest_construction_smoke import asset_fields


def steps(output):
    """게임 값을 쓰지 않고 실제 메뉴/생산/마우스 입력과 상태 전제만 예약한다."""
    rows=[(5,'click','Play Mission'),(7,'right','world:priest'),(35,'click','Construct'),(65,'click','Workshop'),(95,'click',' Build Sun Workshop for 800'),(102,'up','cell:107,103'),(105,'require',r'raw_cursor\t0\t1\t85\t'),(110,'click','cell:107,103'),(120,'require',r'raw_sp\t0\t2200\t'),(1050,'report','completedWorkshop'),(1060,'right','world:sunFactory'),(1070,'click','Put Knowledge into Production'),(1080,'click','Rain Generator'),(1100,'report','researchedGenerator'),(1110,'click','production:76'),(1130,'up','cell:99,108'),(1140,'report','preparedGenerator1'),(1149,'require',r'raw_cursor\t0\t1\t76\t'),(1150,'click','cell:99,108'),(1170,'require',r'raw_sp\t0\t1800\t'),(1170,'report','placedGenerator1'),(1950,'require',r'raw_object\t([0-9]+)\t76\t1\t0\t99\t109\nraw_asset_fields\t\1\t0\t128\t700'),(1950,'report','completedGenerator1'),(1960,'click','production:76'),(1980,'up','cell:112,107'),(1985,'report','preparedGenerator2'),(1990,'require',r'raw_cursor\t0\t1\t76\t'),(2000,'click','cell:112,107'),(2010,'require',r'raw_sp\t0\t1400\t'),(2010,'report','placedGenerator2'),(2800,'require',r'raw_object\t([0-9]+)\t76\t1\t0\t112\t108\nraw_asset_fields\t\1\t0\t128\t700'),(2800,'report','completedGenerator2'),(2810,'right','world:sunFactory'),(2820,'click','Put Knowledge into Production'),(2830,'click','Sun Cannon'),(2850,'report','researchedCannon'),(2860,'click','production:128'),(2870,'up','cell:118,116'),(2880,'report','preparedCannon'),(2890,'require',r'raw_cursor\t0\t1\t128\t'),(2900,'click','cell:118,116'),(2910,'require',r'raw_sp\t0\t1000\t'),(2910,'require',r'raw_object\t[0-9]+\t128\t1\t[0-9]+\t118\t118'),(2910,'report','placedCannon'),(2910,'snapshot',str(output/'placed-cannon.bmp'))]
    # 생존/현재 HP/프레임/탄체는 짧은 간격으로 실제 보고만 읽는다.
    for frame in range(3200,5601,10):rows.append((frame,'report',f'combat{frame}'))
    rows.extend([(3500,'snapshot',str(output/'combat.bmp')),(5600,'snapshot',str(output/'after.bmp'))]);rows.sort(key=lambda r:r[0]);return rows


def main():
    """동일 제품/원본 설정을 보존하며 정상 흐름과 명시 거부 조건의 중단을 따로 검사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--guard-reject',action='store_true');args=parser.parse_args();output=args.output.resolve()
    if output.exists() and any(output.iterdir()):raise RuntimeError('새 관측 경로가 필요합니다')
    output.mkdir(parents=True,exist_ok=True)
    rows=[(5,'report','beforeRejectedGuard'),(6,'require','__deliberately_missing_ui_state__'),(7,'report','mustNotRun')] if args.guard_reject else steps(output)
    script=output/'steps.tsv';report=output/'states.tsv';script.write_text(''.join(f'{f}\t{op}\t{arg}\n' for f,op,arg in rows),encoding='utf-8',newline='\n')
    command=[str(EXE),'--run',str(ROOT/'originals'),'--mission','thewarbegins','--window','--no-audio','--frames','10' if args.guard_reject else '5603','--set','SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=0','--ui-script',str(script),'--ui-report',str(report)]
    before=snapshots(ROOT/'originals')
    with preserve_game_settings(ROOT/'originals','d'):
        startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
        result=subprocess.run(command,cwd=ROOT,capture_output=True,startupinfo=startup,timeout=240);(output/'stdout.log').write_bytes(result.stdout);(output/'stderr.log').write_bytes(result.stderr)
    assert snapshots(ROOT/'originals')==before,'원본 자료 변화'
    observed=states(report)
    if args.guard_reject:
        assert result.returncode!=0 and 'UI require'.encode() in result.stderr and 'require6' in observed and 'mustNotRun' not in observed and Path(str(report)+'.failed.bmp').exists()
        summary=dict(passed=True,guardRejected=True,preservedFailureReport=True,originalsUnchanged=True)
    else:
        if result.returncode:
            # 실패해도 보호 검사 결과와 마지막 완성 보고를 남긴다. 전체 전투 통과로 바꾸지 않는다.
            failure=dict(passed=False,originalsUnchanged=True,guardRejected=False,returnCode=result.returncode,lastReport=next(reversed(observed),None),error=result.stderr.decode('utf-8',errors='replace'))
            (output/'summary.json').write_text(json.dumps(failure,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
        assert result.returncode==0,result.stderr.decode('utf-8',errors='replace')
        fields=asset_fields(report);placed=observed['placedCannon'];cannons={sid:row for sid,row in placed['raw_objects'].items() if row[:2]==['128','1']};assert len(cannons)==1,cannons;cannon=next(iter(cannons))
        assert fields['placedCannon'][cannon]==[0,129,0] and placed['raw_sp'][1]=='1000'
        generators={sid:row for sid,row in observed['completedGenerator2']['raw_objects'].items() if row[:2]==['76','1']};assert len(generators)==2 and {tuple(r[-2:]) for r in generators.values()}=={('99','109'),('112','108')}
        assert all(fields['completedGenerator2'][sid]==[0,128,700] for sid in generators)
        enemy={sid:fields['placedCannon'][sid][2] for sid,row in placed['raw_objects'].items() if row[1]=='2' and sid in fields['placedCannon'] and fields['placedCannon'][sid][2]>0}
        frames=set();missiles=[];damage=[];completion=False
        # 현재 슬롯의 자연 완공/그림/탄체/적 HP 변화만 관찰하며 결과를 쓰지 않는다.
        for label,current in observed.items():
            if not label.startswith('combat'):continue
            objects=current['raw_objects'];hp=fields[label]
            if cannon in hp and hp[cannon]==[0,128,600]:completion=True
            if cannon in objects and cannon in hp and not hp[cannon][1]&1:frames.add(int(objects[cannon][2]))
            missiles.extend((label,sid,row) for sid,row in objects.items() if row[:2]==['80','1'])
            # 적 종류를 발사기로 고정하지 않고 실제 피해를 입은 모든 시작 대상을 기록한다.
            for sid,initial in enemy.items():
                if sid not in hp or hp[sid][2]<initial:damage.append(dict(label=label,sid=sid,type=placed['raw_objects'][sid][0],before=initial,after=hp[sid][2] if sid in hp else None))
        summary=dict(passed=completion and bool(frames) and bool(missiles) and bool(damage),originalsUnchanged=True,guardRejected=False,generators=generators,cannon=cannon,completed=completion,frames=sorted(frames),rotationObserved=len(frames)>1,missileObservations=len(missiles),damage=damage[:20],enemyCount=len(enemy),sp=['3000','2200','1800','1400','1000'],scope='실제1-1 자연 건설/두 제네레이터 공급 연장/전방 캐논 완공·자동 공격. 미션 자연 완주 판정은 별도다.')
    (output/'summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(summary,ensure_ascii=False));assert summary['passed'],summary


if __name__=='__main__':main()
