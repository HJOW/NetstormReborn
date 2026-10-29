"""캠페인 완료 표시와 메뉴 잠금을 사본에서 검사한다. 미션 시작/승리는 재현하지 않는다."""
import argparse
import json
import re
import shutil
import socket
import subprocess
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save, sha
from cpp_menu_smoke import source
from cpp_production_panel_smoke import repeated
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from nscfg import decode_file
from taff import TaffArchive

CHAPTERS = ((3, 'Struggle For Freedom'), (4, 'A Nation Rises'), (5, 'Complete Victory'))  # 원본 캠페인16개의 실제 목록이다.


def completed(path):
    """서명을 확인해 복호화하고 첫 정의 우선의 Done 키/정수 값만 반환한다."""
    values = {}
    for match in re.finditer(r'(?im)^[ \t]*(Done\w+)[ \t]*=[ \t]*"?([+-]?\d+)', decode_file(str(path))):  # 사용자 이름/기타 옵션은 출력하지 않는다.
        values.setdefault(match[1].lower(), int(match[2]))
    return values


def catalog(archive):
    """보호 원문 Checked 메타에서 미션/완료 키/선행 키를 독립 추출한다."""
    result = []
    for number, title in CHAPTERS:  # 파일명의 원문 철자 offical을 그대로 사용한다.
        path = f'd/offical{number}.english'
        text = source(ROOT/'originals', archive, path).decode('cp1252')
        rows = re.findall(r'(?im)^\$Checked=([^,\r\n]+),MissionBegin,([^,\r\n]+),\{([^}]+)\},([^\r\n]+)', text)
        for label, mission, done, requirement in rows:  # 제목과 명시 조건을 입력 자료로만 보존한다.
            result.append(dict(chapter=number-2, title=title, label=label, mission=mission, done=done.lower(), requires=requirement.strip().strip('{}').lower()))
    assert len(result) == 16 and result[0]['requires'] == '1'
    assert all(row['requires'] == result[index-1]['done'] for index, row in enumerate(result) if index)
    return result


def plan(directory):
    """실제 메인/캠페인 세 목록을 클릭하고 페이지별 보고/화면을 남긴다."""
    steps = [(3, 'click-if', 'OK'), (6, 'click-if', 'OK'), (12, 'report', 'main'), (18, 'click', 'Campaign'), (28, 'report', 'sections')]
    frame = 36
    for index, (_, title) in enumerate(CHAPTERS, 1):  # 활성 미션을 누르지 않고 목록만 관측한다.
        steps.extend(((frame, 'click', title), (frame+12, 'report', f'chapter{index}'), (frame+12, 'snapshot', str(directory/f'chapter{index}.bmp')), (frame+20, 'click', 'Back')))
        frame += 34
    steps.extend(((frame, 'click', 'Back'), (frame+12, 'report', 'returned'), (frame+18, 'click', 'Quit')))
    return steps, frame+45


def inspect(report, rows, flags):
    """현재 실제 controls의 enabled/checked와 원본 선행 조건을 대조한다."""
    observed = states(report); menus = repeated(report, 'raw_menu_item'); result = []
    for row in rows:  #16개 모두 실제 메뉴 항목의 표시/비활성/체크 비트를 직접 읽는다.
        state = observed[f"chapter{row['chapter']}"]
        matching = [item for item in menus[f"chapter{row['chapter']}"] if item[-1] == row['label'] and item[5] == '1']
        assert len(matching) == 1, (row, matching)
        control = matching[0]; bits = int(control[2])
        active = row['requires'] == '1' or flags.get(row['requires'], 0) != 0
        checked = flags.get(row['done'], 0) != 0
        assert not bool(bits & 0x200) == active and bool(bits & 4) == checked, (row, control, flags)
        assert int(control[3]) == 0x0200008b
        assert 'raw_world' not in state and not state['objects']
        result.append(dict(mission=row['mission'], enabled=active, checked=checked))
    return result


def gui(directory, data, rows, flags):
    """설정 원본은 복구하며 같은 사본의 실제 게임 메뉴만 자동 검사한다."""
    if (directory/'run.json').exists():
        result = json.loads((directory/'run.json').read_text(encoding='utf-8'))
        assert result['returnCode'] == 0 and not result['timedOut'] and result['dataUnchanged'] and result['binarySha256'] == sha(GAME)
        checked = inspect(directory/'states.tsv', rows, flags)
        save(directory/'menu.json', checked)
        return dict(run=result, menu=checked, generatedFlags=completed(directory/'generated-options.cfg'))
    directory.mkdir(); steps, frames = plan(directory)
    script = directory/'steps.tsv'; report = directory/'states.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame, operation, argument in steps), encoding='utf-8', newline='\n')
    command = [str(GAME), '--run', str(data), '--window', '--no-audio', '--frames', str(frames), '--set', 'SCREENW=1024;SCREENH=768;maxFPS=60;sleepPerLoop=8;autoDemo=0;tellTips=0;WelcomeGiven=1;intro=1;music=0', '--ui-script', str(script), '--ui-report', str(report)]
    before = snapshots(data)
    try:
        with preserve_game_settings(data, 'd', directory/'settings-backup') as options:
            result = execute(command, directory, GAME, 90)
            if options.exists():
                shutil.copy2(options, directory/'generated-options.cfg')
        result.update(dataUnchanged=snapshots(data) == before)
        save(directory/'run.json', result)
        assert result['returnCode'] == 0 and not result['timedOut'] and result['dataUnchanged'], result
        checked = inspect(report, rows, flags)
        save(directory/'menu.json', checked)
        return dict(run=result, menu=checked, generatedFlags=completed(directory/'generated-options.cfg'))
    except Exception as error:
        save(directory/'failure.json', dict(error=repr(error)))
        raise


