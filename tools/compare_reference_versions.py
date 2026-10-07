#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 복원 기준을 유지하면서 CD/10.62/10.82와 DevLog를 읽기 전용으로 비교한다."""
import argparse
import csv
import hashlib
import json
import re
import sys
from pathlib import Path
from reconstruct_patch1062 import derive
from taff import TaffArchive

# PE 분석 의존성은 기존 Git 제외 설치 경로에서 읽는다.
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT/'extracted/oracle-python'))
import pefile

# 파일명에 의존하지 않고 본체/업데이터/패치 도구를 별도 역할로 기록한다.
BINARIES = {'originalCD': 'originalCD/NETSTORM.EXE', 'original1037': 'original1037/netstorm.exe',
            'original1062': 'extracted/original1062/Netstorm.exe', 'originals': 'originals/Netstorm.exe',
            'original1082': 'original1082/netstorm.game', 'launcher1082': 'original1082/Netstorm.exe',
            'zpatch1062': 'originalPatches/NSP1062/zpatch.exe', 'update37': 'originalPatches/NSP1062/update37.exe',
            'R1082': 'original1082/R.exe'}
# 이번 전체 디컴파일 파일과 완료 로그는 기존 판본 출력과 분리되어 있다.
EXPORTS = {'original1062': ('extracted/original1062/decomp', 'Netstorm.c', 'original1062'),
           'original1082': ('extracted/original1082/decomp', 'netstorm.c', 'original1082'),
           'launcher1082': ('extracted/original1082/launcher/decomp', 'Netstorm.c', 'original1082-launcher'),
           'zpatch1062': ('extracted/patch1062/decomp', 'zpatch.c', 'patch1062')}
# 수동 검토한 정상 경로의 대응 주소다. 전체 자동 매칭/동일 코드 판정으로 확장하지 않는다.
REVIEWED = {'originals': (0x463330, 0x4637b0, 0x462c30, 0x4ad390, 0x462bc0),
            'original1062': (0x460c40, 0x460880, 0x460000, 0x4b4030, 0x45ffa0),
            'original1082': (0x4728c0, 0x472dd0, 0x472150, 0x4cf4b0, 0x4720f0)}
ROLES = ('GraphAllocate', 'GraphDetach', 'PositionGraph', 'SlotGraph', 'SidGraph')
# 새 판본의 구조를 복원 기준으로 채택하지 않도록 필드 폭까지 기록한다.
LAYOUTS = {'originals': dict(sid_stride=50, graph_offset=30, graph_width=1, graph_limit=251, invalid_graph=254),
           'original1062': dict(sid_stride=36, graph_offset=28, graph_width=1, graph_limit=251, invalid_graph=254),
           'original1082': dict(sid_stride=77, graph_offset=42, graph_width=4, graph_limit=50000, invalid_graph=50003)}
REPORT = ROOT/'cpppj/recovery-reference-versions.json'


def sha(data):
    """읽은 바이트의 SHA-256을 계산한다."""
    return hashlib.sha256(data).hexdigest()


def file_sha(relative):
    """보호된 파일/분석 출력에 쓰지 않고 SHA만 읽는다."""
    return sha((ROOT/relative).read_bytes())


def log_text(relative):
    """PowerShell 출력의 UTF-8/UTF-16 BOM을 구별해 완료 메시지를 읽는다."""
    data = (ROOT/relative).read_bytes()
    return data.decode('utf-16' if data.startswith(b'\xff\xfe') else 'utf-8-sig')


def pe_facts(relative):
    """PE 엔트리/섹션/실제 가져오기 라이브러리와 파일 해시를 수집한다."""
    data = (ROOT/relative).read_bytes()
    pe = pefile.PE(data=data)
    return dict(path=relative, sha256=sha(data), size=len(data), image_base=pe.OPTIONAL_HEADER.ImageBase,
                entry_rva=pe.OPTIONAL_HEADER.AddressOfEntryPoint, subsystem=pe.OPTIONAL_HEADER.Subsystem,
                characteristics=pe.FILE_HEADER.Characteristics,
                imports=[e.dll.decode('ascii') for e in pe.DIRECTORY_ENTRY_IMPORT],
                sections=[dict(name=s.Name.rstrip(b'\0').decode('ascii'), rva=s.VirtualAddress,
                               raw_size=s.SizeOfRawData, sha256=sha(s.get_data())) for s in pe.sections])


def archive_facts(relative):
    """모든 TAFF 항목의 경계/길이를 확인하고 복호화된 바이트 해시를 수집한다."""
    archive = TaffArchive(str(ROOT/relative))
    entries = {}
    # 목록 전체를 읽되 원본 또는 추출 디렉터리에는 쓰지 않는다.
    for e in archive.entries:
        assert 0 <= e.offset and archive.data_ofs + e.offset + e.size <= len(archive.raw)
        decoded = archive.read(e)
        assert len(decoded) == e.size and e.name.lower() not in entries
        entries[e.name.lower()] = dict(size=e.size, sha256=sha(decoded))
    return dict(path=relative, sha256=sha(archive.raw), count=len(entries), entries=entries)


