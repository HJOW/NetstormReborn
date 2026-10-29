"""캠페인1-6을입력없이진행해자연피격/신전파괴와섬의중립·기본테마복귀를확인한다."""
import argparse
import re
import shutil
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT,GAME,save,sha
from cpp_game_menu_smoke import protected_run
from cpp_solo_entry_smoke import BUTTONS
from cpp_production_panel_smoke import repeated
from cpp_world_smoke import states


def inspect(report,expect_neutral):
    """실제현재객체와섬목록/프레임을읽고피해·부재·소유/테마복귀를구별한다."""
    observed=states(report);fields=repeated(report,'raw_asset_fields');initial=observed['100'];kind,first,anchorCount=map(int,initial['raw_terrain_type']);assert first>=0 and anchorCount>=1  #J는대표프레임이며본문144개는원본처럼그다음인덱스에서선택한다.
    temples=[sid for sid,row in initial['raw_objects'].items() if row[0] in ('123','124','125') and row[1]=='1'];assert len(temples)==1;temple=temples[0]
    land=[sid for sid,row in initial['raw_objects'].items() if int(row[0])==kind and row[1]=='1'];core=[sid for sid in land if first+72<int(initial['raw_objects'][sid][2])<=first+108];assert land and core
    islandRows=repeated(report,'raw_island');island=next(int(row[0]) for row in islandRows['100'] if row[1]==str(temple));assert next(row for row in islandRows['100'] if int(row[0])==island)[3]=='1'
    hits=[];death=None
    for label,state in observed.items():  #처음실제부재를관측한시각까지raw HP변화를읽는다.
        if int(label)<100:continue
        if temple not in state['raw_objects']:
            death=label;break
        row=next(row for row in fields[label] if row[0]==str(temple));hits.append((label,int(row[3])))
    assert death and any(hp<hits[0][1] for _,hp in hits);final=observed['2400'];neutral=sum(final['raw_objects'][sid][1]=='0' for sid in land);natural=sum(first<int(final['raw_objects'][sid][2])<=first+36 for sid in core)
    record=next(row for row in islandRows['2400'] if int(row[0])==island);assert record[1]=='0' and record[2]=='1' and record[3]=='0';assert initial['camera']==final['camera']
    if expect_neutral:assert neutral==len(land) and natural==len(core),(neutral,len(land),natural,len(core))
    return {'templeSid':temple,'island':island,'terrainType':kind,'firstAbsentLabel':death,'firstAbsentGameSeconds':float(observed[death]['raw_world'][0]),'hpObservations':hits,'landCells':len(land),'neutralCells':neutral,'initialElementCoreCells':len(core),'restoredNaturalCoreCells':natural,'islandTempleCleared':True,'islandOwnerNeutral':True,'cameraUnchanged':True}


def main():
    """보호원본직접실행을거부하고작업사본에서브리핑확인외플레이입력을넣지않는다."""
    parser=argparse.ArgumentParser();parser.add_argument('--data-root',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();source=args.data_root.resolve();output=args.output.resolve()
    assert source.is_dir() and all(source!=p.resolve() and p.resolve() not in source.parents for p in (ROOT/'originals',ROOT/'originalCD'));output.mkdir(parents=True,exist_ok=False);data=output/'data';shutil.copytree(source,data)
    steps=[]
    for frame in range(12,100,8):  #초기실제브리핑의확인버튼만누른뒤모든플레이입력을중지한다.
        for label in BUTTONS:
            condition=r'phase\t[13]\n&&(?:control\t'+re.escape(label)+r'\t|raw_html_button[^\n]*\t'+re.escape(label)+r'\n)';steps.append((frame,'when',condition+';;click;;'+label))
    for frame in (5,100,*range(200,2401,100)):  #자연전투중상태를관측하고같은카메라의화면을저장한다.
        steps.append((frame,'report',str(frame)))
        if frame in (100,1500,1700,2000,2400):steps.append((frame,'snapshot',str(output/f'frame-{frame}.bmp')))
    steps.sort(key=lambda row:row[0]);report,run=protected_run(output/'run',1024,768,steps,2402,data_root=data,mission='DissolvedAlliance');result=inspect(report,True)
    save(output/'summary.json',{'binarySha256':sha(GAME),'observed':result,'run':run,'playInputAfterBriefing':False,'forcedDamageOrDeletion':False,'capturesSacrificesOrVictoriesReproduced':False,'networkExecuted':False})


if __name__=='__main__':main()
