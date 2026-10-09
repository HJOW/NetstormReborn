#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.37 일본판(original1037JP) PE·자료·디컴파일 산출물을 CD/10.37과 읽기 전용으로 비교한다.

original1037JP 는 Git 에 커밋하지 않는 PC 로컬 자료(HJOW-Athlon, HJOW-X3D 에만 있음)이다.
따라서 이 도구는 원본 폴더가 없는 PC 에서는 --verify 를 건너뛴다.
"""
import argparse
import csv
import hashlib
import json
import sys
from pathlib import Path

# PE 의존성은 Git 제외 extracted 안에 준비한다.
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile

# 비교 대상 실행 파일 (배포 폴더 이름과 실제 버전 파일은 구분한다).
BINARIES = {'original1037JP': 'original1037JP/NetStorm.exe', 'originalCD': 'originalCD/NETSTORM.EXE',
            'original1037': 'original1037/netstorm.exe'}
# 읽기 전용 비교 결과의 저장 경로다.
REPORT = ROOT / 'cpppj/recovery-original1037jp-evidence.json'
# 일본판 디컴파일 산출물 경로 (Git 제외).
DECOMP = ROOT / 'extracted/original1037JP/decomp'


def sha(path):
    """파일 변경 없이 SHA-256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory(folder):
    """이름의 대소문자 차이를 제거하고 모든 파일의 (크기, 내용 해시)를 읽는다."""
    res = {}
    # 모든 하위 파일을 정렬된 순서로 읽는다.
    for p in sorted(folder.rglob('*')):
        if p.is_file():
            res[p.relative_to(folder).as_posix().lower()] = (p.stat().st_size, sha(p))
    return res


def diff_bytes(a, b):
    """같은 길이의 두 바이트열에서 다른 위치만 (오프셋, a, b) 로 모은다."""
    assert len(a) == len(b)
    return [(i, x, y) for i, (x, y) in enumerate(zip(a, b)) if x != y]


def collect():
    """원본과 디컴파일 산출물을 읽어 JP/CD/10.37 의 동일 범위를 기록한다."""
    binaries = {}
    data = {}
    # 세 실행 파일의 PE 구조·버전 파일·섹션 해시를 읽는다.
    for edition, relative in BINARIES.items():
        path = ROOT / relative
        data[edition] = path.read_bytes()
        pe = pefile.PE(data=data[edition])
        version = path.parent / 'netstorm.ver'
        binaries[edition] = dict(path=relative, sha256=sha(path), size=len(data[edition]),
                                 version_file=version.read_text(encoding='ascii').strip() if version.is_file() else None,
                                 image_base=pe.OPTIONAL_HEADER.ImageBase, entry_rva=pe.OPTIONAL_HEADER.AddressOfEntryPoint,
                                 sections=[dict(name=s.Name.rstrip(b'\0').decode('ascii'), rva=s.VirtualAddress,
                                                raw_size=s.SizeOfRawData, sha256=hashlib.sha256(s.get_data()).hexdigest())
                                           for s in pe.sections])
    # JP 실행 파일과 CD 실행 파일은 바이트 단위로 같고, 10.37 사본과는 한 바이트 다르다.
    jp_vs_cd = diff_bytes(data['original1037JP'], data['originalCD'])
    jp_vs_1037 = diff_bytes(data['original1037JP'], data['original1037'])
    # 폴더 전체 파일 목록을 읽는다 (directx 3천여 개는 요약만 저장한다).
    packages = {e: inventory(ROOT / e) for e in ('original1037JP', 'originalCD', 'original1037')}
    jp = packages['original1037JP']
    comparison = {}
    # 비교 상대 두 폴더에 대해 공통/차이/한쪽에만 있는 파일을 센다.
    for other in ('originalCD', 'original1037'):
        o = packages[other]
        common = jp.keys() & o.keys()
        comparison[other] = dict(
            files=len(o), common=len(common), identical=sum(jp[k][1] == o[k][1] for k in common),
            different=sorted(k for k in common if jp[k][1] != o[k][1]),
            only_jp_non_directx=sorted(k for k in jp.keys() - o.keys() if not k.startswith('directx/')),
            only_jp_directx_count=sum(k.startswith('directx/') for k in jp.keys() - o.keys()),
            only_other_count=len(o.keys() - jp.keys()))
    # directx 하위는 개별 목록 대신 개수·용량·목록 해시만 기록한다.
    dx = {k: v for k, v in jp.items() if k.startswith('directx/')}
    dx_digest = hashlib.sha256('\n'.join(f'{k}\t{v[0]}\t{v[1]}' for k, v in sorted(dx.items())).encode('utf-8')).hexdigest()
    top = {}
    # 최상위 폴더별 파일 수와 용량을 센다.
    for k, (size, _) in jp.items():
        t = k.split('/')[0] if '/' in k else '(root)'
        c = top.setdefault(t, [0, 0])
        c[0] += 1
        c[1] += size
    functions = ROOT / 'extracted/original1037/decomp/functions.tsv'
    decomp = None
    # 디컴파일 결과가 있으면 함수 수와 다른 판본 표와의 일치를 기록한다.
    if (DECOMP / 'functions.tsv').is_file():
        with (DECOMP / 'functions.tsv').open(encoding='utf-8') as fp:
            rows = list(csv.reader(fp, delimiter='\t'))
        decomp = dict(function_count=len(rows) - 1, c_sha256=sha(DECOMP / 'NetStorm.c'),
                      functions_tsv_sha256=sha(DECOMP / 'functions.tsv'),
                      functions_tsv_equals_original1037=functions.is_file() and sha(functions) == sha(DECOMP / 'functions.tsv'))
    return dict(schema=1, storage=dict(git_tracked=False, hosts=['HJOW-Athlon', 'HJOW-X3D'],
                                       note='original1037JP 는 .gitignore 대상이며 두 PC 에만 파일로 보관한다'),
                binaries=binaries,
                exe_vs_originalCD=dict(identical=not jp_vs_cd),
                exe_vs_original1037=dict(differences=[dict(file_offset=hex(i), jp_byte=f'{a:02x}', other_byte=f'{b:02x}')
                                                      for i, a, b in jp_vs_1037]),
                comparison=comparison,
                directx=dict(files=len(dx), bytes=sum(v[0] for v in dx.values()), inventory_sha256=dx_digest),
                top_level={k: dict(files=v[0], bytes=v[1]) for k, v in sorted(top.items())},
                netstorm_tarc_identical_to_original1037=jp['netstorm.tarc'] == packages['original1037']['netstorm.tarc'],
                full_decompile=decomp,
                non_directx_files={k: dict(size=v[0], sha256=v[1]) for k, v in jp.items() if not k.startswith('directx/')},
                tool_sha256=sha(Path(__file__)))


if __name__ == '__main__':
    # 재검증은 같은 읽기 결과와 기록 전체를 비교한다. 원본 파일에는 쓰지 않는다.
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if not (ROOT / 'original1037JP').is_dir():
        print('original1037JP 폴더가 이 PC 에 없어 비교를 건너뜀 (HJOW-Athlon, HJOW-X3D 에만 보관)')
        sys.exit(0)
    result = collect()
    if args.verify:
        assert result == json.loads(REPORT.read_text(encoding='utf-8'))
        print('10.37 일본판 PE/자료/디컴파일 비교 근거 확인')
    else:
        REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
        print('기록:', REPORT)
