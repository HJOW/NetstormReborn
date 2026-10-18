#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제1-1 메뉴/배치·이동을 검사하며 --completion은 같은 워크샾의 자연 건설·완공까지 확인한다."""
import argparse
import json
import subprocess
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT, EXE
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from cpp_priest_build_menu_smoke import menu_items


def asset_fields(path):
    """현재 실제 슬롯의 반복 상태/extra/HP 저장 비트를 보고 지점과 SID별로 읽는다."""
    result={}
    #일반 파서가 마지막 반복 행을 덮어쓰는 것과 구별해 모든 현재 슬롯을 보존한다.
    for block in path.read_text('utf-8').split('frame\t')[1:]:
        header,body=block.split('\n',1);_,label=header.split('\t',1);result[label]={}
        #새 성공 값을 만들지 않고 실제 World.Report의 같은 슬롯 필드만 해석한다.
        for line in body.splitlines():
            fields=line.split('\t')
            if fields[0]=='raw_asset_fields':result[label][int(fields[1])]=list(map(int,fields[2:]))
    return result


def main():
    """같은 실제 시작 자금/기술로 GUI만 조작하며 원본 보호 자료와 관측 결과를 보존한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--completion',action='store_true');parser.add_argument('--select-workshop',action='store_true');parser.add_argument('--factory-menu',action='store_true');parser.add_argument('--factory-research',action='store_true');args=parser.parse_args()
    if args.factory_research:args.factory_menu=True
    if args.factory_menu:args.select_workshop=True
    if args.select_workshop:args.completion=True
    output=args.output.resolve()
    if output.exists() and any(output.iterdir()):raise RuntimeError('새 관측 경로가 필요합니다: '+str(output))
    output.mkdir(parents=True,exist_ok=True)
    steps=[(5,'click','Play Mission'),(7,'right','world:priest'),(35,'click','Construct'),(65,'click','Workshop'),
           (95,'click',' Build Sun Workshop for 800'),(102,'up','cell:107,103'),(105,'report','prepared'),
           (110,'click','cell:107,103'),(120,'report','placed'),(120,'snapshot',str(output/'placed.bmp')),
           (200,'report','moving'),(200,'snapshot',str(output/'moving.bmp')),(230,'report','costDeleted'),(230,'snapshot',str(output/'cost-deleted.bmp'))]
    if args.completion:
        steps.extend([(500,'report','building'),(500,'snapshot',str(output/'building.bmp')),(950,'report','completed'),(950,'snapshot',str(output/'completed.bmp'))])
    if args.select_workshop:
        steps.extend([(975,'click','world:sunFactory'),(980,'report','selectedWorkshop'),(980,'snapshot',str(output/'selected-workshop.bmp'))])
    if args.factory_menu:
        steps.extend([(990,'right','world:sunFactory'),(1000,'report','factoryOpened'),(1000,'snapshot',str(output/'factory-opened.bmp')),
                      (1005,'click','Put Knowledge into Production'),(1020,'report','factoryKnowledge'),(1020,'snapshot',str(output/'factory-knowledge.bmp')),
                      (1025,'esc',''),(1030,'report','factoryClosed'),(1040,'right','world:sunFactory'),(1055,'click','View Current Production'),
                      (1070,'report','factoryProduction'),(1070,'snapshot',str(output/'factory-production.bmp'))])
    if args.factory_research:
        steps.extend([(1080,'esc',''),(1090,'right','world:sunFactory'),(1100,'click','Put Knowledge into Production'),(1110,'report','researchBefore'),
                      (1115,'click','Sun Cannon'),(1120,'report','researched'),(1120,'snapshot',str(output/'researched.bmp')),
                      (1130,'right','world:sunFactory'),(1140,'click','View Current Production'),(1150,'report','productionAfter'),(1150,'snapshot',str(output/'production-after.bmp')),
                      (1155,'esc',''),(1160,'right','world:sunFactory'),(1170,'click','Put Knowledge into Production'),(1180,'report','knowledgeAfter'),(1180,'snapshot',str(output/'knowledge-after.bmp')),
                      (1185,'esc',''),(1400,'report','flareEnded'),(1400,'snapshot',str(output/'flare-ended.bmp'))])
    script=output/'steps.tsv';report=output/'states.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in steps),encoding='utf-8',newline='\n')
    command=[str(EXE),'--run',str(ROOT/'originals'),'--mission','thewarbegins','--window','--no-audio','--frames','1403' if args.factory_research else '1073' if args.factory_menu else '983' if args.select_workshop else '953' if args.completion else '233',
             '--set','SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8','--ui-script',str(script),'--ui-report',str(report)]
    before=snapshots(ROOT/'originals')
    with preserve_game_settings(ROOT/'originals','d'):
        startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
        result=subprocess.run(command,cwd=ROOT,capture_output=True,startupinfo=startup,timeout=60)
        (output/'stdout.log').write_bytes(result.stdout);(output/'stderr.log').write_bytes(result.stderr)
    unchanged=snapshots(ROOT/'originals')==before
    assert result.returncode==0 and unchanged,dict(exitCode=result.returncode,stderr=result.stderr.decode('utf-8',errors='replace'),originals_unchanged=unchanged)
    observed=states(report);prepared=observed['prepared'];placed=observed['placed'];moving=observed['moving'];deleted=observed['costDeleted']
    assert prepared['raw_sp'][1]=='3000' and prepared['raw_cursor'][4:6]==['109','107'] and prepared['raw_cursor'][8:]==['1','1','1']
    before_packets=list(map(int,prepared['raw_packets']));after_packets=list(map(int,placed['raw_packets']))
    assert after_packets[:len(before_packets)]==before_packets and after_packets[len(before_packets):]==[0x3e]
    #현재 실제 저장소의 SP/선택/커서/워크샾을 검사하며 성공 플래그를 주입하지 않는다.
    for current in (placed,moving,deleted):
        assert current['phase']==['4'] and current['paused']==['0'] and current['popup']==['']
        assert current['raw_input'][:3]==['0','0','14'] and current['raw_sp'][1]=='2200'
        assert current['raw_cursor'][0]=='2' and current['raw_cursor'][3]=='0'
    workshops={sid:row for sid,row in placed['raw_objects'].items() if row[:2]==['85','1'] and row[-2:]==['109','107']}
    assert len(workshops)==1,workshops
    priest=prepared['raw_objects'][15000];moved=moving['raw_objects'][15000]
    assert priest[-2:]==['94','108'] and moved[-2:]!=priest[-2:]
    floating=placed['raw_sp_gump'];later=moving['raw_sp_gump']
    assert floating[:3]==later[:3] and floating[7]==later[7]=='-2' and int(floating[9])!=0
    assert float(later[5])<float(floating[5]) and float(later[6])<float(floating[6])
    assert 'raw_sp_gump' not in deleted and all(current['raw_menus'][0]=='0' for current in (placed,moving,deleted))
    completion={}
    if args.completion:
        fields=asset_fields(report);target=next(iter(workshops));building=observed['building'];completed=observed['completed']
        before_fields=fields['placed'][target];during_fields=fields['building'][target];after_fields=fields['completed'][target]
        assert before_fields[1]&1 and during_fields[1]&1 and not (after_fields[1]&1)
        assert 0<during_fields[2]<after_fields[2]==3000 and not (after_fields[0]&7)
        assert building['raw_sp'][1]==completed['raw_sp'][1]=='2200' and completed['raw_objects'][target][-2:]==['109','107']
        assert completed['phase']==['4'] and completed['paused']==['0'] and completed['popup']==['']
        completion=dict(passed=True,sid=target,placed=before_fields,building=during_fields,completed=after_fields,gameSeconds=[building['time'][0],completed['time'][0]])
    selected=None
    if args.select_workshop:
        current=observed['selectedWorkshop'];target=completion['sid'];raw=asset_fields(report)['selectedWorkshop'][target]
        assert current['raw_input'][0]==str(target) and current['phase']==['4'] and current['popup']==[''] and raw[1]&4
        assert raw[2]==3000 and current['raw_sp'][1]=='2200'
        selected=dict(passed=True,sid=target,raw=raw,input=current['raw_input'])
    factory=None
    if args.factory_menu:
        menus=menu_items(report);target=completion['sid']
        for label in ('factoryOpened','factoryKnowledge','factoryClosed','factoryProduction'):
            current=observed[label];assert current['phase']==['4'] and current['paused']==['0'] and current['popup']==[''] and current['raw_sp'][1]=='2200'
        root=[row for row in menus['factoryOpened'] if row['visible']]
        assert any(row['label']=='Put Knowledge into Production' for row in root) and any(row['label']=='View Current Production' for row in root)
        assert any(row['command']==0x03000032 and row['parameter']==target for row in root)
        knowledge=[row for row in menus['factoryKnowledge'] if row['visible']]
        cannon=next(row for row in knowledge if row['command']==0x03000080 and row['parameter']==((128<<16)|target))
        assert not cannon['flags']&0x200,cannon
        production=[row for row in menus['factoryProduction'] if row['visible']]
        # 실제 TYPE128이 썬 캐논이다. 머리글은 항목 보고에 없으므로 빈 생산 창의 실제 도움말 자식 항목을 확인한다.
        assert any(row['command']==0x02000017 and row['parameter']!=85 for row in production),production
        factory=dict(passed=True,sid=target,root=root,knowledge=knowledge,production=production,scope='원본 메뉴 작성/우클릭 진입/하위 열기만 검증. 연구·업그레이드 명령 실행은 후속이다.')
    research=None
    if args.factory_research:
        target=completion['sid'];before=observed['researchBefore'];after=observed['researched'];ended=observed['flareEnded'];menus=menu_items(report)
        assert list(map(int,after['raw_packets'])).count(0x58)==list(map(int,before['raw_packets'])).count(0x58)+1
        content={}
        # 새 결과를 쓰지 않고 실제 부모/Form SID/kind/flags 관측을 그대로 읽는다.
        for block in report.read_text('utf-8').split('frame\t')[1:]:
            header,body=block.split('\n',1);_,label=header.split('\t',1);content[label]=[]
            # 각 실제 Factory 내용물 행을 별개로 보존한다.
            for line in body.splitlines():
                fields=line.split('\t')
                if fields[0]=='raw_factory_content' and int(fields[1])==target:content[label].append(list(map(int,fields[2:])))
        assert not any(row[1]==128 and row[2]&16 for row in content['researchBefore'])
        learned=[row for row in content['researched'] if row[1]==128 and row[2]&16];assert len(learned)==1 and learned[0][0]>=15000,learned
        initial={sid:row for sid,row in after['raw_objects'].items() if row[0]=='106'};assert len(initial)==12,initial
        assert not any(row[0]=='106' for row in ended['raw_objects'].values())
        assert any(row['visible'] and row['command']==0x02000017 and row['parameter']==128 for row in menus['productionAfter'])
        assert not any(row['visible'] and row['command']==0x03000080 and row['parameter']==((128<<16)|target) for row in menus['knowledgeAfter'])
        assert any(row['visible'] and row['command']==0x03000080 and row['parameter']==((76<<16)|target) and not row['flags']&0x200 for row in menus['knowledgeAfter'])
        assert all(observed[label]['raw_sp'][1]=='2200' and observed[label]['phase']==['4'] and observed[label]['popup']==[''] for label in ('researched','productionAfter','knowledgeAfter','flareEnded'))
        productionContents={}
        # 실제 기술 부모의 생산용 내용물만 관측하며 버튼 상태를 주입하지 않는다.
        for block in report.read_text('utf-8').split('frame\t')[1:]:
            header,body=block.split('\n',1);_,label=header.split('\t',1);productionContents[label]=[]
            # 각각의 현재 Form SID/kind/flags/quality/slot을 그대로 보존한다.
            for line in body.splitlines():
                fields=line.split('\t')
                if fields[0]=='raw_production_content':productionContents[label].append(list(map(int,fields[1:])))
        assert not any(row[1]==128 and row[2]&8 for row in productionContents['researchBefore'])
        teleports=[row for row in productionContents['researched'] if row[1]==128 and row[2]&8];assert len(teleports)==1 and 5<=teleports[0][0]<15000 and teleports[0][3]==3,teleports
        assert [row for row in productionContents['flareEnded'] if row[1]==128 and row[2]&8]==teleports
        research=dict(passed=True,sid=target,authoritative=learned[0],initialFlares=initial,productionContents=productionContents,production=menus['productionAfter'],knowledge=menus['knowledgeAfter'],content=content,scope='실제 GUI 지식 선택/58h 수신/server 내용물/생산 메뉴·지식 제거/플레어12개 및 자연 종료. 생산HUD 배치·전체 미션 완주는 후속.')
    summary=dict(passed=True,scope='실제GUI 사제 메뉴/TYPE85 배치/실제3e왕복/SP 차감/원본 서식 비용 표시·이동·지연삭제/사제 경로 이동.'+(' 같은 워크샾의 HP 자연 증가·3000 완공·abstract 해제까지 확인했다. 미션 완주 검사는 아니다.' if args.completion else ' 자연 건설·완공은 --completion 모드로 검사한다.'),
                 command=command,originals_unchanged=unchanged,newPacketTypes=after_packets[len(before_packets):],workshops=workshops,priestBefore=priest,priestMoving=moved,
                 factoryResearch=research,factoryMenus=factory,selection=selected,completion=completion,observed={label:{key:record[key] for key in ('time','raw_input','raw_cursor','raw_sp','raw_menus','raw_sp_gump') if key in record} for label,record in observed.items()})
    (output/'results.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(passed=True,newPacketTypes=summary['newPacketTypes'],workshops=workshops,priestBefore=priest,priestMoving=moved,completion=completion,selection=selected,factoryMenus=factory,factoryResearch=research,originals_unchanged=unchanged),ensure_ascii=False))


if __name__=='__main__':main()
