#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""새 배포 사본의 실제 시나리오 Start에서 randGeysers0/1의 생성과 진행을 확인한다."""
import argparse
import json
import shutil
import zipfile
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT, execute, save, sha
from cpp_world_smoke import states
from cpp_movie_runtime_smoke import verify_presentation
from nscfg import encode_text


def main():
    """외부 원본을 수정하지 않고 ZIP의 새 사본/시험 시나리오만 실제 입력으로 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True);parser.add_argument('--output', type=Path, required=True);args = parser.parse_args()
    output = args.output.resolve();output.mkdir(parents=True, exist_ok=False)
    with zipfile.ZipFile(args.package) as archive:
        archive.extractall(output / '한글 배포본');entries = [entry for entry in archive.infolist() if not entry.is_dir()];assets = 0
        assert len(entries) == 1376 and not any('netstorm_tests' in entry.filename or 'netstorm_help_probe' in entry.filename for entry in entries)
        for entry in entries:  # 시험 자료를 쓰기 전에 ZIP/추출본/원천 자산의 모든 바이트를 대조한다.
            content = archive.read(entry);assert content == (output / '한글 배포본' / entry.filename).read_bytes()
            prefix = 'NetstormReborn-cpppj/game/'
            if entry.filename.startswith(prefix):
                relative = entry.filename[len(prefix):];source_asset = ROOT / ('originalCD' if relative.startswith('MOVIE/') else 'originals') / relative
                assert content == source_asset.read_bytes();assets += 1
        save(output/'package-audit.json',{'entries':len(entries),'gameAssets':assets,'allEntryBytesMatch':True,'allGameAssetsMatchSources':True,'zipSha256':sha(args.package)})
    product = output / '한글 배포본/NetstormReborn-cpppj';data = product / 'game';binary = product / 'Netstorm.exe'
    assert sha(binary) == sha(ROOT / 'cpppj/build/bin/Release/Netstorm.exe')
    source = ROOT / 'extracted/cpp-editor-scenario-590-20261030/unpacked/NetstormReborn-cpppj/game/d/TEST02.fort'
    assert source.is_file();shutil.copyfile(source, data / 'd/TEST02.fort')
    results = []
    for enabled in (0, 1):  # 부모 분기만 다르고 같은 실제 맵/설정/입력/프레임을 사용한다.
        directory = output / f'rand-{enabled}';directory.mkdir()
        scenario = ('[Header]\r\nmissionType="Tutorial"\r\nmissionNumber=1\r\nmyStartMoney=12345\r\nmyTech="all"\r\nmyAllyList="2"\r\n'
                    f'randGeysers={enabled}\r\nmoreGeysers=0\r\ntitle="WORLD GEYSER 609"\r\n'
                    'ai2Name="ScenarioWind609"\r\nai2StartMoney=0\r\nai2Tech="all"\r\nai2AllyList="1"\r\nai2Off=1\r\nai3Off=1\r\n'
                    '[A.]\r\n<h2>WORLD GEYSER 609</h2>\r\n$Button=Go!,DoNothing,0\r\n')
        script = data / 'd/TEST02.english';script.write_bytes(scenario.encode('ascii'));save(directory / 'scenario-input.json', {'fortSha256': sha(source), 'scenario': scenario, 'scenarioSha256': sha(script)})
        (data / 'd/options.cfg').write_bytes(encode_text('\r\nintro=1\r\nWelcomeGiven=1\r\ncurrentLanguage=english\r\ntellTips=0\r\nautoDemo=0\r\n'))
        steps = []
        for frame in range(10, 476, 10):  # 현재 브리핑의 실제 Go!만 선택하며 상태/시계/승패를 쓰지 않는다.
            steps.extend(((frame,'click-if','Go!'),(frame,'report',str(frame))))
        steps.extend(((479,'report','final'),(479,'snapshot',str(directory/'final.bmp')),(479,'snapshot-output',str(directory/'final-output.bmp'))))
        plan = directory / 'steps.tsv';plan.write_text(''.join(f'{frame}\t{op}\t{value}\n' for frame, op, value in steps),encoding='utf-8',newline='\n')
        command = [str(binary),'--run',str(data),'--mission','TEST02','--window','--no-audio','--frames','480','--set','SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=1;autoDemo=0;tellTips=0;currentLanguage=english','--ui-script',str(plan),'--ui-report',str(directory/'states.tsv')]
        run = execute(command,directory,binary,120);save(directory/'run.json',run)
        assert run['returnCode'] == 0 and not run['timedOut'],run
        observed = states(directory/'states.tsv');final = observed['final']
        assert final['phase'] == ['4'] and final['paused'] == ['0'] and final['raw_session'] == ['1','1'],final.get('mission_start')
        assert final['mission'][0].casefold() == 'test02' and final['mission_start'][:2] == ['1','4'] and 'raw_mission_loading' not in final
        assert float(final['raw_sp'][1]) == 12345,final['raw_sp']
        clocks = [float(scene['time'][0]) for scene in observed.values() if scene.get('phase') == ['4'] and scene.get('paused') == ['0']]
        assert len(clocks)>1 and clocks[-1]>clocks[0] and all(after>=before for before,after in zip(clocks,clocks[1:]))
        geysers = {sid:row for sid,row in final['raw_objects'].items() if row[0] == '122'}
        shown = verify_presentation(directory/'final.bmp',directory/'final-output.bmp',final,directory);assert shown['same']
        log = (directory/'stderr.log').read_text(encoding='utf-8',errors='replace')+(directory/'stdout.log').read_text(encoding='utf-8',errors='replace')
        assert log.count('Make Local Geysers') == enabled
        record = {'enabled':enabled,'run':run,'geysers':geysers,'geyserCount':len(geysers),'phase':final['phase'],'mission':final['mission'],'raw_sp':final['raw_sp'],'clockIncrease':clocks[-1]-clocks[0],'presentation':shown}
        save(directory/'summary.json',record);results.append(record)
    assert results[1]['geyserCount']>results[0]['geyserCount'],[(row['enabled'],row['geyserCount']) for row in results]
    save(output/'summary.json',{'unit':609,'zipSha256':sha(args.package),'gameSha256':sha(binary),'fixtureFortSha256':sha(source),'results':results,'scope':'새 ZIP의 별도 시험 시나리오 Start0/1·같은 맵·생성 수 증가/비정지 진행/실제 창RGB. 자연 완주·원본 전체 픽셀/개수 대조·승패/금지 재현/신규MP 아님.'})
    print(json.dumps({'passed':True,'geysers':[row['geyserCount'] for row in results],'gameSha256':sha(binary)}))


if __name__ == '__main__':
    main()