def manifest_facts():
    """10.82 설치 목록의 SHA-1/hex 크기를 실제 파일과 비교하며 낡은 목록도 구별한다."""
    base = (ROOT/'original1082').resolve()
    results = {}
    # 전체 배포 목록과 자동 업데이터 목록은 같은 형식이지만 별도 파일이다.
    for name in ('file.list.txt', 'auto.list.txt'):
        matched, missing, mismatch = 0, [], []
        # 각 항목은 읽기 전용이며 루트 밖 경로를 허용하지 않는다.
        for line in (base/name).read_text(encoding='ascii').splitlines():
            if not line.strip():
                continue
            relative, expected_sha, expected_size = line.split('\t')
            path = (base/relative).resolve()
            assert path.is_relative_to(base)
            if not path.is_file():
                missing.append(relative)
                continue
            data = path.read_bytes()
            if hashlib.sha1(data).hexdigest().lower() != expected_sha.lower() or len(data) != int(expected_size, 16):
                mismatch.append(relative)
            else:
                matched += 1
        results[name] = dict(sha256=file_sha('original1082/'+name), matched=matched, missing=missing, mismatch=mismatch)
    return results


def function_facts(edition, directory, c_name, facts_path=None):
    """검토 함수의 C와 불연속 x86 몸체 해시를 따로 기록한다."""
    text = (ROOT/directory/c_name).read_text(encoding='utf-8')
    matches = list(re.finditer(r'^// ==== .* @ ([0-9a-fA-F]+)\s*$', text, re.M))
    bodies = {}
    # 함수 헤더 사이의 C를 범위별로 자른다.
    for i, match in enumerate(matches):
        bodies[int(match[1], 16)] = text[match.start():matches[i+1].start() if i+1<len(matches) else len(text)]
    with (ROOT/(facts_path or directory+'/functions.tsv')).open(encoding='utf-8') as fp:
        facts = {int(r['entry'], 16): r for r in csv.DictReader(fp, delimiter='\t')}
    pe = pefile.PE(str(ROOT/BINARIES[edition]))
    image = pe.get_memory_mapped_image()
    base = pe.OPTIONAL_HEADER.ImageBase
    result = {}
    # 기계어 몸체는 사이의 다른 함수를 포함하지 않고 연결한다.
    for role, address in zip(ROLES, REVIEWED[edition]):
        row, chunks = facts[address], []
        # 함수별 모든 불연속 구간을 순서대로 보존한다.
        for part in row['ranges'].split(';'):
            start, end = (int(v, 16)-base for v in part.split('-'))
            chunks.append(image[start:end+1])
        result[role] = dict(entry=f'{address:08x}', ranges=row['ranges'], machine_sha256=sha(b''.join(chunks)),
                            decompiled_sha256=sha(bodies[address].encode('utf-8')))
    return result


