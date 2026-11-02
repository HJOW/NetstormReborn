#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""하위 폴더의명시실행위치에서원문setup탐색으로부모자료/설정/메인메뉴를여는짧은검사다."""
import argparse
from pathlib import Path
import re
import shutil
import socket
from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save, sha
from cpp_deck_reactivation_smoke import protection
from cpp_world_smoke import states
from taff import xor_decode


def main():
    """보호원본을읽어Unicode작업사본을만들고실제클론한수명만관측한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert socket.gethostname().lower() in ('hjow-athlon', 'vm-w11-codex', 'vm-debian-codex')
    out = args.output.resolve()
    assert out.is_relative_to(ROOT/'extracted')
    out.mkdir(parents=True, exist_ok=False)
    before = protection()
    save(out/'protected-before.json', before)
    try:
        install = out/'설치😀/package'
        shutil.copytree(ROOT/'originals', install)
        entry = install/'launcher735'
        entry.mkdir()
        shutil.copyfile(install/'Netstorm.exe', entry/'Netstorm.exe')  # 시작리소스만제공하며setup/TAFF는이하위폴더에두지않는다.
        script, report = out/'steps.tsv', out/'states.tsv'
        script.write_text(f'5\treport\tstart\n25\treport\tmain\n25\tsnapshot-presented\t{out/"main.bmp"}\n', encoding='utf-8', newline='\n')
        command = [str(GAME), '--run', str(entry), '--window', '--no-audio', '--frames', '30',
                   '--set', 'SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;startInFullScreen=0',
                   '--ui-script', str(script), '--ui-report', str(report)]
        run = execute(command, out, GAME, 90)
        save(out/'run.json', run)
        assert run['returnCode'] == 0 and not run['timedOut'], run
        observed = states(report)
        assert observed['main']['page'] == ['main'], observed['main'].get('page')
        assert (out/'main.bmp').is_file()
        plain = xor_decode((install/'d/options.cfg').read_bytes())
        assert plain.startswith(b'mQdsT')
        body = plain[5:]
        text = 'mQdsT'+body.decode('utf-8-sig' if body.startswith(b'\xef\xbb\xbf') else 'cp1252')  # 원문에서죽은첫서명줄을설정키로되살리지않는다.
        values = {}
        for key in ('InstallDir', 'CDDir'):  # 실제옵션저장기가쓴두경로를독립복호화해OS경로로대조한다.
            match = re.search(r'^'+key+r'\s*=\s*"([^"\r\n]*)"', text, re.M|re.I)
            assert match, key
            values[key] = match[1]
            assert Path(match[1]).samefile(install), (key, match[1], install)
        assert not (entry/'d/options.cfg').exists()
        save(out/'summary.json', dict(unit=735, host=socket.gethostname(), productSha256=sha(GAME),
                                     processes=1, frameLimit=30, run=run, mainPage=observed['main']['page'],
                                     savedDirectories=values, setupFoundInParent=True, entryOptionsCreated=False,
                                     physicalInput=False, physicalOutput=False, multiplayerTests=0))
        print('main menu / parent setup and archive / saved InstallDir and CDDir: passed', flush=True)
    finally:
        after = protection()
        save(out/'protected-after.json', after)
        assert before == after, 'AGENTS/원본/CD/fixture 변경'


if __name__ == '__main__':
    main()