def cli(directory, arguments):
    """창 없는 현재 제품의 설정 읽기/쓰기 명령을 사본에만 실행한다."""
    directory.mkdir()
    result = execute([str(GAME), *map(str, arguments)], directory, GAME, 30)
    save(directory/'run.json', result); assert result['returnCode'] == 0 and not result['timedOut'], result
    return (directory/'stdout.log').read_text(encoding='utf-8-sig')


def main():
    """보호 파일을 먼저 고정하고 새 사본/현재 SHA의 세 메뉴 실행과16단계 저장 왕복을 기록한다."""
    parser = argparse.ArgumentParser(); parser.add_argument('--output', type=Path, required=True); parser.add_argument('--resume-fresh', action='store_true'); args = parser.parse_args()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=args.resume_fresh)
    protected = [ROOT/'AGENTS.md']
    for name in ('originals', 'originalCD', 'extracted/refined', 'cpppj/tests/fixtures'):  # 기존 원본/정밀 자료/fixture 전체의 바이트를 보존한다.
        protected.extend(path for path in (ROOT/name).rglob('*') if path.is_file())
    before = {path.relative_to(ROOT).as_posix(): sha(path) for path in protected}
    if args.resume_fresh:
        assert json.loads((output/'protected-before.json').read_text(encoding='utf-8')) == before
        assert not list(output.glob('save-*')) and not (output/'data/d/options.cfg').exists()
    else:
        save(output/'protected-before.json', before)
    archive = TaffArchive(str(ROOT/'originals/netstorm.tarc')); rows = catalog(archive)
    bundled = completed(ROOT/'originals/d/options.cfg')
    setup = decode_file(str(ROOT/'originals/d/setup.cfg'))
    assert not re.search(r'(?im)^\s*Done\w+\s*=', setup)
    for row in rows:  # 실제 성공 본문의 Config 완료 쓰기가 각 미션에 있는지만 정적으로 확인한다.
        text = source(ROOT/'originals', archive, f"d/{row['mission']}.english").decode('cp1252')
        assert '<$Config,Done{mission.fileName}=1>' in text
    data = output/'data'
    if not args.resume_fresh:
        shutil.copytree(ROOT/'originals', data, ignore=lambda directory, names: [name for name in names if name.lower() in ('options.cfg', 'fullscreenstatefile.dat')])
    fresh = gui(output/'fresh', data, rows, {})
    assert sum(item['enabled'] for item in fresh['menu']) == 1 and not any(item['checked'] for item in fresh['menu'])
    saved = []
    for index, row in enumerate(rows, 1):  # 실제 승리 대신 원문 완료 키를 명시 입력해 저장/새 프로세스 읽기만 확인한다.
        cli(output/f'save-{index:02}', ['--config-save', data, data/'d/options.cfg', row['done']+'=1'])
        text = cli(output/f'read-{index:02}', ['--config-dump', data])
        flags = completed(data/'d/options.cfg')
        assert all(flags.get(prior['done']) == 1 for prior in rows[:index])
        assert all(re.search(r'(?im)^\s*'+re.escape(prior['done'])+r'\s*=\s*"?1', text) for prior in rows[:index])
        saved.append(dict(completed=index, fileSha256=sha(data/'d/options.cfg'), enabled=sum(item['requires'] == '1' or flags.get(item['requires'], 0) != 0 for item in rows)))
        assert saved[-1]['enabled'] == min(index+1, 16)
        shutil.copy2(data/'d/options.cfg', output/f'saved-{index:02}.cfg')
        if index == 1:
            resumed = gui(output/'resumed-first', data, rows, flags)
    shutil.copy2(ROOT/'originals/d/options.cfg', data/'d/options.cfg')
    included = gui(output/'bundled', data, rows, bundled)
    changed = [name for name, wanted in before.items() if sha(ROOT/name) != wanted]
    assert not changed
    report = dict(unit=578, host=socket.gethostname(), lastFullDecompileHost='HJOW-Athlon', lastFullDecompileDate='2026-10-23', binarySha256=sha(GAME), catalog=rows, bundledDone=bundled, fresh=fresh, resumedFirst=resumed, included=included, saveReadbackStages=saved, protected=len(before), protectedChanges=changed, gameRuns=3, missionStarted=False, victoryReproduced=False, networkExecuted=False, productCodeChanged=False, sources={path.relative_to(ROOT).as_posix(): sha(path) for path in (Path(__file__), ROOT/'originals/d/options.cfg', ROOT/'originals/d/setup.cfg', ROOT/'originals/netstorm.tarc', ROOT/'cpppj/src/client/ClientMain.cpp', ROOT/'cpppj/src/client/ClientOptionsFile.cpp', ROOT/'cpppj/src/o/ConfigInterface.cpp', ROOT/'cpppj/package-cpppj.ps1')})
    save(output/'summary.json', report)
    print(json.dumps(dict(protected=len(before), changes=changed, freshEnabled=sum(row['enabled'] for row in fresh['menu']), resumedEnabled=sum(row['enabled'] for row in resumed['menu']), bundledEnabled=sum(row['enabled'] for row in included['menu']), stages=len(saved))))


if __name__ == '__main__':
    main()