def collect():
    """추가 자료/복원 결과/완료 디컴파일/개발 일지의 서로 다른 근거를 연결한다."""
    outputs, patch = derive()
    # 설치 도구 실행 없이 재계산한 모든 출력 바이트와 저장 결과가 같아야 한다.
    for name, data in outputs.items():
        assert (ROOT/'extracted/original1062'/name).read_bytes() == data
    full = {}
    # Ghidra 종료 코드 외에 내보내기 성공/실패 수와 실제 함수 표를 확인한다.
    for edition, (directory, name, log) in EXPORTS.items():
        completion = re.search(r'디컴파일 완료: 성공 (\d+), 실패 (\d+)', log_text(f'extracted/{log}-headless.log'))
        assert completion and int(completion[2]) == 0
        with (ROOT/directory/'functions.tsv').open(encoding='utf-8') as fp:
            count = sum(1 for _ in csv.DictReader(fp, delimiter='\t'))
        assert count == int(completion[1])
        full[edition] = dict(function_count=count, failed=0,
                             exports={directory+'/'+n: file_sha(directory+'/'+n) for n in (name, 'functions.tsv')})
    reviewed = {}
    # 10.78의 기존 전체 출력은 덮어쓰지 않고 새 판본과 함께 읽는다.
    for edition in REVIEWED:
        directory, name = ('extracted/decomp', 'Netstorm.c') if edition=='originals' else EXPORTS[edition][:2]
        facts_path = 'extracted/graphremove/originals/functions.tsv' if edition=='originals' else directory+'/functions.tsv'
        reviewed[edition] = dict(layout=LAYOUTS[edition], functions=function_facts(edition, directory, name, facts_path),
                                 source_exports={p: file_sha(p) for p in (directory+'/'+name, facts_path)})
        if edition!='originals':
            focused = f'extracted/reference-versions/{edition}'
            assert '생성/Take 디컴파일 완료: 5' in log_text(f'extracted/reference-{edition[-4:]}-export.log')
            reviewed[edition]['focused_exports'] = {focused+'/'+n: file_sha(focused+'/'+n) for n in ('creation.c', 'functions.tsv')}
    archives = {e: archive_facts(p) for e, p in
                [('originalCD', 'originalCD/NETSTORM.TARC'), ('original1062', 'extracted/original1062/netstorm.tarc'),
                 ('originals', 'originals/netstorm.tarc'), ('original1082', 'original1082/netstorm.tarc')]}
    before, after = archives['originals']['entries'], archives['original1082']['entries']
    comparisons = dict(added=sorted(after.keys()-before.keys()), removed=sorted(before.keys()-after.keys()),
                       changed=sorted(k for k in before.keys()&after.keys() if before[k]!=after[k]),
                       identical=sorted(k for k in before.keys()&after.keys() if before[k]==after[k]))
    devlog = (ROOT/'original1082/DevLog.txt').read_text(encoding='cp1252').splitlines()
    headings = [dict(line=i+1, heading=line.strip()) for i, line in enumerate(devlog)
                if re.match(r'^10\.\d', line.strip())]
    claim_lines = {}
    # 개발 일지의 주요 주장 위치를 기록하며 해당 판본 표제를 함께 보존한다.
    for key, needle in [('sid_exhaustion', 'The amount of bridge squids was raised.'),
                        ('bridge_reversion', 'Reverted bridge code to 10.37 style'),
                        ('sun_generator', 'New Unit: Sun Generator'),
                        ('difficulty', 'Campaign Difficulty Modes'), ('font_config', 'fontFaceFamily=Arial')]:
        line = next(i+1 for i, text in enumerate(devlog) if needle in text)
        claim_lines[key] = dict(line=line, version=next(h['heading'] for h in reversed(headings) if h['line']<line))
    keys = ('autoRotateBridges', 'disableAutoRotate', 'fontSize', 'fontFaceFamily',
            'enableFullscreen', 'enableCoopAlpha', 'enableFenceCheck')
    presence = {e: {k: (ROOT/BINARIES[e]).read_bytes().find(k.encode('ascii')) for k in keys}
                for e in ('originals', 'original1082')}
    sun = TaffArchive(str(ROOT/'original1082/netstorm.tarc'))
    description = sun.read(next(e for e in sun.entries if e.name.lower()==r'\d\sunbattery.type'))
    assert b'description="Sun Generator"' in description
    assert r'\d\sunbattery.type' not in before and r'\d\sunbattery.type' in after
    launcher = (ROOT/'extracted/original1082/launcher/decomp/Netstorm.c').read_text(encoding='utf-8')
    assert '_spawnl(1,s___Netstorm_game_0047f100' in launcher
    return dict(schema=1, primary_target='10.78', binaries={e: pe_facts(p) for e, p in BINARIES.items()},
                patch=patch, full_decompile=full, reviewed_graph_functions=reviewed,
                launcher=dict(role='console auto-updater; _spawnl -> ./Netstorm.game',
                              spawn_entries=['0040129e', '004013de'],
                              legacy_version_file=(ROOT/'original1082/netstorm.ver').read_text(encoding='ascii').strip()),
                manifests=manifest_facts(), archives=archives, tarc_1078_to_1082=comparisons,
                devlog=dict(sha256=file_sha('original1082/DevLog.txt'), encoding='Windows-1252', headings=headings, claims=claim_lines,
                            config_ascii_file_offsets=presence,
                            conclusions=['10.78을 1차 복원 기준으로 고정한다; 10.82 본체의 77바이트/DWORD 그래프/50000 한도는 도입하지 않는다',
                                         '10.81 Sun Generator/difficulty 기록은 추가 sunbattery.type와 easy/hard 미션 자료로 뒷받침된다',
                                         '10.81 다리 코드 재변경 기록과 달리 bridge.type 자체는 두 TARC에서 동일하다; 타입 동일성을 코드 동일성으로 확대하지 않는다',
                                         '10.73/74 소진 복구 변경 기록은 10.78 SID 경계/복구 분석의 단서이며 이번에 복구 구현을 완료한 것은 아니다',
                                         '개발 일지의 fontFaceFamily 키는 두 PE ASCII에서 발견되지 않았다; 실제 설정 파서 대조 전 지원을 단정하지 않는다',
                                         '로그의 10.82 V9 표제는 자료에 포함된 기록이며 현재 PE의 V9 동작 전체를 증명하지 않는다']),
                tools={n: file_sha('tools/'+n) for n in ('compare_reference_versions.py', 'reconstruct_patch1062.py')},
                limitations=['원본/SFX/update37/zpatch/batch/업데이터/복원 PE 실행과 네트워크 접속 없음',
                             '전체 함수 내보내기 성공은 함수 원형/동작 전체 복원 완료를 뜻하지 않는다',
                             '10.82 MinGW/PNG 및 게임/10.62 GIF 분석 경고; 본체의 일부 pcode 경고는 남아 수동 검토 필요',
                             '본체/업데이터 시작과 GUI/원본 플레이 비교는 다른 PC 인수인계'])


if __name__ == '__main__':
    # --verify는 기록 전체를 다시 읽기 전용으로 계산해 비교한다.
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    result = collect()
    if args.verify:
        assert json.loads(REPORT.read_text(encoding='utf-8')) == result
        print('10.62/10.82/DevLog 정적 비교 기록 확인 통과')
    else:
        REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
        print({e: d['function_count'] for e, d in result['full_decompile'].items()})
