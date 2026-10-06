#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""새 10.37/CD/패치 PE와 디컴파일 산출물을 읽기 전용으로 비교한다."""
import argparse
import csv
import hashlib
import json
import sys
from pathlib import Path

# PE 의존성과 내보내기는 Git 제외 extracted 안에 준비한다.
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile

# 배포 디렉터리 이름과 실제 버전 파일은 구분한다.
BINARIES = {'original1037': 'original1037/netstorm.exe', 'originalCD': 'originalCD/NETSTORM.EXE',
            'originals': 'originals/Netstorm.exe'}
# 읽기 전용 비교 결과의 저장 경로다.
REPORT = ROOT / 'cpppj/recovery-original1037-evidence.json'


def sha(path):
    """파일 변경 없이 SHA-256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory(folder):
    """이름의 대소문자 차이를 제거하고 모든 파일의 내용 해시를 읽는다."""
    return {p.relative_to(folder).as_posix().lower(): sha(p) for p in sorted(folder.rglob('*')) if p.is_file()}


def collect():
    """원본과 새 내보내기를 읽어 파일/섹션/검토 함수의 실제 동일 범위를 기록한다."""
    binaries = {}
    data = {}
    images = {}
    # 세 원본의 PE 구조·버전 파일·섹션 바이트를 읽는다.
    for edition, relative in BINARIES.items():
        path = ROOT / relative
        data[edition] = path.read_bytes()
        pe = pefile.PE(data=data[edition])
        images[edition] = pe.get_memory_mapped_image()
        version = path.parent / 'netstorm.ver'
        binaries[edition] = dict(path=relative, sha256=sha(path), size=len(data[edition]),
                                version_file=version.read_text(encoding='ascii').strip() if version.is_file() else None,
                                image_base=pe.OPTIONAL_HEADER.ImageBase, entry_rva=pe.OPTIONAL_HEADER.AddressOfEntryPoint,
                                machine=pe.FILE_HEADER.Machine,
                                sections=[dict(name=s.Name.rstrip(b'\0').decode('ascii'), rva=s.VirtualAddress,
                                               raw_offset=s.PointerToRawData, raw_size=s.SizeOfRawData,
                                               sha256=hashlib.sha256(s.get_data()).hexdigest()) for s in pe.sections])
    old, new = data['originalCD'], data['original1037']
    assert len(old) == len(new)
    differences = [(i, a, b) for i, (a, b) in enumerate(zip(old, new)) if a != b]
    assert differences == [(0x3314c, 0x75, 0xeb)]
    packages = {e: inventory(ROOT / e) for e in ('original1037', 'originalCD')}
    common = packages['original1037'].keys() & packages['originalCD'].keys()
    changed = sorted(k for k in common if packages['original1037'][k] != packages['originalCD'][k])
    facts = ROOT / 'extracted/regiongraph/originalCD/functions.tsv'
    functions = []
    with facts.open(encoding='utf-8') as fp:
        # CD 검토 함수와 동일 주소의 새 PE 바이트를 직접 비교한다. 다른 분기가 있는 InsertCD는 제외한다.
        for row in csv.DictReader(fp, delimiter='\t'):
            if int(row['entry'], 16) == 0x433bf0:
                continue
            chunks = []
            # 불연속 몸체 사이의 다른 함수를 해시 범위에 넣지 않는다.
            for part in row['ranges'].split(';'):
                lo, hi = (int(v, 16) - 0x400000 for v in part.split('-'))
                a = images['originalCD'][lo:hi+1]
                b = images['original1037'][lo:hi+1]
                assert a == b
                chunks.append(a)
            functions.append(dict(entry=row['entry'], ranges=row['ranges'], shared_sha256=hashlib.sha256(b''.join(chunks)).hexdigest()))
    exports = ('extracted/original1037/decomp/netstorm.c', 'extracted/original1037/decomp/functions.tsv',
               'extracted/regiongraph/originalCD/creation.c', 'extracted/regiongraph/originalCD/functions.tsv',
               'extracted/regiongraph/original1037/creation.c', 'extracted/regiongraph/original1037/functions.tsv')
    full = (ROOT / exports[1]).read_text(encoding='utf-8').splitlines()
    assert len(full) - 1 == 3711
    return dict(schema=1, binaries=binaries, differences=[dict(file_offset='0x3314c', va='0x433d4c',
                cd_byte='75', new_byte='eb', meaning='JNE → JMP, 둘 다 00433d60 대상; InsertCD 창 생성 구간을 건너뜀')],
                interpretation=['두 디렉터리 netstorm.ver가 10.37; CD 배포판 설명 10.72와 실행 파일 표시는 구분한다',
                                '차이는 InsertCD 안내창 생성 분기 하나이며 모든 CD 검사를 우회한다는 증거는 아님',
                                '그래프/영역 helper 및 데이터 섹션 동일; 구버전 두 사본에서 별도 게임 규칙을 추론하지 않음',
                                '원본 프로세스/게임 진입점 실행 없음. Ghidra GIF 자료 분석 경고가 있었지만 3711 함수 내보내기 성공/실패 0'],
                full_decompile=dict(function_count=3711, failed=0, exports={p: sha(ROOT / p) for p in exports}),
                shared_reviewed_functions=functions,
                packages=dict(counts={e: len(v) for e, v in packages.items()}, common_identical=len(common)-len(changed),
                              changed=changed, only_new=sorted(packages['original1037'].keys()-packages['originalCD'].keys()),
                              only_cd_count=len(packages['originalCD'].keys()-packages['original1037'].keys()),
                              netstorm_tarc_identical=packages['original1037']['netstorm.tarc']==packages['originalCD']['netstorm.tarc']),
                original1037_files=packages['original1037'], tool_sha256=sha(Path(__file__)))


if __name__ == '__main__':
    # 재검증은 같은 읽기 결과와 기록 전체를 비교한다. 원본 파일에는 쓰지 않는다.
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    result = collect()
    if parser.parse_args().verify:
        assert result == json.loads(REPORT.read_text(encoding='utf-8'))
        print('10.37 PE/자료/디컴파일 비교 근거와 전체 원본 SHA 확인')
    else:
        REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
        print('10.37 전체 3711 함수; CD와 실행 파일 1바이트 차이, 자료 236개 동일')
